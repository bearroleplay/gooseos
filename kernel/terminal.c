#include "terminal.h"
#include "panic.h"
#include "keyboard.h"
#include "fs.h"
#include "libc.h"
#include "goovm.h"
#include "cmos.h"
#include "gooc_simple.h"
#include "account.h"
#define BANNER_HEIGHT 5

static int small_snprintf(char* str, size_t size, const char* format, ...) {
    char* ptr = str;
    const char* fmt = format;
    
    while (*fmt && (ptr - str) < (int)size - 1) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == 's') {
                fmt++;
                char* arg = *((char**)(&format + 1));
                while (*arg && (ptr - str) < (int)size - 1) {
                    *ptr++ = *arg++;
                }
            } else if (*fmt == 'd' || *fmt == 'i') {
                fmt++;
                int arg = *((int*)(&format + 1));
                char num[32];
                itoa(arg, num, 10);
                char* n = num;
                while (*n && (ptr - str) < (int)size - 1) {
                    *ptr++ = *n++;
                }
            } else {
                *ptr++ = '%';
                if (*fmt) *ptr++ = *fmt++;
            }
        } else {
            *ptr++ = *fmt++;
        }
    }
    *ptr = 0;
    return ptr - str;
}

// =============== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ===============
uint32_t cursor_x = 0;              // БЫЛО: static uint32_t cursor_x = 0;
uint32_t cursor_y = BANNER_HEIGHT;  // БЫЛО: static uint32_t cursor_y = BANNER_HEIGHT;
uint32_t tick = 0;                  // БЫЛО: static uint32_t tick = 0;
int mode = MODE_COMMAND;            // БЫЛО: static int mode = MODE_COMMAND;

#define EDITOR_LEFT_X       2
#define EDITOR_RIGHT_X      42
#define EDITOR_WIDTH        38
#define EDITOR_HEIGHT       12        // Фикс: 12 строк редактора
#define OUTPUT_START_Y      6         // Начало: строка 6
#define STATUS_BAR_Y        (OUTPUT_START_Y + EDITOR_HEIGHT + 2)  // = 20


// Буфер ввода команд
static char input_buffer[256];
static uint32_t input_pos = 0;

// Редактор
static char editor_buffer[4096];
static uint32_t editor_pos = 0;
static uint32_t editor_x = 1;
static uint32_t editor_y = 6;
static char current_file[64] = "program.goo";
extern FileSystem fs;

void terminal_scroll(void);
void terminal_start_calculator(void);
// Где-то в начале terminal.c (после include'ов):
void terminal_load_file_into_editor(const char* filename);

// =============== БАЗОВЫЕ ФУНКЦИИ ===============
void terminal_newline(void) {
    cursor_x = 0;
    cursor_y++;
    
    // Если достигли САМОГО НИЗА - прокручиваем
    if (cursor_y >= VGA_HEIGHT) {
        terminal_scroll();
        cursor_y = VGA_HEIGHT - 1;  // Курсор остаётся в последней строке
    }
}




void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_newline();
    } else {
        vga_putchar(c, vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), cursor_x, cursor_y);
        cursor_x++;
        if (cursor_x >= VGA_WIDTH) {
            terminal_newline();
        }
    }
}

void terminal_print(const char* str, uint8_t color) {
    while (*str) {
        if (*str == '\n') {
            terminal_newline();
        } else {
            // Проверяем, не вышли ли за правый край
            if (cursor_x >= VGA_WIDTH) {
                terminal_newline();
            }
            
            vga_putchar(*str, color, cursor_x, cursor_y);
            cursor_x++;
        }
        str++;
    }
}

void terminal_print_at(const char* str, uint32_t x, uint32_t y, uint8_t color) {
    uint32_t old_x = cursor_x;
    uint32_t old_y = cursor_y;
    cursor_x = x;
    cursor_y = y;
    terminal_print(str, color);
    cursor_x = old_x;
    cursor_y = old_y;
}

void draw_banner(void) {
    // Очищаем область баннера
    for (int y = 0; y < BANNER_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_putchar(' ', VGA_COLOR_BLACK, x, y);
        }
    }
    
    // Верхняя граница
    terminal_print_at("========================================", 0, 0, VGA_COLOR_CYAN);
    
    // Логотип слева
    terminal_print_at("            GOOSE OS v 1.0  Beta       ", 0, 1, VGA_COLOR_YELLOW);
    terminal_print_at("   Editor | File System | .goo Runner  ", 0, 2, VGA_COLOR_LIGHT_GRAY);
    
    // Время и дата справа с индикатором языка
    char time_str[16], date_str[16];
    get_time_string(time_str);
    get_date_string(date_str);
    
    // Индикатор раскладки ЛЕВЕЕ времени
    const char* layout_str = (keyboard_get_layout() == 0) ? "EN" : "RU";
    uint8_t layout_color = (keyboard_get_layout() == 0) ? VGA_COLOR_LIGHT_BLUE : VGA_COLOR_LIGHT_GREEN;
    
    // Формат: "[EN] 14:30:25"
    terminal_print_at("[", VGA_WIDTH - 14, 1, VGA_COLOR_LIGHT_GRAY);
    terminal_print_at(layout_str, VGA_WIDTH - 13, 1, layout_color);
    terminal_print_at("]", VGA_WIDTH - 11, 1, VGA_COLOR_LIGHT_GRAY);
    terminal_print_at(time_str, VGA_WIDTH - 8, 1, VGA_COLOR_LIGHT_BLUE);
    
    // Только дата на второй строке
    terminal_print_at(date_str, VGA_WIDTH - 10, 2, VGA_COLOR_LIGHT_GREEN);
    
    // Нижняя граница баннера
    terminal_print_at("========================================", 0, 3, VGA_COLOR_CYAN);
    
    // Статусная строка (5-я строка баннера)
    terminal_print_at("Type 'help' for commands | Alt+Shift switch layout", 0, 4, VGA_COLOR_LIGHT_GRAY);
}

void update_banner_time(void) {
    static uint32_t last_time_update = 0;
    
    if (tick - last_time_update > 50) {
        char time_str[16];
        get_time_string(time_str);
        
        // Очищаем область от индикатора до конца строки
        for (int x = VGA_WIDTH - 14; x < VGA_WIDTH; x++) {
            vga_putchar(' ', VGA_COLOR_BLACK, x, 1);
        }
        
        // Обновляем раскладку и время
        const char* layout_str = (keyboard_get_layout() == 0) ? "EN" : "RU";
        uint8_t layout_color = (keyboard_get_layout() == 0) ? VGA_COLOR_LIGHT_BLUE : VGA_COLOR_LIGHT_GREEN;
        
        // Индикатор ЛЕВЕЕ времени
        terminal_print_at("[", VGA_WIDTH - 14, 1, VGA_COLOR_LIGHT_GRAY);
        terminal_print_at(layout_str, VGA_WIDTH - 13, 1, layout_color);
        terminal_print_at("]", VGA_WIDTH - 11, 1, VGA_COLOR_LIGHT_GRAY);
        terminal_print_at(time_str, VGA_WIDTH - 8, 1, VGA_COLOR_LIGHT_BLUE);
        
        last_time_update = tick;
    }
}

// Обновим terminal_init:
void terminal_init(void) {
    vga_clear();
    
    // Баннер сверху
    draw_banner();
    
    // Промпт внизу
    cursor_x = 0;
    cursor_y = VGA_HEIGHT - 1;
    input_pos = 0;
    mode = MODE_COMMAND;
    
    terminal_show_prompt();
}

// Обновим terminal_update для обновления времени:
void terminal_update(void) {
    tick++;
    
    // Обновляем время в баннере
    if (mode == MODE_COMMAND) {
        update_banner_time();
    }
    
    // Мигающий курсор
    if ((tick / 10) % 2 == 0) { // Чаще мигаем
        // Рисуем курсор
        vga_putchar('_', vga_make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK), cursor_x, cursor_y);
    } else {
        // Восстанавливаем символ под курсором
        // (это сделает следующий вызов terminal_handle_input или terminal_putchar)
    }
}

// Функция очистки с сохранением баннера:
void terminal_clear_with_banner(void) {
    // Очищаем только область под баннером
    for (int y = BANNER_HEIGHT; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_putchar(' ', VGA_COLOR_BLACK, x, y);
        }
    }
    
    // Перерисовываем баннер
    draw_banner();
    
    // Сбрасываем курсор
    cursor_x = 0;
    cursor_y = BANNER_HEIGHT;
    input_pos = 0;
    mode = MODE_COMMAND;
}

// =============== ВВОД КОМАНД ===============
void terminal_scroll(void) {
    // Прокручиваем ВСЕ строки ВВЕРХ, начиная С баннера
    for (int y = BANNER_HEIGHT; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            // Копируем строку y+1 в строку y
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    // Очищаем ПОСЛЕДНЮЮ строку (не предпоследнюю!)
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_putchar(' ', VGA_COLOR_BLACK, x, VGA_HEIGHT - 1);
    }
}

void terminal_show_prompt(void) {
    // Всегда на последней строке
    cursor_x = 0;
    cursor_y = VGA_HEIGHT - 1;
    
    // НЕ очищаем всю строку - только после команды
    // Вместо этого просто печатаем промпт
    
    terminal_print("goose> ", vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
}

void terminal_handle_input(char key) {
    if (mode == MODE_EDITOR) {
        terminal_editor_handle_key(key);
        return;
    }
    
    // Командная строка
    if (key == '\n') {
        input_buffer[input_pos] = 0;
        
        // Скрываем курсор на строке промпта
        vga_putchar(' ', VGA_COLOR_BLACK, cursor_x, cursor_y);
        
        // Переходим на строку ВЫШЕ для вывода результата
        uint32_t save_x = cursor_x;
        uint32_t save_y = cursor_y;
        
        cursor_x = 0;
        cursor_y = VGA_HEIGHT - 3;
        
        // Выполняем команду
        if (input_buffer[0] != 0) {
            terminal_execute_command(input_buffer);
        }
        
        // Сбрасываем позицию курсора
        cursor_x = 0;
        cursor_y = VGA_HEIGHT - 1;
        
        // Очищаем строку ввода (последнюю строку)
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_putchar(' ', VGA_COLOR_BLACK, x, cursor_y);
        }
        
        // Показываем промпт (ОДИН РАЗ!)
        if (mode == MODE_COMMAND) {
            terminal_show_prompt();
        }
        
        input_pos = 0;
    } 
    else if (key == '\b') {
        // Backspace
        if (input_pos > 0) {
            input_pos--;
            if (cursor_x > 7) {
                cursor_x--;
                vga_putchar(' ', VGA_COLOR_BLACK, cursor_x, cursor_y);
            }
        }
    }
    else if (input_pos < 255 && key >= 32 && key <= 126) {
        // Обычный ввод
        input_buffer[input_pos] = key;
        vga_putchar(key, vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), cursor_x, cursor_y);
        input_pos++;
        cursor_x++;
    }
}

// =============== КОМАНДЫ ===============
void terminal_execute_command(const char* cmd) {
    // Пропускаем пробелы в начале
    while(*cmd == ' ') cmd++;
    
    if(strlen(cmd) == 0) return;
    
    // Копируем команду для токенизации
    char cmd_copy[256];
    strcpy(cmd_copy, cmd);
    
    // Разбиваем на токены
    char* tokens[10];
    int token_count = 0;
    
    char* token = strtok(cmd_copy, " ");
    while(token && token_count < 10) {
        tokens[token_count++] = token;
        token = strtok(NULL, " ");
    }
    
    if(token_count == 0) return;
    
    // Переходим на строку ВЫШЕ промпта для вывода результатов
    cursor_x = 0;
    cursor_y = VGA_HEIGHT - 2;
    
    // ========== ОБРАБОТКА КОМАНД ==========
    
    if(strcmp(tokens[0], "help") == 0) {
        int page = 1;
        if(token_count > 1) {
            page = atoi(tokens[1]);
            if(page < 1 || page > 6) page = 1;
        }
        
        switch(page) {
            case 1:
                terminal_print("=== GooseOS Commands (Page 1/6) ===\n", VGA_COLOR_YELLOW);
                terminal_print("help [1-6]      - Show this help (1-6 pages)\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("clear           - Clear screen\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("ls              - List files\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("cd <dir>        - Change directory\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("pwd             - Show current directory\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("mkdir <name>    - Create directory\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("rmdir <name>    - Remove directory\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("rm <name>       - Delete file\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("cat <file>      - View file contents\n", VGA_COLOR_LIGHT_GRAY);
                break;
                
            case 2:
                terminal_print("=== GooseOS Commands (Page 2/6) ===\n", VGA_COLOR_YELLOW);
                terminal_print("edit <file>     - Open text editor\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("rename <o> <n>  - Rename file/directory\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("copy <s> <d>    - Copy file\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("files           - List all files\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("format          - Format filesystem\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("fsinfo          - Filesystem information\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("time            - Show current time\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("date            - Show current date\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("calc            - Open calculator\n", VGA_COLOR_LIGHT_GRAY);
                break;
                
            case 3:
                terminal_print("=== GooseOS Commands (Page 3/6) ===\n", VGA_COLOR_YELLOW);
                terminal_print("compile <.goo>  - Compile GooseScript\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("run <file>      - Run program\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("about           - About GooseScript\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("mouse           - Mouse test program\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("chess           - Chess game (planned)\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("panic [1-4]     - Test kernel panic\n", VGA_COLOR_LIGHT_RED);
                terminal_print("reboot          - Reboot system\n", VGA_COLOR_LIGHT_RED);
                terminal_print("shutdown        - Shutdown system\n", VGA_COLOR_LIGHT_RED);
                break;
                
            case 4:
                terminal_print("=== File System Details ===\n", VGA_COLOR_YELLOW);
                terminal_print("Maximum files: 100\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("Max filename: 56 chars\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("File types: .txt, .goo, .goobin\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("Paths: /, /dir, /dir/subdir\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("Special: ., .., ~ (home)\n", VGA_COLOR_LIGHT_GRAY);
                break;
                
            case 5:
                terminal_print("=== Editor Shortcuts ===\n", VGA_COLOR_YELLOW);
                terminal_print("Ctrl+S          - Save file\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("Ctrl+O          - Open file\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("Ctrl+N          - New file\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("Ctrl+R          - Run .goo file\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("Ctrl+Q          - Exit editor\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("Alt+Shift       - Switch keyboard layout\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("Alt+Q           - Exit apps\n", VGA_COLOR_LIGHT_GRAY);
                break;
                
            case 6:
                terminal_print("=== System Information ===\n", VGA_COLOR_YELLOW);
                terminal_print("GooseOS v1.0 Beta\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("Memory FS: Yes\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("ATA Support: Partial\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("Graphics: VGA 13h\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("Mouse: PS/2\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("Keyboard: EN/RU layout\n", VGA_COLOR_LIGHT_GRAY);
                terminal_print("Terminal: 80x25\n", VGA_COLOR_LIGHT_GRAY);
                break;
        }
        
        terminal_print("\nType 'help <page>' for more (1-6)\n", VGA_COLOR_LIGHT_GRAY);
    }
    else if (strcmp(tokens[0], "beep") == 0) {
    if (token_count >= 3) {
        // beep <частота> <длительность>
        int freq = atoi(tokens[1]);
        int dur = atoi(tokens[2]);
        beep_freq(freq, dur);
        terminal_print(" BEEP! (", VGA_COLOR_GREEN);
        terminal_print(tokens[1], VGA_COLOR_WHITE);
        terminal_print("Hz, ", VGA_COLOR_GREEN);
        terminal_print(tokens[2], VGA_COLOR_WHITE);
        terminal_print("ms)\n", VGA_COLOR_GREEN);
    } else {
        // Просто beep
        beep();
        terminal_print(" BEEP!\n", VGA_COLOR_GREEN);
    }
}
    else if(strcmp(tokens[0], "clear") == 0) {
        terminal_clear_with_banner();
        terminal_show_prompt();
        return;
    }
    else if(strcmp(tokens[0], "ls") == 0) {
        fs_list();
    }
    else if(strcmp(tokens[0], "pwd") == 0) {
        terminal_print(fs_pwd(), VGA_COLOR_CYAN);
        terminal_print("\n", VGA_COLOR_WHITE);
    }
    else if(strcmp(tokens[0], "cd") == 0) {
        if(token_count < 2) {
            terminal_print("Usage: cd <directory>\n", VGA_COLOR_RED);
            terminal_print("Example: cd projects\n", VGA_COLOR_LIGHT_GRAY);
            terminal_print("         cd ..\n", VGA_COLOR_LIGHT_GRAY);
            terminal_print("         cd /\n", VGA_COLOR_LIGHT_GRAY);
        } else {
            if(fs_cd(tokens[1])) {
                terminal_print("Changed to: ", VGA_COLOR_GREEN);
                terminal_print(fs_pwd(), VGA_COLOR_WHITE);
                terminal_print("\n", VGA_COLOR_GREEN);
            } else {
                terminal_print("Directory not found: ", VGA_COLOR_RED);
                terminal_print(tokens[1], VGA_COLOR_WHITE);
                terminal_print("\n", VGA_COLOR_RED);
            }
        }
    }
    else if(strcmp(tokens[0], "mkdir") == 0) {
        if(token_count < 2) {
            terminal_print("Usage: mkdir <directory-name>\n", VGA_COLOR_RED);
        } else {
            if(fs_mkdir(tokens[1])) {
                terminal_print("Directory created: ", VGA_COLOR_GREEN);
                terminal_print(tokens[1], VGA_COLOR_WHITE);
                terminal_print("\n", VGA_COLOR_GREEN);
            } else {
                terminal_print("Failed to create directory\n", VGA_COLOR_RED);
            }
        }
    }
    else if(strcmp(tokens[0], "rmdir") == 0) {
        if(token_count < 2) {
            terminal_print("Usage: rmdir <directory-name>\n", VGA_COLOR_RED);
        } else {
            if(fs_rmdir(tokens[1])) {
                terminal_print("Directory removed: ", VGA_COLOR_GREEN);
                terminal_print(tokens[1], VGA_COLOR_WHITE);
                terminal_print("\n", VGA_COLOR_GREEN);
            } else {
                terminal_print("Failed to remove directory\n", VGA_COLOR_RED);
            }
        }
    }
    else if(strcmp(tokens[0], "rm") == 0) {
        if(token_count < 2) {
            terminal_print("Usage: rm <file-name>\n", VGA_COLOR_RED);
        } else {
            if(fs_delete(tokens[1])) {
                terminal_print("File deleted: ", VGA_COLOR_GREEN);
                terminal_print(tokens[1], VGA_COLOR_WHITE);
                terminal_print("\n", VGA_COLOR_GREEN);
            } else {
                terminal_print("File not found: ", VGA_COLOR_RED);
                terminal_print(tokens[1], VGA_COLOR_WHITE);
                terminal_print("\n", VGA_COLOR_RED);
            }
        }
    }
    else if(strcmp(tokens[0], "cat") == 0) {
        if(token_count < 2) {
            terminal_print("Usage: cat <file-name>\n", VGA_COLOR_RED);
        } else {
            uint8_t buffer[4096];
            int size = fs_read(tokens[1], buffer, sizeof(buffer));
            if(size > 0) {
                buffer[size] = 0;
                terminal_print((char*)buffer, VGA_COLOR_WHITE);
                terminal_print("\n", VGA_COLOR_WHITE);
            } else {
                terminal_print("File not found: ", VGA_COLOR_RED);
                terminal_print(tokens[1], VGA_COLOR_WHITE);
                terminal_print("\n", VGA_COLOR_RED);
            }
        }
    }
    else if(strcmp(tokens[0], "edit") == 0) {
        if(token_count < 2) {
            terminal_print("Usage: edit <file-name>\n", VGA_COLOR_RED);
            terminal_print("Example: edit test.txt\n", VGA_COLOR_LIGHT_GRAY);
            terminal_print("         edit hello.goo\n", VGA_COLOR_LIGHT_GRAY);
        } else {
            terminal_print("Opening: ", VGA_COLOR_CYAN);
            terminal_print(tokens[1], VGA_COLOR_WHITE);
            terminal_print("\n", VGA_COLOR_CYAN);
            
            // Если файл с расширением .goo
            const char* dot = strrchr(tokens[1], '.');
            if(dot && strcmp(dot, ".goo") == 0) {
                terminal_print("(GooseScript file)\n", VGA_COLOR_LIGHT_BLUE);
            }
            
            // Загружаем файл в редактор
            strcpy(current_file, tokens[1]);
            
            int size = fs_editor_load(tokens[1], editor_buffer, sizeof(editor_buffer));
            if(size > 0) {
                editor_pos = size;
                terminal_print("Loaded ", VGA_COLOR_GREEN);
                char size_str[16];
                itoa(size, size_str, 10);
                terminal_print(size_str, VGA_COLOR_WHITE);
                terminal_print(" bytes\n", VGA_COLOR_GREEN);
            } else {
                editor_pos = 0;
                editor_buffer[0] = 0;
                terminal_print("New file\n", VGA_COLOR_YELLOW);
            }
            
            // Запускаем редактор
            terminal_start_editor();
            return; // ВАЖНО: выходим, не показываем промпт
        }
    }
    else if(strcmp(tokens[0], "rename") == 0) {
        if(token_count < 3) {
            terminal_print("Usage: rename <old-name> <new-name>\n", VGA_COLOR_RED);
        } else {
            if(fs_rename(tokens[1], tokens[2])) {
                terminal_print("Renamed: ", VGA_COLOR_GREEN);
                terminal_print(tokens[1], VGA_COLOR_WHITE);
                terminal_print(" -> ", VGA_COLOR_GREEN);
                terminal_print(tokens[2], VGA_COLOR_WHITE);
                terminal_print("\n", VGA_COLOR_GREEN);
            } else {
                terminal_print("Rename failed\n", VGA_COLOR_RED);
            }
        }
    }
    else if(strcmp(tokens[0], "copy") == 0) {
        if(token_count < 3) {
            terminal_print("Usage: copy <source> <destination>\n", VGA_COLOR_RED);
        } else {
            if(fs_copy(tokens[1], tokens[2])) {
                terminal_print("Copied: ", VGA_COLOR_GREEN);
                terminal_print(tokens[1], VGA_COLOR_WHITE);
                terminal_print(" -> ", VGA_COLOR_GREEN);
                terminal_print(tokens[2], VGA_COLOR_WHITE);
                terminal_print("\n", VGA_COLOR_GREEN);
            } else {
                terminal_print("Copy failed\n", VGA_COLOR_RED);
            }
        }
    }
    else if(strcmp(tokens[0], "files") == 0) {
        fs_list();
    }
    else if(strcmp(tokens[0], "format") == 0) {
        terminal_print("WARNING: This will erase ALL data!\n", VGA_COLOR_RED);
        terminal_print("Type 'YES' to confirm: ", VGA_COLOR_YELLOW);
        
        char confirm[10];
        int pos = 0;
        
        while(1) {
            char key = keyboard_getch();
            if(key == '\n') {
                confirm[pos] = 0;
                break;
            } else if(key == '\b' && pos > 0) {
                pos--;
                terminal_print("\b \b", VGA_COLOR_WHITE);
            } else if(key >= 32 && key <= 126 && pos < 9) {
                confirm[pos] = key;
                pos++;
                terminal_putchar(key);
            }
        }
        
        if(strcmp(confirm, "YES") == 0) {
            if(fs_format()) {
                terminal_print("\nDisk formatted successfully\n", VGA_COLOR_GREEN);
            } else {
                terminal_print("\nFormat failed\n", VGA_COLOR_RED);
            }
        } else {
            terminal_print("\nFormat cancelled\n", VGA_COLOR_YELLOW);
        }
    }
    else if(strcmp(tokens[0], "fsinfo") == 0) {
        fs_info();
    }
    else if(strcmp(tokens[0], "time") == 0) {
        char time_str[16];
        get_time_string(time_str);
        terminal_print("Current time: ", VGA_COLOR_CYAN);
        terminal_print(time_str, VGA_COLOR_WHITE);
        terminal_print("\n", VGA_COLOR_WHITE);
    }
    else if(strcmp(tokens[0], "date") == 0) {
        char date_str[16];
        get_date_string(date_str);
        terminal_print("Current date: ", VGA_COLOR_CYAN);
        terminal_print(date_str, VGA_COLOR_WHITE);
        terminal_print("\n", VGA_COLOR_WHITE);
    }
    else if(strcmp(tokens[0], "calc") == 0) {
        terminal_start_calculator();
        return;
    }
    else if(strcmp(tokens[0], "compile") == 0) {
        if(token_count < 2) {
            terminal_print("Usage: compile <file.goo>\n", VGA_COLOR_RED);
        } else {
            // Проверяем расширение
            const char* dot = strrchr(tokens[1], '.');
            if(!dot || strcmp(dot, ".goo") != 0) {
                terminal_print("Error: File must have .goo extension\n", VGA_COLOR_RED);
            } else {
                // Читаем файл
                uint8_t buffer[4096];
                int bytes = fs_read(tokens[1], buffer, sizeof(buffer) - 1);
                
                if(bytes > 0) {
                    buffer[bytes] = 0;
                    
                    // Компилируем
                    uint8_t binary[8192];
                    int bin_size = gooc_compile((char*)buffer, binary, sizeof(binary));
                    
                    if(bin_size > 0) {
                        // Сохраняем .goobin
                        char out_name[64];
                        strcpy(out_name, tokens[1]);
                        char* dot2 = strrchr(out_name, '.');
                        if(dot2) *dot2 = 0;
                        strcat(out_name, ".goobin");
                        
                        if(fs_save_goosebin(tokens[1], out_name, binary, bin_size)) {
                            terminal_print("✓ Compiled to: ", VGA_COLOR_GREEN);
                            terminal_print(out_name, VGA_COLOR_WHITE);
                            terminal_print("\n", VGA_COLOR_GREEN);
                        } else {
                            terminal_print("✗ Failed to save binary\n", VGA_COLOR_RED);
                        }
                    } else {
                        terminal_print("✗ Compilation failed\n", VGA_COLOR_RED);
                    }
                } else {
                    terminal_print("✗ File not found: ", VGA_COLOR_RED);
                    terminal_print(tokens[1], VGA_COLOR_WHITE);
                    terminal_print("\n", VGA_COLOR_RED);
                }
            }
        }
    }
    else if(strcmp(tokens[0], "run") == 0) {
        if(token_count < 2) {
            terminal_print("Usage: run <file.goo or file.goobin>\n", VGA_COLOR_RED);
        } else {
            // Проверяем расширение
            const char* dot = strrchr(tokens[1], '.');
            if(dot) {
                if(strcmp(dot, ".goo") == 0) {
                    // Читаем и компилируем .goo
                    uint8_t buffer[4096];
                    int bytes = fs_read(tokens[1], buffer, sizeof(buffer));
                    
                    if(bytes > 0) {
                        uint8_t binary[8192];
                        int bin_size = gooc_compile((char*)buffer, binary, sizeof(binary));
                        
                        if(bin_size > 0) {
                            GooVM* vm = goovm_create();
                            if(vm) {
                                goovm_load(vm, binary, bin_size);
                                goovm_execute(vm);
                                goovm_destroy(vm);
                            }
                        }
                    }
                } else if(strcmp(dot, ".goobin") == 0) {
                    // Читаем .goobin
                    uint8_t binary[8192];
                    int bin_size = fs_load_goosebin(tokens[1], binary, sizeof(binary));
                    
                    if(bin_size > 0) {
                        GooVM* vm = goovm_create();
                        if(vm) {
                            goovm_load(vm, binary, bin_size);
                            goovm_execute(vm);
                            goovm_destroy(vm);
                        }
                    }
                }
            }
        }
    }
    else if(strcmp(tokens[0], "about") == 0) {
        terminal_print("=== GooseOS v1.0 ===\n", VGA_COLOR_YELLOW);
        terminal_print("A simple OS with its own scripting language!\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("\nGooseScript Features:\n", VGA_COLOR_CYAN);
        terminal_print("• print \"text\" - Print strings\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("• push number - Push to stack\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("• add/sub/mul/div - Math operations\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("• syscall num - System calls\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("• exit code   - Exit program\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("\n", VGA_COLOR_WHITE);
    }
    else if(strcmp(tokens[0], "mouse") == 0) {
        terminal_print("Starting mouse test...\n", VGA_COLOR_CYAN);
        mouse_test_program();
        terminal_clear_with_banner();
        terminal_show_prompt();
        return;
    }
    else if(strcmp(tokens[0], "chess") == 0) {
        terminal_print("Not implemented yet\n", VGA_COLOR_YELLOW);
    }
    else if(strcmp(tokens[0], "panic") == 0) {
        if(token_count < 2) {
            terminal_print("Usage: panic <1-4>\n", VGA_COLOR_RED);
            terminal_print("  1 - Blue screen (kernel)\n", VGA_COLOR_LIGHT_BLUE);
            terminal_print("  2 - Red screen (memory)\n", VGA_COLOR_LIGHT_RED);
            terminal_print("  3 - White screen (hardware)\n", VGA_COLOR_WHITE);
            terminal_print("  4 - Black screen (generic)\n", VGA_COLOR_LIGHT_GRAY);
            return;
        }
        
        char choice = tokens[1][0];
        
        switch(choice) {
            case '1':
                panic_kernel("User triggered kernel panic\n"
                             "Reason: Manual test\n"
                             "System halted for safety", 0x12345678);
                break;
                
            case '2':
                panic_memory("Memory corruption detected\n"
                             "Heap integrity check failed\n"
                             "Possible buffer overflow", 0xDEADBEEF);
                break;
                
            case '3':
                panic_hardware("Hardware failure\n"
                               "ATA disk controller timeout\n"
                               "Sector read failed", 0xBADF00D);
                break;
                
            case '4':
                panic_show(PANIC_BLACK, 
                           "System panic - Unknown error\n"
                           "Kernel in unrecoverable state\n"
                           "Manual intervention required", 
                           0xFFFFFFFF);
                break;
                
            default:
                terminal_print("Invalid choice. Use 1-4\n", VGA_COLOR_RED);
                break;
        }
    }
    else if (strcmp(tokens[0], "setuser") == 0) {
    account_show_user_manager();
}
else if (strcmp(tokens[0], "users") == 0) {
    // Быстрый просмотр пользователей
    terminal_print("\n=== USER LIST ===\n", VGA_COLOR_CYAN);
    for (int i = 0; i < acct_sys.account_count; i++) {
        char line[64];
        const char* type = "USER";
        if (acct_sys.accounts[i].account_type == ACCOUNT_ADMIN) type = "ADMIN";
        if (acct_sys.accounts[i].account_type == ACCOUNT_GUEST) type = "GUEST";
        
        char num[4];
        itoa(i + 1, num, 10);
        terminal_print("  ", VGA_COLOR_WHITE);
        terminal_print(num, VGA_COLOR_YELLOW);
        terminal_print(". ", VGA_COLOR_WHITE);
        terminal_print(acct_sys.accounts[i].username, VGA_COLOR_WHITE);
        terminal_print(" [", VGA_COLOR_LIGHT_GRAY);
        
        if (acct_sys.accounts[i].account_type == ACCOUNT_ADMIN)
            terminal_print(type, VGA_COLOR_RED);
        else if (acct_sys.accounts[i].account_type == ACCOUNT_GUEST)
            terminal_print(type, VGA_COLOR_LIGHT_BLUE);
        else
            terminal_print(type, VGA_COLOR_GREEN);
        
        terminal_print("]\n", VGA_COLOR_LIGHT_GRAY);
    }
}
else if (strcmp(tokens[0], "whoami") == 0) {
    if (acct_sys.logged_in && acct_sys.current_account >= 0) {
        terminal_print("Current user: ", VGA_COLOR_CYAN);
        terminal_print(acct_sys.accounts[acct_sys.current_account].username, VGA_COLOR_WHITE);
        
        // Показываем тип
        if (acct_sys.accounts[acct_sys.current_account].account_type == ACCOUNT_ADMIN)
            terminal_print(" (admin)", VGA_COLOR_RED);
        else if (acct_sys.accounts[acct_sys.current_account].account_type == ACCOUNT_GUEST)
            terminal_print(" (guest)", VGA_COLOR_LIGHT_BLUE);
        else
            terminal_print(" (user)", VGA_COLOR_GREEN);
        
        terminal_print("\n", VGA_COLOR_WHITE);
    } else {
        terminal_print("Not logged in\n", VGA_COLOR_RED);
    }
}
else if (strcmp(tokens[0], "passwd") == 0) {
    if (!acct_sys.logged_in) {
        terminal_print("Not logged in!\n", VGA_COLOR_RED);
        return;
    }
    
    terminal_print("Change password for: ", VGA_COLOR_YELLOW);
    terminal_print(acct_sys.accounts[acct_sys.current_account].username, VGA_COLOR_WHITE);
    terminal_print("\n", VGA_COLOR_WHITE);
    
    terminal_print("Current password: ", VGA_COLOR_GREEN);
    char old_pass[MAX_PASSWORD] = {0};
    int p_pos = 0;
    while (1) {
        char key = keyboard_getch();
        if (key == '\n') break;
        if (key == '\b' && p_pos > 0) {
            p_pos--;
            terminal_print("\b \b", VGA_COLOR_WHITE);
        } else if (p_pos < MAX_PASSWORD-1 && key >= 32 && key <= 126) {
            old_pass[p_pos++] = key;
            terminal_print("*", VGA_COLOR_WHITE);
        }
    }
    old_pass[p_pos] = 0;
    
    terminal_print("\nNew password: ", VGA_COLOR_GREEN);
    char new_pass[MAX_PASSWORD] = {0};
    p_pos = 0;
    while (1) {
        char key = keyboard_getch();
        if (key == '\n') break;
        if (key == '\b' && p_pos > 0) {
            p_pos--;
            terminal_print("\b \b", VGA_COLOR_WHITE);
        } else if (p_pos < MAX_PASSWORD-1 && key >= 32 && key <= 126) {
            new_pass[p_pos++] = key;
            terminal_print("*", VGA_COLOR_WHITE);
        }
    }
    new_pass[p_pos] = 0;
    
    if (account_change_password(acct_sys.accounts[acct_sys.current_account].username, old_pass, new_pass)) {
        terminal_print("\nPassword changed!\n", VGA_COLOR_GREEN);
    } else {
        terminal_print("\nWrong password!\n", VGA_COLOR_RED);
    }
}
else if (strcmp(tokens[0], "logout") == 0) {
    account_logout();
    terminal_print("Logged out\n", VGA_COLOR_YELLOW);
    terminal_clear_with_banner();
    account_show_login_manager();
    terminal_clear_with_banner();
    terminal_show_prompt();
}
    else if(strcmp(tokens[0], "reboot") == 0) {
        terminal_print("Rebooting system...\n", VGA_COLOR_RED);
        terminal_print("Press any key to cancel (3 seconds)...\n", VGA_COLOR_YELLOW);
        
        uint32_t timeout = 300;
        int cancelled = 0;
        
        while(timeout-- > 0) {
            char key = keyboard_getch();
            if(key) {
                terminal_print("\nReboot cancelled!\n", VGA_COLOR_GREEN);
                cancelled = 1;
                break;
            }
            for(volatile int i = 0; i < 10000; i++);
        }
        
        if(!cancelled) {
            terminal_print("\nBye! Restarting...\n", VGA_COLOR_RED);
            for(volatile int i = 0; i < 1000000; i++);
            __asm__ volatile("outb %0, $0x64" : : "a"((uint8_t)0xFE));
            while(1);
        }
    }
    else if(strcmp(tokens[0], "shutdown") == 0) {
        terminal_print("Shutting down GooseOS...\n", VGA_COLOR_RED);
        terminal_print("Press any key to cancel (3 seconds)...\n", VGA_COLOR_YELLOW);
        
        uint32_t timeout = 300;
        int cancelled = 0;
        
        while(timeout-- > 0) {
            char key = keyboard_getch();
            if(key) {
                terminal_print("\nShutdown cancelled!\n", VGA_COLOR_GREEN);
                cancelled = 1;
                break;
            }
            for(volatile int i = 0; i < 10000; i++);
        }
        
        if(!cancelled) {
            terminal_print("\nGoodbye! Turning off...\n", VGA_COLOR_RED);
            
            for(int i = 0; i < 5; i++) {
                terminal_print(".", VGA_COLOR_RED);
                for(volatile int j = 0; j < 200000; j++);
            }
            terminal_print("\n", VGA_COLOR_RED);
            
            // Пытаемся выключить
            __asm__ volatile(
                "mov $0x604, %%dx\n\t"
                "mov $0x2000, %%ax\n\t"
                "out %%ax, %%dx"
                : : : "ax", "dx"
            );
            
            terminal_print("Shutdown not supported in this environment.\n", VGA_COLOR_RED);
            terminal_print("Use 'reboot' instead or close the emulator.\n", VGA_COLOR_YELLOW);
        }
    }
    else {
        terminal_print("Unknown command: ", VGA_COLOR_RED);
        terminal_print(tokens[0], VGA_COLOR_WHITE);
        terminal_print("\nType 'help' for commands\n", VGA_COLOR_RED);
    }
    
    // Показываем промпт ВСЕГДА после выполнения команды (кроме редактора)
    if(mode == MODE_COMMAND) {
        terminal_show_prompt();
    }
}

// =============== РЕДАКТОР ===============
void update_editor_status(void) {
    if (mode != MODE_EDITOR) return;
    
    char status[80];
    int line = 1;
    int col = 1;
    
    // Подсчет строки
    for (uint32_t i = 0; i < editor_pos; i++) {
        if (editor_buffer[i] == '\n') line++;
        if (i == editor_pos - 1) {
            col = (editor_x - EDITOR_LEFT_X) + 1;
        }
    }
    
    small_snprintf(status, sizeof(status), "File: %s | Line: %d | Col: %d | Bytes: %d", 
                  current_file, line, col, editor_pos);
    terminal_print_at(status, 2, STATUS_BAR_Y, VGA_COLOR_WHITE);
    
    // Очищаем старую инфу в правом верхнем углу
    terminal_print_at("                ", 43, OUTPUT_START_Y, VGA_COLOR_BLACK);
}


// Обновление статус-бара
void update_status_bar(void) {
    // Очищаем строку статус-бара
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_putchar(' ', VGA_COLOR_BLACK, x, VGA_HEIGHT - 1);
    }
    
    // Показываем информацию о файле
    char status[80];
    strcpy(status, "File: ");
    strcat(status, current_file);
    strcat(status, " | ");
    
    // Показываем текущий путь ФС
    strcat(status, "Path: ");
    strcat(status, fs.current_path);
    
    // Обрезаем если слишком длинный
    if (strlen(status) > 78) status[78] = 0;
    
    terminal_print_at(status, 1, VGA_HEIGHT - 1, VGA_COLOR_LIGHT_GRAY);
    
    // Показываем хоткеи справа
    terminal_print_at("Ctrl+S=Save | Ctrl+R=Run | Ctrl+Q=Exit", 
                      VGA_WIDTH - 40, VGA_HEIGHT - 1, VGA_COLOR_DARK_GRAY);
}
// =============== РЕДАКТОР С РАЗДЕЛЕННЫМ ЭКРАНОМ ===============

#define EDITOR_LEFT_X       2
#define EDITOR_RIGHT_X      42
#define EDITOR_WIDTH        38
#define EDITOR_HEIGHT       12        // Фикс: 12 строк редактора
#define OUTPUT_START_Y      6         // Начало: строка 6
#define STATUS_BAR_Y        (OUTPUT_START_Y + EDITOR_HEIGHT + 2)  // = 20      // Начало редактора

// Рисование рамки из + и -
static void draw_simple_box(int x, int y, int w, int h, uint8_t color, const char* title) {
    // Верх
    vga_putchar('+', color, x, y);
    for (int i = 1; i < w-1; i++) vga_putchar('-', color, x+i, y);
    vga_putchar('+', color, x+w-1, y);
    
    // Бока
    for (int i = 1; i < h-1; i++) {
        vga_putchar('+', color, x, y+i);
        vga_putchar('+', color, x+w-1, y+i);
    }
    
    // Низ
    vga_putchar('+', color, x, y+h-1);
    for (int i = 1; i < w-1; i++) vga_putchar('-', color, x+i, y+h-1);
    vga_putchar('+', color, x+w-1, y+h-1);
    
    // Заголовок
    if (title) {
        int tx = x + (w - strlen(title))/2;
        for (int i = 0; title[i]; i++) vga_putchar(title[i], color, tx+i, y);
    }
}

static void clear_output_panel(void) {
    for (int y = OUTPUT_START_Y+1; y < OUTPUT_START_Y + EDITOR_HEIGHT; y++) {
        for (int x = EDITOR_RIGHT_X+1; x < EDITOR_RIGHT_X + EDITOR_WIDTH-1; x++) {
            vga_putchar(' ', VGA_COLOR_BLACK, x, y);
        }
    }
}
static void print_to_output(const char* str, uint8_t color) {
    static int out_x = EDITOR_RIGHT_X+1;
    static int out_y = OUTPUT_START_Y+1;
    
    while (*str) {
        if (*str == '\n' || out_x >= EDITOR_RIGHT_X + EDITOR_WIDTH - 2) {
            out_x = EDITOR_RIGHT_X+1;
            out_y++;
            
            // ПРОВЕРКА ГРАНИЦ - не выходим за рамку!
            if (out_y >= OUTPUT_START_Y + EDITOR_HEIGHT) {
                // Прокрутка - двигаем все строки вверх
                for (int y = OUTPUT_START_Y+1; y < OUTPUT_START_Y + EDITOR_HEIGHT - 1; y++) {
                    for (int x = EDITOR_RIGHT_X+1; x < EDITOR_RIGHT_X + EDITOR_WIDTH - 1; x++) {
                        vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y+1) * VGA_WIDTH + x];
                    }
                }
                // Очищаем последнюю строку
                for (int x = EDITOR_RIGHT_X+1; x < EDITOR_RIGHT_X + EDITOR_WIDTH - 1; x++) {
                    vga_putchar(' ', VGA_COLOR_BLACK, x, OUTPUT_START_Y + EDITOR_HEIGHT - 1);
                }
                out_y = OUTPUT_START_Y + EDITOR_HEIGHT - 1;
            }
        }
        
        if (*str != '\n') {
            vga_putchar(*str, color, out_x, out_y);
            out_x++;
        }
        str++;
    }
}

void terminal_start_editor(void) {
    mode = MODE_EDITOR;
    vga_clear();
    
    // === ЛЕВАЯ ПАНЕЛЬ - РЕДАКТОР ===
    draw_simple_box(0, OUTPUT_START_Y-1, 41, EDITOR_HEIGHT+2, VGA_COLOR_CYAN, " GOOSE EDITOR ");
    terminal_print_at("Ctrl+S:Save  Ctrl+R:Run  Ctrl+Q:Exit", 2, OUTPUT_START_Y, VGA_COLOR_LIGHT_GRAY);
    
    // === ПРАВАЯ ПАНЕЛЬ - ВЫВОД ===
    draw_simple_box(41, OUTPUT_START_Y-1, 39, EDITOR_HEIGHT+2, VGA_COLOR_MAGENTA, " GOOSE VM OUTPUT ");
    terminal_print_at("Ready", 43, OUTPUT_START_Y, VGA_COLOR_GREEN);
    
    // === НИЖНЯЯ ПАНЕЛЬ - СТАТУС ===
    draw_simple_box(0, STATUS_BAR_Y-1, 80, 3, VGA_COLOR_LIGHT_GRAY, " STATUS ");
    
    // Очищаем области редактора и вывода
    for (int y = OUTPUT_START_Y+1; y < OUTPUT_START_Y + EDITOR_HEIGHT; y++) {
        for (int x = EDITOR_LEFT_X; x < EDITOR_LEFT_X + EDITOR_WIDTH; x++) {
            vga_putchar(' ', VGA_COLOR_BLACK, x, y);
        }
        for (int x = EDITOR_RIGHT_X+1; x < 79; x++) {
            vga_putchar(' ', VGA_COLOR_BLACK, x, y);
        }
    }
    
    // Инициализация редактора
    editor_x = EDITOR_LEFT_X;
    editor_y = OUTPUT_START_Y+1;
    cursor_x = editor_x;
    cursor_y = editor_y;
    
    // Загружаем файл
    char status[80];
    if (fs_exists(current_file)) {
        int size = fs_editor_load(current_file, editor_buffer, sizeof(editor_buffer));
        if (size > 0) {
            editor_pos = size;
            editor_buffer[size] = 0;
            terminal_redraw_editor_text();
            small_snprintf(status, sizeof(status), "Loaded: %s (%d bytes)", current_file, size);
        } else {
            strcpy(status, "New file");
        }
    } else {
        editor_pos = 0;
        editor_buffer[0] = 0;
        small_snprintf(status, sizeof(status), "New file: %s", current_file);
    }
    terminal_print_at(status, 2, STATUS_BAR_Y, VGA_COLOR_WHITE);
    
    // Обновляем статус-бар
    update_editor_status();
}

// ДОБАВЛЯЕМ обработку Ctrl+R в terminal_editor_handle_key
void terminal_editor_handle_key(char key) {
    // Ctrl+Q - выход
    if (keyboard_get_ctrl() && (key == 'q' || key == 'Q')) {
        mode = MODE_COMMAND;
        terminal_clear_with_banner();
        terminal_show_prompt();
        return;
    }
    
    // Ctrl+S - сохранить
    if (keyboard_get_ctrl() && (key == 's' || key == 'S')) {
        if (fs_editor_save(current_file, editor_buffer, editor_pos)) {
            terminal_print_at("✓ File saved", 43, OUTPUT_START_Y, VGA_COLOR_GREEN);
        }
        update_editor_status();
        return;
    }
    
    // Ctrl+R - ЗАПУСТИТЬ
    if (keyboard_get_ctrl() && (key == 'r' || key == 'R')) {
        terminal_run_goo();
        return;
    }
    
    // Enter
    if (key == '\n') {
        if (editor_pos < sizeof(editor_buffer) - 1) {
            editor_buffer[editor_pos++] = '\n';
            editor_x = EDITOR_LEFT_X;
            editor_y++;
            
            if (editor_y >= OUTPUT_START_Y + EDITOR_HEIGHT) {
                // Прокрутка
                for (int y = OUTPUT_START_Y+1; y < OUTPUT_START_Y+EDITOR_HEIGHT-1; y++) {
                    for (int x = EDITOR_LEFT_X; x < EDITOR_LEFT_X+EDITOR_WIDTH; x++) {
                        vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y+1) * VGA_WIDTH + x];
                    }
                }
                editor_y = OUTPUT_START_Y + EDITOR_HEIGHT - 1;
            }
            
            cursor_x = editor_x;
            cursor_y = editor_y;
            update_editor_status();
        }
    }
    // Backspace
    else if (key == '\b') {
        if (editor_pos > 0) {
            editor_pos--;
            
            if (editor_buffer[editor_pos] == '\n') {
                editor_y--;
                editor_x = EDITOR_LEFT_X + EDITOR_WIDTH - 1;
            } else {
                editor_x--;
            }
            
            vga_putchar(' ', VGA_COLOR_BLACK, editor_x, editor_y);
            cursor_x = editor_x;
            cursor_y = editor_y;
            update_editor_status();
        }
    }
    // Обычный символ
    else if (editor_pos < sizeof(editor_buffer) - 1 && key >= 32 && key <= 126) {
        editor_buffer[editor_pos++] = key;
        
        vga_putchar(key, VGA_COLOR_WHITE, editor_x, editor_y);
        editor_x++;
        
        if (editor_x >= EDITOR_LEFT_X + EDITOR_WIDTH) {
            editor_x = EDITOR_LEFT_X;
            editor_y++;
        }
        
        if (editor_y >= OUTPUT_START_Y + EDITOR_HEIGHT) {
            // Прокрутка
            for (int y = OUTPUT_START_Y+1; y < OUTPUT_START_Y+EDITOR_HEIGHT-1; y++) {
                for (int x = EDITOR_LEFT_X; x < EDITOR_LEFT_X+EDITOR_WIDTH; x++) {
                    vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y+1) * VGA_WIDTH + x];
                }
            }
            editor_y = OUTPUT_START_Y + EDITOR_HEIGHT - 1;
        }
        
        cursor_x = editor_x;
        cursor_y = editor_y;
        update_editor_status();
    }
}

// =============== ФУНКЦИИ РЕДАКТОРА ===============

void terminal_save_with_prompt(void) {
    // Диалог сохранения
    int dialog_y = VGA_HEIGHT - 3;
    
    // Очищаем область диалога
    vga_putchar('|', VGA_COLOR_CYAN, 0, dialog_y);
    for (int x = 1; x < 40; x++) {
        vga_putchar(' ', VGA_COLOR_BLACK, x, dialog_y);
    }
    vga_putchar('|', VGA_COLOR_CYAN, 41, dialog_y);
    
    // Заголовок диалога
    terminal_print_at(" Save as: ", 1, dialog_y, VGA_COLOR_YELLOW);
    
    char filename[64];
    uint32_t filename_pos = 0;
    uint32_t input_x = 11;
    
    // Копируем текущее имя файла в буфер ввода
    strcpy(filename, current_file);
    filename_pos = strlen(current_file);
    
    // Показываем текущее имя
    terminal_print_at(filename, input_x, dialog_y, VGA_COLOR_WHITE);
    input_x += filename_pos;
    
    while (1) {
        char key = keyboard_getch();
        if (!key) continue;
        
        if (key == '\n') { // Enter - сохранить
            filename[filename_pos] = 0;
            
            // Обновляем текущее имя файла
            strcpy(current_file, filename);
            
            // ВАЖНО: Используем новую функцию для сохранения
            int success = fs_editor_save(current_file, editor_buffer, editor_pos);
            
            if (success) {
                terminal_print_at(" ✓ Saved!          ", 20, dialog_y, VGA_COLOR_GREEN);
            } else {
                terminal_print_at(" ✗ Save failed!    ", 20, dialog_y, VGA_COLOR_RED);
            }
            
            // Ждем немного
            for (volatile int i = 0; i < 100000; i++);
            
            // Восстанавливаем границу
            vga_putchar('|', VGA_COLOR_CYAN, 0, dialog_y);
            for (int x = 1; x < 40; x++) {
                vga_putchar(' ', VGA_COLOR_BLACK, x, dialog_y);
            }
            vga_putchar('|', VGA_COLOR_CYAN, 41, dialog_y);
            
            // Обновляем статус бар
            update_status_bar();
            break;
            
        } else if (key == '\b' && filename_pos > 0) { // Backspace
            filename_pos--;
            input_x--;
            vga_putchar(' ', VGA_COLOR_BLACK, input_x, dialog_y);
            filename[filename_pos] = 0;
            
        } else if (filename_pos < 63 && key >= 32 && key <= 126) { // Обычный символ
            filename[filename_pos++] = key;
            filename[filename_pos] = 0;
            vga_putchar(key, VGA_COLOR_WHITE, input_x, dialog_y);
            input_x++;
            
        } else if (key == 27) { // ESC - отмена
            // Восстанавливаем границу
            vga_putchar('|', VGA_COLOR_CYAN, 0, dialog_y);
            for (int x = 1; x < 40; x++) {
                vga_putchar(' ', VGA_COLOR_BLACK, x, dialog_y);
            }
            vga_putchar('|', VGA_COLOR_CYAN, 41, dialog_y);
            break;
        }
    }
}

// Загрузка файла в редакторе (при команде edit)
void terminal_load_file_into_editor(const char* filename) {
    strcpy(current_file, filename);
    
    // ВАЖНО: Используем новую функцию для загрузки
    int size = fs_editor_load(filename, editor_buffer, sizeof(editor_buffer));
    
    if (size > 0) {
        editor_pos = size;
        terminal_print("✓ Loaded: ", VGA_COLOR_GREEN);
        terminal_print(filename, VGA_COLOR_WHITE);
        terminal_print("\n", VGA_COLOR_GREEN);
    } else {
        editor_pos = 0;
        editor_buffer[0] = 0;
        terminal_print("New file: ", VGA_COLOR_YELLOW);
        terminal_print(filename, VGA_COLOR_WHITE);
        terminal_print("\n", VGA_COLOR_YELLOW);
    }
    
    // Перерисовываем редактор
    terminal_redraw_editor_text();
}

void terminal_run_goo(void) {
    // Сохраняем перед запуском
    fs_editor_save(current_file, editor_buffer, editor_pos);
    
    // Очищаем панель вывода
    clear_output_panel();
    print_to_output("=== GOOSE VM EXECUTION ===\n", VGA_COLOR_YELLOW);
    
    // Компилируем
    uint8_t binary[8192];
    int bin_size = gooc_compile(editor_buffer, binary, sizeof(binary));
    
    if (bin_size <= 0) {
        print_to_output("Compilation failed!\n", VGA_COLOR_RED);
        return;
    }
    
    char buf[32];
    itoa(bin_size, buf, 10);
    print_to_output("Compiled: ", VGA_COLOR_GREEN);
    print_to_output(buf, VGA_COLOR_WHITE);
    print_to_output(" bytes\n", VGA_COLOR_GREEN);
    print_to_output("Starting VM...\n\n", VGA_COLOR_CYAN);
    
    // Запускаем VM
    GooVM* vm = goovm_create();
    if (!vm) {
        print_to_output("VM creation failed!\n", VGA_COLOR_RED);
        return;
    }
    
    if (goovm_load(vm, binary, bin_size)) {
        // Устанавливаем обработчик вывода
        goovm_set_output_handler(print_to_output);
        goovm_execute(vm);
        print_to_output("\n=== VM HALTED ===\n", VGA_COLOR_YELLOW);
    } else {
        print_to_output("Failed to load program!\n", VGA_COLOR_RED);
    }
    
    goovm_destroy(vm);
    terminal_print_at("✓ Execution complete", 43, OUTPUT_START_Y, VGA_COLOR_GREEN);
}


void terminal_open_file(void) {
    // Диалог открытия
    vga_putchar('|', VGA_COLOR_CYAN, 0, VGA_HEIGHT - 3);
    terminal_print_at(" Open: ", 1, VGA_HEIGHT - 3, VGA_COLOR_YELLOW);
    
    char filename[64];
    uint32_t filename_pos = 0;
    uint32_t input_x = 8;
    
    while (1) {
        char key = keyboard_getch();
        if (!key) continue;
        
        if (key == '\n') {
            filename[filename_pos] = 0;
            
            // Пытаемся загрузить
            uint8_t buffer[4096];
            int bytes_read = fs_read(filename, buffer, sizeof(buffer) - 1);
            
            if (bytes_read > 0) {
                strcpy(current_file, filename);
                
                // Копируем данные в редактор
                memcpy(editor_buffer, buffer, bytes_read);
                editor_pos = bytes_read;
                editor_buffer[editor_pos] = 0;
                
                // Перерисовываем редактор
                terminal_redraw_editor_text();
                update_status_bar();
                
                terminal_print_at("Loaded!             ", 20, VGA_HEIGHT - 3, VGA_COLOR_GREEN);
            } else {
                terminal_print_at("File not found!     ", 20, VGA_HEIGHT - 3, VGA_COLOR_RED);
            }
            
            // Очищаем диалог
            vga_putchar('|', VGA_COLOR_CYAN, 0, VGA_HEIGHT - 3);
            for (int x = 1; x < 40; x++) {
                vga_putchar(' ', VGA_COLOR_BLACK, x, VGA_HEIGHT - 3);
            }
            vga_putchar('|', VGA_COLOR_CYAN, 41, VGA_HEIGHT - 3);
            break;
        }
        else if (key == '\b' && filename_pos > 0) {
            filename_pos--;
            input_x--;
            vga_putchar(' ', VGA_COLOR_BLACK, input_x, VGA_HEIGHT - 3);
            filename[filename_pos] = 0;
        }
        else if (filename_pos < 63 && key >= 32 && key <= 126) {
            filename[filename_pos++] = key;
            filename[filename_pos] = 0;
            vga_putchar(key, VGA_COLOR_WHITE, input_x, VGA_HEIGHT - 3);
            input_x++;
        }
        else if (key == 27) { // ESC
            vga_putchar('|', VGA_COLOR_CYAN, 0, VGA_HEIGHT - 3);
            for (int x = 1; x < 40; x++) {
                vga_putchar(' ', VGA_COLOR_BLACK, x, VGA_HEIGHT - 3);
            }
            vga_putchar('|', VGA_COLOR_CYAN, 41, VGA_HEIGHT - 3);
            break;
        }
    }
}

void terminal_new_file(void) {
    strcpy(current_file, "untitled.txt");
    editor_pos = 0;
    memset(editor_buffer, 0, sizeof(editor_buffer));
    terminal_redraw_editor_text();
    update_status_bar();
    terminal_print_at("New file created!   ", 20, VGA_HEIGHT - 3, VGA_COLOR_GREEN);
}
void terminal_redraw_editor_text(void) {
    // Очищаем область редактора - ТОЛЬКО ВНУТРИ РАМКИ!
    for (int y = OUTPUT_START_Y+1; y < OUTPUT_START_Y + EDITOR_HEIGHT; y++) {
        for (int x = EDITOR_LEFT_X; x < EDITOR_LEFT_X + EDITOR_WIDTH; x++) {
            vga_putchar(' ', VGA_COLOR_BLACK, x, y);
        }
    }
    
    // Выводим текст
    uint32_t x = EDITOR_LEFT_X;
    uint32_t y = OUTPUT_START_Y+1;
    
    for (uint32_t i = 0; i < editor_pos && y < OUTPUT_START_Y + EDITOR_HEIGHT; i++) {
        char c = editor_buffer[i];
        
        if (c == '\n') {
            x = EDITOR_LEFT_X;
            y++;
            if (y >= OUTPUT_START_Y + EDITOR_HEIGHT) break;
        } else if (c >= 32 && c <= 126) {
            if (x < EDITOR_LEFT_X + EDITOR_WIDTH) {
                vga_putchar(c, VGA_COLOR_WHITE, x, y);
                x++;
            }
        }
    }
    
    editor_x = x;
    editor_y = y;
    if (editor_y >= OUTPUT_START_Y + EDITOR_HEIGHT) {
        editor_y = OUTPUT_START_Y + EDITOR_HEIGHT - 1;
    }
    if (editor_x >= EDITOR_LEFT_X + EDITOR_WIDTH) {
        editor_x = EDITOR_LEFT_X + EDITOR_WIDTH - 1;
    }
    
    cursor_x = editor_x;
    cursor_y = editor_y;
}
void vga_scroll_editor(void) {
    // Прокрутка только области редактирования (под баннером)
    // Область: от (6 + BANNER_HEIGHT) до (VGA_HEIGHT - 3)
    
    int start_y = 6 + BANNER_HEIGHT;
    int end_y = VGA_HEIGHT - 3;
    
    for (int y = start_y; y < end_y - 1; y++) {
        for (int x = 1; x < 40; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    // Очищаем предпоследнюю строку редактора
    for (int x = 1; x < 40; x++) {
        vga_putchar(' ', VGA_COLOR_BLACK, x, end_y - 1);
    }
}

// =============== ОБНОВЛЕНИЕ ===============
int terminal_get_mode(void) {
    return mode;
}



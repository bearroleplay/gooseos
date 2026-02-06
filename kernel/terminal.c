#include "terminal.h"
#include "panic.h"
#include "keyboard.h"
#include "fs.h"
#include "libc.h"
#include "goovm.h"
#include "cmos.h"
#include <string.h>
#include "gooc_simple.h"
#define BANNER_HEIGHT 5

// =============== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ===============
uint32_t cursor_x = 0;              // БЫЛО: static uint32_t cursor_x = 0;
uint32_t cursor_y = BANNER_HEIGHT;  // БЫЛО: static uint32_t cursor_y = BANNER_HEIGHT;
uint32_t tick = 0;                  // БЫЛО: static uint32_t tick = 0;
int mode = MODE_COMMAND;            // БЫЛО: static int mode = MODE_COMMAND;

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
    
    // Очищаем последнюю строку
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_putchar(' ', VGA_COLOR_BLACK, x, cursor_y);
    }
    
    // Печатаем промпт
    terminal_print("goose> ", vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
}

void terminal_handle_input(char key) {
    if (mode == MODE_EDITOR) {
        terminal_editor_handle_key(key);
        return;
    }
    
    // Командная строка - работаем на строке промпта
    if (key == '\n') {
        input_buffer[input_pos] = 0;
        
        // Скрываем курсор на строке промпта
        vga_putchar(' ', VGA_COLOR_BLACK, cursor_x, cursor_y);
        
        // Перемещаемся ВЫШЕ для вывода результата
        cursor_x = 0;
        cursor_y = VGA_HEIGHT - 3;  // Строка над промптом
        
        // Выполняем команду
        if (input_buffer[0] != 0) {
            terminal_execute_command(input_buffer);
        }
        
        input_pos = 0;
        
        // Показываем промпт снова
        if (mode == MODE_COMMAND) {
            terminal_show_prompt();
        }
    } 
    else if (key == '\b') {
        if (input_pos > 0) {
            input_pos--;
            
            // Стираем символ на строке промпта
            if (cursor_x > 7) {  // "goose> " занимает 7 символов
                cursor_x--;
                vga_putchar(' ', VGA_COLOR_BLACK, cursor_x, cursor_y);
            }
        }
    }
    else if (input_pos < 255 && key >= 32 && key <= 126) {
        // Вводим на строке промпта
        input_buffer[input_pos] = key;
        vga_putchar(key, vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), cursor_x, cursor_y);
        input_pos++;
        cursor_x++;
    }
}

// =============== КОМАНДЫ ===============
void terminal_execute_command(const char* cmd) {
    // Переходим на строку ВЫШЕ промпта для вывода результатов
    cursor_x = 0;
    cursor_y = VGA_HEIGHT - 2;  // Предпоследняя строка (промпт в последней)
    
    if (strcmp(cmd, "help") == 0) {
        terminal_print("=== GooseOS Commands ===\n", VGA_COLOR_YELLOW);
        
        terminal_print("Disk Operations:\n", VGA_COLOR_CYAN);
        terminal_print("  format          - Format disk (WARNING: erases all data!)\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  fsinfo          - Show filesystem information\n", VGA_COLOR_LIGHT_GRAY);
        
        terminal_print("\nFile System:\n", VGA_COLOR_CYAN);
        terminal_print("  ls              - List files in current directory\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  cd <dir>        - Change directory (cd .., cd /)\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  pwd             - Show current directory\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  mkdir <name>    - Create directory\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  rmdir <name>    - Remove empty directory\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  rm <name>       - Delete file\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  rename <old> <new> - Rename file/directory\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  copy <src> <dst>   - Copy file\n", VGA_COLOR_LIGHT_GRAY);
        
        terminal_print("\nFile Operations:\n", VGA_COLOR_CYAN);
        terminal_print("  cat <file>      - View file contents\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  edit <file>     - Open text editor\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  files           - List all files (recursive)\n", VGA_COLOR_LIGHT_GRAY);
        
        terminal_print("\nGooseScript Programming:\n", VGA_COLOR_CYAN);
        terminal_print("  compile file.goo - Compile to .goobin\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  run file.goo     - Run GooseScript program\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  run file.goobin  - Run compiled binary\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  about           - About GooseScript language\n", VGA_COLOR_LIGHT_GRAY);
        
        terminal_print("\nEditor (Ctrl+Key):\n", VGA_COLOR_CYAN);
        terminal_print("  Ctrl+S          - Save file\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  Ctrl+R          - Run .goo file (in editor)\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  Ctrl+O          - Open file\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  Ctrl+N          - New file\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  Ctrl+Q          - Exit editor\n", VGA_COLOR_LIGHT_GRAY);
        
        terminal_print("\nSystem:\n", VGA_COLOR_CYAN);
        terminal_print("  clear           - Clear screen\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  reboot          - Reboot system\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  shutdown        - Shutdown system\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  calc            - Open calculator\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  time            - Show current time\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  date            - Show current date\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  panic           - Test kernel panic\n", VGA_COLOR_LIGHT_RED);
        
        terminal_print("\n", VGA_COLOR_WHITE);
    }
    else if (strncmp(cmd, "panic", 6) == 0) {
    const char* arg = cmd + 6;
    
    // Пропускаем пробелы после "panic"
    while (*arg == ' ') arg++;
    
    if (arg[0] == 0) {
        terminal_print("Usage: panic <1-4>\n", VGA_COLOR_RED);
        terminal_print("  1 - Blue screen (kernel)\n", VGA_COLOR_LIGHT_BLUE);
        terminal_print("  2 - Red screen (memory)\n", VGA_COLOR_LIGHT_RED);
        terminal_print("  3 - White screen (hardware)\n", VGA_COLOR_WHITE);
        terminal_print("  4 - Black screen (generic)\n", VGA_COLOR_LIGHT_GRAY);
        return;
    }
    
    char choice = arg[0];
    
    switch(choice) {
        case '1':
            terminal_print("Triggering BLUE screen...\n", VGA_COLOR_LIGHT_BLUE);
            panic_kernel("User triggered kernel panic\n"
                         "Reason: Manual test\n"
                         "System halted for safety", 0x12345678);
            break;
            
        case '2':
            terminal_print("Triggering RED screen...\n", VGA_COLOR_LIGHT_RED);
            panic_memory("Memory corruption detected\n"
                         "Heap integrity check failed\n"
                         "Possible buffer overflow", 0xDEADBEEF);
            break;
            
        case '3':
            terminal_print("Triggering WHITE screen...\n", VGA_COLOR_WHITE);
            panic_hardware("Hardware failure\n"
                           "ATA disk controller timeout\n"
                           "Sector read failed", 0xBADF00D);
            break;
            
        case '4':
            terminal_print("Triggering BLACK screen...\n", VGA_COLOR_LIGHT_GRAY);
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
else if (strcmp(cmd, "calc") == 0) {
    terminal_start_calculator();
    return; // Важно - не показываем промпт дважды
}
    else if (strcmp(cmd, "ls") == 0) {
        fs_list();
    }
    else if (strncmp(cmd, "cd", 3) == 0) {
        const char* path = cmd + 3;
        if (fs_cd(path)) {
            terminal_print("Changed to: ", VGA_COLOR_GREEN);
            terminal_print(fs_pwd(), VGA_COLOR_WHITE);
            terminal_print("\n", VGA_COLOR_GREEN);
        } else {
            terminal_print("Directory not found\n", VGA_COLOR_RED);
        }
    }
    else if (strncmp(cmd, "edit", 5) == 0) {
    const char* filename = cmd + 5;
    
    if (strlen(filename) == 0) {
        terminal_print("Usage: edit <filename>\n", VGA_COLOR_RED);
        terminal_print("Example: edit test.txt\n", VGA_COLOR_LIGHT_GRAY);
        return;
    }
    
    // Проверяем, что filename не пустой
    if (filename[0] == 0) {
        terminal_print("Error: No filename specified\n", VGA_COLOR_RED);
        terminal_print("Use: edit filename.txt\n", VGA_COLOR_LIGHT_GRAY);
        return;
    }
    
    terminal_print("Opening: ", VGA_COLOR_CYAN);
    terminal_print(filename, VGA_COLOR_WHITE);
    terminal_print("\n", VGA_COLOR_CYAN);
    
    // Если файл с расширением .goo
    const char* dot = strrchr(filename, '.');
    if (dot && strcmp(dot, ".goo") == 0) {
        terminal_print("(GooseScript file)\n", VGA_COLOR_LIGHT_BLUE);
    }
    
    // Просто запускаем редактор с этим файлом
    strcpy(current_file, filename); // current_file из terminal.c
    
    // Загружаем файл (если есть)
    int size = fs_editor_load(filename, editor_buffer, sizeof(editor_buffer));
    if (size > 0) {
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
    else if (strcmp(cmd, "format") == 0) {
        terminal_print("WARNING: This will erase ALL data on disk!\n", VGA_COLOR_RED);
        terminal_print("Type 'YES' to confirm: ", VGA_COLOR_YELLOW);
        
        // Простая реализация подтверждения
        char confirm[10];
        int pos = 0;
        
        while (1) {
            char key = keyboard_getch();
            if (key == '\n') {
                confirm[pos] = 0;
                break;
            } else if (key == '\b' && pos > 0) {
                pos--;
                terminal_print("\b \b", VGA_COLOR_WHITE);
            } else if (key >= 32 && key <= 126 && pos < 9) {
                confirm[pos] = key;
                pos++;
                terminal_putchar(key);
            }
        }
        
        if (strcmp(confirm, "YES") == 0) {
            if (fs_format()) {
                terminal_print("\nDisk formatted successfully\n", VGA_COLOR_GREEN);
            } else {
                terminal_print("\nFormat failed\n", VGA_COLOR_RED);
            }
        } else {
            terminal_print("\nFormat cancelled\n", VGA_COLOR_YELLOW);
        }
    }
    
    else if (strcmp(cmd, "fsinfo") == 0) {
        fs_info();
    }
    
    else if (strncmp(cmd, "rename ", 7) == 0) {
        char* args = (char*)(cmd + 7);
        char* oldname = args;
        char* space = strchr(args, ' ');
        
        if (space) {
            *space = 0;
            char* newname = space + 1;
            
            if (fs_rename(oldname, newname)) {
                terminal_print("Renamed\n", VGA_COLOR_GREEN);
            } else {
                terminal_print("Rename failed\n", VGA_COLOR_RED);
            }
        } else {
            terminal_print("Usage: rename <old> <new>\n", VGA_COLOR_RED);
        }
    }
    
    else if (strncmp(cmd, "copy", 5) == 0) {
        char* args = (char*)(cmd + 5);
        char* src = args;
        char* space = strchr(args, ' ');
        
        if (space) {
            *space = 0;
            char* dst = space + 1;
            
            if (fs_copy(src, dst)) {
                terminal_print("Copied\n", VGA_COLOR_GREEN);
            } else {
                terminal_print("Copy failed\n", VGA_COLOR_RED);
            }
        } else {
            terminal_print("Usage: copy <src> <dst>\n", VGA_COLOR_RED);
        }
    }
    else if (strcmp(cmd, "testfs") == 0) {
        terminal_print("Testing filesystem...\n", VGA_COLOR_CYAN);
        
        // Пробуем создать файл
        if (fs_create("test.txt", (uint8_t*)"Hello from GooseOS!", 19, FS_TYPE_FILE)) {
            terminal_print("✓ Created test.txt\n", VGA_COLOR_GREEN);
            
            // Пробуем прочитать
            uint8_t buffer[100];
            int size = fs_read("test.txt", buffer, sizeof(buffer));
            if (size > 0) {
                buffer[size] = 0;
                terminal_print("✓ Read: ", VGA_COLOR_GREEN);
                terminal_print((char*)buffer, VGA_COLOR_WHITE);
                terminal_print("\n", VGA_COLOR_GREEN);
            }
        } else {
            terminal_print("✗ Filesystem not working\n", VGA_COLOR_RED);
        }
    }
    else if (strcmp(cmd, "pwd") == 0) {
        terminal_print(fs_pwd(), VGA_COLOR_CYAN);
        terminal_print("\n", VGA_COLOR_WHITE);
    }

       else if (strncmp(cmd, "mkdir", 6) == 0) {
        const char* name = cmd + 6;
        if (fs_mkdir(name)) {
            terminal_print("Directory created\n", VGA_COLOR_GREEN);
        } else {
            terminal_print("Failed to create directory\n", VGA_COLOR_RED);
        }
    }
    
    else if (strncmp(cmd, "rmdir", 6) == 0) {
        const char* name = cmd + 6;
        if (fs_rmdir(name)) {
            terminal_print("Directory removed\n", VGA_COLOR_GREEN);
        } else {
            terminal_print("Directory not found or not empty\n", VGA_COLOR_RED);
        }
    }
    
    else if (strncmp(cmd, "rm", 3) == 0) {
        const char* name = cmd + 3;
        if (fs_delete(name)) {
            terminal_print("Deleted\n", VGA_COLOR_GREEN);
        } else {
            terminal_print("File not found\n", VGA_COLOR_RED);
        }
    }
    
    else if (strncmp(cmd, "cat", 4) == 0) {
        const char* name = cmd + 4;
        uint8_t buffer[4096];
        int size = fs_read(name, buffer, sizeof(buffer));
        if (size > 0) {
            buffer[size] = 0;
            terminal_print((char*)buffer, VGA_COLOR_WHITE);
            terminal_print("\n", VGA_COLOR_WHITE);
        } else {
            terminal_print("File not found\n", VGA_COLOR_RED);
        }
    }
    else if (strcmp(cmd, "clear") == 0) {
        terminal_clear_with_banner();
        // После очистки сразу показываем промпт
        terminal_show_prompt();
        return;
    }
    else if (strcmp(cmd, "files") == 0) {
        fs_list();
    }
    else if (strncmp(cmd, "compile", 8) == 0) {
        const char* filename = cmd + 8;
        if (strlen(filename) == 0) {
            terminal_print("Usage: compile <filename.goo>\n", VGA_COLOR_RED);
        } else {
            // Читаем файл
            uint8_t buffer[4096];
            int bytes = fs_read(filename, buffer, sizeof(buffer) - 1);
            
            if (bytes > 0) {
                buffer[bytes] = 0;
                
                // Компилируем
                uint8_t binary[8192];
                int bin_size = gooc_compile((char*)buffer, binary, sizeof(binary));
                
                if (bin_size > 0) {
                    // Сохраняем .goobin
                    char out_name[64];
                    strcpy(out_name, filename);
                    char* dot = strrchr(out_name, '.');
                    if (dot) *dot = 0;
                    strcat(out_name, ".goobin");
                    
                    if (fs_create(out_name, binary, bin_size, 1)) {
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
                terminal_print(filename, VGA_COLOR_WHITE);
                terminal_print("\n", VGA_COLOR_RED);
            }
        }
    }
      else if (strncmp(cmd, "run", 4) == 0) {
        const char* filename = cmd + 4;
        if (strlen(filename) == 0) {
            terminal_print("Usage: run <filename>\n", VGA_COLOR_RED);
        } else {
            // Читаем файл
            uint8_t buffer[4096];
            int bytes = fs_read(filename, buffer, sizeof(buffer));
            
            if (bytes > 0) {
                // Автокомпиляция
                uint8_t binary[8192];
                int bin_size = gooc_compile((char*)buffer, binary, sizeof(binary));
                
                if (bin_size > 0) {
                    // Запускаем БЕЗ отладки
                    GooVM* vm = goovm_create();
                    if (vm) {
                        goovm_load(vm, binary, bin_size);
                        goovm_execute(vm);  // Тихо!
                        goovm_destroy(vm);
                    }
                }
            }
        }
    }
    else if (strcmp(cmd, "about") == 0) {
        terminal_print("=== GooseOS v1.0 ===\n", VGA_COLOR_YELLOW);
        terminal_print("A simple OS with its own scripting language!\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("\nGooseScript Features:\n", VGA_COLOR_CYAN);
        terminal_print("• print \"text\" - Print strings\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("• push number - Push to stack\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("• add/sub/mul/div - Math operations\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("• syscall num - System calls\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("• exit code   - Exit program\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("\nWARNING: The compiler is experimental!\n", VGA_COLOR_RED);
        terminal_print("Some features may not work correctly.\n", VGA_COLOR_RED);
        terminal_print("  shutdown        - Shut down the computer\n", VGA_COLOR_LIGHT_GRAY); // <-- НОВОЕ
        terminal_print("\n", VGA_COLOR_WHITE);
    }
     else if (strcmp(cmd, "reboot") == 0) {
        terminal_print("Rebooting system...\n", VGA_COLOR_RED);
        terminal_print("Press any key to cancel (3 seconds)...\n", VGA_COLOR_YELLOW);
        
        // Даём 3 секунды на отмену
        uint32_t timeout = 300; // ~3 секунды (300 * 10ms)
        int cancelled = 0;
        
        while (timeout-- > 0) {
            char key = keyboard_getch();
            if (key) {
                terminal_print("\nReboot cancelled!\n", VGA_COLOR_GREEN);
                cancelled = 1;
                break;
            }
            for (volatile int i = 0; i < 10000; i++); // ~10ms задержка
        }
        
        if (!cancelled) {
            terminal_print("\nBye! Restarting...\n", VGA_COLOR_RED);
            for (volatile int i = 0; i < 1000000; i++); // Задержка
            __asm__ volatile("outb %0, $0x64" : : "a"((uint8_t)0xFE)); // Сброс CPU
            while(1); // Зависаем навсегда
        }
    }
    else if (strcmp(cmd, "shutdown") == 0) {  // <-- НОВАЯ КОМАНДА
        terminal_print("Shutting down GooseOS...\n", VGA_COLOR_RED);
        terminal_print("Press any key to cancel (3 seconds)...\n", VGA_COLOR_YELLOW);
        
        // Даём 3 секунды на отмену
        uint32_t timeout = 300; // ~3 секунды
        int cancelled = 0;
        
        while (timeout-- > 0) {
            char key = keyboard_getch();
            if (key) {
                terminal_print("\nShutdown cancelled!\n", VGA_COLOR_GREEN);
                cancelled = 1;
                break;
            }
            for (volatile int i = 0; i < 10000; i++); // ~10ms задержка
        }
        
        if (!cancelled) {
            terminal_print("\nGoodbye! Turning off...\n", VGA_COLOR_RED);
            
            // Анимация выключения
            for (int i = 0; i < 5; i++) {
                terminal_print(".", VGA_COLOR_RED);
                for (volatile int j = 0; j < 200000; j++);
            }
            terminal_print("\n", VGA_COLOR_RED);
            
            // Пытаемся выключить через ACPI (если поддерживается)
            terminal_print("Attempting ACPI shutdown...\n", VGA_COLOR_YELLOW);
            
                        // Способ 1: Через ACPI (если поддерживается)
            // Для 32-битного режима используем правильный синтаксис
            __asm__ volatile(
                "mov $0x604, %%dx\n\t"
                "mov $0x2000, %%ax\n\t"
                "out %%ax, %%dx"
                : : : "ax", "dx"
            );
            
            // Способ 2: Через порт 0xB004 (Bochs)
            __asm__ volatile(
                "mov $0xB004, %%dx\n\t"
                "mov $0x2000, %%ax\n\t"
                "out %%ax, %%dx"
                : : : "ax", "dx"
            );
            
            // Способ 3: Через порт 0x4004 (VirtualBox)
            __asm__ volatile(
                "mov $0x4004, %%dx\n\t"
                "mov $0x3400, %%ax\n\t"
                "out %%ax, %%dx"
                : : : "ax", "dx"
            );
            
                        // Если всё ещё работаем - показываем сообщение
            terminal_print("Shutdown not supported in this environment.\n", VGA_COLOR_RED);
            terminal_print("Use 'reboot' instead or close the emulator.\n", VGA_COLOR_YELLOW);
        }
    }
      else if (cmd[0] != 0) {  // <-- ИСПРАВЛЕНО: убрал лишние пробелы и добавил скобки
        terminal_print("Unknown command: ", VGA_COLOR_RED);
        terminal_print(cmd, VGA_COLOR_WHITE);
        terminal_print("\nType 'help' for commands\n", VGA_COLOR_RED);
    }
    // else для пустой команды не нужен
    
    // Показываем промпт ВСЕГДА после выполнения команды (кроме редактора)
    if (mode == MODE_COMMAND) {  // ← СТРОКА 753
        terminal_show_prompt();
    }
}  // ← СТРОКА 756

// =============== РЕДАКТОР ===============
void terminal_start_editor(void) {
    vga_clear();
    mode = MODE_EDITOR;
    
    // ВСЕ координаты Y + BANNER_HEIGHT!
    terminal_print_at("+--------------------------------------+", 0, 0 + BANNER_HEIGHT, VGA_COLOR_CYAN);
    terminal_print_at("|        GOOSE OS EDITOR v1.0 Beta   |", 0, 1 + BANNER_HEIGHT, VGA_COLOR_CYAN);
    terminal_print_at("| Ctrl+S: Save | Ctrl+R: RUN .goo    |", 0, 2 + BANNER_HEIGHT, VGA_COLOR_CYAN);
    terminal_print_at("| Ctrl+N: New  | Ctrl+O: Open        |", 0, 3 + BANNER_HEIGHT, VGA_COLOR_CYAN);
    terminal_print_at("+--------------------------------------+", 0, 4 + BANNER_HEIGHT, VGA_COLOR_CYAN);
    
    // Боковые границы
    for (int y = 5; y < VGA_HEIGHT - 2 - BANNER_HEIGHT; y++) {
        vga_putchar('|', VGA_COLOR_CYAN, 0, y + BANNER_HEIGHT);
        vga_putchar('|', VGA_COLOR_CYAN, 41, y + BANNER_HEIGHT);
    }
    
    // Нижняя граница
    terminal_print_at("+--------------------------------------+", 0, VGA_HEIGHT - 2, VGA_COLOR_CYAN);
    
   
    // Статус бар (последняя строка экрана)
    update_status_bar();
    
    // Позиция курсора в редакторе (первая строка текста)
    editor_x = 1;
    editor_y = 6 + BANNER_HEIGHT;
    editor_pos = 0;
    memset(editor_buffer, 0, sizeof(editor_buffer));
    
    cursor_x = editor_x;
    cursor_y = editor_y;
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
void terminal_editor_handle_key(char key) {
    // Ctrl+Q - Выход
    if (keyboard_get_ctrl() && (key == 'q' || key == 'Q')) {
        mode = MODE_COMMAND;
        terminal_clear_with_banner(); // Очищаем с баннером!
        terminal_show_prompt();
        return;
    }
    
    // Ctrl+S - Сохранить
    if (keyboard_get_ctrl() && (key == 's' || key == 'S')) {
        terminal_save_with_prompt();
        return;
    }
    
     if (keyboard_get_ctrl() && (key == 'r' || key == 'R')) {
    // Проверяем расширение .goo
    char* dot = strrchr(current_file, '.');
    if (dot && strcmp(dot, ".goo") == 0) {
        // Показываем сообщение о том, что компилятор в разработке
        terminal_print_at("|      GOOSE COMPILER v0.1        |", 
                         42, 8 + BANNER_HEIGHT, VGA_COLOR_CYAN);
        terminal_print_at("|                                |", 
                         42, 9 + BANNER_HEIGHT, VGA_COLOR_BLUE);
        terminal_print_at("|  Compiler is under development |", 
                         42, 10 + BANNER_HEIGHT, VGA_COLOR_YELLOW);
        terminal_print_at("|  Only simple programs work     |", 
                         42, 11 + BANNER_HEIGHT, VGA_COLOR_YELLOW);
        terminal_print_at("|                                |", 
                         42, 12 + BANNER_HEIGHT, VGA_COLOR_BLUE);
        terminal_print_at("|  Try:                          |", 
                         42, 13 + BANNER_HEIGHT, VGA_COLOR_WHITE);
        terminal_print_at("|    print \"Hello\"              |", 
                         42, 14 + BANNER_HEIGHT, VGA_COLOR_LIGHT_GRAY);
        terminal_print_at("|    exit 0                      |", 
                         42, 15 + BANNER_HEIGHT, VGA_COLOR_LIGHT_GRAY);
        terminal_print_at("|                                |", 
                         42, 16 + BANNER_HEIGHT, VGA_COLOR_BLUE);
        terminal_print_at("|  Press any key to continue...  |", 
                         42, 17 + BANNER_HEIGHT, VGA_COLOR_LIGHT_GRAY);
        
        // Ждём нажатия
        while (!keyboard_getch());
        
        // Восстанавливаем панель
        terminal_print_at("| STATUS: UNDER DEVELOPMENT      |", 
                         42, 8 + BANNER_HEIGHT, VGA_COLOR_RED);
        terminal_print_at("|                                |", 
                         42, 9 + BANNER_HEIGHT, VGA_COLOR_BLUE);
        terminal_print_at("| The GooseScript compiler is    |", 
                         42, 10 + BANNER_HEIGHT, VGA_COLOR_WHITE);
        terminal_print_at("| currently unstable. We're      |", 
                         42, 11 + BANNER_HEIGHT, VGA_COLOR_WHITE);
        terminal_print_at("| working on fixes!              |", 
                         42, 12 + BANNER_HEIGHT, VGA_COLOR_WHITE);
    } else {
        terminal_print_at("Not a .goo file!", 20, VGA_HEIGHT - 3, VGA_COLOR_RED);
    }
    return;
}
    
    // Ctrl+O - Открыть файл
    if (keyboard_get_ctrl() && (key == 'o' || key == 'O')) {
        terminal_open_file();
        return;
    }
    
    // Ctrl+N - Новый файл
    if (keyboard_get_ctrl() && (key == 'n' || key == 'N')) {
        terminal_new_file();
        return;
    }
    
    // Обычные клавиши
    if (key == '\n') { // Enter
        if (editor_pos < sizeof(editor_buffer) - 1) {
            editor_buffer[editor_pos++] = '\n';
            
            // Рисуем перевод строки
            vga_putchar(' ', VGA_COLOR_WHITE, editor_x, editor_y); // Стираем курсор
            editor_x = 1;
            editor_y++;
            
            // Проверяем границы редактора
            if (editor_y >= VGA_HEIGHT - 2) {
                vga_scroll_editor();
                editor_y = VGA_HEIGHT - 3;
            }
            
            // Обновляем позицию курсора
            cursor_x = editor_x;
            cursor_y = editor_y;
        }
        update_status_bar();
    }
    else if (key == '\b') { // Backspace
        if (editor_pos > 0) {
            editor_pos--;
            
            // Стираем символ на экране
            vga_putchar(' ', VGA_COLOR_BLACK, editor_x, editor_y);
            
            // Если удаляем перевод строки
            if (editor_buffer[editor_pos] == '\n') {
                editor_y--;
                editor_x = 39; // В конец предыдущей строки
                // Находим реальную позицию конца строки
                for (int x = 39; x >= 1; x--) {
                    if (vga_buffer[editor_y * VGA_WIDTH + x] != ' ') {
                        editor_x = x + 1;
                        break;
                    }
                }
            } else {
                editor_x--;
                if (editor_x < 1) {
                    if (editor_y > 6 + BANNER_HEIGHT) {
                        editor_y--;
                        editor_x = 39;
                    } else {
                        editor_x = 1;
                    }
                }
            }
            
            // Обновляем позицию курсора
            cursor_x = editor_x;
            cursor_y = editor_y;
            update_status_bar();
        }
    }
    else if (editor_pos < sizeof(editor_buffer) - 1 && key >= 32 && key <= 126) {
        // Обычный символ
        editor_buffer[editor_pos++] = key;
        
        // Рисуем символ
        vga_putchar(key, VGA_COLOR_WHITE, editor_x, editor_y);
        editor_x++;
        
        // Перенос строки внутри редактора
        if (editor_x >= 40) {
            editor_x = 1;
            editor_y++;
            if (editor_y >= VGA_HEIGHT - 2) {
                vga_scroll_editor();
                editor_y = VGA_HEIGHT - 3;
            }
        }
        
        // Обновляем позицию курсора
        cursor_x = editor_x;
        cursor_y = editor_y;
        update_status_bar();
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
    terminal_redraw_editor();
}

// Запуск .goo файла из редактора (Ctrl+R)
void terminal_run_goo(void) {
    // Проверяем расширение
    char* dot = strrchr(current_file, '.');
    if (dot && strcmp(dot, ".goo") == 0) {
        terminal_print_at("=== Compiling & Running ===", 42, 6 + BANNER_HEIGHT, VGA_COLOR_CYAN);
        
        // Компилируем
        uint8_t binary[8192];
        int bin_size = gooc_compile(editor_buffer, binary, sizeof(binary));
        
        if (bin_size > 0) {
            // Сохраняем .goobin
            char bin_name[64];
            strcpy(bin_name, current_file);
            char* dot2 = strrchr(bin_name, '.');
            if (dot2) *dot2 = 0;
            strcat(bin_name, ".goobin");
            
            // ВАЖНО: Используем новую функцию для сохранения бинарника
            if (fs_save_goosebin(current_file, bin_name, binary, bin_size)) {
                // Загружаем и запускаем
                uint8_t loaded_binary[8192];
                int loaded_size = fs_load_goosebin(bin_name, loaded_binary, sizeof(loaded_binary));
                
                if (loaded_size > 0) {
                    GooVM* vm = goovm_create();
                    if (vm) {
                        if (goovm_load(vm, loaded_binary, loaded_size)) {
                            terminal_print_at("Executing...", 42, 8 + BANNER_HEIGHT, VGA_COLOR_GREEN);
                            goovm_execute(vm);
                        }
                        goovm_destroy(vm);
                    }
                }
            }
        } else {
            terminal_print_at("Compilation failed!", 42, 8 + BANNER_HEIGHT, VGA_COLOR_RED);
        }
    } else {
        terminal_print_at("Not a .goo file!", 20, VGA_HEIGHT - 3, VGA_COLOR_RED);
    }
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
                terminal_redraw_editor();
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
    terminal_redraw_editor();
    update_status_bar();
    terminal_print_at("New file created!   ", 20, VGA_HEIGHT - 3, VGA_COLOR_GREEN);
}

void terminal_redraw_editor(void) {
    // Очищаем область редактирования (под баннером)
    // От строки 6+BANNER_HEIGHT до предпоследней строки
    for (int y = 6 + BANNER_HEIGHT; y < VGA_HEIGHT - 2; y++) {
        for (int x = 1; x < 40; x++) {
            vga_putchar(' ', VGA_COLOR_BLACK, x, y);
        }
    }
    
    // Перерисовываем текст (начинаем под баннером)
    editor_x = 1;
    editor_y = 6 + BANNER_HEIGHT;  // Начало области редактирования
    
    for (uint32_t i = 0; i < editor_pos && editor_y < VGA_HEIGHT - 2; i++) {
        char c = editor_buffer[i];
        
        if (c == '\n') {
            editor_x = 1;
            editor_y++;
            if (editor_y >= VGA_HEIGHT - 2) {
                // Достигли нижней границы редактора
                editor_y = 6 + BANNER_HEIGHT;  // Вернемся к началу области
            }
        } else if (c >= 32 && c <= 126) {  // Печатные символы
            vga_putchar(c, VGA_COLOR_WHITE, editor_x, editor_y);
            editor_x++;
            
            if (editor_x >= 40) {  // Достигли правой границы редактора
                editor_x = 1;
                editor_y++;
                if (editor_y >= VGA_HEIGHT - 2) {
                    editor_y = 6 + BANNER_HEIGHT;
                }
            }
        }
        // Непечатные символы игнорируем
    }
    
    // Обновляем позицию курсора
    cursor_x = editor_x;
    cursor_y = editor_y;
    
    // Обновляем статус бар (показывает количество строк)
    update_status_bar();
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
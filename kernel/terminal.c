#include "terminal.h"
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
    
    
    // Время и дата справа
    char time_str[16], date_str[16];
    get_time_string(time_str);
    get_date_string(date_str);
    
    terminal_print_at(time_str, VGA_WIDTH - 8, 1, VGA_COLOR_LIGHT_BLUE);
    terminal_print_at(date_str, VGA_WIDTH - 10, 2, VGA_COLOR_LIGHT_GREEN);
    
    // Нижняя граница
    terminal_print_at("========================================", 0, 3, VGA_COLOR_CYAN);
    
    // СТРОКА С ФАКТОМ (4-я строка баннера)
  //  show_random_fact();
    
    // Статусная строка (5-я строка)
    terminal_print_at("Type 'help' for commands", 0, 4, VGA_COLOR_LIGHT_GRAY);
}

//void show_random_fact(void) {
//    static uint32_t last_fact_index = 0;
 //   static uint32_t last_change_tick = 0;
//    
//    uint32_t new_index = last_fact_index;
    
    // Генерируем новый индекс
 //   do {
//        new_index = (tick * 1103515245 + 12345) % FACTS_COUNT;
//    } while (new_index == last_fact_index && FACTS_COUNT > 1);
    
 //   if (tick - last_change_tick > 1000) { // Каждые 10 секунд
        // ВКЛЮЧИТЬ АНИМАЦИЮ - раскомментируй одну из строк:
        
        // 1. Анимация с прокруткой
        // animate_fact_change(last_fact_index, new_index);
        
        // 2. Печатная машинка
        // typewriter_fact_change(new_index);
        
        // 3. Без анимации (текущий вариант)
        // Просто рисуем новый факт
      //  const char* fact = goose_facts[new_index];
      //  int fact_len = strlen(fact);
      //  if (fact_len < VGA_WIDTH) {
      //      int pad = (VGA_WIDTH - fact_len) / 2;
            // Очищаем строку
      //      for (int x = 0; x < VGA_WIDTH; x++) {
      //          vga_putchar(' ', VGA_COLOR_BLACK, x, 3);
      //      }
      //      terminal_print_at(fact, pad, 3, VGA_COLOR_LIGHT_MAGENTA);
      //  }
        
     //   last_fact_index = new_index;
     //   last_change_tick = tick;
  //  }
//}

void update_banner_time(void) {
    static uint32_t last_time_update = 0;
    
    // Обновляем время каждые 50 тиков (~0.5 секунды)
    if (tick - last_time_update > 50) {
        char time_str[16];
        get_time_string(time_str);
        
        // Очищаем область времени
        for (int x = VGA_WIDTH - 8; x < VGA_WIDTH; x++) {
            vga_putchar(' ', VGA_COLOR_BLACK, x, 1);
        }
        
        // Рисуем новое время
        terminal_print_at(time_str, VGA_WIDTH - 8, 1, VGA_COLOR_LIGHT_BLUE);
        last_time_update = tick;
    }
    
    // Также обновляем факты (если пришло время)
  //  static uint32_t last_fact_update = 0;
   // if (tick - last_fact_update > 1000) { // Каждые 10 секунд
   //     show_random_fact();
   //     last_fact_update = tick;
  //  }
}

//void animate_fact_change(int old_index, int new_index) {
    // Если анимация отключена или индексы одинаковые - просто меняем
 //   if (old_index == new_index) {
//        show_random_fact();
//        return;
//    }
    
    // Получаем старый и новый факты
//    const char* old_fact = goose_facts[old_index];
 //   const char* new_fact = goose_facts[new_index];
    
    // Длины фактов
  //  int old_len = strlen(old_fact);
  //  int new_len = strlen(new_fact);
    
    // Позиции для центрирования
  //  int old_pad = (VGA_WIDTH - old_len) / 2;
   // int new_pad = (VGA_WIDTH - new_len) / 2;
    
    // === АНИМАЦИЯ 1: Прокрутка вверх ===
    
    // Шаг 1: Старый факт на своей позиции
   // terminal_print_at(old_fact, old_pad, 3, VGA_COLOR_LIGHT_MAGENTA);
    
    // Небольшая пауза
   // for (volatile int i = 0; i < 20000; i++);
    
    // Шаг 2-4: Прокрутка вверх
  //  for (int step = 1; step <= 3; step++) {
        // Очищаем строку факта
   //     for (int x = 0; x < VGA_WIDTH; x++) {
   //         vga_putchar(' ', VGA_COLOR_BLACK, x, 3);
   //     }
        
        // Рисуем старый факт смещённым вверх
   //     if (3 - step >= 0) {
   //         terminal_print_at(old_fact, old_pad, 3 - step, VGA_COLOR_LIGHT_MAGENTA);
   //     }
        
        // Рисуем новый факт входящий снизу
    //    if (3 + (3 - step) < BANNER_HEIGHT) {
    //        terminal_print_at(new_fact, new_pad, 3 + (3 - step), VGA_COLOR_LIGHT_MAGENTA);
     //   }
        
        // Пауза между кадрами
     //   for (volatile int i = 0; i < 15000; i++);
  //  }
    
    // Шаг 5: Новый факт на своей позиции
  //  for (int x = 0; x < VGA_WIDTH; x++) {
  //      vga_putchar(' ', VGA_COLOR_BLACK, x, 3);
  //  }
  //  terminal_print_at(new_fact, new_pad, 3, VGA_COLOR_LIGHT_MAGENTA);
    
    // === АЛЬТЕРНАТИВНАЯ АНИМАЦИЯ 2: Затухание ===
    /*
    // Постепенно затухаем старый факт
    for (int brightness = 15; brightness >= 0; brightness -= 3) {
        uint8_t color = vga_make_color(brightness, VGA_COLOR_BLACK);
        terminal_print_at(old_fact, old_pad, 3, color);
        for (volatile int i = 0; i < 5000; i++);
    }
    
    // Постепенно проявляем новый факт
    for (int brightness = 0; brightness <= 15; brightness += 3) {
        uint8_t color = vga_make_color(brightness, VGA_COLOR_BLACK);
        terminal_print_at(new_fact, new_pad, 3, color);
        for (volatile int i = 0; i < 5000; i++);
    }
    */
    
    // === АЛЬТЕРНАТИВНАЯ АНИМАЦИЯ 3: Справа налево ===
    /*
    // Новый факт въезжает справа, старый уезжает налево
    for (int pos = VGA_WIDTH; pos >= new_pad; pos -= 2) {
        // Очищаем строку
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_putchar(' ', VGA_COLOR_BLACK, x, 3);
        }
        
        // Старый факт уезжает налево
        int old_pos = old_pad - (VGA_WIDTH - pos);
        if (old_pos + old_len > 0 && old_pos < VGA_WIDTH) {
            // Обрезаем если выходит за границы
            int start = (old_pos < 0) ? -old_pos : 0;
            int len = old_len - start;
            if (old_pos + len > VGA_WIDTH) {
                len = VGA_WIDTH - old_pos;
            }
            
            if (len > 0) {
                char buffer[VGA_WIDTH + 1];
                strncpy(buffer, old_fact + start, len);
                buffer[len] = 0;
                terminal_print_at(buffer, 
                    (old_pos > 0) ? old_pos : 0, 
                    3, 
                    VGA_COLOR_LIGHT_MAGENTA);
            }
        }
        
        // Новый факт въезжает справа
        if (pos + new_len > 0) {
            int start = (pos < 0) ? -pos : 0;
            int len = new_len - start;
            if (pos + len > VGA_WIDTH) {
                len = VGA_WIDTH - pos;
            }
            
            if (len > 0) {
                char buffer[VGA_WIDTH + 1];
                strncpy(buffer, new_fact + start, len);
                buffer[len] = 0;
                terminal_print_at(buffer, 
                    (pos > 0) ? pos : 0, 
                    3, 
                    VGA_COLOR_LIGHT_MAGENTA);
            }
        }
        
        for (volatile int i = 0; i < 3000; i++);
    }
    */
//}

// Обновим terminal_init:
void terminal_init(void) {
    vga_clear();
    
    // Баннер сверху
    draw_banner();
    
    // Промпт в самом низу
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
        terminal_print("File System:\n", VGA_COLOR_CYAN);
        terminal_print("  ls              - List files\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  cd <dir>        - Change directory\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  mkdir <name>    - Create directory\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  rm <name>       - Delete file\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  rmdir <name>    - Delete directory\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  pwd             - Current path\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("\nEditor:\n", VGA_COLOR_CYAN);
        terminal_print("  edit [file]     - Open editor\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  files           - List all files\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  clear           - Clear screen\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("\nGooseScript:\n", VGA_COLOR_CYAN);
        terminal_print("  compile file.goo - Compile to .goobin\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  run file.goo     - Run program\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("  about           - About GooseScript\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("\nSystem:\n", VGA_COLOR_CYAN);
        terminal_print("  reboot          - Reboot system\n", VGA_COLOR_LIGHT_GRAY);
        terminal_print("\n", VGA_COLOR_WHITE);
    }
    else if (strcmp(cmd, "calc") == 0) {
    terminal_start_calculator();
}
    else if (strcmp(cmd, "ls") == 0) {
        fs_list();
    }
    else if (strncmp(cmd, "cd ", 3) == 0) {
        const char* dir = cmd + 3;
        if (strlen(dir) == 0) {
            terminal_print("Usage: cd <directory>\n", VGA_COLOR_RED);
        } else if (fs_cd(dir)) {
            terminal_print("Directory changed\n", VGA_COLOR_GREEN);
        } else {
            terminal_print("Directory not found\n", VGA_COLOR_RED);
        }
    }
    else if (strncmp(cmd, "mkdir ", 6) == 0) {
        const char* dirname = cmd + 6;
        if (strlen(dirname) == 0) {
            terminal_print("Usage: mkdir <name>\n", VGA_COLOR_RED);
        } else if (fs_mkdir(dirname)) {
            terminal_print("Directory created\n", VGA_COLOR_GREEN);
        } else {
            terminal_print("Failed to create directory\n", VGA_COLOR_RED);
        }
    }
    else if (strncmp(cmd, "rm ", 3) == 0) {
        const char* filename = cmd + 3;
        if (strlen(filename) == 0) {
            terminal_print("Usage: rm <filename>\n", VGA_COLOR_RED);
        } else if (fs_delete(filename)) {
            terminal_print("File deleted\n", VGA_COLOR_GREEN);
        } else {
            terminal_print("File not found\n", VGA_COLOR_RED);
        }
    }
    else if (strncmp(cmd, "rmdir ", 6) == 0) {
        const char* dirname = cmd + 6;
        if (strlen(dirname) == 0) {
            terminal_print("Usage: rmdir <directory>\n", VGA_COLOR_RED);
        } else if (fs_rmdir(dirname)) {
            terminal_print("Directory deleted\n", VGA_COLOR_GREEN);
        } else {
            terminal_print("Directory not found or not empty\n", VGA_COLOR_RED);
        }
    }
    else if (strcmp(cmd, "pwd") == 0) {
        char* path = fs_pwd();
        if (path) {
            terminal_print(path, VGA_COLOR_CYAN);
            terminal_print("\n", VGA_COLOR_WHITE);
        }
    }
    else if (strcmp(cmd, "edit") == 0) {
        // Открываем редактор без файла
        strcpy(current_file, "untitled.goo");
        editor_pos = 0;
        memset(editor_buffer, 0, sizeof(editor_buffer));
        terminal_start_editor();
        return; // Не показываем промпт - перешли в редактор
    }
    else if (strncmp(cmd, "edit ", 5) == 0) {
        const char* filename = cmd + 5;
        if (strlen(filename) == 0) {
            terminal_print("Usage: edit <filename>\n", VGA_COLOR_RED);
        } else {
            uint8_t buffer[4096];
            int bytes = fs_read(filename, buffer, sizeof(buffer));
            if (bytes > 0) {
                strcpy(current_file, filename);
                memcpy(editor_buffer, buffer, bytes);
                editor_pos = bytes;
                editor_buffer[editor_pos] = 0;
                terminal_start_editor();
                return; // Не показываем промпт
            } else {
                terminal_print("File not found, creating new\n", VGA_COLOR_YELLOW);
                strcpy(current_file, filename);
                editor_pos = 0;
                memset(editor_buffer, 0, sizeof(editor_buffer));
                terminal_start_editor();
                return; // Не показываем промпт
            }
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
    else if (strncmp(cmd, "compile ", 8) == 0) {
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
      else if (strncmp(cmd, "run ", 4) == 0) {
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
    
    // ===== ПРАВАЯ ПАНЕЛЬ С СООБЩЕНИЕМ =====
    // Верхняя граница правой панели
    terminal_print_at("+-----------------------------------+", 42, 5 + BANNER_HEIGHT, VGA_COLOR_BLUE);
    
    // Заголовок правой панели
    terminal_print_at("|      GooseScript Compiler       |", 42, 6 + BANNER_HEIGHT, VGA_COLOR_BLUE);
    terminal_print_at("+-----------------------------------+", 42, 7 + BANNER_HEIGHT, VGA_COLOR_BLUE);
    
    // Сообщение о статусе
    terminal_print_at("| STATUS: TEMPORARILY DISABLED   |", 42, 8 + BANNER_HEIGHT, VGA_COLOR_RED);
    terminal_print_at("|                                |", 42, 9 + BANNER_HEIGHT, VGA_COLOR_BLUE);
    terminal_print_at("| The GooseScript compiler is    |", 42, 10 + BANNER_HEIGHT, VGA_COLOR_WHITE);
    terminal_print_at("| currently unstable. We're      |", 42, 11 + BANNER_HEIGHT, VGA_COLOR_WHITE);
    terminal_print_at("| working on fixes!              |", 42, 12 + BANNER_HEIGHT, VGA_COLOR_WHITE);
    terminal_print_at("|                                |", 42, 13 + BANNER_HEIGHT, VGA_COLOR_BLUE);
    terminal_print_at("| Use the built-in commands:     |", 42, 14 + BANNER_HEIGHT, VGA_COLOR_CYAN);
    terminal_print_at("|  • ls    - List files          |", 42, 15 + BANNER_HEIGHT, VGA_COLOR_LIGHT_GRAY);
    terminal_print_at("|  • cat   - View file           |", 42, 16 + BANNER_HEIGHT, VGA_COLOR_LIGHT_GRAY);
    terminal_print_at("|  • edit  - Edit file           |", 42, 17 + BANNER_HEIGHT, VGA_COLOR_LIGHT_GRAY);
    terminal_print_at("|  • clear - Clear screen        |", 42, 18 + BANNER_HEIGHT, VGA_COLOR_LIGHT_GRAY);
    terminal_print_at("|                                |", 42, 19 + BANNER_HEIGHT, VGA_COLOR_BLUE);
    terminal_print_at("| Check back later for updates!  |", 42, 20 + BANNER_HEIGHT, VGA_COLOR_YELLOW);
    terminal_print_at("+-----------------------------------+", 42, 21 + BANNER_HEIGHT, VGA_COLOR_BLUE);
    
    // Боковые границы правой панели
    for (int y = 8; y < 21; y++) {
        vga_putchar('|', VGA_COLOR_BLUE, 42, y + BANNER_HEIGHT);
        vga_putchar('|', VGA_COLOR_BLUE, 77, y + BANNER_HEIGHT);
    }
    
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
    // Очищаем строку статус-бара (последняя строка)
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_putchar(' ', VGA_COLOR_BLACK, x, VGA_HEIGHT - 1);
    }
    
    char status[64];
    strcpy(status, "File: ");
    strcat(status, current_file);
    strcat(status, " | Lines: ");
    
    // Считаем строки
    int lines = 1;
    for (uint32_t i = 0; i < editor_pos; i++) {
        if (editor_buffer[i] == '\n') lines++;
    }
    
    char line_str[16];
    itoa(lines, line_str, 10);
    strcat(status, line_str);
    
    // Статус бар внизу экрана
    terminal_print_at(status, 1, VGA_HEIGHT - 1, VGA_COLOR_LIGHT_GRAY);
    terminal_print_at("Ctrl+Q=Exit | GooseScript: DISABLED", 50, VGA_HEIGHT - 1, VGA_COLOR_DARK_GRAY);
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
        // Мигающее сообщение в правой панели
        terminal_print_at("|      *** NOT AVAILABLE ***      |", 42, 8 + BANNER_HEIGHT, VGA_COLOR_RED);
        
        // Ждем немного
        for (volatile int i = 0; i < 500000; i++);
        
        // Возвращаем обычное сообщение
        terminal_print_at("| STATUS: TEMPORARILY DISABLED   |", 42, 8 + BANNER_HEIGHT, VGA_COLOR_RED);
        
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
    // Показываем диалог сохранения (предпоследняя строка)
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
            
            // Добавляем расширение .goo если нет расширения
            char* dot = strrchr(current_file, '.');
            if (!dot) {
                strcat(current_file, ".goo");
            }
            
            // ВАЖНО: Сохраняем ВСЁ содержимое editor_buffer
            // editor_pos - реальная длина текста
            int success = fs_create(current_file, (uint8_t*)editor_buffer, editor_pos, 1);  // 1 = файл
            
            if (success) {
                terminal_print_at(" Saved!             ", 20, dialog_y, VGA_COLOR_GREEN);
            } else {
                terminal_print_at(" Save failed!       ", 20, dialog_y, VGA_COLOR_RED);
            }
            
            // Ждём немного
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

void terminal_run_goo(void) {
    terminal_print_at("Testing terminal_print...", 42, 6 + BANNER_HEIGHT, VGA_COLOR_YELLOW);
    
    // Сохраняем позицию курсора
    uint32_t saved_x = cursor_x;
    uint32_t saved_y = cursor_y;
    
    // Переходим на новую строку для вывода
    cursor_x = 0;
    cursor_y = VGA_HEIGHT - 10;
    
    // Пробуем напечатать напрямую
    terminal_print("DIRECT PRINT TEST: ", VGA_COLOR_RED);
    terminal_print("Hello!", VGA_COLOR_GREEN);
    terminal_print("\n", VGA_COLOR_WHITE);
    
    // Восстанавливаем курсор
    cursor_x = saved_x;
    cursor_y = saved_y;
    
    // Теперь тест VM
    terminal_print_at("Now testing VM...", 42, 7 + BANNER_HEIGHT, VGA_COLOR_YELLOW);
    // ВКЛЮЧАЕМ ОТЛАДКУ!
    terminal_print_at("=== DEBUG START ===", 42, 6 + BANNER_HEIGHT, VGA_COLOR_YELLOW);
    
    // 1. Тестовая программа вручную
    uint8_t binary[256];
    uint32_t pos = 0;
    
    terminal_print_at("Creating test program...", 42, 7 + BANNER_HEIGHT, VGA_COLOR_CYAN);
    
    // Простейшая программа: PUSH 65, SYSCALL PRINT_INT (должно напечатать 'A')
    binary[pos++] = OP_PUSH;
    int num = 65; // ASCII 'A'
    memcpy(binary + pos, &num, 4);
    pos += 4;
    
    binary[pos++] = OP_SYSCALL;
    binary[pos++] = SYS_PRINT_INT;
    
    // Exit
    num = 0;
    binary[pos++] = OP_PUSH;
    memcpy(binary + pos, &num, 4);
    pos += 4;
    
    binary[pos++] = OP_SYSCALL;
    binary[pos++] = SYS_EXIT;
    binary[pos++] = OP_HALT;
    
    // Покажем размер программы
    char debug[64];
    itoa(pos, debug, 10);
    terminal_print_at("Program size: ", 42, 8 + BANNER_HEIGHT, VGA_COLOR_CYAN);
    terminal_print_at(debug, 57, 8 + BANNER_HEIGHT, VGA_COLOR_WHITE);
    
    // 2. Создаём VM
    terminal_print_at("Creating VM...", 42, 9 + BANNER_HEIGHT, VGA_COLOR_CYAN);
    GooVM* vm = goovm_create();
    
    if (!vm) {
        terminal_print_at("FAIL: VM create failed!", 42, 10 + BANNER_HEIGHT, VGA_COLOR_RED);
        return;
    }
    
    terminal_print_at("OK: VM created", 42, 10 + BANNER_HEIGHT, VGA_COLOR_GREEN);
    
    // 3. Загружаем программу
    terminal_print_at("Loading program...", 42, 11 + BANNER_HEIGHT, VGA_COLOR_CYAN);
    if (!goovm_load(vm, binary, pos)) {
        terminal_print_at("FAIL: Load failed!", 42, 12 + BANNER_HEIGHT, VGA_COLOR_RED);
        goovm_destroy(vm);
        return;
    }
    
    terminal_print_at("OK: Program loaded", 42, 12 + BANNER_HEIGHT, VGA_COLOR_GREEN);
    
    // 4. Исполняем
    terminal_print_at("Executing...", 42, 13 + BANNER_HEIGHT, VGA_COLOR_CYAN);
    
    // Сохраняем текущую позицию курсора для вывода программы
    uint32_t old_cursor_x = cursor_x;
    uint32_t old_cursor_y = cursor_y;
    
    // Перемещаем курсор туда, где будет вывод программы
    cursor_x = 0;
    cursor_y = VGA_HEIGHT - 5; // Выше промпта
    
    terminal_print("[PROGRAM OUTPUT START]\n", VGA_COLOR_YELLOW);
    
    // ВЫПОЛНЯЕМ!
    int result = goovm_execute(vm);
    
    terminal_print("\n[PROGRAM OUTPUT END]\n", VGA_COLOR_YELLOW);
    
    // Покажем результат
    itoa(result, debug, 10);
    terminal_print_at("Exit code: ", 42, 14 + BANNER_HEIGHT, VGA_COLOR_CYAN);
    terminal_print_at(debug, 53, 14 + BANNER_HEIGHT, VGA_COLOR_WHITE);
    
    // 5. Очищаем
    goovm_destroy(vm);
    terminal_print_at("VM destroyed", 42, 15 + BANNER_HEIGHT, VGA_COLOR_GREEN);
    
    terminal_print_at("=== DEBUG END ===", 42, 16 + BANNER_HEIGHT, VGA_COLOR_YELLOW);
    
    // Восстанавливаем курсор
    cursor_x = old_cursor_x;
    cursor_y = old_cursor_y;
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
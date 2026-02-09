#include "vga.h"
#include "keyboard.h"
#include "terminal.h"
#include "fs.h"
#include "libc.h"
#include "cmos.h"
#include "graphics.h"

void kernel_main(uint32_t magic, void* mb_info) {
    (void)magic;
    (void)mb_info;
    
    keyboard_init();
//    fs_init();          // Инициализация файловой системы
    terminal_init();    // Инициализация терминала (после FS)
    
    // Главный цикл
    while (1) {
        char key = keyboard_getch();
        if (key) {
            terminal_handle_input(key);
        }
        
        terminal_update();
        
        // Простая задержка
        for (volatile int i = 0; i < 5000; i++);
    }
}

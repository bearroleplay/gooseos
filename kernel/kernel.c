#include "vga.h"
#include "keyboard.h"
#include "terminal.h"
#include "fs.h"
#include "libc.h"
#include "cmos.h"

void kernel_main(uint32_t magic, void* mb_info) {
    (void)magic;
    (void)mb_info;
    
    vga_init();
    keyboard_init();
    fs_init(); // <-- ВАЖНО: Инициализируем файловую систему ПЕРВОЙ!
    terminal_init(); // <-- Теперь терминал знает про FS
    
    // Главный цикл
    while (1) {
        char key = keyboard_getch();
        if (key) {
            terminal_handle_input(key);
        }
        
        terminal_update();
        
        for (volatile int i = 0; i < 5000; i++);
    }
}
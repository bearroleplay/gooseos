#include "vga.h"
#include "keyboard.h"
#include "terminal.h"
#include "fs.h"
#include "libc.h"
#include "cmos.h"
#include "graphics.h"  // ДОБАВЬ ЭТУ СТРОКУ

void kernel_main(uint32_t magic, void* mb_info) {
    (void)magic;
    (void)mb_info;
    
    // Переключаемся в графический режим
    vga_set_mode_13h();
    
    // Устанавливаем палитру
    for (int i = 0; i < 16; i++) {
        vga_set_palette(i, 
            (i & 1) ? 42 : 0,
            (i & 2) ? 42 : 0,
            (i & 4) ? 42 : 0);
    }
    
    // Рисуем тестовую графику
    clear_screen(COLOR_BLUE);
    fill_rect(50, 50, 100, 80, COLOR_GREEN);
    draw_rect(55, 55, 90, 70, COLOR_RED);
    draw_line(0, 0, 319, 199, COLOR_WHITE);
    draw_line(319, 0, 0, 199, COLOR_WHITE);
    draw_circle(160, 100, 40, COLOR_YELLOW);
    
    // Инициализируем клавиатуру
    keyboard_init();
    
    // Ждём нажатия 'q' для возврата в терминал
    while (1) {
        char key = keyboard_getch();
        if (key == 'q') {
            vga_set_mode_text();  // Возвращаемся в текстовый режим
            
            // Инициализируем терминал
            vga_init();
            fs_init();
            terminal_init();
            terminal_show_prompt();
            
            // Запускаем терминал
            while (1) {
                char key = keyboard_getch();
                if (key) terminal_handle_input(key);
                terminal_update();
                for (volatile int i = 0; i < 5000; i++);
            }
        }
    }
}

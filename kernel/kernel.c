#include "vga.h"
#include "keyboard.h"
#include "terminal.h"
#include "fs.h"
#include "libc.h"
#include "cmos.h"
#include "graphics.h"  // <-- ДОБАВЬ

void kernel_main(uint32_t magic, void* mb_info) {
    (void)magic;
    (void)mb_info;
    
    // Включаем графический режим!
    vga_set_mode_13h();
    
    // Устанавливаем стандартную VGA палитру
    for (int i = 0; i < 16; i++) {
        // Простая палитра (можно улучшить)
        vga_set_palette(i, 
            (i & 1) ? 63 : 0,        // R
            (i & 2) ? 63 : 0,        // G  
            (i & 4) ? 63 : 0);       // B
    }
    
    // Очищаем экран синим
    clear_screen(COLOR_BLUE);
    
    // Рисуем тестовую графику
    fill_rect(50, 50, 100, 80, COLOR_GREEN);
    draw_rect(55, 55, 90, 70, COLOR_RED);
    draw_line(0, 0, 319, 199, COLOR_WHITE);
    draw_line(319, 0, 0, 199, COLOR_WHITE);
    draw_circle(160, 100, 40, COLOR_YELLOW);
    
    // Текстовая надпись (пока простыми пикселями)
    for (int i = 0; i < 10; i++) {
        draw_pixel(150 + i, 180, COLOR_WHITE);
        draw_pixel(150 + i, 181, COLOR_WHITE);
    }
    
    // Ждём нажатия клавиши
    keyboard_init();
    
    // Бесконечный цикл
    while (1) {
        char key = keyboard_getch();
        if (key == 'q') {
            // Возвращаемся в текстовый режим
            vga_set_mode_text();
            
            // Инициализируем терминал
            fs_init();
            terminal_init();
            
            // Запускаем обычный терминал
            while (1) {
                char key = keyboard_getch();
                if (key) terminal_handle_input(key);
                terminal_update();
                for (volatile int i = 0; i < 5000; i++);
            }
        }
        
        // Простая анимация
        static int x = 0;
        draw_pixel(x % 320, 190, COLOR_WHITE);
        x++;
        
        for (volatile int i = 0; i < 10000; i++);
    }
}

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
    
    // Переключаемся в графический режим
    vga_set_mode_13h();
    
    // Очищаем экран синим (цвет 1 в VGA)
    clear_screen(1);
    
    // Рисуем тестовую графику
    fill_rect(50, 50, 100, 80, 2);      // Зелёный
    draw_rect(55, 55, 90, 70, 4);       // Красный
    draw_line(0, 0, 319, 199, 15);      // Белый
    draw_line(319, 0, 0, 199, 15);      // Белый
    draw_circle(160, 100, 40, 14);      // Жёлтый
    draw_text(120, 180, "GOOSE OS", 15); // Белый текст
    
    // Инициализируем клавиатуру
    keyboard_init();
    
    // Ждём нажатия
    while (1) {
        char key = keyboard_getch();
        if (key == 't' || key == 'T') {
            vga_set_mode_text();
            break;
        }
    }
    
    // Возвращаемся в терминал
    vga_init();
    fs_init();
    terminal_init();
    
    while (1) {
        char key = keyboard_getch();
        if (key) terminal_handle_input(key);
        terminal_update();
        for (volatile int i = 0; i < 5000; i++);
    }
}

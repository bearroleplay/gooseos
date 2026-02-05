#include "graphics.h"
#include "libc.h"

// Видеопамять для режима 13h (320x200, 256 цветов)
uint8_t* gfx_buffer = (uint8_t*)0xA0000;
int gfx_width = 320;
int gfx_height = 200;

// Простая функция abs
static int my_abs(int x) {
    return (x < 0) ? -x : x;
}

// Установка режима 13h (320x200, 256 цветов) - ПРОСТАЯ
void vga_set_mode_13h(void) {
    // Используем чистый asm без сложных constraints
    __asm__ volatile(
        "mov $0x13, %ax\n"
        "int $0x10"
    );
}

// Возврат в текстовый режим (80x25)
void vga_set_mode_text(void) {
    __asm__ volatile(
        "mov $0x03, %ax\n"
        "int $0x10"
    );
}

// УДАЛИЛИ vga_set_palette - не нужно для базовой работы

// Заливка экрана цветом
void clear_screen(uint8_t color) {
    for (int i = 0; i < 320 * 200; i++) {
        gfx_buffer[i] = color;
    }
}

// Рисование пикселя
void draw_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < gfx_width && y >= 0 && y < gfx_height) {
        gfx_buffer[y * gfx_width + x] = color;
    }
}

// Рисование линии
void draw_line(int x1, int y1, int x2, int y2, uint8_t color) {
    int dx = my_abs(x2 - x1);
    int dy = my_abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    while(1) {
        draw_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = err * 2;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

// Рисование прямоугольника (контур)
void draw_rect(int x, int y, int w, int h, uint8_t color) {
    for (int i = 0; i < w; i++) {
        draw_pixel(x + i, y, color);
        draw_pixel(x + i, y + h - 1, color);
    }
    for (int i = 1; i < h - 1; i++) {
        draw_pixel(x, y + i, color);
        draw_pixel(x + w - 1, y + i, color);
    }
}

// Рисование залитого прямоугольника
void fill_rect(int x, int y, int w, int h, uint8_t color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            draw_pixel(x + j, y + i, color);
        }
    }
}

// Рисование круга
void draw_circle(int x0, int y0, int radius, uint8_t color) {
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        draw_pixel(x0 + x, y0 + y, color);
        draw_pixel(x0 + y, y0 + x, color);
        draw_pixel(x0 - y, y0 + x, color);
        draw_pixel(x0 - x, y0 + y, color);
        draw_pixel(x0 - x, y0 - y, color);
        draw_pixel(x0 - y, y0 - x, color);
        draw_pixel(x0 + y, y0 - x, color);
        draw_pixel(x0 + x, y0 - y, color);
        
        y += 1;
        err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) {
            x -= 1;
            err += 1 - 2 * x;
        }
    }
}

// Простой вывод текста (8x8 пикселей на символ)
void draw_char(int x, int y, char c, uint8_t color) {
    // Простейший шрифт: только заглавные буквы A-Z
    switch(c) {
        case 'A': case 'a':
            draw_line(x+2, y, x+6, y, color);
            draw_line(x+1, y+1, x+1, y+7, color);
            draw_line(x+7, y+1, x+7, y+7, color);
            draw_line(x+2, y+4, x+6, y+4, color);
            break;
        case 'B': case 'b':
            draw_line(x+1, y, x+6, y, color);
            draw_line(x+1, y+7, x+6, y+7, color);
            draw_line(x+1, y, x+1, y+7, color);
            draw_line(x+7, y+1, x+7, y+3, color);
            draw_line(x+7, y+4, x+7, y+6, color);
            draw_line(x+6, y+3, x+6, y+4, color);
            break;
        case 'G': case 'g':
            draw_line(x+2, y, x+6, y, color);
            draw_line(x+1, y+1, x+1, y+6, color);
            draw_line(x+2, y+7, x+6, y+7, color);
            draw_line(x+7, y+4, x+7, y+6, color);
            draw_line(x+5, y+4, x+6, y+4, color);
            break;
        case 'O': case 'o':
            draw_line(x+2, y, x+6, y, color);
            draw_line(x+1, y+1, x+1, y+6, color);
            draw_line(x+7, y+1, x+7, y+6, color);
            draw_line(x+2, y+7, x+6, y+7, color);
            break;
        case 'S': case 's':
            draw_line(x+2, y, x+6, y, color);
            draw_line(x+1, y+1, x+1, y+3, color);
            draw_line(x+2, y+4, x+6, y+4, color);
            draw_line(x+7, y+5, x+7, y+6, color);
            draw_line(x+1, y+7, x+6, y+7, color);
            break;
        default:
            // Прямоугольник для неизвестных символов
            draw_rect(x, y, 8, 8, color);
            break;
    }
}

void draw_text(int x, int y, const char* text, uint8_t color) {
    int pos = 0;
    while (*text && pos < 40) {
        draw_char(x + pos * 9, y, *text, color);
        text++;
        pos++;
    }
}

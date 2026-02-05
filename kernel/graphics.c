#include "graphics.h"
#include "libc.h"

// VGA порты
#define VGA_AC_INDEX 0x3C0
#define VGA_MISC_WRITE 0x3C2
#define VGA_SEQ_INDEX 0x3C4
#define VGA_SEQ_DATA 0x3C5
#define VGA_DAC_READ_INDEX 0x3C7
#define VGA_DAC_WRITE_INDEX 0x3C8
#define VGA_DAC_DATA 0x3C9
#define VGA_GC_INDEX 0x3CE
#define VGA_GC_DATA 0x3CF
#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA 0x3D5
#define VGA_INSTAT_READ 0x3DA

// Видеопамять для режима 13h (320x200, 256 цветов)
uint8_t* gfx_buffer = (uint8_t*)0xA0000;
int gfx_width = 320;
int gfx_height = 200;

// Простая функция abs для целых чисел
static int my_abs(int x) {
    return (x < 0) ? -x : x;
}

// Установка режима 13h (320x200, 256 цветов)
void vga_set_mode_13h(void) {
    __asm__ volatile(
        "mov $0x13, %%ax\n"
        "int $0x10"
        : : : "ax"
    );
}

// Возврат в текстовый режим (80x25)
void vga_set_mode_text(void) {
    __asm__ volatile(
        "mov $0x03, %%ax\n"
        "int $0x10"
        : : : "ax"
    );
}

// Установка палитры VGA (исправленный ассемблер)
void vga_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    // VGA использует 6 бит на цвет (0-63)
    uint8_t vga_r = r >> 2;
    uint8_t vga_g = g >> 2;
    uint8_t vga_b = b >> 2;
    
    __asm__ volatile(
        "movb %0, %%al\n"
        "movw $0x3C8, %%dx\n"
        "outb %%al, %%dx\n"
        "movb %1, %%al\n"
        "incw %%dx\n"
        "outb %%al, %%dx\n"
        "movb %2, %%al\n"
        "outb %%al, %%dx\n"
        "movb %3, %%al\n"
        "outb %%al, %%dx"
        : 
        : "r"(index), "r"(vga_r), "r"(vga_g), "r"(vga_b)
        : "al", "dx"
    );
}

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

// Рисование линии (алгоритм Брезенхэма)
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
    // Верхняя линия
    for (int i = x; i < x + w; i++) {
        draw_pixel(i, y, color);
    }
    // Нижняя линия
    for (int i = x; i < x + w; i++) {
        draw_pixel(i, y + h - 1, color);
    }
    // Левая линия
    for (int i = y; i < y + h; i++) {
        draw_pixel(x, i, color);
    }
    // Правая линия
    for (int i = y; i < y + h; i++) {
        draw_pixel(x + w - 1, i, color);
    }
}

// Рисование залитого прямоугольника
void fill_rect(int x, int y, int w, int h, uint8_t color) {
    for (int i = y; i < y + h; i++) {
        for (int j = x; j < x + w; j++) {
            draw_pixel(j, i, color);
        }
    }
}

// Рисование круга (алгоритм средней точки)
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

// Простая функция для вывода текста (заглушка)
void draw_char(int x, int y, char c, uint8_t color) {
    // Простая реализация - только буквы A и B
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
        default:
            // Прямоугольник для неизвестных символов
            draw_rect(x, y, 8, 8, color);
            break;
    }
}

void draw_text(int x, int y, const char* text, uint8_t color) {
    int pos = 0;
    while (*text) {
        draw_char(x + pos * 9, y, *text, color);
        text++;
        pos++;
        if (x + pos * 9 > gfx_width - 8) break;
    }
}

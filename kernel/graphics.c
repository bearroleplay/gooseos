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

// Видеопамять для режима 13h
uint8_t* vga_buffer = (uint8_t*)0xA0000;
int screen_width = 320;
int screen_height = 200;

// Установка режима 13h (320x200, 256 цветов)
void vga_set_mode_13h(void) {
    __asm__ volatile(
        "mov $0x13, %%ax\n"
        "int $0x10\n"
        : : : "ax"
    );
}

// Возврат в текстовый режим (80x25)
void vga_set_mode_text(void) {
    __asm__ volatile(
        "mov $0x03, %%ax\n"
        "int $0x10\n"
        : : : "ax"
    );
}

// Установка палитры VGA
void vga_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    __asm__ volatile(
        "mov %0, %%dx\n"
        "mov %1, %%al\n"
        "out %%al, %%dx\n"
        "mov %2, %%al\n"
        "out %%al, %%dx\n"
        "mov %3, %%al\n"
        "out %%al, %%dx\n"
        "mov %4, %%al\n"
        "out %%al, %%dx\n"
        : 
        : "i"((uint16_t)VGA_DAC_WRITE_INDEX),
          "r"((uint8_t)index),
          "r"((uint8_t)(r >> 2)),  // VGA использует 6 бит на цвет
          "r"((uint8_t)(g >> 2)),
          "r"((uint8_t)(b >> 2))
        : "dx", "al"
    );
}

// Заливка экрана цветом
void clear_screen(uint8_t color) {
    memset(vga_buffer, color, 320 * 200);
}

// Рисование пикселя
void draw_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < 320 && y >= 0 && y < 200) {
        vga_buffer[y * 320 + x] = color;
    }
}

// Рисование линии
void draw_line(int x1, int y1, int x2, int y2, uint8_t color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
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

// Рисование прямоугольника
void draw_rect(int x, int y, int w, int h, uint8_t color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            draw_pixel(x + j, y + i, color);
        }
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
        
        if (err <= 0) {
            y += 1;
            err += 2*y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2*x + 1;
        }
    }
}

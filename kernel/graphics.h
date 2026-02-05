#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

// УБРАТЬ эти строки - они конфликтуют с vga.h
// extern uint8_t* vga_buffer;
// extern int screen_width;
// extern int screen_height;

// Вместо этого объявим свои переменные с другими именами
extern uint8_t* gfx_buffer;      // Графический буфер
extern int gfx_width;
extern int gfx_height;

// Функции
void vga_set_mode_13h(void);
void vga_set_mode_text(void);
void vga_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void clear_screen(uint8_t color);
void draw_pixel(int x, int y, uint8_t color);
void draw_line(int x1, int y1, int x2, int y2, uint8_t color);
void draw_rect(int x, int y, int w, int h, uint8_t color);
void fill_rect(int x, int y, int w, int h, uint8_t color);
void draw_circle(int x0, int y0, int radius, uint8_t color);

// Цвета VGA
#define COLOR_BLACK     0
#define COLOR_BLUE      1
// ... остальные цвета
#define COLOR_WHITE     15

#endif

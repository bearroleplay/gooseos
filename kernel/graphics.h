#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

// Графический буфер
extern uint8_t* gfx_buffer;
extern int gfx_width;
extern int gfx_height;

// Функции
void vga_set_mode_13h(void);
void vga_set_mode_text(void);
void clear_screen(uint8_t color);
void draw_pixel(int x, int y, uint8_t color);
void draw_line(int x1, int y1, int x2, int y2, uint8_t color);
void draw_rect(int x, int y, int w, int h, uint8_t color);
void fill_rect(int x, int y, int w, int h, uint8_t color);
void draw_circle(int x0, int y0, int radius, uint8_t color);
void draw_char(int x, int y, char c, uint8_t color);
void draw_text(int x, int y, const char* text, uint8_t color);

#endif

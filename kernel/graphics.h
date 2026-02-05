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
void vga_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void clear_screen(uint8_t color);
void draw_pixel(int x, int y, uint8_t color);
void draw_line(int x1, int y1, int x2, int y2, uint8_t color);
void draw_rect(int x, int y, int w, int h, uint8_t color);
void fill_rect(int x, int y, int w, int h, uint8_t color);
void draw_circle(int x0, int y0, int radius, uint8_t color);
void draw_char(int x, int y, char c, uint8_t color);
void draw_text(int x, int y, const char* text, uint8_t color);

// Цвета VGA (первые 16 цветов)
#define COLOR_BLACK         0
#define COLOR_BLUE          1
#define COLOR_GREEN         2
#define COLOR_CYAN          3
#define COLOR_RED           4
#define COLOR_MAGENTA       5
#define COLOR_BROWN         6
#define COLOR_LIGHT_GRAY    7
#define COLOR_DARK_GRAY     8
#define COLOR_LIGHT_BLUE    9
#define COLOR_LIGHT_GREEN   10
#define COLOR_LIGHT_CYAN    11
#define COLOR_LIGHT_RED     12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_YELLOW        14
#define COLOR_WHITE         15

#endif

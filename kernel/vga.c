#include "vga.h"

volatile uint16_t* vga_buffer = (volatile uint16_t*)VGA_BUFFER;
static uint32_t vga_row = 0;
static uint32_t vga_column = 0;
static uint8_t vga_color = 0x0F;

void vga_init(void) {
    vga_clear();
}

void vga_clear(void) {
    for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
        for (uint32_t x = 0; x < VGA_WIDTH; x++) {
            vga_putchar(' ', vga_color, x, y);
        }
    }
    vga_row = 0;
    vga_column = 0;
}

void vga_putchar(char c, uint8_t color, uint32_t x, uint32_t y) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) return;
    vga_buffer[y * VGA_WIDTH + x] = (uint16_t)c | ((uint16_t)color << 8);
}

void vga_scroll(void) {
    for (uint32_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (uint32_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    for (uint32_t x = 0; x < VGA_WIDTH; x++) {
        vga_putchar(' ', vga_color, x, VGA_HEIGHT - 1);
    }
}

uint8_t vga_make_color(enum vga_color fg, enum vga_color bg) {
    return fg | (bg << 4);
}

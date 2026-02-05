#include "graphics.h"
#include "libc.h"

// VGA порты
#define VGA_AC_INDEX 0x3C0
// ... остальные дефайны

// Видеопамять для режима 13h (320x200, 256 цветов)
uint8_t* gfx_buffer = (uint8_t*)0xA0000;
int gfx_width = 320;
int gfx_height = 200;

// Установка режима 13h (320x200, 256 цветов)
void vga_set_mode_13h(void) {
    __asm__ volatile(
        "mov $0x13, %%ax\n"
        "int $0x10\n"
        : : : "ax"
    );
}

// ... остальные функции ИЗМЕНИТЬ:
void clear_screen(uint8_t color) {
    memset(gfx_buffer, color, 320 * 200);  // gfx_buffer вместо vga_buffer
}

void draw_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < gfx_width && y >= 0 && y < gfx_height) {
        gfx_buffer[y * gfx_width + x] = color;  // gfx_width вместо screen_width
    }
}

// ... аналогично во всех функциях

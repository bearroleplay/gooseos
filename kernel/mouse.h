#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

// Состояние мыши
typedef struct {
    int x, y;          // Координаты
    int dx, dy;        // Относительное движение
    int buttons;       // Битовые флаги кнопок
    int scroll;        // Колёсико
    int present;       // Мышь обнаружена
    int enabled;       // Мышь включена
} MouseState;

// Цвета для теста
#define MOUSE_COLOR_BG      0x00      // Чёрный
#define MOUSE_COLOR_CURSOR  0x0F      // Белый
#define MOUSE_COLOR_LMB     0x02      // Зелёный
#define MOUSE_COLOR_RMB     0x01      // Синий
#define MOUSE_COLOR_MIDDLE  0x0E      // Жёлтый
#define MOUSE_COLOR_MOVE    0x0B      // Голубой

// Функции
void mouse_init(void);
void mouse_enable(void);
void mouse_disable(void);
MouseState mouse_get_state(void);
void mouse_draw_cursor(void);
void mouse_test_program(void);  // Команда 'mouse'
void mouse_handle_packet(uint8_t packet[3]);

#endif
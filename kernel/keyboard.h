#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

// Добавляем эти функции:
int keyboard_get_layout(void);      // 0=EN, 1=RU
void keyboard_switch_layout(void);  // Переключение раскладки

void keyboard_init(void);
char keyboard_getch(void);
int keyboard_get_shift(void);
int keyboard_get_caps(void);
int keyboard_get_ctrl(void);
int keyboard_get_alt(void);

#endif
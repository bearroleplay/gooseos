#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

void keyboard_init(void);
char keyboard_getch(void);
int keyboard_get_shift(void);
int keyboard_get_caps(void);
int keyboard_get_ctrl(void);   // НОВОЕ!
int keyboard_get_alt(void);    // НОВОЕ!

#endif
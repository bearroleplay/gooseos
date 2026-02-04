#ifndef TERMINAL_H
#define TERMINAL_H

#include "vga.h"

// Режимы
#define MODE_COMMAND 0
#define MODE_EDITOR 1

// Функции терминала
void terminal_init(void);
void terminal_putchar(char c);
void terminal_print(const char* str, uint8_t color);
void terminal_print_at(const char* str, uint32_t x, uint32_t y, uint8_t color);
void terminal_set_cursor(uint32_t x, uint32_t y);
void terminal_newline(void);
void terminal_handle_input(char key);
void terminal_update(void);
void terminal_execute_command(const char* cmd);

// Редактор
void terminal_start_editor(void);
void terminal_editor_handle_key(char key);
void terminal_update_editor(void);
int terminal_get_mode(void);

// Новые функции редактора
void terminal_save_with_prompt(void);
void terminal_run_goo(void);
void terminal_open_file(void);
void terminal_new_file(void);
void terminal_redraw_editor(void);
void update_status_bar(void);
void vga_scroll_editor(void);

// Утилиты
void terminal_clear_command(void);
void terminal_show_prompt(void);

#endif
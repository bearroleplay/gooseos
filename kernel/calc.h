#ifndef CALC_H
#define CALC_H

typedef enum {
    OP_NONE,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV
} CalcOperation;

typedef struct {
    char display[32];          // Увеличим для больших чисел
    int display_len;
    double value;              // Используем double для дробей
    CalcOperation operation;
    int waiting_for_operand;
    int error;                 // Флаг ошибки
} CalcState;

// Новые функции
void calc_init(void);
void calc_input_digit(int digit);
void calc_input_decimal(void);
void calc_perform_operation(void);
void calc_clear(void);
void calc_clear_entry(void);
void calc_backspace(void);
void calc_equals(void);
void calc_set_operation(CalcOperation op);

// Графический интерфейс
void calc_draw_frame(void);
void calc_draw_display(void);
void calc_draw_buttons(void);
void calc_handle_key(char key);

// Запуск калькулятора
void terminal_start_calculator(void);

#endif
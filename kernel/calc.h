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
    char display[16];
    int display_len;
    int value;
    CalcOperation operation;
    int waiting_for_operand;
} CalcState;

void terminal_start_calculator(void);

#endif
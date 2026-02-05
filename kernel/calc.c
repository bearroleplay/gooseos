#include "calc.h"
#include "terminal.h"
#include "keyboard.h"
#include "libc.h"

// Состояние калькулятора
static CalcState calc;

void calc_init(void) {
    calc.display[0] = '0';
    calc.display[1] = '\0';
    calc.value = 0;
    calc.operation = OP_NONE;
    calc.waiting_for_operand = 1;
    calc.display_len = 1;
}

void calc_input_digit(int digit) {
    if (calc.waiting_for_operand) {
        calc.display[0] = '0' + digit;
        calc.display[1] = '\0';
        calc.display_len = 1;
        calc.waiting_for_operand = 0;
    } else {
        if (calc.display_len < 15) {
            calc.display[calc.display_len] = '0' + digit;
            calc.display_len++;
            calc.display[calc.display_len] = '\0';
        }
    }
}

void calc_perform_operation(void) {
    int current = atoi(calc.display);
    
    switch (calc.operation) {
        case OP_ADD:
            calc.value += current;
            break;
        case OP_SUB:
            calc.value -= current;
            break;
        case OP_MUL:
            calc.value *= current;
            break;
        case OP_DIV:
            if (current != 0) {
                calc.value /= current;
            } else {
                // Ошибка деления на ноль
                calc.value = 0;
                strcpy(calc.display, "ERR:DIV0");
                calc.display_len = 8;
                calc.waiting_for_operand = 1;
                return;
            }
            break;
        case OP_NONE:
            calc.value = current;
            break;
    }
    
    // Обновляем дисплей
    itoa(calc.value, calc.display, 10);
    calc.display_len = strlen(calc.display);
}

// Текстовый интерфейс калькулятора
void terminal_start_calculator(void) {
    calc_init();
    
    terminal_print("\n", VGA_COLOR_WHITE);
    terminal_print("╔══════════════════════════╗\n", VGA_COLOR_CYAN);
    terminal_print("║     GOOSE CALCULATOR     ║\n", VGA_COLOR_YELLOW);
    terminal_print("╚══════════════════════════╝\n", VGA_COLOR_CYAN);
    
    terminal_print("\nUsage:\n", VGA_COLOR_WHITE);
    terminal_print("  5 + 3     - Add\n", VGA_COLOR_LIGHT_GRAY);
    terminal_print("  10 * 2    - Multiply\n", VGA_COLOR_LIGHT_GRAY);
    terminal_print("  15 / 3    - Divide\n", VGA_COLOR_LIGHT_GRAY);
    terminal_print("  C         - Clear\n", VGA_COLOR_LIGHT_GRAY);
    terminal_print("  Q         - Quit\n", VGA_COLOR_LIGHT_GRAY);
    
    terminal_print("\nCurrent: ", VGA_COLOR_CYAN);
    terminal_print(calc.display, VGA_COLOR_WHITE);
    terminal_print("\n", VGA_COLOR_CYAN);
    
    char input[32];
    while (1) {
        terminal_print("\ncalc> ", VGA_COLOR_GREEN);
        
        // Читаем ввод
        int pos = 0;
        while (1) {
            char key = keyboard_getch();
            if (key == '\n') {
                input[pos] = '\0';
                break;
            } else if (key == '\b' && pos > 0) {
                pos--;
                terminal_print("\b \b", VGA_COLOR_WHITE);
            } else if (key >= 32 && key <= 126 && pos < 30) {
                input[pos] = key;
                pos++;
                terminal_putchar(key);
            }
        }
        
        // Обработка команд
        if (strcmp(input, "q") == 0 || strcmp(input, "Q") == 0 ||
            strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
            break;
        }
        else if (strcmp(input, "c") == 0 || strcmp(input, "C") == 0 ||
                 strcmp(input, "clear") == 0) {
            calc_init();
            terminal_print("Cleared\n", VGA_COLOR_YELLOW);
        }
        else if (strlen(input) == 0) {
            continue;
        }
        else {
            // Парсим выражение: число операция число
            char* endptr;
            int a = strtol(input, &endptr, 10);
            
            if (*endptr == '\0') {
                // Просто число
                calc.value = a;
                itoa(a, calc.display, 10);
                calc.display_len = strlen(calc.display);
                calc.operation = OP_NONE;
                calc.waiting_for_operand = 0;
                
                terminal_print("= ", VGA_COLOR_CYAN);
                terminal_print(calc.display, VGA_COLOR_WHITE);
                terminal_print("\n", VGA_COLOR_CYAN);
            }
            else if (*endptr == '+' || *endptr == '-' || 
                     *endptr == '*' || *endptr == '/') {
                char op = *endptr;
                endptr++;
                int b = strtol(endptr, NULL, 10);
                
                // Выполняем операцию
                calc.value = a;
                calc.operation = (op == '+') ? OP_ADD :
                                (op == '-') ? OP_SUB :
                                (op == '*') ? OP_MUL : OP_DIV;
                
                // Второе число в дисплей
                itoa(b, calc.display, 10);
                calc.display_len = strlen(calc.display);
                calc.waiting_for_operand = 0;
                
                // Выполняем операцию
                calc_perform_operation();
                
                // Выводим результат
                terminal_print("= ", VGA_COLOR_CYAN);
                if (strcmp(calc.display, "ERR:DIV0") == 0) {
                    terminal_print("ERROR: Division by zero!\n", VGA_COLOR_RED);
                } else {
                    terminal_print(calc.display, VGA_COLOR_WHITE);
                    terminal_print("\n", VGA_COLOR_CYAN);
                }
                
                calc.operation = OP_NONE;
                calc.waiting_for_operand = 1;
            }
            else {
                terminal_print("Format: number [+-*/] number\n", VGA_COLOR_RED);
                terminal_print("Or: C=clear, Q=quit\n", VGA_COLOR_LIGHT_GRAY);
            }
        }
    }
    
    terminal_print("\nCalculator closed.\n", VGA_COLOR_YELLOW);
}
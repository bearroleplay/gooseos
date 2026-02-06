#include "calc.h"
#include "terminal.h"
#include "keyboard.h"
#include "libc.h"
#include <string.h>

// Состояние калькулятора
static CalcState calc;

// Позиции и размеры
#define CALC_X 20
#define CALC_Y 8
#define CALC_WIDTH 40
#define CALC_HEIGHT 16

// Кнопки
typedef struct {
    int x, y;
    int w, h;
    char label[4];
    char key;
    uint8_t color;
} Button;

static Button buttons[] = {
    // Первый ряд
    {CALC_X+2,  CALC_Y+5,  7, 3, "CE",  'c', VGA_COLOR_LIGHT_RED},
    {CALC_X+10, CALC_Y+5,  7, 3, "C",   'C', VGA_COLOR_RED},
    {CALC_X+18, CALC_Y+5,  7, 3, "<-",  '\b', VGA_COLOR_LIGHT_GRAY},
    {CALC_X+26, CALC_Y+5,  7, 3, "/",   '/', VGA_COLOR_LIGHT_MAGENTA},
    
    // Второй ряд
    {CALC_X+2,  CALC_Y+9,  7, 3, "7",   '7', VGA_COLOR_LIGHT_GRAY},
    {CALC_X+10, CALC_Y+9,  7, 3, "8",   '8', VGA_COLOR_LIGHT_GRAY},
    {CALC_X+18, CALC_Y+9,  7, 3, "9",   '9', VGA_COLOR_LIGHT_GRAY},
    {CALC_X+26, CALC_Y+9,  7, 3, "*",   '*', VGA_COLOR_LIGHT_MAGENTA},
    
    // Третий ряд
    {CALC_X+2,  CALC_Y+13, 7, 3, "4",   '4', VGA_COLOR_LIGHT_GRAY},
    {CALC_X+10, CALC_Y+13, 7, 3, "5",   '5', VGA_COLOR_LIGHT_GRAY},
    {CALC_X+18, CALC_Y+13, 7, 3, "6",   '6', VGA_COLOR_LIGHT_GRAY},
    {CALC_X+26, CALC_Y+13, 7, 3, "-",   '-', VGA_COLOR_LIGHT_MAGENTA},
    
    // Четвертый ряд
    {CALC_X+2,  CALC_Y+17, 7, 3, "1",   '1', VGA_COLOR_LIGHT_GRAY},
    {CALC_X+10, CALC_Y+17, 7, 3, "2",   '2', VGA_COLOR_LIGHT_GRAY},
    {CALC_X+18, CALC_Y+17, 7, 3, "3",   '3', VGA_COLOR_LIGHT_GRAY},
    {CALC_X+26, CALC_Y+17, 7, 3, "+",   '+', VGA_COLOR_LIGHT_MAGENTA},
    
    // Пятый ряд
    {CALC_X+2,  CALC_Y+21, 7, 3, "+/-", 'n', VGA_COLOR_LIGHT_GRAY},
    {CALC_X+10, CALC_Y+21, 7, 3, "0",   '0', VGA_COLOR_LIGHT_GRAY},
    {CALC_X+18, CALC_Y+21, 7, 3, ".",   '.', VGA_COLOR_LIGHT_GRAY},
    {CALC_X+26, CALC_Y+21, 7, 3, "=",   '=', VGA_COLOR_LIGHT_GREEN},
};

void calc_init(void) {
    memset(&calc, 0, sizeof(CalcState));
    calc.display[0] = '0';
    calc.display[1] = '\0';
    calc.display_len = 1;
    calc.value = 0;
    calc.operation = OP_NONE;
    calc.waiting_for_operand = 1;
    calc.error = 0;
}

void calc_clear(void) {
    calc_init();
}

void calc_clear_entry(void) {
    calc.display[0] = '0';
    calc.display[1] = '\0';
    calc.display_len = 1;
    calc.waiting_for_operand = 1;
}

void calc_backspace(void) {
    if (calc.error) {
        calc_clear();
        return;
    }
    
    if (calc.display_len > 1) {
        calc.display_len--;
        calc.display[calc.display_len] = '\0';
    } else {
        calc.display[0] = '0';
        calc.display_len = 1;
        calc.waiting_for_operand = 1;
    }
}

void calc_input_digit(int digit) {
    if (calc.error) calc_clear();
    
    if (calc.waiting_for_operand) {
        calc.display[0] = '0' + digit;
        calc.display[1] = '\0';
        calc.display_len = 1;
        calc.waiting_for_operand = 0;
    } else {
        if (calc.display_len < 30) {
            calc.display[calc.display_len] = '0' + digit;
            calc.display_len++;
            calc.display[calc.display_len] = '\0';
        }
    }
}

void calc_input_decimal(void) {
    if (calc.error) calc_clear();
    
    // Проверяем, есть ли уже точка
    for (int i = 0; i < calc.display_len; i++) {
        if (calc.display[i] == '.') return;
    }
    
    if (calc.waiting_for_operand) {
        calc.display[0] = '0';
        calc.display[1] = '.';
        calc.display[2] = '\0';
        calc.display_len = 2;
        calc.waiting_for_operand = 0;
    } else {
        if (calc.display_len < 29) {
            calc.display[calc.display_len] = '.';
            calc.display_len++;
            calc.display[calc.display_len] = '\0';
        }
    }
}

double calc_get_display_value(void) {
    if (calc.error) return 0;
    
    double result = 0;
    double fraction = 0.1;
    int decimal = 0;
    int sign = 1;
    int i = 0;
    
    if (calc.display[0] == '-') {
        sign = -1;
        i = 1;
    }
    
    for (; i < calc.display_len; i++) {
        if (calc.display[i] == '.') {
            decimal = 1;
            continue;
        }
        
        if (!decimal) {
            result = result * 10 + (calc.display[i] - '0');
        } else {
            result += (calc.display[i] - '0') * fraction;
            fraction *= 0.1;
        }
    }
    
    return result * sign;
}

void calc_set_display_value(double value) {
    if (calc.error) return;
    
    // Проверка на ошибку
    if (value != value) { // NaN
        strcpy(calc.display, "ERROR");
        calc.display_len = 5;
        calc.error = 1;
        return;
    }
    
    // Преобразуем double в строку
    char buffer[32];
    int int_part = (int)value;
    double frac_part = value - int_part;
    
    if (frac_part < 0) frac_part = -frac_part;
    
    // Целая часть
    char int_str[32];
    itoa(int_part, int_str, 10);
    
    // Дробная часть (2 знака)
    if (frac_part > 0.0001) {
        int frac = (int)(frac_part * 100 + 0.5);
        char frac_str[3];
        if (frac < 10) {
            frac_str[0] = '0';
            frac_str[1] = '0' + frac;
        } else {
            itoa(frac, frac_str, 10);
        }
        frac_str[2] = '\0';
        
        strcpy(buffer, int_str);
        strcat(buffer, ".");
        strcat(buffer, frac_str);
    } else {
        strcpy(buffer, int_str);
    }
    
    // Копируем в дисплей
    int len = strlen(buffer);
    if (len > 30) len = 30;
    strncpy(calc.display, buffer, len);
    calc.display[len] = '\0';
    calc.display_len = len;
}

void calc_set_operation(CalcOperation op) {
    if (calc.error) return;
    
    if (!calc.waiting_for_operand) {
        calc_perform_operation();
    }
    
    calc.operation = op;
    calc.value = calc_get_display_value();
    calc.waiting_for_operand = 1;
}

void calc_perform_operation(void) {
    if (calc.error) return;
    
    double current = calc_get_display_value();
    
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
            if (current == 0) {
                strcpy(calc.display, "ERR:DIV0");
                calc.display_len = 8;
                calc.error = 1;
                return;
            }
            calc.value /= current;
            break;
        case OP_NONE:
            calc.value = current;
            break;
    }
    
    calc_set_display_value(calc.value);
    calc.operation = OP_NONE;
}

void calc_equals(void) {
    if (calc.error) return;
    
    calc_perform_operation();
    calc.waiting_for_operand = 1;
}

// Графический интерфейс
void calc_draw_frame(void) {
    // Очищаем область калькулятора
    for (int y = CALC_Y; y < CALC_Y + CALC_HEIGHT; y++) {
        for (int x = CALC_X; x < CALC_X + CALC_WIDTH; x++) {
            vga_putchar(' ', VGA_COLOR_BLACK, x, y);
        }
    }
    
    // Рамка с двойной линией
    // Верхняя граница
    terminal_print_at("╔══════════════════════════════════════╗", 
                     CALC_X, CALC_Y, VGA_COLOR_CYAN);
    
    // Боковые границы
    for (int y = 1; y < CALC_HEIGHT - 1; y++) {
        vga_putchar('║', VGA_COLOR_CYAN, CALC_X, CALC_Y + y);
        vga_putchar('║', VGA_COLOR_CYAN, CALC_X + CALC_WIDTH - 1, CALC_Y + y);
    }
    
    // Нижняя граница
    terminal_print_at("╚══════════════════════════════════════╝", 
                     CALC_X, CALC_Y + CALC_HEIGHT - 1, VGA_COLOR_CYAN);
    
    // Заголовок
    terminal_print_at("        GOOSE CALCULATOR v1.0        ", 
                     CALC_X + 2, CALC_Y + 1, VGA_COLOR_YELLOW);
    
    // Нижняя подпись
    terminal_print_at(" Alt+Q: Exit | F1: Help ", 
                     CALC_X + 8, CALC_Y + CALC_HEIGHT - 2, VGA_COLOR_LIGHT_GRAY);
}

void calc_draw_display(void) {
    // Фон дисплея
    for (int x = CALC_X + 2; x < CALC_X + CALC_WIDTH - 2; x++) {
        vga_putchar(' ', VGA_COLOR_DARK_GRAY, x, CALC_Y + 3);
    }
    
    // Текст дисплея (выравниваем по правому краю)
    int display_x = CALC_X + CALC_WIDTH - 3 - calc.display_len;
    if (display_x < CALC_X + 2) display_x = CALC_X + 2;
    
    // Выводим значение
    uint8_t color = calc.error ? VGA_COLOR_RED : VGA_COLOR_WHITE;
    terminal_print_at(calc.display, display_x, CALC_Y + 3, color);
}

void calc_draw_buttons(void) {
    for (int i = 0; i < sizeof(buttons)/sizeof(Button); i++) {
        Button* btn = &buttons[i];
        
        // Рисуем кнопку
        for (int y = 0; y < btn->h; y++) {
            for (int x = 0; x < btn->w; x++) {
                char c = ' ';
                if (y == 0 || y == btn->h-1 || x == 0 || x == btn->w-1) {
                    c = '─';
                    if ((y == 0 || y == btn->h-1) && (x == 0 || x == btn->w-1)) {
                        c = (y == 0 && x == 0) ? '┌' :
                            (y == 0 && x == btn->w-1) ? '┐' :
                            (y == btn->h-1 && x == 0) ? '└' :
                            '┘';
                    }
                }
                vga_putchar(c, btn->color, btn->x + x, btn->y + y);
            }
        }
        
        // Текст кнопки (центрируем)
        int text_x = btn->x + (btn->w - strlen(btn->label)) / 2;
        int text_y = btn->y + btn->h / 2;
        terminal_print_at(btn->label, text_x, text_y, VGA_COLOR_WHITE);
    }
}

void calc_handle_key(char key) {
    // Выход
    if (keyboard_get_alt() && (key == 'q' || key == 'Q')) {
        return;
    }
    
    // Цифры
    if (key >= '0' && key <= '9') {
        calc_input_digit(key - '0');
        return;
    }
    
    // Операции
    switch (key) {
        case '+':
            calc_set_operation(OP_ADD);
            break;
        case '-':
            calc_set_operation(OP_SUB);
            break;
        case '*':
            calc_set_operation(OP_MUL);
            break;
        case '/':
            calc_set_operation(OP_DIV);
            break;
        case '=':
        case '\n':
            calc_equals();
            break;
        case '.':
            calc_input_decimal();
            break;
        case '\b':
            calc_backspace();
            break;
        case 'c':
            calc_clear_entry();
            break;
        case 'C':
            calc_clear();
            break;
        case 'n': // +/- меняет знак
            if (calc.display[0] == '-') {
                // Убираем минус
                for (int i = 1; i <= calc.display_len; i++) {
                    calc.display[i-1] = calc.display[i];
                }
                calc.display_len--;
            } else {
                // Добавляем минус
                if (calc.display_len < 30) {
                    for (int i = calc.display_len; i >= 0; i--) {
                        calc.display[i+1] = calc.display[i];
                    }
                    calc.display[0] = '-';
                    calc.display_len++;
                }
            }
            break;
    }
}

void terminal_start_calculator(void) {
    calc_init();
    
    // Сохраняем старый экран
    uint16_t backup[VGA_WIDTH * VGA_HEIGHT];
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        backup[i] = vga_buffer[i];
    }
    
    // Рисуем калькулятор
    calc_draw_frame();
    calc_draw_display();
    calc_draw_buttons();
    
    // Главный цикл
    while (1) {
        // Обновляем дисплей
        calc_draw_display();
        
        // Обработка клавиш
        char key = keyboard_getch();
        if (key) {
            // Выход по Alt+Q
            if (keyboard_get_alt() && (key == 'q' || key == 'Q')) {
                break;
            }
            
            // F1 - справка
            if (key == 0x3B) { // F1
                terminal_print_at("Calculator Help:                  ", 
                                 CALC_X+2, CALC_Y+20, VGA_COLOR_LIGHT_BLUE);
                terminal_print_at("+, -, *, / - operations           ", 
                                 CALC_X+2, CALC_Y+21, VGA_COLOR_LIGHT_GRAY);
                terminal_print_at("= or Enter - calculate            ", 
                                 CALC_X+2, CALC_Y+22, VGA_COLOR_LIGHT_GRAY);
                terminal_print_at("C - Clear, CE - Clear Entry       ", 
                                 CALC_X+2, CALC_Y+23, VGA_COLOR_LIGHT_GRAY);
                terminal_print_at("Alt+Q - Exit                      ", 
                                 CALC_X+2, CALC_Y+24, VGA_COLOR_LIGHT_GRAY);
                
                // Ждём любую клавишу
                while (!keyboard_getch());
                
                // Восстанавливаем кнопки
                calc_draw_buttons();
            }
            
            calc_handle_key(key);
            calc_draw_buttons(); // Перерисовываем кнопки (может быть подсветка)
        }
        
        // Небольшая задержка
        for (volatile int i = 0; i < 10000; i++);
    }
    
    // Восстанавливаем экран
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = backup[i];
    }
    
    // Возвращаем курсор в командную строку
    mode = MODE_COMMAND;
    terminal_show_prompt();
}
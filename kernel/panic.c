// panic.c - Критические ошибки и паника
#include "panic.h"
#include "vga.h"
#include "libc.h"
#include "beep.h"
#include <stdarg.h>

// Текущий режим паники
static PanicMode panic_mode = PANIC_NONE;
static char panic_message[256];
static uint32_t panic_code = 0;

// Установить цвет для экрана паники
static void set_panic_colors(PanicMode mode) {
    switch(mode) {
        case PANIC_WHITE:
            // Белый экран смерти
            for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
                vga_buffer[i] = (uint16_t)' ' | ((VGA_COLOR_WHITE << 8) | (VGA_COLOR_WHITE << 12));
            }
            break;
            
        case PANIC_RED:
            // Красный экран
            for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
                vga_buffer[i] = (uint16_t)' ' | ((VGA_COLOR_WHITE << 8) | (VGA_COLOR_RED << 12));
            }
            break;
            
        case PANIC_BLUE:
            // Синий экран (классика)
            for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
                vga_buffer[i] = (uint16_t)' ' | ((VGA_COLOR_WHITE << 8) | (VGA_COLOR_BLUE << 12));
            }
            break;
            
        case PANIC_BLACK:
default:
    // Чёрный экран с белым текстом
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = (uint16_t)' ' | ((VGA_COLOR_WHITE << 8) | (VGA_COLOR_BLACK << 12));
    }
    break;
    }
}

// Нарисовать рамку
static void draw_border(PanicMode mode) {
    uint8_t fg = VGA_COLOR_WHITE;
    uint8_t bg;
    
    switch(mode) {
        case PANIC_WHITE: bg = VGA_COLOR_WHITE; fg = VGA_COLOR_BLACK; break;
        case PANIC_RED: bg = VGA_COLOR_RED; break;
        case PANIC_BLUE: bg = VGA_COLOR_BLUE; break;
        default: bg = VGA_COLOR_BLACK; break;
    }
    
    // Верхняя и нижняя рамка
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_putchar('=', vga_make_color(fg, bg), x, 0);
        vga_putchar('=', vga_make_color(fg, bg), x, VGA_HEIGHT-1);
    }
    
    // Боковые рамки
    for (int y = 1; y < VGA_HEIGHT-1; y++) {
        vga_putchar('|', vga_make_color(fg, bg), 0, y);
        vga_putchar('|', vga_make_color(fg, bg), VGA_WIDTH-1, y);
    }
    
    // Углы
    vga_putchar('+', vga_make_color(fg, bg), 0, 0);
    vga_putchar('+', vga_make_color(fg, bg), VGA_WIDTH-1, 0);
    vga_putchar('+', vga_make_color(fg, bg), 0, VGA_HEIGHT-1);
    vga_putchar('+', vga_make_color(fg, bg), VGA_WIDTH-1, VGA_HEIGHT-1);
}

// Вывести текст по центру строки
static void print_centered(int y, const char* text, uint8_t fg, uint8_t bg) {
    int len = strlen(text);
    int x = (VGA_WIDTH - len) / 2;
    
    for (int i = 0; i < len; i++) {
        vga_putchar(text[i], vga_make_color(fg, bg), x + i, y);
    }
}

// Показать экран паники
void panic_show(PanicMode mode, const char* message, uint32_t code) {
    beep_critical();
    panic_mode = mode;
    panic_code = code;
    
    // Копируем сообщение (обрезаем если длинное)
    int i;
    for (i = 0; message[i] && i < 255; i++) {
        panic_message[i] = message[i];
    }
    panic_message[i] = 0;
    
    // Устанавливаем цвета
    set_panic_colors(mode);
    
    // Определяем цвета текста
    uint8_t text_fg, text_bg;
    switch(mode) {
        case PANIC_WHITE:
            text_fg = VGA_COLOR_BLACK;
            text_bg = VGA_COLOR_WHITE;
            break;
        case PANIC_RED:
            text_fg = VGA_COLOR_WHITE;
            text_bg = VGA_COLOR_RED;
            break;
        case PANIC_BLUE:
            text_fg = VGA_COLOR_WHITE;
            text_bg = VGA_COLOR_BLUE;
            break;
        default:
            text_fg = VGA_COLOR_WHITE;
            text_bg = VGA_COLOR_BLACK;
            break;
    }
    
    // Рисуем рамку
    draw_border(mode);
    
    // Заголовок
    const char* title;
    switch(mode) {
        case PANIC_WHITE: title = "GOOSE OS - WHITE SCREEN OF DEATH"; break;
        case PANIC_RED: title = "GOOSE OS - RED ALERT"; break;
        case PANIC_BLUE: title = "GOOSE OS - SYSTEM FAILURE"; break;
        default: title = "GOOSE OS - KERNEL PANIC"; break;
    }
    
    print_centered(2, title, text_fg, text_bg);
    print_centered(3, "========================================", text_fg, text_bg);
    
    // Код ошибки
    char code_str[32];
    strcpy(code_str, "ERROR CODE: 0x");
    char hex[9];
    itoa(code, hex, 16);
    strcat(code_str, hex);
    print_centered(5, code_str, text_fg, text_bg);
    
    // Сообщение (разбиваем на строки если длинное)
    int msg_y = 7;
    char temp[VGA_WIDTH + 1];
    int temp_pos = 0;
    
    for (i = 0; panic_message[i]; i++) {
        if (panic_message[i] == '\n' || temp_pos >= VGA_WIDTH - 10) {
            temp[temp_pos] = 0;
            print_centered(msg_y++, temp, text_fg, text_bg);
            temp_pos = 0;
            
            if (panic_message[i] == '\n') continue;
        }
        
        if (panic_message[i] != '\n') {
            temp[temp_pos++] = panic_message[i];
        }
    }
    
    if (temp_pos > 0) {
        temp[temp_pos] = 0;
        print_centered(msg_y++, temp, text_fg, text_bg);
    }
    
    // Разделитель
    print_centered(msg_y + 1, "----------------------------------------", text_fg, text_bg);
    
    // Инструкции
    print_centered(msg_y + 3, "SYSTEM HALTED", VGA_COLOR_YELLOW, text_bg);
    print_centered(msg_y + 4, "Press Ctrl+Alt+Delete to reboot", text_fg, text_bg);
    print_centered(msg_y + 5, "or power off your computer.", text_fg, text_bg);
    
    // Инфо внизу
    char bottom[64];
    strcpy(bottom, "GooseOS v1.0 | Kernel Panic Handler");
    print_centered(VGA_HEIGHT - 2, bottom, VGA_COLOR_LIGHT_GRAY, text_bg);
    
    // Зависаем навсегда
    __asm__ volatile("cli");
    while(1) {
        __asm__ volatile("hlt");
    }
}

// Быстрые функции для разных типов ошибок
void panic_kernel(const char* message, uint32_t code) {
    panic_show(PANIC_BLUE, message, code);
}

void panic_memory(const char* message, uint32_t code) {
    panic_show(PANIC_RED, message, code);
}

void panic_hardware(const char* message, uint32_t code) {
    panic_show(PANIC_WHITE, message, code);
}

void panic_assert(const char* file, int line, const char* expr) {
    char message[256];
    strcpy(message, "Assertion failed: ");
    strcat(message, expr);
    strcat(message, "\nFile: ");
    strcat(message, file);
    strcat(message, "\nLine: ");
    
    char line_str[16];
    itoa(line, line_str, 10);
    strcat(message, line_str);
    
    panic_show(PANIC_BLUE, message, 0xA55E47);  // "ASSERT" в leet: A55E47
}

// Вспомогательная функция для проверок
void panic_if(int condition, const char* message, uint32_t code) {
    if (condition) {
        panic_kernel(message, code);
    }
}


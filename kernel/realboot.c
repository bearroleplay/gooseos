// realboot.c - РЕАЛЬНАЯ загрузка с прогресс-баром
#include "realboot.h"
#include "vga.h"
#include "libc.h"
#include "panic.h"
#include <string.h>

// Текущий прогресс
static int current_percent = 0;
static char current_message[64] = "";

// Нарисовать заголовок загрузки (вызывается один раз)
static void draw_boot_header(void) {
    // Заголовок
    const char* title = "GOOSE OS v1.0";
    int title_x = (VGA_WIDTH - strlen(title)) / 2;
    for (int i = 0; title[i]; i++) {
        vga_putchar(title[i], VGA_COLOR_CYAN, title_x + i, 5);
    }
    
    // Подзаголовок
    const char* subtitle = "System Boot";
    int sub_x = (VGA_WIDTH - strlen(subtitle)) / 2;
    for (int i = 0; subtitle[i]; i++) {
        vga_putchar(subtitle[i], VGA_COLOR_LIGHT_GRAY, sub_x + i, 6);
    }
    
    // Разделитель
    for (int x = 10; x < VGA_WIDTH - 10; x++) {
        vga_putchar('-', VGA_COLOR_DARK_GRAY, x, 7);
    }
}

// Нарисовать прогресс-бар
static void draw_progress_bar(int percent) {
    // Рамка
    vga_putchar('[', VGA_COLOR_LIGHT_GRAY, 20, 12);
    vga_putchar(']', VGA_COLOR_LIGHT_GRAY, 61, 12);
    
    // Полоска (40 символов)
    int width = 40;
    int filled = (percent * width) / 100;
    
    for (int i = 0; i < width; i++) {
        if (i < filled) {
            vga_putchar('=', VGA_COLOR_GREEN, 21 + i, 12);
        } else {
            vga_putchar('.', VGA_COLOR_DARK_GRAY, 21 + i, 12);
        }
    }
    
    // Проценты справа
    char percent_str[8];
    itoa(percent, percent_str, 10);
    int px = 64;
    for (int i = 0; percent_str[i]; i++) {
        vga_putchar(percent_str[i], VGA_COLOR_WHITE, px + i, 12);
    }
    vga_putchar('%', VGA_COLOR_WHITE, px + strlen(percent_str), 12);
}

// Показать сообщение
static void show_message(const char* message) {
    // Очищаем строку сообщения
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_putchar(' ', VGA_COLOR_BLACK, x, VGA_HEIGHT - 4);
    }
    
    // Выводим по центру
    int len = strlen(message);
    int x = (VGA_WIDTH - len) / 2;
    for (int i = 0; i < len; i++) {
        vga_putchar(message[i], VGA_COLOR_YELLOW, x + i, VGA_HEIGHT - 4);
    }
}

// Начать реальную загрузку
void real_boot_start(void) {
    // Очищаем экран
    vga_clear();
    
    // Рисуем заголовок
    draw_boot_header();
    
    // Начальный прогресс
    current_percent = 0;
    strcpy(current_message, "Starting boot sequence...");
    
    draw_progress_bar(0);
    show_message(current_message);
}

// Обновить прогресс
void real_boot_update(int percent, const char* message) {
    current_percent = percent;
    
    if (message) {
        strncpy(current_message, message, sizeof(current_message)-1);
        current_message[sizeof(current_message)-1] = 0;
    }
    
    draw_progress_bar(percent);
    show_message(current_message);
    
    // Небольшая задержка чтобы увидеть обновление
    for (volatile int i = 0; i < 50000; i++);
}

// Показать ошибку
void real_boot_error(PanicMode mode, const char* message, uint32_t code) {
    // Показываем последний прогресс
    draw_progress_bar(current_percent);
    show_message("ERROR! System halted.");
    
    // Задержка чтобы увидеть
    for (volatile int i = 0; i < 1000000; i++);
    
    // Передаём в систему паники
    panic_show(mode, message, code);
}

// Завершить загрузку
void real_boot_complete(void) {
    real_boot_update(100, "GooseOS ready!");
    
    // Пауза перед переходом
    for (volatile int i = 0; i < 1000000; i++);
    
    // Очищаем экран для терминала
    vga_clear();
}

// Проверка мультибут
int real_check_multiboot(uint32_t magic) {
    real_boot_update(5, "Checking bootloader...");
    
    if (magic != 0x2BADB002) {
        real_boot_error(PANIC_BLUE,
                       "Invalid multiboot signature\n"
                       "Expected: 0x2BADB002\n"
                       "System may be unstable",
                       0xBADB007);
        return 0;
    }
    
    real_boot_update(10, "Bootloader OK");
    return 1;
}

// Проверка памяти (упрощённая)
int real_check_memory(void* mb_info) {
    real_boot_update(15, "Checking memory...");
    
    if (!mb_info) {
        real_boot_error(PANIC_RED,
                       "No memory information\n"
                       "Multiboot structure missing",
                       0x0F0F0F0);
        return 0;
    }
    
    real_boot_update(20, "Memory OK");
    return 1;
}
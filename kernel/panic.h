#ifndef PANIC_H
#define PANIC_H

#include <stdint.h>

// Типы экранов смерти
typedef enum {
    PANIC_NONE = 0,
    PANIC_WHITE,    // Белый экран смерти
    PANIC_RED,      // Красная тревога
    PANIC_BLUE,     // Синий экран (классика)
    PANIC_BLACK     // Чёрный экран
} PanicMode;

#define PANIC_CODE_MEMORY    0xDEADBEEF  // OK
#define PANIC_CODE_CPU       0xBAD0C0DE  // BAD CODE
#define PANIC_CODE_DISK      0xBADF11ED  // BAD FEED
#define PANIC_CODE_DIV0      0xD1V10000  // DIV10000 (D1V1 = DIV1)
#define PANIC_CODE_STACK     0x57ACC000  // STACK000 (57AC = STAC)
#define PANIC_CODE_ASSERT    0xA55E47    // ASSERT (A55E47)

// Основные функции
void panic_show(PanicMode mode, const char* message, uint32_t code);

// Быстрые вызовы
void panic_kernel(const char* message, uint32_t code);
void panic_memory(const char* message, uint32_t code);
void panic_hardware(const char* message, uint32_t code);
void panic_assert(const char* file, int line, const char* expr);
void panic_if(int condition, const char* message, uint32_t code);

// Макрос для assert
#define PANIC_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            panic_assert(__FILE__, __LINE__, #expr); \
        } \
    } while(0)

#endif
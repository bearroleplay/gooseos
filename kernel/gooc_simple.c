#include "gooc_simple.h"
#include "libc.h"
#include "terminal.h"
#include "libc.h"

// Коды операций
#define OP_PUSH      0x01
#define OP_ADD       0x03
#define OP_SUB       0x04
#define OP_MUL       0x05
#define OP_DIV       0x06
#define OP_PRINT     0x07
#define OP_PRINT_NUM 0x08
#define OP_HALT      0xFF

// Простой компилятор GooseScript
int gooc_compile(const char* source, uint8_t* output, uint32_t max_size) {
    uint32_t out_ptr = 0;
    const char* src = source;
    
    while (*src && out_ptr < max_size - 10) {
        // Пропускаем пробелы
        while (*src == ' ' || *src == '\t') src++;
        if (!*src) break;
        
        // Команда print
        if (strncmp(src, "print", 5) == 0) {
            src += 5;
            while (*src == ' ' || *src == '\t') src++;
            
            if (*src == '"') {
                src++;  // Пропускаем кавычку
                output[out_ptr++] = OP_PRINT;
                
                // Копируем строку
                while (*src && *src != '"' && *src != '\n' && out_ptr < max_size - 1) {
                    output[out_ptr++] = *src;
                    src++;
                }
                output[out_ptr++] = 0;  // null-терминатор
                
                if (*src == '"') src++;
            }
        }
        // Команда print_num
        else if (strncmp(src, "print_num", 9) == 0) {
            src += 9;
            output[out_ptr++] = OP_PRINT_NUM;
        }
        // Команда push
        else if (strncmp(src, "push", 4) == 0) {
            src += 4;
            while (*src == ' ' || *src == '\t') src++;
            
            int value = atoi(src);
            output[out_ptr++] = OP_PUSH;
            *(int32_t*)&output[out_ptr] = value;
            out_ptr += 4;
            
            // Пропускаем цифры
            while (*src >= '0' && *src <= '9') src++;
        }
        // Команда add
        else if (strncmp(src, "add", 3) == 0) {
            src += 3;
            output[out_ptr++] = OP_ADD;
        }
        // Команда sub
        else if (strncmp(src, "sub", 3) == 0) {
            src += 3;
            output[out_ptr++] = OP_SUB;
        }
        // Команда mul
        else if (strncmp(src, "mul", 3) == 0) {
            src += 3;
            output[out_ptr++] = OP_MUL;
        }
        // Команда div
        else if (strncmp(src, "div", 3) == 0) {
            src += 3;
            output[out_ptr++] = OP_DIV;
        }
        // Команда exit
        else if (strncmp(src, "exit", 4) == 0) {
            src += 4;
            output[out_ptr++] = OP_HALT;
        }
        
        // Переход к следующей строке
        while (*src && *src != '\n') src++;
        if (*src == '\n') src++;
    }
    
    // Добавляем HALT если его нет
    if (out_ptr == 0 || output[out_ptr - 1] != OP_HALT) {
        output[out_ptr++] = OP_HALT;
    }
    
    return out_ptr;

}

#include "gooc_simple.h"
#include "goovm.h"
#include "libc.h"
#include <string.h>

// Простой компилятор GooseScript → GooVM байткод
int gooc_compile(const char* source, uint8_t* output, uint32_t max_size) {
    (void)max_size; // Пока не используем
    
    uint32_t pos = sizeof(GooHeader);     // Позиция в секции кода
    uint32_t data_pos = 1024;             // Данные начинаются с 1024 байта
    
    // Копируем исходник для strtok (strtok модифицирует строку!)
    char* source_copy = (char*)malloc(strlen(source) + 1);
    if (!source_copy) return 0;
    strcpy(source_copy, source);
    
    // Разбиваем на строки
    char* lines[256];
    int line_count = 0;
    
    char* line = strtok(source_copy, "\n");
    while (line && line_count < 256) {
        lines[line_count++] = line;
        line = strtok(NULL, "\n");
    }
    
    // Парсим каждую строку
    for (int i = 0; i < line_count; i++) {
        char* trimmed = lines[i];
        
        // Убираем начальные пробелы и табы
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        
        // Пропускаем пустые строки и комментарии
        if (*trimmed == 0 || *trimmed == '#') continue;
        
        // ---- КОМАНДА PRINT ----
        if (strncmp(trimmed, "print", 5) == 0) {
            char* arg = trimmed + 5;
            
            // Пропускаем пробелы после print
            while (*arg == ' ' || *arg == '\t') arg++;
            
            // PRINT СТРОКИ (в кавычках)
            if (*arg == '"' || *arg == '\'') {
                char quote = *arg;
                char* str_start = arg + 1;
                char* str_end = strchr(str_start, quote);
                
                if (str_end) {
                    // Копируем строку в data секцию
                    uint32_t str_len = str_end - str_start;
                    uint32_t str_addr = data_pos - 1024; // Относительный адрес
                    
                    // Копируем строку
                    memcpy(output + data_pos, str_start, str_len);
                    output[data_pos + str_len] = 0; // Null terminator
                    data_pos += str_len + 1;
                    
                    // Генерируем PUSH <адрес строки>
                    output[pos++] = OP_PUSH;
                    memcpy(output + pos, &str_addr, 4);
                    pos += 4;
                    
                    // Генерируем SYSCALL PRINT_STR
                    output[pos++] = OP_SYSCALL;
                    output[pos++] = SYS_PRINT_STR;
                }
            }
            // PRINT ЧИСЛА (без кавычек)
            else if (*arg >= '0' && *arg <= '9' || *arg == '-') {
                int value = atoi(arg);
                
                // Генерируем PUSH <число>
                output[pos++] = OP_PUSH;
                memcpy(output + pos, &value, 4);
                pos += 4;
                
                // Генерируем SYSCALL PRINT_INT
                output[pos++] = OP_SYSCALL;
                output[pos++] = SYS_PRINT_INT;
            }
        }
        
        // ---- КОМАНДА PUSH ----
        else if (strncmp(trimmed, "push", 4) == 0) {
            char* arg = trimmed + 4;
            while (*arg == ' ' || *arg == '\t') arg++;
            
            if (*arg >= '0' && *arg <= '9' || *arg == '-') {
                int value = atoi(arg);
                
                output[pos++] = OP_PUSH;
                memcpy(output + pos, &value, 4);
                pos += 4;
            }
        }
        
        // ---- АРИФМЕТИЧЕСКИЕ ОПЕРАЦИИ ----
        else if (strcmp(trimmed, "add") == 0) {
            output[pos++] = OP_ADD;
        }
        else if (strcmp(trimmed, "sub") == 0) {
            output[pos++] = OP_SUB;
        }
        else if (strcmp(trimmed, "mul") == 0) {
            output[pos++] = OP_MUL;
        }
        else if (strcmp(trimmed, "div") == 0) {
            output[pos++] = OP_DIV;
        }
        
        // ---- КОМАНДА EXIT ----
        else if (strncmp(trimmed, "exit", 4) == 0) {
            char* arg = trimmed + 4;
            while (*arg == ' ' || *arg == '\t') arg++;
            
            int exit_code = 0;
            if (*arg >= '0' && *arg <= '9' || *arg == '-') {
                exit_code = atoi(arg);
            }
            
            // PUSH <код выхода>
            output[pos++] = OP_PUSH;
            memcpy(output + pos, &exit_code, 4);
            pos += 4;
            
            // SYSCALL EXIT
            output[pos++] = OP_SYSCALL;
            output[pos++] = SYS_EXIT;
        }
        
        // ---- КОМАНДА POP ----
        else if (strcmp(trimmed, "pop") == 0) {
            output[pos++] = OP_POP;
        }
        
        // ---- КОМАНДА NOP ----
        else if (strcmp(trimmed, "nop") == 0) {
            output[pos++] = OP_NOP;
        }
        
        // ---- КОМАНДА HALT ----
        else if (strcmp(trimmed, "halt") == 0) {
            output[pos++] = OP_HALT;
        }
    }
    
    // Добавляем HALT в конце программы (на всякий случай)
    if (pos > 0 && output[pos-1] != OP_HALT) {
        output[pos++] = OP_HALT;
    }
    
    // Заполняем заголовок
    GooHeader header = {
        .magic = {'G', 'O', 'O', 'S'},
        .version = 1,
        .type = 1,
        .code_size = pos - sizeof(GooHeader),
        .data_size = data_pos - 1024,
        .stack_size = 1024,
        .entry_point = 0
    };
    
    memcpy(output, &header, sizeof(header));
    
    free(source_copy);
    return pos;  // Общий размер скомпилированного файла
}
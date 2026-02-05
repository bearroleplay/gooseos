#include "goovm.h"
#include "terminal.h"
#include "libc.h"
#include <stddef.h>

// Создание VM
GooVM* goovm_create(void) {
    GooVM* vm = (GooVM*)malloc(sizeof(GooVM));
    if (!vm) return NULL;
    
    memset(vm, 0, sizeof(GooVM));
    
    // Выделяем память
    vm->stack = (int32_t*)malloc(GOO_STACK_SIZE * sizeof(int32_t));
    vm->memory = (int32_t*)malloc(GOO_MEMORY_SIZE * sizeof(int32_t));
    
    if (!vm->stack || !vm->memory) {
        free(vm->stack);
        free(vm->memory);
        free(vm);
        return NULL;
    }
    
    // Инициализация
    vm->sp = 0;
    vm->ip = 0;
    vm->bp = 0;
    vm->running = 1;
    vm->exit_code = 0;
    
    return vm;
}

// Уничтожение VM
void goovm_destroy(GooVM* vm) {
    if (vm) {
        free(vm->code);
        free(vm->stack);
        free(vm->memory);
        free(vm);
    }
}

// Загрузка программы
int goovm_load(GooVM* vm, const uint8_t* data, uint32_t size) {
    if (!vm || !data || size < 4) return 0;
    
    // Проверяем магическое число
    if (data[0] == 'G' && data[1] == 'O' && data[2] == 'O' && data[3] == 'S') {
        // Формат с заголовком
        GooHeader* header = (GooHeader*)data;
        
        if (size < sizeof(GooHeader) + header->code_size + header->data_size) {
            return 0;
        }
        
        // Выделяем память для кода
        vm->code = (uint8_t*)malloc(header->code_size + header->data_size);
        if (!vm->code) return 0;
        
        // Копируем код и данные
        memcpy(vm->code, data + sizeof(GooHeader), header->code_size);
        vm->code_size = header->code_size;
        
        // Устанавливаем указатель на данные
        vm->data = vm->code + header->code_size;
        vm->data_size = header->data_size;
        
        vm->ip = 0;
    } else {
        // Простой бинарник без заголовка
        vm->code = (uint8_t*)malloc(size);
        if (!vm->code) return 0;
        
        memcpy(vm->code, data, size);
        vm->code_size = size;
        vm->data = NULL;
        vm->data_size = 0;
        vm->ip = 0;
    }
    
    return 1;
}

// Системные вызовы
static int goovm_syscall(GooVM* vm, int num) {
    switch (num) {
        case SYS_EXIT:
            if (vm->sp > 0) {
                vm->exit_code = vm->stack[--vm->sp];
            } else {
                vm->exit_code = 0;
            }
            vm->running = 0;
            return vm->exit_code;
            
        case SYS_PRINT_INT: {
            if (vm->sp == 0) return -1;
            int32_t value = vm->stack[--vm->sp];
            char buffer[32];
            itoa(value, buffer, 10);
            terminal_print(buffer, VGA_COLOR_WHITE);
            return 0;
        }
            
        case SYS_PRINT_STR: {
            if (vm->sp == 0) return -1;
            int32_t addr = vm->stack[--vm->sp];
            
            // Ищем строку в данных
            if (vm->data && addr < vm->data_size) {
                char* str = (char*)(vm->data + addr);
                terminal_print(str, VGA_COLOR_WHITE);
                return 0;
            }
            // Или в коде (для простых компиляторов)
            else if (vm->code && addr < vm->code_size) {
                char* str = (char*)(vm->code + addr);
                terminal_print(str, VGA_COLOR_WHITE);
                return 0;
            }
            return -1;
        }
            
        case SYS_READ_KEY: {
            // TODO: Реализовать чтение клавиши
            vm->stack[vm->sp++] = 0;
            return 0;
        }
            
        case SYS_GET_TIME: {
            // TODO: Реализовать получение времени
            vm->stack[vm->sp++] = 0;
            return 0;
        }
            
        default:
            return -1;
    }
}

// Исполнение программы
int goovm_execute(GooVM* vm) {
    if (!vm || !vm->code || !vm->stack) return -1;
    
    vm->running = 1;
    vm->exit_code = 0;
    
    while (vm->running && vm->ip < vm->code_size) {
        uint8_t opcode = vm->code[vm->ip];
        vm->ip++;
        
        switch (opcode) {
            case OP_NOP:
                break;
                
            case OP_PUSH:
                if (vm->ip + 4 <= vm->code_size) {
                    int32_t value;
                    memcpy(&value, vm->code + vm->ip, 4);
                    vm->ip += 4;
                    
                    if (vm->sp < GOO_STACK_SIZE) {
                        vm->stack[vm->sp++] = value;
                    }
                }
                break;
                
            case OP_POP:
                if (vm->sp > 0) {
                    vm->sp--;
                }
                break;
                
            case OP_ADD:
                if (vm->sp >= 2) {
                    int32_t b = vm->stack[--vm->sp];
                    int32_t a = vm->stack[--vm->sp];
                    int32_t result = a + b;
                    vm->stack[vm->sp++] = result;
                }
                break;
                
            case OP_SUB:
                if (vm->sp >= 2) {
                    int32_t b = vm->stack[--vm->sp];
                    int32_t a = vm->stack[--vm->sp];
                    int32_t result = a - b;
                    vm->stack[vm->sp++] = result;
                }
                break;
                
            case OP_MUL:
                if (vm->sp >= 2) {
                    int32_t b = vm->stack[--vm->sp];
                    int32_t a = vm->stack[--vm->sp];
                    int32_t result = a * b;
                    vm->stack[vm->sp++] = result;
                }
                break;
                
            case OP_DIV:
                if (vm->sp >= 2) {
                    int32_t b = vm->stack[--vm->sp];
                    int32_t a = vm->stack[--vm->sp];
                    if (b != 0) {
                        int32_t result = a / b;
                        vm->stack[vm->sp++] = result;
                    }
                }
                break;
                
            case OP_SYSCALL:
                if (vm->ip < vm->code_size) {
                    uint8_t syscall_num = vm->code[vm->ip];
                    vm->ip++;
                    goovm_syscall(vm, syscall_num);
                }
                break;
                
            case OP_HALT:
                vm->running = 0;
                break;
                
            case OP_PRINT:
                // Legacy поддержка (для старых программ)
                if (vm->sp > 0) {
                    int32_t value = vm->stack[--vm->sp];
                    char buffer[32];
                    itoa(value, buffer, 10);
                    terminal_print(buffer, VGA_COLOR_WHITE);
                }
                break;
                
            default:
                // Неизвестная инструкция
                vm->running = 0;
                break;
        }
    }
    
    return vm->exit_code;
}

// Отладочный вывод состояния VM
void goovm_print_state(GooVM* vm) {
    if (!vm) return;
    
    char buffer[64];
    
    terminal_print("\n=== VM State ===\n", VGA_COLOR_YELLOW);
    
    itoa(vm->ip, buffer, 10);
    terminal_print("IP: ", VGA_COLOR_CYAN);
    terminal_print(buffer, VGA_COLOR_WHITE);
    
    terminal_print(" SP: ", VGA_COLOR_CYAN);
    itoa(vm->sp, buffer, 10);
    terminal_print(buffer, VGA_COLOR_WHITE);
    
    terminal_print(" Running: ", VGA_COLOR_CYAN);
    itoa(vm->running, buffer, 10);
    terminal_print(buffer, VGA_COLOR_WHITE);
    
    terminal_print("\nStack: ", VGA_COLOR_CYAN);
    for (int i = 0; i < vm->sp && i < 8; i++) {
        terminal_print("[", VGA_COLOR_LIGHT_GRAY);
        itoa(vm->stack[i], buffer, 10);
        terminal_print(buffer, VGA_COLOR_WHITE);
        terminal_print("]", VGA_COLOR_LIGHT_GRAY);
        if (i < vm->sp - 1) terminal_print(" ", VGA_COLOR_LIGHT_GRAY);
    }
    terminal_print("\n", VGA_COLOR_WHITE);
}
#include "goovm.h"
#include "terminal.h"
#include "keyboard.h"
#include "libc.h"
#include <stddef.h>

GooVM* goovm_create(void) {
    GooVM* vm = (GooVM*)malloc(sizeof(GooVM));
    if (!vm) return NULL;
    
    memset(vm, 0, sizeof(GooVM));
    vm->stack = (int32_t*)malloc(1024 * sizeof(int32_t));
    vm->memory = (int32_t*)malloc(4096 * sizeof(int32_t));
    
    if (!vm->stack || !vm->memory) {
        free(vm->stack);
        free(vm->memory);
        free(vm);
        return NULL;
    }
    
    vm->sp = 0;
    vm->ip = 0;
    vm->bp = 0;
    vm->running = 1;
    
    return vm;
}

void goovm_destroy(GooVM* vm) {
    if (vm) {
        free(vm->code);
        free(vm->data);
        free(vm->stack);
        free(vm->memory);
        free(vm);
    }
}

int goovm_load(GooVM* vm, const uint8_t* data, uint32_t size) {
    if (size < sizeof(GooHeader)) return 0;
    
    GooHeader* header = (GooHeader*)data;
    if (header->magic[0] != 'G' || header->magic[1] != 'O' || 
        header->magic[2] != 'O' || header->magic[3] != 'S') {
        return 0;
    }
    
    vm->code = (uint8_t*)malloc(header->code_size);
    vm->data = (uint8_t*)malloc(header->data_size);
    
    if (!vm->code || !vm->data) {
        free(vm->code);
        free(vm->data);
        return 0;
    }
    
    memcpy(vm->code, data + sizeof(GooHeader), header->code_size);
    memcpy(vm->data, data + sizeof(GooHeader) + header->code_size, header->data_size);
    
    vm->ip = header->entry_point;
    
    // Сохраняем размер кода в VM для отладки
    vm->exit_code = header->code_size; // Временно используем exit_code для хранения code_size
    
    return 1;
}

static int goovm_syscall(GooVM* vm, int num) {
    switch (num) {
        case SYS_EXIT:
            vm->exit_code = vm->stack[--vm->sp];
            vm->running = 0;
            return vm->exit_code;
            
        case SYS_PRINT_INT: {
            int value = vm->stack[--vm->sp];
            char buffer[32];
            itoa(value, buffer, 10);
            terminal_print(buffer, vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
            return 0;
        }
            
        case SYS_PRINT_STR: {
            // ВАЖНО: адрес строки ОТНОСИТЕЛЬНЫЙ к началу data секции!
            int addr = vm->stack[--vm->sp];
            
            // Строка находится в data секции ВМ
            if (vm->data && addr < 65536) {
                char* str = (char*)(vm->data + addr);
                terminal_print(str, vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
            } else {
                terminal_print("[invalid string]", vga_make_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
            }
            return 0;
        }
            
        case SYS_READ_KEY: {
            char key = keyboard_getch();
            vm->stack[vm->sp++] = (int)key;
            return 0;
        }
            
        case SYS_GET_TIME: {
            // Возвращаем тик
            vm->stack[vm->sp++] = 0; // Заглушка
            return 0;
        }
            
        default:
            terminal_print("[unknown syscall]", vga_make_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
            return -1;
    }
}

int goovm_execute(GooVM* vm) {
    // Сохраняем текущую позицию курсора терминала
    uint32_t saved_cursor_x = cursor_x;
    uint32_t saved_cursor_y = cursor_y;
    
    // Создаем буфер для отладки
    char debug[64];
    
    // Печатаем отладочную информацию в начале строки
    terminal_print("\n", VGA_COLOR_WHITE);  // Новая строка
    terminal_print("=== VM DEBUG START ===\n", VGA_COLOR_YELLOW);
    
    // Временное использование exit_code для хранения code_size
    itoa(vm->exit_code, debug, 10);
    terminal_print("Code size: ", VGA_COLOR_CYAN);
    terminal_print(debug, VGA_COLOR_WHITE);
    terminal_print("\n", VGA_COLOR_CYAN);
    
    // Сбрасываем exit_code обратно
    vm->exit_code = 0;
    
    while (vm->running && vm->ip < 65536) {
        uint8_t opcode = vm->code[vm->ip];
        
        // Отладка - в отдельной строке
        itoa(vm->ip, debug, 10);
        terminal_print("IP=", VGA_COLOR_LIGHT_GRAY);
        terminal_print(debug, VGA_COLOR_WHITE);
        terminal_print(" OP=0x", VGA_COLOR_LIGHT_GRAY);
        itoa(opcode, debug, 16);
        terminal_print(debug, VGA_COLOR_YELLOW);
        
        // Добавим текстовое имя опкода
        switch (opcode) {
            case OP_NOP: terminal_print(" (NOP)", VGA_COLOR_LIGHT_GRAY); break;
            case OP_PUSH: terminal_print(" (PUSH)", VGA_COLOR_LIGHT_GREEN); break;
            case OP_POP: terminal_print(" (POP)", VGA_COLOR_LIGHT_GREEN); break;
            case OP_ADD: terminal_print(" (ADD)", VGA_COLOR_LIGHT_GREEN); break;
            case OP_SUB: terminal_print(" (SUB)", VGA_COLOR_LIGHT_GREEN); break;
            case OP_MUL: terminal_print(" (MUL)", VGA_COLOR_LIGHT_GREEN); break;
            case OP_DIV: terminal_print(" (DIV)", VGA_COLOR_LIGHT_GREEN); break;
            case OP_SYSCALL: terminal_print(" (SYSCALL)", VGA_COLOR_LIGHT_BLUE); break;
            case OP_HALT: terminal_print(" (HALT)", VGA_COLOR_RED); break;
            default: terminal_print(" (UNKNOWN)", VGA_COLOR_RED); break;
        }
        terminal_print("\n", VGA_COLOR_LIGHT_GRAY);
        
        vm->ip++;  // Увеличиваем ПОСЛЕ взятия кода
        
        switch (opcode) {
            case OP_NOP:
                break;
                
            case OP_PUSH: {
                if (vm->ip + 4 > 65536) {
                    terminal_print("  ERROR: PUSH out of bounds!\n", VGA_COLOR_RED);
                    vm->running = 0;
                    break;
                }
                int32_t value = *(int32_t*)(vm->code + vm->ip);
                vm->ip += 4;
                vm->stack[vm->sp++] = value;
                
                // Отладка
                terminal_print("    value=", VGA_COLOR_LIGHT_GREEN);
                itoa(value, debug, 10);
                terminal_print(debug, VGA_COLOR_WHITE);
                terminal_print(" sp=", VGA_COLOR_LIGHT_GREEN);
                itoa(vm->sp, debug, 10);
                terminal_print(debug, VGA_COLOR_WHITE);
                terminal_print("\n", VGA_COLOR_LIGHT_GREEN);
                break;
            }
                
            case OP_POP:
                if (vm->sp > 0) {
                    vm->sp--;
                    terminal_print("    POP sp=", VGA_COLOR_LIGHT_GREEN);
                    itoa(vm->sp, debug, 10);
                    terminal_print(debug, VGA_COLOR_WHITE);
                    terminal_print("\n", VGA_COLOR_LIGHT_GREEN);
                } else {
                    terminal_print("    ERROR: stack underflow!\n", VGA_COLOR_RED);
                }
                break;
                
            case OP_ADD: {
                if (vm->sp < 2) {
                    terminal_print("    ERROR: not enough values for ADD\n", VGA_COLOR_RED);
                    vm->running = 0;
                    break;
                }
                int32_t b = vm->stack[--vm->sp];
                int32_t a = vm->stack[--vm->sp];
                int32_t result = a + b;
                vm->stack[vm->sp++] = result;
                terminal_print("    ADD ", VGA_COLOR_LIGHT_GREEN);
                itoa(a, debug, 10);
                terminal_print(debug, VGA_COLOR_WHITE);
                terminal_print(" + ", VGA_COLOR_LIGHT_GREEN);
                itoa(b, debug, 10);
                terminal_print(debug, VGA_COLOR_WHITE);
                terminal_print(" = ", VGA_COLOR_LIGHT_GREEN);
                itoa(result, debug, 10);
                terminal_print(debug, VGA_COLOR_WHITE);
                terminal_print("\n", VGA_COLOR_LIGHT_GREEN);
                break;
            }
                
            case OP_SUB: {
                if (vm->sp < 2) {
                    terminal_print("    ERROR: not enough values for SUB\n", VGA_COLOR_RED);
                    vm->running = 0;
                    break;
                }
                int32_t b = vm->stack[--vm->sp];
                int32_t a = vm->stack[--vm->sp];
                int32_t result = a - b;
                vm->stack[vm->sp++] = result;
                terminal_print("    SUB ", VGA_COLOR_LIGHT_GREEN);
                itoa(a, debug, 10);
                terminal_print(debug, VGA_COLOR_WHITE);
                terminal_print(" - ", VGA_COLOR_LIGHT_GREEN);
                itoa(b, debug, 10);
                terminal_print(debug, VGA_COLOR_WHITE);
                terminal_print(" = ", VGA_COLOR_LIGHT_GREEN);
                itoa(result, debug, 10);
                terminal_print(debug, VGA_COLOR_WHITE);
                terminal_print("\n", VGA_COLOR_LIGHT_GREEN);
                break;
            }
                
            case OP_MUL: {
                if (vm->sp < 2) {
                    terminal_print("    ERROR: not enough values for MUL\n", VGA_COLOR_RED);
                    vm->running = 0;
                    break;
                }
                int32_t b = vm->stack[--vm->sp];
                int32_t a = vm->stack[--vm->sp];
                int32_t result = a * b;
                vm->stack[vm->sp++] = result;
                terminal_print("    MUL ", VGA_COLOR_LIGHT_GREEN);
                itoa(a, debug, 10);
                terminal_print(debug, VGA_COLOR_WHITE);
                terminal_print(" * ", VGA_COLOR_LIGHT_GREEN);
                itoa(b, debug, 10);
                terminal_print(debug, VGA_COLOR_WHITE);
                terminal_print(" = ", VGA_COLOR_LIGHT_GREEN);
                itoa(result, debug, 10);
                terminal_print(debug, VGA_COLOR_WHITE);
                terminal_print("\n", VGA_COLOR_LIGHT_GREEN);
                break;
            }
                
            case OP_DIV: {
                if (vm->sp < 2) {
                    terminal_print("    ERROR: not enough values for DIV\n", VGA_COLOR_RED);
                    vm->running = 0;
                    break;
                }
                int32_t b = vm->stack[--vm->sp];
                int32_t a = vm->stack[--vm->sp];
                if (b != 0) {
                    int32_t result = a / b;
                    vm->stack[vm->sp++] = result;
                    terminal_print("    DIV ", VGA_COLOR_LIGHT_GREEN);
                    itoa(a, debug, 10);
                    terminal_print(debug, VGA_COLOR_WHITE);
                    terminal_print(" / ", VGA_COLOR_LIGHT_GREEN);
                    itoa(b, debug, 10);
                    terminal_print(debug, VGA_COLOR_WHITE);
                    terminal_print(" = ", VGA_COLOR_LIGHT_GREEN);
                    itoa(result, debug, 10);
                    terminal_print(debug, VGA_COLOR_WHITE);
                    terminal_print("\n", VGA_COLOR_LIGHT_GREEN);
                } else {
                    terminal_print("    ERROR: division by zero!\n", VGA_COLOR_RED);
                    vm->running = 0;
                }
                break;
            }
                
            case OP_SYSCALL: {
                if (vm->ip >= 65536) {
                    terminal_print("    ERROR: SYSCALL out of bounds!\n", VGA_COLOR_RED);
                    vm->running = 0;
                    break;
                }
                int syscall_num = vm->code[vm->ip++];
                terminal_print("    SYSCALL ", VGA_COLOR_LIGHT_BLUE);
                itoa(syscall_num, debug, 10);
                terminal_print(debug, VGA_COLOR_WHITE);
                
                // Покажем имя системного вызова
                switch (syscall_num) {
                    case SYS_EXIT: terminal_print(" (EXIT)", VGA_COLOR_LIGHT_BLUE); break;
                    case SYS_PRINT_INT: terminal_print(" (PRINT_INT)", VGA_COLOR_LIGHT_BLUE); break;
                    case SYS_PRINT_STR: terminal_print(" (PRINT_STR)", VGA_COLOR_LIGHT_BLUE); break;
                    case SYS_READ_KEY: terminal_print(" (READ_KEY)", VGA_COLOR_LIGHT_BLUE); break;
                    default: terminal_print(" (UNKNOWN)", VGA_COLOR_RED); break;
                }
                terminal_print("\n", VGA_COLOR_LIGHT_BLUE);
                
                goovm_syscall(vm, syscall_num);
                break;
            }
                
            case OP_HALT:
                terminal_print("    HALT\n", VGA_COLOR_RED);
                vm->running = 0;
                break;
                
            default:
                terminal_print("    ERROR: unknown opcode!\n", VGA_COLOR_RED);
                vm->running = 0;
                break;
        }
    }
    
    terminal_print("=== VM DEBUG END ===\n", VGA_COLOR_YELLOW);
    terminal_print("Exit code: ", VGA_COLOR_CYAN);
    itoa(vm->exit_code, debug, 10);
    terminal_print(debug, VGA_COLOR_WHITE);
    terminal_print("\n", VGA_COLOR_CYAN);
    
    // Восстанавливаем позицию курсора
    cursor_x = saved_cursor_x;
    cursor_y = saved_cursor_y;
    
    return vm->exit_code;
}
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
            return -1;
    }
}

int goovm_execute(GooVM* vm) {
    while (vm->running && vm->ip < 65536) {
        uint8_t opcode = vm->code[vm->ip++];
        
        switch (opcode) {
            case OP_NOP:
                break;
                
            case OP_PUSH: {
                int32_t value = *(int32_t*)(vm->code + vm->ip);
                vm->ip += 4;
                vm->stack[vm->sp++] = value;
                break;
            }
                
            case OP_POP:
                if (vm->sp > 0) vm->sp--;
                break;
                
            case OP_ADD: {
                int32_t b = vm->stack[--vm->sp];
                int32_t a = vm->stack[--vm->sp];
                vm->stack[vm->sp++] = a + b;
                break;
            }
                
            case OP_SUB: {
                int32_t b = vm->stack[--vm->sp];
                int32_t a = vm->stack[--vm->sp];
                vm->stack[vm->sp++] = a - b;
                break;
            }
                
            case OP_MUL: {
                int32_t b = vm->stack[--vm->sp];
                int32_t a = vm->stack[--vm->sp];
                vm->stack[vm->sp++] = a * b;
                break;
            }
                
            case OP_DIV: {
                int32_t b = vm->stack[--vm->sp];
                int32_t a = vm->stack[--vm->sp];
                if (b != 0) vm->stack[vm->sp++] = a / b;
                else vm->running = 0;
                break;
            }
                
            case OP_PRINT: {
                int32_t value = vm->stack[--vm->sp];
                vm->stack[vm->sp++] = value;
                vm->stack[vm->sp++] = SYS_PRINT_INT;
                goovm_syscall(vm, SYS_PRINT_INT);
                break;
            }
                
            case OP_SYSCALL: {
                int syscall_num = vm->code[vm->ip++];
                goovm_syscall(vm, syscall_num);
                break;
            }
                
            case OP_HALT:
                vm->running = 0;
                break;
                
            default:
                vm->running = 0;
                break;
        }
    }
    
    return vm->exit_code;
}
#include "goovm.h"
#include "terminal.h"
#include "libc.h"

// Обработчик вывода по умолчанию
static void default_output_handler(const char* str, uint8_t color) {
    terminal_print(str, color);
}

static vm_output_handler_t output_handler = default_output_handler;

// Установка обработчика вывода
void goovm_set_output_handler(vm_output_handler_t handler) {
    if (handler) {
        output_handler = handler;
    } else {
        output_handler = default_output_handler;
    }
}

// Создание VM
GooVM* goovm_create(void) {
    GooVM* vm = (GooVM*)malloc(sizeof(GooVM));
    if (!vm) return NULL;
    
    memset(vm, 0, sizeof(GooVM));
    vm->running = 0;
    vm->sp = 0;
    vm->call_sp = 0;
    vm->ip = 0;
    vm->flags = 0;
    vm->code = NULL;
    vm->code_size = 0;
    
    return vm;
}

// Уничтожение VM
void goovm_destroy(GooVM* vm) {
    if (vm) {
        if (vm->code) {
            free(vm->code);
        }
        free(vm);
    }
}

// Загрузка программы
int goovm_load(GooVM* vm, const uint8_t* data, uint32_t size) {
    if (!vm || !data || size == 0 || size > 65536) return 0;
    
    // Освобождаем старый код если есть
    if (vm->code) {
        free(vm->code);
    }
    
    vm->code = (uint8_t*)malloc(size);
    if (!vm->code) return 0;
    
    memcpy(vm->code, data, size);
    vm->code_size = size;
    vm->ip = 0;
    vm->sp = 0;
    vm->call_sp = 0;
    vm->running = 1;
    vm->flags = 0;
    
    return 1;
}

// Операции со стеком
static void vm_push(GooVM* vm, int32_t value) {
    if (vm->sp < VM_STACK_SIZE) {
        vm->stack[vm->sp++] = value;
    }
}

static int32_t vm_pop(GooVM* vm) {
    if (vm->sp > 0) {
        return vm->stack[--vm->sp];
    }
    return 0;
}

// Исполнение одной инструкции
static int vm_execute_insn(GooVM* vm) {
    if (!vm->running || vm->ip >= vm->code_size) {
        vm->running = 0;
        return 0;
    }
    
    uint8_t op = vm->code[vm->ip++];
    
    switch (op) {
        case VM_OP_NOP:
            break;
            
        case VM_OP_PUSH: {
            if (vm->ip + 4 > vm->code_size) {
                vm->running = 0;
                return 0;
            }
            int32_t value = *(int32_t*)&vm->code[vm->ip];
            vm->ip += 4;
            vm_push(vm, value);
            break;
        }
        
        case VM_OP_POP: {
            vm_pop(vm);
            break;
        }
        
        case VM_OP_ADD: {
            int32_t b = vm_pop(vm);
            int32_t a = vm_pop(vm);
            vm_push(vm, a + b);
            break;
        }
        
        case VM_OP_SUB: {
            int32_t b = vm_pop(vm);
            int32_t a = vm_pop(vm);
            vm_push(vm, a - b);
            break;
        }
        
        case VM_OP_MUL: {
            int32_t b = vm_pop(vm);
            int32_t a = vm_pop(vm);
            vm_push(vm, a * b);
            break;
        }
        
        case VM_OP_DIV: {
            int32_t b = vm_pop(vm);
            int32_t a = vm_pop(vm);
            if (b != 0) {
                vm_push(vm, a / b);
            } else {
                vm_push(vm, 0);
            }
            break;
        }
        
        case VM_OP_PRINT: {
            // Строка хранится в коде
            char* str = (char*)&vm->code[vm->ip];
            int len = strlen(str);
            output_handler(str, VGA_COLOR_WHITE);
            vm->ip += len + 1;  // +1 для null-терминатора
            break;
        }
        
        case VM_OP_PRINT_NUM: {
            int32_t value = vm_pop(vm);
            char buf[16];
            itoa(value, buf, 10);
            output_handler(buf, VGA_COLOR_LIGHT_GREEN);
            break;
        }
        
        case VM_OP_HALT:
            vm->running = 0;
            break;
            
        default:
            output_handler("Unknown instruction: ", VGA_COLOR_RED);
            char buf[8];
            itoa(op, buf, 16);
            output_handler(buf, VGA_COLOR_WHITE);
            output_handler("\n", VGA_COLOR_WHITE);
            vm->running = 0;
            break;
    }
    
    return 1;
}

// Запуск программы
int goovm_execute(GooVM* vm) {
    if (!vm || !vm->code || !vm->running) return -1;
    
    output_handler("--- GooseVM Execution ---\n", VGA_COLOR_CYAN);
    
    int max_insn = 10000;  // Защита от бесконечных циклов
    while (vm->running && max_insn-- > 0) {
        if (!vm_execute_insn(vm)) {
            break;
        }
    }
    
    if (max_insn <= 0) {
        output_handler("Error: Infinite loop detected!\n", VGA_COLOR_RED);
        vm->running = 0;
    }
    
    output_handler("\n--- VM Halted ---\n", VGA_COLOR_CYAN);
    
    return 0;
}

void goovm_print_state(GooVM* vm) {
    if (!vm) return;
    
    output_handler("VM State:\n", VGA_COLOR_YELLOW);
    output_handler("  IP: ", VGA_COLOR_LIGHT_GRAY);
    char buf[16];
    itoa(vm->ip, buf, 10);
    output_handler(buf, VGA_COLOR_WHITE);
    output_handler("\n  SP: ", VGA_COLOR_LIGHT_GRAY);
    itoa(vm->sp, buf, 10);
    output_handler(buf, VGA_COLOR_WHITE);
    output_handler("\n  Code size: ", VGA_COLOR_LIGHT_GRAY);
    itoa(vm->code_size, buf, 10);
    output_handler(buf, VGA_COLOR_WHITE);
    output_handler("\n", VGA_COLOR_WHITE);

}

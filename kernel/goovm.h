#ifndef GOOVM_H
#define GOOVM_H

#include <stdint.h>

// Коды операций GooseVM
#define VM_OP_NOP       0x00
#define VM_OP_PUSH      0x01
#define VM_OP_POP       0x02
#define VM_OP_ADD       0x03
#define VM_OP_SUB       0x04
#define VM_OP_MUL       0x05
#define VM_OP_DIV       0x06
#define VM_OP_PRINT     0x07
#define VM_OP_PRINT_NUM 0x08
#define VM_OP_JMP       0x09
#define VM_OP_JZ        0x0A
#define VM_OP_JNZ       0x0B
#define VM_OP_CMP       0x0C
#define VM_OP_CALL      0x0D
#define VM_OP_RET       0x0E
#define VM_OP_HALT      0xFF

// Размеры стека
#define VM_STACK_SIZE   256
#define VM_CALL_STACK   32

// ПОЛНАЯ структура виртуальной машины
typedef struct GooVM {
    uint8_t* code;          // Указатель на байт-код
    uint32_t code_size;     // Размер кода
    uint32_t ip;            // Instruction Pointer
    
    int32_t stack[VM_STACK_SIZE];  // Стек данных
    uint32_t sp;            // Stack Pointer
    
    uint32_t call_stack[VM_CALL_STACK];  // Стек вызовов
    uint32_t call_sp;       // Call Stack Pointer
    
    int running;            // Флаг выполнения
    int flags;              // Флаги состояния (ZF, SF и т.д.)
} GooVM;

// Создание и уничтожение
GooVM* goovm_create(void);
void goovm_destroy(GooVM* vm);

// Загрузка и выполнение
int goovm_load(GooVM* vm, const uint8_t* data, uint32_t size);
int goovm_execute(GooVM* vm);
void goovm_print_state(GooVM* vm);

// Управление выводом
typedef void (*vm_output_handler_t)(const char* str, uint8_t color);
void goovm_set_output_handler(vm_output_handler_t handler);

#endif
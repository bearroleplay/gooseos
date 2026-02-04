#ifndef GOOVM_H
#define GOOVM_H

#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    char magic[4];      // "GOOS"
    uint8_t version;
    uint8_t type;
    uint16_t code_size;
    uint16_t data_size;
    uint16_t stack_size;
    uint32_t entry_point;
} GooHeader;
#pragma pack(pop)

typedef struct {
    uint8_t* code;
    uint8_t* data;
    int32_t* stack;
    int32_t* memory;
    
    uint32_t ip;
    uint32_t sp;
    uint32_t bp;
    
    int running;
    int exit_code;
} GooVM;

// Системные вызовы
enum {
    SYS_EXIT = 0,
    SYS_PRINT_INT = 1,
    SYS_PRINT_STR = 2,
    SYS_READ_KEY = 3,
    SYS_GET_TIME = 4,
    SYS_DRAW_CHAR = 5,
};

// Инструкции
enum {
    OP_NOP = 0x00,
    OP_PUSH = 0x01,
    OP_POP = 0x02,
    OP_ADD = 0x03,
    OP_SUB = 0x04,
    OP_MUL = 0x05,
    OP_DIV = 0x06,
    OP_PRINT = 0x07,
    OP_CALL = 0x08,
    OP_RET = 0x09,
    OP_JMP = 0x0A,
    OP_JZ = 0x0B,
    OP_LOAD = 0x0C,
    OP_STORE = 0x0D,
    OP_SYSCALL = 0x0E,
    OP_HALT = 0xFF,
};

GooVM* goovm_create(void);
void goovm_destroy(GooVM* vm);
int goovm_load(GooVM* vm, const uint8_t* data, uint32_t size);
int goovm_execute(GooVM* vm);
void goovm_print_state(GooVM* vm);

#endif
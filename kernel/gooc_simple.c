// gooc_simple.c - ЗАГЛУШКА
#include "gooc_simple.h"
#include "terminal.h"
#include "libc.h"
#include <string.h>

int gooc_compile(const char* source, uint8_t* output, uint32_t max_size) {
    // Минимальная программа, которая просто выходит
    uint8_t program[] = {
        0x01, 0x00, 0x00, 0x00, 0x00,  // OP_PUSH 0
        0x0E, 0x00,                     // OP_SYSCALL 0 (exit)
        0xFF                            // OP_HALT
    };
    
    if (max_size < sizeof(program)) {
        return 0;
    }
    
    memcpy(output, program, sizeof(program));
    return sizeof(program);
}
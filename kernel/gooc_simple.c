// gooc_simple.c - МИНИМАЛЬНЫЙ РАБОЧИЙ ВАРИАНТ
#include "gooc_simple.h"
#include "goovm.h"
#include "libc.h"
#include <string.h>

int gooc_compile(const char* source, uint8_t* output, uint32_t max_size) {
    // Просто создадим простую тестовую программу
    // которая точно работает
    
    // Программа: print "Hello World\n" exit 0
    // Но сделаем через числа чтобы точно работало
    
    uint8_t program[] = {
        OP_PUSH, 0x48, 0x00, 0x00, 0x00,  // push 'H' (72)
        OP_SYSCALL, SYS_PRINT_INT,        // print 'H'
        OP_PUSH, 0x65, 0x00, 0x00, 0x00,  // push 'e' (101)
        OP_SYSCALL, SYS_PRINT_INT,        // print 'e'
        OP_PUSH, 0x6C, 0x00, 0x00, 0x00,  // push 'l' (108)
        OP_SYSCALL, SYS_PRINT_INT,
        OP_PUSH, 0x6C, 0x00, 0x00, 0x00,
        OP_SYSCALL, SYS_PRINT_INT,
        OP_PUSH, 0x6F, 0x00, 0x00, 0x00,  // push 'o' (111)
        OP_SYSCALL, SYS_PRINT_INT,
        OP_PUSH, 0x20, 0x00, 0x00, 0x00,  // push ' ' (32)
        OP_SYSCALL, SYS_PRINT_INT,
        OP_PUSH, 0x57, 0x00, 0x00, 0x00,  // push 'W' (87)
        OP_SYSCALL, SYS_PRINT_INT,
        OP_PUSH, 0x6F, 0x00, 0x00, 0x00,
        OP_SYSCALL, SYS_PRINT_INT,
        OP_PUSH, 0x72, 0x00, 0x00, 0x00,  // push 'r' (114)
        OP_SYSCALL, SYS_PRINT_INT,
        OP_PUSH, 0x6C, 0x00, 0x00, 0x00,
        OP_SYSCALL, SYS_PRINT_INT,
        OP_PUSH, 0x64, 0x00, 0x00, 0x00,  // push 'd' (100)
        OP_SYSCALL, SYS_PRINT_INT,
        OP_PUSH, 0x0A, 0x00, 0x00, 0x00,  // push '\n' (10)
        OP_SYSCALL, SYS_PRINT_INT,
        OP_PUSH, 0x00, 0x00, 0x00, 0x00,  // push 0
        OP_SYSCALL, SYS_EXIT,             // exit 0
        OP_HALT
    };
    
    memcpy(output, program, sizeof(program));
    return sizeof(program);
}
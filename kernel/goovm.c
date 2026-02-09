// goovm.c - ЗАГЛУШКА
#include "goovm.h"
#include "terminal.h"
#include "libc.h"
#include <string.h>

GooVM* goovm_create(void) {
    return (GooVM*)malloc(sizeof(GooVM));
}

void goovm_destroy(GooVM* vm) {
    if (vm) free(vm);
}

int goovm_load(GooVM* vm, const uint8_t* data, uint32_t size) {
    if (!vm || !data || size == 0) return 0;
    
    // Просто отмечаем что загружено
    vm->code = (uint8_t*)data;
    vm->code_size = size;
    vm->ip = 0;
    vm->sp = 0;
    vm->running = 1;
    
    return 1;
}

int goovm_execute(GooVM* vm) {
    if (!vm || !vm->code) return -1;
    
    terminal_print("\n[VM] Executing GooseScript program...\n", VGA_COLOR_CYAN);
    
    // Просто печатаем что выполняется
    terminal_print("[VM] Program finished (stub)\n", VGA_COLOR_GREEN);
    
    return 0;
}

void goovm_print_state(GooVM* vm) {
    terminal_print("[VM] State: STUB\n", VGA_COLOR_YELLOW);
}

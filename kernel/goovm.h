#ifndef GOOVM_H
#define GOOVM_H

#include <stdint.h>

// Минимальный набор
typedef struct {
    uint8_t* code;
    uint32_t code_size;
    uint32_t ip;
    uint32_t sp;
    int running;
} GooVM;

GooVM* goovm_create(void);
void goovm_destroy(GooVM* vm);
int goovm_load(GooVM* vm, const uint8_t* data, uint32_t size);
int goovm_execute(GooVM* vm);
void goovm_print_state(GooVM* vm);

#endif
#ifndef GOOC_SIMPLE_H
#define GOOC_SIMPLE_H

#include <stdint.h>

int gooc_compile(const char* source, uint8_t* output, uint32_t max_size);

#endif
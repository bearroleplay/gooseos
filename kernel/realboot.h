#ifndef REALBOOT_H
#define REALBOOT_H

#include "panic.h"

// Функции реальной загрузки
void real_boot_start(void);
void real_boot_update(int percent, const char* message);
void real_boot_error(PanicMode mode, const char* message, uint32_t code);
void real_boot_complete(void);

// Проверки
int real_check_multiboot(uint32_t magic);
int real_check_memory(void* mb_info);

#endif
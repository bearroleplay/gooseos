#include "libc.h"

static char heap[65536];
static size_t heap_ptr = 0;

void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    while (num--) *p++ = (unsigned char)value;
    return ptr;
}

void* memcpy(void* dest, const void* src, size_t num) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (num--) *d++ = *s++;
    return dest;
}

int memcmp(const void* ptr1, const void* ptr2, size_t num) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    while (num--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++; p2++;
    }
    return 0;
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* strncpy(char* dest, const char* src, size_t num) {
    char* d = dest;
    while (num-- && (*d++ = *src++));
    if (num) *d = 0;
    return dest;
}

int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++; str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

int strncmp(const char* str1, const char* str2, size_t num) {
    while (num-- && *str1 && (*str1 == *str2)) {
        str1++; str2++;
    }
    if (num == (size_t)-1) return 0;
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

char* itoa(int value, char* buffer, int base) {
    if (base < 2 || base > 36) {
        *buffer = 0;
        return buffer;
    }
    
    char* ptr = buffer;
    int is_negative = 0;
    
    if (value < 0 && base == 10) {
        is_negative = 1;
        value = -value;
    }
    
    do {
        int digit = value % base;
        *ptr++ = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[digit];
        value /= base;
    } while (value);
    
    if (is_negative) *ptr++ = '-';
    *ptr = 0;
    
    // Реверс строки
    char* start = buffer;
    char* end = ptr - 1;
    while (start < end) {
        char tmp = *start;
        *start = *end;
        *end = tmp;
        start++; end--;
    }
    
    return buffer;
}

void* malloc(size_t size) {
    if (heap_ptr + size > sizeof(heap)) return NULL;
    void* ptr = &heap[heap_ptr];
    heap_ptr += size;
    return ptr;
}

void free(void* ptr) {
    // Простая реализация - не освобождаем память
    (void)ptr;
}

char* strcat(char* dest, const char* src) {
    char* d = dest;
    while (*d) d++;
    while (*src) *d++ = *src++;
    *d = 0;
    return dest;
}
// В libc.c добавь в конец:

char* strchr(const char* str, int ch) {
    while (*str) {
        if (*str == ch) return (char*)str;
        str++;
    }
    return NULL;
}

char* strrchr(const char* str, int ch) {
    const char* last = NULL;
    while (*str) {
        if (*str == ch) last = str;
        str++;
    }
    return (char*)last;
}

int atoi(const char* str) {
    int result = 0;
    int sign = 1;
    
    // Пропускаем пробелы
    while (*str == ' ' || *str == '\t') str++;
    
    // Знак
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    // Цифры
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}
// В libc.c добавь:
char* strtok(char* str, const char* delimiters) {
    static char* next_token = NULL;
    
    if (str != NULL) {
        next_token = str;
    }
    
    if (next_token == NULL || *next_token == 0) {
        return NULL;
    }
    
    // Пропускаем разделители в начале
    while (*next_token && strchr(delimiters, *next_token)) {
        next_token++;
    }
    
    if (*next_token == 0) {
        return NULL;
    }
    
    char* token_start = next_token;
    
    // Ищем следующий разделитель
    while (*next_token && !strchr(delimiters, *next_token)) {
        next_token++;
    }
    
    if (*next_token) {
        *next_token = 0;
        next_token++;
    }
    
    return token_start;
}
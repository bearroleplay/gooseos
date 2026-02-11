#ifndef LIBC_H
#define LIBC_H

#include <stddef.h>
#include <stdint.h>

void* memset(void* ptr, int value, size_t num);
char* strstr(const char* haystack, const char* needle);  // Добавить в libc.h
void* memcpy(void* dest, const void* src, size_t num);
int memcmp(const void* ptr1, const void* ptr2, size_t num);
size_t strlen(const char* str);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t num);
int strcmp(const char* str1, const char* str2);
int strncmp(const char* str1, const char* str2, size_t num);
char* itoa(int value, char* buffer, int base);
void* malloc(size_t size);
void free(void* ptr);
char* strcat(char* dest, const char* src);
char* strchr(const char* str, int ch);      
char* strrchr(const char* str, int ch);    
int atoi(const char* str);                   
char* strtok(char* str, const char* delimiters);
uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t val);
uint16_t inw(uint16_t port);
void outw(uint16_t port, uint16_t val);
int sprintf(char* str, const char* format, ...);

#endif
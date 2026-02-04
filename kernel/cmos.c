// cmos.c
#include "cmos.h"
#include "libc.h"

static inline uint8_t cmos_read(uint8_t reg) {
    __asm__ volatile("outb %0, $0x70" : : "a"(reg));
    __asm__ volatile("inb $0x71, %0" : "=a"(reg));
    return reg;
}

uint8_t bcd_to_bin(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

Time get_time(void) {
    Time time;
    
    // Читаем время из CMOS
    time.second = cmos_read(0x00);
    time.minute = cmos_read(0x02);
    time.hour = cmos_read(0x04);
    time.day = cmos_read(0x07);
    time.month = cmos_read(0x08);
    time.year = cmos_read(0x09);
    
    // Конвертируем из BCD в бинарный
    time.second = bcd_to_bin(time.second);
    time.minute = bcd_to_bin(time.minute);
    time.hour = bcd_to_bin(time.hour);
    time.day = bcd_to_bin(time.day);
    time.month = bcd_to_bin(time.month);
    time.year = bcd_to_bin(time.year);
    
    // 24-часовой формат
    if (time.hour & 0x80) { // Проверяем bit 7 для 12/24 часа
        time.hour = ((time.hour & 0x7F) + 12) % 24;
    }
    
    return time;
}

void get_time_string(char* buffer) {
    Time time = get_time();
    
    // Формат: "HH:MM:SS"
    buffer[0] = '0' + (time.hour / 10);
    buffer[1] = '0' + (time.hour % 10);
    buffer[2] = ':';
    buffer[3] = '0' + (time.minute / 10);
    buffer[4] = '0' + (time.minute % 10);
    buffer[5] = ':';
    buffer[6] = '0' + (time.second / 10);
    buffer[7] = '0' + (time.second % 10);
    buffer[8] = 0;
}

void get_date_string(char* buffer) {
    Time time = get_time();
    
    // Формат: "DD/MM/YY"
    buffer[0] = '0' + (time.day / 10);
    buffer[1] = '0' + (time.day % 10);
    buffer[2] = '/';
    buffer[3] = '0' + (time.month / 10);
    buffer[4] = '0' + (time.month % 10);
    buffer[5] = '/';
    buffer[6] = '2';
    buffer[7] = '0';
    buffer[8] = '0' + (time.year / 10);
    buffer[9] = '0' + (time.year % 10);
    buffer[10] = 0;
}
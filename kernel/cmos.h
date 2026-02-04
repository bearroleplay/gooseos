#ifndef CMOS_H
#define CMOS_H

#include <stdint.h>

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} Time;

Time get_time(void);
void get_time_string(char* buffer);
void get_date_string(char* buffer);

#endif
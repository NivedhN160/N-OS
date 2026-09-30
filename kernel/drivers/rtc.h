#ifndef RTC_H
#define RTC_H

#include <stdint.h>

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint32_t year;
} time_t;

// Read the current hardware time from CMOS
void rtc_read_time(time_t* t);

// Unix timestamp (seconds since Jan 1 1970)
uint32_t time();

#endif

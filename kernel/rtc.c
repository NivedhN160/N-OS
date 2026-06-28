#include "rtc.h"

static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline unsigned char inb(unsigned short port) {
    unsigned char val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static int get_update_in_progress_flag() {
    outb(0x70, 0x0A);
    return (inb(0x71) & 0x80);
}

static unsigned char get_rtc_register(int reg) {
    outb(0x70, reg);
    return inb(0x71);
}

void rtc_init() {}

void rtc_get_time(rtc_time_t *time) {
    while (get_update_in_progress_flag());
    unsigned char sec = get_rtc_register(0x00);
    unsigned char min = get_rtc_register(0x02);
    unsigned char hour = get_rtc_register(0x04);
    unsigned char day = get_rtc_register(0x07);
    unsigned char month = get_rtc_register(0x08);
    unsigned char year = get_rtc_register(0x09);
    
    unsigned char reg_b = get_rtc_register(0x0B);
    
    // Convert BCD to binary
    if (!(reg_b & 0x04)) {
        sec = (sec & 0x0F) + ((sec / 16) * 10);
        min = (min & 0x0F) + ((min / 16) * 10);
        hour = ( (hour & 0x0F) + (((hour & 0x70) / 16) * 10) ) | (hour & 0x80);
        day = (day & 0x0F) + ((day / 16) * 10);
        month = (month & 0x0F) + ((month / 16) * 10);
        year = (year & 0x0F) + ((year / 16) * 10);
    }
    
    // Convert 12 hour to 24 hour
    if (!(reg_b & 0x02) && (hour & 0x80)) {
        hour = ((hour & 0x7F) + 12) % 24;
    }
    
    time->second = sec;
    time->minute = min;
    time->hour = hour;
    time->day = day;
    time->month = month;
    time->year = 2000 + year;
}

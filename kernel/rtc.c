#include "rtc.h"

// Basic IO port access
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__ ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__ ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

// Helper to convert BCD (Binary Coded Decimal) to standard integers
static uint8_t bcd_to_bin(uint8_t bcd) {
    return (bcd & 0x0F) + ((bcd / 16) * 10);
}

static uint8_t get_rtc_register(int reg) {
    outb(CMOS_ADDR, reg);
    return inb(CMOS_DATA);
}

void rtc_read_time(time_t* t) {
    // Note: This is a simplified read that doesn't check the "update in progress" flag.
    // Good enough for a basic OS implementation.
    t->second = bcd_to_bin(get_rtc_register(0x00));
    t->minute = bcd_to_bin(get_rtc_register(0x02));
    t->hour   = bcd_to_bin(get_rtc_register(0x04));
    t->day    = bcd_to_bin(get_rtc_register(0x07));
    t->month  = bcd_to_bin(get_rtc_register(0x08));
    t->year   = bcd_to_bin(get_rtc_register(0x09)) + 2000; // CMOS only stores last 2 digits
}

// Very rough approximation of unix timestamp (doesn't perfectly handle leap years)
uint32_t time() {
    time_t t;
    rtc_read_time(&t);
    
    uint32_t days = (t.year - 1970) * 365 + (t.month * 30) + t.day;
    uint32_t seconds = (days * 86400) + (t.hour * 3600) + (t.minute * 60) + t.second;
    
    return seconds;
}

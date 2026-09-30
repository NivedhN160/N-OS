#include "pit.h"
#include "process.h"

volatile uint32_t tick = 0;

static inline void outb_pit(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static void timer_callback(registers_t *regs) {
    (void)regs;
    tick++;
    scheduler_tick(); // Preemptively run background tasks on timer tick!
}

void pit_init(uint32_t frequency) {
    register_interrupt_handler(32, timer_callback);
    uint32_t divisor = 1193180 / frequency;
    outb_pit(0x43, 0x36);
    outb_pit(0x40, (uint8_t)(divisor & 0xFF));
    outb_pit(0x40, (uint8_t)((divisor>>8) & 0xFF));
}

uint32_t get_tick_count() {
    return tick;
}


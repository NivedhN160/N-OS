#include "isr.h"
#include "idt.h"

extern void term_print(const char *s, unsigned int col);

isr_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

void isr_handler(registers_t *regs) {
    if (interrupt_handlers[regs->int_no] != 0) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    } else {
        term_print("Unhandled interrupt\n", 0xFF0000);
        while(1); // Halt on unhandled exception for now
    }
}

void irq_handler(registers_t *regs) {
    // Send EOI to PICs
    if (regs->int_no >= 40) {
        // Send reset signal to slave.
        __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0xA0));
    }
    // Send reset signal to master.
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0x20));

    if (interrupt_handlers[regs->int_no] != 0) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    }
}

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1
#define ICW1_INIT    0x10
#define ICW1_ICW4    0x01
#define ICW4_8086    0x01

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

void pic_remap() {
    uint8_t a1, a2;
    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    
    outb(PIC1_DATA, 0x20); // Master PIC vector offset (32)
    outb(PIC2_DATA, 0x28); // Slave PIC vector offset (40)
    
    outb(PIC1_DATA, 4); // Tell Master there is a slave PIC at IRQ2
    outb(PIC2_DATA, 2); // Tell Slave its cascade identity
    
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
    
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

#define SET_ISR(n) extern void isr##n(); idt_set_gate(n, (uint32_t)isr##n, 0x08, 0x8E);
#define SET_IRQ(n, idx) extern void irq##n(); idt_set_gate(idx, (uint32_t)irq##n, 0x08, 0x8E);

void isr_install() {
    SET_ISR(0); SET_ISR(1); SET_ISR(2); SET_ISR(3);
    SET_ISR(4); SET_ISR(5); SET_ISR(6); SET_ISR(7);
    SET_ISR(8); SET_ISR(9); SET_ISR(10); SET_ISR(11);
    SET_ISR(12); SET_ISR(13); SET_ISR(14); SET_ISR(15);
    SET_ISR(16); SET_ISR(17); SET_ISR(18); SET_ISR(19);
    SET_ISR(20); SET_ISR(21); SET_ISR(22); SET_ISR(23);
    SET_ISR(24); SET_ISR(25); SET_ISR(26); SET_ISR(27);
    SET_ISR(28); SET_ISR(29); SET_ISR(30); SET_ISR(31);

    SET_IRQ(0, 32); SET_IRQ(1, 33); SET_IRQ(2, 34); SET_IRQ(3, 35);
    SET_IRQ(4, 36); SET_IRQ(5, 37); SET_IRQ(6, 38); SET_IRQ(7, 39);
    SET_IRQ(8, 40); SET_IRQ(9, 41); SET_IRQ(10, 42); SET_IRQ(11, 43);
    SET_IRQ(12, 44); SET_IRQ(13, 45); SET_IRQ(14, 46); SET_IRQ(15, 47);
}

#include "idt.h"

extern void idt_flush(uint32_t);
extern void isr_install();
extern void pic_remap();

struct idt_entry_struct idt_entries[256];
struct idt_ptr_struct   idt_ptr;

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags   = flags /* | 0x60 */ ; // For user mode we might OR 0x60
}

void idt_init() {
    idt_ptr.limit = sizeof(struct idt_entry_struct) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    // memset idt_entries to 0
    uint8_t *p = (uint8_t *)&idt_entries;
    for (uint32_t i = 0; i < sizeof(struct idt_entry_struct) * 256; i++) {
        p[i] = 0;
    }

    pic_remap();
    isr_install();

    idt_flush((uint32_t)&idt_ptr);
}

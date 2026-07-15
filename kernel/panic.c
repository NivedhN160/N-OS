#include "isr.h"
extern void term_print(const char*, unsigned int);
extern void draw_rect(int,int,int,int,int);
extern void draw_string_scaled(int,int,const char*,int,int);
extern void gfx_swap();

void itoa_hex(uint32_t val, char* buf) {
    const char* hex = "0123456789ABCDEF";
    buf[0] = '0'; buf[1] = 'x';
    for(int i=0; i<8; i++) {
        buf[9-i] = hex[val & 0xF];
        val >>= 4;
    }
    buf[10] = 0;
}

void panic(const char *msg, registers_t *regs) {
    __asm__ volatile("cli");
    draw_rect(0, 0, 1024, 768, 0x0000AA); // BSOD Blue
    draw_string_scaled(50, 50, ":(", 0xFFFFFF, 8);
    draw_string_scaled(50, 150, "A fatal exception has occurred in N-OS.", 0xFFFFFF, 2);
    draw_string_scaled(50, 200, msg, 0xFFFFFF, 2);
    
    if (regs) {
        char buf[16];
        draw_string_scaled(50, 300, "Register Dump:", 0xFFFFFF, 2);
        itoa_hex(regs->eax, buf); draw_string_scaled(50, 340, "EAX:", 0xFFFFFF, 2); draw_string_scaled(150, 340, buf, 0xFFFFFF, 2);
        itoa_hex(regs->eip, buf); draw_string_scaled(50, 370, "EIP:", 0xFFFFFF, 2); draw_string_scaled(150, 370, buf, 0xFFFFFF, 2);
        itoa_hex(regs->int_no, buf); draw_string_scaled(50, 400, "INT:", 0xFFFFFF, 2); draw_string_scaled(150, 400, buf, 0xFFFFFF, 2);
    }
    
    gfx_swap();
    while(1) { __asm__ volatile("hlt"); }
}

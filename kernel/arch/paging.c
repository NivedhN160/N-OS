#include "paging.h"

extern void term_print(const char*, unsigned int);
extern void panic(const char*, registers_t*);

uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

void paging_init() {
    for(int i=0; i<1024; i++) {
        page_directory[i] = 0x00000002;
    }
    
    for(unsigned int i=0; i<1024; i++) {
        first_page_table[i] = (i * 0x1000) | 3;
    }
    
    page_directory[0] = ((uint32_t)first_page_table) | 3;
    
    register_interrupt_handler(14, page_fault);

    __asm__ volatile("mov %0, %%cr3":: "r"(page_directory));
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0":: "r"(cr0));
    
    term_print("Paging enabled. Virtual Memory active.\n", 0x00FF00);
}

void page_fault(registers_t *regs) {
    uint32_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));
    panic("PAGE FAULT", regs);
}

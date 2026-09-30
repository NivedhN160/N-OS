#include "paging.h"

extern void term_print(const char*, unsigned int);
extern void panic(const char*, registers_t*);

uint32_t page_directory[1024] __attribute__((aligned(4096)));

void paging_init() {
    // Identity map the entire 4GB space using 4MB pages (PSE)
    for(int i=0; i<1024; i++) {
        // bit 7 (0x80) = Page Size (4MB)
        // bit 1 (0x02) = Read/Write
        // bit 0 (0x01) = Present
        page_directory[i] = (i * 0x400000) | 0x83;
    }
    
    register_interrupt_handler(14, page_fault);

    // Enable PSE (Page Size Extension) in CR4
    uint32_t cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r" (cr4));
    cr4 |= 0x00000010; // Bit 4 is PSE
    __asm__ volatile("mov %0, %%cr4" :: "r" (cr4));

    // Load CR3 with page directory
    __asm__ volatile("mov %0, %%cr3":: "r"(page_directory));
    
    // Enable Paging in CR0
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0":: "r"(cr0));
    
    term_print("Paging enabled. 4GB Virtual Memory active.\n", 0x00FF00);
}

void page_fault(registers_t *regs) {
    uint32_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));
    panic("PAGE FAULT", regs);
}

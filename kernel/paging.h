#ifndef PAGING_H
#define PAGING_H
#include <stdint.h>
#include "isr.h"

void paging_init();
void page_fault(registers_t *regs);

#endif

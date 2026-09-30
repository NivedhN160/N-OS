#ifndef PIT_H
#define PIT_H
#include <stdint.h>
#include "isr.h"

void pit_init(uint32_t frequency);
uint32_t get_tick_count();

#endif

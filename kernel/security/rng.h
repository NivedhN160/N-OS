#ifndef RNG_H
#define RNG_H

#include <stdint.h>

// Initialize the RNG by pulling entropy from the hardware
void rng_init();

// Get a 32-bit random number
uint32_t rand();

#endif

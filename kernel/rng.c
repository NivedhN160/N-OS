#include "rng.h"
extern void term_print(const char *s, unsigned int col);

static uint32_t current_seed = 0;

// Read the CPU's Time Stamp Counter (RDTSC)
static inline uint32_t read_tsc() {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return lo;
}

void rng_init() {
    // Seed the RNG using the CPU's timestamp counter
    current_seed = read_tsc();
    term_print("RNG initialized using CPU entropy.\n", 0x00FFFF);
}

uint32_t rand() {
    if (current_seed == 0) {
        rng_init();
    }
    // Linear Congruential Generator (LCG) parameters
    current_seed = (1103515245 * current_seed + 12345);
    return current_seed;
}

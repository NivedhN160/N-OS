#include "fat32.h"
#include "block.h"

extern void term_print(const char *s, unsigned int col);

void fat32_init(uint32_t partition_offset) {
    // Stub
    term_print("FAT32 initialized (stub).\n", 0x00FF00);
}

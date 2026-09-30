#include "pmm.h"
#include <stdint.h>

#define PAGE_SIZE 4096
#define BLOCKS_PER_BUCKET 32

static uint32_t *pmm_bitmap = 0;
static uint32_t pmm_total_blocks = 0;
static uint32_t pmm_used_blocks = 0;

static inline void mmap_set(uint32_t bit) {
    pmm_bitmap[bit / BLOCKS_PER_BUCKET] |= (1 << (bit % BLOCKS_PER_BUCKET));
}

static inline void mmap_unset(uint32_t bit) {
    pmm_bitmap[bit / BLOCKS_PER_BUCKET] &= ~(1 << (bit % BLOCKS_PER_BUCKET));
}

static inline int mmap_test(uint32_t bit) {
    return pmm_bitmap[bit / BLOCKS_PER_BUCKET] & (1 << (bit % BLOCKS_PER_BUCKET));
}

typedef struct multiboot_memory_map {
    uint32_t size;
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
} __attribute__((packed)) multiboot_memory_map_t;

static int pmm_find_first_free() {
    for (uint32_t i = 0; i < pmm_total_blocks / BLOCKS_PER_BUCKET; i++) {
        if (pmm_bitmap[i] != 0xFFFFFFFF) {
            for (int j = 0; j < 32; j++) {
                if (!(pmm_bitmap[i] & (1 << j))) {
                    return i * BLOCKS_PER_BUCKET + j;
                }
            }
        }
    }
    return -1;
}

void pmm_init(multiboot_info_t *mbi) {
    // Assume 128MB RAM for simplicity if mbi memory size is not fully trusted,
    // or use mbi->mem_upper.
    uint32_t mem_size = (mbi->mem_lower + mbi->mem_upper) * 1024;
    if (mem_size == 0) mem_size = 128 * 1024 * 1024; // fallback

    pmm_total_blocks = mem_size / PAGE_SIZE;
    pmm_used_blocks = pmm_total_blocks; // Initially all used/reserved

    // Place bitmap after the kernel (e.g. 4MB mark for simplicity)
    pmm_bitmap = (uint32_t*)0x400000;
    
    // Set all to 1 (reserved)
    for (uint32_t i = 0; i < pmm_total_blocks / BLOCKS_PER_BUCKET; i++) {
        pmm_bitmap[i] = 0xFFFFFFFF;
    }

    // Now unreserve regions based on multiboot memory map
    if (mbi->flags & (1 << 6)) {
        multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)mbi->mmap_addr;
        while ((uint32_t)mmap < mbi->mmap_addr + mbi->mmap_length) {
            if (mmap->type == 1) {
                uint32_t start = mmap->base_addr_low;
                uint32_t len = mmap->length_low;
                // Unset bits for this free region
                for (uint32_t addr = start; addr < start + len; addr += PAGE_SIZE) {
                    mmap_unset(addr / PAGE_SIZE);
                    pmm_used_blocks--;
                }
            }
            mmap = (multiboot_memory_map_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
        }
    }

    // Re-reserve first 8MB for Kernel + Bitmap + Video
    for(uint32_t i=0; i < (8 * 1024 * 1024) / PAGE_SIZE; i++) {
        if (!mmap_test(i)) {
            mmap_set(i);
            pmm_used_blocks++;
        }
    }
}

void* pmm_alloc_frame() {
    int frame = pmm_find_first_free();
    if (frame == -1) return 0;
    mmap_set(frame);
    pmm_used_blocks++;
    return (void*)(frame * PAGE_SIZE);
}

void pmm_free_frame(void* p) {
    uint32_t addr = (uint32_t)p;
    int frame = addr / PAGE_SIZE;
    mmap_unset(frame);
    pmm_used_blocks--;
}

uint32_t pmm_get_free_frames() {
    return pmm_total_blocks - pmm_used_blocks;
}


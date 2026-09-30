#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include "../multiboot.h"

void pmm_init(multiboot_info_t *mbi);
void* pmm_alloc_frame();
void pmm_free_frame(void*);
uint32_t pmm_get_free_frames();

#endif

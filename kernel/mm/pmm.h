#ifndef PMM_H
#define PMM_H
void pmm_init();
void* pmm_alloc_frame();
void pmm_free_frame(void*);
#endif

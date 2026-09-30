#include "kmalloc.h"
#include "pmm.h"

extern void term_print(const char *s, unsigned int col);

typedef struct memory_block {
    size_t size;
    int is_free;
    struct memory_block* next;
} memory_block_t;

static memory_block_t* head = 0;

void kmalloc_init() {
    head = (memory_block_t*)pmm_alloc_frame();
    if(head) {
        head->size = 4096 - sizeof(memory_block_t);
        head->is_free = 1;
        head->next = 0;
    }
}

void* kmalloc(size_t size) {
    if (head == 0) kmalloc_init();
    if (size == 0) return 0;
    
    // align size to 4 bytes
    if(size % 4 != 0) size += 4 - (size % 4);

    memory_block_t* current = head;
    memory_block_t* last = head;
    
    while (current != 0) {
        if (current->is_free && current->size >= size) {
            // Split block if it's large enough
            if (current->size > size + sizeof(memory_block_t) + 4) {
                memory_block_t* new_block = (memory_block_t*)((uint8_t*)current + sizeof(memory_block_t) + size);
                new_block->is_free = 1;
                new_block->size = current->size - size - sizeof(memory_block_t);
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            current->is_free = 0;
            return (void*)((uint8_t*)current + sizeof(memory_block_t));
        }
        last = current;
        current = current->next;
    }
    
    // Need more memory. For simplicity, just grab enough pages from PMM 
    // assuming it gives us contiguous pages (it usually does early on).
    size_t required = size + sizeof(memory_block_t);
    int pages_needed = (required / 4096) + 1;
    memory_block_t* new_region = (memory_block_t*)pmm_alloc_frame();
    for(int i=1; i<pages_needed; i++) {
        pmm_alloc_frame(); // grab next continuous frames
    }
    
    if(!new_region) {
        term_print("kmalloc: Out of memory!\n", 0xFF0000);
        return 0;
    }
    
    new_region->size = (pages_needed * 4096) - sizeof(memory_block_t);
    new_region->is_free = 1;
    new_region->next = 0;
    last->next = new_region;
    
    return kmalloc(size); // try again
}

void kfree(void* ptr) {
    if (ptr == 0) return;
    memory_block_t* block = (memory_block_t*)((uint8_t*)ptr - sizeof(memory_block_t));
    block->is_free = 1;
    
    // Coalesce adjacent free blocks
    memory_block_t* current = head;
    while (current != 0 && current->next != 0) {
        if (current->is_free && current->next->is_free && 
           (uint8_t*)current + sizeof(memory_block_t) + current->size == (uint8_t*)current->next) {
            current->size += sizeof(memory_block_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

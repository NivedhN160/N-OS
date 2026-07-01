#include "kmalloc.h"
extern void term_print(const char *s, unsigned int col);

// We will reserve 16MB of memory for the heap, starting at physical address 16MB (0x1000000)
#define HEAP_START 0x1000000
#define HEAP_SIZE  0x1000000 // 16MB

typedef struct memory_block {
    size_t size;
    int is_free;
    struct memory_block* next;
} memory_block_t;

static memory_block_t* head = NULL;

void kmalloc_init() {
    head = (memory_block_t*)HEAP_START;
    head->size = HEAP_SIZE - sizeof(memory_block_t);
    head->is_free = 1;
    head->next = NULL;
    term_print("kmalloc: Heap initialized at 16MB boundary\n", 0x00FF00);
}

void* kmalloc(size_t size) {
    if (head == NULL) kmalloc_init();

    memory_block_t* current = head;
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            // Can we split this block?
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
        current = current->next;
    }
    term_print("kmalloc: Out of memory!\n", 0xFF0000);
    return NULL;
}

void kfree(void* ptr) {
    if (ptr == NULL) return;
    memory_block_t* block = (memory_block_t*)((uint8_t*)ptr - sizeof(memory_block_t));
    block->is_free = 1;
    
    // Coalesce adjacent free blocks
    memory_block_t* current = head;
    while (current != NULL && current->next != NULL) {
        if (current->is_free && current->next->is_free) {
            current->size += sizeof(memory_block_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

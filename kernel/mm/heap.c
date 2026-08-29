#include <stdint.h>
#include <stddef.h>
#include "../include/pmm.h"
#include "../include/vmm.h"

#define HEAP_START 0xFFFF800000000000 // Higher-half address for heap
#define HEAP_INITIAL_SIZE (1024 * 1024) // 1 MB

typedef struct heap_block {
    size_t size;
    bool free;
    struct heap_block *next;
} heap_block_t;

static heap_block_t *heap_head = NULL;
static uint64_t heap_end_addr = HEAP_START;

void heap_init(void) {
    // Map initial heap pages
    page_table_t *pml4 = vmm_get_kernel_pml4();
    for (uint64_t i = 0; i < HEAP_INITIAL_SIZE; i += PAGE_SIZE) {
        void *phys_page = pmm_alloc_page();
        vmm_map_page(pml4, HEAP_START + i, (uint64_t)phys_page, PAGE_PRESENT | PAGE_RW);
    }
    
    heap_end_addr = HEAP_START + HEAP_INITIAL_SIZE;
    heap_head = (heap_block_t*)HEAP_START;
    heap_head->size = HEAP_INITIAL_SIZE - sizeof(heap_block_t);
    heap_head->free = true;
    heap_head->next = NULL;
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    // Align to 8 bytes
    if (size % 8 != 0) {
        size = size + (8 - (size % 8));
    }

    heap_block_t *current = heap_head;
    while (current != NULL) {
        if (current->free && current->size >= size) {
            // Split block if large enough
            if (current->size >= size + sizeof(heap_block_t) + 8) {
                heap_block_t *new_block = (heap_block_t*)((uint8_t*)current + sizeof(heap_block_t) + size);
                new_block->free = true;
                new_block->size = current->size - size - sizeof(heap_block_t);
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            current->free = false;
            return (void*)((uint8_t*)current + sizeof(heap_block_t));
        }
        current = current->next;
    }
    
    // TODO: Expand heap by mapping more pages if out of memory
    return NULL; // Out of memory
}

void kfree(void *ptr) {
    if (!ptr) return;
    
    heap_block_t *block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    block->free = true;
    
    // Merge adjacent free blocks
    heap_block_t *current = heap_head;
    while (current != NULL && current->next != NULL) {
        if (current->free && current->next->free) {
            current->size += sizeof(heap_block_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

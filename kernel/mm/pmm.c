#include "../include/pmm.h"

extern void serial_write(const char *str);

// Bitmap implementation
static uint8_t *bitmap = NULL;
static uint64_t bitmap_size = 0;      // In bytes
static uint64_t total_pages = 0;
static uint64_t free_memory = 0;      // In bytes
static uint64_t last_alloc_index = 0; // For faster sequential allocation

#define BITMAP_SET(bit) (bitmap[(bit) / 8] |= (1 << ((bit) % 8)))
#define BITMAP_CLEAR(bit) (bitmap[(bit) / 8] &= ~(1 << ((bit) % 8)))
#define BITMAP_TEST(bit) ((bitmap[(bit) / 8] & (1 << ((bit) % 8))) != 0)

// Helper: Get length of memory map entries
#define MEMORY_MAP_NEXT(desc, size) ((EFI_MEMORY_DESCRIPTOR*)((uint8_t*)(desc) + (size)))

// Simple hex print helper for PMM
static void pmm_print_hex(uint64_t num) {
    char buf[17];
    buf[16] = '\0';
    for (int i = 15; i >= 0; i--) {
        uint8_t nibble = num & 0xF;
        if (nibble < 10) buf[i] = '0' + nibble;
        else buf[i] = 'A' + (nibble - 10);
        num >>= 4;
    }
    serial_write("0x");
    serial_write(buf);
}

void pmm_init(BootInfo *boot_info) {
    uint64_t max_memory = 0;
    
    // 1. Find the highest memory address to know how big our bitmap needs to be
    EFI_MEMORY_DESCRIPTOR *desc = boot_info->MemoryMap;
    uint64_t map_entries = boot_info->MemoryMapSize / boot_info->MemoryMapDescriptorSize;

    serial_write("PMM: Map Entries = ");
    pmm_print_hex(map_entries);
    serial_write("\r\n");
    
    for (uint64_t i = 0; i < map_entries; i++) {
        uint64_t top = desc->PhysicalStart + (desc->NumberOfPages * PAGE_SIZE);
        if (top > max_memory) {
            max_memory = top;
        }
        desc = MEMORY_MAP_NEXT(desc, boot_info->MemoryMapDescriptorSize);
    }
    
    total_pages = max_memory / PAGE_SIZE;
    bitmap_size = total_pages / 8;
    if (bitmap_size * 8 < total_pages) bitmap_size++; // Round up

    serial_write("PMM: Bitmap Size = ");
    pmm_print_hex(bitmap_size);
    serial_write("\r\n");

    // 2. Find a place to store the bitmap itself (needs to be EfiConventionalMemory and large enough)
    desc = boot_info->MemoryMap;
    for (uint64_t i = 0; i < map_entries; i++) {
        if (desc->Type == 7 && desc->PhysicalStart != 0) { // 7 = EfiConventionalMemory, skip address 0
            if (desc->NumberOfPages * PAGE_SIZE >= bitmap_size) {
                bitmap = (uint8_t*)desc->PhysicalStart;
                break;
            }
        }
        desc = MEMORY_MAP_NEXT(desc, boot_info->MemoryMapDescriptorSize);
    }

    if (!bitmap) {
        serial_write("PMM: Failed to allocate bitmap!\r\n");
        while(1);
    }

    // 3. Initialize the bitmap: set everything to 1 (used/reserved) by default
    for (uint64_t i = 0; i < bitmap_size; i++) {
        bitmap[i] = 0xFF;
    }

    // 4. Iterate again and clear bits (mark as free) for EfiConventionalMemory
    desc = boot_info->MemoryMap;
    for (uint64_t i = 0; i < map_entries; i++) {
        if (desc->Type == 7) { // EfiConventionalMemory
            uint64_t start_page = desc->PhysicalStart / PAGE_SIZE;
            uint64_t pages = desc->NumberOfPages;
            
            for (uint64_t p = 0; p < pages; p++) {
                BITMAP_CLEAR(start_page + p);
            }
            free_memory += pages * PAGE_SIZE;
        }
        desc = MEMORY_MAP_NEXT(desc, boot_info->MemoryMapDescriptorSize);
    }

    // 5. Mark the pages used by the bitmap itself as used!
    uint64_t bitmap_start_page = (uint64_t)bitmap / PAGE_SIZE;
    uint64_t bitmap_pages = (bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t p = 0; p < bitmap_pages; p++) {
        BITMAP_SET(bitmap_start_page + p);
        free_memory -= PAGE_SIZE;
    }

    serial_write("PMM: Initialized. Total memory tracked.\r\n");
}

void *pmm_alloc_pages(size_t count) {
    if (count == 0) return NULL;

    size_t contiguous = 0;
    uint64_t start_index = 0;

    for (uint64_t i = last_alloc_index; i < total_pages; i++) {
        if (!BITMAP_TEST(i)) {
            if (contiguous == 0) start_index = i;
            contiguous++;
            
            if (contiguous == count) {
                // Found a block!
                for (uint64_t j = start_index; j < start_index + count; j++) {
                    BITMAP_SET(j);
                }
                free_memory -= count * PAGE_SIZE;
                last_alloc_index = start_index + count;
                return (void*)(start_index * PAGE_SIZE);
            }
        } else {
            contiguous = 0;
        }
    }
    
    // If not found, wrap around (simple approach)
    if (last_alloc_index > 0) {
        last_alloc_index = 0;
        return pmm_alloc_pages(count);
    }

    serial_write("PMM: Out of Memory!\r\n");
    return NULL;
}

void *pmm_alloc_page(void) {
    return pmm_alloc_pages(1);
}

void pmm_free_pages(void *ptr, size_t count) {
    uint64_t start_page = (uint64_t)ptr / PAGE_SIZE;
    for (uint64_t p = 0; p < count; p++) {
        BITMAP_CLEAR(start_page + p);
    }
    free_memory += count * PAGE_SIZE;
    if (start_page < last_alloc_index) {
        last_alloc_index = start_page;
    }
}

void pmm_free_page(void *ptr) {
    pmm_free_pages(ptr, 1);
}

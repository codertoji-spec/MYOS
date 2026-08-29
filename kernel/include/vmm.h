#pragma once

#include <stdint.h>
#include <stdbool.h>

#define PAGE_PRESENT    (1ull << 0)
#define PAGE_RW         (1ull << 1)
#define PAGE_USER       (1ull << 2)
#define PAGE_WRITE_THRU (1ull << 3)
#define PAGE_CACHE_DIS  (1ull << 4)
#define PAGE_ACCESSED   (1ull << 5)
#define PAGE_DIRTY      (1ull << 6)
#define PAGE_LARGE      (1ull << 7)
#define PAGE_GLOBAL     (1ull << 8)
#define PAGE_NX         (1ull << 63)

#define PTRS_PER_TABLE 512

typedef uint64_t pt_entry_t;

// A Page Table can be a PML4, PDPT, PD, or PT. All have 512 64-bit entries.
__attribute__((aligned(4096)))
typedef struct {
    pt_entry_t entries[PTRS_PER_TABLE];
} page_table_t;

void vmm_init(void);
void vmm_map_page(page_table_t *pml4, uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags);
void vmm_unmap_page(page_table_t *pml4, uint64_t virtual_addr);
void vmm_switch_pml4(page_table_t *pml4);
page_table_t* vmm_get_kernel_pml4(void);

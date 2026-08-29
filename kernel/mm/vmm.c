#include "../include/vmm.h"
#include "../include/pmm.h"

extern void serial_write(const char *str);

// The kernel's top-level page table
static page_table_t *kernel_pml4 = NULL;

#define PML4_GET_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_GET_INDEX(addr) (((addr) >> 30) & 0x1FF)
#define PD_GET_INDEX(addr)   (((addr) >> 21) & 0x1FF)
#define PT_GET_INDEX(addr)   (((addr) >> 12) & 0x1FF)

// Helper to clear a page table
static void clear_table(page_table_t *table) {
    for (int i = 0; i < PTRS_PER_TABLE; i++) {
        table->entries[i] = 0;
    }
}

// Maps a virtual page to a physical page in the given PML4
void vmm_map_page(page_table_t *pml4, uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags) {
    uint64_t pml4_index = PML4_GET_INDEX(virtual_addr);
    uint64_t pdpt_index = PDPT_GET_INDEX(virtual_addr);
    uint64_t pd_index   = PD_GET_INDEX(virtual_addr);
    uint64_t pt_index   = PT_GET_INDEX(virtual_addr);

    // 1. Check PML4 entry
    if (!(pml4->entries[pml4_index] & PAGE_PRESENT)) {
        page_table_t *new_pdpt = (page_table_t*)pmm_alloc_page();
        clear_table(new_pdpt);
        pml4->entries[pml4_index] = (uint64_t)new_pdpt | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }
    
    // 2. Check PDPT entry
    page_table_t *pdpt = (page_table_t*)(pml4->entries[pml4_index] & ~0xFFF);
    if (!(pdpt->entries[pdpt_index] & PAGE_PRESENT)) {
        page_table_t *new_pd = (page_table_t*)pmm_alloc_page();
        clear_table(new_pd);
        pdpt->entries[pdpt_index] = (uint64_t)new_pd | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }

    // 3. Check PD entry
    page_table_t *pd = (page_table_t*)(pdpt->entries[pdpt_index] & ~0xFFF);
    if (!(pd->entries[pd_index] & PAGE_PRESENT)) {
        page_table_t *new_pt = (page_table_t*)pmm_alloc_page();
        clear_table(new_pt);
        pd->entries[pd_index] = (uint64_t)new_pt | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }

    // 4. Set PT entry
    page_table_t *pt = (page_table_t*)(pd->entries[pd_index] & ~0xFFF);
    pt->entries[pt_index] = (physical_addr & ~0xFFF) | flags;
}

void vmm_switch_pml4(page_table_t *pml4) {
    __asm__ volatile("mov %0, %%cr3" : : "r"((uint64_t)pml4));
}

page_table_t* vmm_get_kernel_pml4(void) {
    return kernel_pml4;
}

void vmm_init(void) {
    // Allocate the kernel PML4
    kernel_pml4 = (page_table_t*)pmm_alloc_page();
    clear_table(kernel_pml4);

    // We must identity map the first few gigabytes of memory so that 
    // the kernel can continue executing exactly where it is in physical memory,
    // and so it can access MMIO devices like the Framebuffer.
    
    // Let's identity map the first 4GB (0x0 to 0x100000000)
    serial_write("VMM: Identity mapping first 4GB of physical memory...\r\n");
    for (uint64_t i = 0; i < 0x100000000; i += PAGE_SIZE) {
        vmm_map_page(kernel_pml4, i, i, PAGE_PRESENT | PAGE_RW | PAGE_USER);
    }

    // Switch to our brand new page table
    vmm_switch_pml4(kernel_pml4);
    serial_write("VMM: Switched to Kernel PML4.\r\n");
}

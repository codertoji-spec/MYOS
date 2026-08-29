#include "../include/fs/elf.h"
#include "../include/pmm.h"
#include "../include/vmm.h"

extern void serial_write(const char *str);

uint64_t elf_load(vfs_node_t *file, void **out_pml4) {
    Elf64_Ehdr header;
    
    // Read the ELF Header
    if (vfs_read(file, 0, sizeof(Elf64_Ehdr), (uint8_t*)&header) != sizeof(Elf64_Ehdr)) {
        serial_write("ELF: Failed to read header.\r\n");
        return 0;
    }
    
    // Verify Magic
    uint32_t magic = *(uint32_t*)header.e_ident;
    if (magic != ELF_MAGIC) {
        serial_write("ELF: Invalid Magic.\r\n");
        return 0;
    }
    
    // Create new PML4
    page_table_t *new_pml4 = (page_table_t*)pmm_alloc_page();
    page_table_t *kernel_pml4 = vmm_get_kernel_pml4();
    
    // Clear new PML4
    for (int i = 0; i < 512; i++) {
        new_pml4->entries[i] = 0;
    }
    
    // Copy the Kernel's PML4 mappings (index 0 covers 0 - 512GB)
    // This allows the user thread to call kernel code during syscalls.
    new_pml4->entries[0] = kernel_pml4->entries[0];
    
    // Read Program Headers
    Elf64_Phdr phdr;
    for (int i = 0; i < header.e_phnum; i++) {
        uint32_t offset = header.e_phoff + (i * header.e_phentsize);
        vfs_read(file, offset, sizeof(Elf64_Phdr), (uint8_t*)&phdr);
        
        if (phdr.p_type == PT_LOAD) {
            uint64_t virt_start = phdr.p_vaddr & ~0xFFFULL;
            uint64_t virt_end = (phdr.p_vaddr + phdr.p_memsz + 0xFFFULL) & ~0xFFFULL;
            uint32_t pages = (uint32_t)((virt_end - virt_start) / 4096);
            if (pages == 0) pages = 1;
            
            uint64_t phys_addr = (uint64_t)pmm_alloc_pages(pages);
            
            // Zero out allocated physical memory
            uint8_t *mem = (uint8_t *)phys_addr;
            for (uint32_t j = 0; j < pages * 4096; j++) {
                mem[j] = 0;
            }
            
            // Read file data into the memory at the in-page offset
            uint32_t page_offset = (uint32_t)(phdr.p_vaddr & 0xFFFULL);
            if (phdr.p_filesz > 0) {
                vfs_read(file, phdr.p_offset, phdr.p_filesz, mem + page_offset);
            }
            
            // Map each page into the user's PML4
            for (uint32_t j = 0; j < pages; j++) {
                uint64_t virt = virt_start + (j * 4096);
                uint64_t phys = phys_addr + (j * 4096);
                vmm_map_page(new_pml4, virt, phys, PAGE_PRESENT | PAGE_RW | PAGE_USER);
            }
        }
    }
    
    *out_pml4 = new_pml4;
    return header.e_entry;
}

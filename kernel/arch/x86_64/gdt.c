#include "../../include/gdt.h"

#define GDT_ENTRIES 7

// 0: Null
// 1: Kernel Code
// 2: Kernel Data
// 3: User Code
// 4: User Data
// 5: TSS (Low)
// 6: TSS (High)
static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gdtr;
static struct tss_entry tss;

extern void gdt_flush(uint64_t gdtr);

static void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[index].base_low = (base & 0xFFFF);
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high = (base >> 24) & 0xFF;

    gdt[index].limit_low = (limit & 0xFFFF);
    gdt[index].granularity = ((limit >> 16) & 0x0F);

    gdt[index].granularity |= gran & 0xF0;
    gdt[index].access = access;
}

void gdt_init(void) {
    gdtr.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gdtr.base = (uint64_t)&gdt;

    // Null descriptor
    gdt_set_entry(0, 0, 0, 0, 0);
    
    // Kernel Code Segment
    // Access: Present(1) | Privilege(00) | Type(1) | Executable(1) | Direction(0) | Read/Write(1) | Accessed(0) = 0x9A
    // Granularity: Page(1) | 64-bit(1) | 32-bit(0) | Reserved(0) = 0xA0
    gdt_set_entry(1, 0, 0, 0x9A, 0xA0);
    
    // Kernel Data Segment
    // Access: Present(1) | Privilege(00) | Type(1) | Executable(0) | Direction(0) | Read/Write(1) | Accessed(0) = 0x92
    gdt_set_entry(2, 0, 0, 0x92, 0xA0);
    
    // User Code Segment (Privilege level 3)
    // Access: 0xFA
    gdt_set_entry(3, 0, 0, 0xFA, 0xA0);
    
    // User Data Segment (Privilege level 3)
    // Access: 0xF2
    gdt_set_entry(4, 0, 0, 0xF2, 0xA0);

    // Write TSS Segment
    uint64_t tss_base = (uint64_t)&tss;
    uint32_t tss_limit = sizeof(struct tss_entry);
    
    gdt[5].limit_low = tss_limit & 0xFFFF;
    gdt[5].base_low = tss_base & 0xFFFF;
    gdt[5].base_middle = (tss_base >> 16) & 0xFF;
    gdt[5].access = 0x89; // Present | Executable | Accessed
    gdt[5].granularity = ((tss_limit >> 16) & 0x0F) | 0x00;
    gdt[5].base_high = (tss_base >> 24) & 0xFF;
    
    // 64-bit TSS needs the upper 32 bits of the base
    struct gdt_entry* tss_high = (struct gdt_entry*)&gdt[6];
    uint32_t *tss_high_val = (uint32_t*)tss_high;
    *tss_high_val = (tss_base >> 32) & 0xFFFFFFFF;
    *(tss_high_val + 1) = 0;

    gdt_flush((uint64_t)&gdtr);

    // Load the TSS
    __asm__ volatile("ltr %%ax" : : "a"((uint16_t)GDT_TSS_SELECTOR));
}

void tss_set_stack(uint64_t stack) {
    tss.rsp0 = stack;
}

#include "../../include/idt.h"

#define IDT_ENTRIES 256

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idtr;

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags, uint8_t ist) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_mid = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    
    idt[num].selector = sel;
    idt[num].ist = ist & 0x07;
    idt[num].type_attr = flags;
    idt[num].zero = 0;
}

void idt_init(void) {
    idtr.base = (uint64_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(struct idt_entry) * 256 - 1;

    for (int i = 0; i < 256; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].ist = 0;
        idt[i].type_attr = 0;
        idt[i].offset_mid = 0;
        idt[i].offset_high = 0;
        idt[i].zero = 0;
    }
}

extern uint64_t isr_stub_table[];

extern void isr128(void);

void isrs_install(void) {
    for (int i = 0; i < 48; i++) {
        idt_set_gate(i, isr_stub_table[i], 0x08, 0x8E, 0);
    }
    
    // System call interrupt 0x80 (128) - DPL 3
    // Present(1) | DPL(11) | Type(1110) = 0xEE
    idt_set_gate(128, (uint64_t)isr128, 0x08, 0xEE, 0);

    __asm__ volatile("lidt %0" : : "m"(idtr));
}

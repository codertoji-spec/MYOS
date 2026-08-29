#include "../include/ioapic.h"
#include "../include/acpi.h"

extern void serial_write(const char *str);

static uint64_t ioapic_base = 0;

static uint32_t ioapic_read(uint32_t reg) {
    *((volatile uint32_t*)ioapic_base) = reg;
    return *((volatile uint32_t*)(ioapic_base + 0x10));
}

static void ioapic_write(uint32_t reg, uint32_t val) {
    *((volatile uint32_t*)ioapic_base) = reg;
    *((volatile uint32_t*)(ioapic_base + 0x10)) = val;
}

void ioapic_init(void) {
    ioapic_base = acpi_get_ioapic_base();
    if (ioapic_base == 0) {
        serial_write("IOAPIC: Base is 0, cannot initialize!\r\n");
        return;
    }
    serial_write("IOAPIC: Initialized.\r\n");
}

void ioapic_set_entry(uint8_t irq, uint8_t vector) {
    uint32_t low_index = 0x10 + irq * 2;
    uint32_t high_index = 0x10 + irq * 2 + 1;

    // Vector, Fixed Delivery Mode, Unmasked, Edge Triggered, Active High
    uint32_t low = vector;
    // Destination APIC ID 0
    uint32_t high = 0;

    ioapic_write(low_index, low);
    ioapic_write(high_index, high);
}

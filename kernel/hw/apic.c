#include "../include/apic.h"
#include "../include/acpi.h"
#include "../include/vmm.h"

extern void serial_write(const char *str);

#define LAPIC_SVR 0xF0 // Spurious Interrupt Vector Register
#define LAPIC_EOI 0x0B0 // End of Interrupt Register

static uint64_t lapic_base = 0;

static uint32_t apic_read(uint32_t reg) {
    return *((volatile uint32_t*)(lapic_base + reg));
}

static void apic_write(uint32_t reg, uint32_t val) {
    *((volatile uint32_t*)(lapic_base + reg)) = val;
}

void apic_init(void) {
    lapic_base = acpi_get_lapic_base();
    if (lapic_base == 0) {
        serial_write("APIC: Local APIC base is 0, cannot initialize!\r\n");
        return;
    }

    // Ensure the APIC memory is mapped in our page tables.
    // Usually it's at 0xFEE00000, which is below 4GB, so our identity map covers it!

    // Enable APIC and set spurious interrupt vector to 0xFF
    apic_write(LAPIC_SVR, 0x1FF); // 0x100 (enable) | 0xFF (vector)

    serial_write("APIC: Initialized and Enabled.\r\n");
}

#define LAPIC_TIMER 0x320
#define LAPIC_TIMER_INITCNT 0x380
#define LAPIC_TIMER_CURCNT  0x390
#define LAPIC_TIMER_DIV     0x3E0

void apic_timer_init(void) {
    // Set divider to 16
    apic_write(LAPIC_TIMER_DIV, 0x03);

    // Set LVT Timer for Periodic mode (bit 17) and vector 32
    apic_write(LAPIC_TIMER, 32 | 0x20000);

    // Set initial count for 100Hz (10ms) periodic timer
    apic_write(LAPIC_TIMER_INITCNT, 625000);
}

void apic_eoi(void) {
    apic_write(LAPIC_EOI, 0);
}

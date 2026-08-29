#include "../include/acpi.h"
#include <stddef.h>

extern void serial_write(const char *str);

static void *rsdt_or_xsdt = NULL;
static bool use_xsdt = false;
static uint64_t lapic_base = 0;

static bool memcmp_sig(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

static uint64_t ioapic_base = 0;

void acpi_init(BootInfo *boot_info) {
    if (!boot_info->Rsdp) {
        serial_write("ACPI: RSDP not found in BootInfo!\r\n");
        return;
    }

    acpi_rsdp_t *rsdp = (acpi_rsdp_t*)boot_info->Rsdp;
    
    if (rsdp->Revision >= 2) {
        acpi_rsdp2_t *rsdp2 = (acpi_rsdp2_t*)rsdp;
        rsdt_or_xsdt = (void*)rsdp2->XsdtAddress;
        use_xsdt = true;
        serial_write("ACPI: Found XSDT.\r\n");
    } else {
        rsdt_or_xsdt = (void*)(uint64_t)rsdp->RsdtAddress;
        use_xsdt = false;
        serial_write("ACPI: Found RSDT.\r\n");
    }

    // Find MADT to get Local APIC
    acpi_madt_t *madt = (acpi_madt_t*)acpi_find_table("APIC");
    if (madt) {
        lapic_base = madt->LocalAPICAddress;
        serial_write("ACPI: Found MADT (APIC).\r\n");

        // Parse MADT entries
        uint8_t *ptr = (uint8_t*)madt + sizeof(acpi_madt_t);
        uint8_t *end = (uint8_t*)madt + madt->h.Length;

        while (ptr < end) {
            uint8_t type = ptr[0];
            uint8_t len = ptr[1];

            if (type == 1) { // IOAPIC
                ioapic_base = *(uint32_t*)(ptr + 4);
                serial_write("ACPI: Found IOAPIC.\r\n");
            }
            ptr += len;
        }

    } else {
        serial_write("ACPI: MADT not found!\r\n");
    }
}

void* acpi_find_table(const char *signature) {
    if (!rsdt_or_xsdt) return NULL;

    acpi_sdt_header_t *header = (acpi_sdt_header_t*)rsdt_or_xsdt;
    uint32_t entries = (header->Length - sizeof(acpi_sdt_header_t)) / (use_xsdt ? 8 : 4);

    for (uint32_t i = 0; i < entries; i++) {
        acpi_sdt_header_t *table_header;
        if (use_xsdt) {
            uint64_t *xsdt = (uint64_t*)((uint8_t*)header + sizeof(acpi_sdt_header_t));
            table_header = (acpi_sdt_header_t*)xsdt[i];
        } else {
            uint32_t *rsdt = (uint32_t*)((uint8_t*)header + sizeof(acpi_sdt_header_t));
            table_header = (acpi_sdt_header_t*)(uint64_t)rsdt[i];
        }

        if (memcmp_sig(table_header->Signature, signature, 4)) {
            return table_header;
        }
    }
    return NULL;
}

uint64_t acpi_get_lapic_base(void) {
    return lapic_base;
}

uint64_t acpi_get_ioapic_base(void) {
    return ioapic_base;
}

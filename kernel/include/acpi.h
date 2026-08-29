#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "bootinfo.h"

typedef struct {
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;
} __attribute__((packed)) acpi_rsdp_t;

typedef struct {
    acpi_rsdp_t firstPart;
    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t ExtendedChecksum;
    uint8_t reserved[3];
} __attribute__((packed)) acpi_rsdp2_t;

typedef struct {
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    char OEMID[6];
    char OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;
} __attribute__((packed)) acpi_sdt_header_t;

typedef struct {
    acpi_sdt_header_t h;
    uint32_t LocalAPICAddress;
    uint32_t Flags;
} __attribute__((packed)) acpi_madt_t;

void acpi_init(BootInfo *boot_info);
void* acpi_find_table(const char *signature);
uint64_t acpi_get_lapic_base(void);
uint64_t acpi_get_ioapic_base(void);

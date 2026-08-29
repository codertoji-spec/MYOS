#include "../include/ata.h"
#include "../include/io.h" // We need an inb/outb/insw header? We might need to add one.

extern void serial_write(const char *str);

#define ATA_PRIMARY_IO      0x1F0
#define ATA_PRIMARY_CTRL    0x3F6



static inline void insw(uint16_t port, void *addr, uint32_t count) {
    __asm__ volatile("rep insw" : "+D"(addr), "+c"(count) : "d"(port) : "memory");
}

void ata_wait_bsy(void) {
    while (inb(ATA_PRIMARY_IO + 7) & 0x80);
}

void ata_wait_drq(void) {
    while (!(inb(ATA_PRIMARY_IO + 7) & 0x08));
}

void ata_init(void) {
    serial_write("ATA: Initializing Primary PIO...\r\n");
    // Just a placeholder, assume drive exists and is working.
}

void ata_read_sector(uint32_t lba, uint8_t *buffer) {
    ata_wait_bsy();
    
    // Select master drive, LBA mode, and bits 24-27 of LBA
    outb(ATA_PRIMARY_IO + 6, 0xE0 | ((lba >> 24) & 0x0F));
    
    outb(ATA_PRIMARY_IO + 2, 1); // Read 1 sector
    outb(ATA_PRIMARY_IO + 3, (uint8_t) lba);
    outb(ATA_PRIMARY_IO + 4, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_IO + 5, (uint8_t)(lba >> 16));
    
    // Send READ SECTORS command
    outb(ATA_PRIMARY_IO + 7, 0x20);
    
    ata_wait_bsy();
    ata_wait_drq();
    
    // Read 256 words (512 bytes)
    insw(ATA_PRIMARY_IO, buffer, 256);
}

void ata_read_sectors(uint32_t lba, uint8_t *buffer, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        ata_read_sector(lba + i, buffer + (i * 512));
    }
}

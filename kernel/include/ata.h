#pragma once

#include <stdint.h>

void ata_init(void);
void ata_read_sector(uint32_t lba, uint8_t *buffer);
void ata_read_sectors(uint32_t lba, uint8_t *buffer, uint32_t count);

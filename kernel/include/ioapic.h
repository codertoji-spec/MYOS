#pragma once

#include <stdint.h>

void ioapic_init(void);
void ioapic_set_entry(uint8_t irq, uint8_t vector);

#pragma once
#include <stdint.h>

void shell_init(void);
void shell_tick(void);       // Call this from the main idle loop
void shell_puts(const char *s);
void shell_putchar(char c);

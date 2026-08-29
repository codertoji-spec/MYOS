#pragma once

#include <stdint.h>

void keyboard_init(void);
void keyboard_handler(void);
char keyboard_getchar(void);   // Non-blocking: returns 0 if no key waiting

#pragma once
#include <stdint.h>

void mouse_init(void);
void mouse_handler(void);
int mouse_get_x(void);
int mouse_get_y(void);
uint8_t mouse_get_buttons(void);

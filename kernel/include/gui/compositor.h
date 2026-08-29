#pragma once

#include <stdint.h>
#include "../bootinfo.h"

// Core init & primitives
void compositor_init(BootInfo *boot_info);
void compositor_draw_rect(int x, int y, int width, int height, uint32_t color);
void compositor_draw_char(int x, int y, char c, uint32_t fg_color, uint32_t bg_color);
void compositor_draw_string(int x, int y, const char *str, uint32_t fg_color, uint32_t bg_color);
void compositor_render(void);

// Desktop / GUI
void compositor_draw_desktop(void);
void compositor_draw_taskbar(void);
void compositor_draw_window(int x, int y, int w, int h, const char *title);

// Screen metrics
int  compositor_screen_width(void);
int  compositor_screen_height(void);

// Input
void compositor_handle_key(char c);
void compositor_set_cursor(int x, int y);

void compositor_blit_doom(uint32_t *src, int src_w, int src_h);

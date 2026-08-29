#include "doomgeneric.h"
#include <stdint.h>
#include <stdio.h>

uint32_t doom_screen[DOOMGENERIC_RESX * DOOMGENERIC_RESY];

void DG_Init() {
    printf("DG_Init: setting screen buffer\n");
    DG_ScreenBuffer = doom_screen;
}

void DG_DrawFrame() {
    __asm__ volatile("int $0x80" : : "a"(7), "b"(DG_ScreenBuffer));
}

uint32_t DG_GetTicksMs() {
    uint64_t ticks;
    __asm__ volatile("int $0x80" : "=a"(ticks) : "a"(10) : "memory");
    return (uint32_t)ticks;
}

void DG_SleepMs(uint32_t ms) {
    uint32_t start = DG_GetTicksMs();
    while ((DG_GetTicksMs() - start) < ms) {
        __asm__ volatile("pause");
    }
}


int DG_GetKey(int *pressed, unsigned char *doomKey) {
    int result;
    __asm__ volatile("int $0x80" : "=a"(result) : "a"(8) : "memory");
    if (!result) return 0;
    *pressed = (result >> 8) & 1;
    *doomKey = (unsigned char)(result & 0xFF);
    return 1;
}

void DG_SetWindowTitle(const char *title) { (void)title; }

void _start() {
    printf("Starting DOOM engine...\n");
    char *argv[] = { "doom", "-iwad", "doom1.wad", NULL };
    int argc = 3;
    doomgeneric_Create(argc, argv);
    while(1) { doomgeneric_Tick(); }
}

#include "../include/bootinfo.h"

// Basic COM1 Serial Port for Logging
#define PORT 0x3f8

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

void serial_init() {
    outb(PORT + 1, 0x00);    // Disable all interrupts
    outb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(PORT + 1, 0x00);    //                  (hi byte)
    outb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

void serial_write_char(char c) {
    while ((inb(PORT + 5) & 0x20) == 0);
    outb(PORT, c);
}

void serial_write(const char *str) {
    while (*str) {
        serial_write_char(*str++);
    }
}

// Simple Framebuffer Plot Pixel
void put_pixel(BootInfo* boot_info, uint32_t x, uint32_t y, uint32_t color) {
    uint32_t* fb = (uint32_t*)boot_info->FramebufferBase;
    fb[y * boot_info->Framebuffer->PixelsPerScanLine + x] = color;
}

// Draw a colored rectangle
void draw_rect(BootInfo* boot_info, uint32_t start_x, uint32_t start_y, uint32_t width, uint32_t height, uint32_t color) {
    for (uint32_t y = start_y; y < start_y + height; y++) {
        for (uint32_t x = start_x; x < start_x + width; x++) {
            put_pixel(boot_info, x, y, color);
        }
    }
}

#include "../include/gdt.h"
#include "../include/idt.h"
#include "../include/isr.h"
#include "../include/pmm.h"
#include "../include/vmm.h"
#include "../include/heap.h"
#include "../include/acpi.h"
#include "../include/apic.h"
#include "../include/ioapic.h"
#include "../include/keyboard.h"
#include "../include/mouse.h"
#include "../include/fs/vfs.h"
#include "../include/fs/fat32.h"
#include "../include/fs/elf.h"
#include "../include/ata.h"
#include "../include/gui/compositor.h"
#include "../include/shell.h"

// Simple integer to hex string function for debug printing
static void print_hex(uint64_t num) {
    char buf[17];
    buf[16] = '\0';
    for (int i = 15; i >= 0; i--) {
        uint8_t nibble = num & 0xF;
        if (nibble < 10) buf[i] = '0' + nibble;
        else buf[i] = 'A' + (nibble - 10);
        num >>= 4;
    }
    serial_write("0x");
    serial_write(buf);
}

#include "../include/task/thread.h"

static void enable_sse(void) {
    uint64_t cr0, cr4;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2); // Clear CR0.EM (Emulation)
    cr0 |= (1 << 1);  // Set CR0.MP (Monitor Coprocessor)
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));

    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 9);  // Set CR4.OSFXSR (Operating System FXSAVE/FXRSTOR Support)
    cr4 |= (1 << 10); // Set CR4.OSXMMEXCPT (Operating System Unmasked Exception Support)
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
}

// The Kernel Entry Point
void _start(BootInfo* boot_info) {
    serial_init();
    serial_write("Kernel Started!\r\n");

    gdt_init();
    enable_sse();
    serial_write("GDT & SSE Initialized.\r\n");

    idt_init();
    isrs_install();
    serial_write("IDT & ISRs Initialized.\r\n");

    pmm_init(boot_info);
    vmm_init();
    heap_init();
    serial_write("Heap Initialized.\r\n");



    acpi_init(boot_info);
    apic_init();
    ioapic_init();
    apic_timer_init();

    // Route IRQ 1 (Keyboard) to Vector 33
    ioapic_set_entry(1, 33);
    keyboard_init();

    // Route IRQ 12 (PS/2 Mouse) to Vector 44
    ioapic_set_entry(12, 44);
    mouse_init();

    ata_init();
    vfs_init();
    fat32_init();


    thread_init();
    compositor_init(boot_info);
    shell_init();   // Shell takes over the full screen


    serial_write("Shell initialized. Enabling Interrupts...\r\n");
    __asm__ volatile("sti");

    // Main idle loop — poll the shell for keyboard input
    while (1) {
        shell_tick();
        __asm__ volatile("pause"); // hint CPU to relax
    }
}

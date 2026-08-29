#include "../include/mouse.h"
#include "../include/io.h"
#include "../include/apic.h"

extern void serial_write(const char *str);

// PS/2 mouse ports
#define MOUSE_DATA_PORT   0x60
#define MOUSE_STATUS_PORT 0x64
#define MOUSE_CMD_PORT    0x64

// Mouse state
static int mouse_x = 400;
static int mouse_y = 300;
static uint8_t mouse_buttons = 0;

// Packet assembly: PS/2 sends 3 bytes per movement event
static uint8_t mouse_cycle = 0;
static uint8_t mouse_packet[3];

static void mouse_wait_input(void) {
    // Wait until bit 1 of status is clear (input buffer empty)
    int timeout = 100000;
    while (timeout-- && (inb(MOUSE_STATUS_PORT) & 0x02));
}

static void mouse_wait_output(void) {
    // Wait until bit 0 of status is set (output buffer full)
    int timeout = 100000;
    while (timeout-- && !(inb(MOUSE_STATUS_PORT) & 0x01));
}

static void mouse_write(uint8_t data) {
    // Tell controller we're sending to mouse (auxiliary device)
    mouse_wait_input();
    outb(MOUSE_CMD_PORT, 0xD4);
    mouse_wait_input();
    outb(MOUSE_DATA_PORT, data);
}

static uint8_t mouse_read(void) {
    mouse_wait_output();
    return inb(MOUSE_DATA_PORT);
}

void mouse_init(void) {
    // Enable auxiliary PS/2 device (mouse)
    mouse_wait_input();
    outb(MOUSE_CMD_PORT, 0xA8);

    // Enable mouse interrupt (bit 1 of compaq status byte)
    mouse_wait_input();
    outb(MOUSE_CMD_PORT, 0x20);          // Read current config byte
    mouse_wait_output();
    uint8_t status = inb(MOUSE_DATA_PORT);
    status |= 0x02;  // Enable mouse IRQ12
    status &= ~0x20; // Enable mouse clock
    mouse_wait_input();
    outb(MOUSE_CMD_PORT, 0x60);          // Write config byte
    mouse_wait_input();
    outb(MOUSE_DATA_PORT, status);

    // Reset mouse
    mouse_write(0xFF);
    mouse_read(); // ACK (0xFA)
    mouse_read(); // Self-test pass (0xAA)
    mouse_read(); // Mouse ID (0x00)

    // Set default settings
    mouse_write(0xF6);
    mouse_read(); // ACK

    // Enable data reporting
    mouse_write(0xF4);
    mouse_read(); // ACK

    serial_write("PS/2 Mouse Initialized.\r\n");
}

void mouse_handler(void) {
    uint8_t data = inb(MOUSE_DATA_PORT);

    switch (mouse_cycle) {
        case 0:
            // First byte: button flags + overflow bits
            // Bit 3 must be set for a valid packet
            if (!(data & 0x08)) {
                // Invalid packet start, resync
                break;
            }
            mouse_packet[0] = data;
            mouse_cycle = 1;
            break;
        case 1:
            mouse_packet[1] = data;
            mouse_cycle = 2;
            break;
        case 2:
            mouse_packet[2] = data;
            mouse_cycle = 0;

            // Parse the packet
            mouse_buttons = mouse_packet[0] & 0x07;

            // X delta: signed 9-bit (sign in packet[0] bit 4)
            int dx = (int)mouse_packet[1];
            if (mouse_packet[0] & 0x10) dx -= 256;

            // Y delta: signed 9-bit, but Y axis is inverted on PS/2
            int dy = (int)mouse_packet[2];
            if (mouse_packet[0] & 0x20) dy -= 256;
            dy = -dy; // Invert Y so moving mouse up goes up on screen

            mouse_x += dx;
            mouse_y += dy;

            // Clamp to screen bounds (assume 1024x768 for VirtualBox)
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_x > 1023) mouse_x = 1023;
            if (mouse_y > 767) mouse_y = 767;

            // Tell the compositor to update cursor position
            extern void compositor_set_cursor(int x, int y);
            compositor_set_cursor(mouse_x, mouse_y);
            break;
    }

    apic_eoi();
}

int mouse_get_x(void) { return mouse_x; }
int mouse_get_y(void) { return mouse_y; }
uint8_t mouse_get_buttons(void) { return mouse_buttons; }

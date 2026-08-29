#include "../include/keyboard.h"
#include "../include/io.h"
#include "../include/apic.h"
#include "../include/gui/compositor.h"

extern void serial_write(const char *str);

// ── Keyboard ring buffer ──────────────────────────────────────────────────────
#define KEY_BUF_SIZE 256
static char key_buf[KEY_BUF_SIZE];
static volatile int key_read_idx  = 0;
static volatile int key_write_idx = 0;

static void key_buf_push(char c) {
    int next = (key_write_idx + 1) % KEY_BUF_SIZE;
    if (next != key_read_idx) {   // drop if full
        key_buf[key_write_idx] = c;
        key_write_idx = next;
    }
}

// Returns 0 if buffer is empty
char keyboard_getchar(void) {
    if (key_read_idx == key_write_idx) return 0;
    char c = key_buf[key_read_idx];
    key_read_idx = (key_read_idx + 1) % KEY_BUF_SIZE;
    return c;
}


// Simple US QWERTY scancode set 1 map
const char scancode_chars[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', // 0-14
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', // 15-28
    0, // Ctrl
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', // 30-41
    0, // LShift
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', // 43-53
    0, // RShift
    '*', // Keypad *
    0, // LAlt
    ' ', // Space
    0, // CapsLock
    // ... rest can be 0 for now
};

void keyboard_init(void) {
    // Clear out any pending data
    while (inb(0x64) & 1) {
        inb(0x60);
    }
    serial_write("PS/2 Keyboard Initialized.\r\n");
}

// Doom key event queue
#define KEY_QUEUE_SIZE 64
static struct {
    int pressed;
    unsigned char key;
} key_queue[KEY_QUEUE_SIZE];
static volatile int key_queue_head = 0;
static volatile int key_queue_tail = 0;

void keyboard_push_event(int pressed, unsigned char key) {
    int next = (key_queue_head + 1) % KEY_QUEUE_SIZE;
    if (next != key_queue_tail) {
        key_queue[key_queue_head].pressed = pressed;
        key_queue[key_queue_head].key = key;
        key_queue_head = next;
    }
}

int keyboard_pop_event(int *pressed, unsigned char *key) {
    if (key_queue_head == key_queue_tail) return 0;
    *pressed = key_queue[key_queue_tail].pressed;
    *key = key_queue[key_queue_tail].key;
    key_queue_tail = (key_queue_tail + 1) % KEY_QUEUE_SIZE;
    return 1;
}

static unsigned char scancode_to_doom(uint8_t scancode, int is_extended) {
    if (is_extended) {
        switch (scancode) {
            case 0x48: return 0xad; // KEY_UPARROW
            case 0x50: return 0xaf; // KEY_DOWNARROW
            case 0x4B: return 0xac; // KEY_LEFTARROW
            case 0x4D: return 0xae; // KEY_RIGHTARROW
            case 0x1D: return 0xa3; // KEY_FIRE (RCTRL)
            case 0x38: return 0xb8; // KEY_RALT
            case 0x1C: return 13;   // KEY_ENTER
        }
    }
    switch (scancode) {
        case 0x01: return 27;   // KEY_ESCAPE
        case 0x1C: return 13;   // KEY_ENTER
        case 0x0E: return 127;  // KEY_BACKSPACE
        case 0x0F: return 9;    // KEY_TAB
        case 0x1D: return 0xa3; // KEY_FIRE (LCTRL)
        case 0x2A: return 0xb6; // KEY_RSHIFT (LSHIFT)
        case 0x36: return 0xb6; // KEY_RSHIFT (RSHIFT)
        case 0x38: return 0xb8; // KEY_LALT
        case 0x39: return 0xa2; // KEY_USE (Space)
        case 0x48: return 0xad; // KEY_UPARROW
        case 0x50: return 0xaf; // KEY_DOWNARROW
        case 0x4B: return 0xac; // KEY_LEFTARROW
        case 0x4D: return 0xae; // KEY_RIGHTARROW
        case 0x11: return 0xad; // 'w' -> KEY_UPARROW
        case 0x1F: return 0xaf; // 's' -> KEY_DOWNARROW
        case 0x1E: return 0xac; // 'a' -> KEY_LEFTARROW
        case 0x20: return 0xae; // 'd' -> KEY_RIGHTARROW
        case 0x12: return 0xa2; // 'e' -> KEY_USE
        default:
            if (scancode < 128 && scancode_chars[scancode]) {
                char c = scancode_chars[scancode];
                if (c == '\n') return 13;
                return (unsigned char)c;
            }
            return 0;
    }
}

static int extended_scancode = 0;

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);

    if (scancode == 0xE0) {
        extended_scancode = 1;
        apic_eoi();
        return;
    }

    int pressed = !(scancode & 0x80);
    uint8_t code = scancode & 0x7F;

    unsigned char doom_key = scancode_to_doom(code, extended_scancode);
    if (doom_key) {
        keyboard_push_event(pressed, doom_key);
    }

    if (pressed && !extended_scancode) {
        char c = scancode_chars[code];
        if (c) {
            key_buf_push(c);
            char buf[2] = {c, '\0'};
            serial_write(buf);
        }
    }

    extended_scancode = 0;
    apic_eoi();
}

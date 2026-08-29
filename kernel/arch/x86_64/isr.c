#include "../../include/isr.h"
#include "../../include/idt.h"
#include "../../include/keyboard.h"
#include "../../include/mouse.h"
#include "../../include/task/sched.h"
#include "../../include/apic.h"
#include "../../include/pmm.h"
#include "../../include/vmm.h"
#include "../../include/gui/compositor.h"
#include "../../include/fs/vfs.h"

typedef struct {
    vfs_node_t *node;
    uint32_t offset;
} fd_entry_t;

static fd_entry_t fd_table[8];

volatile uint64_t system_millis = 0;



extern void serial_write(const char *str);

// This array contains pointers to our assembly ISR stubs
extern uint64_t isr_stub_table[];

// Array of exception messages for debugging
const char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security Exception",
    "Reserved"
};



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

uint64_t isr_handler(registers_t *regs) {
    if (regs->int_no == 128) {
        // System call!
        thread_t *curr = sched_get_current_thread();
        
        if (regs->rax == 1) { // SYS_PRINT
            serial_write((const char *)regs->rbx);
            const char *s = (const char *)regs->rbx;
            while (*s) {
                compositor_handle_key(*s++);
            }
            regs->rax = 0;
        } else if (regs->rax == 2) { // SYS_EXIT
            // Simple exit: just yield forever or mark thread as dead (not implemented yet)
            serial_write("User thread exited.\r\n");
            // Sleep forever by looping and yielding
            while (1) {
                __asm__ volatile("sti; hlt"); // Wait for interrupt (timer will preempt)
            }

        } else if (regs->rax == 3) { // SYS_OPEN
            const char *path = (const char *)regs->rbx;
            vfs_node_t *node = vfs_open(path);
            if (!node) {
                regs->rax = -1;
            } else {
                int fd = -1;
                for (int i = 0; i < 8; i++) {
                    if (fd_table[i].node == NULL) {
                        fd = i;
                        fd_table[i].node = node;
                        fd_table[i].offset = 0;
                        break;
                    }
                }
                regs->rax = fd;
            }
        } else if (regs->rax == 4) { // SYS_READ
            int fd = regs->rbx;
            uint8_t *buf = (uint8_t *)regs->rcx;
            uint32_t size = regs->rdx;
            if (fd >= 0 && fd < 8 && fd_table[fd].node != NULL) {
                uint32_t bytes_read = vfs_read(fd_table[fd].node, fd_table[fd].offset, size, buf);
                fd_table[fd].offset += bytes_read;
                regs->rax = bytes_read;
            } else {
                regs->rax = -1;
            }
        } else if (regs->rax == 5) { // SYS_SEEK
            int fd = regs->rbx;
            int offset = regs->rcx;
            int whence = regs->rdx; // 0=SET, 1=CUR, 2=END
            if (fd >= 0 && fd < 8 && fd_table[fd].node != NULL) {
                if (whence == 0) fd_table[fd].offset = offset;
                else if (whence == 1) fd_table[fd].offset += offset;
                else if (whence == 2) fd_table[fd].offset = fd_table[fd].node->length + offset;
                regs->rax = fd_table[fd].offset;
            } else {
                regs->rax = -1;
            }
        } else if (regs->rax == 6) { // SYS_CLOSE
            int fd = regs->rbx;
            if (fd >= 0 && fd < 8) {
                fd_table[fd].node = NULL;
                regs->rax = 0;
            } else {
                regs->rax = -1;
            }
        } else if (regs->rax == 7) { // SYS_BLIT
            uint32_t *src = (uint32_t *)regs->rbx;
            compositor_blit_doom(src, 640, 400);
            regs->rax = 0;
        } else if (regs->rax == 8) { // SYS_GETKEY
            extern int keyboard_pop_event(int *pressed, unsigned char *key);
            int pressed;
            unsigned char key;
            if (keyboard_pop_event(&pressed, &key)) {
                regs->rax = (pressed << 8) | key;
            } else {
                regs->rax = 0;
            }
        } else if (regs->rax == 9) { // SYS_SBRK
            uint64_t size = regs->rbx;
            uint64_t pages = (size + 0xFFF) / 4096;
            
            uint64_t old_top = curr->user_heap_top;
            for (uint64_t i = 0; i < pages; i++) {
                uint64_t phys_addr = (uint64_t)pmm_alloc_page();
                if (!phys_addr) {
                    serial_write("SYS_SBRK: Out of physical memory!\r\n");
                    regs->rax = -1;
                    return (uint64_t)regs;
                }
                vmm_map_page(curr->pml4, old_top + (i * 4096), phys_addr, PAGE_PRESENT | PAGE_RW | PAGE_USER);
            }
            curr->user_heap_top += (pages * 4096);
            regs->rax = old_top;
        } else if (regs->rax == 10) { // SYS_GETTICKS
            regs->rax = system_millis;
        }
        return (uint64_t)regs;
    }
    
    if (regs->int_no == 32) { // Timer
        extern volatile uint64_t system_millis;
        system_millis += 10;
        apic_eoi(); // Acknowledge interrupt
        return schedule(regs);
    }
    
    if (regs->int_no == 33) {
        keyboard_handler();
        return (uint64_t)regs;
    }

    if (regs->int_no == 44) {
        mouse_handler();
        return (uint64_t)regs;
    }
    if (regs->int_no >= 32) {
        // Return for other interrupts for now
        return (uint64_t)regs;
    }

    serial_write("\r\n\r\n*** KERNEL PANIC ***\r\n");
    
    if (regs->int_no < 32) {
        serial_write("Exception: ");
        serial_write(exception_messages[regs->int_no]);
        serial_write("\r\n");
    } else {
        serial_write("Unknown Exception\r\n");
    }

    serial_write("Error Code: ");
    print_hex(regs->err_code);
    serial_write("\r\n");

    serial_write("RIP: ");
    print_hex(regs->rip);
    serial_write("\r\n");
    
    serial_write("RSP: ");
    print_hex(regs->rsp);
    serial_write("\r\n");

    serial_write("System Halted.\r\n");

    // Halt forever
    while (1) {
        __asm__ volatile("cli; hlt");
    }
}

#pragma once

#include <stdint.h>

// Struct to hold the state of the CPU registers when an interrupt occurs
typedef struct {
    // Pushed by our assembly ISR stubs (lowest address to highest)
    uint64_t rax, rbx, rcx, rdx, rbp, rsi, rdi;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    
    uint64_t int_no;
    uint64_t err_code;
    
    // Pushed by the CPU automatically
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) registers_t;

uint64_t isr_handler(registers_t *regs);
void isrs_install(void);

#pragma once

#include <stdint.h>
#include "../../include/isr.h" // For registers_t

typedef enum {
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_DEAD
} thread_state_t;

typedef struct thread {
    uint64_t id;
    uint64_t rsp; // Kernel stack pointer
    uint64_t stack_base; // Base of the kernel stack (for freeing)
    uint64_t kernel_stack_top; // We need this for the TSS rsp0
    void *pml4; // Page table for this thread
    uint64_t user_heap_top; // Current top of the user heap
    thread_state_t state;
    struct thread *next;
} thread_t;

void thread_init(void);
thread_t *thread_create(void (*entry_point)(void));
thread_t *thread_create_user(void (*entry_point)(void), void *pml4);
void thread_yield(void);
void thread_exit(void);

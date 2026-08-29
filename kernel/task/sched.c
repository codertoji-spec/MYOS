#include "../include/task/sched.h"
#include "../include/task/thread.h"
#include "../include/gdt.h"
#include "../include/vmm.h"
#include <stddef.h>

static thread_t *current_thread = NULL;
static thread_t *ready_queue = NULL;
static thread_t *ready_queue_tail = NULL;
static int scheduler_enabled = 0;

void sched_init(void) {
    scheduler_enabled = 1;
}

void sched_set_current_thread(thread_t *thread) {
    current_thread = thread;
    current_thread->state = THREAD_RUNNING;
}

void sched_add_thread(thread_t *thread) {
    thread->state = THREAD_READY;
    thread->next = NULL;

    if (!ready_queue) {
        ready_queue = thread;
        ready_queue_tail = thread;
    } else {
        ready_queue_tail->next = thread;
        ready_queue_tail = thread;
    }
}

thread_t *sched_get_current_thread(void) {
    return current_thread;
}

uint64_t schedule(registers_t *regs) {
    if (!scheduler_enabled || !ready_queue) {
        return (uint64_t)regs;
    }

    // Save current thread state if there is a running thread
    if (current_thread && current_thread->state == THREAD_RUNNING) {
        current_thread->rsp = (uint64_t)regs;
        current_thread->state = THREAD_READY;
        sched_add_thread(current_thread);
    }

    // Pick the next thread
    current_thread = ready_queue;
    ready_queue = ready_queue->next;
    if (!ready_queue) {
        ready_queue_tail = NULL;
    }

    current_thread->state = THREAD_RUNNING;
    current_thread->next = NULL;

    // Update TSS so next time we interrupt from user mode we land on this thread's kernel stack
    if (current_thread->kernel_stack_top) {
        tss_set_stack(current_thread->kernel_stack_top);
    }
    
    // Switch address space
    if (current_thread->pml4) {
        vmm_switch_pml4(current_thread->pml4);
    }

    return current_thread->rsp;
}

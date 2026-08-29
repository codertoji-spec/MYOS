#include "../include/task/thread.h"
#include "../include/task/sched.h"
#include "../include/pmm.h"
#include "../include/vmm.h"
#include <stddef.h>

#define STACK_SIZE 8192

static uint64_t next_thread_id = 1;

extern void thread_exit_stub(void); // Defined in assembly

static thread_t main_thread;

void thread_init(void) {
    sched_init();
    
    main_thread.id = 0;
    main_thread.kernel_stack_top = 0;
    main_thread.pml4 = vmm_get_kernel_pml4();
    sched_set_current_thread(&main_thread);
}

thread_t *thread_create(void (*entry_point)(void)) {
    // We don't have a kmalloc yet, so we'll just allocate a page for the thread structure and stack.
    // PMM returns 4KB pages. Let's use 1 page for struct (overkill) and 2 pages for stack.
    thread_t *thread = (thread_t *)pmm_alloc_page();
    if (!thread) return NULL;
    
    uint64_t stack_base = (uint64_t)pmm_alloc_pages(2);
    
    thread->id = next_thread_id++;
    thread->stack_base = stack_base;
    thread->kernel_stack_top = 0;
    thread->pml4 = vmm_get_kernel_pml4();
    
    uint64_t stack_top = stack_base + 8192;
    
    // Set up the initial stack frame for the thread
    // The stack must look like it was interrupted by a timer.
    // We push SS, RSP, RFLAGS, CS, RIP, ERR_CODE, INT_NO, and then all general purpose registers.
    
    uint64_t *stack = (uint64_t *)stack_top;
    
    // SS
    *(--stack) = 0x10; // Kernel data segment
    
    // RSP (Stack pointer before interrupt)
    *(--stack) = stack_top;
    
    // RFLAGS (Enable interrupts)
    *(--stack) = 0x202; // IF = 1, bit 1 is always 1
    
    // CS
    *(--stack) = 0x08; // Kernel code segment
    
    // RIP (Instruction pointer)
    *(--stack) = (uint64_t)entry_point;
    
    // ERR_CODE
    *(--stack) = 0;
    
    // INT_NO
    *(--stack) = 32;
    
    // General purpose registers (15 of them)
    // r15 to r8
    for (int i = 0; i < 8; i++) {
        *(--stack) = 0;
    }
    
    // rdi, rsi, rbp, rdx, rcx, rbx, rax
    for (int i = 0; i < 7; i++) {
        *(--stack) = 0;
    }
    
    // Now replace the return address on the stack for entry_point?
    // Actually, when entry_point returns, it will try to pop a return address, which isn't there, leading to a fault.
    // Instead of pushing it as a return address within entry_point, we can just make entry_point end with an infinite loop or call thread_exit().
    
    thread->rsp = (uint64_t)stack;
    thread->state = THREAD_READY;
    thread->next = NULL;
    
    sched_add_thread(thread);
    
    return thread;
}

thread_t *thread_create_user(void (*entry_point)(void), void *pml4) {
    thread_t *thread = (thread_t *)pmm_alloc_page();
    if (!thread) return NULL;
    
    // Allocate Kernel Stack
    uint64_t kstack_base = (uint64_t)pmm_alloc_pages(2);
    
    // Allocate User Stack (must be mapped in the user's PML4!)
    // Wait, the user stack needs to be accessible in the new PML4!
    // Let's allocate physical pages and map them into the user PML4 at a high address.
    uint64_t ustack_phys = (uint64_t)pmm_alloc_pages(2);
    uint64_t ustack_virt = 0x00007FFFF0000000; // arbitrary high address
    vmm_map_page((page_table_t*)pml4, ustack_virt, ustack_phys, PAGE_PRESENT | PAGE_RW | PAGE_USER);
    vmm_map_page((page_table_t*)pml4, ustack_virt + 4096, ustack_phys + 4096, PAGE_PRESENT | PAGE_RW | PAGE_USER);
    
    thread->id = next_thread_id++;
    thread->stack_base = kstack_base;
    thread->kernel_stack_top = kstack_base + 8192;
    thread->pml4 = pml4;
    thread->user_heap_top = 0x9000000000; // Start heap at a high address
    
    uint64_t stack_top = thread->kernel_stack_top;
    uint64_t user_stack_top = ustack_virt + 8192 - 8; // x86_64 ABI requires (RSP + 8) % 16 == 0 on entry

    
    uint64_t *stack = (uint64_t *)stack_top;
    
    // Push the interrupt frame for User Mode
    // SS (User Data Selector = 0x20 | RPL 3 = 0x23)
    *(--stack) = 0x23;
    
    // RSP (User Stack)
    *(--stack) = user_stack_top;
    
    // RFLAGS (Enable interrupts)
    *(--stack) = 0x202;
    
    // CS (User Code Selector = 0x18 | RPL 3 = 0x1B)
    *(--stack) = 0x1B;
    
    // RIP (Instruction pointer)
    *(--stack) = (uint64_t)entry_point;
    
    // ERR_CODE
    *(--stack) = 0;
    
    // INT_NO
    *(--stack) = 32;
    
    // General purpose registers
    for (int i = 0; i < 15; i++) {
        *(--stack) = 0;
    }
    
    thread->rsp = (uint64_t)stack;
    thread->state = THREAD_READY;
    thread->next = NULL;
    
    sched_add_thread(thread);
    
    return thread;
}

void thread_yield(void) {
    // Generate a software interrupt that calls the scheduler
    __asm__ volatile("int $32");
}

void thread_exit(void) {
    // For now, just hang
    while(1) {
        __asm__ volatile("cli; hlt");
    }
}

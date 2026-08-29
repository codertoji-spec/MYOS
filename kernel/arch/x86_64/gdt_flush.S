.global gdt_flush

gdt_flush:
    # RDI contains the address of gdtr
    lgdt (%rdi)

    # Load data segments with Kernel Data Selector (0x10)
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    # Far return to load CS with Kernel Code Selector (0x08)
    popq %rdi
    movq $0x08, %rax
    pushq %rax
    pushq %rdi
    lretq


.global isr_stub_table
.text

.global isr0
.align 8
isr0:
    pushq $0
    pushq $0
    jmp isr_common_stub

.global isr1
.align 8
isr1:
    pushq $0
    pushq $1
    jmp isr_common_stub

.global isr2
.align 8
isr2:
    pushq $0
    pushq $2
    jmp isr_common_stub

.global isr3
.align 8
isr3:
    pushq $0
    pushq $3
    jmp isr_common_stub

.global isr4
.align 8
isr4:
    pushq $0
    pushq $4
    jmp isr_common_stub

.global isr5
.align 8
isr5:
    pushq $0
    pushq $5
    jmp isr_common_stub

.global isr6
.align 8
isr6:
    pushq $0
    pushq $6
    jmp isr_common_stub

.global isr7
.align 8
isr7:
    pushq $0
    pushq $7
    jmp isr_common_stub

.global isr8
.align 8
isr8:
    pushq $8
    jmp isr_common_stub

.global isr9
.align 8
isr9:
    pushq $0
    pushq $9
    jmp isr_common_stub

.global isr10
.align 8
isr10:
    pushq $10
    jmp isr_common_stub

.global isr11
.align 8
isr11:
    pushq $11
    jmp isr_common_stub

.global isr12
.align 8
isr12:
    pushq $12
    jmp isr_common_stub

.global isr13
.align 8
isr13:
    pushq $13
    jmp isr_common_stub

.global isr14
.align 8
isr14:
    pushq $14
    jmp isr_common_stub

.global isr15
.align 8
isr15:
    pushq $0
    pushq $15
    jmp isr_common_stub

.global isr16
.align 8
isr16:
    pushq $0
    pushq $16
    jmp isr_common_stub

.global isr17
.align 8
isr17:
    pushq $17
    jmp isr_common_stub

.global isr18
.align 8
isr18:
    pushq $0
    pushq $18
    jmp isr_common_stub

.global isr19
.align 8
isr19:
    pushq $0
    pushq $19
    jmp isr_common_stub

.global isr20
.align 8
isr20:
    pushq $0
    pushq $20
    jmp isr_common_stub

.global isr21
.align 8
isr21:
    pushq $21
    jmp isr_common_stub

.global isr22
.align 8
isr22:
    pushq $0
    pushq $22
    jmp isr_common_stub

.global isr23
.align 8
isr23:
    pushq $0
    pushq $23
    jmp isr_common_stub

.global isr24
.align 8
isr24:
    pushq $0
    pushq $24
    jmp isr_common_stub

.global isr25
.align 8
isr25:
    pushq $0
    pushq $25
    jmp isr_common_stub

.global isr26
.align 8
isr26:
    pushq $0
    pushq $26
    jmp isr_common_stub

.global isr27
.align 8
isr27:
    pushq $0
    pushq $27
    jmp isr_common_stub

.global isr28
.align 8
isr28:
    pushq $0
    pushq $28
    jmp isr_common_stub

.global isr29
.align 8
isr29:
    pushq $29
    jmp isr_common_stub

.global isr30
.align 8
isr30:
    pushq $30
    jmp isr_common_stub

.global isr31
.align 8
isr31:
    pushq $0
    pushq $31
    jmp isr_common_stub

.global isr32
.align 8
isr32:
    pushq $0
    pushq $32
    jmp isr_common_stub

.global isr33
.align 8
isr33:
    pushq $0
    pushq $33
    jmp isr_common_stub

.global isr34
.align 8
isr34:
    pushq $0
    pushq $34
    jmp isr_common_stub

.global isr35
.align 8
isr35:
    pushq $0
    pushq $35
    jmp isr_common_stub

.global isr36
.align 8
isr36:
    pushq $0
    pushq $36
    jmp isr_common_stub

.global isr37
.align 8
isr37:
    pushq $0
    pushq $37
    jmp isr_common_stub

.global isr38
.align 8
isr38:
    pushq $0
    pushq $38
    jmp isr_common_stub

.global isr39
.align 8
isr39:
    pushq $0
    pushq $39
    jmp isr_common_stub

.global isr40
.align 8
isr40:
    pushq $0
    pushq $40
    jmp isr_common_stub

.global isr41
.align 8
isr41:
    pushq $0
    pushq $41
    jmp isr_common_stub

.global isr42
.align 8
isr42:
    pushq $0
    pushq $42
    jmp isr_common_stub

.global isr43
.align 8
isr43:
    pushq $0
    pushq $43
    jmp isr_common_stub

.global isr44
.align 8
isr44:
    pushq $0
    pushq $44
    jmp isr_common_stub

.global isr45
.align 8
isr45:
    pushq $0
    pushq $45
    jmp isr_common_stub

.global isr46
.align 8
isr46:
    pushq $0
    pushq $46
    jmp isr_common_stub

.global isr47
.align 8
isr47:
    pushq $0
    pushq $47
    jmp isr_common_stub

.global isr128
.align 8
isr128:
    pushq $0
    pushq $128
    jmp isr_common_stub

isr_common_stub:
    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12
    pushq %r11
    pushq %r10
    pushq %r9
    pushq %r8
    pushq %rdi
    pushq %rsi
    pushq %rbp
    pushq %rdx
    pushq %rcx
    pushq %rbx
    pushq %rax
    mov %rsp, %rdi
    cld
    call isr_handler
    mov %rax, %rsp
    popq %rax
    popq %rbx
    popq %rcx
    popq %rdx
    popq %rbp
    popq %rsi
    popq %rdi
    popq %r8
    popq %r9
    popq %r10
    popq %r11
    popq %r12
    popq %r13
    popq %r14
    popq %r15
    add $16, %rsp
    iretq

.data
.align 8
isr_stub_table:
    .quad isr0
    .quad isr1
    .quad isr2
    .quad isr3
    .quad isr4
    .quad isr5
    .quad isr6
    .quad isr7
    .quad isr8
    .quad isr9
    .quad isr10
    .quad isr11
    .quad isr12
    .quad isr13
    .quad isr14
    .quad isr15
    .quad isr16
    .quad isr17
    .quad isr18
    .quad isr19
    .quad isr20
    .quad isr21
    .quad isr22
    .quad isr23
    .quad isr24
    .quad isr25
    .quad isr26
    .quad isr27
    .quad isr28
    .quad isr29
    .quad isr30
    .quad isr31
    .quad isr32
    .quad isr33
    .quad isr34
    .quad isr35
    .quad isr36
    .quad isr37
    .quad isr38
    .quad isr39
    .quad isr40
    .quad isr41
    .quad isr42
    .quad isr43
    .quad isr44
    .quad isr45
    .quad isr46
    .quad isr47

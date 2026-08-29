#pragma once
#include <stdint.h>

#define SYS_PRINT  1
#define SYS_EXIT   2
#define SYS_OPEN   3
#define SYS_READ   4
#define SYS_SEEK   5
#define SYS_CLOSE  6
#define SYS_BLIT   7
#define SYS_GETKEY 8
#define SYS_SBRK   9
#define SYS_GETTICKS 10

static inline uint64_t syscall(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    uint64_t ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(sys_num), "b"(arg1), "c"(arg2), "d"(arg3)
        : "memory"
    );
    return ret;
}

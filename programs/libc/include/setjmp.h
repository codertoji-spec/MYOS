
#pragma once
typedef unsigned long long jmp_buf[8]; // rbx, rbp, r12, r13, r14, r15, rsp, rip
int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);

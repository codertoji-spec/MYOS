#pragma once

#include "thread.h"
#include "../../include/isr.h"

void sched_init(void);
void sched_set_current_thread(thread_t *thread);
void sched_add_thread(thread_t *thread);
uint64_t schedule(registers_t *regs);
thread_t *sched_get_current_thread(void);

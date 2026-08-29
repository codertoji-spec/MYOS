
#pragma once
typedef void (*sighandler_t)(int);
#define SIGINT 2
static inline sighandler_t signal(int signum, sighandler_t handler) { return handler; }

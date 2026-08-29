#pragma once
#include <stddef.h>
#include <stdint.h>

typedef long long ssize_t;
typedef long long off_t;

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

int usleep(unsigned int usec);
unsigned int sleep(unsigned int seconds);
int access(const char *pathname, int mode);
#define F_OK 0
#define R_OK 4
#define W_OK 2
#define X_OK 1

#include "include/stdlib.h"
#include "include/syscall.h"
#include "include/ctype.h"

// Very simple bump allocator
void *malloc(size_t size) {
    if (size == 0) return NULL;
    
    void *ptr = (void *)syscall(SYS_SBRK, size, 0, 0);
    if ((int64_t)ptr == -1) {
        return NULL;
    }
    return ptr;
}

void free(void *ptr) {
    (void)ptr;
}

void exit(int status) {
    syscall(SYS_EXIT, status, 0, 0);
    while (1);
}

void *calloc(size_t nmemb, size_t size) {
    void *ptr = malloc(nmemb * size);
    if (ptr) {
        char *p = ptr;
        for (size_t i = 0; i < nmemb * size; i++) p[i] = 0;
    }
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) { free(ptr); return NULL; }
    void *new_ptr = malloc(size);
    if (new_ptr) {
        char *d = new_ptr, *s = ptr;
        for (size_t i = 0; i < size; i++) d[i] = s[i];
        free(ptr);
    }
    return new_ptr;
}

int abs(int j) { return j < 0 ? -j : j; }

static unsigned long int next = 1;
int rand(void) {
    next = next * 1103515245 + 12345;
    return (unsigned int)(next / 65536) % 32768;
}

void srand(unsigned int seed) { next = seed; }
char *getenv(const char *name) { (void)name; return NULL; }
void abort(void) { while(1); }

double strtod(const char *nptr, char **endptr) {
    if (endptr) *endptr = (char *)nptr;
    return 0.0;
}

double atof(const char *nptr) { return strtod(nptr, NULL); }

long strtol(const char *nptr, char **endptr, int base) {
    (void)base;
    long result = 0;
    while (isspace(*nptr)) nptr++;
    int sign = 1;
    if (*nptr == '-') { sign = -1; nptr++; }
    else if (*nptr == '+') nptr++;
    while (isdigit(*nptr)) {
        result = result * 10 + (*nptr - '0');
        nptr++;
    }
    if (endptr) *endptr = (char *)nptr;
    return result * sign;
}

unsigned long strtoul(const char *nptr, char **endptr, int base) {
    return (unsigned long)strtol(nptr, endptr, base);
}

int atoi(const char *nptr) { return (int)strtol(nptr, NULL, 10); }

void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *)) {
    (void)base; (void)nmemb; (void)size; (void)compar;
}

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *)) {
    (void)key; (void)base; (void)nmemb; (void)size; (void)compar;
    return NULL;
}

int system(const char *command) {
    (void)command;
    return -1;
}

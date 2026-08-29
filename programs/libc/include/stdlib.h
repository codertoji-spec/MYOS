#pragma once
#include <stddef.h>

void *malloc(size_t size);
void free(void *ptr);
void exit(int status);

void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
int abs(int j);
int rand(void);
void srand(unsigned int seed);
char *getenv(const char *name);
void abort(void);
double strtod(const char *nptr, char **endptr);
double atof(const char *nptr);
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
int atoi(const char *nptr);
void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));
int system(const char *command);

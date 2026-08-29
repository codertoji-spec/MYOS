
#pragma once
#include <stddef.h>
#include "stdarg.h"

#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct {
    int fd;
    int eof;
    int err;
    int ungetc_buf;
    int has_ungetc;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

FILE *fopen(const char *pathname, const char *mode);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int fclose(FILE *stream);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);

int printf(const char *format, ...);
int sprintf(char *str, const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int vsprintf(char *str, const char *format, va_list ap);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
int vfprintf(FILE *stream, const char *format, va_list ap);

char *fgets(char *s, int size, FILE *stream);
int fputs(const char *s, FILE *stream);
int fputc(int c, FILE *stream);
int fgetc(FILE *stream);
int ungetc(int c, FILE *stream);
int puts(const char *s);
int putchar(int c);

int remove(const char *pathname);
int rename(const char *oldpath, const char *newpath);
int fflush(FILE *stream);
int sscanf(const char *str, const char *format, ...);
int vsscanf(const char *str, const char *format, va_list ap);

void perror(const char *s);
char *strerror(int errnum);


#include "include/stdio.h"
#include "include/string.h"
#include "include/stdint.h"

static FILE _stdin = {0, 0, 0, 0, 0};
static FILE _stdout = {1, 0, 0, 0, 0};
static FILE _stderr = {2, 0, 0, 0, 0};
FILE *stdin = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;

int errno = 0;

static int syscall_open(const char *path) {
    int fd;
    __asm__ volatile("int $0x80" : "=a"(fd) : "a"(3), "b"(path) : "memory");
    return fd;
}

static int syscall_read(int fd, void *buf, size_t size) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(4), "b"(fd), "c"(buf), "d"(size) : "memory");
    return ret;
}

static int syscall_seek(int fd, int offset, int whence) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(5), "b"(fd), "c"(offset), "d"(whence) : "memory");
    return ret;
}

static int syscall_close(int fd) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(6), "b"(fd) : "memory");
    return ret;
}

FILE *fopen(const char *pathname, const char *mode) {
    (void)mode;
    int fd = syscall_open(pathname);
    if (fd < 0) return NULL;
    
    // allocate in heap, need malloc from stdlib.h (not included here, but we can prototype)
    extern void *malloc(size_t);
    FILE *f = malloc(sizeof(FILE));
    if (!f) { syscall_close(fd); return NULL; }
    f->fd = fd;
    f->eof = 0;
    f->err = 0;
    f->has_ungetc = 0;
    return f;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    if (size == 0 || nmemb == 0) return 0;
    
    size_t total = size * nmemb;
    char *cptr = ptr;
    size_t read_bytes = 0;
    
    if (stream->has_ungetc) {
        *cptr++ = stream->ungetc_buf;
        stream->has_ungetc = 0;
        read_bytes++;
    }
    
    int ret = syscall_read(stream->fd, cptr, total - read_bytes);
    if (ret < 0) {
        stream->err = 1;
        return read_bytes / size;
    }
    if (ret == 0) {
        stream->eof = 1;
    }
    read_bytes += ret;
    return read_bytes / size;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    if (stream == stdout || stream == stderr) {
        const char *cptr = ptr;
        size_t total = size * nmemb;
        char buf[256];
        size_t written = 0;
        while (written < total) {
            size_t chunk = total - written;
            if (chunk >= sizeof(buf)) chunk = sizeof(buf) - 1;
            for (size_t i = 0; i < chunk; i++) buf[i] = cptr[written + i];
            buf[chunk] = '\0';
            __asm__ volatile("int $0x80" : : "a"(1), "b"(buf) : "memory");
            written += chunk;
        }
        return nmemb;
    }
    return 0;
}

int fclose(FILE *stream) {
    if (stream == stdin || stream == stdout || stream == stderr) return 0;
    int ret = syscall_close(stream->fd);
    extern void free(void *);
    free(stream);
    return ret;
}

int fseek(FILE *stream, long offset, int whence) {
    int ret = syscall_seek(stream->fd, offset, whence);
    if (ret < 0) return -1;
    stream->eof = 0;
    return 0;
}

long ftell(FILE *stream) {
    return syscall_seek(stream->fd, 0, SEEK_CUR);
}

int feof(FILE *stream) { return stream->eof; }
int ferror(FILE *stream) { return stream->err; }

char *fgets(char *s, int size, FILE *stream) {
    if (size <= 1) return NULL;
    char *c = s;
    while (size-- > 1) {
        if (fread(c, 1, 1, stream) != 1) {
            if (c == s) return NULL;
            break;
        }
        if (*c == '\n') { c++; break; }
        c++;
    }
    *c = '\0';
    return s;
}

int fputs(const char *s, FILE *stream) {
    size_t len = strlen(s);
    return fwrite(s, 1, len, stream) == len ? 1 : EOF;
}

int fputc(int c, FILE *stream) {
    char ch = c;
    return fwrite(&ch, 1, 1, stream) == 1 ? c : EOF;
}

int fgetc(FILE *stream) {
    unsigned char ch;
    if (fread(&ch, 1, 1, stream) != 1) return EOF;
    return ch;
}

int ungetc(int c, FILE *stream) {
    if (c == EOF) return EOF;
    stream->ungetc_buf = c;
    stream->has_ungetc = 1;
    stream->eof = 0;
    return c;
}

int puts(const char *s) {
    fputs(s, stdout);
    return fputc('\n', stdout);
}

int putchar(int c) {
    return fputc(c, stdout);
}

void perror(const char *s) {
    if (s && *s) {
        fputs(s, stderr);
        fputs(": ", stderr);
    }
    fputs("Error\n", stderr);
}

char *strerror(int errnum) {
    return "Error";
}

// Helper for integer to string
static int format_int(char *buf, size_t max, long long val, int base, int is_signed, int width, int zero_pad, int precision) {
    if (max <= 1) return 0;
    char tmp[64];
    int i = 0;
    unsigned long long uval;
    int neg = 0;
    if (is_signed && val < 0) {
        neg = 1;
        uval = (unsigned long long)(-val);
    } else {
        uval = (unsigned long long)val;
    }
    if (uval == 0) {
        tmp[i++] = '0';
    } else {
        while (uval > 0) {
            int rem = uval % base;
            tmp[i++] = (rem < 10) ? ('0' + rem) : ('a' + rem - 10);
            uval /= base;
        }
    }
    // Apply precision (minimum digits)
    while (i < precision && i < 60) {
        tmp[i++] = '0';
    }
    if (neg) tmp[i++] = '-';
    // Apply width with zero-padding
    if (zero_pad) {
        while (i < width && i < 60) {
            tmp[i++] = '0';
        }
    }
    int written = 0;
    while (i > 0 && written < (int)max - 1) {
        buf[written++] = tmp[--i];
    }
    return written;
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    if (size == 0) return 0;
    size_t out_len = 0;
    while (*format && out_len < size - 1) {
        if (*format != '%') {
            str[out_len++] = *format++;
            continue;
        }
        format++;
        if (*format == '%') {
            str[out_len++] = '%';
            format++;
            continue;
        }
        
        int zero_pad = 0;
        int width = 0;
        int precision = -1;
        
        if (*format == '0') {
            zero_pad = 1;
            format++;
        }
        while (*format >= '0' && *format <= '9') {
            width = width * 10 + (*format - '0');
            format++;
        }
        if (*format == '.') {
            format++;
            precision = 0;
            while (*format >= '0' && *format <= '9') {
                precision = precision * 10 + (*format - '0');
                format++;
            }
        }
        while (*format == 'l' || *format == 'z' || *format == 'h' || *format == '+' || *format == ' ' || *format == '#') {
            format++;
        }
        
        if (*format == 's') {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            while (*s && out_len < size - 1) {
                str[out_len++] = *s++;
            }
            format++;
        } else if (*format == 'd' || *format == 'i') {
            long long val = va_arg(ap, int);
            out_len += format_int(str + out_len, size - out_len, val, 10, 1, width, zero_pad, precision);
            format++;
        } else if (*format == 'u') {
            unsigned long long val = va_arg(ap, unsigned int);
            out_len += format_int(str + out_len, size - out_len, val, 10, 0, width, zero_pad, precision);
            format++;
        } else if (*format == 'x' || *format == 'X') {
            unsigned long long val = va_arg(ap, unsigned int);
            out_len += format_int(str + out_len, size - out_len, val, 16, 0, width, zero_pad, precision);
            format++;
        } else if (*format == 'p') {
            void *ptr = va_arg(ap, void *);
            if (out_len + 2 < size) {
                str[out_len++] = '0';
                str[out_len++] = 'x';
            }
            out_len += format_int(str + out_len, size - out_len, (uintptr_t)ptr, 16, 0, 0, 0, -1);
            format++;
        } else if (*format == 'c') {
            int c = va_arg(ap, int);
            str[out_len++] = (char)c;
            format++;
        } else {
            str[out_len++] = *format ? *format++ : '%';
        }
    }
    str[out_len] = '\0';
    return (int)out_len;
}

int snprintf(char *str, size_t size, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(str, size, format, ap);
    va_end(ap);
    return ret;
}

int vsprintf(char *str, const char *format, va_list ap) {
    return vsnprintf(str, 1024, format, ap); // unsafe dummy
}

int sprintf(char *str, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vsprintf(str, format, ap);
    va_end(ap);
    return ret;
}

int vfprintf(FILE *stream, const char *format, va_list ap) {
    char buf[256];
    int ret = vsnprintf(buf, sizeof(buf), format, ap);
    fwrite(buf, 1, ret, stream);
    return ret;
}

int fprintf(FILE *stream, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vfprintf(stream, format, ap);
    va_end(ap);
    return ret;
}

int printf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vfprintf(stdout, format, ap);
    va_end(ap);
    return ret;
}

int remove(const char *pathname) {
    (void)pathname;
    return 0;
}

int rename(const char *oldpath, const char *newpath) {
    (void)oldpath;
    (void)newpath;
    return 0;
}

int fflush(FILE *stream) {
    (void)stream;
    return 0;
}

int vsscanf(const char *str, const char *format, va_list ap) {
    int count = 0;
    while (*str && *format) {
        if (*format == '%') {
            format++;
            if (*format == 'd') {
                int *val = va_arg(ap, int *);
                int sign = 1;
                while (*str == ' ' || *str == '\t') str++;
                if (*str == '-') { sign = -1; str++; }
                else if (*str == '+') str++;
                int res = 0;
                while (*str >= '0' && *str <= '9') {
                    res = res * 10 + (*str - '0');
                    str++;
                }
                *val = res * sign;
                count++;
                format++;
            } else if (*format == 's') {
                char *buf = va_arg(ap, char *);
                while (*str == ' ' || *str == '\t') str++;
                while (*str && *str != ' ' && *str != '\t' && *str != '\n') {
                    *buf++ = *str++;
                }
                *buf = '\0';
                count++;
                format++;
            } else {
                format++;
            }
        } else if (*format == *str) {
            format++;
            str++;
        } else {
            break;
        }
    }
    return count;
}

int sscanf(const char *str, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vsscanf(str, format, ap);
    va_end(ap);
    return ret;
}

int mkdir(const char *pathname, unsigned int mode) {
    (void)pathname;
    (void)mode;
    return 0;
}

int access(const char *pathname, int mode) {
    (void)pathname;
    (void)mode;
    return 0;
}



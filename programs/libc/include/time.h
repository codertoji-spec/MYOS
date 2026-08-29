#pragma once
#include <stddef.h>

typedef unsigned long long clock_t;
typedef long long time_t;

#define CLOCKS_PER_SEC 1000000UL

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

static inline clock_t clock(void) { return 0; }
static inline time_t time(time_t *tloc) { if (tloc) *tloc = 0; return 0; }
static inline struct tm *localtime(const time_t *timep) {
    static struct tm t = {0};
    (void)timep;
    return &t;
}
static inline char *ctime(const time_t *timep) {
    (void)timep;
    return "Thu Jan  1 00:00:00 1970\n";
}

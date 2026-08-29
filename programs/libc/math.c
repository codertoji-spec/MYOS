
#include "include/math.h"
#include "include/stdint.h"

double fabs(double x) { return x < 0 ? -x : x; }
double floor(double x) {
    long long i = (long long)x;
    return (double)(i - (x < 0 && x != i));
}
double ceil(double x) {
    long long i = (long long)x;
    return (double)(i + (x > 0 && x != i));
}
double sqrt(double x) {
    if (x < 0) return 0.0/0.0;
    double ans = x;
    for (int i=0; i<10; i++) ans = (ans + x/ans)/2.0;
    return ans;
}
double fmod(double x, double y) {
    return x - (int)(x / y) * y;
}
double sin(double x) {
    double res = 0, term = x;
    for (int i=1; i<20; i++) {
        res += term;
        term = -term * x * x / ((2*i) * (2*i+1));
    }
    return res;
}
double cos(double x) {
    double res = 0, term = 1;
    for (int i=1; i<20; i++) {
        res += term;
        term = -term * x * x / ((2*i-1) * (2*i));
    }
    return res;
}
double tan(double x) { return sin(x)/cos(x); }
double atan2(double y, double x) { return 0.0; } // dummy
double pow(double x, double y) { return 0.0; } // dummy
double log(double x) { return 0.0; } // dummy
double exp(double x) { return 0.0; } // dummy
double ldexp(double x, int exp) { return x; } // dummy
double frexp(double x, int *exp) { *exp = 0; return x; } // dummy

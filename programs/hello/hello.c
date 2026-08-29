#include "../libc/include/stdio.h"
#include "../libc/include/stdlib.h"
#include "../libc/include/string.h"

void _start(void) {
    printf("Hello %s! The answer is %d\n", "World", 42);
    
    char *buf = malloc(32);
    if (buf) {
        strcpy(buf, "Malloc works!");
        printf("%s\n", buf);
    } else {
        printf("Malloc failed.\n");
    }
    
    exit(0);
}

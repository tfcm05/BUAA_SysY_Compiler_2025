#include <stdio.h>

int getint() {
    int x;
    scanf("%d", &x);
    return x;
}

void putint(int x) {
    printf("%d", x);
}

void putstr(const char* s) {
    printf("%s", s);
}

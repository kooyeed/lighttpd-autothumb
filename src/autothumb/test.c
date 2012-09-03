#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
//#define _GNU_SOURCE
#include <string.h>

#define test(a, b) \
    a##_test(b)

void a1_test(char *p) {
    printf("%s\n", p);
}

int main() {
    test("a1", "hi,world");
    return 0;
}

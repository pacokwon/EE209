#include "str.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void test1(void) {
    char *c1, *c2;
    assert(strtol("1234", &c1, 10) == StrToLong("1234", &c2, 10));
    assert(c1 == c2);
}

void test2(void) {
    char *c1, *c2;
    assert(strtol("12b34", &c1, 10) == StrToLong("12b34", &c2, 10));
    assert(c1 == c2);
}

void test3(void) {
    char *c1, *c2;
    assert(strtol("", &c1, 10) == StrToLong("", &c2, 10));
    assert(c1 == c2);
}

int main(void) {
    test1();
    test2();
    test3();

    printf("(strtol) All Tests Passed!\n");

    return 0;
}

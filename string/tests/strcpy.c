#include "str.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test1(void) {
    char *original = "some string";

    char buffer1[12], buffer2[12];
    strcpy (buffer1, original);
    strcpy (buffer2, original);
    assert(strcmp (buffer1, buffer2) == 0);
}

void test2(void) {
    char *original = "some\0string";

    char buffer1[12], buffer2[12];
    strcpy (buffer1, original);
    strcpy (buffer2, original);
    assert(strcmp (buffer1, buffer2) == 0);
}

int main(void) {
    test1();
    test2();

    printf("(strcpy) All Tests Passed!\n");

    return 0;
}

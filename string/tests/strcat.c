#include "str.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test1(void) {
    char original1[20], original2[20];
    strcpy(original1, "123456789");
    strcpy(original2, "123456789");

    strcat(original1, "123456789");
    StrConcat(original2, "123456789");

    assert(strcmp(original1, original2) == 0);
}

void test2(void) {
    char original1[20], original2[20];
    strcpy(original1, "1234\06789");
    strcpy(original2, "1234\06789");

    strcat(original1, "56789");
    StrConcat(original2, "56789");

    assert(strcmp(original1, original2) == 0);
}

int main(void) {
    test1();
    test2();

    printf("(strcat) All Tests Passed!\n");

    return 0;
}

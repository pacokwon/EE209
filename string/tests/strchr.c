#include "str.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test1(void) {
    char *str = "foobar";

    assert(strchr(str, 'f') == StrFindChr(str, 'f'));
    assert(strchr(str, 'o') == StrFindChr(str, 'o'));
    assert(strchr(str, 'b') == StrFindChr(str, 'b'));
    assert(strchr(str, 'r') == StrFindChr(str, 'r'));
    assert(strchr(str, '\0') == StrFindChr(str, '\0'));
    assert(strchr(str, 'z') == StrFindChr(str, 'z'));
}

void test2(void) {
    char *str = "bet\0ween";

    assert(strchr(str, 'b') == StrFindChr(str, 'b'));
    assert(strchr(str, '\0') == StrFindChr(str, '\0'));
    assert(strchr(str, 'e') == StrFindChr(str, 'e'));
    assert(strchr(str, 'z') == StrFindChr(str, 'z'));
}

int main(void) {
    test1();
    test2();

    printf("(strchr) All Tests Passed!\n");

    return 0;
}

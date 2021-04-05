#include "str.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test1(void) {
    char *str = "foobar";

    assert(strstr(str, "foo") == StrFindStr(str, "foo"));
    assert(strstr(str, "oobar") == StrFindStr(str, "oobar"));
    assert(strstr(str, "barr") == StrFindStr(str, "barr"));
    assert(strstr(str, "baz") == StrFindStr(str, "baz"));
}

void test2(void) {
    char *str = "bet\0ween";

    assert(strstr(str, "et") == StrFindStr(str, "et"));
    assert(strstr(str, "et\0w") == StrFindStr(str, "et\0w"));
    assert(strstr(str, "een") == StrFindStr(str, "een"));
}

int main(void) {
    test1();
    test2();

    printf("(strstr) All Tests Passed!\n");

    return 0;
}

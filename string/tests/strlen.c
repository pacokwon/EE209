#include "str.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    assert(strlen("foo") == StrGetLength("foo"));
    assert(strlen("bet\0ween") == StrGetLength("bet\0ween"));
    assert(strlen("bet\0ween") == StrGetLength("bet\0ween"));

    printf("(strlen) All Tests Passed!\n");

    return 0;
}

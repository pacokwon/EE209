#include "str.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    assert(strcmp("a", "b") == StrCompare("a", "b"));
    assert(strcmp("aaa", "aab") == StrCompare("aaa", "aab"));
    assert(strcmp("byone", "byon") == StrCompare("byone", "byon"));
    assert(strcmp("equal", "equal") == StrCompare("equal", "equal"));

    printf("(strcmp) All Tests Passed!\n");

    return 0;
}

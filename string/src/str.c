#include <assert.h> /* to use assert() */
#include <ctype.h>
#include <stdio.h>
#include "str.h"

/* Your task is:
   1. Rewrite the body of "Part 1" functions - remove the current
   body that simply calls the corresponding C standard library
   function.
   2. Write appropriate comment per each function
   */

/* Part 1 */
/*------------------------------------------------------------------------*/
size_t StrGetLength(const char *pcSrc) {
    assert(pcSrc); /* NULL address, 0, and FALSE are identical. */

    const char *pcEnd;
    pcEnd = pcSrc;

    while (*pcEnd) /* null character and FALSE are identical. */
        pcEnd++;

    return (size_t)(pcEnd - pcSrc);
}

/*------------------------------------------------------------------------*/
char *StrCopy(char *pcDest, const char *pcSrc) {
    assert(pcDest);
    assert(pcSrc);

    const char *srcCursor;
    char *destCursor;

    srcCursor = pcSrc;
    destCursor = pcDest;

    while (*srcCursor) {
        *destCursor = *srcCursor;

        srcCursor++;
        destCursor++;
    }

    *destCursor = '\0';

    return pcDest;
}

/*------------------------------------------------------------------------*/
int StrCompare(const char *s1, const char *s2) {
    assert(s1);
    assert(s2);

    const char *s1Cursor, *s2Cursor;

    s1Cursor = s1;
    s2Cursor = s2;

    while (*s1Cursor != '\0' && *s2Cursor != '\0') {
        if (*s1Cursor > *s2Cursor)
            return 1;
        else if (*s1Cursor < *s2Cursor)
            return -1;

        s1Cursor++;
        s2Cursor++;
    }

    if (*s1Cursor == *s2Cursor)
        return 0;
    else if (*s1Cursor > *s2Cursor)
        return 1;
    else
        return -1;
}
/*------------------------------------------------------------------------*/
char *StrFindChr(const char *pcHaystack, int c) {
    assert(pcHaystack);

    char *cursor = (char *) pcHaystack;
    while (*cursor) {
        if (*cursor == c)
            return cursor;
        cursor++;
    }

    return *cursor == c ? cursor : NULL;
}
/*------------------------------------------------------------------------*/
char *StrFindStr(const char *pcHaystack, const char *pcNeedle) {
    assert(pcHaystack);
    assert(pcNeedle);

    const char *cursor, *haystackCursor, *needleCursor;
    int isEqual;

    cursor = pcHaystack;
    while (*cursor) {
        // equal by default
        isEqual = 1;

        // start string comparison from here
        haystackCursor = cursor;
        needleCursor = pcNeedle;
        while (*needleCursor) {
            if (*haystackCursor != *needleCursor) {
                isEqual = 0;
                break;
            }

            needleCursor++;
            haystackCursor++;
        }

        // early return if equal
        if (isEqual)
            return (char *) cursor;

        // if not equal, move to next character
        cursor++;
    }

    return NULL;
}

/*------------------------------------------------------------------------*/
char *StrConcat(char *pcDest, const char *pcSrc) {
    assert(pcDest);
    assert(pcSrc);

    char *cursor;
    const char *srcCursor;

    cursor = pcDest;

    // move to NULL character
    while (*cursor)
        cursor++;

    // from here, cursor points to NULL
    srcCursor = pcSrc;
    while (*srcCursor) {
        *cursor = *srcCursor;

        cursor++;
        srcCursor++;
    }
    *cursor = '\0';

    return pcDest;
}

/*------------------------------------------------------------------------*/
long int StrToLong(const char *nptr, char **endptr, int base) {
    assert(nptr);

    const char *cursor;
    unsigned long sum, threshold, digitLimit;
    int isNegative = 0, isOutOfRange = 0, digit;

    /* handle only when base is 10 */
    if (base != 10) return 0;

    sum = 0L;
    cursor = nptr;

    while (isspace(*cursor))
        cursor++;

    if (*cursor == '+') {
        isNegative = 0;
        cursor++;
    } else if (*cursor == '-') {
        isNegative = 1;
        cursor++;
    }

    if (isNegative) {
        threshold = (-(unsigned long) LONG_MIN) / 10L;
        digitLimit = (-(unsigned long) LONG_MIN) % 10L;
    } else {
        threshold = LONG_MAX / 10L;
        digitLimit = LONG_MAX % 10L;
    }

    while (*cursor) {
        if (!('0' <= *cursor && *cursor <= '9'))
            break;

        digit = *cursor - '0';

        if (isOutOfRange || sum > threshold || sum == threshold && digit > digitLimit) {
            isOutOfRange = 1;
            cursor++;
            continue;
        }

        sum = base * sum + digit;
        cursor++;
    }

    // only save when enptr is provided
    if (endptr)
        *endptr = (char *) cursor;
    if (isOutOfRange)
        sum = isNegative ? LONG_MIN : LONG_MAX;
    if (isNegative)
        sum = -sum;

    return sum;
}

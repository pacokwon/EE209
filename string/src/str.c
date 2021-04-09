#include <assert.h> /* to use assert() */
#include <ctype.h>
#include <stdio.h>
#include "str.h"

/**
 * StrGetLength - accept a char pointer and return the length of the string
 *
 * pcSrc: char pointer pointing to the string
 * returns: the length of the null terminated string
 */
size_t StrGetLength(const char *pcSrc) {
    assert(pcSrc); /* NULL address, 0, and FALSE are identical. */

    const char *pcEnd;
    pcEnd = pcSrc;

    while (*pcEnd) /* null character and FALSE are identical. */
        pcEnd++;

    return (size_t)(pcEnd - pcSrc);
}

/**
 * StrCopy - copy a string to another memory location
 *
 * pcDest: char pointer pointing to the destination string
 * pcSrc: char pointer pointing to original source string
 * returns: a pointer to the destination string (pcDest)
 */
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
/**
 * StrCompare - lexicographically compare two strings s1 and s2
 *
 * s1: first string to compare
 * s2: second string to compare
 * returns:
 *  1 if s1 is greater than s2
 *  0 if s1 is equal to s2
 *  -1 if s1 is lesser than s2
 */
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
/**
 * StrFindChr - return the address to the first occurrence of c in pcHaystack
 *
 * pcHaystack: string to find c from
 * c: character to find from pcHaystack
 * returns:
 *  address to the first occurrence of c in pcHaystack
 *  NULL if c is not present
 */
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
/**
 * StrFindStr - return the address to the
 *  first occurrence of pcNeedle in pcHaystack
 *
 * pcHaystack: string to find pcNeedle from
 * pcNeedle: string to find from pcHaystack
 * returns:
 *  address to the first occurrence of pcNeedle in pcHaystack
 *  NULL if pcNeedle is not present
 */
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
/**
 * StrConcat - append a copy of pcSrc to the end of pcDest
 *
 * pcDest: destination of the string to be appended to
 * pcSrc: string whose copy is to be appended to the end of pcDest
 * returns:
 *  address of the destination string
 *
 */
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
/**
 * StrToLong - convert a string to a long value
 *
 * nptr: string whose value is desired to be converted to
 * endptr: address of the first invalid character, if an error occurs
 * base: given base to interpret the integer value
 * returns:
 *  converted integer value
 *
 * nptr accepts a variety of forms.
 * the string can contain a sign before the integer, such as + and -.
 * the function will also ignore any whitespace before the sign or integer
 * if the integer value exceeds data type bounds, it will clamped to the
 * boundary of long.
 */
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

        if (
            isOutOfRange ||
            sum > threshold ||
            sum == threshold &&
            digit > digitLimit
        ) {
            isOutOfRange = 1;
            cursor++;
            continue;
        }

        sum = base * sum + digit;
        cursor++;
    }

    // only save when endptr is provided
    if (endptr)
        *endptr = (char *) cursor;
    if (isOutOfRange)
        sum = isNegative ? LONG_MIN : LONG_MAX;
    if (isNegative)
        sum = -sum;

    return sum;
}

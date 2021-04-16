/**
 * Author: Haechan Kwon (권해찬)
 * Assignment: String (Assignment 2)
 * Filename: sgrep.c
 */

#include "str.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for skeleton code */
#include <unistd.h> /* for getopt */

#define MAX_STR_LEN 1023

#define FALSE 0
#define TRUE 1

/*
 * Fill out your own functions here (If you need)
 */

/*--------------------------------------------------------------------*/
/* PrintUsage()
   print out the usage of the Simple Grep Program                     */
/*--------------------------------------------------------------------*/
void PrintUsage(const char *argv0) {
    const static char *fmt = "Simple Grep (sgrep) Usage:\n"
                             "%s pattern [stdin]\n";

    printf(fmt, argv0);
}

/**
 * LineHasPattern
 *   return whether or not a line has the corresponding pattern
 *
 * param buffer: pointer to a NULL terminated string. contains the line
 * param pattern: pointer to a NULL terminated string. contains the
 *   pattern
 * param isStart: a flag indicating whether the initial function call is
 *   being made. 1 if initial call to function. 0 otherwise.
 *
 * this function recursively searches for the given pattern in the given
 *   buffer.
 */
int LineHasPattern(char *buffer, const char *pattern, int isStart) {
    char *bufferCursor;
    char patternChar;
    int hasPattern;

    bufferCursor = buffer;
    patternChar = *pattern;

    // end of pattern. return true
    if (patternChar == '\0')
        return TRUE;

    // not end of pattern, but end of line.
    if (*bufferCursor == '\0')
        return FALSE;

    if (patternChar == '*') {
        // wildcard character. iterate the buffer and start search from
        // every character
        while (*bufferCursor) {
            hasPattern =
                LineHasPattern(bufferCursor, pattern + 1, FALSE);

            // early return
            if (hasPattern)
                return TRUE;

            bufferCursor++;
        }
    } else if (isStart) {
        // initial call to function. iterate buffer and search from
        // every occurrence of the "first letter of the pattern"
        bufferCursor = StrFindChr(bufferCursor, patternChar);
        while (bufferCursor != NULL) {
            hasPattern =
                LineHasPattern(bufferCursor + 1, pattern + 1, FALSE);
            // early return
            if (hasPattern)
                return TRUE;

            bufferCursor = StrFindChr(bufferCursor + 1, patternChar);
        }
    } else {
        // recursive call if pattern continues to match. return otherwise.
        if (*bufferCursor == patternChar)
            return LineHasPattern(bufferCursor + 1, pattern + 1, FALSE);
    }

    return FALSE;
}

/*-------------------------------------------------------------------*/
/* SearchPattern()
   Your task:
   1. Do argument validation
   - String or file argument length is no more than 1023
   - If you encounter a command-line argument that's too long,
   print out "Error: argument is too long"

   2. Read the each line from standard input (stdin)
   - If you encounter a line larger than 1023 bytes,
   print out "Error: input line is too long"
   - Error message should be printed out to standard error (stderr)

   3. Check & print out the line contains a given string (search-string)

Tips:
- fgets() is an useful function to read characters from file. Note
that the fget() reads until newline or the end-of-file is reached.
- fprintf(sderr, ...) should be useful for printing out error
message to standard error

NOTE: If there is any problem, return FALSE; if not, return TRUE  */
/*-------------------------------------------------------------------*/
int SearchPattern(const char *pattern) {
    // room for newline and null
    // buffer ends with a newline, pattern ends with a null char
    char buf[MAX_STR_LEN + 2];
    int len;

    if (StrGetLength(pattern) > 1023) {
        fprintf(stderr, "Error: pattern is too long\n");
        return FALSE;
    }

    /* Read one line at a time from stdin, and process each line */
    while (fgets(buf, sizeof(buf), stdin)) {
        /* check the length of an input line */
        if ((len = StrGetLength(buf)) > MAX_STR_LEN) {
            fprintf(stderr, "Error: input line is too long\n");
            return FALSE;
        }

        // if line has pattern, print the line to the console.
        if (LineHasPattern(buf, pattern, TRUE))
            printf("%s", buf);
    }

    return TRUE;
}

/*-------------------------------------------------------------------*/
int main(const int argc, const char *argv[]) {
    /* Do argument check and parsing */
    if (argc < 2) {
        fprintf(stderr, "Error: argument parsing error\n");
        PrintUsage(argv[0]);
        return (EXIT_FAILURE);
    }

    return SearchPattern(argv[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
}

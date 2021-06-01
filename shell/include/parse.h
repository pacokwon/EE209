#ifndef __ISH_PARSE_H
#define __ISH_PARSE_H

#include "dynarray.h"
#include <stdbool.h>

enum TokenType {
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIRECT_IN,
    TOKEN_REDIRECT_OUT,
};

struct Token {
    enum TokenType type;
    char *pcValue;
};

bool parseLine(char *, DynArray_T);
void freeLine(DynArray_T);

#endif

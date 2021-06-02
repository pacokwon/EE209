#ifndef __ISH_PARSE_H
#define __ISH_PARSE_H

#include "dynarray.h"
#include <stdbool.h>

enum TokenType {
  TOKEN_WORD,
  TOKEN_PIPE,
  TOKEN_REDIRECT_IN,
  TOKEN_REDIRECT_OUT,
  TOKEN_BACKGROUND,
};

enum ParseResult {
  PARSE_NO_TOKENS,
  PARSE_NO_QUOTE_PAIR,
  PARSE_SUCCESS,
};

struct Token {
  enum TokenType type;
  char *value;
};

enum ParseResult parseLine(char *, DynArray_T);
void freeLine(DynArray_T);
void printToken(void *, void *);

#endif

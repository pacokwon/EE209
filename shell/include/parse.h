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

enum ParseResult parse_line(char *, DynArray_T);
void free_line(DynArray_T);
void print_token(void *, void *);
int count_pipes(DynArray_T);
bool is_background(DynArray_T);

#endif

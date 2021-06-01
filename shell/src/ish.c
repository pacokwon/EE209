#include "parse.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_LINE_SIZE 1023

void loop() {}

int main(void) {
  char cmd[MAX_LINE_SIZE];
  DynArray_T tokens;
  bool success;

  while (fgets(cmd, MAX_LINE_SIZE, stdin) != NULL) {
    tokens = DynArray_new(0);
    if (tokens == NULL) {
      fprintf(stderr, "Cannot allocate memory\n");
      exit(EXIT_FAILURE);
    }

    success = parseLine(cmd, tokens);
    if (success) {
      DynArray_map(tokens, printToken, NULL);
      printf("\n");
    }

    freeLine(tokens);
  }

  return 0;
}

#include "parse.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE_SIZE 1023

extern char **environ;
DynArray_T jobs;
char *prompt = "% ";

/* static int allocateJobID() { */
/*   static int jobID = 1; */
/*   return jobID++; */
/* } */

void evaluate(char *);
bool handleIfBuiltin(DynArray_T);
void countPipes(void *, void *);

int main(int argc, char **argv) {
  char cmd[MAX_LINE_SIZE];
  jobs = DynArray_new(0);

  while (true) {
    printf("%s", prompt);

    if (fgets(cmd, MAX_LINE_SIZE, stdin) == NULL)
      exit(0);

    evaluate(cmd);
  }

  return 0;
}

void evaluate(char *cmd) {
  DynArray_T tokens;
  enum ParseResult result;
  bool isBuiltin;
  int pipes;

  tokens = DynArray_new(0);
  if (tokens == NULL) {
    fprintf(stderr, "Cannot allocate memory\n");
    exit(EXIT_FAILURE);
  }

  result = parseLine(cmd, tokens);

  // 1. tokens must not be empty
  // 2. first token MUST be a command
  if (!DynArray_getLength(tokens) ||
      ((struct Token *)DynArray_get(tokens, 0))->type != TOKEN_WORD)
    goto done;

  if (result == PARSE_SUCCESS) {
    DynArray_map(tokens, printToken, NULL);
    /* printf("\n"); */
  }

  isBuiltin = handleIfBuiltin(tokens);
  if (isBuiltin)
    goto done;

  pipes = 0;
  DynArray_map(tokens, countPipes, &pipes);
  printf ("# of pipes: %d\n", pipes);

done:
  freeLine(tokens);
}

bool handleIfBuiltin(DynArray_T tokens) {
  // length of more than 0 should be guaranteed
  assert(DynArray_getLength(tokens));

  bool isBuiltin = false;
  int length = DynArray_getLength(tokens);
  struct Token *first = DynArray_get(tokens, 0);
  if (first->type != TOKEN_WORD)
    return false;

  if (!strcmp(first->value, "setenv")) {
    char *var, *val;

    assert(length >= 2);
    var = ((struct Token *) DynArray_get(tokens, 1))->value;
    val = length == 2 ? "" : ((struct Token *)DynArray_get(tokens, 2))->value;
    setenv(var, val, 1);

    return true;
  } else if (!strcmp(first->value, "unsetenv")) {
    char *var;

    assert(length >= 2);
    var = ((struct Token *)DynArray_get(tokens, 1))->value;
    unsetenv(var);

    return true;
  } else if (!strcmp(first->value, "cd")) {
    char *dir = length == 2 ? ((struct Token *) DynArray_get(tokens, 1))->value
                            : getenv("HOME");
    if (!chdir(dir)) {
      // TODO: print error message
    }

    return true;
  } else if (!strcmp(first->value, "exit")) {
    exit(0);
  } else if (!strcmp(first->value, "fg")) {
    return true;
  }

  return isBuiltin;
}

void countPipes(void *element, void *extra) {
  struct Token *token = element;
  int *sum = extra;

  if (token->type == TOKEN_PIPE)
    (*sum)++;
}

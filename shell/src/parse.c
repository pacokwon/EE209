#include "parse.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum ParseState {
  STATE_START,
  STATE_WORD,
  STATE_PIPE,
  STATE_REDIRECT_IN,
  STATE_REDIRECT_OUT,
  STATE_BACKGROUND,
  STATE_FINISH,
};

struct StateBox {
  bool is_inside_quote;
  enum ParseState state;
  int index;
  int word_start_index;

  // number of characters in the current word
  int word_char_count;
};

static void parse_start(char *line, DynArray_T tokens, struct StateBox *box);
static void parse_word(char *line, DynArray_T tokens, struct StateBox *box);
static void parse_pipe(char *line, DynArray_T tokens, struct StateBox *box);
static void parse_redirect(char *line, DynArray_T tokens, struct StateBox *box);
static void parse_background(char *line, DynArray_T tokens, struct StateBox *box);
static struct Token *make_token(char *line, struct StateBox *box,
                               enum TokenType type);
static void free_token(void *pvItem, void *pvExtra);
static void count_pipes_helper(void *, void *);

enum ParseResult parse_line(char *line, DynArray_T tokens) {
  struct StateBox box = (struct StateBox){
      .is_inside_quote = false,
      .state = STATE_START,
      .index = 0,
      .word_start_index = 0,
      .word_char_count = 0,
  };

  while (box.state != STATE_FINISH) {
    switch (box.state) {
    case STATE_START:
      parse_start(line, tokens, &box);
      break;
    case STATE_WORD:
      parse_word(line, tokens, &box);
      break;
    case STATE_PIPE:
      parse_pipe(line, tokens, &box);
      break;
    case STATE_REDIRECT_IN:
    case STATE_REDIRECT_OUT:
      parse_redirect(line, tokens, &box);
      break;
    case STATE_BACKGROUND:
      parse_background(line, tokens, &box);
      break;
    case STATE_FINISH:
      assert("Shouldn't Reach Here!");
      break;
    }
  }

  if (box.is_inside_quote)
    /* printf("Could not find quote pair\n"); */
    return PARSE_NO_QUOTE_PAIR;

  if (!DynArray_getLength(tokens))
    return PARSE_NO_TOKENS;

  return PARSE_SUCCESS;
}

void free_line(DynArray_T tokens) {
    DynArray_map(tokens, free_token, NULL);
    DynArray_free(tokens);
}

static void parse_start(char *line, DynArray_T tokens, struct StateBox *box) {
  char c = line[box->index];
  switch (c) {
  case '\0':
    box->state = STATE_FINISH;
    break;
  case '|':
    box->state = STATE_PIPE;
    break;
  case '<':
    box->state = STATE_REDIRECT_IN;
    break;
  case '>':
    box->state = STATE_REDIRECT_OUT;
    break;
  case '&':
    box->state = STATE_BACKGROUND;
    break;
  default:
    if (isspace(c))
      break; // do nothing
    box->state = STATE_WORD;
    box->word_start_index = box->index;

    if (c == '"') {
      box->is_inside_quote = true;
      box->word_char_count = 0;
    } else {
      box->word_char_count = 1;
    }
  }

  box->index++;
  return;
}

static void parse_word(char *line, DynArray_T tokens, struct StateBox *box) {
  char c = line[box->index];
  bool shouldEarlyReturn = true;
  struct Token *token;

  if (box->is_inside_quote) {
    if (c == '\0') {
      box->state = STATE_FINISH;
      token = make_token(line, box, TOKEN_WORD);
      DynArray_add(tokens, token);
    } else if (c == '"') {
      assert(box->is_inside_quote);
      box->is_inside_quote = false;
      box->word_char_count--; // hack; decrement beforehand
    }
  } else {
    switch (c) {
    case '\0':
      box->state = STATE_FINISH;
      break;
    case '|':
      box->state = STATE_PIPE;
      break;
    case '<':
      box->state = STATE_REDIRECT_IN;
      break;
    case '>':
      box->state = STATE_REDIRECT_OUT;
      break;
    case '&':
      box->state = STATE_BACKGROUND;
      break;
    default:
      shouldEarlyReturn = false;
    }

    if (shouldEarlyReturn) {
      token = make_token(line, box, TOKEN_WORD);
      DynArray_add(tokens, token);
      box->index++;
      return;
    }

    if (isspace(c)) {
      box->state = STATE_START;
      token = make_token(line, box, TOKEN_WORD);
      DynArray_add(tokens, token);
    } else if (c == '"') {
      box->is_inside_quote = true;
      box->word_char_count--; // hack; decrement beforehand
    }
  }

  box->index++;

  // has no meaning if state is being transferred
  box->word_char_count++;
  return;
}

static void parse_pipe(char *line, DynArray_T tokens, struct StateBox *box) {
  char c = line[box->index];
  struct Token *token = make_token(line, box, TOKEN_PIPE);
  DynArray_add(tokens, token);

  if (c == '\0') box->state = STATE_FINISH;
  else if (c == '&') box->state = STATE_BACKGROUND;
  else if (c == '|') box->state = STATE_PIPE;
  else if (isspace(c)) box->state = STATE_START;
  else {
    box->state = STATE_WORD;
    box->word_start_index = box->index;

    if (c == '"') {
      assert(!box->is_inside_quote);
      box->is_inside_quote = true;
      box->word_char_count = 0;
    } else {
      box->word_char_count = 1;
    }
  }

  box->index++;
  return;
}

static void parse_redirect(char *line, DynArray_T tokens, struct StateBox *box) {
  assert(box->state == STATE_REDIRECT_IN || box->state == STATE_REDIRECT_OUT);

  char c = line[box->index];
  enum TokenType type = box->state == STATE_REDIRECT_IN ? TOKEN_REDIRECT_IN : TOKEN_REDIRECT_OUT;
  struct Token *token = make_token(line, box, type);
  DynArray_add(tokens, token);

  if (c == '\0') box->state = STATE_FINISH;
  else if (c == '&') box->state = STATE_BACKGROUND;
  else if (c == '<') box->state = STATE_REDIRECT_IN;
  else if (c == '>') box->state = STATE_REDIRECT_OUT;
  else if (isspace(c)) box->state = STATE_START;
  else {
    box->state = STATE_WORD;
    box->word_start_index = box->index;

    if (c == '"') {
      assert(!box->is_inside_quote);
      box->is_inside_quote = true;
      box->word_char_count = 0;
    } else {
      box->word_char_count = 1;
    }
  }

  box->index++;
  return;
}

static void parse_background(char *line, DynArray_T tokens, struct StateBox *box) {
  struct Token *token = make_token(line, box, TOKEN_BACKGROUND);
  DynArray_add(tokens, token);

  box->state = STATE_FINISH;

  return;
}

static struct Token *make_token(char *line, struct StateBox *box,
                               enum TokenType type) {
  int start = box->word_start_index;
  int end = box->index;
  int size = box->word_char_count;

  // empty strings are also not allowed
  assert(start < end);

  int i;
  char *tokenCursor = line + start;
  struct Token *token = malloc(sizeof(struct Token));
  if (token == NULL)
    return NULL;
  token->type = type;

  if (type != TOKEN_WORD)
    return token;

  token->value = malloc((size + 1) * sizeof(char));
  if (token->value == NULL) {
    free(token);
    return NULL;
  }

  i = 0;
  while (i < size) {
    if (*tokenCursor == '"') {
      tokenCursor++;
      continue;
    }

    token->value[i] = *tokenCursor;
    tokenCursor++;
    i++;
  }
  token->value[size] = '\0';

  return token;
}

static void free_token(void *pvItem, void *pvExtra) {
   struct Token *token = (struct Token *) pvItem;
   if (token->type == TOKEN_WORD)
     free(token->value);

   free(token);
}

void print_token(void *pvItem, void *pvExtra) {
  struct Token *token = (struct Token *) pvItem;

  if (token->type == TOKEN_WORD) printf("TOKEN_WORD\t%s\n", token->value);
  else if (token->type == TOKEN_PIPE) printf("TOKEN_PIPE\t|\n");
  else if (token->type == TOKEN_REDIRECT_IN) printf("TOKEN_REDIR_IN\t<\n");
  else if (token->type == TOKEN_REDIRECT_OUT) printf("TOKEN_REDIR_OUT\t>\n");
  else if (token->type == TOKEN_BACKGROUND) printf("TOKEN_BACKGROUND\t&\n");
  else printf("Invalid Token\n");
}

int count_pipes(DynArray_T tokens) {
  int pipes = 0;
  DynArray_map(tokens, count_pipes_helper, &pipes);
  return pipes;
}

static void count_pipes_helper(void *element, void *extra) {
  struct Token *token = element;
  int *sum = extra;

  if (token->type == TOKEN_PIPE)
    (*sum)++;
}

bool is_background(DynArray_T tokens) {
  int length = DynArray_getLength(tokens);
  struct Token *token = DynArray_get(tokens, length - 1);

  return token && token->type == TOKEN_BACKGROUND;
}

// construct an array of strings from a tokens dynarray
// tokenCursor will be updated to point to the next command token
int construct_exec_unit(DynArray_T tokens, int token_cursor, struct ExecUnit *e) {
  int i = token_cursor, j;
  int length = DynArray_getLength(tokens);
  int argc = 0;
  struct Token *token;

  e->infile = NULL;
  e->outfile = NULL;

  // iteration #1, to compute `argc`
  while (i < length && ((struct Token *) DynArray_get(tokens, i))->type == TOKEN_WORD) {
    argc++;
    i++;
  }

  char **argv = calloc(argc + 1, sizeof(char *));
  // iteration #2, to construct `argv`
  i = token_cursor;
  j = 0;
  while (i < length && (token = DynArray_get(tokens, i))->type == TOKEN_WORD) {
    argv[j] = token->value;
    i++;
    j++;
  }
  argv[argc] = NULL;
  e->argc = argc;
  e->argv = argv;

  // the ith token is a token that is not a word. so one of:
  // 1) redir in | 2) redir out | 3) pipe | 4) background

  // if this loop ends,
  // its either because the token is a pipe,
  // or its the end of the list
  while (i < length && (token = DynArray_get(tokens, i))->type != TOKEN_PIPE) {
    enum TokenType type = token->type;

    if (type == TOKEN_REDIRECT_IN || type == TOKEN_REDIRECT_OUT) {
      token = DynArray_get(tokens, ++i);

      if (token->type != TOKEN_WORD) {
        if (type == TOKEN_REDIRECT_IN)
          fprintf(stderr, "%s: Standard input redirection without file name\n", shell_name);

        if (type == TOKEN_REDIRECT_OUT)
          fprintf(stderr, "%s: Standard output redirection without file name\n", shell_name);

        return -1;
      }

      // NOTE: from here, `token` is a word
      if (type == TOKEN_REDIRECT_IN) {
        if (e->infile != NULL) {
          fprintf(stderr, "%s: Multiple redirection of standard input\n", shell_name);
          return -1;
        }

        e->infile = token->value;
      } else {
        if (e->outfile != NULL) {
          fprintf(stderr, "%s: Multiple redirection of standard out\n", shell_name);
          return -1;
        }

        e->outfile = token->value;
      }
    }

    i++;
  }

  // ignore anything that comes in between a pipe and the next word
  while (i < length && ((token = DynArray_get(tokens, i))->type != TOKEN_WORD))
    i++;

  // i will point to a word or an end of the token list
  return i;
}

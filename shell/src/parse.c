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
  bool isInsideQuote;
  enum ParseState state;
  int index;
  int wordStartIndex;

  // number of characters in the current word
  int wordCharCount;
};

static void parseStart(char *line, DynArray_T tokens, struct StateBox *box);
static void parseWord(char *line, DynArray_T tokens, struct StateBox *box);
static void parsePipe(char *line, DynArray_T tokens, struct StateBox *box);
static void parseRedirect(char *line, DynArray_T tokens, struct StateBox *box);
static void parseBackground(char *line, DynArray_T tokens, struct StateBox *box);
static struct Token *makeToken(char *line, struct StateBox *box,
                               enum TokenType type);
static void freeToken(void *pvItem, void *pvExtra);

enum ParseResult parseLine(char *line, DynArray_T tokens) {
  struct StateBox box = (struct StateBox){
      .isInsideQuote = false,
      .state = STATE_START,
      .index = 0,
      .wordStartIndex = 0,
      .wordCharCount = 0,
  };

  while (box.state != STATE_FINISH) {
    switch (box.state) {
    case STATE_START:
      parseStart(line, tokens, &box);
      break;
    case STATE_WORD:
      parseWord(line, tokens, &box);
      break;
    case STATE_PIPE:
      parsePipe(line, tokens, &box);
      break;
    case STATE_REDIRECT_IN:
    case STATE_REDIRECT_OUT:
      parseRedirect(line, tokens, &box);
      break;
    case STATE_BACKGROUND:
      parseBackground(line, tokens, &box);
      break;
    case STATE_FINISH:
      assert("Shouldn't Reach Here!");
      break;
    }
  }

  if (box.isInsideQuote)
    /* printf("Could not find quote pair\n"); */
    return PARSE_NO_QUOTE_PAIR;

  if (!DynArray_getLength(tokens))
    return PARSE_NO_TOKENS;

  return PARSE_SUCCESS;
}

void freeLine(DynArray_T tokens) {
    DynArray_map(tokens, freeToken, NULL);
    DynArray_free(tokens);
}

static void parseStart(char *line, DynArray_T tokens, struct StateBox *box) {
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
    box->wordStartIndex = box->index;

    if (c == '"') {
      box->isInsideQuote = true;
      box->wordCharCount = 0;
    } else {
      box->wordCharCount = 1;
    }
  }

  box->index++;
  return;
}

static void parseWord(char *line, DynArray_T tokens, struct StateBox *box) {
  char c = line[box->index];
  bool shouldEarlyReturn = true;
  struct Token *token;

  if (box->isInsideQuote) {
    if (c == '\0') {
      box->state = STATE_FINISH;
      token = makeToken(line, box, TOKEN_WORD);
      DynArray_add(tokens, token);
    } else if (c == '"') {
      assert(box->isInsideQuote);
      box->isInsideQuote = false;
      box->wordCharCount--; // hack; decrement beforehand
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
      token = makeToken(line, box, TOKEN_WORD);
      DynArray_add(tokens, token);
      box->index++;
      return;
    }

    if (isspace(c)) {
      box->state = STATE_START;
      token = makeToken(line, box, TOKEN_WORD);
      DynArray_add(tokens, token);
    } else if (c == '"') {
      box->isInsideQuote = true;
      box->wordCharCount--; // hack; decrement beforehand
    }
  }

  box->index++;

  // has no meaning if state is being transferred
  box->wordCharCount++;
  return;
}

static void parsePipe(char *line, DynArray_T tokens, struct StateBox *box) {
  char c = line[box->index];
  struct Token *token = makeToken(line, box, TOKEN_PIPE);
  DynArray_add(tokens, token);

  if (c == '\0') box->state = STATE_FINISH;
  else if (c == '&') box->state = STATE_BACKGROUND;
  else if (c == '|') box->state = STATE_PIPE;
  else if (isspace(c)) box->state = STATE_START;
  else {
    box->state = STATE_WORD;
    box->wordStartIndex = box->index;

    if (c == '"') {
      assert(!box->isInsideQuote);
      box->isInsideQuote = true;
      box->wordCharCount = 0;
    } else {
      box->wordCharCount = 1;
    }
  }

  box->index++;
  return;
}

static void parseRedirect(char *line, DynArray_T tokens, struct StateBox *box) {
  assert(box->state == STATE_REDIRECT_IN || box->state == STATE_REDIRECT_OUT);

  char c = line[box->index];
  enum TokenType type = box->state == STATE_REDIRECT_IN ? TOKEN_REDIRECT_IN : TOKEN_REDIRECT_OUT;
  struct Token *token = makeToken(line, box, type);
  DynArray_add(tokens, token);

  if (c == '\0') box->state = STATE_FINISH;
  else if (c == '&') box->state = STATE_BACKGROUND;
  else if (c == '<') box->state = STATE_REDIRECT_IN;
  else if (c == '>') box->state = STATE_REDIRECT_OUT;
  else if (isspace(c)) box->state = STATE_START;
  else {
    box->state = STATE_WORD;
    box->wordStartIndex = box->index;

    if (c == '"') {
      assert(!box->isInsideQuote);
      box->isInsideQuote = true;
      box->wordCharCount = 0;
    } else {
      box->wordCharCount = 1;
    }
  }

  box->index++;
  return;
}

static void parseBackground(char *line, DynArray_T tokens, struct StateBox *box) {
  struct Token *token = makeToken(line, box, TOKEN_BACKGROUND);
  DynArray_add(tokens, token);

  box->state = STATE_FINISH;

  return;
}

static struct Token *makeToken(char *line, struct StateBox *box,
                               enum TokenType type) {
  int start = box->wordStartIndex;
  int end = box->index;
  int size = box->wordCharCount;

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

static void freeToken(void *pvItem, void *pvExtra) {
   struct Token *token = (struct Token *) pvItem;
   if (token->type == TOKEN_WORD)
     free(token->value);

   free(token);
}

void printToken(void *pvItem, void *pvExtra) {
  struct Token *token = (struct Token *) pvItem;

  if (token->type == TOKEN_WORD) printf("TOKEN_WORD\t%s\n", token->value);
  else if (token->type == TOKEN_PIPE) printf("TOKEN_PIPE\t|\n");
  else if (token->type == TOKEN_REDIRECT_IN) printf("TOKEN_REDIR_IN\t<\n");
  else if (token->type == TOKEN_REDIRECT_OUT) printf("TOKEN_REDIR_OUT\t>\n");
  else if (token->type == TOKEN_BACKGROUND) printf("TOKEN_BACKGROUND\t&\n");
  else printf("Invalid Token\n");
}

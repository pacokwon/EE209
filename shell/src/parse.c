#include "parse.h"
#include <ctype.h>

enum ParseState {
    STATE_START,
    STATE_WORD,
    STATE_PIPE,
    STATE_REDIRECT,
};

struct StateBox {
    int index;
    enum ParseState state;
};

static void parseStart(char *line, DynArray_T tokens, struct StateBox *box);
static void parseWord(char *line, DynArray_T tokens, struct StateBox *box);
static void parsePipe(char *line, DynArray_T tokens, struct StateBox *box);
static void parseRedirect(char *line, DynArray_T tokens, struct StateBox *box);

bool parseLine(char *line, DynArray_T tokens) {
    struct StateBox box = (struct StateBox) {
        .index = 0,
        .state = STATE_START,
    };

    while (true) {
        switch (box.state) {
        case STATE_START:

        case STATE_WORD:
        case STATE_PIPE:
        case STATE_REDIRECT:
        }
    }

    return true;
}

void freeLine(DynArray_T tokens) {

}

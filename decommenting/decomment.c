#include "decomment.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int c;

    struct StateBox box;
    box.currentSurrounder = '\0';
    box.commentStartLine = 1;
    box.currentLine = 1;
    box.state = Start;

    while ((c = getchar()) != EOF) {
        if (c == '\n')
            box.currentLine++;

        switch (box.state) {
        case Start:
            opStart(c, &box);
            break;
        case StringLiteralContent:
            opStringLiteralContent(c, &box);
            break;
        case StringLiteralBackslash:
            opStringLiteralBackslash(c, &box);
            break;
        case CommentInitialBackslash:
            opCommentInitialBackslash(c, &box);
            break;
        case CommentInitialAsterisk:
            opCommentInitialAsterisk(c, &box);
            break;
        case CommentFinalAsterisk:
            opCommentFinalAsterisk(c, &box);
            break;
        default:
            assert(0);
        }
    }

    if (box.state == CommentInitialAsterisk ||
        box.state == CommentFinalAsterisk) {
        fprintf(stderr, "Error: line %d: unterminated comment\n",
                box.commentStartLine);

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void opStart(char inputChar, struct StateBox *box) {
    if (inputChar == '/') {
        box->state = CommentInitialBackslash;
    } else if (inputChar == '\'' || inputChar == '"') {
        box->currentSurrounder = inputChar;
        box->state = StringLiteralContent;
        printf("%c", inputChar);
    } else {
        printf("%c", inputChar);
    }
}

void opStringLiteralContent(char inputChar, struct StateBox *box) {
    char surrounder = box->currentSurrounder;

    assert(surrounder != '\0');

    if (inputChar == surrounder) {
        box->currentSurrounder = '\0';
        box->state = Start;
    } else if (inputChar == '\\') {
        box->state = StringLiteralBackslash;
    } else {
        // no-op
    }

    printf("%c", inputChar);
}

void opStringLiteralBackslash(char inputChar, struct StateBox *box) {
    box->state = StringLiteralContent;
    printf("%c", inputChar);
}

void opCommentInitialBackslash(char inputChar, struct StateBox *box) {
    if (inputChar == '*') {
        box->state = CommentInitialAsterisk;
        box->commentStartLine = box->currentLine;
        printf(" ");
        return;
    } else if (inputChar == '/') {
        printf("/"); // print the previously unprinted slash
    } else {
        box->state = Start;
        printf("/%c", inputChar);
    }
}

void opCommentInitialAsterisk(char inputChar, struct StateBox *box) {
    if (inputChar == '*') {
        box->state = CommentFinalAsterisk;
    } else if (inputChar == '\n') {
        printf("%c", inputChar);
    } else {
        // no-op
    }
}

void opCommentFinalAsterisk(char inputChar, struct StateBox *box) {
    if (inputChar == '/') {
        box->state = Start;
    } else if (inputChar == '*') {
        // no-op
    } else if (inputChar == '\n') {
        printf("%c", inputChar);
    } else {
        box->state = CommentInitialAsterisk;
    }
}

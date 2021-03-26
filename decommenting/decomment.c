/**
 * Author: Haechan Kwon (권해찬)
 * Assignment: Decommenting (Assignment 1)
 * Filename: decomment.c
 */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// enum containing all DFA states
enum DFAState {
    Start,
    StringLiteralContent,
    StringLiteralBackslash,
    CommentInitialBackslash,
    CommentInitialAsterisk,
    CommentFinalAsterisk,
};

// struct containing the states necessary to keep track of
struct StateBox {
    // current surrounding character.
    // if the current state is inside a string literal,
    // the surrounder might be something like "
    char currentSurrounder;

    // the line no. that the current comment (if any) has started from
    int commentStartLine;

    // current line number
    int currentLine;

    // current DFA state
    enum DFAState state;
};

void stateBoxInit(struct StateBox* box);

void opStart(char inputChar, struct StateBox *box);
void opStringLiteralContent(char inputChar, struct StateBox *box);
void opStringLiteralBackslash(char inputChar, struct StateBox *box);
void opStringLiteralEnd(char inputChar, struct StateBox *box);
void opCommentInitialBackslash(char inputChar, struct StateBox *box);
void opCommentInitialAsterisk(char inputChar, struct StateBox *box);
void opCommentContent(char inputChar, struct StateBox *box);
void opCommentFinalAsterisk(char inputChar, struct StateBox *box);
void opCommentEnd(char inputChar, struct StateBox *box);

/**
 * main function
 * Initialize the state box containing the current state.
 * And read characters, modifying the current state until EOF
 * If state is invalid after EOF, print error message to stderr
 */
int main(void) {
    int c;
    struct StateBox box;
    stateBoxInit(&box);

    // keep reading characters until EOF
    while ((c = getchar()) != EOF) {
        // keep track of current line
        if (c == '\n')
            box.currentLine++;

        // run the respective handler according to the state
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

    // if EOF is encountered and state is still inside comment,
    // print error message to stderr
    if (box.state == CommentInitialAsterisk ||
        box.state == CommentFinalAsterisk) {
        fprintf(stderr, "Error: line %d: unterminated comment\n",
                box.commentStartLine);

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * stateBoxInit function
 * param box: a pointer to a struct StateBox type
 *
 * initialize a statebox value's fields
 */
void stateBoxInit(struct StateBox *box) {
    // no surrounder. initialize with null
    box->currentSurrounder = '\0';

    // at initial state, line number is 1
    box->commentStartLine = 1;
    box->currentLine = 1;

    // state starts at Start
    box->state = Start;
}

/**
 * opStart function
 * param inputChar: the current character that is read as input
 * param box: a pointer to a state box
 */
void opStart(char inputChar, struct StateBox *box) {
    if (inputChar == '/') {
        // transition state
        // this is start of comment, so do not print
        box->state = CommentInitialBackslash;
    } else if (inputChar == '\'' || inputChar == '"') {
        // update current surrounder, and transition state
        box->currentSurrounder = inputChar;
        box->state = StringLiteralContent;
        printf("%c", inputChar);
    } else {
        printf("%c", inputChar);
    }
}

/**
 * opStringLiteralContent function
 * param inputChar: the current character that is read as input
 * param box: a pointer to a state box
 */
void opStringLiteralContent(char inputChar, struct StateBox *box) {
    char surrounder = box->currentSurrounder;

    // surrounder must exist in this state
    assert(surrounder != '\0');

    if (inputChar == surrounder) {
        // end of string. go back to start
        box->currentSurrounder = '\0';
        box->state = Start;
    } else if (inputChar == '\\') {
        // escape letter inside string. transition state
        box->state = StringLiteralBackslash;
    }

    // always print string content
    printf("%c", inputChar);
}

/**
 * opStringLiteralBackslash function
 * param inputChar: the current character that is read as input
 * param box: a pointer to a state box
 */
void opStringLiteralBackslash(char inputChar, struct StateBox *box) {
    // accept any single character and transition state
    box->state = StringLiteralContent;
    printf("%c", inputChar);
}

/**
 * opCommentInitialBackslash function
 * param inputChar: the current character that is read as input
 * param box: a pointer to a state box
 */
void opCommentInitialBackslash(char inputChar, struct StateBox *box) {
    if (inputChar == '*') {
        // start of comment if * comes after /. transition state
        box->state = CommentInitialAsterisk;

        // remember where this comment is starting from
        box->commentStartLine = box->currentLine;
        printf(" ");
    } else if (inputChar == '/') {
        // print the previously unprinted /
        printf("/");
    } else {
        // not a comment! print the unprinted / and character
        box->state = Start;
        printf("/%c", inputChar);
    }
}

/**
 * opCommentInitialAsterisk function
 * param inputChar: the current character that is read as input
 * param box: a pointer to a state box
 */
void opCommentInitialAsterisk(char inputChar, struct StateBox *box) {
    if (inputChar == '*') {
        // has potential to be end of comment. transition state
        box->state = CommentFinalAsterisk;
    } else if (inputChar == '\n') {
        // comments are not printed. except newlines
        printf("%c", inputChar);
    }
}

/**
 * opCommentFinalAsterisk function
 * param inputChar: the current character that is read as input
 * param box: a pointer to a state box
 */
void opCommentFinalAsterisk(char inputChar, struct StateBox *box) {
    if (inputChar == '/') {
        // end of comment. go back to start
        box->state = Start;
    } else if (inputChar == '*') {
        // no-op. but do not transition state
    } else if (inputChar == '\n') {
        // comments are not printed. except newlines
        printf("%c", inputChar);
    } else {
        // still inside comment. go back to previous state.
        box->state = CommentInitialAsterisk;
    }
}

/**
 * Author: Haechan Kwon (권해찬)
 * Assignment: Decommenting (Assignment 1)
 * Filename: decomment.h
 */

#ifndef DECOMMENT_H
#define DECOMMENT_H

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

#endif

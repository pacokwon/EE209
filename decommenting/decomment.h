#ifndef DECOMMENT_H
#define DECOMMENT_H

enum DFAState {
    Start,
    StringLiteralContent,
    StringLiteralBackslash,
    CommentInitialBackslash,
    CommentInitialAsterisk,
    CommentFinalAsterisk,
};


struct StateBox {
    char currentSurrounder;
    int commentStartLine;
    int currentLine;
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

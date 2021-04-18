#ifndef __LIST_H
#define __LIST_H

#include <stdbool.h>

struct Node {
    char *key;
    int value;
    struct Node *next;
};

struct Table {
    struct Node *first;
};

struct Table *table_create(void);
void table_add(struct Table *, const char *, int);
bool table_search(struct Table *, const char *, int *);
void table_free(struct Table *);
void table_print(struct Table *);
void table_update(struct Table *, const char *, int);
void table_delete(struct Table *, const char *);
void table_reverse(struct Table *);
#endif

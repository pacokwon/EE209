#ifndef __HT_H
#define __HT_H

#include <stdbool.h>

enum { BUCKET_COUNT = 1024 };

struct Node {
    char *key;
    int value;
    struct Node *next;
};

struct Table {
    struct Node *array[BUCKET_COUNT];
};

struct Table *table_create(void);
void table_add(struct Table *, const char *, int);
bool table_search(struct Table *, const char *, int *);
void table_free(struct Table *);
void table_print(struct Table *);

#endif

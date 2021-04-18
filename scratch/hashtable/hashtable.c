#include "hashtable.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static unsigned int hash(const char *x) {
    int i;
    unsigned int h = 0U;
    for (i = 0; x[i] != '\0'; i++)
        h = h * 65599 + (unsigned char) x[i];

    return h % BUCKET_COUNT;
}

struct Table *table_create(void) {
    struct Table *table = malloc(sizeof (struct Table));
    memset(table->array, 0, sizeof (struct Node *) * BUCKET_COUNT);
    return table;
}

void table_add(struct Table *table, const char *key, int value) {
    int hashed = hash(key);
    struct Node *node = malloc(sizeof (struct Node));
    node->key = malloc(strlen(key) + 1);
    strcpy(node->key, key);
    node->value = value;
    node->next = table->array[hashed];
    table->array[hashed] = node;
}

bool table_search(struct Table *table, const char *key, int *value) {
    int hashed = hash(key);

    for (struct Node *p = table->array[hashed]; p != NULL; p = p->next)
        if (strcmp(p->key, key) == 0) {
            *value = p->value;
            return true;
        }

    return false;
}

void table_free(struct Table *table) {
    struct Node *nextp;

    for (unsigned u = 0; u < BUCKET_COUNT; u++) {
        for (struct Node *p = table->array[u]; p != NULL; p = nextp) {
            nextp = p->next;
            free(p->key);
            free(p);
        }
    }

    free(table);
}

void table_print(struct Table *table) {
    printf("------- Table Print Start-------\n");
    for (unsigned u = 0; u < BUCKET_COUNT; u++) {
        if (table->array[u] == NULL) continue;

        printf("Entry in %u\n", u);
        for (struct Node *p = table->array[u]; p != NULL; p = p->next) {
            printf("\t%s\t%d\n", p->key, p->value);
        }
    }
    printf("-------Table Print Finish-------\n\n");
}

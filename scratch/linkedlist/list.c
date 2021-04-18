#include "list.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct Table *table_create(void) {
    struct Table *new_table = malloc(sizeof (struct Table));
    new_table->first = NULL;
    return new_table;
}

void table_add(struct Table *table, const char *key, int value) {
    struct Node *new_node = malloc(sizeof (struct Node));

    new_node->key = malloc(strlen(key) + 1);
    strcpy((char *) new_node->key, key);

    new_node->value = value;

    new_node->next = table->first;
    table->first = new_node;
}

bool table_search(struct Table *table, const char *key, int *result) {
    for (struct Node *p = table->first; p != NULL; p = p->next) {
        if (strcmp(p->key, key) == 0) {
            *result = p->value;
            return true;
        }
    }

    return false;
}

void table_free(struct Table *table) {
    struct Node *nextp;

    for (struct Node *p = table->first; p != NULL; p = nextp) {
        nextp = p->next;
        free(p->key);
        free(p);
    }

    free(table);
}

void table_print(struct Table *table) {
    printf("------- Table Print Start-------\n");
    for (struct Node *p = table->first; p != NULL; p = p->next)
        printf("\t%s\t%d\n", p->key, p->value);
    printf("-------Table Print Finish-------\n\n");
}

void table_update(struct Table *table, const char *key, int value) {
    for (struct Node *p = table->first; p != NULL; p = p->next)
        if (strcmp(p->key, key) == 0)
            p->value = value;
}

void table_delete(struct Table *table, const char *key) {
    struct Node *nextp, *p;

    while (table->first != NULL && strcmp(table->first->key, key) == 0) {
        nextp = table->first->next;

        free(table->first->key);
        free(table->first);

        table->first = nextp;
    }

    if (table->first == NULL) return;

    p = table->first;
    nextp = p->next;

    while (nextp != NULL) {
        if (strcmp(nextp->key, key) == 0) {
            p->next = nextp->next;
            free (nextp->key);
            free (nextp);
            nextp = p->next;
            continue;
        }

        p = nextp;
        nextp = p->next;
    }
}

void table_reverse(struct Table *table) {
    // no point in reversing if table is empty or there's only one
    if (table->first == NULL || table->first->next == NULL) return;

    struct Node *before, *curr, *after;
    before = table->first;
    curr = before->next;
    after = curr->next;

    before->next = NULL;
    while (after != NULL) {
        curr->next = before;
        before = curr;
        curr = after;
        after = after->next;
    }
    curr->next = before;
    table->first = curr;
}

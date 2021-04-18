#include "hashtable.h"
#include <stdio.h>

int main(void) {
    struct Table *table = table_create();

    table_add(table, "foo", 3);
    table_add(table, "bar", 4);
    table_add(table, "hello", 7);
    table_add(table, "world", 9);
    table_add(table, "baz", 10);

    table_print(table);

    int result;
    if (table_search(table, "hello", &result))
        printf("\"hello\"'s value is %d\n", result);

    if (!table_search(table, "bacon", &result))
        printf("\"bacon\" is not present in the hash table\n");

    table_free(table);
    return 0;
}

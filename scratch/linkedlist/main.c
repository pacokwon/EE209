#include "list.h"
#include <stdio.h>

int main(void) {
    struct Table *table = table_create();

    table_add(table, "hello", 3);
    table_add(table, "world", 5);
    table_add(table, "foo", 1);
    table_add(table, "world", 10);
    table_add(table, "world", 9);
    table_add(table, "hello", 30);

    table_print(table);

    int result;
    if (table_search(table, "foo", &result))
        printf("foo is in the list. Its value is %d\n", result);

    if (!table_search(table, "bar", &result))
        printf("bar is not in the list\n");

    printf("Reversing table...\n");
    table_reverse(table);
    table_print(table);

    printf("Updating values of \"world\" to 7...\n");
    table_update(table, "world", 7);
    table_print(table);

    printf("Deleting occurrences of \"hello\"...\n");
    table_delete(table, "hello");
    table_print(table);

    printf("Deleting occurrences of \"world\"...\n");
    table_delete(table, "world");
    table_print(table);

    printf("Reversing table...\n");
    table_reverse(table);
    table_print(table);

    printf("Adding \"bar\" with value 10...\n");
    table_add(table, "bar", 10);
    table_print(table);

    printf("Reversing table...\n");
    table_reverse(table);
    table_print(table);

    table_free(table);
    return 0;
}

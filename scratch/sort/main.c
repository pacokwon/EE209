#include "sort.h"
#include <stdio.h>

int main(void) {
    int x[] = {9, 10, 2, 6, 5, 7};
    merge_sort(x, 6);

    for (int i = 0; i < 6; i++)
        printf("%d ", x[i]);
    printf("\n");

    return 0;
}

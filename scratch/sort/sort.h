#ifndef __SORT_H
#define __SORT_H

#include <stdio.h>

typedef int (*pivotfunc) (int, int);

void quick_sort(int *, int, pivotfunc);
void merge_sort(int *, int);

int middle_pivot(int, int);
int leftmost_pivot(int, int);

#endif

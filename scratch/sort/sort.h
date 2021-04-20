#ifndef __SORT_H
#define __SORT_H

#include <stdio.h>

typedef int (*pivotfunc) (int, int);

void quicksort(int *, int, pivotfunc);
void quicksort_helper(int *, int, int, pivotfunc);

int middle_pivot(int, int);
int leftmost_pivot(int, int);

#endif

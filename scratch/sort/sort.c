#include "sort.h"

static void swap(int *arr, int idx1, int idx2) {
    int tmp = arr[idx1];
    arr[idx1] = arr[idx2];
    arr[idx2] = tmp;
}

int middle_pivot(int left, int right) {
    return left + (right - left) / 2;
}

int leftmost_pivot(int left, int right) {
    return left;
}

void quicksort_helper(int *array, int left, int right, pivotfunc func) {
    if (left >= right) return;

    int mid = func(left, right);
    swap(array, left, mid);

    int l = left + 1;
    int r = right;

    while (l <= r) {
        while (l <= right && array[l] <= array[left]) l++;
        while (r > left && array[r] >= array[left]) r--;
        if (l > r) break;

        swap(array, l, r);
    }

    swap(array, left, r);

    quicksort_helper(array, left, r - 1, func);
    quicksort_helper(array, r + 1, right, func);
}

void quicksort(int *array, int length, pivotfunc func) {
    quicksort_helper(array, 0, length - 1, func);
}

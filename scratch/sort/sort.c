#include "sort.h"
#include <stdlib.h>

static void swap(int *, int, int);
static void quick_sort_helper(int *, int, int, pivotfunc);
static void merge_sort_helper(int *, int, int);
static void merge(int *, int, int, int);

int middle_pivot(int left, int right) {
    return left + (right - left) / 2;
}

int leftmost_pivot(int left, int right) {
    return left;
}

void quick_sort(int *array, int length, pivotfunc func) {
    quick_sort_helper(array, 0, length - 1, func);
}

void merge_sort(int *array, int length) {
    merge_sort_helper(array, 0, length - 1);
}

static void quick_sort_helper(int *array, int left, int right, pivotfunc func) {
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

    quick_sort_helper(array, left, r - 1, func);
    quick_sort_helper(array, r + 1, right, func);
}

static void merge_sort_helper(int *array, int left, int right) {
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    merge_sort_helper(array, left, mid);
    merge_sort_helper(array, mid + 1, right);
    merge(array, left, mid, right);
}

static void merge(int *array, int left, int mid, int right) {
    int ltrav = left;
    int rtrav = mid + 1;
    int length = right - left + 1;
    int trav = 0;
    int *storage = malloc(length * sizeof (int));
    if (storage == NULL) return;

    while (ltrav <= mid && rtrav <= right) {
        if (array[ltrav] < array[rtrav])
            storage[trav++] = array[ltrav++];
        else
            storage[trav++] = array[rtrav++];
    }

    while (ltrav <= mid)
        storage[trav++] = array[ltrav++];

    while (rtrav <= right)
        storage[trav++] = array[rtrav++];

    for (int i = 0; i < length; i++)
        array[left + i] = storage[i];

    free(storage);
}

static void swap(int *array, int idx1, int idx2) {
    int tmp = array[idx1];
    array[idx1] = array[idx2];
    array[idx2] = tmp;
}

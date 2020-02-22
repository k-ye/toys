#ifndef SORT_HPP
#define SORT_HPP

#include "header.hpp"
#include "heap.hpp"

// Average & Worst: O(n^2)
// Stable sort
template <typename T>
void insertion_sort(std::vector<T>& A, int begin, int end)
{
    for (int i = begin; i < end; ++i)
    {
        const T key = A[i];
        int j = i - 1;
        // uses `<` s.t. the sort is stable
        while (begin <= j && key < A[j])
        {
            A[j + 1] = A[j];
            --j;
        }
        A[j + 1] = key;
    }
}

// Average & Worst: O(nlgn)
// Stable sort
// Needs auxiliary memory
template <typename T>
void merge_sort(std::vector<T>& A, int begin, int end)
{
    int sz = end - begin, mid = begin + (sz >> 1);
    if (sz <= 1)
        return;

    // divide
    merge_sort(A, begin, mid);
    merge_sort(A, mid, end);

    // merge
    std::vector<T> buf1(A.begin() + begin, A.begin() + mid);
    std::vector<T> buf2(A.begin() + mid, A.begin() + end);
    int i = 0, j = 0, sz1 = buf1.size(), sz2 = buf2.size(), k = begin;

    while (i < sz1 && j < sz2)
    {
        // use `<=` s.t. the sort is stable
        if (buf1[i] <= buf2[j])
            A[k++] = buf1[i++];
        else
            A[k++] = buf2[j++];
    }
    while (i < sz1)
        A[k++] = buf1[i++];
    while (j < sz2)
        A[k++] = buf2[j++];

}

// O(n)
template <typename T>
int partition(std::vector<T>& A, int begin, int end, int idx)
{
    swap(A[idx], A[end - 1]);

    idx = end - 1;
    const T& pivot = A[idx];

    int next = begin, i = begin;
    // NOT `i < end`, it is `i < end - 1`.
    // Do not step on the index that holds @pivot.
    // invariant: A[begin..next) <= @pivot
    while (i < idx)
    {
        if (A[i] <= pivot)
            swap(A[next++], A[i]);
        ++i;
    }
    // post condition:
    // - A[begin..next):        <= @pivot
    // - A[next]:               >= @pivot, it can be @pivot itself, if @next == @end - 1.
    // - A[(next + 1)..end):    >  @pivot
    swap(A[next], A[idx]);
    return next;
}

// Average: O(nlgn)
// Worst: O(n^2)
// Unstable sort
// In-place sort
template <typename T>
void quick_sort(std::vector<T>& A, int begin, int end)
{
    if (begin >= end)
        return;

    int p = partition(A, begin, end, end - 1);
    quick_sort(A, begin, p);
    quick_sort(A, p + 1, end);
}

template <typename T>
void quick_sort_3way(std::vector<T>& A, int begin, int end)
{
    if (begin >= end)
        return;

    int eq_begin = begin, i = eq_begin + 1, eq_end = end;
    T pivot = A[eq_begin];
    // invariant: A[eq_begin] == pivot (not necessarily the same element b/c of duplication)
    while (i < eq_end)
    {
        if (A[i] < pivot)
            swap(A[eq_begin++], A[i++]);
        else if (pivot < A[i])
            swap(A[i], A[--eq_end]);
        else
            ++i;
    }
    // post condition:
    // - A[begin..eq_begin):      < pivot
    // - A[eq_begin..eq_end):     == pivot
    // - A[eq_end..end):          > pivot
    quick_sort_3way(A, begin, eq_begin);
    quick_sort_3way(A, eq_end, end);
}

// Average & Worst: O(nlgn)
// Unstable sort
// In-place sort, although the implementation uses auxiliary buffer.
template <typename T>
void heap_sort(std::vector<T>& A, int begin, int end)
{
    std::vector<T> heap(1); // empty head to occupy heap[0]
    heap.insert(heap.end(), A.begin() + begin, A.begin() + end);

    auto _less = [&](int i, int j) { return heap[i] < heap[j]; };
    auto _swap = [&](int i, int j)
    {
        using namespace std;
        swap(heap[i], heap[j]);
    };

    // build heap is O(n)
    int sz = end - begin;
    for (int i = (sz >> 1); 1 <= i; --i)
        sink(i, sz, _less, _swap);

    // heap sort is O(nlgn)
    for (int i = end - 1; begin <= i; --i)
    {
        A[i] = heap[1];
        _swap(1, sz--);
        sink(1, sz, _less, _swap);
    }
}

#endif
#ifndef HEADER_HPP
#define HEADER_HPP

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <deque>
#include <queue>
#include <stack>
#include <algorithm>
#include <cassert>

#include <iterator>
#include <type_traits>

using namespace std;

template <typename C>
void print(const C& container)
{
    for (const auto& i : container)
        cout << i << ", ";
    cout << endl;
}

template <typename C>
void print(const C& container, int begin, int end)
{
    for (int i = begin; i < end; ++i)
        cout << container[i] << ", ";
    cout << endl;
}

template <typename T>
void print(const T* container, int begin, int end)
{
    for (int i = begin; i < end; ++i)
        cout << container[i] << ", ";
    cout << endl;
}

template <typename C>
void print(const C& container, int begin)
{
    print(container, begin, container.size());
}

template <typename T>
void print2d(const T& grid)
{
    for (unsigned i = 0; i < grid.size(); ++i)
    {
        for (unsigned j = 0; j < grid[0].size(); ++j)
        {
            cout << grid[i][j] << ", ";
        }
        cout << endl;
    }
}


template <typename T>
using is_random_access = std::is_same<typename std::iterator_traits<T>::iterator_category, std::random_access_iterator_tag>;

template <typename Iter, typename T, typename = typename std::enable_if<is_random_access<Iter>::value>::type>
Iter _lower_bound_(Iter begin, Iter end, T val)
{
    typedef typename std::iterator_traits<Iter>::difference_type distance_t;
    distance_t len = end - begin;
    distance_t half;
    Iter middle;

    while (len)
    {
        half = (len >> 1);
        middle = begin + half;

        if (*middle < val)
        {
            begin = middle + 1;
            len = len - half - 1;
        }
        else
        {
            len = half;
        }
    }
    return begin;
}

template <typename Iter, typename T, typename = typename std::enable_if<is_random_access<Iter>::value>::type>
Iter _upper_bound_(Iter begin, Iter end, T val)
{
    typedef typename std::iterator_traits<Iter>::difference_type distance_t;
    distance_t len = end - begin;
    distance_t half;
    Iter middle;

    while (len)
    {
        half = (len >> 1);
        middle = begin + half;

        if (*middle <= val)
        {
            begin = middle + 1;
            len = len - half - 1;
        }
        else
        {
            len = half;
        }
    }
    return begin;
}

#endif
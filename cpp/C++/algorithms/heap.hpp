#ifndef HEAP_HPP
#define HEAP_HPP

#include "header.hpp"

#define HP_PARENT(i) ((i) >> 1)
#define HP_LEFT(i) ((i) << 1)
#define HP_RIGHT(i) (((i) << 1) + 1)

template <typename LESS, typename SWAP>
void swim(int i, LESS&& _less, SWAP&& _swap)
{
    int p = HP_PARENT(i);
    while (1 < i && _less(p, i))
    {
        _swap(p, i);
        i = p;
        p = HP_PARENT(i);
    }
}

template <typename LESS, typename SWAP>
void sink(int i, int N, LESS&& _less, SWAP&& _swap)
{
    int c = HP_LEFT(i);
    while (c <= N) // @c can be @N, henc `<=`
    {
        if (c < N && _less(c, c + 1))
            ++c;
        if (!_less(i, c))
            break;

        _swap(i, c);
        i = c;
        c = HP_LEFT(i);
    }
}


template <typename T, typename CMP = std::less<T>>
class Heap
{
    // Heap property: 
    // for every node i, other than the root (1): _cmp(A[i], _A[parent(i)]) == true
    //
    // Root index is 1! (NOT 0). If it was 0, then left(0) == 0, cannot distinguish.
    // index range: [1, _N], both end inclusive!
public:
    typedef T value_t;

    Heap() : _A(1), _N(0),_less(this), _swap(this) { }
    Heap(const CMP& cmp) : _A(1), _N(0), _cmp(cmp), _less(this), _swap(this) { }

    int size() const { return _N; }
    bool empty() const { return _N == 0; }

    void push(const value_t& val)
    {
        _A.push_back(val);
        ++_N;
        swim(_N, _less, _swap);
    }

    value_t& top() { return _A[1]; }

    value_t pop()
    {
        value_t result = top();
        _swap(1, _N--);
        _A.pop_back();
        sink(1, _N, _less, _swap);
        return result;
    }

private:
    class Lesser
    {
    public:
        Lesser(Heap* h) : _h(h) { }
        bool operator()(int i, int j) const
        {
            return _h->_cmp(_h->_A[i], _h->_A[j]);
        }
    private:
        Heap* _h;
    };

    class Swapper
    {
    public:
        Swapper(Heap* h) : _h(h) { }
        void operator()(int i, int j)
        {
            using namespace std;
            swap(_h->_A[i], _h->_A[j]);
        }

    private:
        Heap* _h;
    };

    friend class Lesser;
    friend class Swapper;

private:
    std::vector<value_t> _A;
    int _N;

    CMP _cmp;
    Lesser _less;
    Swapper _swap;

};

template <typename T, typename CMP = std::less<T>>
class IndexedHeap
{
public:
    typedef int key_t;
    typedef T value_t;

    IndexedHeap() : _A(1), _N(0),_less(this), _swap(this) { }
    IndexedHeap(const CMP& cmp) : _A(1), _N(0), _cmp(cmp), _less(this), _swap(this) { }

    int size() const { return _N; }
    bool empty() const { return _N == 0; }
    key_t top_key() const { return _A[1]; }

    bool contains(key_t k) const
    {
        assert (_k2ai.count(k) == _k2v.count(k));
        return _k2v.count(k) > 0;
    }

    bool push(key_t k, const value_t& val)
    {
        if (contains(k))
            return false;

        _A.push_back(k);
        ++_N;
        _k2ai[k] = _N;
        _k2v[k] = val;
        swim(_N, _less, _swap);
        return true;
    }

    value_t& top() { return _k2v.at(top_key()); }

    key_t pop()
    {
        key_t pkey = top_key();
        remove(pkey);
        // _swap(1, _N--);
        // _A.pop_back();
        // _k2ai.erase(pkey);
        // _k2v.erase(pkey);
        // sink(1, _N, _less, _swap);

        return pkey;
    }

    bool remove(key_t k)
    {
        if (!contains(k))
            return false;

        int kidx = _k2ai.at(k);

        _swap(kidx, _N--);
        _A.pop_back();
        _k2ai.erase(k);
        _k2v.erase(k);
        sink(kidx, _N, _less, _swap);

        return true;
    }

    bool change(key_t k, const value_t& val)
    {
        if (!contains(k))
            return false;

        _k2v[k] = val;
        int kidx = _k2ai.at(k);
        sink(kidx, _N, _less, _swap);
        swim(kidx, _less, _swap);
        return true;
    }

private:
    class Lesser
    {
    public:
        Lesser(IndexedHeap* h) : _h(h) { }
        bool operator()(int i, int j) const
        {
            const auto& l = _h->_k2v.at(_h->_A[i]);
            const auto& r = _h->_k2v.at(_h->_A[j]);
            return _h->_cmp(l, r);
        }
    private:
        IndexedHeap* _h;
    };

    class Swapper
    {
    public:
        Swapper(IndexedHeap* h) : _h(h) { }
        void operator()(int i, int j)
        {
            using namespace std;

            auto ki = _h->_A[i], kj = _h->_A[j];
            _h->_k2ai[ki] = j;
            _h->_k2ai[kj] = i;
            // _h->_A[j] = ki;
            // _h->_A[i] = kj;
            swap(_h->_A[i], _h->_A[j]);

            assert(_h->_k2ai.at(_h->_A[i]) == i && _h->_A[_h->_k2ai.at(ki)] == ki);
            assert(_h->_k2ai.at(_h->_A[j]) == j && _h->_A[_h->_k2ai.at(kj)] == kj);
        }

    private:
        IndexedHeap* _h;
    };

    friend class Lesser;
    friend class Swapper;

private:
    std::vector<key_t> _A;                    // index in @_A -> key
    std::unordered_map<key_t, int> _k2ai;     // key -> index in @_A
    std::unordered_map<key_t, value_t> _k2v;  // key -> val
    int _N;

    CMP _cmp;
    Lesser _less;
    Swapper _swap;
};

#undef HP_PARENT
#undef HP_LEFT
#undef HP_RIGHT

#endif
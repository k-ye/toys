#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <vector>
#include <queue>
#include <stack>
#include <iostream>
#include <cassert>
#include <algorithm>

namespace graph
{
namespace undirected
{

class UGraph
{
public:
    // Undirected Graph
    //
    // Adjacency-list representation
    // Space: O(V + E)
    //
    // Vertices are initialized at constructor time and fixed. 
    typedef std::vector<int> adj_iter_t;

    UGraph(int V = 0) : _V(V), _E(0), _adj_list(V) { }

    inline int V() const { return _V; }
    inline int E() const { return _E; }

    void add_edge(int v, int w) 
    {
        _adj_list[v].push_back(w);
        _adj_list[w].push_back(v);
        ++_E;
    }

    inline const adj_iter_t& adjacent(int v) const { return _adj_list[v]; }

private:
    int _V;
    int _E;
    std::vector<adj_iter_t> _adj_list;
};

class ConnectedComponents
{
public:
    // Time complexity: O(|V| + |E|)
    //
    // Suitable for undirected graph

    typedef UGraph graph_t;
    
    void search(const graph_t& graph)
    {
        _count = 0;
        _ids = std::vector<int>(graph.V(), -1);

        for (int v = 0; v < graph.V(); ++v)
        {
            if (_ids[v] == -1)
            {
                dfs(graph, v);
                ++_count;
            }
        }
    }

    bool connected(int v, int w) const 
    {
        return _ids[v] == _ids[w];
    }

    int id(int v) const { return _ids[v]; }
    int num_cc() const { return _count; }

private:
    void dfs(const graph_t& graph, int v)
    {
        _ids[v] = _count;
        for (int w : graph.adjacent(v))
        {
            if (_ids[w] == -1)
                dfs(graph, w);
        }
    }

    int _count;
    std::vector<int> _ids;
};

class UCycle
{
public:
    typedef UGraph graph_t;

    void search(const graph_t& graph)
    {
        _has_cycle = false;
        _marked = std::vector<bool>(graph.V(), false);

        for (int s = 0; s < graph.V(); ++s)
        {
            if (!_marked[s])
                dfs(graph, s, s);

            if (_has_cycle) return;
        }
    }

    bool has_cycle() const { return _has_cycle; }

private:
    void dfs(const graph_t& graph, int v, int p)
    {
        // @p is the predecessor of @v
        _marked[v] = true;

        for (int w : graph.adjacent(v))
        {
            if (!_marked[w])
                dfs(graph, w, v);
            else if (w != p)
            {
                _has_cycle = true;
                return;
            }
        }
    }

    bool _has_cycle;
    std::vector<bool> _marked;
};

}; // namespace graph::undirected

namespace directed
{

class DGraph
{
public:
    typedef std::vector<int> adj_iter_t;
    DGraph(int V = 0) : _V(V), _E(0), _adj_list(V) { }

    inline int V() const { return _V; }
    inline int E() const { return _E; }

    void add_edge(int v, int w) 
    {
        _adj_list[v].push_back(w);
        ++_E;
    }

    inline const adj_iter_t& adjacent(int v) const { return _adj_list[v]; }

    DGraph reverse() const
    {
        DGraph result(_V);
        for (int v = 0; v < _V; ++v)
        {
            for (int w : adjacent(v))
                result.add_edge(w, v);
        }
        return result;
    }
private:
    int _V;
    int _E;
    std::vector<adj_iter_t> _adj_list;

};

class DCycle
{
private:
    constexpr static int _UNMARKED = 0;
    constexpr static int _ON_STACK = 1;
    constexpr static int _MARKED = 2;

public:
    typedef DGraph graph_t;

    void search(const graph_t& graph)
    {
        _has_cycle = false;
        _status = std::vector<int>(graph.V(), _UNMARKED);

        for (int v = 0; v < graph.V(); ++v)
        {
            if (_status[v] == _UNMARKED)
                dfs(graph, v);

            if (_has_cycle) return;       
        }
    }

    bool has_cycle() const { return _has_cycle; }

private:
    void dfs(const graph_t& graph, int v)
    {
        _status[v] = _ON_STACK;
        for (int w : graph.adjacent(v))
        {
            if (_status[w] == _UNMARKED)
                dfs(graph, w);
            else if (_status[w] == _ON_STACK)
            {
                _has_cycle = true;
                return;
            }
        }
        _status[v] = _MARKED;
    }

    bool _has_cycle;
    std::vector<int> _status;
};

class TopologicalSort
{
public:
    typedef DGraph graph_t;
    typedef std::stack<int> result_t;
    // Topological Sort
    //
    // Time complexity: O(|V| + |E|)
    
    result_t sort(const graph_t& graph) 
    {
        _marked = std::vector<bool>(graph.V(), false);
        _result = result_t();

        DCycle dc;
        dc.search(graph);
        
        if (!dc.has_cycle())
        {
            for (int v = 0; v < graph.V(); ++v)
            {
                if (!_marked[v])
                    dfs(graph, v);
            }
        }

        return _result;
    }

private:
    void dfs(const graph_t& graph, int v)
    {
        _marked[v] = true;
        
        for (int w : graph.adjacent(v))
        {
            if (!_marked[w])
                dfs(graph, w);
        }
        _result.push(v);
    }

    std::vector<bool> _marked;
    result_t _result;
};

}; // namespace graph::directed

template <typename G>
class DFS
{
public:
    typedef G graph_t;

    // Single source problem
    //
    // Time complexity: O(|V| + |E|)
    void traverse(const graph_t& graph) 
    {
        _marked = std::vector<bool>(graph.V(), false);
        for (int u = 0; u < graph.V(); ++u)
        {
            if (!_marked[u])
                dfs(graph, u);
        }
    }

private:
    void dfs(const graph_t& graph, int v)
    {
        _marked[v] = true;
        for (int w : graph.adjacent(v))
        {
            if (!_marked[w])
                dfs(graph, w);
        }
    }

    std::vector<bool> _marked;
};

template <typename G>
class DFSPaths
{
public:
    typedef G graph_t;

    // Single source problem
    //
    // Time complexity: O(|V| + |E|)
    
    void search(const graph_t& graph, int src) 
    {
        _src = src;
        _marked = std::vector<bool>(graph.V(), false);
        _edge_to = std::vector<int>(graph.V(), -1);
        _count = 0;

        dfs(graph, src);
    }

    inline bool reachable(int v) const { return _marked[v]; }
    inline int num_reachable(int v) const { return _count; }

    std::vector<int> path_to(int v) const
    {
        assert(reachable(v));
        std::vector<int> result;

        while (v != _src)
        {
            result.push_back(v);
            v = _edge_to[v];
        }
        result.push_back(_src);

        reverse(result.begin(), result.end());
        return result;
    }

private:
    void dfs(const graph_t& graph, int v)
    {
        _marked[v] = true;
        ++_count;
        for (int w : graph.adjacent(v))
        {
            if (!_marked[w])
            {
                _edge_to[w] = v;
                dfs(graph, w);
            }
        }
    }

    int _src;
    std::vector<bool> _marked;
    std::vector<int> _edge_to;  // a tree rooted at @_src
    int _count;
};

template <typename G>
class BFSPaths
{
public:
    typedef G graph_t;

    // Single source problem
    //
    // Time complexity: O(|V| + |E|)
    //
    // Suitable for directed/undirected, unweighted graph

    void search(const graph_t& graph, int src) 
    {
        _src = src;
        _marked = std::vector<bool>(graph.V(), false);
        _edge_to = std::vector<int>(graph.V(), -1);
        _count = 0;

        std::queue<int> frontier;
        frontier.push(src);
        _marked[src] = true;

        while (!frontier.empty())
        {
            int v = frontier.front();
            frontier.pop();
            ++_count;

            for (int w : graph.adjacent(v))
            {
                if (!_marked[w])
                {
                    _edge_to[w] = v;
                    _marked[w] = true;
                    frontier.push(w);
                }
            }
        }

    }

    inline bool reachable(int v) const { return _marked[v]; }
    inline int num_reachable(int v) const { return _count; }

    std::vector<int> path_to(int v) const
    {
        assert(reachable(v));
        std::vector<int> result;

        while (v != _src)
        {
            result.push_back(v);
            v = _edge_to[v];
        }
        result.push_back(_src);

        reverse(result.begin(), result.end());
        return result;
    }

private:
    int _src;
    std::vector<bool> _marked;
    std::vector<int> _edge_to;  // a tree rooted at @_src
    int _count;
};

}; // namespace graph

#endif
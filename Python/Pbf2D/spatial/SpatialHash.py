import os, sys
import numpy as np

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from debug.dassert import *
import AABB.AABB as AABB_m

_NULL_NODE = (1 << 32)

class _Node(object):
    def __init__(self):
        self.reset()

    def reset(self):
        self._data = None
        self._hash_ij = None
        self.next_free = _NULL_NODE        

class SpatialHash(object):
    def __init__(self, cell_size):
        self._cell_size = 0.0 # definition, maybe unnecessary
        self.cell_size = cell_size
        
        self._hashmap = {}

        self._capacity = 64
        self._num_nodes = 0

        self._nodes = [_Node() for _ in xrange(self._capacity)]
        for i in xrange(self._capacity - 1):
            self._nodes[i].next_free = i + 1
        self._nodes[-1].next_free = _NULL_NODE

        self._free_list = 0

    @property
    def cell_size(self):
        return self._cell_size

    @cell_size.setter
    def cell_size(self, value):
        self._cell_size = value
        self._one_over_cell_size = 1.0 / value

    @property
    def num_nodes(self):
        return self._num_nodes

    def get_node(self, index):
        return self._nodes[index]

    def _allocate(self):
        if self._free_list == _NULL_NODE:
            dassert(self._num_nodes == self._capacity)
            self._capacity *= 2

            append_nodes = [_Node() for i in xrange(self._num_nodes, self._capacity)]
            for i in xrange(len(append_nodes) - 1):
                append_nodes[i].next_free = i + self._num_nodes + 1
            append_nodes[-1].next_free = _NULL_NODE

            self._nodes += append_nodes
            self._free_list = self._num_nodes

        new_index = self._free_list
        self._free_list = self._nodes[new_index].next_free

        self._nodes[new_index].reset()        
        self._num_nodes += 1

        return new_index

    def _deallocate(self, index):
        dassert(0 < self._num_nodes)
        
        self._nodes[index].reset()
        
        self._nodes[index].next_free = self._free_list
        self._free_list = index

        self._num_nodes -= 1

    def add(self, data):
        new_index = self._allocate()

        pt = data.point()
        hash_ij = self._hash_point(pt)

        self._nodes[new_index]._data = data
        self._nodes[new_index]._hash_ij = hash_ij

        if hash_ij not in self._hashmap:
            self._hashmap[hash_ij] = set()

        dassert(new_index not in self._hashmap[hash_ij])
        self._hashmap[hash_ij].add(new_index)

        return new_index

    def remove(self, index):
        dassert(0 <= index and index < self._num_nodes)

        old_hash_ij = self._nodes[index]._hash_ij
        dassert(index in self._hashmap[old_hash_ij])
        self._hashmap[old_hash_ij].remove(index)

        self._deallocate(index)

    def update(self, index):
        dassert(0 <= index and index < self._num_nodes)

        data = self._nodes[index]._data
        self.remove(index)
        # implementation depedent!
        new_index = self.add(data)
        dassert(index == new_index)

        return True

    def query(self, pt1, pt2):
        for d in xrange(AABB_m._NDIM):
            pt1[d], pt2[d] = min(pt1[d], pt2[d]), max(pt1[d], pt2[d])

        aabb = AABB_m.AABB()
        aabb.min = np.array(pt1, copy=True)
        aabb.max = np.array(pt2, copy=True)

        return self.query_aabb(aabb)

    def query_aabb(self, aabb):
        result = []

        min_i, min_j = self._hash_point(aabb.min)
        max_i, max_j = self._hash_point(aabb.max)

        for iter_j in xrange(min_j, max_j + 1):
            for iter_i in xrange(min_i, max_i + 1):
                hash_ij = (iter_i, iter_j)

                if hash_ij in self._hashmap:
                    for index in self._hashmap[hash_ij]:
                        pt = self._nodes[index]._data.point()
                        if aabb.contains_point(pt):
                            result.append(index)

        return result

    def _hash_point(self, pt):
        i = int(pt[0] * self._one_over_cell_size)
        j = int(pt[1] * self._one_over_cell_size)

        return (i, j)
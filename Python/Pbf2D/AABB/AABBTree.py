import os, sys
import numpy as np

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from debug.dassert import *
import AABB as AABB_m
from InsertPolicy import *

_NULL_NODE = (1 << 32)

class AABBTreeNode(object):
    def __init__(self):
        self.reset()

    @property
    def next_free(self):
        dassert(self._height == -1)
        return self.left

    @next_free.setter
    def next_free(self, value):
        dassert(self._height == -1)
        self.left = value

    @property
    def is_leaf(self):
        # return self.left == _NULL_NODE and self.right == _NULL_NODE and self._height == 0
        return self._height == 0

    @property
    def height(self):
        return self._height
    
    def reset(self):
        # 0: leaf, -1: unused
        self._height = -1
        # flat aabb
        self._aabb = AABB_m.AABB()
        # user data
        self._data = None

        self.parent = _NULL_NODE
        self.left = _NULL_NODE
        self.right = _NULL_NODE

    def __str__(self):
        result = 'AABBTreeNode <{}>: \n'.format(id(self))
        result += '  _height: {} \n'.format(self._height)
        result += '  _aabb: {} \n'.format(self._aabb)
        if self._data is not None:
            result += '  _data.aabb: {} \n'.format(self._data.aabb())
        result += '  parent: {}, left: {}, right: {}'.format(self.parent, self.left, self.right)
        return result

class AABBTree(object):
    def __init__(self, flat_delta, policy=InsertPolicyArea()):
        self._root = _NULL_NODE

        self._capacity = 64
        self._num_nodes = 0

        self._nodes = [AABBTreeNode() for i in xrange(self._capacity)]
        for i in xrange(self._capacity - 1):
            self._nodes[i].next_free = i + 1
        self._nodes[-1].next_free = _NULL_NODE
        self._leaves = set()
        self._free_list = 0

        self._flat_delta = flat_delta
        self._policy = policy

    @property
    def num_nodes(self):
        return self._num_nodes

    @property
    def num_leaves(self):
        return len(self._leaves)

    def node_indices(self):
        for i in xrange(self._num_nodes):
            yield i

    def leaf_indices(self):
        for i in self._leaves:
            yield i

    def get_node(self, index):
        return self._nodes[index]

    def get_leaf(self, index):
        dassert(self._expect_leaf(index))
        return self._nodes[index]

    def _allocate(self):
        if self._free_list == _NULL_NODE:
            dassert(self._num_nodes == self._capacity)
            self._capacity *= 2

            append_nodes = [AABBTreeNode() for i in xrange(self._num_nodes, self._capacity)]
            for i in xrange(len(append_nodes) - 1):
                append_nodes[i].next_free = i + self._num_nodes + 1
            append_nodes[-1].next_free = _NULL_NODE

            self._nodes += append_nodes
            self._free_list = self._num_nodes

        new_index = self._free_list
        self._free_list = self._nodes[new_index].next_free

        self._nodes[new_index].reset()
        # leaf node height is always 0
        self._nodes[new_index]._height = 0
        
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

        self._nodes[new_index]._aabb = data.aabb().clone()
        self._nodes[new_index]._aabb.inflate(self._flat_delta, self._flat_delta)
        self._nodes[new_index]._data = data

        self._insert_leaf(new_index)

        return new_index

    def remove(self, index):
        self._remove_leaf(index)

    def update(self, leaf):
        dassert(self._expect_leaf(leaf))

        data_aabb = self._nodes[leaf]._data.aabb()
        if self._nodes[leaf]._aabb.contains(data_aabb):
            return False

        data = self._nodes[leaf]._data
        self.remove(leaf)
        self.add(data)

        return True

    def query(self, pt1, pt2, strictly_contained=False):        
        if self._root == _NULL_NODE:
            return []
        
        for d in xrange(AABB_m._NDIM):
            pt1[d], pt2[d] = min(pt1[d], pt2[d]), max(pt1[d], pt2[d])

        aabb = AABB_m.AABB()
        aabb.min = np.array(pt1, copy=True)
        aabb.max = np.array(pt2, copy=True)

        return self.query_aabb(aabb, strictly_contained)

    def query_aabb(self, aabb, strictly_contained=False):
        result = []

        if self._root == _NULL_NODE:
            return result

        stack = [None] * 32
        stack[0] = self._root
        stack_ptr = 1

        while stack_ptr:
            index = stack[stack_ptr - 1]
            stack_ptr -= 1
            # del stack[-1]

            if index == _NULL_NODE:
                continue
            node = self._nodes[index]

            # tree-pruning condition
            if aabb.overlaps(node._aabb):
                if node.is_leaf and (not strictly_contained or aabb.contains(node._aabb)):
                    result.append(index)
                else:
                    # stack.append(node.right)
                    # stack.append(node.left)
                    if stack_ptr + 2 > len(stack):
                        extend_size = max(32, len(stack))
                        stack += [None for _ in xrange(extend_size)]
                    stack[stack_ptr] = node.right
                    stack_ptr += 1
                    stack[stack_ptr] = node.left
                    stack_ptr += 1

        return result

    def _expect_leaf(self, index):
        return self._nodes[index].is_leaf and index in self._leaves

    def _insert_leaf(self, leaf):
        dassert(leaf not in self._leaves)
        self._leaves.add(leaf)
        # redundant
        self._nodes[leaf]._height = 0

        if self._root == _NULL_NODE:
            self._root = leaf
            self._nodes[self._root].parent = _NULL_NODE
            return

        leaf_aabb = self._nodes[leaf]._aabb
        index = self._root

        while not self._nodes[index].is_leaf:
            selected = self._policy.choose(leaf_aabb, index, self._nodes)

            if selected == InsertPolicy.PARENT:
                break
            elif selected == InsertPolicy.LEFT:
                index = self._nodes[index].left
            else:
                dassert(selected == InsertPolicy.RIGHT)
                index = self._nodes[index].right

        sibling = index
        old_parent = self._nodes[sibling].parent
        new_parent = self._allocate()

        self._nodes[new_parent].parent = old_parent
        self._nodes[new_parent]._aabb.as_union_of(leaf_aabb, self._nodes[sibling]._aabb)
        self._nodes[new_parent]._height = self._nodes[sibling]._height + 1

        if old_parent != _NULL_NODE:
            # sibling is not the root
            if self._nodes[old_parent].left == sibling:
                self._nodes[old_parent].left = new_parent
            else:
                dassert(self._nodes[old_parent].right == sibling)
                self._nodes[old_parent].right = new_parent

        else:
            # sibling is the root
            dassert(sibling == self._root)
            self._root = new_parent

        self._nodes[new_parent].left = sibling
        self._nodes[new_parent].right = leaf
        self._nodes[sibling].parent = new_parent
        self._nodes[leaf].parent = new_parent

        index = self._nodes[leaf].parent
        # re-balance the tree
        while index != _NULL_NODE:
            index = self._balance(index)
            left = self._nodes[index].left
            right = self._nodes[index].right

            dassert(left != _NULL_NODE and right != _NULL_NODE)

            self._nodes[index]._height = 1 + max(self._nodes[left]._height, self._nodes[right]._height)
            self._nodes[index]._aabb.as_union_of(self._nodes[left]._aabb, self._nodes[right]._aabb)

            index = self._nodes[index].parent

        self._validate()
    # END: def _insert_leaf(self, leaf)

    def _remove_leaf(self, leaf):
        dassert(self._expect_leaf(leaf))
        self._leaves.remove(leaf)

        if leaf == self._root:
            self._deallocate(leaf)
            self._root = _NULL_NODE
            return

        parent = self._nodes[leaf].parent
        dassert(parent != _NULL_NODE)
        grand_parent = self._nodes[parent].parent
        sibling = _NULL_NODE

        if self._nodes[parent].left == leaf:
            sibling = self._nodes[parent].right
        else:
            dassert(self._nodes[parent].right == leaf)
            sibling = self._nodes[parent].left

        if grand_parent != _NULL_NODE:
            if self._nodes[grand_parent].left == parent:
                self._nodes[grand_parent].left = sibling
            else:
                self._nodes[grand_parent].right = sibling

            self._nodes[sibling].parent = grand_parent

            index = grand_parent

            while index != _NULL_NODE:
                index = self._balance(index)
                left = self._nodes[index].left
                right = self._nodes[index].right

                dassert(left != _NULL_NODE and right != _NULL_NODE)

                self._nodes[index]._height = 1 + max(self._nodes[left]._height, self._nodes[right]._height)
                self._nodes[index]._aabb.as_union_of(self._nodes[left]._aabb, self._nodes[right]._aabb)

                index = self._nodes[index].parent
        else:
            self._root = sibling
            self._nodes[sibling].parent = _NULL_NODE

        self._deallocate(parent)
        self._deallocate(leaf)

        self._validate()
    # END: def _remove_leaf(self, leaf)

    def _balance(self, iA):
        dassert(iA != _NULL_NODE)

        if self._nodes[iA].is_leaf or self._nodes[iA]._height < 2:
            return iA

        iB = self._nodes[iA].left
        iC = self._nodes[iA].right

        balance = self._nodes[iC]._height - self._nodes[iB]._height

        if balance > 1:
            return self._rotate_left(iA, iB, iC)
        elif balance < -1:
            return self._rotate_right(iA, iB, iC)
        return iA
    # END: def _balance(self, iA)
    
    def _rotate_left(self, iA, iB, iC):
        '''
            A           - parent
          /   \
         B     C        - left, right
             /   \
            F     G
        '''
        iF = self._nodes[iC].left
        iG = self._nodes[iC].right

        # swap A and C
        self._nodes[iC].left = iA
        self._nodes[iC].parent = self._nodes[iA].parent
        self._nodes[iA].parent = iC

        # update the info of A's parent's child to point to C
        iC_parent = self._nodes[iC].parent
        if iC_parent != _NULL_NODE:
            if self._nodes[iC_parent].left == iA:
                self._nodes[iC_parent].left = iC
            else:
                dassert(self._nodes[iC_parent].right == iA)
                self._nodes[iC_parent].right = iC
        else:
            self._root = iC

        # rotate
        if self._nodes[iF]._height > self._nodes[iG]._height:
            '''
                    C
                  /   \
                 A     F
               /   \
              B     G
            '''
            self._nodes[iC].right = iF
            self._nodes[iA].right = iG
            self._nodes[iG].parent = iA

            self._nodes[iA]._aabb.as_union_of(self._nodes[iB]._aabb, self._nodes[iG]._aabb)
            self._nodes[iC]._aabb.as_union_of(self._nodes[iA]._aabb, self._nodes[iF]._aabb)

            self._nodes[iA]._height = 1 + max(self._nodes[iB]._height, self._nodes[iG]._height)
            self._nodes[iC]._height = 1 + max(self._nodes[iA]._height, self._nodes[iF]._height)
        else:
            '''
                    C
                  /   \
                 A     G
               /   \
              B     F
            '''
            self._nodes[iC].right = iG
            self._nodes[iA].right = iF
            self._nodes[iF].parent = iA

            self._nodes[iA]._aabb.as_union_of(self._nodes[iB]._aabb, self._nodes[iF]._aabb)
            self._nodes[iC]._aabb.as_union_of(self._nodes[iA]._aabb, self._nodes[iG]._aabb)

            self._nodes[iA]._height = 1 + max(self._nodes[iB]._height, self._nodes[iF]._height)
            self._nodes[iC]._height = 1 + max(self._nodes[iA]._height, self._nodes[iG]._height)

        return iC
    # END: def _rotate_left(self, iA, iB, iC)

    def _rotate_right(self, iA, iB, iC):
        '''
                A       - parent
              /   \
             B     C    - left, right
           /   \
          D     E
        '''
        iD = self._nodes[iB].left
        iE = self._nodes[iB].right

        # swap A and B
        self._nodes[iB].left = iA
        self._nodes[iB].parent = self._nodes[iA].parent
        self._nodes[iA].parent = iB

        # update the info of A's parent's child to point to B
        iB_parent = self._nodes[iB].parent
        if iB_parent != _NULL_NODE:
            if self._nodes[iB_parent].left == iA:
                self._nodes[iB_parent].left = iB
            else:
                dassert(self._nodes[iB_parent].right == iA)
                self._nodes[iB_parent].right = iB
        else:
            self._root = iB

        # rotate
        if self._nodes[iD]._height > self._nodes[iE]._height:
            '''
                    B
                  /   \
                 A     D
               /   \
              E     C
            '''
            self._nodes[iB].right = iD
            self._nodes[iA].left = iE
            self._nodes[iE].parent = iA

            self._nodes[iA]._aabb.as_union_of(self._nodes[iE]._aabb, self._nodes[iC]._aabb)
            self._nodes[iB]._aabb.as_union_of(self._nodes[iA]._aabb, self._nodes[iD]._aabb)

            self._nodes[iA]._height = 1 + max(self._nodes[iE]._height, self._nodes[iC]._height)
            self._nodes[iB]._height = 1 + max(self._nodes[iA]._height, self._nodes[iD]._height)
        else:
            '''
                    B
                  /   \
                 A     E
               /   \
              D     C
            '''
            self._nodes[iB].right = iE
            self._nodes[iA].left = iD
            self._nodes[iD].parent = iA

            self._nodes[iA]._aabb.as_union_of(self._nodes[iD]._aabb, self._nodes[iC]._aabb)
            self._nodes[iB]._aabb.as_union_of(self._nodes[iA]._aabb, self._nodes[iE]._aabb)

            self._nodes[iA]._height = 1 + max(self._nodes[iD]._height, self._nodes[iC]._height)
            self._nodes[iB]._height = 1 + max(self._nodes[iA]._height, self._nodes[iE]._height)

        return iB
    # END: def _rotate_right(self, iA, iB, iC)

    def _validate_helper(self, index):
        if index == _NULL_NODE:
            return

        if index == self._root:
            dassert(self._nodes[index].parent == _NULL_NODE)

        left = self._nodes[index].left
        right = self._nodes[index].right

        if self._nodes[index].is_leaf:
            dassert(left == _NULL_NODE)
            dassert(right == _NULL_NODE)
            dassert(self._nodes[index]._height == 0)
            return

        dassert(0 <= left and left < self._capacity)
        dassert(0 <= right and right < self._capacity)

        dassert(self._nodes[left].parent == index)
        dassert(self._nodes[right].parent == index)

        lheight = self._nodes[left]._height
        rheight = self._nodes[right]._height
        height = 1 + max(lheight, rheight)
        dassert(self._nodes[index]._height == height)

        test_aabb = get_union(self._nodes[left]._aabb, self._nodes[right]._aabb)
        dassert(test_aabb.equals(self._nodes[index]._aabb))

        self._validate_helper(left)
        self._validate_helper(right)
    # END: def _validate_helper(self, index)

    def _validate(self):
        pass
        # self._dvalidate()

    def _dvalidate(self):
        self._validate_helper(self._root)

        num_free = 0
        free_index = self._free_list
        while free_index != _NULL_NODE:
            dassert(0 <= free_index and free_index < self._capacity)
            num_free += 1
            free_index = self._nodes[free_index].next_free
        dassert(self._num_nodes + num_free == self._capacity)

        dassert(self._get_height(self._root) == self._compute_height(self._root))

    def _compute_height(self, index):
        if index == _NULL_NODE:
            return -1

        if self._nodes[index].is_leaf:
            return 0

        lheight = self._compute_height(self._nodes[index].left)
        rheight = self._compute_height(self._nodes[index].right)
        return 1 + max(lheight, rheight)

    def _get_height(self, index):
        if index == _NULL_NODE:
            return -1

        return self._nodes[index]._height
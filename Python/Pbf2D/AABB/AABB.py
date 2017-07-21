import os, sys
import numpy as np

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from debug.dassert import *

_NDIM = 2
_X, _Y = 0, 1

class AABB(object):

    def __init__(self):
        self.min = np.zeros(2)
        self.max = np.zeros(2)

    @property
    def center(self):
        c = 0.5 * (self.min + self.max)
        return c

    @property
    def perimeter(self):
        return 2 * (self.length(_X) + self.length(_Y))
    
    @property
    def area(self):
        return self.length(_X) * self.length(_Y)

    def length(self, dim):
        return self.max[dim] - self.min[dim]

    def inflate(self, delta_x, delta_y):
        self.min[_X] -= delta_x
        self.min[_Y] -= delta_y
        self.max[_X] += delta_x
        self.max[_Y] += delta_y
        return self

    def clone(self):
        result = AABB()
        result.min = np.copy(self.min)
        result.max = np.copy(self.max)

        return result

    def union(self, other):
        for d in xrange(_NDIM):
            self.min[d] = min(self.min[d], other.min[d])
            self.max[d] = max(self.max[d], other.max[d])

        return self

    def as_union_of(self, lhs, rhs):
        for d in xrange(_NDIM):
            self.min[d] = min(lhs.min[d], rhs.min[d])
            self.max[d] = max(lhs.max[d], rhs.max[d])

        return self

    def equals(self, other, epsilon=1e-10):
        for d in xrange(_NDIM):
            if epsilon < abs(self.min[d] - other.min[d]) or  \
                epsilon < abs(self.max[d] - other.max[d]):
                return False
        return True

    def overlaps(self, other):
        # negative condition
        ncond = (other.max[_X] < self.min[_X] or self.max[_X] < other.min[_X] or \
                other.max[_Y] < self.min[_Y] or self.max[_Y] < other.min[_Y])
        return not ncond

    def contains(self, other):
        cond = (self.min[_X] <= other.min[_X] and other.max[_X] <= self.max[_X])
        cond = (cond and self.min[_Y] <= other.min[_Y] and other.max[_Y] <= self.max[_Y])
        return cond

    def contains_point(self, pt):
        cond = (self.min[_X] <= pt[_X] and pt[_X] <= self.max[_X]) and \
                (self.min[_Y] <= pt[_Y] and pt[_Y] <= self.max[_Y])
        return cond

    def __str__(self):
        result = 'AABB <{}>: (min: {}, max: {})'.format(id(self), self.min, self.max)
        return result

def get_intersection(lhs, rhs):
    if not lhs.overlaps(rhs):
        return None

    result = lhs.clone()

    result.min[_X] = max(result.min[_X], other.min[_X])
    result.max[_X] = min(result.max[_X], other.max[_X])
    result.min[_Y] = max(result.min[_Y], other.min[_Y])
    result.max[_Y] = min(result.max[_Y], other.max[_Y])

    dassert(result.min[_X] <= result.max[_X] and \
        result.min[_Y] <= result.max[_Y])

    return result

def get_union(lhs, rhs):
    result = lhs.clone()
    result.union(rhs)
    return result
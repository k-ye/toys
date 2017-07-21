import numpy as np
import threading as thr

import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import AABB.AABB as AABB_m

# import warnings
# warnings.filterwarnings('error')

_NDIM = AABB_m._NDIM
_EDGESIZE = 35.0

def _dprint(msg):
    pass
    # print msg

def _overlaps(ptc_i, ptc_j):
    R = ptc_i.radius
    x_ji = ptc_j.position - ptc_i.position
    return np.dot(x_ji, x_ji) < 4.0 * R * R

def _moving_apart(ptc_i, ptc_j):
    x_ji = ptc_j.position - ptc_i.position
    v_ji = ptc_j.velocity - ptc_i.velocity
    return np.dot(x_ji, v_ji) > 0.0

# def _compute_collision(ptc_i, ptc_j, impulse=1e-3):
def _compute_collision(ptc_i, ptc_j, impulse=1e-3):
    # https://en.wikipedia.org/wiki/Elastic_collision#Two-dimensional_collision_with_two_moving_objects
    # The masses of all the particles are the same.
    x_ji = ptc_j.position - ptc_i.position
    v_ji = ptc_j.velocity - ptc_i.velocity

    dot_x_ji = np.dot(x_ji, x_ji)
    dot_x_v = np.dot(x_ji, v_ji)

    change_vi = (dot_x_v / dot_x_ji) * x_ji

    ptc_i.velocity += change_vi
    ptc_j.velocity -= change_vi

    # give a slight impulse to help separate them
    # ptc_i.velocity -= impulse * x_ji
    # ptc_j.velocity += impulse * x_ji

def _update_collision(particles, aabbtree, callback=None):
    
    for leaf, ptc_i in particles.iteritems():
        R = ptc_i.radius
        half_R = 0.5 * R
        aabb = ptc_i.aabb()
        # make the query aabb slightly larger is enough
        aabb.inflate(half_R, half_R)

        query_result = aabbtree.query_aabb(aabb)

        for neighbor in query_result:
            if neighbor > leaf:
                ptc_j = particles[neighbor]

                if _overlaps(ptc_i, ptc_j) and not _moving_apart(ptc_i, ptc_j):
                    _compute_collision(ptc_i, ptc_j)

def length(vec):
    result = np.sqrt(np.sum(vec) ** 2)
    return result

def distance(p1, p2):
    result = length(p2 - p1)
    return result

class W_poly6(object):
    def __init__(self, h):
        # self._factor = 315.0 / (64.0 * np.pi)
        self.reset(h)

    def reset(self, h):
        self.h = h
        # self._9logh = 9.0 * np.log(h)
        h2 = h * h
        h4 = h2 * h2
        h9 = h4 * h4 * h

        self._h2 = h2
        self._factor = 315.0 / (64.0 * np.pi) / h9

    def value(self, x):
        r_squared = np.dot(x, x)
        if r_squared >= self._h2:
            return 0

        result = (self._h2 - r_squared) ** 3
        result *= self._factor

        return result
        # log ( (h^2 - r^2)^3 / h^9 ) = 3log(h^2 - r^2) - 9log(h)
        # result = 3.0 * np.log(self._h_squred - r_squared)
        # result -= self._9logh
        # result = np.exp(result) * self._factor

class W_spiky(object):
    def __init__(self, h):
        self._epsilon = 1e-12
        self.reset(h)

    def reset(self, h):
        self.h = h
        self.h2 = h * h
        h3 = h * h * h
        h6 = h3 * h3
        self._factor = 15.0 / np.pi / h6

    '''
    def value(self, x):
        len_x = length(x)
        if len_x >= self.h:
            return 0

        result = (self.h - len_x) ** 3
        result *= self._factor

        return result
    '''

    def gradient(self, x):
        x2 = np.dot(x, x)

        if x2 >= self.h2 or x2 < self._epsilon:
            return np.zeros(_NDIM)

        len_x = np.sqrt(x2)

        g_factor = (self.h - len_x) ** 2
        g_factor *= (-3.0 * self._factor / len_x)
        return g_factor * x

        # return result

_GRAVITY = np.array([0.0, 15.0])

class _PbfParticleRecord(object):
    def __init__(self):
        self.old_position = None
        
        self.neighbors = set()
        
        self.lambda_ = 0.0
        self.delta_p = None

        self.vortocity = None
        self.len_vortocity = 0.0

        self.vel_refined = None

    def add_neighbor(self, n):
        self.neighbors.add(n)

    def clear_neighbors(self):
        self.neighbors = set()

def _create_ptc_records(particles):
    result = {}

    for leaf, ptc in particles.iteritems():
        ptc_record = _PbfParticleRecord()
        ptc_record.old_position = np.copy(ptc.position) # COPY!
        result[leaf] = ptc_record

    return result

def _predicate_positions(particles, aabbtree, delta_t):
    for leaf, ptc in particles.iteritems():
        ptc.velocity += _GRAVITY * delta_t
        old = np.copy(ptc.position)
        ptc.position += ptc.velocity * delta_t

        # _dprint('leaf: {}, old pos: {}, pred pos: {}'.format(leaf, old, ptc.position))

        # update to reflect the tentative new position p^*
        aabbtree.update(leaf)

def _find_neighbors(particles, aabbtree, ptc_records, h, callback=None):
    h2 = h * h
    h_half = 0.5 * h

    for i, ptc_i in particles.iteritems():
        ptc_record = ptc_records[i]
        ptc_record.clear_neighbors()

        query_min = ptc_i.position - h_half
        query_max = ptc_i.position + h_half
        query_result = aabbtree.query(query_min, query_max)

        # add valid neighbors
        for j in query_result:
            # skip self
            if j == i:
                continue

            ptc_j = particles[j]
            pos_ji = ptc_i.position - ptc_j.position
            dist2_ji = np.dot(pos_ji, pos_ji) 

            if dist2_ji < h2:
                # import pdb; pdb.set_trace()
                ptc_record.add_neighbor(j)

        if callback is not None:
            callback(i, ptc_record.neighbors, particles, aabbtree)

def _compute_lambda(particles, ptc_records, w_poly6, w_spiky, one_over_rho_0, epsilon):
    for i, ptc_i in particles.iteritems():
        pos_i = ptc_i.position

        rho_i = 0.0
        grad_C_pk = np.zeros(_NDIM)
        grad_C_pi = np.zeros(_NDIM)
        sum_grad_C_pk = 0.0

        ptc_record = ptc_records[i]
        # import pdb; pdb.set_trace()

        for j in ptc_record.neighbors:
            pos_j = particles[j].position
            pos_ji = pos_i - pos_j
            
            rho_i += w_poly6.value(pos_ji)
            grad_C_pk = w_spiky.gradient(pos_ji) # drop the negative sign '-' is OK
            grad_C_pk *= one_over_rho_0
            sum_grad_C_pk += np.dot(grad_C_pk, grad_C_pk)

            grad_C_pi += grad_C_pk

        sum_grad_C_pk += np.dot(grad_C_pi, grad_C_pi)
        # eq (1)
        C_i = rho_i * one_over_rho_0 - 1.0
        # eq (9)
        lambda_i = -C_i / (sum_grad_C_pk + epsilon)
        if len(ptc_record.neighbors) == 0:
            lambda_i = 0
        # print 'leaf: {}, lambda: {}'.format(i, lambda_i)
        # _dprint('leaf: {}, C_i: {}, num neighbors: {}, lambda_i: {}'.format(i, C_i, len(ptc_record.neighbors), lambda_i))

        ptc_record.lambda_ = lambda_i

def _compute_pos(particles, ptc_records, w_poly6, w_spiky, one_over_rho_0):
    h = w_poly6.h
    delta_q = np.array([0.2 * h, 0])
    k = 0.1
    n = 4
    one_over_w_delta_q = 1.0 / w_poly6.value(delta_q)

    for i, ptc_i in particles.iteritems():
        ptc_record_i = ptc_records[i]
        pos_i = ptc_i.position
        delta_p_i = np.zeros(_NDIM)

        for j in ptc_record_i.neighbors:
            ptc_record_j = ptc_records[j]

            pos_j = particles[j].position
            pos_ji = pos_i - pos_j

            s_corr = w_poly6.value(pos_ji) * one_over_w_delta_q
            s_corr *= s_corr
            s_corr *= s_corr # ^4
            s_corr = -k * s_corr

            factor = ptc_record_i.lambda_ + ptc_record_j.lambda_ + s_corr

            delta_p_i += (factor * w_spiky.gradient(pos_ji))

        delta_p_i *= one_over_rho_0
        # _dprint('leaf: {}, delta_p: {}, pos_i: {}'.format(i, delta_p_i, pos_i))

        ptc_record_i.delta_p = delta_p_i

    for i, ptc_i in particles.iteritems():
        ptc_record = ptc_records[i]
        pos_i = ptc_i.position
        # eq (12)
        position = pos_i + ptc_record.delta_p

        # boundary collision detection
        R = ptc_i.radius
        for d in xrange(_NDIM):
            if position[d] < R:
                position[d] = R + np.random.random() * 1e-1
            elif position[d] > (_EDGESIZE - R):
                position[d] = _EDGESIZE - R - np.random.random() * 1e-1

        ptc_i.position = np.copy(position)

def _solve_constraint(particles, ptc_records, num_iterations, h):
    epsilon = 5.0
    one_over_rho_0 = 10.0 # TODO: should be configurable

    w_poly6 = W_poly6(h)
    w_spiky = W_spiky(h)

    iteration = 0

    while iteration < num_iterations:
        # step 1
        _compute_lambda(particles, ptc_records, w_poly6, w_spiky, one_over_rho_0, epsilon)
        # step 2
        _compute_pos(particles, ptc_records, w_poly6, w_spiky, one_over_rho_0)

        iteration += 1

def _update_velocity(particles, ptc_records, delta_t):
    one_over_delta_t = 1.0 / delta_t

    _dprint('delta_t: {}, 1 / delta_t: {}'.format(delta_t, one_over_delta_t))
    for leaf, ptc in particles.iteritems():
        old_position = ptc_records[leaf].old_position

        old = ptc.velocity
        ptc.velocity = (one_over_delta_t * (ptc.position - old_position))

        # _dprint('leaf: {}, old pos: {}, pos: {}'.format(leaf, old_position, ptc.position))
        # _dprint('leaf: {}, old vel: {}, vel: {}'.format(leaf, old, ptc.velocity))

def _vortocity_refinement(particles, ptc_records, w_poly6, w_spiky, delta_t, c):
    epsilon = .1
    epsilon_delta_t = epsilon * delta_t

    for i, ptc_i in particles.iteritems():
        ptc_record = ptc_records[i]
        new_velocity = np.zeros(_NDIM)
        vortocity = np.zeros(_NDIM)

        for j in ptc_record.neighbors:
            ptc_j = particles[j]

            vel_ij = ptc_j.velocity - ptc_i.velocity
            pos_ji = ptc_i.position - ptc_j.position
            new_velocity += vel_ij * w_poly6.value(pos_ji)

            # vortocity += np.cross(vel_ij, w_spiky.gradient(-pos_ji))

        ptc_record.vortocity = vortocity
        # ptc_record.len_vortocity = length(vortocity)

        new_velocity = ptc_i.velocity + c * new_velocity
        ptc_record.vel_refined = new_velocity
    
    for i, ptc_i in particles.iteritems():
        ptc_record_i = ptc_records[i]
        grad_vortocity = np.zeros(_NDIM)

        for j in ptc_record_i.neighbors:
            ptc_j = particles[j]
            ptc_record_j = ptc_records[j]

            pos_ji = ptc_i.position - ptc_j.position
            grad_vortocity += ptc_record_j.len_vortocity * w_spiky.gradient(pos_ji)

        len_grad = length(grad_vortocity)
        if len_grad > 0:
            grad_vortocity /= len_grad

        ptc_record_i.vel_refined += epsilon_delta_t * np.cross(grad_vortocity, ptc_record_i.vortocity)
    
    for i, ptc in particles.iteritems():
        ptc_record = ptc_records[i]
        ptc.velocity = ptc_record.vel_refined

def update(particles, aabbtree, delta_t, callback=None):
    h = 3.3
    c = 0.01
    num_iterations = 2

    w_poly6 = W_poly6(h)
    w_spiky = W_spiky(h)

    ptc_records = _create_ptc_records(particles)
    _predicate_positions(particles, aabbtree, delta_t)
    _find_neighbors(particles, aabbtree, ptc_records, h, callback)
    _solve_constraint(particles, ptc_records, num_iterations, h)
    _update_velocity(particles, ptc_records, delta_t)
    # _XSPH(particles, ptc_records, w_poly6, c)
    # _vortocity_refinement(particles, ptc_records, w_poly6, w_spiky, delta_t, c)

'''
def _XSPH(particles, aabbtree, h=2.5, c=1e-2):
    kernel = W_poly6(h)
    
    new_velocity_map = {}
    for leaf, ptc_i in particles.iteritems():
        new_velocity = np.zeros(2)

        query_min = ptc_i.position - h
        query_max = ptc_i.position + h
        query_result = aabbtree.query(query_min, query_max, False)

        count = 0
        for neighbor in query_result:
            if neighbor == leaf:
                continue
            ptc_j = particles[neighbor]
            
            wval = kernel.value(ptc_i.position - ptc_j.position)
            if wval > 0:
                new_velocity += (ptc_j.velocity - ptc_i.velocity) * wval
                count += 1

        # print count
        # print ptc_i.velocity, c * new_velocity

        new_velocity = ptc_i.velocity + c * new_velocity
        new_velocity_map[leaf] = new_velocity

    for leaf, ptc in particles.iteritems():
        ptc.velocity = new_velocity_map[leaf]

def update(particles, aabbtree, delta_t, collision_callback=None):

    for leaf, ptc in particles.iteritems():
        ptc.velocity += delta_t * _GRAVITY
        ptc.position += delta_t * ptc.velocity

        # bounce off the wall
        R = ptc.radius
        for d in xrange(_NDIM):
            if (ptc.position[d] < R and ptc.velocity[d] < 0):
                ptc.velocity[d] = -ptc.velocity[d]
                ptc.position[d] = R + np.random.random() * 1e-4
            elif (ptc.position[d] > 100.0 - R and ptc.velocity[d] > 0):
                ptc.velocity[d] = -ptc.velocity[d]
                ptc.position[d] = 100.0 - R - np.random.random() * 1e-4

        aabbtree.update(leaf)
    # collision detection
    # _update_collision(particles, aabbtree, collision_callback)
    _XSPH(particles, aabbtree)

    # update position
    # for leaf, ptc in particles.iteritems():
'''



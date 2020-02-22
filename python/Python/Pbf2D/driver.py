import numpy as np
import numpy.random as nr
import pygame
from pygame.locals import *

from AABB.AABB import *
from AABB.AABBTree import *
from spatial.SpatialHash import SpatialHash
import physics.physics as phy
from time import sleep

# set up a bunch of constants
DISPLAYSURF = None

WHITE       = (255, 255, 255)
BLACK       = (  0,   0,   0)
LIGHTGRAY   = (211, 211, 211)
BLUE        = (  0,   0, 255)
RED         = (255,   0,   0)
LIGHTBLUE   = (125, 249, 255)

BGCOLOR = WHITE
WINDOWWIDTH = 640 # width of the program's window, in pixels
WINDOWHEIGHT = 640 # height in pixels

FPS = 24

MAX_NUM_PARTICLES = 144
PARTICLE_RADIUS = 0.8
SCREEN_RADIUS = int(PARTICLE_RADIUS * WINDOWWIDTH * 0.01)
VELOCITY_SCALE = 5.0
TIME_SCALE = 1.0

def draw_rect(min, max, transform, color=BLACK, thickness=1):
    global DISPLAYSURF

    min_t = transform(min)
    max_t = transform(max)

    width = max_t[0] - min_t[0]
    height = max_t[1] - min_t[1]
    pygame.draw.rect(DISPLAYSURF, color, (min_t[0], min_t[1], width, height), thickness)
    # pygame.display.update()

def draw_AABBTree(aabbtree, transform):
    global DISPLAYSURF

    for index in aabbtree.node_indices():
        node = aabbtree.get_node(index)
        if node.height > 0:
            draw_rect(node._aabb.min, node._aabb.max, transform, LIGHTGRAY, 1)

    for index in aabbtree.leaf_indices():
        node = aabbtree.get_node(index)
        assert node.is_leaf
        draw_rect(node._aabb.min, node._aabb.max, transform, BLACK, 1)

class ParticleData(object):
    def __init__(self, position, radius=PARTICLE_RADIUS):
        self.position = position
        self.radius = radius
        self.velocity = np.zeros(2)

        self._color = BLUE
        self._override_color = None

    def point(self):
        return self.position

    def aabb(self):
        result = AABB()
        vec = np.array([self.radius, self.radius])
        result.min = self.position - vec
        result.max = self.position + vec
        return result

    def draw(self, transform, radius):
        color = self._override_color if self._override_color is not None else self._color
            
        position_t = transform(self.position)
        pygame.draw.circle(DISPLAYSURF, color, position_t, radius)
        # pygame.display.update()

class PositionGenerator(object):
    def __init__(self, step, num_per_row, origin=np.array([0.5, 0.5])):
        self._i = 0
        self._j = 0
        self._step = float(step)
        self._num_per_row = num_per_row
        self._origin = origin * 100.0

    def generate_pos(self):
        delta_xy = np.array([self._i, self._j]) * self._step
        result = self._origin + delta_xy

        for d in xrange(2):
            result[d] = min(max(result[d], 0), 25.0)

        self._i += 1
        if self._i >= self._num_per_row:
            self._i = 0
            self._j += 1

        return result

    def init_velocity(self):
        # return (nr.random(2) - np.array([0.5, 0.5])) * VELOCITY_SCALE
        return np.array([nr.random(), 0]) * VELOCITY_SCALE
        # return np.zeros(2)

def generate_pos(radius):
    return nr.random(2) * (1 - radius * 2) + radius

def _main():
    global DISPLAYSURF
    # standard pygame setup code
    pygame.init()
    FPSCLOCK = pygame.time.Clock()
    flags = FULLSCREEN | DOUBLEBUF
    DISPLAYSURF = pygame.display.set_mode((WINDOWWIDTH, WINDOWHEIGHT), flags)
    # DISPLAYSURF = pygame.display.set_mode((WINDOWWIDTH, WINDOWHEIGHT))
    DISPLAYSURF.set_alpha(None)
    pygame.display.set_caption('AABB Test')

    DISPLAYSURF.fill(BGCOLOR)

    def transform(xy):
        x, y = xy
        x = int(x * WINDOWWIDTH * 0.01)
        y = int(y * WINDOWHEIGHT * 0.01)

        return (x, y)

    def transform_mouse(xy):
        return xy

    INV_WINDOWWIDTH = 100.0 / float(WINDOWWIDTH)
    INV_WINDOWHEIGHT = 100.0 / float(WINDOWHEIGHT)
    def inv_transform_mouse(xy):
        x, y = xy
        x = float(x) * INV_WINDOWWIDTH
        y = float(y) * INV_WINDOWHEIGHT
        return np.array([x, y])


    aabbtree = AABBTree(PARTICLE_RADIUS * 2.5)
    grid = SpatialHash(5.0)

    particles = {}
    done = False

    # note that due to the mouse drag direction, @screen_query_pt1 is
    # not necessarily < @scree_query_pt2
    screen_query_pt1 = None
    screen_query_pt2 = None
    make_query = False
    query_result = set()

    will_draw_aabbtree = False    

    pgen = PositionGenerator(PARTICLE_RADIUS * 2.0, 15, np.array([0.0, 0.1]))
    # while aabbtree.num_leaves < MAX_NUM_PARTICLES:
    while grid.num_nodes < MAX_NUM_PARTICLES:
        # position = generate_pos(PARTICLE_RADIUS)
        position = pgen.generate_pos()
        # print '{}: position: {}'.format(aabbtree.num_leaves, position)
        particle = ParticleData(position, PARTICLE_RADIUS)
        particle.velocity = pgen.init_velocity()
        
        # index = aabbtree.add(particle)
        index = grid.add(particle)
        particles[index] = particle

    def nb_callback(leaf, neighbors, particles, aabbtree):
        if leaf:
            return

        for _, ptc in particles.iteritems():
            ptc._override_color = None

        particles[leaf]._override_color = RED
        for n in neighbors:
            particles[n]._override_color = (255, 150, 0)

    while not done:
        # event handling loop for quit events
        for event in pygame.event.get():
            if event.type == QUIT or (event.type == KEYUP and event.key == K_ESCAPE):
                pygame.quit()
                done = True
                break
            elif event.type == MOUSEBUTTONDOWN:
                query_result = set()
                screen_query_pt1 = pygame.mouse.get_pos()
                make_query = False
            elif event.type == MOUSEBUTTONUP:
                if screen_query_pt1 is not None:
                    screen_query_pt2 = pygame.mouse.get_pos()
                    make_query = True
            elif event.type == KEYUP:
                if event.key == K_c:
                    screen_query_pt1, screen_query_pt2 = None, None
                    query_result = set()
                    make_query = False
                elif event.key == K_a:
                    will_draw_aabbtree = not will_draw_aabbtree

        if done:
            break

        # initialize particles if necessary
        

        # update query area
        if not (screen_query_pt1 is None or make_query):
            screen_query_pt2 = pygame.mouse.get_pos()
        
        # try:
        #     delta_timestep = 1.0 / FPSCLOCK.get_fps()
        # except:
        delta_timestep = TIME_SCALE / float(FPS)
        print 'FPS: {}'.format(FPSCLOCK.get_fps())
        
        # phy.update(particles, aabbtree, delta_timestep)
        phy.update(particles, grid, delta_timestep)

        # make a spatial query
        '''
        if make_query:
            assert screen_query_pt1 is not None and screen_query_pt2 is not None
            query_pt1 = inv_transform_mouse(screen_query_pt1)
            query_pt2 = inv_transform_mouse(screen_query_pt2)

            query_result = aabbtree.query(query_pt1, query_pt2)
            query_result = set(query_result)
        '''

        # initialize rendering
        DISPLAYSURF.fill(BGCOLOR)

        # render query region
        # if screen_query_pt1 is not None and screen_query_pt2 is not None:
        #     draw_rect(screen_query_pt1, screen_query_pt2, transform_mouse, RED, 2)

        # render aabbtree
        # if will_draw_aabbtree:
        #     draw_AABBTree(aabbtree, transform)

        # render queried data
        for l, p in particles.iteritems():
            in_query = l in query_result
            p._color = [BLUE, RED][in_query]
            p.draw(transform, SCREEN_RADIUS)

        pygame.display.flip()

        FPSCLOCK.tick(FPS)

if __name__ == '__main__':
    _main()

#!/usr/bin/python3

# The to_theta and to_xy functions are supposed to be inverses, though
# since the arm "hyperextension".

import argparse
import os
import random
import sys
import numpy as np

import graph_tools as mut  # module under test

MAX_RADIUS = mut.l1 + mut.l2
MAX_RADIUS_SQ = MAX_RADIUS * MAX_RADIUS
MIN_RADIUS = abs(mut.l1 - mut.l2)
MIN_RADIUS_SQ = MIN_RADIUS * MIN_RADIUS
JOINT_CENTER = np.array(mut.joint_center)

MIN_X = JOINT_CENTER[0] - MAX_RADIUS
MAX_X = JOINT_CENTER[0] + MAX_RADIUS
MIN_Y = JOINT_CENTER[1] - MAX_RADIUS
MAX_Y = JOINT_CENTER[1] + MAX_RADIUS

EPSILON = 1.0e-5

options = None  # argparse.Namespace

# notation: pt is in usual x,y coordinate system, _z is in coordinate
# system where JOINT_CENTER is (0,0).

# abs_sum if l is really an np.array object can be done as
# np.linalg.norm(l, ord=1)

def valid_position(pt):
    # numpy.linalg.norm(pt - JOINT_CENTER) < MAX_RADIUS
    pt_z = pt - JOINT_CENTER
    lensq = pt_z.dot(pt_z)
    return MIN_RADIUS_SQ < lensq and lensq < MAX_RADIUS_SQ

def feq(x1, x2, eps = EPSILON):
    return abs(x1 - x2) < eps

def one_trial(pt, orient):
    thetas = mut.to_theta(pt, orient)

    r = mut.to_xy(thetas[0], thetas[1])

    assert feq(r[0], pt[0]), f'pt {pt}, orient {orient}, r {r}'
    assert feq(r[1], pt[1]), f'pt {pt}, orient {orient}, r {r}'
    assert r[2] == orient, f'pt {pt}, orient {orient}, r {r}'

def trials(num_iter):
    for trial in range(num_iter):
        if options.verbose > 2:
            if trial % 1000 == 0:
                sys.stdout.write('.')
                sys.stdout.flush()
        elif options.verbose > 1:
            if trial % 100000 == 0:
                sys.stdout.write('.')
                sys.stdout.flush()
        elif options.verbose > 0:
            if trail % 10000000 == 0:
                sys.stdout.write('.')
                sys.stdout.flush()
        while True:
            pt = np.array([random.uniform(MIN_X, MAX_X),
                           random.uniform(MIN_Y, MAX_Y)])
            if valid_position(pt):
                break
            # chance of success each loop iteration: pi/4 - center hole
        orient = random.getrandbits(1) != 0
        one_trial(pt, orient)

def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('--iterations', '-n', type=int, default=1_000_000,
                        help='number of random sample iterations')
    parser.add_argument('--seed', '-s', type=int, default=None,
                        help='random number generator seed')
    parser.add_argument('--verbose', '-v', action='count',
                        default=0,
                        help='increment the verbosity level by 1')
    global options
    options = parser.parse_args(sys.argv[1:])
    if options.seed is None:
        options.seed = int.from_bytes(os.getrandom(4), byteorder='little')
    print(f'Seed = {options.seed}')
    random.seed(options.seed)
    trials(options.iterations)

if __name__ == '__main__':
    main(sys.argv)

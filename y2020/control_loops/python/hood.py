#!/usr/bin/python

from aos.util.trapezoid_profile import TrapezoidProfile
from frc971.control_loops.python import control_loop
from frc971.control_loops.python import angular_system
from frc971.control_loops.python import controls
import numpy
import sys
from matplotlib import pylab
import gflags
import glog

FLAGS = gflags.FLAGS

try:
    gflags.DEFINE_bool('plot', False, 'If true, plot the loop response.')
except gflags.DuplicateFlagError:
    pass

'''
Hood is an angular subsystem due to the mounting of the encoder on the hood joint.
We are currently electing to ignore potential non-linearity.
'''

#TODO: update constants
kHood = angular_system.AngularSystemParams(
    name='Hood',
    motor=control_loop.BAG(),
    # meters / rad (used the displacement of the lead screw and the angle)
    G=(0.1601 / 0.6457),
    J=0.3,
    q_pos=0.20,
    q_vel=5.0,
    kalman_q_pos=0.12,
    kalman_q_vel=2.0,
    kalman_q_voltage=4.0,
    kalman_r_position=0.05)


def main(argv):
    if FLAGS.plot:
        R = numpy.matrix([[numpy.pi / 2.0], [0.0]])
        angular_system.PlotKick(kHood, R)
        angular_system.PlotMotion(kHood, R)

    # Write the generated constants out to a file.
    if len(argv) != 5:
        glog.fatal(
            'Expected .h file name and .cc file name for the hood and integral hood.'
        )
    else:
        namespaces = ['y2020', 'control_loops', 'superstructure', 'hood']
        angular_system.WriteAngularSystem(kHood, argv[1:3], argv[3:5],
                                          namespaces)


if __name__ == '__main__':
    argv = FLAGS(sys.argv)
    glog.init()
    sys.exit(main(argv))
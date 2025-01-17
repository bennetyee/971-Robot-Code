#ifndef Y2018_CONTROL_LOOPS_SUPERSTRUCTURE_ARM_ARM_CONSTANTS_H_
#define Y2018_CONTROL_LOOPS_SUPERSTRUCTURE_ARM_ARM_CONSTANTS_H_

#include "frc971/control_loops/double_jointed_arm/dynamics.h"

namespace y2018 {
namespace control_loops {
namespace superstructure {
namespace arm {

constexpr double kEfficiencyTweak = 0.95;
constexpr double kStallTorque = 1.41 * kEfficiencyTweak;
constexpr double kFreeSpeed = (5840.0 / 60.0) * 2.0 * M_PI;
constexpr double kStallCurrent = 89.0;

constexpr ::frc971::control_loops::arm::ArmConstants kArmConstants = {
    .l0 = 46.25 * 0.0254,
    .l1 = 41.80 * 0.0254,
    .m0 = 9.34 / 2.2,
    .m1 = 9.77 / 2.2,

    // Moment of inertia of the joints in kg m^2
    .j0 = 2957.05 * 0.0002932545454545454,
    .j1 = 2824.70 * 0.0002932545454545454,

    // Radius of the center of mass of the joints in meters.
    .r0 = 21.64 * 0.0254,
    .r1 = 26.70 * 0.0254,

    // Gear ratios for the two joints.
    .g0 = 140.0,
    .g1 = 90.0,

    // MiniCIM motor constants.
    .efficiency_tweak = kEfficiencyTweak,
    .stall_torque = kStallTorque,
    .free_speed = kFreeSpeed,
    .stall_current = kStallCurrent,
    .resistance = 12.0 / kStallCurrent,
    .Kv = kFreeSpeed / 12.0,
    .Kt = kStallTorque / kStallCurrent,

    // Number of motors on the distal joint.
    .num_distal_motors = 2.0,
};


} // namespace arm
} // namespace superstructure
} // namespace control_loops
} // namespace y2018

#endif // Y2018_CONTROL_LOOPS_SUPERSTRUCTURE_ARM_ARM_CONSTANTS_H_

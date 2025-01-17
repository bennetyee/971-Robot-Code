#ifndef Y2022_CONSTANTS_H_
#define Y2022_CONSTANTS_H_

#include <array>
#include <cmath>
#include <cstdint>

#include "frc971/constants.h"
#include "frc971/control_loops/pose.h"
#include "frc971/control_loops/static_zeroing_single_dof_profiled_subsystem.h"
#include "frc971/shooter_interpolation/interpolation.h"
#include "y2022/control_loops/drivetrain/drivetrain_dog_motor_plant.h"
#include "y2022/control_loops/superstructure/catapult/catapult_plant.h"
#include "y2022/control_loops/superstructure/climber/climber_plant.h"
#include "y2022/control_loops/superstructure/intake/intake_plant.h"
#include "y2022/control_loops/superstructure/turret/turret_plant.h"

using ::frc971::shooter_interpolation::InterpolationTable;

namespace y2022 {
namespace constants {

constexpr uint16_t kCompTeamNumber = 971;
constexpr uint16_t kPracticeTeamNumber = 9971;
constexpr uint16_t kCodingRobotTeamNumber = 7971;

struct Values {
  static const int kZeroingSampleSize = 200;

  static constexpr double kDrivetrainCyclesPerRevolution() { return 512.0; }
  static constexpr double kDrivetrainEncoderCountsPerRevolution() {
    return kDrivetrainCyclesPerRevolution() * 4;
  }
  static constexpr double kDrivetrainEncoderRatio() { return 1.0; }
  static constexpr double kMaxDrivetrainEncoderPulsesPerSecond() {
    return control_loops::drivetrain::kFreeSpeed / (2.0 * M_PI) *
           control_loops::drivetrain::kHighOutputRatio /
           constants::Values::kDrivetrainEncoderRatio() *
           kDrivetrainEncoderCountsPerRevolution();
  }

  static double DrivetrainEncoderToMeters(int32_t in) {
    return ((static_cast<double>(in) /
             kDrivetrainEncoderCountsPerRevolution()) *
            (2.0 * M_PI)) *
           kDrivetrainEncoderRatio() * control_loops::drivetrain::kWheelRadius;
  }

  // Climber
  static constexpr ::frc971::constants::Range kClimberRange() {
    return ::frc971::constants::Range{.lower_hard = -0.01,
                                      .upper_hard = 0.59,
                                      .lower = 0.003,
                                      .upper = 0.555};
  }
  static constexpr double kClimberPotMetersPerRevolution() {
    return 22 * 0.25 * 0.0254;
  }
  static constexpr double kClimberPotRatio() { return 1.0; }

  static constexpr double kClimberPotMetersPerVolt() {
    return kClimberPotRatio() * (5.0 /*turns*/ / 5.0 /*volts*/) *
           kClimberPotMetersPerRevolution();
  }

  struct PotConstants {
    ::frc971::control_loops::StaticZeroingSingleDOFProfiledSubsystemParams<
        ::frc971::zeroing::RelativeEncoderZeroingEstimator>
        subsystem_params;
    double potentiometer_offset;
  };

  PotConstants climber;

  // Intake
  // two encoders with same gear ratio for intake
  static constexpr double kIntakeEncoderCountsPerRevolution() { return 4096.0; }

  static constexpr double kIntakeEncoderRatio() {
    return (16.0 / 64.0) * (18.0 / 62.0);
  }

  static constexpr double kIntakePotRatio() { return 16.0 / 64.0; }

  static constexpr double kIntakePotRadiansPerVolt() {
    return kIntakePotRatio() * (3.0 /*turns*/ / 5.0 /*volts*/) *
           (2 * M_PI /*radians*/);
  }

  static constexpr double kMaxIntakeEncoderPulsesPerSecond() {
    return control_loops::superstructure::intake::kFreeSpeed / (2.0 * M_PI) *
           control_loops::superstructure::intake::kOutputRatio /
           kIntakeEncoderRatio() * kIntakeEncoderCountsPerRevolution();
  }

  struct PotAndAbsEncoderConstants {
    ::frc971::control_loops::StaticZeroingSingleDOFProfiledSubsystemParams<
        ::frc971::zeroing::PotAndAbsoluteEncoderZeroingEstimator>
        subsystem_params;
    double potentiometer_offset;
  };

  PotAndAbsEncoderConstants intake_front;
  PotAndAbsEncoderConstants intake_back;

  // TODO (Yash): Constants need to be tuned
  static constexpr ::frc971::constants::Range kIntakeRange() {
    return ::frc971::constants::Range{
        .lower_hard = -0.85,  // Back Hard
        .upper_hard = 1.85,   // Front Hard
        .lower = -0.400,      // Back Soft
        .upper = 1.57         // Front Soft
    };
  }

  // Intake rollers
  static constexpr double kIntakeRollerSupplyCurrentLimit() { return 40.0; }
  static constexpr double kIntakeRollerStatorCurrentLimit() { return 60.0; }

  // Transfer rollers
  static constexpr double kTransferRollerVoltage() { return 12.0; }

  // Voltage to wiggle the transfer rollers and keep a ball in.
  static constexpr double kTransferRollerWiggleVoltage() { return 3.0; }

  // Turret
  PotAndAbsEncoderConstants turret;
  frc971::constants::Range turret_range;

  static constexpr double kTurretBackIntakePos() { return -M_PI; }
  static constexpr double kTurretFrontIntakePos() { return 0; }

  static constexpr double kTurretPotRatio() { return 27.0 / 110.0; }
  static constexpr double kTurretPotRadiansPerVolt() {
    return kTurretPotRatio() * (10.0 /*turns*/ / 5.0 /*volts*/) *
           (2 * M_PI /*radians*/);
  }
  static constexpr double kTurretEncoderRatio() { return kTurretPotRatio(); }
  static constexpr double kTurretEncoderCountsPerRevolution() { return 4096.0; }

  static constexpr double kMaxTurretEncoderPulsesPerSecond() {
    return control_loops::superstructure::turret::kFreeSpeed / (2.0 * M_PI) *
           control_loops::superstructure::turret::kOutputRatio /
           kTurretEncoderRatio() * kTurretEncoderCountsPerRevolution();
  }

  // Flipper arms
  static constexpr double kFlipperArmSupplyCurrentLimit() { return 40.0; }
  static constexpr double kFlipperArmStatorCurrentLimit() { return 60.0; }

  // Voltage to open the flippers for firing
  static constexpr double kFlipperOpenVoltage() { return 3.0; }
  // Voltage to keep the flippers open for firing once they already are
  static constexpr double kFlipperHoldVoltage() { return 2.5; }
  // Voltage to feed a ball from the transfer rollers to the catpult with the
  // flippers
  static constexpr double kFlipperFeedVoltage() { return -12.0; }

  // Ball is fed into catapult for atleast this time no matter what
  static constexpr std::chrono::milliseconds kExtraLoadingTime() {
    return std::chrono::milliseconds(100);
  }
  // If we have been trying to transfer the ball for this amount of time, it
  // probably got lost so abort
  static constexpr std::chrono::seconds kBallLostTime() {
    return std::chrono::seconds(2);
  }
  // If the flippers took more than this amount of time to open for firing,
  // reseat the ball
  static constexpr std::chrono::milliseconds kFlipperOpeningTimeout() {
    return std::chrono::milliseconds(1000);
  }
  // Don't use flipper velocity readings more than this amount of time in the
  // past
  static constexpr std::chrono::milliseconds kFlipperVelocityValidTime() {
    return std::chrono::milliseconds(100);
  }

  // TODO: (Griffin) this needs to be set
  static constexpr ::frc971::constants::Range kFlipperArmRange() {
    return ::frc971::constants::Range{
        .lower_hard = -0.01, .upper_hard = 0.4, .lower = 0.0, .upper = 0.5};
  }
  // Position of the flippers when they are open
  static constexpr double kFlipperOpenPosition() { return 0.20; }
  // If the flippers were open but now moved back, reseat the ball if they go
  // below this position
  static constexpr double kReseatFlipperPosition() { return 0.1; }

  static constexpr double kFlipperArmsPotRatio() { return 16.0 / 36.0; }

  static constexpr double kFlipperArmsPotRadiansPerVolt() {
    return kFlipperArmsPotRatio() * (3.0 /*turns*/ / 5.0 /*volts*/) *
           (2 * M_PI /*radians*/);
  }

  PotConstants flipper_arm_left;
  PotConstants flipper_arm_right;

  // Catapult.
  static constexpr double kCatapultPotRatio() { return (12.0 / 33.0); }
  static constexpr double kCatapultEncoderRatio() {
    return kCatapultPotRatio();
  }
  static constexpr double kCatapultEncoderCountsPerRevolution() {
    return 4096.0;
  }

  static constexpr double kMaxCatapultEncoderPulsesPerSecond() {
    return control_loops::superstructure::catapult::kFreeSpeed / (2.0 * M_PI) *
           control_loops::superstructure::catapult::kOutputRatio /
           kCatapultEncoderRatio() * kCatapultEncoderCountsPerRevolution();
  }
  static constexpr ::frc971::constants::Range kCatapultRange() {
    return ::frc971::constants::Range{
        .lower_hard = -1.0,
        .upper_hard = 2.0,
        .lower = -0.91,
        .upper = 1.57,
    };
  }

  PotAndAbsEncoderConstants catapult;

  // TODO(milind): set this
  static constexpr double kImuHeight() { return 0.0; }

  struct ShotParams {
    // Measured in radians
    double shot_angle;
    // Muzzle velocity (m/s) of the ball as it is released from the catapult.
    double shot_velocity;

    static ShotParams BlendY(double coefficient, ShotParams a1, ShotParams a2) {
      using ::frc971::shooter_interpolation::Blend;
      return ShotParams{
          .shot_angle = Blend(coefficient, a1.shot_angle, a2.shot_angle),
          .shot_velocity =
              Blend(coefficient, a1.shot_velocity, a2.shot_velocity),
      };
    }
  };

  struct ShotVelocityParams {
    // Speed over ground to use for shooting-on-the-fly.
    double shot_speed_over_ground;

    static ShotVelocityParams BlendY(double coefficient, ShotVelocityParams a1,
                                     ShotVelocityParams a2) {
      using ::frc971::shooter_interpolation::Blend;
      return ShotVelocityParams{Blend(coefficient, a1.shot_speed_over_ground,
                                      a2.shot_speed_over_ground)};
    }
  };

  InterpolationTable<ShotParams> shot_interpolation_table;

  InterpolationTable<ShotVelocityParams> shot_velocity_interpolation_table;

  struct BallColorParams {
    // Rects stored as {x, y, width, height}
    std::array<int, 4> reference_red;
    std::array<int, 4> reference_blue;
    std::array<int, 4> ball_location;
  };

  BallColorParams ball_color;
};

// Creates and returns a Values instance for the constants.
// Should be called before realtime because this allocates memory.
// Only the first call to either of these will be used.
Values MakeValues(uint16_t team);

// Calls MakeValues with aos::network::GetTeamNumber()
Values MakeValues();

}  // namespace constants
}  // namespace y2022

#endif  // Y2022_CONSTANTS_H_

#ifndef Y2019_ACTORS_AUTONOMOUS_ACTOR_H_
#define Y2019_ACTORS_AUTONOMOUS_ACTOR_H_

#include <chrono>
#include <memory>

#include "aos/actions/actions.h"
#include "aos/actions/actor.h"
#include "frc971/autonomous/base_autonomous_actor.h"
#include "frc971/control_loops/control_loops.q.h"
#include "frc971/control_loops/drivetrain/drivetrain.q.h"
#include "frc971/control_loops/drivetrain/drivetrain_config.h"
#include "y2019/control_loops/superstructure/superstructure.q.h"

namespace y2019 {
namespace actors {

using ::frc971::ProfileParameters;
using ::y2019::control_loops::superstructure::superstructure_queue;

class AutonomousActor : public ::frc971::autonomous::BaseAutonomousActor {
 public:
  explicit AutonomousActor(::frc971::autonomous::AutonomousActionQueueGroup *s);

  bool RunAction(
      const ::frc971::autonomous::AutonomousActionParams &params) override;

 private:
  void Reset();

  double elevator_goal_ = 0.0;
  double wrist_goal_ = 0.0;
  double intake_goal_ = 0.0;

  bool suction_on_ = true;
  int suction_gamepiece_ = 1;

  double elevator_max_velocity_ = 0.0;
  double elevator_max_acceleration_ = 0.0;
  double wrist_max_velocity_ = 0.0;
  double wrist_max_acceleration_ = 0.0;

  void set_elevator_goal(double elevator_goal) {
    elevator_goal_ = elevator_goal;
  }
  void set_wrist_goal(double wrist_goal) { wrist_goal_ = wrist_goal; }
  void set_intake_goal(double intake_goal) { intake_goal_ = intake_goal; }

  void set_suction_goal(bool on, int gamepiece_mode) {
    suction_on_ = on;
    suction_gamepiece_ = gamepiece_mode;
  }

  void set_elevator_max_velocity(double elevator_max_velocity) {
    elevator_max_velocity_ = elevator_max_velocity;
  }
  void set_elevator_max_acceleration(double elevator_max_acceleration) {
    elevator_max_acceleration_ = elevator_max_acceleration;
  }
  void set_wrist_max_velocity(double wrist_max_velocity) {
    wrist_max_velocity_ = wrist_max_velocity;
  }
  void set_wrist_max_acceleration(double wrist_max_acceleration) {
    wrist_max_acceleration_ = wrist_max_acceleration;
  }

  void SendSuperstructureGoal() {
    auto new_superstructure_goal = superstructure_queue.goal.MakeMessage();
    new_superstructure_goal->elevator.unsafe_goal = elevator_goal_;
    new_superstructure_goal->wrist.unsafe_goal = wrist_goal_;
    new_superstructure_goal->intake.unsafe_goal = intake_goal_;

    new_superstructure_goal->suction.grab_piece = suction_on_;
    new_superstructure_goal->suction.gamepiece_mode = suction_gamepiece_;

    new_superstructure_goal->elevator.profile_params.max_velocity =
        elevator_max_velocity_;
    new_superstructure_goal->elevator.profile_params.max_acceleration =
        elevator_max_acceleration_;

    new_superstructure_goal->wrist.profile_params.max_velocity =
        wrist_max_velocity_;
    new_superstructure_goal->wrist.profile_params.max_acceleration =
        wrist_max_acceleration_;

    if (!new_superstructure_goal.Send()) {
      LOG(ERROR, "Sending superstructure goal failed.\n");
    }
  }

  bool IsSucked() {
    superstructure_queue.status.FetchLatest();

    if (superstructure_queue.status.get()) {
      return superstructure_queue.status->has_piece;
    }
    return false;
  }

  bool WaitForGamePiece() {
    ::aos::time::PhasedLoop phased_loop(::std::chrono::milliseconds(5),
                                        ::std::chrono::milliseconds(5) / 2);

    while (true) {
      if (ShouldCancel()) {
        return false;
      }
      phased_loop.SleepUntilNext();
      if (IsSucked()) {
        return true;
      }
    }
  }

  bool IsSuperstructureDone() {
    superstructure_queue.status.FetchLatest();

    double kElevatorTolerance = 0.01;
    double kWristTolerance = 0.05;

    if (superstructure_queue.status.get()) {
      const bool elevator_at_goal =
          ::std::abs(elevator_goal_ -
                     superstructure_queue.status->elevator.position) <
          kElevatorTolerance;

      const bool wrist_at_goal =
          ::std::abs(wrist_goal_ -
                     superstructure_queue.status->wrist.position) <
          kWristTolerance;

      return elevator_at_goal && wrist_at_goal;
    }
    return false;
  }

  bool WaitForSuperstructureDone() {
    ::aos::time::PhasedLoop phased_loop(::std::chrono::milliseconds(5),
                                        ::std::chrono::milliseconds(5) / 2);

    while (true) {
      if (ShouldCancel()) {
        return false;
      }
      phased_loop.SleepUntilNext();
      superstructure_queue.status.FetchLatest();
      superstructure_queue.goal.FetchLatest();
      if (IsSuperstructureDone()) {
        return true;
      }
    }
  }
};

}  // namespace actors
}  // namespace y2019

#endif  // Y2019_ACTORS_AUTONOMOUS_ACTOR_H_
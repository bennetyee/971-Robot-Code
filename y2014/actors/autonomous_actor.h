#ifndef Y2014_ACTORS_AUTONOMOUS_ACTOR_H_
#define Y2014_ACTORS_AUTONOMOUS_ACTOR_H_

#include <chrono>
#include <memory>

#include "aos/actions/actions.h"
#include "aos/actions/actor.h"
#include "frc971/autonomous/base_autonomous_actor.h"
#include "frc971/control_loops/drivetrain/drivetrain.q.h"
#include "frc971/control_loops/drivetrain/drivetrain_config.h"
#include "y2014/queues/auto_mode.q.h"
#include "y2014/queues/hot_goal.q.h"

namespace y2014 {
namespace actors {

class AutonomousActor : public ::frc971::autonomous::BaseAutonomousActor {
 public:
  explicit AutonomousActor(::aos::EventLoop *event_loop,
                           ::frc971::autonomous::AutonomousActionQueueGroup *s);

  bool RunAction(
      const ::frc971::autonomous::AutonomousActionParams &params) override;

 private:
  void Reset() {
    InitializeEncoders();
    ResetDrivetrain();
  }

  void PositionClawVertically(double intake_power, double centering_power);
  void PositionClawBackIntake();
  void PositionClawUpClosed();
  void PositionClawForShot();
  void SetShotPower(double power);
  void Shoot();

  bool WaitUntilClawDone();

  ::aos::Fetcher<::y2014::sensors::AutoMode> auto_mode_fetcher_;
  ::aos::Fetcher<::y2014::HotGoal> hot_goal_fetcher_;
};

}  // namespace actors
}  // namespace y2014

#endif  // Y2014_ACTORS_AUTONOMOUS_ACTOR_H_
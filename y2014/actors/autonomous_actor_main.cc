#include <stdio.h>

#include "aos/events/shm-event-loop.h"
#include "aos/init.h"
#include "frc971/autonomous/auto.q.h"
#include "y2014/actors/autonomous_actor.h"

int main(int /*argc*/, char * /*argv*/ []) {
  ::aos::Init(-1);

  ::aos::ShmEventLoop event_loop;
  ::y2014::actors::AutonomousActor autonomous(
      &event_loop, &::frc971::autonomous::autonomous_action);
  autonomous.Run();

  ::aos::Cleanup();
  return 0;
}
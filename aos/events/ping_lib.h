#ifndef AOS_EVENTS_PING_LIB_H_
#define AOS_EVENTS_PING_LIB_H_

#include <chrono>

#include "aos/events/event_loop.h"
#include "aos/events/pong_generated.h"
#include "aos/events/ping_generated.h"

namespace aos {

// Class which sends out a Ping message every X ms, and times the response.
class Ping {
 public:
  Ping(EventLoop *event_loop);

 private:
  // Sends out the ping message with an incrementing count.
  void SendPing();

  // Receives the reply and measures the latency.
  void HandlePong(const examples::Pong &pong);

  aos::EventLoop *event_loop_;
  aos::Sender<examples::Ping> sender_;
  // Timer handle which sends the Ping message.
  aos::TimerHandler *timer_handle_;
  aos::PhasedLoopHandler *phased_loop_handle_;
  aos::Fetcher<examples::Pong> pong_fetcher_;
  // Number of pings sent.
  int count_ = 0;
};

}  // namespace aos

#endif  // AOS_EVENTS_PING_LIB_H_
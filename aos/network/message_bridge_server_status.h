#ifndef AOS_NETWORK_MESSAGE_BRIDGE_SERVER_STATUS_H_
#define AOS_NETWORK_MESSAGE_BRIDGE_SERVER_STATUS_H_

#include <chrono>
#include <functional>

#include "aos/events/event_loop.h"
#include "aos/network/message_bridge_client_generated.h"
#include "aos/network/message_bridge_server_generated.h"
#include "aos/network/timestamp_filter.h"
#include "aos/network/timestamp_generated.h"
#include "aos/time/time.h"

namespace aos {
namespace message_bridge {

// This class encapsulates the server side of sending server statistics and
// managing timestamp offsets.
class MessageBridgeServerStatus {
 public:
  // Time after which we consider the client statistics message stale, and reset
  // the filter.
  static constexpr std::chrono::seconds kClientStatisticsStaleTimeout{1};
  // Time after which we consider the timestamp stale, and reset the filter.
  static constexpr std::chrono::milliseconds kTimestampStaleTimeout{250};

  MessageBridgeServerStatus(aos::EventLoop *event_loop,
                            std::function<void(const Context &)> send_data =
                                std::function<void(const Context &)>());

  MessageBridgeServerStatus(const MessageBridgeServerStatus &) = delete;
  MessageBridgeServerStatus(MessageBridgeServerStatus &&) = delete;
  MessageBridgeServerStatus &operator=(const MessageBridgeServerStatus &) =
      delete;
  MessageBridgeServerStatus &operator=(MessageBridgeServerStatus &&) = delete;

  void set_send_data(std::function<void(const Context &)> send_data) {
    send_data_ = send_data;
  }

  // Resets the filter and clears the entry from the server statistics.
  void ResetFilter(int node_index);

  // Returns the ServerConnection message which is updated by the server.
  ServerConnection *FindServerConnection(std::string_view node_name);

  std::vector<ServerConnection *> server_connection() {
    return server_connection_;
  }

  // Disables sending out any statistics messages.
  void DisableStatistics();

 private:
  static constexpr std::chrono::nanoseconds kStatisticsPeriod =
      std::chrono::seconds(1);
  static constexpr std::chrono::nanoseconds kPingPeriod =
      std::chrono::milliseconds(100);

  // Handle timestamps and statistics.
  void Tick();

  // Sends out the statistics that are continually updated by the
  // ChannelState's.
  void SendStatistics();

  aos::EventLoop *event_loop_;

  // Statistics, timer, and associated sender.
  aos::Sender<ServerStatistics> sender_;
  aos::TimerHandler *statistics_timer_;
  FlatbufferDetachedBuffer<ServerStatistics> statistics_;
  std::vector<flatbuffers::Offset<ServerConnection>> server_connection_offsets_;

  // Fetcher to grab the measured offsets in the client.
  aos::Fetcher<ClientStatistics> client_statistics_fetcher_;

  // ServerConnection to fill out the offsets for from each node.
  std::vector<ServerConnection *> server_connection_;
  // All of these are indexed by the other node index.
  // Fetcher to grab timestamps and therefore offsets from the other nodes.
  std::vector<aos::Fetcher<Timestamp>> timestamp_fetchers_;
  // Bidirectional filters for each connection.
  std::vector<ClippedAverageFilter> filters_;

  // Sender for the timestamps that we are forwarding over the network.
  aos::Sender<Timestamp> timestamp_sender_;

  aos::monotonic_clock::time_point last_statistics_send_time_ =
      aos::monotonic_clock::min_time;

  std::function<void(const Context &)> send_data_;

  bool send_ = true;
};


}  // namespace message_bridge
}  // namespace aos

#endif  // AOS_NETWORK_MESSAGE_BRIDGE_SERVER_STATUS_H_
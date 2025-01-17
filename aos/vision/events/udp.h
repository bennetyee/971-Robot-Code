#ifndef AOS_VISION_EVENTS_UDP_H_
#define AOS_VISION_EVENTS_UDP_H_

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cmath>
#include <string>
#include <vector>

#include "aos/macros.h"
#include "aos/scoped/scoped_fd.h"

namespace aos {
namespace events {

// Simple wrapper around a transmitting UDP socket.
//
// LOG(FATAL)s for all errors, including from Send.
class TXUdpSocket {
 public:
  TXUdpSocket(const std::string &ip_addr, int port);

  // Returns the number of bytes actually sent.
  int Send(const char *data, int size);

 private:
  ScopedFD fd_;

  DISALLOW_COPY_AND_ASSIGN(TXUdpSocket);
};

// Send a protobuf.  Not RT (mallocs on send).
template <typename PB>
class ProtoTXUdpSocket {
 public:
  ProtoTXUdpSocket(const std::string &ip_addr, int port)
      : socket_(ip_addr, port) {}

  void Send(const PB &pb) {
    ::std::string serialized_data;
    pb.SerializeToString(&serialized_data);
    socket_.Send(serialized_data.data(), serialized_data.size());
  }

 private:
  TXUdpSocket socket_;
  DISALLOW_COPY_AND_ASSIGN(ProtoTXUdpSocket);
};

// Simple wrapper around a receiving UDP socket.
//
// LOG(FATAL)s for all errors, including from Recv.
class RXUdpSocket {
 public:
  RXUdpSocket(int port);

  // Returns the number of bytes received.
  int Recv(void *data, int size);

  static int SocketBindListenOnPort(int port);

 private:
  ScopedFD fd_;

  DISALLOW_COPY_AND_ASSIGN(RXUdpSocket);
};

}  // namespace events
}  // namespace aos

#endif  // AOS_VISION_EVENTS_UDP_H_

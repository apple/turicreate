/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef FAULT_SOCKETS_PUBLISH_SOCKET_HPP
#define FAULT_SOCKETS_PUBLISH_SOCKET_HPP
#include <string>
#include <vector>
#include <set>
#include <core/system/nanosockets/zmq_msg_vector.hpp>
#include <core/parallel/mutex.hpp>
#include <core/export.hpp>

namespace turi {
namespace nanosockets {

/**
 * \ingroup nanosockets
 *
 * Constructs a Nanomsg publish socket.
 *
 * The publish socket is bound to a zeromq style endpoint address
 * Endpoints are standard Zeromq style endpoint addresses , for instance,
 * tcp://[ip]:[port], or ipc://[filename] (interprocess socket) or
 * inproc://[handlename] (inprocess socket).
 *
 * Subscribe sockets (\ref subscribe_socket) can then attach to the endpoint
 * and listen for published messages. Note that publish-subscribe is not
 * necessarily reliable; i.e.  subscribers may not receive all published data.
 *
 * \code
 * publish_socket pubsock("ipc:///tmp/publish");
 * pubsock.send("hello world")
 * \endcode
 */
class EXPORT publish_socket {
 public:
  /**
   * Constructs a publish socket.
   * The request will be sent to the current owners of the key
   *
   * \param bind_address this will be address to bind to.
   */
  publish_socket(std::string bind_address);

  /**
   * Closes this socket. Once closed, the socket cannot be used again.
   */
  void close();


  /**
   * Sends a message. All subscribers which match the message (by prefix)
   * will receive a copy. This function is not safe to call in parallel.
   */
  void send(const std::string& message);

  ~publish_socket();

  /**
   * Returns the address the socket is bound to
   */
  std::string get_bound_address();

 private:
  int z_socket;
  mutex z_mutex;
  std::string local_address;
};


} // namespace fault
}
#endif

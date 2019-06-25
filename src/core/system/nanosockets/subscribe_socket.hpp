/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef FAULT_SOCKETS_SUBSCRIBE_SOCKET_HPP
#define FAULT_SOCKETS_SUBSCRIBE_SOCKET_HPP
#include <string>
#include <vector>
#include <set>
#include <boost/function.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/system/nanosockets/zmq_msg_vector.hpp>
#include <core/export.hpp>
namespace turi {

namespace nanosockets {
/**
 * \ingroup nanosockets
 *
 * Constructs a subscribe socket.
 *
 * The subscribe socket binds to at least one endpoint bound by a publish socket
 * (\ref publish_socket). Everything the publish socket publishes will be
 * received by the callback. You can register interest in prefix matches
 * of the message.
 *
 * \code
 * void callback(const std::string& message) {
 *   std::cout << message;
 * }
 * subscribe_socket subsock(callback);
 * subsock.connect("ipc:///tmp/publish");
 * subsock.subscribe("ABC"); // only received messages beginning with ABC
 * \endcode
 */
class EXPORT subscribe_socket {
 public:

   typedef boost::function<void(const std::string& message)> callback_type;

  /**
   * Constructs a subscribe socket.
   * \param callback The function used to process replies.
   *
   * keyval can be NULL in which case all "connect/disconnect" calls
   * must refer to a ZeroMQ endpoints.
   */
  subscribe_socket(callback_type callback);

  /**
   * Closes the socket. Once closed. It cannot be opened again
   */
  void close();

  /**
   * the argument must be a Nanomsg endpoint to connect to.
   */
  void connect(std::string endpoint);

  /**
   * Disconnects from a given endpoint.
   */
  void disconnect(std::string endpoint);

  /**
   * Subscribes to a topic. A topic is any message prefix. Only messages
   * with prefix matching the topic will be received.
   */
  void subscribe(std::string topic);

  /**
   * Unsubscribes from a topic. A topic is any message prefix. See \subscribe
   */
  void unsubscribe(std::string topic);

  bool unsubscribe_all();

  ~subscribe_socket();

 private:
  int z_socket = -1;
  volatile bool shutting_down = false;

  std::map<std::string, size_t> publishers;

  callback_type callback;
  std::set<std::string> topics;
  mutex lock;
  thread thr;


  void thread_function();
};

} // nanosockets
}
#endif

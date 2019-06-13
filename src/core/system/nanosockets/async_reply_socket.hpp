/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef NANOSOCKETS_SOCKETS_ASYNC_REPLY_SOCKET_HPP
#define NANOSOCKETS_SOCKETS_ASYNC_REPLY_SOCKET_HPP
#include <string>
#include <vector>
#include <set>
#include <queue>
#include <boost/function.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/system/nanosockets/zmq_msg_vector.hpp>
#include <core/export.hpp>
namespace turi {

namespace nanosockets {
/**
 * \ingroup nanosockets
 *
 * A nanomsg asynchronous reply socket.
 *
 * The Asynchronous reply socket is the target endpoint of the the
 * asynchronous request socket (\ref async_request_socket). The reply socket
 * listens on an endpoint, and the request socket sends requests to an endpoint.
 * Endpoints are standard Zeromq style endpoint addresses , for instance,
 * tcp://[ip]:[port], or ipc://[filename] (interprocess socket) or
 * inproc://[handlename] (inprocess socket).
 * Ipc sockets are emulated on windows using TCP.
 *
 * The asynchronous reply socket is constructed with a callback which is
 * called whenever a request is received. The callback may be called in
 * parallel; up to the value of nthreads. For instance, a simple echo service
 * can be built with:
 *
 * \code
 * void echo(zmq_msg_vector& recv, zmq_msg_vector& reply) {
 *   reply = recv;
 * }
 * async_reply_socket sock(echo, 4, "ipc:///tmp/echo_service")
 * sock.start_polling();
 * \endcode
 *
 * All messaging is done via \ref zmq_msg_vector which is internally, an array
 * of nn_msg_t objects. Message boundaries are preserved across the wire; i.e.
 * if a request of 4 messages is sent, we will receive exactly a zmq_msg_vector
 * of 4 messages.
 *
 * Also, \see async_request_socket
 */
class EXPORT async_reply_socket {
 public:

  /**
   * Returns true if there are contents to reply.
   * Returns false otherwise.
   * If the reply socket is connected to a request socket,
   * this must always return true.
   * \note There is no provided way to figure out if a reply is necessary.
   * This must be managed on a higher protocol layer.
   */
   typedef boost::function<bool (zmq_msg_vector& recv,
                                 zmq_msg_vector& reply)> callback_type;

  /**
   * Constructs a reply socket.
   * \param callback The function used to process replies. Multiple
   *                 threads may call the callback simultaneously
   * \param nthreads The maximum number of threads to use
   * \param alternate_bind_address If set, this will be address to bind to.
   *                               Otherwise, binds to a free tcp address.
   */
  async_reply_socket(callback_type callback,
                     size_t nthreads = 4,
                     std::string bind_address = "");

  void start_polling();
  void stop_polling();

  /**
   * Closes the socket. Once closed. It cannot be opened again
   */
  void close();

  /**
   * Returns the address the socket is bound to
   */
  std::string get_bound_address();

  ~async_reply_socket();

 private:
  struct job {
    char* data = nullptr;
    size_t datalen = 0;
    void* control = nullptr;
  };
  mutex socketlock;
  int z_socket = -1;
  std::string local_address;
  callback_type callback;

  std::queue<job> jobqueue;
  mutex queuelock;
  conditional queuecond;
  bool queue_terminate = false; // false initially. If true, all threads die.

  void thread_function();
  void poll_function();

  void process_job(job j);
  thread_group threads;
  thread_group poll_thread;
};

} // nanosockets
}
#endif

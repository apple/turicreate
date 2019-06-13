/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef NANOSOCKETS_SOCKETS_ASYNC_REQUEST_SOCKET_HPP
#define NANOSOCKETS_SOCKETS_ASYNC_REQUEST_SOCKET_HPP
#include <string>
#include <vector>
#include <core/parallel/mutex.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/system/nanosockets/zmq_msg_vector.hpp>
#include <core/export.hpp>
namespace turi {

namespace nanosockets {



/**
 * \ingroup nanosockets
 *
 * Constructs a nanomsg asynchronous request socket.
 *
 * The async_request_socket is the requesting endpoint of
 * \ref async_reply_socket. The \ref async_reply_socket listens and waits for
 * requests, and the \ref async_request_socket sends requests.
 * Communication is atomic; either the listener receives all of a message, or
 * none at all. Communication will be automatically retried as needed.
 *
 * \code
 * async_request_socket sock("ipc:///tmp/echo_service")
 * zmq_msg_vector ret;
 * int ret = sock.request_master(msg, ret, 10); // 10 second timeout
 * \endcode
 *
 * This object is multi-threaded. Calls can be made from multiple threads
 * simultaneously and will be queued accordingly.
 *
 * All messaging is done via \ref zmq_msg_vector which is internally, an array
 * of nn_msg_t objects. Message boundaries are preserved across the wire; i.e.
 * if a request of 4 messages is sent, we will receive exactly a zmq_msg_vector
 * of 4 messages.
 */
class EXPORT async_request_socket {
 public:
   /**
   * Constructs a request socket.
   * The request will be sent to the current owners of the key
   *
   * \param target_address Where to connect to
   * \param num_connections Number of parallel connections
   */
  async_request_socket(std::string target_address, size_t num_connections=2);


  /**
   * Closes this socket. Once closed, the socket cannot be used again.
   */
  void close();

  ~async_request_socket();

  /**
   * Sends a request to the server.
   * Returns 0 on success, an error number on failure
   *
   * \param msgs The message to send
   * \param ret The returned message will be stored here
   * \param timeout Number of seconds to wait before timeout. Defaults to 0
   */
  int request_master(zmq_msg_vector& msgs,
                     zmq_msg_vector& ret,
                     size_t timeout = 0);

  /**
   * When waiting for a response, this function will be polled once per second.
   * If this function returns false, the receive polling will quit.
   * This can be used for instance, to quit a receive if we know for certain
   * the remote is no longer alive.
   */
  void set_receive_poller(boost::function<bool()>);
 private:

  struct socket_data {

    inline socket_data() { }

    // fake copy constructor
    inline socket_data(const socket_data& other) { }
    // fake operator=
    inline void operator=(const socket_data& other) { }

    // The actual zmq socket
    int z_socket = -1;
  };
  // queue of available sockets
  mutex global_lock;
  conditional cvar;
  std::vector<size_t> available;

  std::string server;
  std::vector<socket_data> sockets;

  boost::function<bool()> receive_poller;
  // create a socket for socket i
  // returns 0 on success, errno on failure.
  int create_socket(size_t i);
};


}
}
#endif

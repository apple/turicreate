/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#ifndef FAULT_SOCKETS_PUBLISH_SOCKET_HPP
#define FAULT_SOCKETS_PUBLISH_SOCKET_HPP
#include <zmq.h>
#include <fault/zmq/zmq_msg_vector.hpp>
#include <export.hpp>
namespace turi {
namespace zookeeper_util {
class key_value;
} 
}

namespace libfault {

/**
 * \ingroup fault
 * Constructs a zookeeper backed publish socket.
 * This object is very much single threaded.
 * Sends to this socket will be received by all machines
 * subscribed to the socket.
 * 
 * \code
 * publish_socket pubsock(zmq_ctx, NULL, listen_addr);
 * pubsock.send(...);
 * \endcode
 */
class EXPORT publish_socket {
 public:
  /**
   * Constructs a request socket.
   * The request will be sent to the current owners of the key
   *
   * \param zmq_ctx A zeroMQ Context
   * \param keyval A zookeeper key_value object to bind to
   * \param alternate_bind_address If set, this will be address to bind to.
   *                               Otherwise, binds to a free tcp address.
   *
   * keyval can be NULL, in which case, the alternate_bind_address must be
   * provided, in which case this socket behaves like a simple
   * messaging wrapper around ZeroMQ which provides a publish socket.
   */
  publish_socket(void* zmq_ctx,
                 turi::zookeeper_util::key_value* keyval,
                 std::string alternate_bind_address = "");

  /**
   * Closes this socket. Once closed, the socket cannot be used again.
   */
  void close();


  /**
   * Sends a message. All subscribers which match the message (by prefix)
   * will receive a copy.
   */
  void send(zmq_msg_vector& msg);

  ~publish_socket();

  /**
   * Tries to register this socket under a given object key.
   * Returns true on success and false on failure. Only relevant when zookeeper
   * is used. Otherwise this will always return true.
   */
  bool register_key(std::string key);

  /**
   * Like register, but sets the key to an empty value.
   * Reserves ownership of the key, but prohibits people from joining. Only
   * relevant when zookeeper is used. Otherwise this will always return true.
   */
  bool reserve_key(std::string key);


  /**
   * Tries to unregister this socket from a given object key.
   * Returns true on success and false on failure. Only relevant when zookeeper
   * is used. Otherwise this will always return true.
   */
  bool unregister_key(std::string key);

  /**
   * Unregisters all keys this socket was registered under. Only relevant when
   * zookeeper is used. Otherwise this will always return true.
   */
  void unregister_all_keys();

  /**
   * Returns the address the socket is bound to
   */
  std::string get_bound_address();

 private:
  void* z_ctx;
  void* z_socket;
  turi::zookeeper_util::key_value* zk_keyval;
  std::mutex z_mutex;
  std::string local_address;
  std::set<std::string> registered_keys;
};


} // namespace fault
#endif

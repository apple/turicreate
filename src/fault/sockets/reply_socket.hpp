/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef FAULT_SOCKETS_REPLY_SOCKET_HPP
#define FAULT_SOCKETS_REPLY_SOCKET_HPP
#include <string>
#include <vector>
#include <set>
#include <zmq.h>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <fault/zmq/zmq_msg_vector.hpp>
#include <fault/sockets/socket_receive_pollset.hpp>
#include <export.hpp>
namespace turi {
namespace zookeeper_util {
class key_value;
} 
}

namespace libfault {
/**
 * \ingroup fault
 * Constructs an (optionally) zookeeper backed reply socket.
 * Will automatically retry sockets.
 * This object is very much single threaded, with the
 * exception of the key management routines. i.e. keyval_change()
 * could be called from a different thread.
 *
 * This object works together with the socket_receive_pollset().
 * The general construction is to
 *  - Create a reply_socket
 *  - Create a socket_receive_pollset
 *  - Register the keys this socket should listen on (register_key() )
 *  - register this reply_socket with the pollset
 *    (add_to_pollset())
 *  - start the pollset ( socket_receive_pollset::start_poll_thread() )
 *
 * \note
 * if zookeeper is enabled, The first part of the message must be a key. 
 * This must be a key the current client is registered for.
 */
class EXPORT reply_socket {
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
   *
   * If "keyval" is NULL, the alternate_bind_address MUST be a valid 
   * zeromq endpoint address. In which case, the reply_socket will bind to that
   * address.
   * \param zmq_ctx A zeroMQ Context
   * \param keyval A zookeeper key_value object to bind to (may be NULL)
   * \param callback The function used to process replies.
   * \param alternate_bind_address If set, this will be address to bind to.
   *                               Otherwise, binds to a free tcp address.
   */
  reply_socket(void* zmq_ctx,
               turi::zookeeper_util::key_value* keyval,
               callback_type callback,
               std::string alternate_bind_address = "");

  /**
   * Closes the socket. Once closed. It cannot be opened again
   */
  void close();

  /**
   * Tries to register this socket under a given object key.
   * Returns true on success and false on failure.
   */
  bool register_key(std::string key);

  /**
   * Like register, but sets the key to an empty value.
   * Reserves ownership of the key, but prohibits people from joining
   */
  bool reserve_key(std::string key);


  /**
   * Tries to unregister this socket from a given object key.
   * Returns true on success and false on failure
   */
  bool unregister_key(std::string key);

  /**
   * Unregisters all keys this socket was registered under
   */
  void unregister_all_keys();

  /**
   * Registers this socket with the pollset
   * This socket should only registered with one pollset.
   */
  void add_to_pollset(socket_receive_pollset* pollset);

  /**
   * Unregisters this socket with the pollset
   */
  void remove_from_pollset();


  ~reply_socket();

 private:
  void* z_ctx;
  void* z_socket;
  std::string local_address;
  turi::zookeeper_util::key_value* zk_keyval;
  callback_type callback;
  socket_receive_pollset* associated_pollset;

  std::set<std::string> registered_keys;

  void wrapped_callback(socket_receive_pollset* unused, const zmq_pollitem_t& unused2);
};

} // libfault
#endif

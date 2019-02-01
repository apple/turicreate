/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#ifndef FAULT_SOCKETS_SUBSCRIBE_SOCKET_HPP
#define FAULT_SOCKETS_SUBSCRIBE_SOCKET_HPP
#include <zmq.h>
#include <boost/function.hpp>
#include <boost/thread/recursive_mutex.hpp>
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
 * Constructs a zookeeper backed subscribe socket.
 *
 * This object works together with the socket_receive_pollset().
 * The general construction is to
 *  - Create a subscribe_socket
 *  - Create a socket_receive_pollset
 *  - start the pollset ( socket_receive_pollset::start_poll_thread()
 *  - subscribe to a prefix. (Can be the empty string). It is important to at 
 *    least subscribe to the empty string, or nothing will ever be received.
 *
 * \code
 * subscribe_socket subsock(zmq_ctx, NULL, callback);
 * socket_receive_pollset pollset;
 * subsock.add_to_pollset(&pollset);
 * pollset.start_poll_thread();
 * subsock.connect(pub_server);
 * subsock.subscribe("");
 * \endcode
 */
class EXPORT subscribe_socket {
 public:

   typedef boost::function<void(zmq_msg_vector& recv)> callback_type;

  /**
   * Constructs a reply socket.
   * \param zmq_ctx A zeroMQ Context
   * \param keyval A zookeeper key_value object to bind to
   * \param callback The function used to process replies.
   *
   * keyval can be NULL in which case all "connect/disconnect" calls 
   * must refer to a ZeroMQ endpoints.
   */
  subscribe_socket(void* zmq_ctx,
               turi::zookeeper_util::key_value* keyval,
               callback_type callback);

  /**
   * Closes the socket. Once closed. It cannot be opened again
   */
  void close();

  /**
   * If Zookeeper is used, this connects to receive broadcasts on a given 
   * object key. Otherwise, the argument must be a ZeroMQ endpoint to 
   * connect to.
   */
  void connect(std::string objectkey);

  /**
   * Disconnects from a given object key or endpoint. If zookeeper is used,
   * this must be an object key, otherwise the argument must be a ZeroMQ endpoint
   * to disconnect from.
   */
  void disconnect(std::string objectkey);

  /**
   * Subscribes to a topic. A topic is any message prefix.
   */
  void subscribe(std::string topic);

  /**
   * Unsubscribes from a topic. A topic is any message prefix.
   */
  void unsubscribe(std::string topic);

  bool unsubscribe_all();

  /**
   * Registers this socket with the pollset
   * This socket should only registered with one pollset.
   */
  void add_to_pollset(socket_receive_pollset* pollset);

  /**
   * Unregisters this socket with the pollset
   */
  void remove_from_pollset();

   /**
   * Signals that some sets of keys have changed and we should refresh
   * some values. May be called from a different thread
   */
  void keyval_change(turi::zookeeper_util::key_value* unused,
                     const std::vector<std::string>& newkeys,
                     const std::vector<std::string>& deletedkeys,
                     const std::vector<std::string>& modifiedkeys);



  ~subscribe_socket();

 private:
  void* z_ctx;
  void* z_socket;
  std::string local_address;
  turi::zookeeper_util::key_value* zk_keyval;
  callback_type callback;
  socket_receive_pollset* associated_pollset;
  size_t zk_kv_callback_id;

  struct publisher_info {
    std::string key;
    std::string connected_server;
    bool server_changed;
    std::string server;
  };
  bool publisher_info_changed; // true if any of the servers changed.
  std::vector<publisher_info> publishers;
  std::set<std::string> topics;
  boost::recursive_mutex lock;


  void message_callback(socket_receive_pollset* unused, const zmq_pollitem_t& unused2);

  void timer_callback(socket_receive_pollset* pollset, const zmq_pollitem_t& unused);


};

} // libfault
#endif

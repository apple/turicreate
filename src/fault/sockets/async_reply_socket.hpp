/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef FAULT_SOCKETS_ASYNC_REPLY_SOCKET_HPP
#define FAULT_SOCKETS_ASYNC_REPLY_SOCKET_HPP
#include <string>
#include <vector>
#include <set>
#include <queue>
#include <zmq.h>
#include <boost/function.hpp>
#include <boost/thread/condition_variable.hpp>
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
 * Constructs a zookeeper backed asynchronous reply socket.
 * Will automatically retry sockets.
 *
 * This object works together with the socket_receive_pollset().
 * The general construction is to
 *  - Create a async_reply_socket
 *  - Create a socket_receive_pollset
 *  - Register the keys this socket should listen on (register_key() )
 *  - register this reply_socket with the pollset
 *    (add_to_pollset())
 *  - start the pollset ( socket_receive_pollset::start_poll_thread() )
 *
 * \note
 * The first part of the message must be a key. This must be a key the
 * current client is registered for.
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
   * \param zmq_ctx A zeroMQ Context
   * \param keyval A zookeeper key_value object to bind to
   * \param callback The function used to process replies. Multiple
   *                 threads may call the callback simultaneously
   * \param nthreads The maximum number of threads to use
   * \param alternate_bind_address If set, this will be address to bind to.
   *                               Otherwise, binds to a free tcp address.
   *
   * keyval can be NULL, in which case, the alternate_bind_address must be
   * provided, in which case this socket behaves like a simple
   * messaging wrapper around ZeroMQ which provides asynchronous messaging
   * capabilities.
   */
  async_reply_socket(void* zmq_ctx,
                     turi::zookeeper_util::key_value* keyval,
                     callback_type callback,
                     size_t nthreads = 4,
                     std::string alternate_bind_address = "",
                     std::string secret_key = "");

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

  /**
   * Returns the address the socket is bound to
   */
  std::string get_bound_address();

  ~async_reply_socket();

 private:
  void* z_ctx;
  void* z_socket;
  std::string local_address;
  std::string secret_key;
  turi::zookeeper_util::key_value* zk_keyval;
  callback_type callback;
  socket_receive_pollset* associated_pollset;

  std::set<std::string> registered_keys;

  // some thought went into the decision whether to use a condition variable
  // or to use the DEALER-REP pattern. I decided that the condition variable
  // is better in this case since the DEALER's distribution pattern is not
  // exactly what we want here. If a callback takes a long time, the DEALER-REP
  // pattern could queue additional messages on that thread rather than
  // distributing it to other available threads.

  // for now, we use a single queue, but we could use multiple queues +
  // work stealing later if this turns out to block too heavily.
  std::queue<zmq_msg_vector*> jobqueue;
  boost::mutex queuelock;
  boost::condition_variable queuecond;
  void* inproc_pull_socket;
  bool queue_terminate; // false initially. If true, all threads die.

  // replies however have to go through the PUSH-PULL pattern.
  struct thread_data {
    async_reply_socket* parent;
    void* inproc_push_socket;
    boost::thread* thread;
  };

  void thread_function(thread_data* data);
  void process_job(thread_data* data, zmq_msg_vector* msg);

  std::vector<thread_data> threads;

  void wrapped_callback(socket_receive_pollset* unused, const zmq_pollitem_t& unused2);
  void pull_socket_callback(socket_receive_pollset* unused, const zmq_pollitem_t& data);
};

} // libfault
#endif

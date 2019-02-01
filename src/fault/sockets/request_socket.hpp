/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#ifndef FAULT_SOCKETS_REQUEST_SOCKET_HPP
#define FAULT_SOCKETS_REQUEST_SOCKET_HPP
#include <zmq.h>
#include <boost/thread/mutex.hpp>
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
 * Constructs an optionally zookeeper backed request socket.
 * Will automatically retry sockets..
 * This object is very much single threaded, with the
 * exception of the key management routines. i.e. keyval_change()
 * could be called from a different thread.
 *
 * \note
 * The design of this object is relatively straightforward.
 * An array of sockets , one for each possible key (master or slave) is 
 * constructed. A watch is set on zookeeper to identify changes in the servers
 * and sockets and opened/closed appropriately. The key logic for this procedure
 * is the get_socket() and the keyval_change() function. Given this, the actual
 * send/recv logic is simple, and much of the complication is in the error 
 * checking necessary. 
 *
 */
class EXPORT request_socket {
 public: 
  /**
   * Constructs a request socket.
   *
   * If keyval is not NULL, all request will be sent to the current owners of 
   * the key. 
   *
   * If "keyval" is NULL, the masterkey and slavekeys MUST be valid 
   * zeromq endpoint addresss. In which case, the request_socket will send 
   * messages to those addresses.
   *
   * \param zmq_ctx A zeroMQ Context
   * \param keyval A zookeeper key_value object to bind to
   * \param masterkey The master object key where requests (via request_master) 
   *                  are sent. 
   * \param slavekeys The slave object keys where requests (via request_any)
   *                  are sent
   */ 
  request_socket(void* zmq_ctx,
                 turi::zookeeper_util::key_value* keyval,
                 std::string masterkey,
                 std::vector<std::string> slavekeys);

  /**
   * Closes this socket. Once closed, the socket cannot be used again.
   */
  void close();

  ~request_socket();

  /**
   * Sends a request to the object registered to the master key.
   * Returns 0 on success.  Returns an error code on failure.
   * ret will contain the reply message.
   * max_retry_count is the maximum number of times to retry the send on a
   * failure.
   *
   * Possible errors are:
   *
   * - \b EHOSTUNREACH  Target machine is currently unreachable. This could be
   *   because the node registered with the master key is down, or not 
   *   responding.
   * - \b EPIPE  The message was sent, but the connection failed while waiting
   *   for a response. It is unknown if the message was actually received
   *   on the master machine.
   */
  int request_master(zmq_msg_vector& msgs, 
                     zmq_msg_vector& ret,
                     size_t max_retry_count = 5);

  /**
   * Sends a request to the master or any slave key. Returns 0 on success.
   * For fairness, the implementation sweeps over the possible targets and tries 
   * each in turn. 
   * Returns an error code on failure.
   * ret will contain the reply message.
   *
   * Possible errors are:
   *
   * - \b EHOSTUNREACH  All target machines are currently unreachable. This 
   *   could be because all nodes registered with the masterkey or slave keys
   *   are down, or not responding.
   * - \b EPIPE  The message was sent, but the connection failed while waiting
   *   for a response. It is unknown if the message was actually received
   *   on the target machines
   */
  int request_any(zmq_msg_vector& msgs, 
                  zmq_msg_vector& ret,
                  size_t max_retry_count = 5);


  /**
   * Signals that some sets of keys have changed and we should refresh
   * some values. May be called from a different thread
   */
  void keyval_change(turi::zookeeper_util::key_value* unused,
                     const std::vector<std::string>& newkeys,
                     const std::vector<std::string>& deletedkeys,
                     const std::vector<std::string>& modifiedkeys);


 private:
  void* z_ctx;
  turi::zookeeper_util::key_value* zk_keyval;

  struct socket_data {
    // the key this socket references
    std::string key;
    // A cached of the server address obtained from the key set.
    std::string server;
    // The actual zmq socket
    void* z_socket;
    // whether the server has been changed
    bool server_changed;
  };

  boost::mutex target_lock;
  // targetss[0] will be the master. and the remaining are slaves
  std::vector<socket_data> targets;
  
  size_t last_any_id;

  int zk_kv_callback_id;
  // gets the socket connecting to targetservers[id].
  // If already constructed, this will be z_sockets[id], otherwise
  // it will be constructed and z_sockets[id] will be filled in.
  // If return NULL, the caller should retry after some time
  void* get_socket(const size_t id);

  // Forces a socket to close. Might be necessary for some bad errors 
  // (like a request with no reply)
  void force_close_socket(const size_t id);

  // Re-reads the server from zookeeper for the given key
  // target_lock must be acquired. Return true if the value is found.
  bool update_target_from_zk_locked(const size_t id);


  int send_and_retry(size_t id, size_t max_retry,
                      zmq_msg_vector& msgs, zmq_msg_vector& ret);
};


} // namespace fault
#endif

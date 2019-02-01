/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#ifndef FAULT_SOCKETS_ASYNC_REQUEST_SOCKET_HPP
#define FAULT_SOCKETS_ASYNC_REQUEST_SOCKET_HPP
#include <zmq.h>
#include <boost/thread/future.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/unordered_map.hpp>
#include <fault/zmq/zmq_msg_vector.hpp>
#include <fault/sockets/socket_receive_pollset.hpp>
#include <export.hpp>
namespace turi {
namespace zookeeper_util {
class key_value;
} 
}

namespace libfault {

struct message_reply {
  zmq_msg_vector msgvec;
  int status;
};

typedef boost::unique_future<message_reply*> unique_future_reply;
typedef boost::shared_future<message_reply*> future_reply;

/**
 * \ingroup fault
 * Constructs a zookeeper backed asynchronous request socket.
 * Will automatically retry sockets.
 * This object is multi-threaded. Calls can be made from any thread.
 *
 * \note
 * The design is to use DEALER sockets to communicate with the REP sockets
 * on the other side. However, background sends are an issue because zeromq
 * does not let me do sends and receives in seperate threads.
 * The solution is to create 3 sockets.
 *  - A DEALER socket to talk to the REP socket remotely.
 *    This can then be attached to the pollset and callbacks will then set the
 *    future.
 *  - An inproc PULL socket also attached to the pollset. Messages received
 *    on the PULL socket are forwarded out the DEALER socket.
 *  - An inproc PUSH socket to send stuff to the PULL socket.
 *
 *  An issue is the retries are somewhat tricky to manage. So unlike the
 *  synchronous request_socket, this socket does not do retries. Any failure
 *  will return a status in the future, and it is up to the client to perform
 *  the retry.
 */
class EXPORT async_request_socket {
 public:
  typedef libfault::message_reply message_reply;
  typedef libfault::unique_future_reply unique_future_reply;
  typedef libfault::future_reply future_reply;

   /**
   * Constructs a request socket.
   * The request will be sent to the current owners of the key
   *
   * \param zmq_ctx A zeroMQ Context
   * \param keyval A zookeeper key_value object to bind to. 
   * \param masterkey The master object key where requests (via request_master)
   *                  are sent.
   * \param slavekeys The slave object keys where requests (via request_any)
   *                  are sent.
   *
   * If keyval is NULL, masterkey and slavekeys must directly refer to ZeroMQ
   * endpoints. ex: tcp://.... , in which case this socket behaves like a simple
   * messaging wrapper around ZeroMQ which provides asynchronous messaging
   * capabilities.
   */
  async_request_socket(void* zmq_ctx,
                 turi::zookeeper_util::key_value* keyval,
                 std::string masterkey,
                 std::vector<std::string> slavekeys,
                 const std::string public_key = "",
                 const std::string secret_key = "",
                 const std::string server_public_key = "");


  /**
   * Closes this socket. Once closed, the socket cannot be used again.
   */
  void close();

  ~async_request_socket();

  /**
   * Sends a request to the object registered to the master key.
   * Returns a future to a message_reply pointer containing the reply message
   * as well as a status which may contain an error. The message_reply pointer
   * must be freed by the user when done.
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
  future_reply request_master(zmq_msg_vector& msgs, bool noreply = false);

  /**
   * Returns 0 on success.
   * Returns a future to a message_reply pointer containing the reply message
   * as well as a status which may contain an error. The message_reply pointer
   * must be freed by the user when done.
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
  future_reply request_any(zmq_msg_vector& ret, bool noreply = false);


  /**
   * Signals that some sets of keys have changed and we should refresh
   * some values. May be called from a different thread
   */
  void keyval_change(turi::zookeeper_util::key_value* unused,
                     const std::vector<std::string>& newkeys,
                     const std::vector<std::string>& deletedkeys,
                     const std::vector<std::string>& modifiedkeys);

  /**
   * Registers this socket with the pollset
   * This socket should only registered with one pollset.
   */
  void add_to_pollset(socket_receive_pollset* pollset);

  /**
   * Unregisters this socket with the pollset.
   * Also destroys any pending futures
   */
  void remove_from_pollset();



 private:
  void* z_ctx;
  turi::zookeeper_util::key_value* zk_keyval;

  void* inproc_push_socket;
  void* inproc_pull_socket;
  socket_receive_pollset* associated_pollset;

  const std::string public_key;
  const std::string secret_key;
  const std::string server_public_key;

  struct socket_data {

    inline socket_data(): promise_id_ctr(0) { }

    // fake copy constructor
    inline socket_data(const socket_data& other): promise_id_ctr(0) { }
    // fake operator=
    inline void operator=(const socket_data& other) { }

    // the key this socket references
    std::string key;
    // A cached of the server address obtained from the key set.
    std::string server;
    // The actual zmq socket
    void* z_socket;
    // whether the server has been changed
    bool server_changed;

    // an incrementing counter of promise IDs
    size_t promise_id_ctr;

    boost::mutex lock;

    bool socket_may_be_ok();

    boost::unordered_map<uint64_t, boost::promise<message_reply*>* > promises;

    // adds a promise, returning the promise ID
    inline uint64_t add_promise(boost::promise<message_reply*>* promiseptr) {
      assert(promiseptr != NULL);
      promises[promise_id_ctr] = promiseptr;
      return promise_id_ctr++;
    }

    // finds a promise by ID, returns and unregisters the promise
    inline boost::promise<message_reply*>* get_promise(uint64_t promise_id) {
      boost::unordered_map<uint64_t, boost::promise<message_reply*>* >
          ::iterator iter = promises.find(promise_id);
      if (iter == promises.end()) return NULL;
      else {
        boost::promise<message_reply*>* ret = iter->second;
        promises.erase(iter);
        return ret;
      }

    }
  };

  boost::mutex global_lock;
  boost::mutex push_lock;
  // targetss[0] will be the master. and the remaining are slaves
  std::vector<socket_data> targets;

  size_t last_any_id;

  int zk_kv_callback_id;
  // gets the socket connecting to targetservers[id].
  // If already constructed, this will be z_sockets[id], otherwise
  // it will be constructed and z_sockets[id] will be filled in.
  // If return NULL, the caller should retry after some time
  void* get_socket(const size_t id);

  // Re-reads the server from zookeeper for the given key
  // global_lock must be acquired. Return true if the value is found.
  bool update_target_from_zk_locked(const size_t id);


  struct target_header {
    size_t target_id;
    boost::promise<message_reply*>* promise;
  };


  // A single read from pull to determine the target.
  // exclusively accessed by pull_callback
  bool has_next_target;
  target_header next_target;

  // called by an incoming message on the pull socket
  void pull_socket_callback(socket_receive_pollset* pollset,
                            const zmq_pollitem_t& unused);

  // called by an incoming message on the pull socket
  void timer_callback(socket_receive_pollset* pollset,
                      const zmq_pollitem_t& unused);

  // called by an incoming message from one of the DEALER sockets
  void remote_message_callback(size_t id,
                               socket_receive_pollset* pollset,
                               const zmq_pollitem_t& item);

  // clears all pending promises in the target id
  void kill_all_pending_promises_locked(size_t id);

  boost::shared_future<message_reply*> send_to_target(size_t id,
                                                      zmq_msg_vector& msg,
                                                      bool noreply);

};


} // namespace fault

#endif

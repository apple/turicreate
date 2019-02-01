/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#ifndef QUERY_OBJECT_SERVER_REPLICA_HPP
#define QUERY_OBJECT_SERVER_REPLICA_HPP
#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <zookeeper_util/key_value.hpp>
#include <fault/sockets/reply_socket.hpp>
#include <fault/sockets/subscribe_socket.hpp>
#include <fault/query_object_server_common.hpp>
#include <fault/query_object.hpp>
#include <fault/zmq/zmq_msg_vector.hpp>
namespace libfault {

/**
 * \ingroup fault
 */
struct query_object_server_replica {

  void* z_ctx;
  turi::zookeeper_util::key_value* keyval;
  std::string objectkey; // the object key associated with this object
  size_t replicaid;
  query_object* qobj; // The query object
  reply_socket* repsock; // the reply socket associated with the query object
  subscribe_socket* subsock; // If this is a master, it also has an associated
  bool waiting_for_snapshot;
  size_t zk_kv_callback_id;


  boost::shared_mutex query_obj_rwlock;

  int localpipes[2];

  std::vector<zmq_msg_vector> buffered_messages;

  socket_receive_pollset pollset;

  query_object_server_replica(void* zmq_ctx,
                             turi::zookeeper_util::key_value* zk_keyval,
                             std::string objectkey,
                             query_object* qobj,
                             size_t replicaid);

  bool replica_reply_callback(zmq_msg_vector& recv,
                              zmq_msg_vector& reply);

  bool subscribe_callback(zmq_msg_vector& recv);

  // returns 0 on completion, -1 on failure, +1 if promote to master
  int start();

  void playback_recorded_messages();


  void keyval_change(turi::zookeeper_util::key_value* unused,
                     const std::vector<std::string>& newkeys,
                     const std::vector<std::string>& deletedkeys,
                     const std::vector<std::string>& modifiedkeys);

  ~query_object_server_replica();
};



} // namespace libfault
#endif

/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <stdint.h>
#include <vector>
#include <string>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <zookeeper_util/key_value.hpp>
#include <fault/sockets/async_reply_socket.hpp>
#include <fault/sockets/publish_socket.hpp>
#include <fault/query_object_server_common.hpp>
#include <fault/query_object.hpp>
#include <fault/zmq/zmq_msg_vector.hpp>
#include <fault/query_object_server_master.hpp>
#include <fault/query_object_server_internal_signals.hpp>
#include <fault/message_types.hpp>
#include <fault/message_flags.hpp>
namespace libfault {

query_object_server_master::query_object_server_master(void* zmq_ctx,
                                                       turi::zookeeper_util::key_value* zk_keyval,
                                                       std::string _objectkey,
                                                       query_object* _qobj) {
  objectkey = _objectkey;
  qobj = _qobj;
  repsock = new async_reply_socket(zmq_ctx, zk_keyval,
                                   boost::bind(&query_object_server_master::master_reply_callback,
                                               this,
                                               _1, _2));

  pubsock = new publish_socket(zmq_ctx, zk_keyval);
  repsock->register_key(objectkey);
  assert(pubsock->register_key(get_publish_key(objectkey)));
  repsock->add_to_pollset(&pollset);
}


query_object_server_master::~query_object_server_master() {
  repsock->unregister_all_keys();
  pubsock->unregister_all_keys();
  delete repsock;
  delete pubsock;
}

bool query_object_server_master::master_reply_callback(zmq_msg_vector& recv,
                                                       zmq_msg_vector& reply) {
  bool hasreply = false;
  reply.clear();
  uint64_t version = qobj->get_version();

  query_object_message qrecv;
  qobj->parse_message(recv, qrecv);

  bool is_shared_lock = false;
  if (qrecv.header.flags & QO_MESSAGE_FLAG_QUERY) {
    query_obj_rwlock.lock_shared();
    is_shared_lock = true;
  } else {
    query_obj_rwlock.lock();
    is_shared_lock = false;
  }
  bool changed = qobj->process_message(qrecv, reply, &hasreply);
  // if this is the master object
  if (changed) {
    // push out of publish socket. Attach a version number to the head
    // this is the version BEFORE the message
    zmq_msg_t* msg = recv.insert_front();
    zmq_msg_init_size(msg, sizeof(version));
    (*(size_t*)zmq_msg_data(msg)) = version;
    pubsock->send(recv);
    recv.pop_front();
  }

  // we must release the lock only after we send out the pub socket
  if (is_shared_lock) {
    query_obj_rwlock.unlock_shared();
  } else {
    query_obj_rwlock.unlock();
  }

  return hasreply;
}



int query_object_server_master::start() {
  pollset.start_poll_thread();
  int ret = 0;
  while(1) {
    std::cin >> ret;

    if (ret == QO_SERVER_FAIL ||
        ret == QO_SERVER_STOP) {
      break;
    }
    else if (ret == QO_SERVER_PROMOTE) {
      std::cerr << "Cannot promote master! Unexpected message. Ignoring\n";
    }
    else if (ret == QO_SERVER_PRINT) {
      std::cout << "\t" << objectkey << ":0\n";
    }
  }
  pollset.stop_poll_thread();
  return ret;
}




} // namespace libfault

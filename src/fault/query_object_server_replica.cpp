/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <poll.h>
#include <unistd.h>
#include <fault/query_object_server_replica.hpp>
#include <fault/query_object_server_internal_signals.hpp>
#include <fault/query_object_client.hpp>

namespace libfault{

query_object_server_replica::query_object_server_replica(void* zmq_ctx,
                                                         turi::zookeeper_util::key_value* zk_keyval,
                                                         std::string _objectkey,
                                                         query_object* _qobj,
                                                         size_t _replicaid) {
  z_ctx = zmq_ctx;
  keyval = zk_keyval;
  objectkey = _objectkey;
  replicaid = _replicaid;
  qobj = _qobj;
  localpipes[0] = 0;
  localpipes[1] = 0;
  pipe(localpipes);
  repsock = new reply_socket(zmq_ctx, keyval,
                             boost::bind(&query_object_server_replica::replica_reply_callback,
                                         this,
                                         _1, _2));

  subsock = new subscribe_socket(zmq_ctx, keyval,
                                 boost::bind(&query_object_server_replica::subscribe_callback,
                                             this,
                                             _1));

  waiting_for_snapshot = true;
  assert(repsock->reserve_key(get_zk_objectkey_name(objectkey, replicaid)));
  subsock->subscribe("");
  subsock->connect(get_publish_key(objectkey));
  subsock->add_to_pollset(&pollset);
  pollset.start_poll_thread();
}


query_object_server_replica::~query_object_server_replica() {
  subsock->remove_from_pollset();
  repsock->remove_from_pollset();
  close(localpipes[0]);
  close(localpipes[1]);
  delete subsock;
  delete repsock;
}

int query_object_server_replica::start() {
  // spawn a client to request for a serialization of the server
  query_object_client client(z_ctx, keyval, 0);
  void* handle = client.get_object_handle(objectkey);
  assert(handle != NULL);

  query_object_client::query_result res =
      client.query_update_general(handle, NULL, 0,
                                  QO_MESSAGE_FLAG_GET_SERIALIZED_CONTENTS |
                                  QO_MESSAGE_FLAG_QUERY);
  res.wait();


  if (res.get_status() != 0) {
    std::cout << "Unable to acquire serialized object from master.\n";
    return -1;
  } else {
    waiting_for_snapshot = false;
    qobj->deserialize(res.get_reply().c_str(), res.get_reply().length());
    qobj->version = res.reply_header_version();
    playback_recorded_messages();
  }
  repsock->add_to_pollset(&pollset);
  assert(repsock->register_key(get_zk_objectkey_name(objectkey, replicaid)));

  // ok. now watch for changes. if the server goes down... etc
  zk_kv_callback_id = keyval->add_callback(
      boost::bind(&query_object_server_replica::keyval_change, this, _1, _2, _3, _4));

  int ret;
  pollfd pfd[2];
  pfd[0].fd = STDIN_FILENO;
  pfd[0].revents = 0;
  pfd[0].events = POLLIN;
  pfd[1].fd = localpipes[0];
  pfd[1].revents = 0;
  pfd[1].events = POLLIN;

  while(1) {
    pfd[0].revents = 0;
    pfd[1].revents = 0;
    poll(pfd, 2, -1);
    char buf[64] = {0}; ssize_t buflen = 0;
    if (pfd[0].revents) {
      buflen = read(pfd[0].fd, buf, 63);
    } else if (pfd[1].revents) {
      buflen = read(pfd[1].fd, buf, 63);
    }
    if (buflen > 0) {
      // null terminate if necessary
      buf[buflen] = 0;
      ret = atoi(buf);
    }

    if (ret == QO_SERVER_FAIL ||
        ret == QO_SERVER_STOP ||
        ret == QO_SERVER_PROMOTE) {
       break;
    }
    else if (ret == QO_SERVER_PRINT) {
      std::cout << "\t" << objectkey << ":" << replicaid << "\n";
    }
  }
  keyval->remove_callback(zk_kv_callback_id);
  pollset.stop_poll_thread();
  repsock->unregister_all_keys();
  return ret;
}


void query_object_server_replica::playback_recorded_messages(){
  for (size_t i = 0;i < buffered_messages.size(); ++i) {
    uint64_t version = (*(size_t*)zmq_msg_data(buffered_messages[i].front()));
    if (version >= qobj->version) {
      buffered_messages[i].pop_front();
      zmq_msg_vector ignored_reply;
      bool hasreply = false;
      qobj->message_wrapper(buffered_messages[i], ignored_reply,
                            &hasreply, QO_MESSAGE_FLAG_NOREPLY);
    }
  }
  buffered_messages.clear();
}


bool query_object_server_replica::replica_reply_callback(zmq_msg_vector& recv,
                                                         zmq_msg_vector& reply) {
  bool hasreply = false;
  reply.clear();

  bool is_shared_lock = false;
  query_object_message qrecv;
  qobj->parse_message(recv, qrecv);

  // acquire lock
  if (qrecv.header.flags & QO_MESSAGE_FLAG_QUERY) {
    query_obj_rwlock.lock_shared();
    is_shared_lock = true;
  } else {
    query_obj_rwlock.lock();
    is_shared_lock = false;
  }

  qobj->process_message(qrecv, reply, &hasreply);

  // release the lock
  if (is_shared_lock) {
    query_obj_rwlock.unlock_shared();
  } else {
    query_obj_rwlock.unlock();
  }

  return hasreply;
}

bool query_object_server_replica::subscribe_callback(zmq_msg_vector& recv) {
  if (waiting_for_snapshot) {
    buffered_messages.push_back(recv);
    return false;
  } else {
    assert(zmq_msg_size(recv.front()) == sizeof(uint64_t));
    uint64_t version = (*(size_t*)zmq_msg_data(recv.front()));
    if (version != qobj->version) {
      std::cout << "Slave master version divergence\n";
    }
    recv.pop_front();
    bool hasreply = false;
    zmq_msg_vector ignored_reply;

    // parse the message to determine if it a query or an update
    // (thus read lock, or write lock)
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
    // set the no reply flag
    qrecv.header.flags |= QO_MESSAGE_FLAG_NOREPLY;
    qobj->process_message(qrecv, ignored_reply, &hasreply);

    // release the lcok
    if (is_shared_lock) {
      query_obj_rwlock.unlock_shared();
    } else {
      query_obj_rwlock.unlock();
    }
    return false;
  }
}


void query_object_server_replica::keyval_change(turi::zookeeper_util::key_value* unused,
                                                const std::vector<std::string>& newkeys,
                                                const std::vector<std::string>& deletedkeys,
                                                const std::vector<std::string>& modifiedkeys) {
  // if the master for this key was deleted
  if (std::find(deletedkeys.begin(), deletedkeys.end(), objectkey) !=
      deletedkeys.end()) {
    if (master_election(keyval, objectkey)) {
      // halt and promote
      const char* promote = QO_SERVER_PROMOTE_STR;
      write(localpipes[1], promote, strlen(promote));
    }
  }
}



} // namespace

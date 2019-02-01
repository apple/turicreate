/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#include <fault/query_object_server_common.hpp>
#include <fault/query_object_client.hpp>
#include <fault/zmq/zmq_msg_standard_free.hpp>
#include <fault/message_types.hpp>
#include <fault/message_flags.hpp>

namespace libfault {
namespace zookeeper_util = turi::zookeeper_util;

query_object_client::query_object_client(void* zmq_ctx,
                                         std::vector<std::string> zkhosts,
                                         std::string prefix,
                                         size_t replica_count)
    :z_ctx(zmq_ctx), replica_count(replica_count) {
  zk_keyval = new zookeeper_util::key_value(zkhosts, prefix, "");
  my_keyval = true;
  pollset.start_poll_thread();
}


query_object_client::query_object_client(void* zmq_ctx,
                                         zookeeper_util::key_value* keyval,
                                         size_t replica_count)
    :z_ctx(zmq_ctx), zk_keyval(keyval), replica_count(replica_count) {
  my_keyval = false;
  pollset.start_poll_thread();
}



query_object_client::~query_object_client() {
  pollset.stop_poll_thread();
  std::map<std::string, socket_data*>::iterator iter = sockets.begin();
  while (iter != sockets.end()) {
    if (iter->second != NULL) {
      if (iter->second->sock != NULL) {
        delete iter->second->sock;
      }
      delete iter->second;
    }
    ++iter;
  }
  sockets.clear();
  if (my_keyval) delete zk_keyval;
}


void* query_object_client::get_object_handle(const std::string& objectkey) {
  return get_socket(objectkey);
}


query_object_client::socket_data*
query_object_client::get_socket(const std::string& objectkey) {
  boost::lock_guard<boost::mutex> guard(lock);
  std::map<std::string, socket_data*>::iterator iter = sockets.find(objectkey);
  if (iter != sockets.end()) return iter->second;
  else {
    // create a socket
    socket_data* newsock = new socket_data;
    newsock->creation_time = time(NULL);
    newsock->key = objectkey;
    newsock->randid = ((uint64_t)(rand()) << 32) + rand();
    std::string masterkey = get_zk_objectkey_name(objectkey, 0);
    std::vector<std::string> slavekeys;
    for (size_t i = 1; i <= replica_count; ++i) {
      slavekeys.push_back(get_zk_objectkey_name(objectkey, i));
    }

    newsock->sock = new async_request_socket(z_ctx,
                                             zk_keyval,
                                             masterkey,
                                             slavekeys);

    newsock->sock->add_to_pollset(&pollset);

    sockets[objectkey] = newsock;
    return newsock;
  }
}


query_object_client::query_result
query_object_client::query_update_general(void* objecthandle,
                                          char* msg, size_t msglen,
                                          uint64_t flags) {
  // the returned object
  query_result ret;

  socket_data* sock = (socket_data*)(objecthandle);

  query_object_message qmsg;
  qmsg.header.flags = flags;
  qmsg.header.msgid = __sync_fetch_and_add(&sock->randid, 113);
  qmsg.msg = msg;
  qmsg.msglen = msglen;

  zmq_msg_vector send;
  qmsg.write(send);
  if (flags & QO_MESSAGE_FLAG_ANY_TARGET) {
    ret.content_ptr->future = sock->sock->request_any(send);
  } else {
    ret.content_ptr->future = sock->sock->request_master(send);
  }
  return ret;
}




} // namespace libfault

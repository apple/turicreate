/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#include <boost/bind.hpp>
#include <parallel/atomic.hpp>
#include <logger/logger.hpp>
#include <fault/zmq/print_zmq_error.hpp>
#include <fault/sockets/socket_errors.hpp>
#include <fault/sockets/socket_config.hpp>
#include <fault/sockets/async_request_socket.hpp>
#ifdef FAKE_ZOOKEEPER
#include <fault/fake_key_value.hpp>
#else
#include <zookeeper_util/key_value.hpp>
#endif

static turi::atomic<size_t> ASYNC_SOCKET_CTR;

namespace libfault {



async_request_socket::async_request_socket(void* zmq_ctx,
                               turi::zookeeper_util::key_value* keyval,
                               std::string masterkey,
                               std::vector<std::string> slavekeys,
                               const std::string public_key,
                               const std::string secret_key,
                               const std::string server_public_key): 
      public_key(public_key),
      secret_key(secret_key),
      server_public_key(server_public_key) {

  // Is at least one encryption key non-empty AND are they all not non-empty?
  if((!public_key.empty() || !secret_key.empty() || !server_public_key.empty())
     && !(!public_key.empty() && !secret_key.empty() && !server_public_key.empty())) {
    logstream(LOG_ERROR) << "Unable to encrypt socket communication. At least one, but not all, of the "
      "following parameters were set: public_key secret_key server_public_key" << std::endl;
  }

  z_ctx = zmq_ctx;
  zk_keyval = keyval;
  associated_pollset = NULL;
  has_next_target = false;

  global_lock.lock();
  targets.resize(slavekeys.size() + 1);

  // targets[0] is the master and the rest are the slaves
  targets[0].key = masterkey;
  for (size_t i = 1;i < targets.size(); ++i) {
    targets[i].key = slavekeys[i - 1];
  }

  for (size_t i = 0;i < targets.size(); ++i) {
    // initialize all sockets to NULL
    targets[i].z_socket = NULL;
    targets[i].server_changed = false;
    // ask zookeeper for the addresses of the keys
    if (zk_keyval != NULL) {
      std::pair<bool, std::string> ret = zk_keyval->get(targets[i].key);
      if (ret.first) targets[i].server = ret.second;
    } else {
      targets[i].server = targets[i].key;
    }
  }


  //create the inproc sockets for sending data

  // spawn the inproc sockets.
  // need to make a unique name for the pull socket
  char inprocname[64];
  size_t socket_number = ASYNC_SOCKET_CTR.inc();
  sprintf(inprocname, "inproc://async_req_%ld", socket_number);

  inproc_pull_socket = zmq_socket(zmq_ctx, ZMQ_PULL);
  if (inproc_pull_socket == NULL) {
    print_zmq_error("async_request_socket");
    assert(inproc_pull_socket != NULL);
  }
  inproc_push_socket = zmq_socket(zmq_ctx, ZMQ_PUSH);
  if (inproc_push_socket == NULL) {
    print_zmq_error("async_request_socket");
    assert(inproc_push_socket != NULL);
  }
  int rc = zmq_bind(inproc_pull_socket, inprocname);
  if (rc != 0) {
    print_zmq_error("async_request_socket");
    assert(false);
  }
  rc = zmq_connect(inproc_push_socket, inprocname);
  if (rc != 0) {
    print_zmq_error("async_request_socket");
    assert(false);
  }

  global_lock.unlock();
  // register the key value store callback
  if (zk_keyval != NULL) {
    zk_kv_callback_id = zk_keyval->add_callback(boost::bind(&async_request_socket::keyval_change, 
                                                            this, _1, _2, _3, _4));
  }
  last_any_id = 0;
}

void async_request_socket::close() {
  // the value zk_keyval is a proxy to determine if this socket was closed or not
  if (zk_keyval != NULL) {
    zk_keyval->remove_callback(zk_kv_callback_id);
    zk_keyval = NULL;
  }
  if (associated_pollset != NULL) {
    remove_from_pollset();
    associated_pollset = NULL;
  }
  if (inproc_pull_socket != NULL) {
    zmq_close(inproc_pull_socket); inproc_pull_socket = NULL;
    zmq_close(inproc_push_socket); inproc_push_socket = NULL;
  }
}


async_request_socket::~async_request_socket() {
  close();
}

void async_request_socket::keyval_change(turi::zookeeper_util::key_value* unused,
                                   const std::vector<std::string>& newkeys,
                                   const std::vector<std::string>& deletedkeys,
                                   const std::vector<std::string>& modifiedkeys) {
  boost::lock_guard<boost::mutex> guard(global_lock);
  // yes this scan is somewhat inefficient, but the key sets are generally small
  // so the cost of this search will be small. Constructing a hash set will
  // be more costly.

  // scan deleted keys
  for (size_t i = 0;i < targets.size(); ++i) {
    for (size_t j = 0;j < deletedkeys.size(); ++j) {
      if (targets[i].key == deletedkeys[j]) {
        targets[i].server = "";
        // set the change flag if the socket was created
        targets[i].server_changed = (targets[i].z_socket != NULL);
        std::cout << "Server for " << targets[i].key << " has been lost\n";
      }
    }
  }

  // scan new and modified keys
  for (size_t i = 0;i < targets.size(); ++i) {
    for (size_t j = 0;j < newkeys.size(); ++j) {
      if (targets[i].key == newkeys[j]) {
        update_target_from_zk_locked(i);
        std::cout << "Server " << targets[i].server << " has joined " << targets[i].key << "\n";
      }
    }
    for (size_t j = 0;j < modifiedkeys.size(); ++j) {
      if (targets[i].key == modifiedkeys[j]) {
        update_target_from_zk_locked(i);
        std::cout << "Server for " << targets[i].key << " has changed to " << targets[i].server << "\n";
      }
    }
  }
}

bool async_request_socket::update_target_from_zk_locked(const size_t id) {
  // get the new key value
  std::pair<bool, std::string> ret = zk_keyval->get(targets[id].key);
  if (ret.first) {
    targets[id].server = ret.second;
    // set the change flag if the socket was created
    targets[id].server_changed = (targets[id].z_socket != NULL);
  }
  return ret.first;
}


void async_request_socket::add_to_pollset(socket_receive_pollset* pollset) {
  // we need to register the pull socket with the poller
  assert(associated_pollset == NULL);
  zmq_pollitem_t item;
  item.fd = 0;
  item.socket = inproc_pull_socket;
  pollset->add_pollitem(item,
                        boost::bind(&async_request_socket::pull_socket_callback,
                                    this, _1, _2));
  if (zk_keyval) {
    pollset->add_timer_item(this,
                            boost::bind(&async_request_socket::timer_callback,
                                        this, _1, _2));
  }
  associated_pollset = pollset;
}

void async_request_socket::remove_from_pollset() {
  if (associated_pollset != NULL) {
    zmq_pollitem_t item;
    item.fd = 0;
    item.socket = inproc_pull_socket;
    associated_pollset->remove_pollitem(item);
    associated_pollset->remove_timer_item(this);

    for (size_t i = 0;i < targets.size(); ++i) {
      targets[i].lock.lock();
      if (targets[i].z_socket != NULL) {
        item.fd = 0; item.socket = targets[i].z_socket;
        associated_pollset->remove_pollitem(item);
        kill_all_pending_promises_locked(i);
        zmq_close(targets[i].z_socket);
        targets[i].z_socket = NULL;
      }
      targets[i].lock.unlock();
    }
    associated_pollset = NULL;
  }
}

void* async_request_socket::get_socket(const size_t id) {
  // fast path. No changes and socket allocated
  void* zsock = targets[id].z_socket;
  if (!targets[id].server_changed && zsock != NULL) {
    return zsock;
  }
  // acquire a lock on the mutex protecting this target
  boost::lock_guard<boost::mutex> guard(global_lock);

  // same as fast path case. But must retry it in the lock
  if (!targets[id].server_changed && targets[id].z_socket != NULL) {
    return targets[id].z_socket;
  }
  // server was changed
  if (targets[id].server_changed) {
    if (targets[id].z_socket) {
      // kill all the requests here.
      // disassociate with the poller
      if (associated_pollset) {
        zmq_pollitem_t item;
        item.fd = 0; item.socket = targets[id].z_socket;
        associated_pollset->remove_pollitem(item);
        // since the callbacks are performed with the pollset locked,
        // removing the pollitem is safe and also guarantees that no other
        // requests can happen while I am killing requests.
        kill_all_pending_promises_locked(id);
      }
      // close the socket.
      zmq_close(targets[id].z_socket);
      targets[id].z_socket = NULL;
      targets[id].server_changed = false;
    }
  }
  // no socket. construct it
  if (targets[id].z_socket == NULL) {
    // do I have a server to connect to?
    if (targets[id].server.empty()) {
      // unfortunately I do not have a server!
      // return failure
      return NULL;
    }
    targets[id].z_socket = zmq_socket(z_ctx, ZMQ_DEALER);

    if(!public_key.empty() && !secret_key.empty() && !server_public_key.empty()) {
      const int curveKeyLen = 40;

      assert(server_public_key.length() == curveKeyLen);
      int rc = zmq_setsockopt (targets[id].z_socket, ZMQ_CURVE_SERVERKEY, server_public_key.c_str(), curveKeyLen);
      assert (rc == 0);

      assert(public_key.length() == curveKeyLen);
      rc = zmq_setsockopt (targets[id].z_socket, ZMQ_CURVE_PUBLICKEY, public_key.c_str(), curveKeyLen);
      assert (rc == 0);

      assert(secret_key.length() == curveKeyLen);
      rc = zmq_setsockopt (targets[id].z_socket, ZMQ_CURVE_SECRETKEY, secret_key.c_str(), curveKeyLen);
      assert (rc == 0);
    }

    set_conservative_socket_parameters(targets[id].z_socket);
    assert(targets[id].z_socket != NULL);
    // if I have a server to connect to...
    std::string real_address = normalize_address(targets[id].server);
    int rc = zmq_connect(targets[id].z_socket, real_address.c_str());
    if (rc) {
      // unable to connect. Close the socket and fail
      logstream(LOG_ERROR) << "async_request_socket error: Unable to connect to " << real_address
                << ". Error(" << zmq_errno() << ") = " << zmq_strerror(zmq_errno()) << std::endl;
      zmq_close(targets[id].z_socket);
      return NULL;
    }
    // we have just created a socket. add it to the pollset
    zmq_pollitem_t item;
    item.fd = 0; item.socket = targets[id].z_socket;

    if (associated_pollset) {
      associated_pollset->add_pollitem(item,
                                       boost::bind(&async_request_socket::remote_message_callback,
                                                   this, id,
                                                   _1, _2));
    }
    return targets[id].z_socket;
  }
  return targets[id].z_socket;
}



boost::shared_future<async_request_socket::message_reply*>
async_request_socket::send_to_target(size_t id,
                                     zmq_msg_vector& msgs,
                                     bool noreply) {
  assert(msgs.size() > 0);
  boost::promise<message_reply*>* promise = NULL;
  boost::shared_future<async_request_socket::message_reply*> ret;
  if (!noreply) {
    promise = new boost::promise<message_reply*>;
#if __cplusplus < 201103L
    ret = boost::detail::thread_move_t<unique_future_reply>(promise->get_future());
#else
    ret = promise->get_future();
#endif
  }
  else {
    boost::promise<message_reply*> promisetmp;
    promisetmp.set_value(NULL);
#if __cplusplus < 201103L
    ret = boost::detail::thread_move_t<unique_future_reply>(promisetmp.get_future());
#else
    ret = promisetmp.get_future();
#endif
  }
  // format of the packet is
  // [target header]
  // []
  // [target key]
  // [request]
  //
  // construct the header
  target_header header;
  header.promise = promise;
  header.target_id = id;
  // request header
  if (zk_keyval) msgs.insert_front(targets[id].key);
  // we need to make a header and a single empty message
  msgs.insert_front(NULL, 0);
  msgs.insert_front(&header, sizeof(target_header));

  push_lock.lock();
  int rc = msgs.send(inproc_push_socket);
  if (rc != 0) {
    logstream(LOG_ERROR) << "Failed to send message: " << zmq_strerror(rc) << std::endl;
  }
  push_lock.unlock();

  // undo the changes we did to msgs
  msgs.pop_front_and_free();
  msgs.pop_front_and_free();
  if (zk_keyval) msgs.pop_front_and_free();
  return ret;


}

future_reply async_request_socket::request_master(zmq_msg_vector& msgs,
                                                  bool noreply) {
  if (targets[0].socket_may_be_ok()) {
    return send_to_target(0, msgs, noreply);
  } else {
    // this can be replaced with make_future in boost 1.53
    message_reply* reply = new message_reply;
    reply->status = EHOSTUNREACH;
    boost::promise<message_reply*> promise;
    promise.set_value(reply);
#if __cplusplus < 201103L
    return boost::detail::thread_move_t<unique_future_reply>(promise.get_future());
#else
  return promise.get_future();
#endif
  }
}


boost::shared_future<async_request_socket::message_reply*>
async_request_socket::request_any(zmq_msg_vector& msgs,
                                  bool noreply) {
  for (size_t i = 0; i < targets.size(); ++i) {
    size_t id = last_any_id % targets.size();
    last_any_id++;
    if (targets[id].socket_may_be_ok()) {
      return send_to_target(id, msgs, noreply);
    }
  }
  // this can be replaced with make_future in boost 1.53
  message_reply* reply = new message_reply;
  reply->status = EHOSTUNREACH;
  boost::promise<message_reply*> promise;
  promise.set_value(reply);
#if __cplusplus < 201103L
  return boost::detail::thread_move_t<unique_future_reply>(promise.get_future());
#else
  return promise.get_future();
#endif
}


void async_request_socket::timer_callback(socket_receive_pollset* pollset,
                                          const zmq_pollitem_t& unused) {
  // I need to poke every socket once every now and then
  // otherwise messages could get stuck forever
  for (size_t i = 0; i < targets.size(); ++i) get_socket(i);
}

void async_request_socket::pull_socket_callback(socket_receive_pollset* pollset,
                                                const zmq_pollitem_t& unused){
  while (1) {
    // the target header is not filled in
    if (!has_next_target) {
      zmq_msg_t header;
      zmq_msg_init(&header);
      int ret = zmq_msg_recv(&header,  inproc_pull_socket, ZMQ_DONTWAIT);
      if (ret < 0) return;
      next_target = *(target_header*)(zmq_msg_data(&header));
      zmq_msg_close(&header);
      has_next_target = true;
    }
    if (has_next_target) {
      void* zsock = get_socket(next_target.target_id);
      if (zsock != NULL) {
        // check if the socket is ready for sending. Otherwise there is no point

        int ready_for_sending;
        size_t ready_for_sending_len = sizeof(int);
        zmq_getsockopt(zsock, ZMQ_EVENTS,
                       &ready_for_sending, &ready_for_sending_len);
        // socket is not ready for sending.
        if ((ready_for_sending & ZMQ_POLLOUT) == 0) {
          return;
        }
        // ok, it is ready for sending

        // receive the rest of the mesasge
        zmq_msg_vector vec;
        vec.recv(inproc_pull_socket, 0);
        assert(zmq_msg_size(vec[0]) == 0);
        has_next_target = false;
        // push into the target promise queue
        targets[next_target.target_id].lock.lock();
        uint64_t promise_id = (uint64_t)(-1);
        if (next_target.promise != NULL) {
          promise_id = targets[next_target.target_id].add_promise(next_target.promise);
        }
        // insert the promise as the new header.
        // replace the 1st element with the promise ID
        // that will be the routing information
        // see ZeroMQ message envelopes for details
        vec.insert_front(&promise_id, sizeof(uint64_t));

        if (vec.send(zsock, SEND_TIMEOUT) != 0) {
          // fail! Remove it from the promise list
          targets[next_target.target_id].get_promise(promise_id);
          targets[next_target.target_id].lock.unlock();
          // construct the failed reply.
          message_reply* ret = new message_reply;
          ret->status = EHOSTUNREACH;
          next_target.promise->set_value(ret);
          delete next_target.promise;
        } else {
          // ok just unlock
          targets[next_target.target_id].lock.unlock();
        }
      } else {
        // ah bad. there is no valid socket.
        // read the rest of the message,
        // kill this promise. Construct the failed message_reply.
        zmq_msg_vector dumped;
        dumped.recv(inproc_pull_socket, 0);
        has_next_target = false;
        message_reply* ret = new message_reply;
        ret->status = EHOSTUNREACH;
        next_target.promise->set_value(ret);
        delete next_target.promise;
      }
    }
  }
}

void async_request_socket::remote_message_callback(size_t id,
                                                   socket_receive_pollset* pollset,
                                                   const zmq_pollitem_t& item) {
  while(1) {
    // an incoming message!
    zmq_msg_vector tmp;
    int rc = tmp.recv(item.socket, 0);
    if (rc != 0) break;


    uint64_t promise_id = *(uint64_t*)(zmq_msg_data(tmp[0]));
    // the first element should be the promise id
    if (promise_id == (uint64_t)(-1)) continue;

    // ok. read the message into the reply structure.
    message_reply* reply = new message_reply;
    reply->msgvec.clone_from(tmp);
    tmp.clear();
    reply->status = 0;

    // now, we need to massage the reply a bit.
    reply->msgvec.extract_front(&promise_id, sizeof(uint64_t));
    reply->msgvec.assert_pop_front(NULL, 0);

    targets[id].lock.lock();
    boost::promise<message_reply*>* promise = targets[id].get_promise(promise_id);
    targets[id].lock.unlock();

    if (promise != NULL) {
      promise->set_value(reply);
      delete promise;
    } else {
      // ok. this was a reply which I had already forced to fail.
      // undo the work I have done
      delete reply;
    }
  }
}

void async_request_socket::kill_all_pending_promises_locked(size_t id) {
  boost::unordered_map<uint64_t, boost::promise<message_reply*>* >
      ::iterator iter = targets[id].promises.begin();
  while(iter != targets[id].promises.end()) {
    message_reply* ret = new message_reply;
    ret->status = EPIPE;
    iter->second->set_value(ret);
    delete iter->second;
    ++iter;
  }
  targets[id].promises.clear();
}



/**************************************************************************/
/*                                                                        */
/*                         socket_data functions                          */
/*                                                                        */
/**************************************************************************/
bool async_request_socket::socket_data::socket_may_be_ok() {
  if (z_socket != NULL) {
    return true;
  } else if (z_socket == NULL) {
    if (server_changed) return false;
    // if socket is NULL,
    // make sure we do have something we can try to connect to
    // otherwise, we quickly exit
    boost::lock_guard<boost::mutex> guard(lock);
    if (server.empty()) return false;
    return true;
  }
  // unreachable. return anyway to avoid a warning
  return true;
}

} // namespace libfault

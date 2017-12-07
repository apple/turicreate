/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cassert>
#include <thread>
#include <chrono>
#include <logger/logger.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <fault/sockets/socket_errors.hpp>
#include <fault/sockets/request_socket.hpp>
#include <fault/sockets/socket_config.hpp>
#ifdef FAKE_ZOOKEEPER
#include <fault/fake_key_value.hpp>
#else
#include <zookeeper_util/key_value.hpp>
#endif

namespace libfault {
request_socket::request_socket(void* zmq_ctx,
                               turi::zookeeper_util::key_value* keyval,
                               std::string masterkey,
                               std::vector<std::string> slavekeys) {
  z_ctx = zmq_ctx;
  zk_keyval = keyval;

  target_lock.lock();
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
  target_lock.unlock();
  // register the key value store callback
  if (zk_keyval != NULL) {
    zk_kv_callback_id = zk_keyval->add_callback(boost::bind(&request_socket::keyval_change, 
                                                            this, _1, _2, _3, _4));
  }
  last_any_id = 0;
}

void request_socket::close() {
  // the value zk_keyval can act as a proxy as to whether this socket was closed or not
  if (zk_keyval != NULL) {
    zk_keyval->remove_callback(zk_kv_callback_id);
    zk_keyval = NULL;
  }
  target_lock.lock();
  for (size_t i = 0;i < targets.size(); ++i) {
    if (targets[i].z_socket != NULL) zmq_close(targets[i].z_socket);
    targets[i].z_socket = NULL;
  }
  target_lock.unlock();
}



request_socket::~request_socket() {
  close();
}

void request_socket::keyval_change(turi::zookeeper_util::key_value* unused,
                                   const std::vector<std::string>& newkeys,
                                   const std::vector<std::string>& deletedkeys,
                                   const std::vector<std::string>& modifiedkeys) {
  boost::lock_guard<boost::mutex> guard(target_lock);
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

bool request_socket::update_target_from_zk_locked(const size_t id) {
  // get the new key value
  std::pair<bool, std::string> ret = zk_keyval->get(targets[id].key);
  if (ret.first) {
    targets[id].server = ret.second;
    // set the change flag if the socket was created
    targets[id].server_changed = (targets[id].z_socket != NULL);
  } 
  return ret.first;
}

void* request_socket::get_socket(const size_t id) {
  // fast path. No changes and socket allocated
  void* zsock = targets[id].z_socket;
  if (!targets[id].server_changed && zsock != NULL) {
    return zsock;
  }
  // acquire a lock on the mutex 
  boost::lock_guard<boost::mutex> guard(target_lock);

  // same as fast path case. But must retry it in the lock
  if (!targets[id].server_changed && targets[id].z_socket != NULL) {
    return targets[id].z_socket;
  }
  // server was changed
  if (targets[id].server_changed) {
    if (targets[id].z_socket) {
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
    targets[id].z_socket = zmq_socket(z_ctx, ZMQ_REQ);
    set_conservative_socket_parameters(targets[id].z_socket);
    assert(targets[id].z_socket != NULL);
    // if I have a server to connect to...
    std::string real_address = targets[id].server;
    int rc = zmq_connect(targets[id].z_socket, real_address.c_str());
    if (rc) {
      // unable to connect. Close the socket and fail
      logstream(LOG_ERROR) << "request_socket error: Unable to connect to " << real_address
                << ". Error(" << zmq_errno() << ") = " << zmq_strerror(zmq_errno()) << std::endl;
      zmq_close(targets[id].z_socket);
      return NULL;
    }
    return targets[id].z_socket;
  }
  return targets[id].z_socket;
}


void request_socket::force_close_socket(const size_t id) {
  boost::lock_guard<boost::mutex> guard(target_lock);
  if (targets[id].z_socket) {
    zmq_close(targets[id].z_socket);
    targets[id].z_socket = NULL;
    targets[id].server_changed = false;
  }
}



int request_socket::request_master(zmq_msg_vector& msgs, 
                                   zmq_msg_vector& ret,
                                   size_t max_retry_count) {
  return send_and_retry(0, max_retry_count, msgs, ret);
}


int request_socket::request_any(zmq_msg_vector& msgs, 
                                zmq_msg_vector& ret,
                                size_t max_retry_count) {
  int retval;
  size_t i = 0;
  while (i < max_retry_count) {
    size_t id = last_any_id % targets.size();
    last_any_id++;
    if (targets[id].z_socket == NULL && targets[id].server_changed == false) {
      continue;
    }
    retval = send_and_retry(last_any_id, 0, msgs, ret);
    ++i;
    if (retval != 0) break;
  }
  return retval;
}




int request_socket::send_and_retry(size_t id, size_t max_retry,
                                   zmq_msg_vector& msgs, zmq_msg_vector& ret) {
  // insert the header for the target key
  if (zk_keyval) msgs.insert_front(targets[id].key);
  size_t failure_counter = 0;
  
  if (msgs.size() == 0) {
    logstream(LOG_ERROR) << "request socket error: Attempting to send 0 length message"
                         << std::endl;
    assert(msgs.size() > 0);
  } 

  int retval = 1;

  while (retval != 0 && failure_counter <= max_retry) {
    // make a copy of the stuff to be sent
    
    retval = 0;
    // try to get a socket. If we can't get a socket. we fail
    void* zsock = get_socket(id);
    if (zsock == NULL) {
      retval = EHOSTUNREACH;
      goto SEND_AND_RETRY_FAILURE;
    }
    retval = msgs.send(zsock, SEND_TIMEOUT);
    if (retval != 0) {
      goto SEND_AND_RETRY_FAILURE;
    }

    retval = ret.recv(zsock, RECV_TIMEOUT);
    if (retval != 0) {
      // this is kind of fatal. since this will leave the socket in a bad state
      // close the socket.
      goto SEND_AND_RETRY_FAILURE;
    }

SEND_AND_RETRY_FAILURE:
    if (retval != 0) {
      ++failure_counter;
      // sleep for a little while. This is 100ms
      if (failure_counter <= max_retry) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    } 
  }
  // revert msgs to what it was before
  msgs.pop_front();
  // if we have an error, lets keep the return value empty
  if (retval != 0) ret.clear();
  return retval;
}

} // namespace libfault

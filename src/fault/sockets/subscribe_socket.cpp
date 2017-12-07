/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cstdlib>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <fault/sockets/socket_errors.hpp>
#include <fault/sockets/socket_config.hpp>
#include <fault/sockets/subscribe_socket.hpp>
#include <fault/zmq/print_zmq_error.hpp>
#include <network/net_util.hpp>
#include <fault/sockets/get_next_port_number.hpp>
#ifdef FAKE_ZOOKEEPER
#include <fault/fake_key_value.hpp>
#else
#include <zookeeper_util/key_value.hpp>
#endif

namespace libfault {

subscribe_socket::subscribe_socket(void* zmq_ctx,
                                   turi::zookeeper_util::key_value* keyval,
                                   callback_type callback)
    :z_ctx(zmq_ctx), zk_keyval(keyval), callback(callback) {
  // create a socket
  z_socket = zmq_socket(z_ctx, ZMQ_SUB);
  set_conservative_socket_parameters(z_socket);
  associated_pollset = NULL;
  publisher_info_changed = false;
  if (zk_keyval) {
    zk_kv_callback_id = zk_keyval->add_callback(
        boost::bind(&subscribe_socket::keyval_change, this, _1, _2, _3, _4));
  }
}

void subscribe_socket::close() {
  if (zk_keyval != NULL) {
    zk_keyval->remove_callback(zk_kv_callback_id);
    zk_keyval = NULL;
  }
  if (z_socket != NULL) {
    remove_from_pollset();
    zmq_close(z_socket);
    z_socket = NULL;
  }
}

subscribe_socket::~subscribe_socket() {
  close();
}



/**
 * Signals that some sets of keys have changed and we should refresh
 * some values. May be called from a different thread
 */
void subscribe_socket::keyval_change(turi::zookeeper_util::key_value* unused,
                                     const std::vector<std::string>& newkeys,
                                     const std::vector<std::string>& deletedkeys,
                                     const std::vector<std::string>& modifiedkeys) {
  boost::lock_guard<boost::recursive_mutex> guard(lock);
  for (size_t i = 0;i < deletedkeys.size(); ++i) {
    for (size_t j = 0;j < publishers.size(); ++j) {
      if (publishers[j].key == deletedkeys[i]) {
        publishers[j].server_changed = true;
        publishers[j].server = "";
        publisher_info_changed = true;
      }
    }
  }


  for (size_t i = 0;i < newkeys.size(); ++i) {
    for (size_t j = 0;j < publishers.size(); ++j) {
      if (publishers[j].key == newkeys[i]) {
        publishers[j].server = zk_keyval->get(newkeys[i]).second;
        publishers[j].server_changed = publishers[j].server != publishers[j].connected_server;
        publisher_info_changed = true;
      }
    }
  }

  for (size_t i = 0;i < modifiedkeys.size(); ++i) {
    for (size_t j = 0;j < publishers.size(); ++j) {
      if (publishers[j].key == modifiedkeys[i]) {
        publishers[j].server = zk_keyval->get(modifiedkeys[i]).second;
        publishers[j].server_changed = publishers[j].server != publishers[j].connected_server;
        publisher_info_changed = true;
      }
    }
  }
}

void subscribe_socket::subscribe(std::string topic) {
  boost::lock_guard<boost::recursive_mutex> guard(lock);
  if (topics.find(topic) != topics.end()) return;
  topics.insert(topic);
  zmq_setsockopt(z_socket, ZMQ_SUBSCRIBE, topic.c_str(), topic.length());
  return;
}



void subscribe_socket::unsubscribe(std::string topic) {
  boost::lock_guard<boost::recursive_mutex> guard(lock);
  if (topics.find(topic) == topics.end()) return;
  topics.erase(topic);
  zmq_setsockopt(z_socket, ZMQ_UNSUBSCRIBE, topic.c_str(), topic.length());
}



/**
 * Connects to receive broadcasts on a given object key
 */
void subscribe_socket::connect(std::string objectkey) {
  boost::lock_guard<boost::recursive_mutex> guard(lock);
  for (size_t i = 0;i < publishers.size(); ++i) {
    if (publishers[i].key == objectkey) {
      return;
    }
  }
  publisher_info pi;
  pi.key = objectkey;
  pi.server_changed = false;
  if (zk_keyval) {
    pi.server = zk_keyval->get(objectkey).second;
  } else {
    pi.server = objectkey;
  }
  pi.connected_server = pi.server;
  if (pi.server != "") {
    // TODO check return value
    std::string local_address = normalize_address(pi.server);
    zmq_connect(z_socket, local_address.c_str());
  } 
  publishers.push_back(pi);
}

/**
 * Disconnects from a given object key
 */
void subscribe_socket::disconnect(std::string objectkey) {
  boost::lock_guard<boost::recursive_mutex> guard(lock);
  for (size_t i = 0;i < publishers.size(); ++i) {
    if (publishers[i].key == objectkey) {
      zmq_disconnect(z_socket, publishers[i].connected_server.c_str());
      publishers.erase(publishers.begin() + i);
      break;
    }
  }
}



void subscribe_socket::message_callback(socket_receive_pollset* unused,
                                    const zmq_pollitem_t& unused2) {
  while(1) {
    zmq_msg_vector recv;
    // receive with a timeout of 0
    // if nothing to receive, return
    if (recv.recv(z_socket, 0) != 0) break;

    callback(recv);
  }
}

void subscribe_socket::timer_callback(socket_receive_pollset* unused,
                                      const zmq_pollitem_t& unused2) {
  if (publisher_info_changed == false) return;
  else {
    // loop though all the subscriptions and check for server changes.
    // disconnecting, and reconnecting if necessary
    boost::lock_guard<boost::recursive_mutex> guard(lock);
    for(size_t i = 0;i < publishers.size(); ++i) {
      if (publishers[i].server_changed &&
          !publishers[i].connected_server.empty()) {
        zmq_disconnect(z_socket, publishers[i].connected_server.c_str());
        publishers[i].connected_server.clear();
      }
      if (!publishers[i].server.empty()) {
        std::string local_address = normalize_address(publishers[i].server);
        zmq_connect(z_socket, local_address.c_str());
        publishers[i].connected_server = publishers[i].server;
      }
      publishers[i].server_changed = false;
    }
    publisher_info_changed = false;
  }
}


void subscribe_socket::add_to_pollset(socket_receive_pollset* pollset) {
  assert(associated_pollset == NULL);
  associated_pollset = pollset;
  zmq_pollitem_t item;
  item.socket = z_socket;
  item.fd = 0;
  pollset->add_pollitem(item, boost::bind(&subscribe_socket::message_callback, this,
                                          _1, _2));
  if (zk_keyval) {
    pollset->add_timer_item(this,
                            boost::bind(&subscribe_socket::timer_callback,
                                        this, _1, _2));
  }
}


void subscribe_socket::remove_from_pollset() {
  if (associated_pollset != NULL) {
    assert(associated_pollset != NULL);
    zmq_pollitem_t item;
    item.socket = z_socket;
    item.fd = 0;
    associated_pollset->remove_pollitem(item);
    associated_pollset->remove_timer_item(this);
    associated_pollset = NULL;
  }
}

} // namespace libfault


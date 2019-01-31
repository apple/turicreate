/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#include <boost/bind.hpp>
#include <logger/logger.hpp>
#include <fault/sockets/socket_errors.hpp>
#include <fault/sockets/socket_config.hpp>
#include <fault/sockets/reply_socket.hpp>
#include <fault/zmq/print_zmq_error.hpp>
#include <network/net_util.hpp>
#include <fault/sockets/get_next_port_number.hpp>
#ifdef FAKE_ZOOKEEPER
#include <fault/fake_key_value.hpp>
#else
#include <zookeeper_util/key_value.hpp>
#endif

namespace libfault {

reply_socket::reply_socket(void* zmq_ctx,
                           turi::zookeeper_util::key_value* keyval,
                           callback_type callback,
                           std::string alternate_bind_address)
    :z_ctx(zmq_ctx), zk_keyval(keyval), callback(callback), associated_pollset(NULL) {
  // create a socket
  z_socket = zmq_socket(z_ctx, ZMQ_ROUTER);
  set_conservative_socket_parameters(z_socket);
  assert(z_socket != NULL);

  if (!alternate_bind_address.empty()) {
    local_address = normalize_address(alternate_bind_address);
    int rc = zmq_bind(z_socket, local_address.c_str());
    if (rc != 0) {
      print_zmq_error("reply_socket construction: ");
      assert(rc == 0);
    }
  } else {
    // what is the listening socket?
    std::string localip = turi::get_local_ip_as_str(true);
    bool ok = false;
    while (!ok) {
      size_t port = get_next_port_number();
      char port_as_string[128];
      sprintf(port_as_string, "%ld", port);
      local_address = "tcp://" + localip + ":" + port_as_string;
      // try to bind
      int rc = zmq_bind(z_socket, local_address.c_str());
      ok = (rc == 0);
      /*if (rc == EADDRINUSE) {
        std::cout << local_address << " in use. Trying another port.\n";
        continue;
      } else if (rc != 0) {
        std::cout << "Unable to bind to " << local_address << ". "
                  << "Error(" << rc << ") = " << zmq_strerror(rc) << "\n";
      }*/
    }
  }
  // std::cout << "Bound to " << local_address << "\n";
}

void reply_socket::close() {
  if (z_socket != NULL) {
    remove_from_pollset();
    unregister_all_keys();
    zmq_close(z_socket);
    z_socket = NULL;
  }
}

reply_socket::~reply_socket() {
  close();
}

bool reply_socket::register_key(std::string key) {
  if (zk_keyval == NULL) return false;
  bool ret = zk_keyval->insert(key, local_address);
  if (ret) registered_keys.insert(key);
  return ret;
}

bool reply_socket::reserve_key(std::string key) {
  if (zk_keyval == NULL) return false;
  bool ret = zk_keyval->insert(key, "");
  if (ret) registered_keys.insert(key);
  return ret;
}



bool reply_socket::unregister_key(std::string key) {
  if (zk_keyval == NULL) return false;
  if (registered_keys.count(key)) {
    registered_keys.erase(registered_keys.find(key));
    return zk_keyval->erase(key);
  } else {
    return false;
  }
}

void reply_socket::unregister_all_keys() {
  // make a copy of the registered_keys set since the
  // unregister_key call will modify it
  std::set<std::string> keys = registered_keys;
  std::set<std::string>::const_iterator iter = keys.begin();
  while (iter != keys.end()) {
    assert(unregister_key(*iter));
    ++iter;
  }
}


void reply_socket::wrapped_callback(socket_receive_pollset* unused,
                                    const zmq_pollitem_t& unused2) {
  while(1) {
    zmq_msg_vector recv;
    // receive with a timeout of 0
    // if nothing to receive, return
    if (recv.recv(z_socket, 0) != 0) break;

    zmq_msg_vector send;
    // shift what we received into send
    while(recv.size() > 0) {
      zmq_msg_init(send.insert_back());
      zmq_msg_copy(send.back(), recv.front()); recv.pop_front_and_free();
      if (zmq_msg_size(send.back()) == 0) break;
    }

    // bad packet
    if (recv.size() == 0) {
      logstream(LOG_ERROR) << "Unexpected Message Format" << std::endl;
      continue;
    }

    // check that this is for a key I am registered for
    if (zk_keyval) {
      std::string s = recv.extract_front();
      if(registered_keys.count(s) == 0) {
        logstream(LOG_ERROR) << "Received message "<< s << " destined for a different object!" << std::endl;
        continue;
      }
    }
    zmq_msg_vector reply;
    bool hasreply = callback(recv, reply);
    if (hasreply) {
      while(!reply.empty()) {
        zmq_msg_init(send.insert_back());
        zmq_msg_copy(send.back(), reply.front());
        reply.pop_front_and_free();
      }
      // reply
      send.send(z_socket);
    }
  }
}


void reply_socket::add_to_pollset(socket_receive_pollset* pollset) {
  assert(associated_pollset == NULL);
  associated_pollset = pollset;
  zmq_pollitem_t item;
  item.socket = z_socket;
  item.fd = 0;
  pollset->add_pollitem(item, boost::bind(&reply_socket::wrapped_callback, this,
                                          _1, _2));
}
void reply_socket::remove_from_pollset() {
  if (associated_pollset != NULL) {
    assert(associated_pollset != NULL);
    zmq_pollitem_t item;
    item.socket = z_socket;
    item.fd = 0;
    associated_pollset->remove_pollitem(item);
    associated_pollset = NULL;
  }
}

} // namespace libfault


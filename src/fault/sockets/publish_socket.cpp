/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cassert>
#include <boost/bind.hpp>
#include <fault/sockets/socket_errors.hpp>
#include <fault/sockets/publish_socket.hpp>
#include <fault/sockets/socket_config.hpp>
#include <fault/zmq/print_zmq_error.hpp>
#include <network/net_util.hpp>
#include <fault/sockets/get_next_port_number.hpp>
#ifdef FAKE_ZOOKEEPER
#include <fault/fake_key_value.hpp>
#else
#include <zookeeper_util/key_value.hpp>
#endif
namespace libfault {
publish_socket::publish_socket(void* zmq_ctx,
                               turi::zookeeper_util::key_value* keyval,
                               std::string alternate_bind_address)
    :z_ctx(zmq_ctx), zk_keyval(keyval) {
  // create a socket
  z_socket = zmq_socket(z_ctx, ZMQ_PUB);
  set_conservative_socket_parameters(z_socket);
#ifdef ZMQ_PUB_NODROP
  {
    // if no drop is defined. Set the HWM to something more sensible
    int nodrop = 1;
    int rc = zmq_setsockopt(z_socket, ZMQ_PUB_NODROP, &nodrop, sizeof(nodrop));
    assert(rc == 0);
  }
#endif

  int hwm = 1024*1024; // 1MB
  int rc = zmq_setsockopt(z_socket, ZMQ_SNDHWM, &hwm, sizeof(hwm));
  assert(rc == 0);
  assert(z_socket != NULL);

  if (!alternate_bind_address.empty()) {
    local_address = normalize_address(alternate_bind_address);
    int rc = zmq_bind(z_socket, local_address.c_str());
    if (rc != 0) {
      print_zmq_error("publish_socket construction: ");
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

publish_socket::~publish_socket() {
  close();
}

void publish_socket::close() {
  unregister_all_keys();
  if (z_socket != NULL) zmq_close(z_socket);
  z_socket = NULL;
}

bool publish_socket::register_key(std::string key) {
  // if we are not using zookeeper, we always succeed on key operations.
  // since keys are not used without zookeeper, everyone connected to the same
  // address will receive all published messages. We should thus return
  // true since it ensures compatibility with any code which assumes zookeeper's
  // existance.
  if (!zk_keyval) return true;
  bool ret = zk_keyval->insert(key, local_address);
  if (ret) registered_keys.insert(key);
  return ret;
}

bool publish_socket::reserve_key(std::string key) {
  // if we are not using zookeeper, we always succeed on key operations
  // since keys are not used anyway
  if (!zk_keyval) return true;
  bool ret = zk_keyval->insert(key, "");
  if (ret) registered_keys.insert(key);
  return ret;
}



bool publish_socket::unregister_key(std::string key) {
  // if we are not using zookeeper, we always succeed on key operations
  // since keys are not used anyway
  if (!zk_keyval) return true;
  if (registered_keys.count(key)) {
    registered_keys.erase(registered_keys.find(key));
    return zk_keyval->erase(key);
  } else {
    return false;
  }
}

void publish_socket::unregister_all_keys() {
  // if we are not using zookeeper, we always succeed on key operations
  // since keys are not used anyway
  if (!zk_keyval) return;
  // make a copy of the registered_keys set since the
  // unregister_key call will modify it
  std::set<std::string> keys = registered_keys;
  std::set<std::string>::const_iterator iter = keys.begin();
  while (iter != keys.end()) {
    assert(unregister_key(*iter));
    ++iter;
  }
}


void publish_socket::send(zmq_msg_vector& msg) {
  std::lock_guard<std::mutex> lock(z_mutex);
  msg.send(z_socket);
}

std::string publish_socket::get_bound_address() {
  char* buf[256];
  size_t optlen = 256;
  zmq_getsockopt(z_socket, ZMQ_LAST_ENDPOINT, (void*)buf, &optlen);
  return std::string((char*)buf);
}


} // namespace libfault

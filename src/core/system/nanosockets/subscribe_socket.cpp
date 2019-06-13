/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cstdlib>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <core/system/nanosockets/socket_errors.hpp>
#include <core/system/nanosockets/socket_config.hpp>
#include <core/system/nanosockets/subscribe_socket.hpp>
#include <core/system/nanosockets/print_zmq_error.hpp>
#include <network/net_util.hpp>

extern "C" {
#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>
}
namespace turi {
namespace nanosockets {

subscribe_socket::subscribe_socket(callback_type callback)
    :callback(callback) {
  // create a socket
  z_socket = nn_socket(AF_SP, NN_SUB);
  set_conservative_socket_parameters(z_socket);
  thr.launch([=](){this->thread_function();});
}

void subscribe_socket::close() {
  if (z_socket != -1) {
    nn_close(z_socket);
    z_socket = -1;
  }
  if (shutting_down == false) {
    shutting_down = true;
    thr.join();
  }
}

subscribe_socket::~subscribe_socket() {
  close();
}

void subscribe_socket::subscribe(std::string topic) {
  std::lock_guard<mutex> guard(lock);
  if (topics.find(topic) != topics.end()) return;
  topics.insert(topic);
  nn_setsockopt(z_socket, NN_SUB, NN_SUB_SUBSCRIBE, topic.c_str(), topic.length());
  return;
}



void subscribe_socket::unsubscribe(std::string topic) {
  std::lock_guard<mutex> guard(lock);
  if (topics.find(topic) == topics.end()) return;
  topics.erase(topic);
  nn_setsockopt(z_socket, NN_SUB, NN_SUB_UNSUBSCRIBE, topic.c_str(), topic.length());
}



/**
 * Connects to receive broadcasts on a given object key
 */
void subscribe_socket::connect(std::string objectkey) {
  std::lock_guard<mutex> guard(lock);
  if (publishers.count(objectkey) == 0) {
    std::string local_address = normalize_address(objectkey);
    int ret = nn_connect(z_socket, local_address.c_str());
    if (ret > 0) {
      publishers[objectkey] = ret;
    }
  }
}

/**
 * Disconnects from a given object key
 */
void subscribe_socket::disconnect(std::string objectkey) {
  std::lock_guard<mutex> guard(lock);
  if (publishers.count(objectkey)) {
    nn_shutdown(z_socket, publishers[objectkey]);
    publishers.erase(objectkey);
  }
}


void subscribe_socket::thread_function() {
  int rc = 0;
  while(!shutting_down) {
    nn_pollfd pollitem;
    pollitem.fd = z_socket;
    pollitem.revents = 0;
    pollitem.events = NN_POLLIN;

    rc = nn_poll(&pollitem, 1, RECV_TIMEOUT);

    if (rc <= 0) continue;

    char* buf = NULL;
    size_t len = NN_MSG;
    rc = nn_recv(z_socket, reinterpret_cast<void*>(&buf), len, 0);
    if (rc < 0) continue;
    std::string val(buf, rc);
    callback(val);
    nn_freemsg(buf);
  }
}

} // namespace nanosockets
}

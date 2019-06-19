/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cassert>
#include <core/system/nanosockets/socket_errors.hpp>
#include <core/system/nanosockets/socket_config.hpp>
#include <core/system/nanosockets/publish_socket.hpp>
#include <core/system/nanosockets/print_zmq_error.hpp>
#include <core/system/nanosockets/get_next_port_number.hpp>
#include <network/net_util.hpp>
#include <mutex>
#include <core/storage/serialization/oarchive.hpp>
extern "C" {
#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>
}
namespace turi {
namespace nanosockets {
publish_socket::publish_socket(std::string bind_address) {
  // create a socket
  z_socket = nn_socket(AF_SP, NN_PUB);
  set_conservative_socket_parameters(z_socket);
  if (bind_address.length() > 0) {
    local_address = normalize_address(bind_address);
    int rc = nn_bind(z_socket, local_address.c_str());
    if (rc < 0) {
      print_zmq_error("publish_socket construction: ");
      assert(rc >= 0);
    }
  } else {
    std::string localip = turi::get_local_ip_as_str(true);
    bool ok = false;
    while (!ok) {
      size_t port = get_next_port_number();
      char port_as_string[128];
      sprintf(port_as_string, "%ld", port);
      local_address = "tcp://" + localip + ":" + port_as_string;
      // try to bind
      int rc = nn_bind(z_socket, local_address.c_str());
      ok = (rc >= 0);
      /*if (rc == EADDRINUSE) {
        std::cout << local_address << " in use. Trying another port.\n";
        continue;
      } else if (rc != 0) {
        std::cout << "Unable to bind to " << local_address << ". "
                  << "Error(" << rc << ") = " << zmq_strerror(rc) << "\n";
      }*/
    }
  }
}

publish_socket::~publish_socket() {
  close();
}

void publish_socket::close() {
  if (z_socket != -1) nn_close(z_socket);
  z_socket = -1;
}


void publish_socket::send(const std::string& msg) {
  std::lock_guard<mutex> lock(z_mutex);
  nn_send(z_socket, msg.c_str(), msg.length(), 0);
}

std::string publish_socket::get_bound_address() {
  return local_address;
}


} // namespace nanosockets
}

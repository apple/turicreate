/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cassert>
#include <core/parallel/atomic.hpp>
#include <core/logging/logger.hpp>
#include <core/system/nanosockets/print_zmq_error.hpp>
#include <core/system/nanosockets/socket_errors.hpp>
#include <core/system/nanosockets/socket_config.hpp>
#include <core/system/nanosockets/async_request_socket.hpp>

extern "C" {
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
}

static turi::atomic<size_t> ASYNC_SOCKET_CTR;
namespace turi {
namespace nanosockets {



async_request_socket::async_request_socket(std::string target_connection,
                                           size_t num_connections) {
  server = normalize_address(target_connection);
  sockets.resize(num_connections);
  for (size_t i = 0;i < sockets.size(); ++i) {
    available.push_back(i);
  }

}

void async_request_socket::close() {
  // closes all sockets
  for (size_t i = 0;i < sockets.size(); ++i) {
    if (sockets[i].z_socket != -1) {
      nn_close(sockets[i].z_socket);
      sockets[i].z_socket = -1;
    }
  }
}


async_request_socket::~async_request_socket() {
  std::unique_lock<mutex> lock(global_lock);
  sockets.clear();
  available.clear();
  cvar.signal();
  lock.unlock();
  close();
}


void async_request_socket::set_receive_poller(boost::function<bool()> fn) {
  receive_poller = fn;
}

int async_request_socket::request_master(zmq_msg_vector& msgs,
                                         zmq_msg_vector& ret,
                                         size_t timeout) {
  // find a free target to lock
  std::unique_lock<mutex> lock(global_lock);
  while(available.size() == 0 && sockets.size() > 0) {
    cvar.wait(lock);
  }
  if (sockets.size() == 0) {
    // no sockets available!
    return -1;
  }
  size_t wait_socket = available[available.size() - 1];
  available.pop_back();
  lock.unlock();
  create_socket(wait_socket);
  int rc = 0;
  // retry a few times
  for (size_t retry = 0; retry < 3; ++retry) {
    do {
      rc = msgs.send(sockets[wait_socket].z_socket, SEND_TIMEOUT);
    } while(rc == EAGAIN);
    if (rc == 0) break;
  }
  if (rc == 0) {
    timer ti;
    do {
      rc = ret.recv(sockets[wait_socket].z_socket, 1000);
      if (rc != 0 && receive_poller && receive_poller() == false) break;
      if (rc != 0 && timeout > 0 && ti.current_time() > timeout) break;
    } while(rc == EAGAIN);
  }
  // restore available socket
  lock.lock();
  available.push_back(wait_socket);
  cvar.signal();
  lock.unlock();

  // return
  if (rc != 0) {
    return rc;
  } else {
    return 0;
  }
}

int async_request_socket::create_socket(size_t i) {
  if (sockets[i].z_socket == -1) {
    sockets[i].z_socket = nn_socket(AF_SP, NN_REQ);
    int resendintl = 2147483647;
    int rc = nn_setsockopt(sockets[i].z_socket, NN_REQ, NN_REQ_RESEND_IVL , &(resendintl), sizeof(resendintl));
    assert(rc == 0);
    set_conservative_socket_parameters(sockets[i].z_socket);
    rc = nn_connect(sockets[i].z_socket, server.c_str());
    if (rc == -1) {
      print_zmq_error("Unexpected error on connection");
      return rc;
    }
  }
  return 0;
}

} // namespace nanosockets
}

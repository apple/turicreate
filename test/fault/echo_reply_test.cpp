/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#include <fault/sockets/socket_errors.hpp>
#include <fault/sockets/async_reply_socket.hpp>
#include <fault/sockets/socket_receive_pollset.hpp>
#include <zookeeper_util/key_value.hpp>
using namespace libfault;
size_t ctr = 0;
bool callback(zmq_msg_vector& recv,
              zmq_msg_vector& reply) {
  reply.clone_from(recv);
  ++ctr;
  return true;
}


int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout << "Usage: zookeeper_test [zkhost] [prefix] \n";
    return 0;
  }
  std::string zkhost = argv[1];
  std::string prefix = argv[2];
  std::vector<std::string> zkhosts; zkhosts.push_back(zkhost);
  std::string name = "echo";
  void* zmq_ctx = zmq_ctx_new();
  zmq_ctx_set(zmq_ctx, ZMQ_IO_THREADS, 4);
  turi::zookeeper_util::key_value key_value(zkhosts, prefix, name);
  async_reply_socket repsock(zmq_ctx,
                             &key_value,
                             callback);
  if(!repsock.register_key("echo")) {
    std::cout << "Unable to register the echo service. An echo service already exists\n";
  }
  socket_receive_pollset pollset;
  repsock.add_to_pollset(&pollset);
  pollset.start_poll_thread();
  std::cout << "Echo server running. Hit enter to quit\n";
  getchar();
  pollset.stop_poll_thread();
  repsock.close();
}

/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <iostream>
#include <string>
#include <cassert>
#include <fault/sockets/socket_errors.hpp>
#include <fault/sockets/subscribe_socket.hpp>
#include <fault/sockets/socket_receive_pollset.hpp>
using namespace libfault;
size_t ctr = 0;
bool callback(zmq_msg_vector& recv) {
  zmq_msg_t* msg = recv.read_next();
  std::string s((char*)zmq_msg_data(msg), zmq_msg_size(msg));
  std::cout << "Received: " << s << "\n";
  return true;
}


int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << "Usage: sub_test [pub_server]\n";
    return 0;
  }
  std::string pub_server = argv[1];
  std::string name = "echo";
  void* zmq_ctx = zmq_ctx_new();
  zmq_ctx_set(zmq_ctx, ZMQ_IO_THREADS, 4);
  subscribe_socket subsock(zmq_ctx,
                           NULL,
                           callback);
  socket_receive_pollset pollset;
  subsock.add_to_pollset(&pollset);
  pollset.start_poll_thread();
  subsock.connect(pub_server);
  std::cout << "Subscribe service running. Empty line to quit\n";
  // subscribe the empty topic
  std::string topic;
  subsock.subscribe(topic);
  while(1) {
    std::cout << "Prefix to Subscribe: ";
    std::string newtopic;
    std::getline(std::cin, newtopic);
    if (newtopic.length() == 0) break;
    subsock.unsubscribe(topic);
    subsock.subscribe(newtopic);
    topic = newtopic;
  }
  pollset.stop_poll_thread();
  subsock.close();
}

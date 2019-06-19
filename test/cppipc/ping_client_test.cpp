/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/system/cppipc/client/comm_client.hpp>
#include <iostream>
int main(int argc, char** argv) {
  //cppipc::comm_client client({"localhost:2181"}, "pingtest");
  cppipc::comm_client client({}, "tcp://127.0.0.1:19000");
  //cppipc::comm_client client({}, "ipc:///tmp/ping_server_test");
  client.start();
  std::cout << "Ping test. \"quit\" to quit\n";
  while(1) {
    std::string s;
    std::cin >> s;
    try {
      std::string ret = client.ping(s);
      std::cout << "pong: " << ret << "\n";
    } catch (cppipc::reply_status status) {
      std::cout << "Exception: " << cppipc::reply_status_to_string(status) << "\n";
    }
    if (s == "quit") break;
  }
}

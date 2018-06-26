/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <shmipc/shmipc.hpp>
#include <string.h>
#include <iostream>
using namespace turi;
int main(int argc, char** argv) {
  // ./shm_ping_server_test --help
  // or too many arguments
  if ((argc == 2 && strcmp(argv[1], "--help") == 0) || argc > 2) {
    std::cout << "Usage: " << argv[0] << " [ipc file name]\n";
    return 1;
  }
  shmipc::server server;

  if (argc >= 2) {
    server.bind(argv[1]);
  } else {
    server.bind();
  }
  std::cout << server.get_shared_memory_name() << std::endl;
  while(1) {
    bool ok = server.wait_for_connect(1);
    if (ok) break;
    else {
      std::cout << "timeout" << std::endl;
    }
  }
  std::cout << "Connected" << std::endl;
  char *c = nullptr;
  size_t len = 0;  
  while(1) {
    size_t receivelen = 0;
    server.receive_direct(&c, &len, receivelen, 10);
    if (receivelen >= 3) {
      if (strncmp(c, "end", 3) == 0) break;
    } 
    server.send(c, receivelen);
  }
}

/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#include <fault/query_object_client.hpp>

using namespace libfault;

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout << "Usage: echo_qo_test_client[zkhost] [prefix] \n";
    return 0;
  }
  std::string zkhost = argv[1];
  std::string prefix = argv[2];
  std::vector<std::string> zkhosts; zkhosts.push_back(zkhost);

  void* zmq_ctx = zmq_ctx_new();
  query_object_client client(zmq_ctx, zkhosts, prefix);
  std::cout << "[echotarget] [stuff]\n";
  std::cout << "An echotarget of \"q\" quits\n";

  std::cout << "\n\n\n";
  while(1) {
    std::string target;
    std::cin >> target;
    if (target == "q") break;
    std::string val;
    std::getline(std::cin, val);

    char* msg = (char*)malloc(val.length());
    memcpy(msg, val.c_str(), val.length());
    query_object_client::query_result result =
        client.update(target, msg, val.length());
    if (result.get_status() != 0) {
      std::cout << "\tError\n\n";
    } else {
      std::cout << "\tReply: " << result.get_reply() << "\n\n";
    }
    // this takes over the pointer
  }
}


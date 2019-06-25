/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/system/cppipc/cppipc.hpp>
#include <process/process_util.hpp>
#include "dummy_worker_interface.hpp"
#include <core/system/nanosockets/socket_config.hpp>
#include <thread>

using namespace turi;

class dummy_worker_obj : public dummy_worker_interface {
  public:
    std::string echo(const std::string& s) {
      return s;
    }
    void throw_error() {
      throw "error";
    }
    void quit(int exitcode=0) {
      exit(exitcode);
    }
};

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: ./dummy_worker ipc:///tmp/test_address" << std::endl;
    exit(1);
  }

  // Manually set this one.
  char* use_fallback = std::getenv("TURI_FORCE_IPC_TO_TCP_FALLBACK");
  if(use_fallback != nullptr && std::string(use_fallback) == "1") {
    nanosockets::FORCE_IPC_TO_TCP_FALLBACK = true;
  }

  size_t parent_pid = get_parent_pid();
  // Options
  std::string program_name = argv[0];
  std::string server_address = argv[1];

  // construct the server
  cppipc::comm_server server(std::vector<std::string>(), "", server_address);
  server.register_type<dummy_worker_interface>([](){ return new dummy_worker_obj(); });

  server.start();

  //Wait for parent that spawned me to die
  //TODO: I don't think we need to write any explicit code for this on Unix,
  //but we may on Windows. Revisit later.
  wait_for_parent_exit(parent_pid);
}

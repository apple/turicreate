/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <process/process_util.hpp>
#include <cstdlib>

namespace turi {

size_t get_parent_pid() {
  return (size_t)getppid();
}

size_t get_my_pid() {
  return (size_t)getpid();
}

void wait_for_parent_exit(size_t parent_pid) {
  while(1) {
    sleep(1);
    if (parent_pid != 0 && kill(parent_pid, 0) == -1) {
      break;
    }
  }
}

bool is_process_running(size_t pid) {
  return (kill(pid, 0) == 0);
}

boost::optional<std::string> getenv_str(const char* variable_name) {
  char* val = std::getenv(variable_name);
  if (val == nullptr) {
    return boost::optional<std::string>();
  } else {
    return boost::optional<std::string>(std::string(val));
  }
}

} // namespace turi

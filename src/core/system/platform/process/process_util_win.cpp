/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cross_platform/windows_wrapper.hpp>
#include <tlhelp32.h>
#include <process/process_util.hpp>
#include <core/logging/logger.hpp>

namespace turi {

  //TODO: This has a race condition that I'm ignoring as I don't think it will
  //happen much, if at all.  The parent could crash before the child gets a
  //chance to run this, and the PID reassigned to an unrelated process.  Since
  //stakes are not high (the child processes just won't get reaped for awhile
  //but won't be doing much of anything), I'm ignoring it.
  size_t get_parent_pid() {
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    PROCESSENTRY32 pe;
    ZeroMemory(&pe, sizeof(PROCESSENTRY32));
    pe.dwSize = sizeof(PROCESSENTRY32);

    auto my_pid = get_my_pid();
    size_t parent_pid = size_t(-1);

    // Iterate through all processes to find myself
    if(Process32First(h, &pe)) {
      do {
        if(pe.th32ProcessID == my_pid) {
          parent_pid = pe.th32ParentProcessID;
        }
      } while(Process32Next(h, &pe));
    }

    CloseHandle(h);

    return parent_pid;
  }

  size_t get_my_pid() {
    return (size_t)GetCurrentProcessId();
  }

  void wait_for_parent_exit(size_t parent_pid) {
    HANDLE parent_handle = OpenProcess(SYNCHRONIZE, FALSE, (DWORD)parent_pid);
    DWORD wait_ret = WAIT_TIMEOUT;
    while(wait_ret == WAIT_TIMEOUT) {
      // Wait 1 seconds for the parent to be signaled
      wait_ret = WaitForSingleObject(parent_handle, 1000);
    }
  }

bool is_process_running(size_t pid) {
  // Strategy taken from
  // http://stackoverflow.com/questions/1591342/c-how-to-determine-if-a-windows-process-is-running
  HANDLE proc = OpenProcess(SYNCHRONIZE, FALSE, (DWORD)pid);
  if(proc != NULL) {
    DWORD ret = WaitForSingleObject(proc, 0);
    if(proc != NULL)
      CloseHandle(proc);
    return (ret == WAIT_TIMEOUT);
  }
  return false;
}

boost::optional<std::string> getenv_str(const char* variable_name) {
  size_t bufsize = 65535;
  char buf[bufsize];
  size_t retsize = GetEnvironmentVariable(variable_name, buf, bufsize);
  if (retsize == 0) {
    return boost::optional<std::string>();
  } else if (retsize == bufsize) {
    logstream(LOG_WARNING) << "Environment variable " << variable_name << " exceeds max size" << std::endl;
    return boost::optional<std::string>();
  } else {
    return boost::optional<std::string>(std::string(buf, retsize));
  }
}

} // namespace turi

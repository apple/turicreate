/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <stdint.h>
/**
 * \defgroup minipsutil Mini Process Utilility Library
 */

/**
 * \ingroup minipsutil
 * Returns the total amount of physical memory on the system.
 * Returns 0 on failure.
 */
uint64_t total_mem();

/**
 * \ingroup minipsutil
 * Returns 1 if the pid is running, 0 otherwise.
 */
int32_t pid_is_running(int32_t pid);


#if defined(_WIN32)
/**************************************************************************/
/*                                                                        */
/*                                Windows                                 */
/*                                                                        */
/**************************************************************************/
#include <cross_platform/windows_wrapper.hpp>
#include <Psapi.h>

typedef BOOL (WINAPI *LPFN_GLPI)(
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,
    PDWORD);


/*
 * Return a Python integer indicating the total amount of physical memory
 * in bytes.
 */
uint64_t total_mem() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (! GlobalMemoryStatusEx(&memInfo) ) {
        return 0;
    }
    return memInfo.ullTotalPhys;
}


int32_t pid_is_running(int32_t pid)
{
    HANDLE hProcess;
    DWORD exitCode;

    // Special case for PID 0 System Idle Process
    if (pid == 0) {
        return 1;
    }

    if (pid < 0) {
        return 0;
    }

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                           FALSE, pid);
    if (NULL == hProcess) {
        // invalid parameter is no such process
        if (GetLastError() == ERROR_INVALID_PARAMETER) {
            CloseHandle(hProcess);
            return 0;
        }

        // access denied obviously means there's a process to deny access to...
        if (GetLastError() == ERROR_ACCESS_DENIED) {
            CloseHandle(hProcess);
            return 1;
        }

        CloseHandle(hProcess);
	// some kind of error
        return 1;
    }

    if (GetExitCodeProcess(hProcess, &exitCode)) {
        CloseHandle(hProcess);
        return (exitCode == STILL_ACTIVE);
    }

    // access denied means there's a process there so we'll assume
    // it's running
    if (GetLastError() == ERROR_ACCESS_DENIED) {
        CloseHandle(hProcess);
        return 1;
    }

    CloseHandle(hProcess);
    return 1;
}


#elif defined(__APPLE__)
/**************************************************************************/
/*                                                                        */
/*                                  Mac                                   */
/*                                                                        */
/**************************************************************************/
#include <sys/sysctl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>


uint64_t total_mem() {

    int      mib[2];
    uint64_t total;
    size_t   len = sizeof(total);

    // physical mem
    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    if (sysctl(mib, 2, &total, &len, NULL, 0)) {
        return 0;
    }

    return total;
}


int32_t pid_is_running(int32_t pid) {
    int kill_ret;

    // save some time if it's an invalid PID
    if (pid < 0) {
        return 0;
    }

    // if kill returns success of permission denied we know it's a valid PID
    kill_ret = kill(pid , 0);
    if ( (0 == kill_ret) || (EPERM == errno) ) {
        return 1;
    }

    // otherwise return 0 for PID not found
    return 0;
}


#else
/**************************************************************************/
/*                                                                        */
/*                                 Linux                                  */
/*                                                                        */
/**************************************************************************/
#include <sys/sysinfo.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>


uint64_t total_mem() {
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        return 0;
    }
    uint64_t total_mem_from_sysinfo = (uint64_t)info.totalram  * info.mem_unit;

    // we might be inside a container. check
    // /sys/fs/cgroup/memory/memory.stats
    FILE* f = fopen("/sys/fs/cgroup/memory/memory.stat", "r");
    if (f == NULL) return total_mem_from_sysinfo;

    uint64_t total_mem_from_cgroup = (uint64_t)(-1);
    while(1) {
      char key[64];
      unsigned long long value;
      // 63. one byte for null ptr
      int ret = fscanf(f, "%63s %llu", key, &value);
      if (ret != 2) break;
      if (strcmp(key, "hierarchical_memory_limit") == 0) {
        total_mem_from_cgroup = value;
        break;
      }
    }
    fclose(f);
    // return the minimum of system or cgroup
    if (total_mem_from_cgroup < total_mem_from_sysinfo) return total_mem_from_cgroup;
    else return total_mem_from_sysinfo;
}


int32_t pid_is_running(int32_t pid) {
    int kill_ret;

    // save some time if it's an invalid PID
    if (pid < 0) {
        return 0;
    }

    // if kill returns success of permission denied we know it's a valid PID
    kill_ret = kill(pid , 0);
    if ( (0 == kill_ret) || (EPERM == errno) ) {
        return 1;
    }

    // otherwise return 0 for PID not found
    return 0;
}

#endif

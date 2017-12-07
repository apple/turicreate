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
 * Returns the number of (Logical) CPUs.
 * Returns 0 on failure
 */
int32_t num_cpus();

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

/**
 * \ingroup minipsutil
 * Kill a process. Returns 1 on success, 0 on failure.
 */
int32_t kill_process(int32_t pid);


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

int32_t num_cpus() {
    LPFN_GLPI glpi;
    DWORD rc;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
    DWORD length = 0;
    DWORD offset = 0;
    int ncpus = 0;

    glpi = (LPFN_GLPI)GetProcAddress(GetModuleHandle(TEXT("kernel32")),
                                     "GetLogicalProcessorInformation");
    if (glpi == NULL)
        goto return_none;

    while (1) {
        rc = glpi(buffer, &length);
        if (rc == FALSE) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                if (buffer)
                    free(buffer);
                buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(
                    length);
                if (NULL == buffer) {
                    return 0;
                }
            }
            else {
                goto return_none;
            }
        }
        else {
            break;
        }
    }

    ptr = buffer;
    while (offset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= length) {
        if (ptr->Relationship == RelationProcessorCore)
            ncpus += 1;
        offset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
    }

    free(buffer);
    if (ncpus == 0)
        goto return_none;
    else
        return ncpus;

return_none:
    // mimic os.cpu_count()
    if (buffer != NULL)
        free(buffer);
    return 0;
}

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


int32_t kill_process(int32_t pid) {
    if (pid == 0) return 0;

    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == NULL) {
      return 0;
    }

    // kill the process
    if (! TerminateProcess(hProcess, 0)) {
        CloseHandle(hProcess);
        return 0;
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

/*
 * Return the number of logical CPUs in the system.
 * XXX this could be shared with BSD.
 */
int32_t num_cpus() {
  int mib[2];
  int ncpu;
  size_t len;

  mib[0] = CTL_HW;
  mib[1] = HW_NCPU;
  len = sizeof(ncpu);

  if (sysctl(mib, 2, &ncpu, &len, NULL, 0) == -1) {
    return 0;
  }
  else {
    return ncpu;
  }
}


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


int32_t kill_process(int32_t pid) {
    if (kill(pid, SIGKILL) == 0) return 1;
    else return 0;
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

int32_t num_cpus() {
  const char* physical_id = "physical id";
  int physical_id_len = strlen(physical_id);

  FILE* f = fopen("/proc/cpuinfo", "rb");
  if (f == NULL) return 0;

  char* c = NULL;
  size_t n = 0;
  int cpu_count = 0;
  while(1) {
    // read /proc/cpuinfo line by line looking for the string "physical id"
    ssize_t chars_read = getline(&c, &n, f);

    if (chars_read == -1) {
      // eof
      fclose(f);
      free(c);
      if (cpu_count == 0) return 0;  // fail
      return cpu_count;
    } else {
      // if c begins with the string physical id
      if (chars_read >= physical_id_len) {
        c[physical_id_len] = 0;
        if (strcmp(c, physical_id) == 0) ++cpu_count;
      }
    }
  }
}

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

int32_t kill_process(int32_t pid) {
    // In linux kill the negative of the pid will kill the process group
    if (kill(-pid, SIGKILL) == 0) return 1;
    else return 0;
}
#endif

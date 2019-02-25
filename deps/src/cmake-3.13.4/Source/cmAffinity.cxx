/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmAffinity.h"

#include "cm_uv.h"

#ifndef CMAKE_USE_SYSTEM_LIBUV
#  ifdef _WIN32
#    define CM_HAVE_CPU_AFFINITY
#    include <windows.h>
#  elif defined(__linux__) || defined(__FreeBSD__)
#    define CM_HAVE_CPU_AFFINITY
#    include <pthread.h>
#    include <sched.h>
#    if defined(__FreeBSD__)
#      include <pthread_np.h>
#      include <sys/cpuset.h>
#      include <sys/param.h>
#    endif
#    if defined(__linux__)
typedef cpu_set_t cm_cpuset_t;
#    else
typedef cpuset_t cm_cpuset_t;
#    endif
#  endif
#endif

namespace cmAffinity {

std::set<size_t> GetProcessorsAvailable()
{
  std::set<size_t> processorsAvailable;
#ifdef CM_HAVE_CPU_AFFINITY
  int cpumask_size = uv_cpumask_size();
  if (cpumask_size > 0) {
#  ifdef _WIN32
    DWORD_PTR procmask;
    DWORD_PTR sysmask;
    if (GetProcessAffinityMask(GetCurrentProcess(), &procmask, &sysmask) !=
        0) {
      for (int i = 0; i < cpumask_size; ++i) {
        if (procmask & (((DWORD_PTR)1) << i)) {
          processorsAvailable.insert(i);
        }
      }
    }
#  else
    cm_cpuset_t cpuset;
    CPU_ZERO(&cpuset); // NOLINT(clang-tidy)
    if (pthread_getaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) == 0) {
      for (int i = 0; i < cpumask_size; ++i) {
        if (CPU_ISSET(i, &cpuset)) {
          processorsAvailable.insert(i);
        }
      }
    }
#  endif
  }
#endif
  return processorsAvailable;
}
}

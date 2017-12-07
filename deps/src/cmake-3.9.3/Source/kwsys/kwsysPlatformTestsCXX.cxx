/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#ifdef TEST_KWSYS_CXX_HAS_CSTDIO
#include <cstdio>
int main()
{
  return 0;
}
#endif

#ifdef TEST_KWSYS_CXX_HAS_LONG_LONG
long long f(long long n)
{
  return n;
}
int main()
{
  long long n = 0;
  return static_cast<int>(f(n));
}
#endif

#ifdef TEST_KWSYS_CXX_HAS___INT64
__int64 f(__int64 n)
{
  return n;
}
int main()
{
  __int64 n = 0;
  return static_cast<int>(f(n));
}
#endif

#ifdef TEST_KWSYS_CXX_STAT_HAS_ST_MTIM
#include <sys/types.h>

#include <sys/stat.h>
#include <unistd.h>
int main()
{
  struct stat stat1;
  (void)stat1.st_mtim.tv_sec;
  (void)stat1.st_mtim.tv_nsec;
  return 0;
}
#endif

#ifdef TEST_KWSYS_CXX_STAT_HAS_ST_MTIMESPEC
#include <sys/types.h>

#include <sys/stat.h>
#include <unistd.h>
int main()
{
  struct stat stat1;
  (void)stat1.st_mtimespec.tv_sec;
  (void)stat1.st_mtimespec.tv_nsec;
  return 0;
}
#endif

#ifdef TEST_KWSYS_CXX_SAME_LONG_AND___INT64
void function(long**)
{
}
int main()
{
  __int64** p = 0;
  function(p);
  return 0;
}
#endif

#ifdef TEST_KWSYS_CXX_SAME_LONG_LONG_AND___INT64
void function(long long**)
{
}
int main()
{
  __int64** p = 0;
  function(p);
  return 0;
}
#endif

#ifdef TEST_KWSYS_IOS_HAS_ISTREAM_LONG_LONG
#include <iostream>
int test_istream(std::istream& is, long long& x)
{
  return (is >> x) ? 1 : 0;
}
int main()
{
  long long x = 0;
  return test_istream(std::cin, x);
}
#endif

#ifdef TEST_KWSYS_IOS_HAS_OSTREAM_LONG_LONG
#include <iostream>
int test_ostream(std::ostream& os, long long x)
{
  return (os << x) ? 1 : 0;
}
int main()
{
  long long x = 0;
  return test_ostream(std::cout, x);
}
#endif

#ifdef TEST_KWSYS_IOS_HAS_ISTREAM___INT64
#include <iostream>
int test_istream(std::istream& is, __int64& x)
{
  return (is >> x) ? 1 : 0;
}
int main()
{
  __int64 x = 0;
  return test_istream(std::cin, x);
}
#endif

#ifdef TEST_KWSYS_IOS_HAS_OSTREAM___INT64
#include <iostream>
int test_ostream(std::ostream& os, __int64 x)
{
  return (os << x) ? 1 : 0;
}
int main()
{
  __int64 x = 0;
  return test_ostream(std::cout, x);
}
#endif

#ifdef TEST_KWSYS_LFS_WORKS
/* Return 0 when LFS is available and 1 otherwise.  */
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _LARGE_FILES
#define _FILE_OFFSET_BITS 64
#include <sys/types.h>

#include <assert.h>
#include <sys/stat.h>
#if KWSYS_CXX_HAS_CSTDIO
#include <cstdio>
#endif
#include <stdio.h>

int main(int, char** argv)
{
/* check that off_t can hold 2^63 - 1 and perform basic operations... */
#define OFF_T_64 (((off_t)1 << 62) - 1 + ((off_t)1 << 62))
  if (OFF_T_64 % 2147483647 != 1)
    return 1;

  // stat breaks on SCO OpenServer
  struct stat buf;
  stat(argv[0], &buf);
  if (!S_ISREG(buf.st_mode))
    return 2;

  FILE* file = fopen(argv[0], "r");
  off_t offset = ftello(file);
  fseek(file, offset, SEEK_CUR);
  fclose(file);
  return 0;
}
#endif

#ifdef TEST_KWSYS_CXX_HAS_SETENV
#include <stdlib.h>
int main()
{
  return setenv("A", "B", 1);
}
#endif

#ifdef TEST_KWSYS_CXX_HAS_UNSETENV
#include <stdlib.h>
int main()
{
  unsetenv("A");
  return 0;
}
#endif

#ifdef TEST_KWSYS_CXX_HAS_ENVIRON_IN_STDLIB_H
#include <stdlib.h>
int main()
{
  char* e = environ[0];
  return e ? 0 : 1;
}
#endif

#ifdef TEST_KWSYS_CXX_HAS_GETLOADAVG
// Match feature definitions from SystemInformation.cxx
#if (defined(__GNUC__) || defined(__PGI)) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include <stdlib.h>
int main()
{
  double loadavg[3] = { 0.0, 0.0, 0.0 };
  return getloadavg(loadavg, 3);
}
#endif

#ifdef TEST_KWSYS_CXX_HAS_RLIMIT64
#if defined(KWSYS_HAS_LFS)
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _LARGE_FILES
#define _FILE_OFFSET_BITS 64
#endif
#include <sys/resource.h>
int main()
{
  struct rlimit64 rlim;
  return getrlimit64(0, &rlim);
}
#endif

#ifdef TEST_KWSYS_CXX_HAS_ATOLL
#include <stdlib.h>
int main()
{
  const char* str = "1024";
  return static_cast<int>(atoll(str));
}
#endif

#ifdef TEST_KWSYS_CXX_HAS_ATOL
#include <stdlib.h>
int main()
{
  const char* str = "1024";
  return static_cast<int>(atol(str));
}
#endif

#ifdef TEST_KWSYS_CXX_HAS__ATOI64
#include <stdlib.h>
int main()
{
  const char* str = "1024";
  return static_cast<int>(_atoi64(str));
}
#endif

#ifdef TEST_KWSYS_CXX_HAS_UTIMES
#include <sys/time.h>
int main()
{
  struct timeval* current_time = 0;
  return utimes("/example", current_time);
}
#endif

#ifdef TEST_KWSYS_CXX_HAS_UTIMENSAT
#include <fcntl.h>
#include <sys/stat.h>
int main()
{
  struct timespec times[2] = { { 0, UTIME_OMIT }, { 0, UTIME_NOW } };
  return utimensat(AT_FDCWD, "/example", times, AT_SYMLINK_NOFOLLOW);
}
#endif

#ifdef TEST_KWSYS_CXX_HAS_BACKTRACE
#if defined(__PATHSCALE__) || defined(__PATHCC__) ||                          \
  (defined(__LSB_VERSION__) && (__LSB_VERSION__ < 41))
backtrace doesnt work with this compiler or os
#endif
#if (defined(__GNUC__) || defined(__PGI)) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include <execinfo.h>
int main()
{
  void* stackSymbols[256];
  backtrace(stackSymbols, 256);
  backtrace_symbols(&stackSymbols[0], 1);
  return 0;
}
#endif

#ifdef TEST_KWSYS_CXX_HAS_DLADDR
#if (defined(__GNUC__) || defined(__PGI)) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
int main()
{
  Dl_info info;
  int ierr = dladdr((void*)main, &info);
  return 0;
}
#endif

#ifdef TEST_KWSYS_CXX_HAS_CXXABI
#if (defined(__GNUC__) || defined(__PGI)) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#if defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x5130 && __linux &&               \
  __SUNPRO_CC_COMPAT == 'G'
#include <iostream>
#endif
#include <cxxabi.h>
int main()
{
  int status = 0;
  size_t bufferLen = 512;
  char buffer[512] = { '\0' };
  const char* function = "_ZN5kwsys17SystemInformation15GetProgramStackEii";
  char* demangledFunction =
    abi::__cxa_demangle(function, buffer, &bufferLen, &status);
  return status;
}
#endif

#ifdef TEST_KWSYS_CXX_HAS_BORLAND_ASM
int main()
{
  int a = 1;
  __asm {
    xor EBX, EBX;
    mov a, EBX;
  }

  return a;
}
#endif

#ifdef TEST_KWSYS_CXX_HAS_BORLAND_ASM_CPUID
int main()
{
  int a = 0;
  __asm {
    xor EAX, EAX;
    cpuid;
    mov a, EAX;
  }

  return a;
}
#endif

#ifdef TEST_KWSYS_STL_HAS_WSTRING
#include <string>
void f(std::wstring*)
{
}
int main()
{
  return 0;
}
#endif

#ifdef TEST_KWSYS_CXX_HAS_EXT_STDIO_FILEBUF_H
#include <ext/stdio_filebuf.h>
int main()
{
  return 0;
}
#endif

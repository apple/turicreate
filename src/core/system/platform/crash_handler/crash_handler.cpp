/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef _WIN32
#include <execinfo.h>
#include <alloca.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <signal.h>

// The filename which we write backtrace to,
// default empty and we write to STDERR_FILENO
std::string BACKTRACE_FNAME = "";

/**
 * Dump back trace to file, refer to segfault.c in glibc.
 * https://sourceware.org/git/?p=glibc.git;a=blob;f=debug/segfault.c;
 */
void crit_err_hdlr(int sig_num, siginfo_t * info, void * ucontext) {
  void** array = (void**)alloca (256 * sizeof (void *));
  int size = backtrace(array, 256);

  int fd = STDERR_FILENO;
  if (!BACKTRACE_FNAME.empty()) {
    const char* fname = BACKTRACE_FNAME.c_str();
    fd = open (fname, O_TRUNC | O_WRONLY | O_CREAT, 0666);
    if (fd == -1)
      fd = STDERR_FILENO;
  }
  backtrace_symbols_fd(array, size, fd);
  close(fd);
  abort();
}
#endif

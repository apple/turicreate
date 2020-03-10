/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/system/platform/crash_handler/crash_handler.hpp>
#include <cstdlib>
#include <cstring>

void crash() {
  int *a = (int*)-1; // make a bad pointer
  printf("%d\n", *a);       // causes segfault
}

void bar() {
  crash();
}

void foo() {
  bar();
}

int main(int argc, char** argv) {
  struct sigaction sigact;
  sigact.sa_sigaction = crit_err_hdlr;
  sigact.sa_flags = SA_RESTART | SA_SIGINFO;
  if (sigaction(SIGSEGV, &sigact, (struct sigaction *)NULL) != 0)
  {
    fprintf(stderr, "error setting signal handler for %d (%s)\n",
        SIGSEGV, strsignal(SIGSEGV));
    exit(EXIT_FAILURE);
  }

  foo();

  return 0;
}

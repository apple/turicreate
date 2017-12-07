/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UTIL_CRASH_HANDLER_HPP
#define TURI_UTIL_CRASH_HANDLER_HPP

#include <string>
#include <signal.h>

// The filename which we write backtrace to, default empty and we write to STDERR_FILENO
extern std::string BACKTRACE_FNAME;

void crit_err_hdlr(int sig_num, siginfo_t * info, void * ucontext);

#endif

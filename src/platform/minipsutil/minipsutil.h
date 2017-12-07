/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_MINIPSUTIL_H
#define TURI_MINIPSUTIL_H
#include <stdint.h>
extern "C" {
/**
 * Returns the number of (Logical) CPUs.
 * Returns 0 on failure
 */
int32_t num_cpus();

/**
 * Returns the total amount of physical memory on the system.
 * Returns 0 on failure.
 */
uint64_t total_mem();

/**
 * Returns 1 if the pid is running, 0 otherwise. 
 */
int32_t pid_is_running(int32_t pid);

/**
 * Kill a process. Returns 1 on success, 0 on failure.
 */
int32_t kill_process(int32_t pid);
}
#endif

/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef PROCESS_UTIL_HPP
#define PROCESS_UTIL_HPP

#include<string>
#include<boost/optional.hpp>

namespace turi {

/**
 * \ingroup process_management
 * Get the PID of my parent process.
 */
size_t get_parent_pid();

/**
 * Get the PID of my current process.
 */
size_t get_my_pid();

/**
 * \ingroup process_management
 * Waits for a pid to exit.
 * (The function is misnamed. This will work for all PIDs not just my parent)
 */
void wait_for_parent_exit(size_t parent_pid);


/**
 * \ingroup process_management
 * Returns true if process is running
 */
bool is_process_running(size_t pid);

/**
 * \ingroup process_management
 * Returns the environment variable's value;
 * Note: on windows, the length of the return
 * value is limited to 65534.
 */
boost::optional<std::string> getenv_str(const char* variable_name);

} // namespace turi
#endif // PROCESS_UTIL_HPP

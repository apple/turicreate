/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef PROCESS_HPP
#define PROCESS_HPP

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#include <string>
#include <vector>
#include <core/util/syserr_reporting.hpp>

#ifdef _WIN32
#include <cross_platform/windows_wrapper.hpp>
#endif
#ifdef __APPLE__
#include <sys/types.h>
#endif
namespace turi
{

/**
 * \defgroup process_management Process Management
 * \brief Cross platform (Windows, Mac, Linux) sub-process management.
 */

/**
 * \ingroup process_management
 * \class process
 * Cross platform process launching, and management.
 */
class process {
 public:
  process() {};
  ~process();

  /**
   * A "generic" process launcher.
   *
   * Launched the command with the given arguments as a separate child process.
   *
   * This function does not throw.
   */
  bool launch(const std::string &cmd,
              const std::vector<std::string> &args);

  /**
   * A generic implementation of popen in read mode.
   *
   * This means that whatever the child writes on the given file descriptor
   * (target_child_write_fd) can be read by calling read_from_child. On Unix systems,
   * this could be any file descriptor inherited by the child from the parent.
   * On Windows, we only accept STDOUT_FILENO and STDERR_FILENO.
   *
   * NOTE: The STD*_FILENO constants are Unix specific, but defined in this
   * header for convenience.
   *
   * For instance,
   *   if target_child_write_fd == STDOUT_FILENO, when child writes to STDOUT,
   *   the parent can use read_from_child to read it.
   *
   *   if target_child_write_fd == STDERR_FILENO, when child writes to STDERR,
   *   the parent can use read_from_child to read it.
   *
   * if open_write_pipe == true, write_to_child can be used.
   *
   * This function does not throw.
   */
  bool popen(const std::string &cmd,
             const std::vector<std::string> &args,
             int target_child_write_fd,
             bool open_write_pipe=false);
  /**
   * If we've set up a way to read from the child, use this to read.
   *
   * Returns -1 on error, otherwise bytes received
   *
   * Throws if a way to read was not set up or if process was not launched.
   */
  ssize_t read_from_child(void *buf, size_t count);

  std::string read_from_child();

  /**
   * Writes to the child's stdin.
   *
   * Returns false on error.
   *
   * Throws if a way to read was not set up or if process was not launched.
   */
  bool write_to_child(const void *buf, size_t count);

  /**
   * Kill the launched process
   *
   * Throws if process was never launched.
   */
  bool kill(bool async=true);

  /**
   * Check if the process launched is running.
   *
   * Throws if process was never launched.
   */
  bool exists();

  /**
   * Return the process's return code if it has exited.
   *
   * Returns INT_MIN if the process is still running.
   * Returns INT_MAX if getting the error code failed for some other reason.
   */
  int get_return_code();

  void close_read_pipe();

  size_t get_pid();

  /**
   * Sets O_NONBLOCK on the process if the argument is true.
   */
  void set_nonblocking(bool nonblocking);

  /**
   * Mark that this process should be automatically reaped.
   * In which case, get_return_code() will not work.
   */
  void autoreap();
 private:
  //***Methods
  process(process const&) = delete;
  process& operator=(process const&) = delete;

#ifdef _WIN32
  //***Methods
  //***Variables

  // Handle needed to interact with the process
  HANDLE m_proc_handle = NULL;

  // Handle needed for parent to read from child
  HANDLE m_read_handle = NULL;

  // Handle needed for child to write to parent
  HANDLE m_write_handle = NULL;

  // Duplicate of handle so child can pipe stderr and stdout to console.
  HANDLE m_stderr_handle = NULL;
  HANDLE m_stdout_handle = NULL;

  DWORD m_pid = DWORD(-1);

  BOOL m_launched = FALSE;

  BOOL m_launched_with_popen = FALSE;
#else

  //***Variables
  // Handle needed to read/write to this process
  int m_read_handle = -1;
  int m_write_handle = -1;

  pid_t m_pid = 0;

  bool m_launched = false;

  bool m_launched_with_popen = false;
#endif
};

} // namespace turi
#endif //PROCESS_HPP

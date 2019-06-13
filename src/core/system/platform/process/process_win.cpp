/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <process/process.hpp>
#include <core/logging/logger.hpp>
#include <limits>

namespace turi
{

// Programmer responsible for calling delete[] on returned char*
char *convert_args(const std::string &cmd,
    const std::vector<std::string> &args) {
  // Convert argument list to a single string
  std::stringstream master_arg_list;
  master_arg_list << "\"" << cmd << "\" ";
  for(auto &i : args) {
    master_arg_list << "\"";
    master_arg_list << i;
    master_arg_list << "\"";
    master_arg_list << " ";
  }

  size_t c_arglist_sz = master_arg_list.str().size() + 1;
  char* c_arglist = new char[c_arglist_sz];
  memcpy(c_arglist, master_arg_list.str().c_str(), c_arglist_sz);

  return c_arglist;
}

bool process::launch(const std::string &cmd,
                     const std::vector<std::string> &args) {

  PROCESS_INFORMATION proc_info;
  STARTUPINFO startup_info;
  ZeroMemory(&proc_info, sizeof(PROCESS_INFORMATION));
  ZeroMemory(&startup_info, sizeof(STARTUPINFO));
  startup_info.cb = sizeof(startup_info);

  char *c_arglist = convert_args(cmd, args);

  logstream(LOG_INFO) << "Launching process using command: >>> " << c_arglist << " <<< " << std::endl;

  // Set up the proper handlers.  We are duplicating the handlers as
  // the given handles may or may not be inheritable.  DuplicateHandle
  // is (supposedly) the safest way to do this.
  startup_info.dwFlags |= STARTF_USESTDHANDLES;

  BOOL ret;

  // First, set up redirection for stdout.
  ret = DuplicateHandle( GetCurrentProcess(), GetStdHandle(STD_OUTPUT_HANDLE),
                         GetCurrentProcess(), &m_stdout_handle, 0, TRUE, DUPLICATE_SAME_ACCESS);

  if(!ret) {
    auto err = GetLastError();
    logstream(LOG_WARNING) << "Failed to duplicate stdout file handle: " << get_last_err_str(err)
                           << "; continuing with default handle." << std::endl;
    m_stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
  }

  startup_info.hStdOutput = m_stdout_handle;

  // Second, set up redirection for stderr.
  ret = DuplicateHandle( GetCurrentProcess(), GetStdHandle(STD_ERROR_HANDLE),
                         GetCurrentProcess(), &m_stderr_handle, 0, TRUE, DUPLICATE_SAME_ACCESS);

  if(!ret) {
    auto err = GetLastError();
    logstream(LOG_WARNING) << "Failed to duplicate stderr file handle: " << get_last_err_str(err)
                           << "; continuing with default handle." << std::endl;
    m_stderr_handle = GetStdHandle(STD_ERROR_HANDLE);
  }

  startup_info.hStdError = m_stderr_handle;

  // For Windows, we include the cmd with the arguments so that a search path
  // is used for any executable without a full path
  ret = CreateProcess(NULL,
      c_arglist, // command line
      NULL,      // process security attributes
      NULL,      // primary thread security attributes
      TRUE,      // handles are inherited
      CREATE_NO_WINDOW, // creation flags
      NULL,      // use parent's environment
      NULL,      // use parent's current directory
      &startup_info, // use parent's current directory
      &proc_info); // use parent's current directory

  if(!ret) {
    auto err = GetLastError();
    logstream(LOG_ERROR) << "Failed to launch process: " << get_last_err_str(err) << std::endl;
    delete[] c_arglist;
    return false;
  } else {

    // Don't need to have a thread handle. We'll just cancel the process if
    // need be.
    CloseHandle(proc_info.hThread);
    m_launched = true;
  }

  // Used for killing the process
  m_proc_handle = proc_info.hProcess;
  m_pid = proc_info.dwProcessId;

  logstream(LOG_INFO) << "Launched process with pid: " << m_pid << std::endl;

  // Wait up to 100 milliseconds before querying the process for it's status.
  DWORD dwMillisec = 100;
  DWORD dwWaitStatus = WaitForSingleObject(m_proc_handle, dwMillisec );

  if(dwWaitStatus == WAIT_FAILED) {
    auto err = GetLastError();
    logstream(LOG_WARNING) << "Error in WaitForSingleObject after CreateProcess: "
                           << get_last_err_str(err) << std::endl;
  }

  // Query the status of the process.  Will return STILL_ACTIVE if all
  // is good.
  DWORD potential_exit_code;
  ret = GetExitCodeProcess(m_proc_handle, &potential_exit_code);

  if(!ret) {
    auto err = GetLastError();
    logstream(LOG_WARNING) << "Error querying process status code: " << get_last_err_str(err) << std::endl;
  }

  logstream(LOG_INFO) << "Process status of " << m_pid << " = " << potential_exit_code << std::endl;

  if(potential_exit_code != STILL_ACTIVE) {
    logstream(LOG_ERROR) << "Launched process " << m_pid
                         << " exited immediately with error code " << potential_exit_code << std::endl;
    return false;
  }

  return true;
}

bool process::popen(const std::string &cmd,
                     const std::vector<std::string> &args,
                     int child_write_fd) {
  // We will only support stdout and stderr in Windows
  if(child_write_fd != STDOUT_FILENO && child_write_fd != STDERR_FILENO) {
    logstream(LOG_ERROR) << "Cannot read anything other than stdout or stderr "
      "from child on Windows." << std::endl;
    return false;
  }

  SECURITY_ATTRIBUTES sa_attr;
  sa_attr.nLength=sizeof(SECURITY_ATTRIBUTES);
  // Allow handles to be inherited when process created
  sa_attr.bInheritHandle = TRUE;
  sa_attr.lpSecurityDescriptor = NULL;


  if(!CreatePipe(&m_read_handle, &m_write_handle, &sa_attr, 0)) {
    //TODO: Figure out how to get error string on Windows
    logstream(LOG_ERROR) << "Failed to create pipe: " <<
      get_last_err_str(GetLastError()) << std::endl;

    return false;
  }

  // Make sure the parent end of the pipe is NOT inherited
  if(!SetHandleInformation(m_read_handle, HANDLE_FLAG_INHERIT, 0)) {
    logstream(LOG_ERROR) << "Failed to set handle information: " <<
      get_last_err_str(GetLastError()) << std::endl;
    return false;
  }

  PROCESS_INFORMATION proc_info;
  STARTUPINFO startup_info;
  ZeroMemory(&proc_info, sizeof(PROCESS_INFORMATION));
  ZeroMemory(&startup_info, sizeof(STARTUPINFO));

  if(m_read_handle != NULL) {
    startup_info.cb = sizeof(STARTUPINFO);
    if(child_write_fd == STDOUT_FILENO) {
      startup_info.hStdOutput = m_write_handle;
    } else if(child_write_fd == STDERR_FILENO) {
      startup_info.hStdError = m_write_handle;
    }
    startup_info.dwFlags |= STARTF_USESTDHANDLES;
  } else {
    logstream(LOG_ERROR) << "Read handle NULL after pipe created." << std::endl;
    return false;
  }

  char *c_arglist = convert_args(cmd, args);

  BOOL ret;
  // For Windows, we include the cmd with the arguments so that a search path
  // is used for any executable without a full path
  ret = CreateProcess(NULL,
      c_arglist,
      NULL,
      NULL,
      TRUE,
      0,
      NULL,
      NULL,
      &startup_info,
      &proc_info);


  if(!ret) {
    auto err = GetLastError();
    logstream(LOG_ERROR) << "Failed to launch process: " <<
      get_last_err_str(err) << std::endl;
    delete[] c_arglist;
    return false;

  } else {
    // Don't need to have a thread handle. We'll just cancel the process if
    // need be
    CloseHandle(proc_info.hThread);

    // Now that the process has been created, close the handle that was
    // inherited by the child.  Apparently if you DON'T do this, reading from
    // the child will never report an error when the child is done writing, and
    // you'll hang forever waiting for an EOF. There goes a few hours of my
    // life.
    CloseHandle(m_write_handle);
    m_write_handle = NULL;

    m_launched = TRUE;
    m_launched_with_popen = TRUE;
  }

  // Used for killing the process
  m_proc_handle = proc_info.hProcess;
  m_pid = proc_info.dwProcessId;

  logstream(LOG_INFO) << "Launched process with pid: " << m_pid << std::endl;

  return true;
}

ssize_t process::read_from_child(void *buf, size_t count) {
  if(!m_launched)
    log_and_throw("No process launched!");
  if(!m_launched_with_popen || m_read_handle == NULL)
    log_and_throw("Cannot read from child, no pipe initialized. "
        "Launch with popen to do this.");

  // Keep from overflowing
  if(count > std::numeric_limits<DWORD>::max()) {
    count = std::numeric_limits<DWORD>::max();
  }

  DWORD bytes_read;
  BOOL ret = ReadFile(m_read_handle, (LPWORD)buf, count, &bytes_read, NULL);
  if(!ret) {
    logstream(LOG_ERROR) << "ReadFile failed: " <<
      get_last_err_str(GetLastError()) << std::endl;
  }

  return ret ? ssize_t(bytes_read) : ssize_t(-1);
}

bool process::write_to_child(const void *buf, size_t count) {
  log_and_throw("Not Supported");
  return false;
}

void process::close_read_pipe() {
  if(!m_launched)
    log_and_throw("No process launched!");
  if(!m_launched_with_popen || m_read_handle == NULL)
    log_and_throw("Cannot close read pipe from child, no pipe initialized.");
  CloseHandle(m_read_handle);
  m_read_handle = NULL;
}

bool process::kill(bool async) {
  if(!m_launched)
    log_and_throw("No process launched!");

  if(m_proc_handle != NULL) {
    BOOL ret = TerminateProcess(m_proc_handle, 1);
    auto err_code = GetLastError();
    if(!async)
      WaitForSingleObject(m_proc_handle, 10000);
    CloseHandle(m_proc_handle);
    m_proc_handle = NULL;

    if(!ret) {
      logstream(LOG_INFO) << get_last_err_str(err_code);
      return false;
    }

    return true;
  }

  return false;
}

bool process::exists() {
  if(!m_launched)
    log_and_throw("No process launched!");

  if(m_proc_handle != NULL) {
    DWORD exit_code;
    auto rc = GetExitCodeProcess(m_proc_handle, &exit_code);
    return (rc && exit_code == STILL_ACTIVE);
  }

  return false;
}

int process::get_return_code() {
  DWORD exit_code;
  if(m_proc_handle != NULL) {
    auto rc = GetExitCodeProcess(m_proc_handle, &exit_code);
    if(!rc)
      return INT_MAX;
  }

  if(exit_code == STILL_ACTIVE)
    return INT_MIN;

  return exit_code;
}

size_t process::get_pid() {
  return size_t(m_pid);
}

process::~process() {
  if(m_proc_handle != NULL) {
    CloseHandle(m_proc_handle);
  }

  if(m_read_handle != NULL) {
    CloseHandle(m_read_handle);
  }

  if(m_write_handle != NULL) {
    CloseHandle(m_write_handle);
  }

  if(m_stderr_handle != NULL) {
    CloseHandle(m_stderr_handle);
  }

  if(m_stdout_handle != NULL) {
    CloseHandle(m_stdout_handle);
  }
}

// no op on windows
void process::autoreap() { }

// not implemented on windows
void process::set_nonblocking(bool nonblocking) {
  throw std::runtime_error("not implemented");
}

} //namespace turi

/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <process/process.hpp>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <climits>
#include <mutex>
#include <set>
#include <logger/logger.hpp>
#include <boost/filesystem.hpp>
#include <parallel/mutex.hpp>

namespace fs = boost::filesystem;

/*
 * We need to handle SIGCHLD here to support reaping of processes
 * marked for auto reaping. Basically, all we need to do really is to
 * loop through a set of procids registered by process::autoreap()
 * and call waitpid on them.
 *
 * The trick however is how to make this reentrant safe.
 *
 * To do so, the autoreap function must unregister the signal handler,
 * add the pid to the list of PIDs to reap, and then re-register the signal 
 * handler.
 */
turi::mutex sigchld_handler_lock;

// This is an intentional leak of raw pointer because it is used on program termination.
static std::set<size_t>* __proc_ids_to_reap;
static std::once_flag __proc_ids_to_reap_initialized;
static std::set<size_t>& get_proc_ids_to_reap() { 
  std::call_once(__proc_ids_to_reap_initialized, 
                 []() {
                  __proc_ids_to_reap = new std::set<size_t>(); 
                 });
  return *__proc_ids_to_reap;
}

static void sigchld_handler(int sig) {
  // loop through the set of stuff to reap
  // and try to reap them
  auto iter = __proc_ids_to_reap->begin();
  while (iter != __proc_ids_to_reap->end()) {
    if (waitpid(*iter, NULL, WNOHANG) > 0) {
      iter = __proc_ids_to_reap->erase(iter);
    } else {
      ++iter;
    }
  }
}
 
/**
 * Install the SIGCHLD handler. Should be called
 * with sigchld_handler_lock_acquired
 */
static void install_sigchld_handler() {
  struct sigaction act;
  memset (&act, 0, sizeof(act));
  act.sa_handler = sigchld_handler;
  sigaction(SIGCHLD, &act, 0);
}

/**
 * Uninstalls the SIGCHLD handler. Should be called
 * with sigchld_handler_lock_acquired
 */
static void uninstall_sigchld_handler() {
  struct sigaction act;
  memset (&act, 0, sizeof(act));
  act.sa_handler = SIG_DFL;
  sigaction(SIGCHLD, &act, 0);
}

namespace turi
{

static const char** convert_args(std::string &cmd, const std::vector<std::string> &args) {
  // Convert args to c_str list
  // Size of filename of cmd + args + NULL
  const char** c_arglist = new const char*[args.size() + 2];
  size_t i = 0;
  c_arglist[i++] = cmd.c_str();
  for (const auto& arg: args) {
    c_arglist[i++] = arg.c_str();
  }
  c_arglist[i] = NULL;

  // std::cerr << "stuff: " << cmd << std::endl;
  /*
   * for(size_t i = 0; c_arglist[i] != NULL; ++i) {
   *   std::cerr << c_arglist[i] << std::endl;
   * }
   */

  return c_arglist;
}

bool process::popen(const std::string &cmd,
                    const std::vector<std::string> &args,
                    int target_child_write_fd,
                    bool open_write_pipe) {
  // build pipe
  int parent_reader_pipe[2];
  int& parent_read_fd = parent_reader_pipe[0];
  int& child_write_fd = parent_reader_pipe[1];

  int parent_writer_pipe[2];
  int& child_read_fd = parent_writer_pipe[0];
  int& parent_write_fd = parent_writer_pipe[1];

  if (pipe(parent_reader_pipe)) {
    logstream(LOG_ERROR) << "Error building pipe for process launch: " <<
        get_last_err_str(errno);
  }

  if (open_write_pipe) {
    if (pipe(parent_writer_pipe)) {
      logstream(LOG_ERROR) << "Error building pipe for process launch: " <<
          get_last_err_str(errno);
    }
  }
  std::string cmd_tmp = cmd;
  const char **c_arglist = convert_args(cmd_tmp, args);

#ifdef __APPLE__
  pid_t pid = fork();
#else
  pid_t pid = vfork();
#endif

  if(pid < 0) {
    logstream(LOG_ERROR) << "Fail to fork process: " << strerror(errno) << std::endl;
    delete[] c_arglist;
    return false;
  } else if(pid == 0) {
    //***In child***
    close(parent_read_fd);
    if((target_child_write_fd > -1) && (child_write_fd != target_child_write_fd)) {
      errno = 0;
      if(dup2(child_write_fd, target_child_write_fd) != target_child_write_fd) {
        _exit(1);
      }
      close(child_write_fd);
    }

    if (open_write_pipe) {
      close(parent_write_fd);
      if(dup2(child_read_fd, STDIN_FILENO) != STDIN_FILENO) {
        _exit(1);
      }
      close(child_read_fd);
      
    }

    int exec_ret = execvp(&cmd[0], (char**)c_arglist);
    if(exec_ret == -1) {
      std::stringstream msg;
      msg << "Fail to exec command '" << cmd.c_str() << "': " << strerror(errno) << std::endl;
      log_and_throw(msg.str());
    }
    _exit(0);
  } else {
    m_launched = true;
    m_launched_with_popen = true;
    m_pid = pid;
    if(target_child_write_fd > -1) {
      m_read_handle = parent_read_fd;
    } else {
      close(parent_read_fd);
    }
    close(child_write_fd);
    if (open_write_pipe) {
      close(child_read_fd);
      m_write_handle = parent_write_fd;
    }
    delete[] c_arglist;
  }
  logstream(LOG_INFO) << "Launched process with pid: " << m_pid << std::endl;

  return true;
}

void process::set_nonblocking(bool nonblocking) {
  int flag = fcntl(m_read_handle, F_GETFL);
  if (nonblocking) {
    flag |= O_NONBLOCK;
  } else {
    flag ^= (flag & O_NONBLOCK);
  }
  fcntl(m_read_handle, F_SETFL, flag);
}

/**
 * A "generic" process launcher
 */
bool process::launch(const std::string &cmd,
                             const std::vector<std::string> &args) {
  std::string cmd_tmp = cmd;
  const char **c_arglist = convert_args(cmd_tmp, args);
#ifdef __APPLE__
  pid_t pid = fork();
#else
  pid_t pid = vfork();
#endif

  if(pid < 0) {
    logstream(LOG_ERROR) << "Fail to fork process: " << strerror(errno) << std::endl;
    delete[] c_arglist;
    return false;
  } else if(pid == 0) {
    int exec_ret = execvp(&cmd[0], (char**)c_arglist);
    if(exec_ret == -1) {
      std::cerr << "Fail to exec: " << strerror(errno) << std::endl;
    }
    _exit(0);
  } else {
    m_launched = true;
    m_pid = pid;
    delete[] c_arglist;
  }

  logstream(LOG_INFO) << "Launched process with pid: " << m_pid << std::endl;

  return true;
}

ssize_t process::read_from_child(void *buf, size_t count) {
  if(!m_launched)
    log_and_throw("No process launched!");
  if(!m_launched_with_popen)
    log_and_throw("Cannot read from process launched without a pipe!");
  if(m_read_handle == -1)
    log_and_throw("Cannot read from child, no pipe initialized. "
        "Specify target_child_write_fd on launch to do this.");
  return read(m_read_handle, buf, count);
}

bool process::write_to_child(const void *buf, size_t count) {
  if(!m_launched)
    log_and_throw("No process launched!");
  if(!m_launched_with_popen)
    log_and_throw("Cannot write to process launched without a pipe!");
  if(m_write_handle == -1)
    log_and_throw("Cannot write to child, no pipe initialized. "
        "You need to specify open_write_pipe == true.");

  const char* cbuf = (const char*)buf; 
  while(count > 0) {
    ssize_t cursent = write(m_write_handle, (void*)cbuf, count);
    if (cursent == -1) {
      return false;
    }
    count -= cursent;
    cbuf += cursent;
  }
  return true;
}

bool process::kill(bool async) {
  if(!m_launched)
    log_and_throw("No process launched!");

  ::kill(m_pid, SIGKILL);

  if(!async) {
    pid_t wp_rc = waitpid(m_pid, NULL, 0);
    if(wp_rc == -1) {
      auto err_str = get_last_err_str(errno);
      logstream(LOG_INFO) << "Cannot kill process: " << err_str << std::endl;
      return false;
    }
  }

  return true;
}

bool process::exists() {
  if(!m_launched)
    log_and_throw("No process launched!");
  int status;
  auto wp_ret = waitpid(m_pid, &status, WNOHANG);
  if(wp_ret == -1) {
    logstream(LOG_WARNING) << "Failed while checking for existence of process "
      << m_pid << ": " << strerror(errno) << std::endl;
  } else if(wp_ret == 0) {
    return true;
  }

  return false;
}

int process::get_return_code() {
  int status;
  auto wp_ret = waitpid(m_pid, &status, WNOHANG);
  if(wp_ret == -1) {
    return INT_MAX;
  } else if(wp_ret == 0) {
    return INT_MIN;
  } else if(wp_ret != m_pid) {
    return INT_MAX;
  }

  return WEXITSTATUS(status);
}

void process::close_read_pipe() {
  if(!m_launched)
    log_and_throw("No process launched!");
  if(!m_launched_with_popen)
    log_and_throw("Cannot close pipe from process when launched without a pipe!");
  if(m_read_handle == -1)
    log_and_throw("Cannot close pipe from child, no pipe initialized.");
  if(m_read_handle > -1) {
    close(m_read_handle);
    m_read_handle = -1;
  }
}

size_t process::get_pid() {
  return size_t(m_pid);
}

process::~process() {
  if(m_read_handle > -1) close(m_read_handle);
  if (m_write_handle > -1) close(m_write_handle);
}

void process::autoreap() {
  if (m_pid) {
    std::lock_guard<turi::mutex> guard(sigchld_handler_lock);
    uninstall_sigchld_handler();
    auto& proc_ids_to_reap = get_proc_ids_to_reap();
    proc_ids_to_reap.insert(m_pid);
    install_sigchld_handler();
  }
}

} // namespace turi

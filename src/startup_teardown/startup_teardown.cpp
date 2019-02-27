/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <sys/time.h>
#ifdef HAS_TCMALLOC
#include <google/malloc_extension.h>
#endif
#ifndef _WIN32
#include <sys/resource.h>
#endif
#ifndef _WIN32
#include <crash_handler/crash_handler.hpp>
#include <signal.h>
#endif
#include <Eigen/Core>
#include <globals/globals.hpp>
#include <globals/global_constants.hpp>
#include <sframe/sframe_constants.hpp>
#include <sframe/sframe_config.hpp>
#include <fileio/file_download_cache.hpp>
#include <fileio/fixed_size_cache_manager.hpp>
#include <fileio/temp_files.hpp>
#include <fileio/block_cache.hpp>
#include <parallel/thread_pool.hpp>
#include <logger/logger.hpp>
#include <logger/log_rotate.hpp>
#ifdef TC_HAS_PYTHON
#include <lambda/lambda_master.hpp>
#include <lambda/graph_pylambda_master.hpp>
#endif
#include <minipsutil/minipsutil.h>

#include "startup_teardown.hpp"

namespace turi {

// Global Variables
class memory_release_thread;
memory_release_thread* MEMORY_RELEASE_THREAD;

/**************************************************************************/
/*                                                                        */
/*                             Helper functions                           */
/*                                                                        */
/**************************************************************************/
/**
 * Attempts to increase the file handle limit.
 * Returns true on success, false on failure.
 */
bool upgrade_file_handle_limit(size_t limit) {
#ifndef _WIN32
  struct rlimit rlim;
  rlim.rlim_cur = limit;
  rlim.rlim_max = limit;
  return setrlimit(RLIMIT_NOFILE, &rlim) == 0;
#else
  return true;
#endif
}

/**
 * Gets the current file handle limit.
 * Returns the current file handle limit on success, 
 * -1 on infinity, and 0 on failure.
 */
int get_file_handle_limit() {
#ifndef _WIN32
  struct rlimit rlim;
  int ret = getrlimit(RLIMIT_NOFILE, &rlim);
  if (ret != 0) return 0;
  return int(rlim.rlim_cur);
#else
  return 4096;
#endif
}

void install_sighandlers() {
#ifdef _WIN32
  // Make sure dialog boxes don't come up for errors (apparently doesn't affect
  // "hard system errors")
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);

  // Don't listen to ctrl-c.  On Windows, a ctrl-c is delivered to every
  // application "sharing" the console that is selected with the mouse. This
  // causes unity_server to crash even though the client handles it correctly,
  // unless we disable ctrl-c events.
  SetConsoleCtrlHandler(NULL, true);
#endif
}


#ifdef HAS_TCMALLOC
/**
 *  If TCMalloc is available, we try to release memory back to the
 *  system every 15 seconds or so. TCMalloc is otherwise somewhat...
 *  aggressive about keeping memory around.
 */
class memory_release_thread {
  public:
  void start() {
    memory_release_thread.launch([this]() { this->memory_release_loop(); });
  }
  void stop() {
    stop_memory_release_thread = true;
    memory_release_cond.signal();
    memory_release_thread.join();
  }

  private:
  void memory_release_loop() {
    memory_release_lock.lock();
    while (!stop_memory_release_thread) {
      memory_release_cond.timedwait(memory_release_lock, 15);
      MallocExtension::instance()->ReleaseFreeMemory();
    }
    memory_release_lock.unlock();
  }
  private:
  turi::thread memory_release_thread;
  bool stop_memory_release_thread = false;
  turi::mutex memory_release_lock;
  turi::conditional memory_release_cond;
};
#else
class memory_release_thread {
  public:
    void start() { }
    void stop() { }
};
#endif

void configure_global_environment(std::string argv0) {
  // file limit upgrade has to be the very first thing that happens. The 
  // reason is that on Mac, once a file descriptor has been used (even STDOUT),
  // the file handle limit increase will appear to work, but will in fact fail
  // silently.
  upgrade_file_handle_limit(4096);
  int file_handle_limit = get_file_handle_limit();
  if (file_handle_limit < 4096) {
    logstream(LOG_WARNING) 
        << "Unable to raise the file handle limit to 4096. "
        << "Current file handle limit = " << file_handle_limit << ". "
        << "You may be limited to frames with about " << file_handle_limit / 16 
        << " columns" << std::endl;
  }
  // if file handle limit is >= 512,
  //    we take either 3/4 of the limit
  // otherwise
  //    we keep it to 128
  if (file_handle_limit >= 512) {
    size_t practical_limit = file_handle_limit / 4 * 3;
    turi::SFRAME_FILE_HANDLE_POOL_SIZE = practical_limit;
  } else {
    turi::SFRAME_FILE_HANDLE_POOL_SIZE = 128;
  }

  turi::SFRAME_DEFAULT_NUM_SEGMENTS = turi::thread::cpu_count();
  turi::SFRAME_MAX_BLOCKS_IN_CACHE = 16 * turi::thread::cpu_count();
  turi::SFRAME_SORT_MAX_SEGMENTS =
      std::max(turi::SFRAME_SORT_MAX_SEGMENTS, turi::SFRAME_FILE_HANDLE_POOL_SIZE / 4);
  // configure all memory constants
  // use up at most half of system memory.
  size_t total_system_memory = total_mem();
  total_system_memory /= 2;
  boost::optional<std::string> envval = getenv_str("DISABLE_MEMORY_AUTOTUNE");
  bool disable_memory_autotune = ((bool)envval) && (std::string(*envval) == "1");

  
  // memory limit
  envval = getenv_str("TURI_MEMORY_LIMIT_IN_MB");
  if (envval) {
    size_t limit = atoll((*envval).c_str()) * 1024 * 1024; /* MB */
    if (limit == 0) {
      logstream(LOG_WARNING) << "TURI_MEMORY_LIMIT_IN_MB environment "
                                "variable cannot be parsed" << std::endl;
    } else {
      total_system_memory = limit;
    }
  }

  if (total_system_memory > 0 && !disable_memory_autotune) {
    // TODO: MANY MANY HEURISTICS 
    // assume we have 1/2 of working memory to do things like sort, join, etc.
    // and the other 1/2 of working memory goes to file caching
    // HUERISTIC 1: Cell size estimate is 64
    // HUERISTIC 2: Row size estimate is Cell size estimate * 5
    //
    // Also, we only allow upgrades on the existing conservative values when
    // duing these estimates to prevent us from having impractically small 
    // values.
    size_t CELL_SIZE_ESTIMATE = 64;
    size_t ROW_SIZE_ESTIMATE = CELL_SIZE_ESTIMATE * 5;
    size_t max_cell_estimate = total_system_memory / 4 / CELL_SIZE_ESTIMATE;
    size_t max_row_estimate = total_system_memory / 4 / ROW_SIZE_ESTIMATE;

    turi::SFRAME_GROUPBY_BUFFER_NUM_ROWS = max_row_estimate;
    turi::SFRAME_JOIN_BUFFER_NUM_CELLS = max_cell_estimate;
    turi::sframe_config::SFRAME_SORT_BUFFER_SIZE = total_system_memory / 4;
    turi::fileio::FILEIO_MAXIMUM_CACHE_CAPACITY_PER_FILE = total_system_memory / 2;
    turi::fileio::FILEIO_MAXIMUM_CACHE_CAPACITY = total_system_memory / 2;
  }
  turi::globals::initialize_globals_from_environment(argv0);


  // force initialize rng
  turi::random::get_source();
}


/**************************************************************************/
/*                                                                        */
/*                            Global Startup                              */
/*                                                                        */
/**************************************************************************/
void global_startup::perform_startup() {
  if (startup_performed) return;
  startup_performed = true;
  // non turicreate stuff
  Eigen::initParallel();
  install_sighandlers();
  MEMORY_RELEASE_THREAD = new memory_release_thread();
  MEMORY_RELEASE_THREAD->start();
  // turicreate stuff
  turi::reap_unused_temp_files();
}

global_startup::~global_startup() { }

namespace startup_impl {
// we use an externed global variable so that only one occurance of this 
// object shows up after many shared library linkings.
global_startup startup_instance;
} // startup_impl

global_startup& global_startup::get_instance() {
  return startup_impl::startup_instance;
}

/**************************************************************************/
/*                                                                        */
/*                            Global Teardown                             */
/*                                                                        */
/**************************************************************************/

void global_teardown::perform_teardown() {
  if (teardown_performed) {
    logstream(LOG_WARNING) << "Teardown already performed" << std::endl;
    return;
  }
  teardown_performed = true;
  logstream(LOG_INFO) << "Performing teardown" << std::endl;
  try {
#ifdef TC_HAS_PYTHON
    turi::lambda::lambda_master::shutdown_instance();
    turi::lambda::graph_pylambda_master::shutdown_instance();
#endif
    MEMORY_RELEASE_THREAD->stop();
    delete MEMORY_RELEASE_THREAD;
    turi::fileio::fixed_size_cache_manager::get_instance().clear();
#ifdef TC_ENABLE_REMOTEFS
    turi::file_download_cache::get_instance().clear();
#endif
    turi::block_cache::release_instance();
    turi::reap_current_process_temp_files();
    turi::reap_unused_temp_files();
    turi::stop_log_rotation();
    turi::thread_pool::release_instance();
    turi::timer::stop_approx_timer();
  } catch (...) {
    std::cerr << "Exception on teardown." << std::endl;
  }
  logstream(LOG_INFO) << "Teardown complete" << std::endl;
}

global_teardown::~global_teardown() { }

namespace teardown_impl {
// we use an externed global variable so that only one occurance of this 
// object shows up after many shared library linkings.
global_teardown teardown_instance;
} // teardown_impl

global_teardown& global_teardown::get_instance() {
  return teardown_impl::teardown_instance;
}

} // turicreate

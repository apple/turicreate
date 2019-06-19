/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <thread>
#include <core/parallel/atomic.hpp>
#include <core/logging/logger.hpp>
#include <core/logging/assertions.hpp>
#include <shmipc/shmipc.hpp>
#include <shmipc/shmipc_garbage_collect.hpp>
#include <process/process_util.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION <= 105600 && defined(_WIN32)
#define BOOST_INTERPROCESS_WIN32_PRIMITIVES_HPP
#include "boost_alt_winapi.h"
#endif

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <core/parallel/mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/thread/thread_time.hpp>
namespace turi {
namespace shmipc {
using boost::interprocess::interprocess_mutex;
using boost::interprocess::interprocess_condition;
using boost::interprocess::read_write;
using boost::interprocess::open_only;
using boost::interprocess::open_or_create;
using boost::interprocess::create_only;
using boost::interprocess::shared_memory_object;
using boost::interprocess::mapped_region;
using boost::interprocess::scoped_lock;
using boost::posix_time::second_clock;
using boost::posix_time::seconds;

struct shared_memory_buffer {
  shared_memory_buffer() = default;

  // Semaphores to protect and synchronize access
  interprocess_mutex m_lock;

  // This semaphore counts the number of messages coming from client
  // to server. Server waits on this semaphore.
  // This probably is optimally a condition, however, the interprocess
  // condition variable says:
  //
  // \paragraph
  // "Unlike std::condition_variable in C++11, it is NOT safe to invoke the
  // destructor if all threads have been only notified. It is required that
  // they have exited their respective wait functions."
  // Which is somewhat problematic.
  //
  // Hence we just use a semaphore  and be a little more careful with the
  // mutex.

  struct semaphore_count_pair {
    interprocess_condition cond;
    size_t sender_pid;
    size_t count = 0;
  };

  semaphore_count_pair m_client_to_server;
  semaphore_count_pair m_server_to_client;

  bool terminating = false; // termination

  // when both of these are true, the connection is established
  bool client_connecting = false; // true if client is connecting
  bool server_listening = false; // true if server is listening

  size_t m_buffer_content_size = 0;

  // buffer size
  size_t m_buffer_size;

  // variable length buffer.
  char m_buffer[0];
};

/*
 * If ipcfile is not specified, this is used to generate a unique counter
 * of the sort [pid]_[counter]
 */

static atomic<size_t> SERVER_IPC_COUNTER;
bool server::bind(const std::string& ipcfile, size_t buffer_size) {
  logstream(LOG_INFO) << "Server attaching to " << ipcfile << " " << buffer_size << std::endl;
  size_t _run_progress = 0;
  try {
    m_shmname = ipcfile;
    if (m_shmname.empty()) {
      std::stringstream strm;
      strm << get_my_pid() << "_" << SERVER_IPC_COUNTER.inc();
      m_shmname= strm.str();
    }

    // sets up the raii deleter so that we eventually
    // delete this file
    _run_progress = 1;
    m_ipcfile_deleter = register_shared_memory_name(m_shmname);


    // this is really modified from code in
    // http://www.boost.org/doc/libs/1_58_0/doc/html/interprocess/synchronization_mechanisms.html
    _run_progress = 2;
    m_shared_object.reset(new shared_memory_object(create_only,
                                                   m_shmname.c_str(),
                                                   read_write));

    //Set size
    _run_progress = 3;
    m_shared_object->truncate(buffer_size + sizeof(shared_memory_buffer));

    //Map the whole shared memory in this process
    _run_progress = 4;
    m_mapped_region.reset(new mapped_region(*m_shared_object,
                                            read_write));

    _run_progress = 5;
    void* buffer = m_mapped_region->get_address();
    // placement new. put the data in the buffer

    _run_progress = 6;
    m_buffer = new (buffer) shared_memory_buffer;
    m_buffer->m_buffer_size = buffer_size;
    m_buffer->m_server_to_client.sender_pid = get_my_pid();

  } catch (const std::string& error) {
    logstream(LOG_ERROR) << "SHMIPC initialization Error (1), stage "
                         << _run_progress << ": " << error << std::endl;
    return false;

  } catch (const std::exception& error) {
    logstream(LOG_ERROR) << "SHMIPC initialization Error (2), stage "
                         << _run_progress << ": " << error.what() << std::endl;
    return false;

  } catch (...) {
    logstream(LOG_ERROR) << "Unknown SHMIPC Initialization Error, stage "
                         << _run_progress << "." << std::endl;
    return false;
  }

  return true;
}

std::string server::get_shared_memory_name() const {
  return m_shmname;
}

/**
 * Little utility wrapper around a condition variable wait.
 * If has_timeout is true, timed_wait will be used. Otherwise the regular wait
 * will be used.
 */
template <typename Guard, typename CondVar>
void condvar_wait(Guard& guard, CondVar& condvar,
          bool has_timeout, boost::system_time timeout_time = boost::system_time()) {
  if (has_timeout) {
    condvar.timed_wait(guard, timeout_time);
  } else {
    condvar.wait(guard);
  }
}

/**
 * A generic receiver used for both client to server and server to client
 * communications.
 *
 * \param c A pointer to a pointer to the output buffer to store the message.
 *          (*c) may be null. The buffer at (*c) will be resized as required.
 *          if c == NULL, it is ignored.
 *
 * \param clen (*clen) The current length of (*c). The receive call may resize
 *          change this value.
 *
 * \param receivelen The received length will be stored here.
 *
 * \param region A pointer to the shared memory region
 *
 * \param semcount The semaphore to wait on. For instance, for client->server
 *                 communication this will be region->m_client_to_server.
 *
 * \param timeout The maximum number of seconds to wait for a timeout.
 *                Can be "-1" in which we will wait forever.
 */
bool generic_receiver(char** c, size_t* clen,
                      size_t& receivelen,
                      shared_memory_buffer* region,
                      shared_memory_buffer::semaphore_count_pair& semcount,
                      size_t timeout) {
  boost::system_time timeout_time = boost::get_system_time();
  bool has_timeout = (timeout != (size_t)(-1));
  if (has_timeout) timeout_time += seconds(timeout);

  scoped_lock<interprocess_mutex> guard(region->m_lock);
  while (1) {
    boost::system_time cur_time = boost::get_system_time();
    if (region->terminating) return false;
    // if there are objects to be received
    if (semcount.count > 0) {
      // reallocate receiving buffer if necessary
      if (clen != nullptr && c != nullptr && (*clen) < region->m_buffer_content_size) {
        (*c) = (char*)realloc(c, region->m_buffer_content_size);
        (*clen) = region->m_buffer_content_size;
      }

      // set the length and copy the region into the buffer
      receivelen = region->m_buffer_content_size;
      region->m_buffer_content_size = 0;
      if (region->m_buffer_content_size > 0 && c != nullptr) {
        memcpy((*c), region->m_buffer, region->m_buffer_content_size);
      }
      --semcount.count;
//       std::cout << "Receiving " << receivelen << std::endl;
      return true;
    }
    if (has_timeout && cur_time > timeout_time) break;
    if (is_process_running(semcount.sender_pid) == false) break;
    if (has_timeout == false) {
      // even when there is no timeout, we still wait for 3 second timeout
      // and recheck the process id of the sender
      timeout_time = boost::get_system_time() + seconds(3);
    }
    condvar_wait(guard, semcount.cond, true, timeout_time);
  }
  return false;
}

bool server::wait_for_connect(size_t timeout) {
  ASSERT_TRUE(m_buffer != nullptr);
  logstream(LOG_INFO) << "Server Waiting for connection at " << m_shmname << std::endl;

  // wait for a connection for a certain amount of time
  boost::system_time timeout_time = boost::get_system_time();
  bool has_timeout = (timeout != (size_t)(-1));
  if (has_timeout) timeout_time += seconds(timeout);

  // we flag server_listening and we wait on the
  // condition variable for the client to show up.
  scoped_lock<interprocess_mutex> guard(m_buffer->m_lock);
  m_buffer->server_listening = true;
  m_buffer->m_server_to_client.cond.notify_all();
  while (1) {
    boost::system_time cur_time = boost::get_system_time();
    if (m_buffer->client_connecting) break;
    if (has_timeout && cur_time > timeout_time) break;
    condvar_wait(guard, m_buffer->m_client_to_server.cond,
                 has_timeout, timeout_time);
  }
  // we are connected if both the client has shown up
  // and we are listening
  bool connected = m_buffer->client_connecting && m_buffer->server_listening;

  if (connected) {
    // unlink the shared memory segment so as to minimize leakage potential
    m_ipcfile_deleter.reset();
    logstream(LOG_INFO) << "Server connection successful at " << m_shmname << std::endl;
  } else {
    // failed to connect, reset listening and returning
    logstream(LOG_INFO) << "Server connection timeout at " << m_shmname << std::endl;
    m_buffer->server_listening = false;
  }
  return connected;
}

size_t server::buffer_size() const {
  if (m_buffer == nullptr) {
    return 0;
  } else {
    return m_buffer->m_buffer_size;
  }
}

bool server::send(const char* c, size_t len) {
  // if buffer is empty, or write is too large, fail
  if (m_buffer == nullptr || len > m_buffer->m_buffer_size) return false;

  scoped_lock<interprocess_mutex> guard(m_buffer->m_lock);
  if (m_buffer->m_buffer_content_size != 0) return false;
  if (c && len > 0) memcpy(m_buffer->m_buffer, c, len);
  m_buffer->m_buffer_content_size = len;
  ++m_buffer->m_server_to_client.count;
  m_buffer->m_server_to_client.cond.notify_all();
//   std::cout << "Server sending " << len << std::endl;
  return true;
}

bool server::receive(char**c, size_t* len, size_t& receivelen, size_t timeout) {
  receivelen = 0;
  return generic_receiver(c, len, receivelen,
                          m_buffer, m_buffer->m_client_to_server, timeout);
}

bool server::receive_direct(char**c, size_t* len, size_t& receivelen, size_t timeout) {
  receivelen = 0;
  bool ret = generic_receiver(nullptr, nullptr, receivelen,
                              m_buffer, m_buffer->m_client_to_server, timeout);
  if (c) (*c) = m_buffer->m_buffer;
  if (len) (*len) = receivelen;
  return ret;
}

void server::shutdown() {
  // set terminating and wake up the semaphore
  if (m_buffer && m_buffer->terminating == false) {
    m_buffer->terminating = true;
    m_buffer->m_client_to_server.cond.notify_all();
    m_buffer->m_server_to_client.cond.notify_all();
  }
}

server::~server() {
  shutdown();
}

bool client::connect(std::string ipcfile, size_t timeout) {
  logstream(LOG_INFO) << "Client connecting to " << ipcfile << std::endl;
  // this is really modified from code in
  // http://www.boost.org/doc/libs/1_58_0/doc/html/interprocess/synchronization_mechanisms.html
  m_shared_object.reset(new shared_memory_object(open_only,
                                                 ipcfile.c_str(),
                                                 read_write));

  //Map the whole shared memory in this process
  m_mapped_region.reset(new mapped_region(*m_shared_object,
                                          read_write));
  void* buffer = m_mapped_region->get_address();
  if (buffer == nullptr) return false;
  // placement new. put the data in the buffer
  m_buffer = reinterpret_cast<shared_memory_buffer*>(buffer);


  // wait for the server for a certain amount of time
  boost::system_time timeout_time = boost::get_system_time();
  bool has_timeout = (timeout != (size_t)(-1));
  if (has_timeout) timeout_time += seconds(timeout);

  // we flag server_listening and we wait on the
  // condition variable for the client to show up.
  scoped_lock<interprocess_mutex> guard(m_buffer->m_lock);
  m_buffer->client_connecting = true;
  m_buffer->m_client_to_server.cond.notify_all();
  while (1) {
    boost::system_time cur_time = boost::get_system_time();
    if (has_timeout && cur_time > timeout_time) break;
    if (m_buffer->server_listening) break;
    condvar_wait(guard, m_buffer->m_server_to_client.cond,
                 has_timeout, timeout_time);
  }
  // we are connected if both the client has shown up
  // and we are listening
  bool connected = m_buffer->client_connecting && m_buffer->server_listening;

  // failed to connect, reset connecting and return
  if (!connected) {
    m_buffer->client_connecting = false;
    logstream(LOG_INFO) << "Client connection to " << ipcfile << " timeout" << std::endl;
  } else {
    m_buffer->m_client_to_server.sender_pid = get_my_pid();
    logstream(LOG_INFO) << "Client connection to " << ipcfile << " successful" << std::endl;
  }
  return connected;
}


size_t client::buffer_size() const {
  if (m_buffer == nullptr) {
    return 0;
  } else {
    return m_buffer->m_buffer_size;
  }
}

bool client::send(const char* c, size_t len) {
  // if buffer is empty, or write is too large, fail
  if (m_buffer == nullptr || len > m_buffer->m_buffer_size) return false;

  scoped_lock<interprocess_mutex> guard(m_buffer->m_lock);
  if (m_buffer->m_buffer_content_size != 0) return false;
  if (c && len > 0) memcpy(m_buffer->m_buffer, c, len);
  m_buffer->m_buffer_content_size = len;
  ++m_buffer->m_client_to_server.count;
  m_buffer->m_client_to_server.cond.notify_all();
//   std::cout << "Client sending " << len << std::endl;
  return true;
}

bool client::receive(char**c, size_t* len, size_t& receivelen, size_t timeout) {
  receivelen = 0;
  return generic_receiver(c, len, receivelen,
                          m_buffer, m_buffer->m_server_to_client, timeout);
}

bool client::receive_direct(char**c, size_t* len, size_t& receivelen, size_t timeout) {
  receivelen = 0;
  bool ret = generic_receiver(nullptr, nullptr, receivelen,
                              m_buffer, m_buffer->m_server_to_client, timeout);
  if (c) (*c) = m_buffer->m_buffer;
  if (len) (*len) = receivelen;
  return ret;
}
} // shmipc
} // turicreate

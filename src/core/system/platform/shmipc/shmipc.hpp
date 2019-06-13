/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SHMIPC_HPP
#define TURI_SHMIPC_HPP
#include <string>
#include <cstddef>
#include <memory>
#include <core/storage/fileio/fs_utils.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <core/logging/logger.hpp>
namespace turi {
namespace shmipc {

struct shared_memory_buffer;
struct raii_deleter;
/**
 * \defgroup shmipc Shared Memory Interprocess Communication
 */

/**
 * \ingroup shmipc
 * The SHM IPC server/client defines a simple unsynchronized single server /
 * single client communication system over interprocess shared memory.
 *
 * The communication is mostly unsynchronized between server and client, so
 * users of the server/client implementations have to be careful about who
 * is sending and who is receiving.
 *
 * The class uses Posix Shared Memory segments. Essentially you define a
 * "name" (on Linux this name shows up in /dev/shm, on Mac this is unfortunately
 * not enumerable).
 *
 * Within the shared memory segment is essentially a buffer, and a pair of
 * condition variables used to wake a client receiver, or a server receiver.
 *
 * The server creates a name, and a size and waits for a client to connect to
 * it. Once a client connects, the shared memory segment is deleted (unlink).
 * This means that once both server and client terminate (or crash), the shared
 * memory segment is released. (otherwise the named segment will hang around
 * until a reboot).
 *
 * However, this does mean that program crash prior to connection can result in
 * leaked segments. And that is bad. Hence we need a "garbage collection"
 * mechanism.
 */
class server {
 public:
  server() = default;
  ~server();
  /**
   * Binds a server to an ipc name. This SHM name will show up in
   * /dev/shm on linux machines. Every server must bind to a different file.
   * If ipcfile is an empty string, a file is automatically constructed and
   * \ref get_shared_memory_name() can be used to get the shared memory name.
   */
  bool bind(const std::string& ipcfile = "",
            size_t buffer_size = 1024*1024);

  /**
   * Returns the maximum amount of data that can be sent or received.
   */
  size_t buffer_size() const;

  /**
   * Returns the shared memory object name.
   */
  std::string get_shared_memory_name() const;

  /**
   * Waits up to timeout seconds for a connection.
   */
  bool wait_for_connect(size_t timeout = 10);

  /**
   * Sends a bunch of bytes.
   */
  bool send(const char* c, size_t len);

  /**
   * Receives a bunch of bytes into (*c) and (*clen). (*c) and (*clen)
   * may be resized as required to fit the data. if c == nullptr and
   * clen == nullptr, the return data is discarded.
   */
  bool receive(char** c, size_t* clen, size_t& receivelen, size_t timeout);

  /**
   * receives a direct buffer to the data. It is up to the caller to make sure
   * that no other sends/receives happen while the caller accesses the data.
   */
  bool receive_direct(char**c, size_t* len, size_t& receivelen, size_t timeout);

  /**
   * shutsdown the server
   */
  void shutdown();

 private:
  std::shared_ptr<raii_deleter> m_ipcfile_deleter;
  std::shared_ptr<boost::interprocess::shared_memory_object> m_shared_object;
  std::shared_ptr<boost::interprocess::mapped_region> m_mapped_region;

  std::string m_shmname;
  shared_memory_buffer* m_buffer = nullptr;
};


/**
 * \ingroup shmipc
 * The corresponding client object to the \ref server.
 *
 * The communication is mostly unsynchronized between server and client, so
 * users of the server/client implementations have to be careful about who
 * is sending and who is receiving.
 *
 * More design details in \ref server
 */
class client{
 public:
  client() = default;
  ~client() = default;
  /**
   * Connects to a server via the ipc file.
   */
  bool connect(std::string ipcfile, size_t timeout = 10);

  /**
   * Returns the maximum amount of data that can be sent or received.
   */
  size_t buffer_size() const;

  /**
   * Sends a bunch of bytes.
   */
  bool send(const char* c, size_t len);

  /**
   * Receives a bunch of bytes into (*c) and (*clen). (*c) and (*clen)
   * may be resized as required to fit the data. if c == nullptr and
   * clen == nullptr, the return data is discarded.
   */
  bool receive(char**c, size_t* len, size_t& receivelen, size_t timeout);

  /**
   * receives a direct buffer to the data. It is up to the caller to make sure
   * that no other sends/receives happen while the caller accesses the data.
   */
  bool receive_direct(char**c, size_t* len, size_t& receivelen, size_t timeout);
 private:
  std::shared_ptr<boost::interprocess::shared_memory_object> m_shared_object;
  std::shared_ptr<boost::interprocess::mapped_region> m_mapped_region;
  shared_memory_buffer* m_buffer = nullptr;
};

/**
 * \ingroup shmipc
 * Send an arbitrarily large amount of data
 * through an SHMIPC channel. T can be either a server or a client.
 * Receiver must use the matching large_receive function.
 */
template <typename T>
bool large_send(T& shm, const char *c, size_t len) {
//   logstream(LOG_WARNING) << "Sending : " << len << std::endl;
  /*
   * Essentially, we keep sending full buffers. The send is complete
   * when an less than full buffer is received.
   */
  size_t buffer_size = shm.buffer_size();
  size_t receivelen = 0;
  if (buffer_size == 0) return false;
  if (len < shm.buffer_size() - 1) {
    shm.send(c, len);
  } else {
    /*
     * send a full buffer then wait for a reply then send again. etc. We don't
     * need to wait for reply on the last buffer which is not full.
     */
    size_t sent = 0;
    int ret = shm.send(c, shm.buffer_size());
    if (ret == false) return false;
    sent = shm.buffer_size();
    while (sent < len) {
      bool ret = shm.receive_direct(nullptr, nullptr, receivelen, (size_t)(-1));
      if (ret == false) return false;
      size_t send_length = std::min(len - sent, buffer_size);
      ret = shm.send(c + sent, send_length);
      if (ret == false) return false;
      sent += send_length;
    }
    if (len % buffer_size == 0) {
      bool ret = shm.receive_direct(nullptr, nullptr, receivelen, (size_t)(-1));
      if (ret == false) return false;
      ret = shm.send(nullptr, 0); // send an empty terminator
      if (ret == false) return false;
    }
  }
  return true;
}


/**
 * \ingroup shmipc
 * Receives an arbitrarily large amount of data
 * through an SHMIPC channel. T can be either a server or a client.
 * Receiver must use the matching large_receive function.
 */
template <typename T>
bool large_receive(T& shm, char **c, size_t* clen,
                   size_t& receivelen, size_t timeout) {
  /*
   * Essentially, we keep receiving as long as we are getting full buffers.
   * timeout only applies to the first receive.
   */
  size_t buffer_size = shm.buffer_size();
  if (buffer_size == 0) log_and_throw("Invalid shared memory object");

  receivelen = 0;
  size_t last_receivelen = 0;
  size_t cur_timeout = timeout;
  while(1) {
    char* recv_buffer;
    size_t recvlen;
    bool ret = shm.receive_direct(&recv_buffer, &recvlen,
                                  last_receivelen, cur_timeout);
    if (ret == false) return false;
    // make sure we have room to receive an entire full buffer
    if (receivelen + last_receivelen > (*clen)) {
      // at least double c or fit.
      size_t realloc_size = std::max<size_t>((*clen) * 2, receivelen + last_receivelen);
      (*c) = (char*)realloc(*c, realloc_size);
      (*clen) = realloc_size;
    }
    memcpy((*c) + receivelen, recv_buffer, last_receivelen);
    cur_timeout = (size_t)(-1); // timeout only applies to the first receive

    // increment the receive count
    receivelen += last_receivelen;
    // non-full buffer. we are done
    if (last_receivelen < buffer_size) {
      // logstream(LOG_WARNING) << "Receiving: " << last_receivelen << std::endl;
      return true;
    } else {
      bool ret = shm.send(nullptr, 0); // send an empty message (continue with next)
      if (ret == false) return false;
    }
  }
}

} // shmipc
} // namespace turi
#endif

/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef SAFE_CIRCULAR_CHAR_BUFFER_HPP
#define SAFE_CIRCULAR_CHAR_BUFFER_HPP
#include <core/parallel/pthread_tools.hpp>
#include <core/logging/assertions.hpp>

namespace turi {

/**
\ingroup util
A non-resizing circular char buffer
with thread-safe write operations and a single reader
*/
class safe_circular_char_buffer {
 public:
  safe_circular_char_buffer(std::streamsize bufsize = 10485760 /*10 MB */);

  ~safe_circular_char_buffer();

  /**
   * Stops the buffer and signals any blocking calls.
   */
  void stop_reader();


  /**
   * Determine if the buffer is empty
   */
  bool empty() const;

  inline bool is_done() const {
    return done;
  }

  inline bool reader_is_blocked() const {
    return iswaiting;
  }
  /**
   * Get the total contents currently stored in the buffer.
   */
  std::streamsize size() const;

  /**
   * Get the amount of free space reamining in the buffer
   */
  std::streamsize free_space() const;

  /** Gets the size of the buffer.
     \note: The useable space is reserved_size() - 1 */
  inline std::streamsize reserved_size() const {
    return bufsize - 1;
  }


  /**
   * Returns 0 if the write doesn't fit
   *
   * This function acquires the critical section
   * to perform the write
   */
  std::streamsize write(const char* c, std::streamsize clen);

  /**
   * Returns 0 if the write doesn't fit
   *
   * This does the same as write(), but does not acquire the critical
   * section. The caller should ensure safety
   */
  std::streamsize write_unsafe(const char* c, std::streamsize clen);


  /**
   * Returns a pointer (through s) and a length of the read.  This
   * pointer is a direct pointer into the internal buffer of this
   * datastructure. The pointer is valid as long as no other
   * operations are performed on this structure.  The length of the
   * introspective_read may be less than the number of bytes
   * requested. Multiple calls to introspective_read may be necessary
   * to read all data in the buffer. If the function returns 0, the
   * buffer is empty.
   *
   * No locks are acquired on this call.
   */
  std::streamsize introspective_read(char* &s, std::streamsize clen);


  /**
   * Same as introspective read. But blocks until there is something to read
   * This function does not acquire a critical section.
   */
  std::streamsize blocking_introspective_read(char* &s,
                                              std::streamsize clen);


  void advance_head(const std::streamsize advance_len);


  /** When begin critical section returns, it is
      guaranteed that no other writer will be touching
      the tail of the queue */
  inline void begin_critical_section() {
    mut.lock();
  }

  /** Releases a critical section acquired by begin_critical_section */
  inline void end_critical_section() {
    mut.unlock();
  }

  /** Releases a critical section acquired by begin_critical_section,
   and signals the reader to begin reading if the reader is blocked */
  inline void end_critical_section_with_signal() {
    cond.signal();
    mut.unlock();
  }


 private:
  char* buffer;
  std::streamsize bufsize; // current size of the buffer

  /**
   * points to the head of the queue.  Reader reads from here
   */
  std::streamsize head;

  /**
   * points to one past the end of the queue.  writer writes to
   * here. if tail == head, buffer must be empty
   */
  std::streamsize tail;

  mutex mut;
  conditional cond;

  volatile bool done; // Once
  volatile bool iswaiting;
};

}

#endif

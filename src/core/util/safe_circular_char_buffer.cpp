/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/util/safe_circular_char_buffer.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/logging/assertions.hpp>

namespace turi {

  safe_circular_char_buffer::safe_circular_char_buffer(std::streamsize bufsize)
    :bufsize(bufsize), head(0), tail(0), done(false), iswaiting(false){
    ASSERT_GT(bufsize, 0);
    buffer = (char*)malloc((size_t)bufsize);
  }

  safe_circular_char_buffer::~safe_circular_char_buffer() {
    free(buffer);
  }

  void safe_circular_char_buffer::stop_reader() {
    mut.lock();
    done = true;
    cond.signal();
    mut.unlock();
  }

  // Head == tail implies empty
  bool safe_circular_char_buffer::empty() const {
    return (head == tail);
  }

  std::streamsize safe_circular_char_buffer::size() const {
    std::streamsize headval = head;
    std::streamsize tailval = tail;
    if (tailval >= headval) return tailval - headval;
    else if (headval > tailval) return tailval + bufsize - headval;
    return 0;
  }

  std::streamsize safe_circular_char_buffer::free_space() const {
    return bufsize - size() - 1;
  }

  std::streamsize safe_circular_char_buffer::
  write(const char* c, std::streamsize clen) {
    mut.lock();
    std::streamsize ret = write_unsafe(c, clen);
    if (iswaiting && ret > 0) {
      cond.signal();
    }
    mut.unlock();
    return ret;
  }

  std::streamsize safe_circular_char_buffer::
  write_unsafe(const char* c, std::streamsize clen) {
    // If the char array does not fit simply return
    if (clen + size() >= bufsize) return 0;

    /// Adding c characters to the buffer
    /// 0--------------H...body...T------------->Bufsize
    /// 0--------------H...body...T--(Part A)--->Bufsize
    /// T--(Part B)----H...body....ccccccccccccc>Bufsize
    /// 0cccccccccccT--H...body....ccccccccccccc>Bufsize

    // First we copy the contents into Part A
    std::streamsize firstcopy = std::min(clen, bufsize - tail);
    memcpy(buffer + tail, c, (size_t)firstcopy);
    // Move the tail to the end
    tail += firstcopy;
    // If tail moved to the end wrap around
    if (tail == bufsize) tail = 0;
    // If the copy is not complete
    if (firstcopy < clen) {
      // Assert: This only happens on wrape around
      ASSERT_EQ(tail, 0);
      // Determine what is left to be coppied
      std::streamsize secondcopy = clen - firstcopy;
      ASSERT_GT(secondcopy, 0);
      // Do the copy and advance the pointer
      memcpy(buffer, c + firstcopy, (size_t)secondcopy);
      tail += secondcopy;

    }
    return clen;
  }

  std::streamsize safe_circular_char_buffer::
  introspective_read(char* &s, std::streamsize clen) {
    ASSERT_GT(clen,0);
    // early termination check
    if(empty() || clen == 0) {
      s = NULL;
      return 0;
    }
    const std::streamsize curtail = tail;

    s = buffer + head;
    // how much we do read?  we can go up to the end of the requested
    // size or until a looparound
    // case 1: no looparound  |------H......T----->
    // case 2: looparound     |...T--H............>
    std::streamsize available_readlen = 0;
    const bool loop_around(curtail < head);
    if (loop_around) available_readlen = bufsize - head;
    else available_readlen = curtail - head;
    ASSERT_GE(available_readlen, 0);
    const std::streamsize actual_readlen =
      std::min(available_readlen, clen);
    ASSERT_GT(actual_readlen, 0);
    return actual_readlen;
  }


  std::streamsize safe_circular_char_buffer::
  blocking_introspective_read(char* &s, std::streamsize clen) {
    // try to read
    std::streamsize ret = introspective_read(s, clen);
    if (ret != 0) return ret;
    // if read failed. acquire the lock and try again
    while(1) {
      iswaiting = true;
      mut.lock();
      while (empty() && !done) cond.wait(mut);
      iswaiting = false;
      mut.unlock();
      std::streamsize ret = introspective_read(s, clen);
      if (ret != 0) return ret;
      if (done) return 0;
    }
  }


  void safe_circular_char_buffer::
  advance_head(const std::streamsize advance_len) {
    ASSERT_GE(advance_len, 0);
    ASSERT_LE(advance_len, size());
    // advance the head forward as far as possible
    head += advance_len;
    // If head wraps around move head to begginning and then offset
    if (head >= bufsize) head -= bufsize;
  } // end of advance head



} // end of namespace

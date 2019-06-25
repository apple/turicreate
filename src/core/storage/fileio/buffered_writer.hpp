/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include<vector>
#include<core/parallel/mutex.hpp>
#include<core/storage/sframe_data/sframe_constants.hpp>

namespace turi {
/**
 * \ingroup fileio
 * Provide buffered write abstraction.
 * The class manages buffered concurrent write to an output iterator.
 *
 * Example:
 *
 * Suppose there are M data sources randomly flow to N sinks. We can use
 * buffered_writer to achieve efficient concurrent write.
 *
 * \code
 *
 * std::vector<input_iterator> sources; // size M
 * std::vector<output_iterator>  sinks; // size N
 * std::vector<turi::mutex>  sink_mutex; // size N
 *
 * parallel_for_each(s : sources) {
 *   std::vector<buffered_writer> writers;
 *   for (i = 1...N) {
 *    writers.push_back(buffered_writer(sinks[i], sink_mutex[i]));
 *   }
 *   while (s.has_next()) {
 *     size_t destination = random.randint(N);
 *     writers[destination].write(s.next());
 *   }
 *   for (i = 1...N) {
 *     writers[i].flush();
 *   }
 * }
 * \endcode
 *
 * Two parameters "soft_limit" and "hard_limit" are used to control the buffer
 * size. When soft_limit is met, the writer will try to flush the buffer
 * content to the sink. When hard_limit is met, the writer will force the flush.
 */
template<typename ValueType, typename OutIterator>
class buffered_writer {
public:
  buffered_writer(OutIterator& out, turi::mutex& out_lock,
                  size_t soft_limit = SFRAME_WRITER_BUFFER_SOFT_LIMIT,
                  size_t hard_limit = SFRAME_WRITER_BUFFER_HARD_LIMIT) :
    out(out), out_lock(out_lock),
    soft_limit(soft_limit),
    hard_limit(hard_limit) {
    ASSERT_GT(hard_limit, soft_limit);
  }

  /**
   * Write the value to the buffer.
   * Try flush when buffer exceeds soft limit and force
   * flush when buffer exceeds hard limit.
   */
  void write(const ValueType& val) {
    buffer.push_back(val);
    if (buffer.size() >= soft_limit) {
      bool locked = out_lock.try_lock();
      if (locked || buffer.size() >= hard_limit) {
        flush(locked);
      }
    }
  }

  void write(ValueType&& val) {
    buffer.push_back(val);
    if (buffer.size() >= soft_limit) {
      bool locked = out_lock.try_lock();
      if (locked || buffer.size() >= hard_limit) {
        flush(locked);
      }
    }
  }


  /**
   * Flush the buffer to the output sink. Clear the buffer when finished.
   */
  void flush(bool is_locked = false) {
    if (!is_locked) {
      out_lock.lock();
    }
    std::lock_guard<turi::mutex> guard(out_lock, std::adopt_lock);
    for (auto& val : buffer) {
      *out++ = std::move(val);
    }
    buffer.clear();
  }

private:
  OutIterator& out;
  turi::mutex& out_lock;
  size_t soft_limit;
  size_t hard_limit;
  std::vector<ValueType> buffer;
};
}

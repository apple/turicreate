/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef _CANVAS_STREAMING_TRANSFORMATION
#define _CANVAS_STREAMING_TRANSFORMATION

#include <flexible_type/flexible_type.hpp>
#include <parallel/lambda_omp.hpp>

namespace turi {
namespace visualization {

class transformation_output {
  public:
    virtual ~transformation_output() = default;
    virtual std::string vega_column_data(bool sframe = false) const = 0;
};

class sframe_transformation_output : public transformation_output {
  public:
    virtual std::string vega_summary_data() const = 0;
};

class transformation_base {
  public:
    virtual ~transformation_base() = default;
    virtual std::shared_ptr<transformation_output> get() = 0;
    virtual bool eof() const = 0;
    double get_percent_complete() const;
    virtual size_t get_batch_size() const = 0;
    virtual flex_int get_total_rows() const = 0;
    virtual flex_int get_rows_processed() const = 0;
};

class transformation_collection : public std::vector<std::shared_ptr<transformation_base>> {
  public:
    // combines all of the transformations in the collection
    // into a single transformer interface to simplify consumption
};

template<typename InputIterable,
         typename Output,
         size_t BATCH_SIZE>
class transformation : public transformation_base {
  protected:
    InputIterable m_source;
    std::shared_ptr<Output> m_transformer;
    size_t m_currentIdx = 0;
    bool m_initialized = false;

  private:
    void check_init(const char * msg, bool initialized) const {
      if (initialized != m_initialized) {
        log_and_throw(msg);
      }
    }
    void require_init() const {
      check_init("Transformer must be initialized before performing this operation.", true);
    }

  protected:
    /* Subclasses may override: */
    /* Get the current result (without iterating over any new values) */
    virtual Output get_current() {
      return *m_transformer;
    }
    /* Create multiple transformers from input */
    virtual std::vector<Output> split_input(size_t num_threads) {
      return std::vector<Output>(num_threads);
    }
    /* Merge multiple transformers into output */
    virtual void merge_results(std::vector<Output>& transformers) = 0;

  public:
    virtual void init(const InputIterable& source) {
      check_init("Transformer is already initialized.", false);
      m_source = source;
      m_transformer = std::make_shared<Output>();
      m_currentIdx = 0;
      m_initialized = true;
    }
    virtual bool eof() const override {
      require_init();
      DASSERT_LE(m_currentIdx, m_source.size());
      return m_currentIdx == m_source.size();
    }
    virtual flex_int get_rows_processed() const override {
      require_init();
      DASSERT_LE(m_currentIdx, m_source.size());
      return m_currentIdx;
    }
    virtual flex_int get_total_rows() const override {
      require_init();
      return m_source.size();
    }
    virtual std::shared_ptr<transformation_output> get() override {
      require_init();
      if (this->eof()) {
        // bail out, done streaming
        return m_transformer;
      }

      const size_t num_threads_reported = thread_pool::get_instance().size();
      const size_t start = m_currentIdx;
      const size_t input_size = std::min(BATCH_SIZE, m_source.size() - m_currentIdx);
      const size_t end = start + input_size;
      auto transformers = this->split_input(num_threads_reported);
      const auto& source = this->m_source;
      in_parallel(
        [&transformers, &source, input_size, start]
        (size_t thread_idx, size_t num_threads) {

        DASSERT_LE(transformers.size(), num_threads);
        if (thread_idx >= transformers.size()) {
          // this operation isn't parallel enough to use all threads.
          // bail out on this thread.
          return;
        }

        auto& transformer = transformers[thread_idx];
        size_t thread_input_size = input_size / transformers.size();
        size_t thread_start = start + (thread_idx * thread_input_size);
        size_t thread_end = thread_idx == transformers.size() - 1 ?
          start + input_size :
          thread_start + thread_input_size;
        DASSERT_LE(thread_end, start + input_size);
        for (const auto& value : source.range_iterator(thread_start, thread_end)) {
          transformer.add_element_simple(value);
        }
      });

      this->merge_results(transformers);
      m_currentIdx = end;

      return m_transformer;
    }

    virtual size_t get_batch_size() const override {
      return BATCH_SIZE;
    }
};

}}

#endif

/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef SFRAME_QUERY_ENGINE_broadcast_queue_HPP
#define SFRAME_QUERY_ENGINE_broadcast_queue_HPP
#include <deque>
#include <string>
#include <memory>
#include <core/storage/fileio/general_fstream.hpp>
#include <core/storage/fileio/fixed_size_cache_manager.hpp>
#include <core/logging/assertions.hpp>

namespace turi {
class oarchive;
class iarchive;

template <typename T>
struct broadcast_queue_serializer {
  void save(oarchive& oarc, const T& t) {
    oarc << t;
  }
  void load(iarchive& iarc, T& t) {
    iarc >> t;
  }
};
/**
 * \ingroup sframe_query_engine
 * \addtogroup Utilities Utilities
 * \{
 */
/**
 * This implements a external memory single producer, multiple-consumer queue
 * where every consumer sees all the produced elements.
 *
 * This class is *not* thread safe.
 *
 * \tparam T Datatype to be saved. Must be serializable
 *
 * This class guarantees very high efficiency if the total number
 * of elements do not exceed the cache limit.
 *
 * The key design constraint are that files are either open for reading
 * or writing, but not both simultaneously. Random writes are prohibited, but
 * random reads are allowed.
 *
 * Single consumer queue
 * ---------------------
 * The single consumer case is easy to understand so we will explain that first.
 *
 * 1. push file
 * 2. center queue
 * 3. pop file
 *
 * Elements enter at the top and leave from the bottom.
 *
 * When data is pushed, we push directly into the center queue. But if the
 * center queue is full, we start writing to the push_file.
 *
 * When data is popped, we first read from the pop-file. When there are no more
 * pop files, we reads from the center queue. When the center queue is empty,
 * it closes the push and swaps it to a pop file and starts reading from the pop
 * file.
 *
 * Thus there can be at most one pop file.
 *
 * There are two interesting properties.
 * Where n is the total number of elements, and k is the limit on the size
 * of center_queue.
 *
 * 1) no more than n / kfiles will be created even under
 * adversial schedulings of consumer and producer.
 *
 * (Simple proof. Everytime a push file becomes a pop file, the center is
 * empty. Hence at least k elements must be inserted
 * into the center queue before a new push file is started.)
 *
 * 2) Assuming producer and consumer rates match. i.e. we alternate
 * producing 'q' elements and consuming 'q' elements. At most a proportion of
 * (q - k)/q elements will be written to or read from disk.
 *
 * Multiple consumer queue 1
 * -------------------------
 * In the multiple consumer case, every consumer must see every element. This
 * makes the datastructure substantially trickier. Specifically you want to make
 * sure the setting where there are 2 or more consumers and all consumers are
 * very slow except for one which is as fast as the producer, do not result in:
 *  - producer writes a single element write to a push file
 *  - fast consumer needs the element, closes that push file turns it into a
 *  pop file and reads it.
 *  - This results in one file per element.
 *
 * In other words, we must try to preserve the property (1) above, that at most
 * O(n / k) files are created for some datastructure cache parameter k.
 *
 * A simple method is to shift to a pure caching model.
 * 1. memory queue
 * 2. pop file
 * 3. pop file
 * 4. pop file
 * 5. ...
 *
 * When we push:
 *  - It goes into the memory queue
 *  - When memory queue exceeds some threshold, it gets written to disk
 *    and a new memory queue is started.
 *
 * Every consumer remembers a current file position pointer, and
 *  - pops from file
 *  - or reads from the in memory queue. (a little bit of bookkeeping
 *    is needed since the queue may get flushed to disk)
 *
 * While this satisfies the property (1), this does not satisfy property (2).
 * i.e. Assuming producer and consumer rates match. i.e. we alternate
 * producing 'q' elements and every consumer consuming 'q' elements.
 * When q > k, This procedure will write every element to file.
 *
 *
 * Multiple consumer queue 2
 * -------------------------
 * Lets soften the cache a bit by allowing caching of up to 2k elements.
 * When 2k elements are reached, we flush the first k elements to a file.
 *
 * It is easy to show that at most O(n / k) files are created since each file
 * must have at least k elements. Similarly, when producer and consumer rates
 * match. At most a proportion of (q - k)/q elements will be written to or read
 * from disk.
 *
 * This thus satisfies the desirable properties of the single consumer queue.
 *
 * The architecture is hence:
 * 1. memory queue
 * 1. push file
 * 2. pop file
 * 3. pop file
 * 4. pop file
 * 5. ...
 *
 * On push:
 *  - push into memory_queue
 *  - if memory_queue size exceeds 2k,
 *       - flush first k into a new push_file
 *       - If there is a consumer reading from the memory queue, flip push file
 *       to a pop file. (make the
 *         push file into a pop file and update all consumers so that
 *         they are reading from the pop file.)
 *  - else if push file exists,
 *       - pop first element of memory_queue to push_file
 *
 * On pop:
 *  - If I am reading from pop file just read next element from pop file
 *    advancing to next pop file if necessary
 *  - If there are no more pop files, but if there is a push file, flip push
 *    file to a pop file.
 *  - If I am reading from memory queue, advance to the
 *    next memory queue element
 *
 * Optimizations
 * -------------
 * More optimizations are needed to minimize the number of files created.
 * 1. Use a pool of file names which we can reuse rather than create a new one
 * everytime we need a new file.
 *
 * 2. Once we reach 2k, we dump the first k to a file.  but we don't close the
 *  file. After which every new insertion will shift the queue. one element
 *  gets written to disk, as one element gets inserted. Only when the file must
 *  be read by a consumer, then it gets flushed.
 *
 * 3. Delay the creation of the "pop file" from the "push file" as late as we
 * can. i.e. we only create the pop file when there is a consumer who needs to
 * read from the data just written to the push file
 */
template <typename T, typename Serializer = broadcast_queue_serializer<T>>
class broadcast_queue {
 public:
  /**
   * Constructs a disk backed queue
   * \param cache limit Number of elements to cache
   */
  explicit broadcast_queue(size_t num_consumers,
                           size_t cache_limit = 128,
                           const Serializer& serializer = Serializer()):
      m_cache_limit(cache_limit), m_serializer(serializer), m_consumers(num_consumers) {
    if (m_cache_limit == 0) m_cache_limit = 1;
  }
  void reset() {
    m_consumers.clear();
    // clear the pop queue, deleting files if necessary
    while(!m_pop_queues.empty()) release_pop_queue_front();

    // clear the push queue, deleting files if necessary
    m_push_queue.write_handle.reset();
    if (!m_push_queue.file_name.empty()) {
      fileio::delete_path(m_push_queue.file_name);
    }
    m_push_queue.file_name.clear();


    delete_all_cache_files();
  }

  ~broadcast_queue() {
    reset();
  }
  /**
   * Sets the cache limit
   * \param cache limit Number of elements to cache
   */
  void set_cache_limit(size_t cache_limit) {
    m_cache_limit = cache_limit;
    if (m_cache_limit == 0) m_cache_limit = 1;
  }

  /**
   * Pushes an element into the queue.
   */
  void push(const T& el) {
    m_push_queue.element_cache.push_back(el);
    ++m_push_queue.nelements;
    ++nelements_pushed;

    if (m_push_queue.write_handle) {
      for (auto& c: m_consumers) {
        if (c.reading_from_push_queue() && c.element_offset == 0) {
          flip_queues();
          break;
        }
      }
    }
    if (!m_push_queue.write_handle) {
      // no output file yet
      // push until 2 * cache limit
      // try to trim first. Then flush.
      if (m_push_queue.nelements >= 2 * m_cache_limit) {
        trim_push_queue();
      }
      if (m_push_queue.nelements >= 2 * m_cache_limit) {
        flush_push_queue();
      }
    } else {
      // we have an output file. this is now a rolling cache
      // pop one element and write
      oarchive oarc(*m_push_queue.write_handle);
      m_serializer.save(oarc, m_push_queue.element_cache.front());
      m_push_queue.element_cache.pop_front();
      for (auto& c: m_consumers) {
        if (c.reading_from_push_queue()) --c.element_offset;
      }
    }
  }
  /**
   * if this returns false, the next call to pop(consumer) will succeed,
   * Otherwise it will fail.
   */
  bool empty(size_t consumer) {
    return m_consumers[consumer].nelements_popped == nelements_pushed;
  }
  /**
   * Pops and element from the queue.
   * Returns true on success, false on failure (queue is empty)
   */
  bool pop(size_t consumer, T& ret) {
    DASSERT_LT(consumer, m_consumers.size());
    // current consumer
    auto& cc = m_consumers[consumer];

    if (!cc.reading_from_push_queue()) {
      if(cc.file_offset >= cc.current_pop_queue->file_length) {
        if (cc.current_pop_queue->next_queue == nullptr &&
            m_push_queue.write_handle) {
          flip_queues();
        }
        // advance to next queue if this queue is finished
        cc.current_pop_queue = cc.current_pop_queue->next_queue;
        cc.file_offset = 0;
        if (m_pop_queues.front().unique()) release_pop_queue_front();
        return pop(consumer, ret);
      }
      auto& pq = cc.current_pop_queue;
      pq->read_handle->seekg(cc.file_offset, std::ios_base::beg);
      iarchive iarc(*(pq->read_handle));
      m_serializer.load(iarc, ret);
      cc.file_offset = pq->read_handle->tellg();
    } else {
      if (m_push_queue.element_cache.size() <= cc.element_offset) return false;
      ret = m_push_queue.element_cache[cc.element_offset];
      ++cc.element_offset;
      trim_push_queue();
    }
    ++cc.nelements_popped;
    return true;
  }

  /**
   * Returns the number of elements in the queue.
   */
  size_t num_elements() const {
    return m_element_count;
  }

  /**
   * Deletes all unused cache files
   */
  void delete_all_cache_files() {
    while(!m_allocated_filenames.empty()) {
      fileio::delete_path(m_allocated_filenames.front());
      m_allocated_filenames.pop();
    }
  }

 private:

  size_t m_cache_limit = 0;
  size_t m_element_count = 0;
  Serializer m_serializer;

  /**
   * A queue of this is managed by m_pop_queues.
   * Each pop_queue points to a file containing the data for this queue
   */
  struct pop_queue {
    /// file name of this queue
    std::string file_name;
    /// length of file
    size_t file_length = 0;
    /// a ifstream handle
    std::shared_ptr<general_ifstream> read_handle;
    /// Number of elements in this queue
    size_t nelements = 0;
    /// The next queue in this list of queue
    std::shared_ptr<pop_queue> next_queue;
  };

  /**
   * There is only one of this; this is where data gets pushed.
   * There are 2 possible states for this structure.
   *
   * When there are < 2 * cache_limit elements:
   *   All data is in element_cache
   *   write_handle is nullptr
   * When there are >= 2 * cache_limit elements:
   *   The first nelements - cache_limit elements are
   *      stored in the file using write_handle
   *   element_cache contains the most recently inserted cache_limit elements
   */
  struct push_queue {
    std::string file_name;
    std::shared_ptr<general_ofstream> write_handle;
    std::deque<T> element_cache;
    /// The total number of elements stored
    size_t nelements = 0;
  };

  /**
   * There is one of this for each consumer.
   *
   * if current_pop_queue is nullptr the consumer is reading from the push_queue
   * in which case element_offset is the index into push_queue::element_cache
   * for the NEXT element to read.
   *
   * if curent_pop_queue is not null, we are reading from a pop_queue and
   * file_offset is the file position to seek to for the NEXT element to read.
   *
   * Global Invariant
   * ----------------
   * When current_pop_queue is nullptr, push_queue.nelements cannot be
   * greater than 2 * cache_limit_elements. (we cannot maintain the
   * element_offset index correctly when the rolling cache is used).
   */
  struct consumer {
    std::shared_ptr<pop_queue> current_pop_queue;
    size_t element_offset = 0;
    size_t file_offset = 0;
    /// total elements popped so far
    size_t nelements_popped = 0;

    bool reading_from_push_queue() const {
      return current_pop_queue == nullptr;
    }
  };

  std::deque<std::shared_ptr<pop_queue> > m_pop_queues;
  push_queue m_push_queue;
  std::vector<consumer> m_consumers;
  size_t nelements_pushed = 0;
  /// we keep a pool of cache files to recycle
  std::queue<std::string> m_allocated_filenames;

  std::string get_cache_file() {
    if (!m_allocated_filenames.empty()) {
      auto ret = m_allocated_filenames.front();
      m_allocated_filenames.pop();
      return ret;
    } else {
      return fileio::fixed_size_cache_manager::get_instance().get_temp_cache_id("dqueue");
    }
  }

  void release_cache_file(const std::string& f) {
    m_allocated_filenames.push(f);
  }

  /**
   * If all readers are in the push queue, and
   * reading directly from elements, we might be able to trim the queue.
   */
  void trim_push_queue() {
    size_t min_element_offset = (size_t)(-1);
    for (auto& c: m_consumers) {
      // we have a reader that is not reading from push queue. cannot trim
      if (c.reading_from_push_queue() == false) {
        return;
      } else if (c.element_offset < min_element_offset) {
        min_element_offset = c.element_offset;
      }
    }
    if (min_element_offset > 0) {
      for (auto& c: m_consumers) {
        c.element_offset -= min_element_offset;
      }
      auto start = m_push_queue.element_cache.begin();
      auto end = start + min_element_offset;
      m_push_queue.nelements -= min_element_offset;

      // remove those m_cache_limit elements from the in memory cache
      m_push_queue.element_cache.erase(m_push_queue.element_cache.begin(),
                                       end);
    }
  }

  bool has_push_queue_reader() {
    return std::any_of(m_consumers.begin(),
                    m_consumers.end(),
                    [](const consumer& c) {
                      return c.reading_from_push_queue();
                    });
  }

  /**
   * To be called when the size of the in memory cache exceeds 2 *
   * the cache limit.
   */
  void flush_push_queue() {
    // write out m_cache_limit elements
    auto start = m_push_queue.element_cache.begin();
    auto end = m_push_queue.element_cache.begin() + m_cache_limit;

    // if there is a consumer reading from the push_queue, we need to
    // completely close the file and shift it to the pull queue.
    // this requires a whole bunch of datastructure updates to be
    // performed by flip_queues
    if (has_push_queue_reader()) {
      m_push_queue.file_name = get_cache_file();
      m_push_queue.write_handle =
          std::make_shared<general_ofstream>(m_push_queue.file_name);
      // remember the offset of each element
      std::vector<size_t> file_offsets;
      file_offsets.reserve(m_cache_limit);
      oarchive oarc;
      size_t filepos_ctr = 0;
      for(; start != end; ++start) {
        file_offsets.push_back(filepos_ctr);
        m_serializer.save(oarc, *start);
        m_push_queue.write_handle->write(oarc.buf, oarc.off);
        filepos_ctr += oarc.off;
        oarc.off = 0;
      }
      free(oarc.buf);

      bool must_flip_queue = false;
      // now we need to update all the consumers
      // which are reading directly from elements.
      for (auto& c: m_consumers) {
        if (c.reading_from_push_queue() && c.element_offset < m_cache_limit) {
          must_flip_queue = true;
          break;
        }
      }
      if (must_flip_queue) flip_queues();

      for (auto& c: m_consumers) {
        // reading from elements
        if (c.reading_from_push_queue()) {
          if (c.element_offset >= m_cache_limit) {
            c.element_offset -= m_cache_limit;
          } else {
            DASSERT_EQ(must_flip_queue, true);
            // convert element offset to file_offset
            c.current_pop_queue = m_pop_queues.back();
            c.file_offset = file_offsets[c.element_offset];
            c.element_offset = 0;
          }
        }
      }
    } else {
      // no push queue reader. open a push file and just dump
      m_push_queue.file_name = get_cache_file();
      m_push_queue.write_handle =
          std::make_shared<general_ofstream>(m_push_queue.file_name);
      oarchive oarc(*m_push_queue.write_handle);
      for(; start != end; ++start) {
        m_serializer.save(oarc, *start);
      }
    }

    // remove those m_cache_limit elements from the in memory cache
    m_push_queue.element_cache.erase(m_push_queue.element_cache.begin(),
                                     end);

  }

  /**
   * Converts the file in the push_queue to a pull queue
   * and update all the consumers
   * \param file_offsets file_offsets[i] is the offset of element i in the file
   * currently managed by push_queue
   */
  void flip_queues() {
    // here we are just doing alot of invariant fixing to convert the push
    // queue file to a pull queue
    //
    // close the push queue handle
    m_push_queue.write_handle.reset();
    // make a new pop queue
    auto pq = std::make_shared<pop_queue>();
    pq->file_name = std::move(m_push_queue.file_name);
    m_push_queue.file_name.clear();
    pq->read_handle = std::make_shared<general_ifstream>(pq->file_name);
    pq->file_length = pq->read_handle->file_size();
    pq->nelements = m_push_queue.nelements - m_push_queue.element_cache.size();
    // insert into the queue
    // update the linked list managed by pop_queue::next_queue
    if (!m_pop_queues.empty()) {
      auto last_elem = m_pop_queues.back();
      last_elem->next_queue = pq;
    }
    m_pop_queues.push_back(pq);
    // update the nelement counter in the push_queue
    m_push_queue.nelements = m_push_queue.element_cache.size();
  }

  void release_pop_queue_front() {
    auto i = m_pop_queues.front();
    i->read_handle.reset();
    if (!i->file_name.empty()) {
      release_cache_file(i->file_name);
    }
    i->file_name.clear();
    m_pop_queues.pop_front();
  }
};

/// \}
} // turicreate
#endif

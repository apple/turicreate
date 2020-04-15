/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FILEIO_CACHING_DEVICE_HPP
#define TURI_FILEIO_CACHING_DEVICE_HPP
#include <core/logging/logger.hpp>
#include <core/parallel/mutex.hpp>
#include <core/storage/fileio/block_cache.hpp>
#include <core/storage/fileio/sanitize_url.hpp>
#include <core/util/basic_types.hpp>
#include <map>
#include <mutex>
#include <thread>
namespace turi {

// private namespace
namespace {
const size_t READ_CACHING_BLOCK_SIZE = 64 * 1024 * 1024;  // 64 MB
}  // namespace

/*
 * For each parallel (streamlined) network IO, we can view them as intervals
 * in parallel universe.
 *
 * What we do is to merge all universe into one: the very start and the
 * very end of the entire IO, which is the elapsed time, using the same time
 * axis.
 * t1         |-----|
 * t2 |----|
 * t3    |-----|
 *    s ------------ e
 *
 * Since SFrame can be streamed, meaning it does not download all of the files
 * at once. It may have multiple times of fetching data. So it consists many
 * small parts of IO activities.
 *
 * |----|<- less than 150 ms -> |----|
 * s ------------------------------- e
 *
 * if 2 IO acticity are only separated by less than 150 ms time intervals,
 * they should be considered as the same part of an IO process, instead of 2.
 *
 * |-----| <- 10 mins -> |-------|
 * s --- e               s ----- e
 *
 * Since 2 adjacent IO activities are too distant, we should view it as 2
 * separate activities. For example, user may play with first 1000 rows, and
 * later the user jumps to the tail of the frame to mess around with the data.
 *
 */
template <class T>
class StopWatch {
 public:
  using steady_clock = std::chrono::steady_clock;
  using my_time_t = decltype(steady_clock::now());
  StopWatch(size_t interval)
      : interval_(interval), beg_(steady_clock::now()), end_(beg_) {}

  StopWatch(const StopWatch&) = delete;
  StopWatch(StopWatch&& rhs) = delete;

  void start() {
    std::lock_guard<std::mutex> lk(mx_);

    if (thread_set_.size() == 0) {
      using milliseconds = std::chrono::milliseconds;
      auto lag = std::chrono::duration_cast<milliseconds>(
          std::chrono::steady_clock::now() - beg_);
      if (lag > milliseconds{150}) {
        beg_ = steady_clock::now();
        mile_stone_ = beg_;
      }
    }

    if (!thread_set_.insert(std::this_thread::get_id()).second) {
      logstream(LOG_DEBUG) << "this thread " << std::this_thread::get_id()
                           << "already starts the clock" << std::endl;
    }
  }

  bool is_time_to_record() {
    std::lock_guard<std::mutex> lock(mx_);

    if (thread_set_.size() == 0) return false;

    // reach to timepoint, log it.
    auto cur = steady_clock::now();
    if (cur > mile_stone_) {
      mile_stone_ = cur + T(interval_);
      return true;
    }

    // no need to record
    return false;
  }

  /*
   * @return: int. number of threads still holding
   */
  int stop() {
    std::lock_guard<std::mutex> lk(mx_);
    if (thread_set_.count(std::this_thread::get_id())) {
      thread_set_.erase(std::this_thread::get_id());
      // last man standing
      if (thread_set_.size() == 0) {
        end_ = steady_clock::now();
        return 0;
      }
      // still running
      return thread_set_.size();
    }

    return thread_set_.size();
  }

  template <class Output>
  Output duration() const {
    std::lock_guard<std::mutex> lk(mx_);
    // the clock is still on
    if (thread_set_.count(std::this_thread::get_id())) {
      return std::chrono::duration_cast<Output>(steady_clock::now() - beg_);
    }

    return std::chrono::duration_cast<Output>(end_ - beg_);
  }

  // singleton destroyed since it's non-copy and movable,
  // any dangling pointer or reference to this stop_watah is illegal
  ~StopWatch() { stop(); }

 private:
  uint64_t interval_;
  my_time_t beg_;
  my_time_t end_;
  my_time_t mile_stone_;
  mutable std::mutex mx_;

  std::set<std::thread::id> threads_;
};

using StopWatchSec_t = StopWatch<std::chrono::seconds>;

/**
 * \ingroup fileio
 * Can be wrapped around any device implement to provide read caching. This
 * should be used only when the filesystem we are accessing is rather remote.
 * It uses the \ref block_cache to cache large blocks on the cache:// file
 * system.
 *
 * Before:
 * \code
 *   typedef boost::iostreams::stream<s3_device> s3_fstream;
 * \endcode
 *
 * After:
 * \code
 *   typedef boost::iostreams::stream<read_caching_device<s3_device> >
 * s3_fstream; \endcode
 *
 * It uses the \ref block_cache to pro
 */
template <typename T>
class read_caching_device {
 public:  // boost iostream concepts
  typedef typename T::char_type char_type;
  typedef typename T::category category;

  read_caching_device() {}

  read_caching_device(const std::string& filename, const bool write = false) {
    logstream(LOG_DEBUG) << "read_cachine_device: " << filename << std::endl;
    m_filename = filename;
    if (write == false) {
      // check the filesize cache for the filesize so we don't poke s3 again
      // even if all the data we care about are in a cache
      std::lock_guard<mutex> file_size_guard(m_filesize_cache_mutex);
      auto iter = m_filename_to_filesize_map.find(filename);
      if (iter != m_filename_to_filesize_map.end()) {
        m_file_size = iter->second;
      } else {
        m_contents = std::make_shared<T>(filename, write);
        m_file_size = m_contents->file_size();
        m_filename_to_filesize_map[filename] = m_file_size;
        // report downloading every 30s
        if (m_filename_to_filesize_map.find(filename) !=
            m_filename_to_filesize_map.end()) {
          m_filename_to_stop_watch[filename] =
              std::unique_ptr<StopWatchSec_t>(new StopWatchSec_t(30));
        }
      }
    } else {
      m_contents = std::make_shared<T>(filename, write);
    }

    m_writing = write;
  }

  // Because the device has bidirectional tag, close will be called
  // twice, one with the std::ios_base::in, followed by out.
  // Only close the file when the close tag matches the actual file type.
  void close(std::ios_base::openmode mode = std::ios_base::openmode()) {
    if (mode == std::ios_base::out && m_writing) {
      if (m_contents) m_contents->close(mode);
      m_contents.reset();
      // evict all blocks for this key
      auto& bc = block_cache::get_instance();
      size_t block_number = 0;
      while (1) {
        std::string key = get_key_name(block_number);
        if (bc.evict_key(key) == false) break;
        ++block_number;
      }
      // evict the file size cache
      {
        std::lock_guard<mutex> file_size_guard(m_filesize_cache_mutex);
        m_filename_to_filesize_map.erase(m_filename);

        auto iter = m_filename_to_stop_watch.find(m_filename);
        if (iter != m_filename_to_stop_watch.end()) {
          if (!iter->stop()) {
            m_filename_to_stop_watch.erase(iter);
          }
        }
      }

    } else if (mode == std::ios_base::in && !m_writing) {
      if (m_contents) m_contents->close(mode);
      m_contents.reset();
      {
        std::lock_guard<mutex> file_size_guard(m_filesize_cache_mutex);
        if (m_filename_to_stop_watch.find(m_filename) !=
            m_filename_to_stop_watch.end())
          m_filename_to_stop_watch[m_filename]->stop();
      }
    }
  }

  /** the optimal buffer size is 0. */
  inline std::streamsize optimal_buffer_size() const { return 0; }

  std::streamsize read(char* strm_ptr, std::streamsize n) {
    // there is an upper limit of how many bytes we can read
    // based on the file size
    n = std::min<std::streamsize>(n, m_file_size - m_file_pos);

    // suspicious
    if (n < 0) {
      logstream(LOG_DEBUG) << "read size is " << n << "; file size is "
                           << m_file_size << std::endl;
    }

    std::streamsize ret = 0;

    while (n > 0) {
      // the block number containing the offset.
      auto block_number = m_file_pos / READ_CACHING_BLOCK_SIZE;
      // the offset inside the block
      auto block_offset = m_file_pos % READ_CACHING_BLOCK_SIZE;
      // number of bytes I can read inside this block before I hit the next
      // block
      size_t n_bytes =
          (block_number + 1) * READ_CACHING_BLOCK_SIZE - m_file_pos;
      n_bytes = std::min<size_t>(n_bytes, n);
      bool success =
          fetch_block(strm_ptr + ret, block_number, block_offset, n_bytes);
      if (success == false) {
        log_and_throw(std::string("Unable to read ") + m_filename);
      }
      n -= n_bytes;
      ret += n_bytes;
      // advance the file position
      m_file_pos += n_bytes;
    }
    return ret;
  }

  std::streamsize write(const char* strm_ptr, std::streamsize n) {
    {
      std::lock_guard<mutex> file_size_guard(m_filesize_cache_mutex);
      m_filename_to_stop_watch[m_filename]->start();
    }

    auto bytes_written = get_contents()->write(strm_ptr, n);

    {
      std::lock_guard<mutex> file_size_guard(m_filesize_cache_mutex);
      auto& stop_watch = m_filename_to_stop_watch[m_filename];
      if (stop_watch->is_time_to_record()) {
        logprogress_stream
            << "Finish uploading " << sanitize_url(m_filename)
            << ". Elapsed time "
            << stop_watch->template duration<std::chrono::seconds>().count()
            << " seconds" << std::endl;
      }
    }

    return bytes_written;
  }

  bool good() const { return get_contents()->good(); }

  /**
   * Seeks to a different location.
   */
  std::streampos seek(std::streamoff off, std::ios_base::seekdir way,
                      std::ios_base::openmode openmode) {
    if (openmode == std::ios_base::in) {
      if (way == std::ios_base::beg) {
        m_file_pos = std::min<std::streamoff>(off, m_file_size);
      } else if (way == std::ios_base::cur) {
        m_file_pos = std::min<std::streamoff>(m_file_pos + off, m_file_size);
        m_file_pos = std::max<std::streamoff>(m_file_pos, 0);
      } else if (way == std::ios_base::end) {
        m_file_pos =
            std::min<std::streamoff>(m_file_size + off - 1, m_file_size);
        m_file_pos = std::max<std::streamoff>(m_file_pos, 0);
      }
      return m_file_pos;
    } else {
      return get_contents()->seek(off, way, openmode);
    }
  }

  /**
   * Returns the file size of the opened file.
   * Returns (size_t)(-1) if there is no file opened, or if there is an
   * error obtaining the file size.
   */
  size_t file_size() const { return m_file_size; }

  /// Not supported
  std::shared_ptr<std::istream> get_underlying_stream() { return nullptr; }

 private:
  std::string m_filename;
  std::shared_ptr<T> m_contents;
  size_t m_file_size = 0;
  std::streamoff m_file_pos = 0;
  bool m_writing = true;

  static mutex m_filesize_cache_mutex;
  static std::map<std::string, size_t> m_filename_to_filesize_map;
  static std::map<std::string, std::unique_ptr<StopWatchSec_t>>
      m_filename_to_stop_watch;

  std::shared_ptr<T>& get_contents() {
    if (!m_contents) {
      m_contents = std::make_shared<T>(m_filename, m_writing);
    }
    return m_contents;
  }
  std::string get_key_name(size_t block_number) {
    // we generate a key name that will never appear in any filename
    return m_filename + "////:" + std::to_string(block_number);
  }
  /**
   * Fetches the contents of a block.
   * Returns true on success and false on failure.
   */
  bool fetch_block(char* output, size_t block_number, size_t startpos,
                   size_t length) {
    auto& bc = block_cache::get_instance();
    std::string key = get_key_name(block_number);
    int64_t ret = bc.read(key, output, startpos, startpos + length);
    if (static_cast<size_t>(ret) == length) return true;

    logstream(LOG_INFO) << "Fetching " << sanitize_url(m_filename) << " Block "
                        << block_number << std::endl;
    // ok. failure... no such block or block is bad. We read it ourselves.
    // read the whole block
    auto block_start = block_number * READ_CACHING_BLOCK_SIZE;
    auto block_end =
        std::min(block_start + READ_CACHING_BLOCK_SIZE, m_file_size);
    // seek to the block and read the whole block at once
    auto& contents = get_contents();
    contents->seek(block_start, std::ios_base::beg, std::ios_base::in);
    std::string block_contents(block_end - block_start, 0);

    {
      std::lock_guard<mutex> file_size_guard(m_filesize_cache_mutex);
      m_filename_to_stop_watch[m_filename]->start();
    }

    auto bytes_read =
        contents->read(&(block_contents[0]), block_end - block_start);

    {
      std::lock_guard<mutex> file_size_guard(m_filesize_cache_mutex);
      auto& stop_watch = m_filename_to_stop_watch[m_filename];
      if (stop_watch->is_time_to_record()) {
        logprogress_stream
            << "Finished fetching block " << block_number << ". Elapsed "
            << stop_watch->template duration<std::chrono::seconds>().count()
            << "s for downloading " << sanitize_url(m_filename) << std::endl;
      }
    }

    // read failed.
    static_assert(std::is_signed<decltype(bytes_read)>::value,
                  "decltype(bytes_read) signed");
    static_assert(std::is_integral<decltype(bytes_read)>::value,
                  "decltype(bytes_read) integral");
    if (bytes_read < truncate_check<int64_t>(block_end - block_start)) {
      return false;
    }

    // write the block
    bool write_block_ok = bc.write(key, block_contents);
    if (write_block_ok == false) {
      logstream(LOG_ERROR) << "Unable to write block " << key << std::endl;
      // still ok. we can continue. but too many of these are bad.
    }
    // since we just read the block, lets fill the output
    const char* src = block_contents.c_str();
    memcpy(output, src + startpos, length);
    return true;
  }

};  // end of read_caching_device

template <typename T>
mutex read_caching_device<T>::m_filesize_cache_mutex;

template <typename T>
std::map<std::string, size_t>
    read_caching_device<T>::m_filename_to_filesize_map;

template <typename T>
std::map<std::string, std::unique_ptr<StopWatchSec_t>>
    read_caching_device<T>::m_filename_to_stop_watch;
}  // namespace turi
#endif

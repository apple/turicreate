/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_FILE_LINE_COUNT_ESTIMATOR_HPP
#define TURI_UNITY_FILE_LINE_COUNT_ESTIMATOR_HPP
#include <cstdlib>
namespace turi {

/**
 * Estimate the number of lines in a file and the number of bytes used
 * to represent each line.
 *
 * We estimate the number of lines in a file by making continuous observations
 * of the current file position, and the number of lines read so far, and
 * making simple assumptions about buffering behavior.
 *
 * \code
 * ifstream fin;
 * file_line_count_estimator estimator;
 * while(...) {
 *    read_lines.
 *    estimator.observe(lines_read since we last called observe,
 *                      fin.tellg());
 *    estimator.lines_in_file() contains estimate of the number
 *                              of lines in the file
 * }
 * \endcode
 *
 */
class file_line_count_estimator {
 public:
  /**
   * The default constructor.
   * If used, set_file_size must be used to set the filesize in bytes.
   */
  inline file_line_count_estimator() {}

  /**
   * Constructs a file line count estimator.
   * \param file_size_in_bytes The file size in bytes.
   */
  inline file_line_count_estimator(size_t file_size_in_bytes):
      file_size(file_size_in_bytes) {}

  /**
   * Sets the file size in bytes.
   */
  inline void set_file_size(size_t file_size_in_bytes) {
    file_size = file_size_in_bytes;
  }


  /**
   * Integrates statistics from another estimator
   */
  inline void observe(file_line_count_estimator& other_estimator) {
    accumulated_bytes += other_estimator.accumulated_bytes;
    accumulated_lines += other_estimator.accumulated_lines;
    num_observations += other_estimator.num_observations;
  }

  /**
   * This should be called for every block of read operations performed on the
   * file. Missing observations will cause the estimate to drift.
   * The more frequently this is called (preferably once for every line),
   * the more accurate the estimate.
   */
  inline void observe(size_t line_count, size_t file_pos) {
    if (file_pos == 0) {
      // no reads have been performed yet. How can line_count have anything?
      return;
    }

    if (file_pos != 0 && last_file_pos == 0) {
      // first read has been performed. buffer is now filled.
      last_file_pos = file_pos;
      last_buffer_size = file_pos;
      current_lines_from_buffer += line_count;
    } else if (file_pos == last_file_pos) {
      //  we are now reading from the buffer.
      current_lines_from_buffer += line_count;
    } else if (file_pos != last_file_pos) {
      //  we have now switched buffers
      accumulated_lines += current_lines_from_buffer + line_count;
      accumulated_bytes += last_buffer_size;

      current_lines_from_buffer = 0;
      last_buffer_size = file_pos - last_file_pos;
      last_file_pos = file_pos;
      ++num_observations;
    }

  }

  /**
   * The current estimate of the number of lines left in the file.
   * This returns 0 if the estimate is not available. One call to observe
   * is sufficient to get a rough estimate.
   */
  inline double number_of_lines() const {
    if (accumulated_lines == 0) {
      return (double)file_size / last_buffer_size * current_lines_from_buffer;
    } else {
      // say we have on average half a buffer excess in the accumulated bytes
      return (double)file_size / accumulated_bytes * accumulated_lines;
    }
  }

  /**
   * Total number of lines observed so far
   */
  inline size_t num_lines_observed() const {
    return accumulated_lines + current_lines_from_buffer;
  }


 private:
  /// the size of the file in bytes
  size_t file_size = 0;

  /// The number of lines read that are no longer in a buffer.
  size_t accumulated_lines = 0;

  /// The number of bytes read that are no longer in a buffer.
  size_t accumulated_bytes = 0;

  /// The number of lines read that may still be in a buffer.
  size_t current_lines_from_buffer = 0;

  /// The last file position we have seen.
  size_t last_file_pos = 0;

  /// The last change in file position (i.e. the buffer size)
  size_t last_buffer_size = 0;

  /** The effective number of observations made. i.e.
   *  The number of times a buffer size change was observed
   */
  size_t num_observations = 0;
};


} // namespace turi
#endif

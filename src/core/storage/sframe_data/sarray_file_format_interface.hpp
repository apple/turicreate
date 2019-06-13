/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SARRAY_FILE_FORMAT_INTERFACE_HPP
#define TURI_UNITY_SARRAY_FILE_FORMAT_INTERFACE_HPP

#define BOOST_SPIRIT_THREADSAFE

#include <cstdlib>
#include <string>
#include <sstream>
#include <map>
#include <core/storage/fileio/general_fstream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <core/storage/sframe_data/sarray_index_file.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_data/sframe_rows.hpp>
namespace turi {

/**
 * \internal
 * \ingroup sframe_physical
 * \addtogroup sframe_internal SFrame Internal
 * \{
 */

/**
 * A generic \ref sarray file format reader interface. File format
 * implementations should extend this.
 *
 * The sarray file layout should generally be a file set (collection of files)
 * with a common prefix. File format implementations can create or use as many
 * prefixes as required. There must be an [prefix].sidx in the the Microsoft
 * INI format with the following sections
 *
 * \verbatim
 * [sarray]
 * ; The version of the file format. Required.
 * version=0
 * \endverbatim
 */
template <typename T>
class sarray_format_reader_common_base {
 public:
  virtual ~sarray_format_reader_common_base() {}


  /**
   * Open has to be called before any of the other functions are called.
   * Throws a string exception if it is unable to open the file set, or if there
   * is a format error in the sarray.
   *
   * Will throw an exception if a file set is already open.
   */
  virtual void open(index_file_information index) = 0;

  /**
   * Open has to be called before any of the other functions are called.
   * Throws a string exception if it is unable to open the file set, or if there
   * is a format error in the sarray.
   *
   * Will throw an exception if a file set is already open.
   */
  virtual void open(std::string sidx_file) = 0;

  /**
   * Closes an sarray file set. No-op if the array is already closed.
   */
  virtual void close() = 0;

  /**
   * Return the number of segments in the sarray.
   * Throws an exception if the array is not open.
   */
  virtual size_t num_segments() const = 0;

  /**
   * Returns the number of elements in a given segment.
   * should throw an exception if the segment ID does not exist,
   */
  virtual size_t segment_size(size_t segmentsid) const = 0;

  /**
   * Reads a collection of rows, storing the result in out_obj.
   * This function is independent of the open_segment/read_segment/close_segment
   * functions, and can be called anytime. This function is also fully
   * concurrent.
   * \param row_start First row to read
   * \param row_end one past the last row to read (i.e. EXCLUSIVE). row_end can
   *                be beyond the end of the array, in which case,
   *                fewer rows will be read.
   * \param out_obj The output array
   * \returns Actual number of rows read. Return (size_t)(-1) on failure.
   */
  virtual size_t read_rows(size_t row_start,
                           size_t row_end,
                           std::vector<T>& out_obj) = 0;

  /**
   * Returns the file index of the array (the argument in \ref open)
   */
  virtual std::string get_index_file() const = 0;

  /**
   * Gets the contents of the index file information read from the index file
   */
  virtual const index_file_information& get_index_info() const = 0;
};

template <typename T>
class sarray_format_reader : public sarray_format_reader_common_base<T> {
 public:
  virtual ~sarray_format_reader() {}
};

template <>
class sarray_format_reader<flexible_type>
      : public sarray_format_reader_common_base<flexible_type> {
 public:
  virtual ~sarray_format_reader() {}

  /**
   * Reads a collection of rows, storing the result in out_obj.
   * This function is independent of the open_segment/read_segment/close_segment
   * functions, and can be called anytime. This function is also fully
   * concurrent.
   * \param row_start First row to read
   * \param row_end one past the last row to read (i.e. EXCLUSIVE). row_end can
   *                be beyond the end of the array, in which case,
   *                fewer rows will be read.
   * \param out_obj The output array
   * \returns Actual number of rows read. Return (size_t)(-1) on failure.
   */
  using sarray_format_reader_common_base::read_rows;

  virtual size_t read_rows(size_t row_start,
                           size_t row_end,
                           sframe_rows& out_obj) {
    size_t ret = 0;
    out_obj.resize(1);
    ret = read_rows(row_start, row_end, *(out_obj.get_columns()[0]));
    return ret;
  }
};




/**
 * A generic \ref sarray group file format writer interface. File format
 * implementations should extend this.
 *
 * The sarray_group is a collection of sarrays in a single file set.
 *
 * The writer is assumed to always to writing to new file sets; we are
 * never modifying an existing file set.
 */
template <typename T>
class sarray_group_format_writer {
 public:
  virtual ~sarray_group_format_writer() {}

  /**
   * Open has to be called before any of the other functions are called.
   * No files are actually opened at this point.
   */
  virtual void open(std::string index_file,
                    size_t segments_to_create,
                    size_t columns_to_create) = 0;

  /**
   * Set write options.
   * Available options are
   * "disable_padding" = true or false
   */
  virtual void set_options(const std::string& option, int64_t value) = 0;

  /**
   * Gets a modifiable reference to the index file information which will
   * be written to the index file. Can only be called after close()
   */
  virtual group_index_file_information& get_index_info() = 0;


  /** Closes all segments.
   */
  virtual void close() = 0;

  /**
   * Flushes the index_file_information to disk
   */
  virtual void write_index_file() = 0;


  /**
   * Writes a row to the array group
   */
  virtual void write_segment(size_t segmentid,
                             const std::vector<T>&) = 0;

  /**
   * Writes a row to the array group
   */
  virtual void write_segment(size_t segmentid,
                             std::vector<T>&&) = 0;

  /**
   * Writes a row to the array group
   */
  virtual void write_segment(size_t columnid,
                             size_t segmentid,
                             const T&) = 0;

  /**
   * Writes a row to the array group
   */
  virtual void write_segment(size_t columnid,
                             size_t segmentid,
                             T&&) = 0;

  /**
   * Writes a bunch of rows to the array group
   */
  virtual void write_segment(size_t segmentid,
                             const sframe_rows& rows) = 0;

  /**
   * Writes a collection of rows to a column
   */
  virtual void write_column(size_t columnid,
                            size_t segmentid,
                            const std::vector<T>& t) = 0;

  /**
   * Writes a collection of rows to a column
   */
  virtual void write_column(size_t columnid,
                            size_t segmentid,
                            std::vector<T>&& t) = 0;

  /**
   * Flush all writes for a particular segment
   */
  virtual void flush_segment(size_t segmentid) { }

  /**
   * Return the number of segments in the sarray.
   * Throws an exception if the array is not open.
   */
  virtual size_t num_segments() const = 0;


  /**
   * Return the number of columns in the sarray.
   * Throws an exception if the array is not open.
   */
  virtual size_t num_columns() const = 0;
};


/// \}
} // namespace turi

#endif

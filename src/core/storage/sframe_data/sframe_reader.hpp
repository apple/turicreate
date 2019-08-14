/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_LIB_SFRAME_READER_HPP
#define TURI_UNITY_LIB_SFRAME_READER_HPP
#include <iostream>
#include <algorithm>
#include <memory>
#include <vector>
#include <core/logging/logger.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_data/sarray_reader.hpp>
#include <core/storage/sframe_data/sframe_index_file.hpp>
#include <core/storage/sframe_data/sframe_constants.hpp>


namespace turi {
// forward declaration of th csv_line_tokenizer to avoid a
// circular dependency
struct csv_line_tokenizer;
class sframe;


/**
 * \ingroup sframe_physical
 * \addtogroup sframe_main Main SFrame Objects
 * \{
 */

/**
 * A input iterator over an SFrame.
 *
 * The sframe_iterator provides a simple input iterator (like forward iterator,
 * but one pass. i.e. increment of one, invalidates all other copies.) over a
 * segment of an sframe. It essentially exposes a column of vectors, where each
 * vector is a row in a table.
 *
 * Since this class wraps several sarray_iterators, it inherits their guarantees,
 * and is thus an input iterator.
 */
class sframe_iterator {
 public:
  // Standard iterator stuff
  typedef std::vector<flexible_type> value_type;
  typedef int difference_type;
  typedef value_type* pointer;
  typedef value_type& reference;
  typedef std::input_iterator_tag iterator_category;

  sframe_iterator() {}

  /**
   * Constructs an iterator from the underlying data structure of an SFrame
   *
   * \param data The "SFrame" to iterate over
   * \param segmentid The segment to read. Must be a valid segment.
   * \param is_begin_iterator If true, constructs an iterator pointing to
   *                            the first row of the given segment
   *                          If false, constructs an iterator pointing to
   *                            one row past the end of the given segment
   */
  sframe_iterator(
      const std::vector<std::shared_ptr<sarray_reader<flexible_type> > > &data,
      size_t segmentid,
      bool is_begin_iterator) : _data(&data),
    _segmentid(segmentid) {

      // Create an SArray iterator for each column of the SFrame.
      cur_iter.resize(_data->size());
      cur_element.resize(_data->size());
      for(size_t i = 0; i < _data->size(); ++i) {
        if(is_begin_iterator) {
          cur_iter[i] = _data->at(i)->begin(segmentid);
        } else {
          cur_iter[i] = _data->at(i)->end(segmentid);
        }
      }

      // Variables that make equality easier to check
      segment_limit = _data->at(0)->segment_length(segmentid);

      if(is_begin_iterator) {
        cur_segment_pos = 0;
      } else {
        cur_segment_pos = segment_limit;
      }
    }

  /**
   * Advances the iterator to the next row of the segment
   */
  sframe_iterator& operator++() {
    for(auto& i : cur_iter) {
      ++i;
    }

    ++cur_segment_pos;

    // Never go past the limit (one past the end of the segment)
    if(cur_segment_pos > segment_limit) {
      cur_segment_pos = segment_limit;
    }

    return *this;
  }


  /*
   * This is the post-fix increment.  Returns the previous value of the
   * iterator.
   */
  sframe_iterator operator++(int) {
    sframe_iterator orig = *this;
    ++(*this);
    return orig;
  }

  // Default assignment operator
  sframe_iterator& operator=(const sframe_iterator& other) = default;

  // Default copy constructor
  sframe_iterator(const sframe_iterator& other) = default;

  /**
   * Returns true if iterators are identical (points to the same SFrame,
   * in the same segment, at the same position)
   */
  bool operator==(const sframe_iterator& other) const {
    return _data == other._data &&
        _segmentid == other._segmentid &&
        cur_segment_pos == other.cur_segment_pos;
  }

  /**
   * Returns true if iterators are different (different SFrame, different
   * segment, or different position)
   */
  bool operator!=(const sframe_iterator& other) const {
    return _data != other._data ||
        _segmentid != other._segmentid ||
        cur_segment_pos != other.cur_segment_pos;
  }

  /**
   * Returns the current element. Value will be invalid if the iterator
   * is past the end of the sarray (points to end)
   */
  const value_type& operator*() const {
    for (size_t i = 0; i < _data->size(); ++i) {
      cur_element[i] = *(cur_iter[i]);
    }
    return cur_element;
  }

  /**
   * Returns a pointer to the current element. Value will be invalid if
   * iterator == end.
   */
  const value_type *operator->() const {
    this->operator*();
    return &cur_element;
  }

  /**
   * Returns the distance between two iterators. Both iterators must be
   * from the same segment of the same sframe, otherwise result is undefined.
   */
  int operator-(const sframe_iterator& other) const {
    return (int)(cur_segment_pos) - (int)(other.cur_segment_pos);
  }
 private:
  const std::vector<std::shared_ptr<sarray_reader<flexible_type> > > *_data;
  std::vector<sarray_iterator<flexible_type> > cur_iter;
  // Relies on all the segments being the same length
  size_t _segmentid = (size_t)(-1);
  size_t cur_segment_pos = 0;
  size_t segment_limit = 0;
  mutable value_type cur_element;
};

/**
 * The sframe_reader provides a reading interface to an \ref sframe :
 * an immutable on-disk set of columns, each with
 * their own type.  These types are represented as a flexible_type.
 *
 * The SFrame is represented as an ordered set of SArrays, each with an
 * enforcable name and type. Each SArray in an SFrame must have the same
 * number of segments as all other SArrays in the SFrame, which each must
 * have the same number of elements as all other segments. A segment of an
 * SFrame is a disjoint subset of rows with an entry from each column.
 * Segments can be read in parallel.
 *
 * To read from an sframe use \ref sframe::get_reader():
 * \code
 * auto reader = frame.get_reader();
 * \endcode
 * reader will be of type sframe_reader
 *
 * reader can then provide input iterators from segments via the begin()
 * and end() functions.
 */
class sframe_reader : public siterable<sframe_iterator> {
 public:
  /// The iterator type which \ref begin and \ref end returns
  typedef sframe_iterator iterator;

  /// The value type the sframe stores
  typedef sframe_iterator::value_type value_type;

  /**
   * Constructs an empty sframe.
   */
  sframe_reader() = default;

  /// Deleted Copy constructor
  sframe_reader(const sframe_reader& other) = delete;

  /// Deleted Assignment operator
  sframe_reader& operator=(const sframe_reader& other) = delete;

  /**
   * Attempts to construct an sframe_iterator which reads
   * If the index file cannot be opened, an exception is thrown.
   *
   * \param array The array to read
   * \param num_segments If num_segments == (size_t)(-1), the
   *                     segmentation of the first column is used. Otherwise,
   *                     the array is cut into num_segments number of
   *                     logical segments which distribute the rows uniformly.
   */
  void init(const sframe& array, size_t num_segments = (size_t)(-1));

  /**
   * Attempts to construct an sframe_iterator which reads from
   * an existing sframe and uses a segmentation defined by an argument.
   * If the index file cannot be opened, an exception is thrown.
   * If the sum of the lengths of all the segments do not add up to the
   * length of the sframe , an exception is thrown
   *
   * \param array The frame to read
   * \param segment_lengths An array describing the lengths of each segment.
   *                        This must sum up to the length of the array.
   */
  void init(const sframe& array, const std::vector<size_t>& segment_lengths);

  /// Return the begin iterator of the segment.
  iterator begin (size_t segmentid) const;

  /// Return the end iterator of the segment.
  iterator end (size_t segmentid) const;

  /**
   * Reads a collection of rows, storing the result in out_obj.
   * This function is independent of the begin/end iterator
   * functions, and can be called anytime. This function is also fully
   * concurrent.
   * \param row_start First row to read
   * \param row_end one past the last row to read (i.e. EXCLUSIVE). row_end can
   *                be beyond the end of the array, in which case,
   *                fewer rows will be read.
   * \param out_obj The output array
   * \returns Actual number of rows read. Return (size_t)(-1) on failure.
   *
   * \note This function is not always efficient. Different file formats
   * implementations will have different characteristics.
   */
  size_t read_rows(size_t row_start,
                   size_t row_end,
                   std::vector<std::vector<flexible_type> >& out_obj);


  /**
   * Reads a collection of rows, storing the result in out_obj.
   * This function is independent of the begin/end iterator
   * functions, and can be called anytime. This function is also fully
   * concurrent.
   * \param row_start First row to read
   * \param row_end one past the last row to read (i.e. EXCLUSIVE). row_end can
   *                be beyond the end of the array, in which case,
   *                fewer rows will be read.
   * \param out_obj The output array
   * \returns Actual number of rows read. Return (size_t)(-1) on failure.
   *
   * \note This function is not always efficient. Different file formats
   * implementations will have different characteristics.
   */
  size_t read_rows(size_t row_start,
                   size_t row_end,
                   sframe_rows& out_obj);


  /**
   * Resets all the file handles. All existing iterators are invalidated.
   */
  void reset_iterators();

  /// Returns the number of columns in the SFrame. Does not throw.
  inline size_t num_columns() const {
    return index_info.ncolumns;
  }

  /// Returns the length of each sarray.
  inline size_t num_rows() const {
    return index_info.nrows;
  }

  /// Returns the length of each sarray.
  inline size_t size() const {
    return index_info.nrows;
  }

  /**
   * Returns the name of the given column.  Throws an exception if the
   * column id is out of range.
   */
  inline std::string column_name(size_t i) const {
    ASSERT_LT(i, index_info.ncolumns);

    return index_info.column_names[i];
  }

  /**
   * Returns the type of the given column.  Throws an exception if the
   * column id is out of range.
   */
  inline flex_type_enum column_type(size_t i) const {
    ASSERT_LT(i, index_info.ncolumns);

    return column_data[i]->get_type();
  }

  /// Returns the number of segments in the SFrame. Does not throw.
  inline size_t num_segments() const {
    return m_num_segments;
  }

  /**
   * Returns the length of the given segment.  Throws an exception if the
   * segment id is out of range.
   */
  inline size_t segment_length(size_t segment) const {
    ASSERT_LT(segment, num_segments());
    if (index_info.ncolumns == 0) return 0;
    return column_data[0]->segment_length(segment);
  }


  /**
   * Returns true if the sframe contains the given column.
   */
  inline bool contains_column(const std::string& column_name) const {
    auto iter = std::find(index_info.column_names.begin(),
                          index_info.column_names.end(),
                          column_name);
    return iter != index_info.column_names.end();
  }
  /**
   * Returns the column index of column_name.
   *
   * Throws an exception of the column_ does not exist.
   */
  inline size_t column_index(const std::string& column_name) const {
    auto iter = std::find(index_info.column_names.begin(),
                          index_info.column_names.end(),
                          column_name);
    if (iter != index_info.column_names.end()) {
      return (iter) - index_info.column_names.begin();
    } else {
      throw (std::string("Column name " + column_name + " does not exist."));
    }
  }

 private:

  // Internal data storage
  bool inited = false;
  sframe_index_file_information index_info;
  std::vector<std::shared_ptr<sarray_reader<flexible_type> > > column_data;
  buffer_pool<std::vector<flexible_type>> column_pool;
  size_t m_num_segments = 0;
};

/// \}
} // end of namespace turi



namespace std {

// specialization of std::distance
inline int distance(const turi::sframe_iterator& begin,
                    const turi::sframe_iterator& end) {
  return end - begin;
}

} // namespace std

#endif

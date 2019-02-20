/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SFRAME_SARRAY_READER_HPP
#define TURI_UNITY_SFRAME_SARRAY_READER_HPP
#include <set>
#include <iterator>
#include <type_traits>
#include <functional>
#include <logger/assertions.hpp>
#include <serialization/iarchive.hpp>
#include <parallel/pthread_tools.hpp>
#include <sframe/siterable.hpp>
#include <flexible_type/flexible_type.hpp>
#include <sframe/sarray_file_format_v2.hpp>
#include <sframe/sframe_constants.hpp>
#include <fileio/file_ownership_handle.hpp>
#include <sframe/sarray_reader_buffer.hpp>
namespace turi {

template <typename T>
class sarray;



/**
 * \ingroup sframe_physical
 * \addtogroup sframe_main Main SFrame Objects
 * \{
 */

/**
 * Implements a simple input iterator over an sarray.
 * 
 * The sarray_iterator provides a simple input iterator (like forward iterator,
 * but one pass. i.e. increment of one, invalidates all other copies.) over a 
 * segment of an sarray.
 */
template <typename T>
class sarray_iterator {
 public:
  // standard iterator stuff
  typedef T value_type;
  typedef int difference_type;
  typedef T* pointer;
  typedef T& reference;
  typedef std::input_iterator_tag iterator_category;

  /**
   * Default constructor. Makes an iterator which points nowhere.
   * Use of this iterator will produce undefined results.
   */
  sarray_iterator()
      :reader(NULL), segmentid(0), 
       current_element(T()), current_element_counter(0) {} 

  /// Default copy constructor
  sarray_iterator(const sarray_iterator& other) = default;

  /// Default assignment operator
  sarray_iterator& operator=(const sarray_iterator& other) = default;

  /**
   * Constructs an iterator from an input archive. 
   * Creates a start iterator to the segment.
   *
   * \param reader The file format reader to use
   * \param segmentid The segment to read. Must be a valid segment
   * \param is_start_iterator If true, creates a start iterator to the segment.
   *                          If false, creates an end iterator to the segment.
   */
  sarray_iterator(sarray_reader_buffer<T>* reader, 
                  size_t segmentid, 
                  bool is_start_iterator)
      :reader(reader), segmentid(segmentid),
       current_element(T()), current_element_counter(0) {
    num_elements = reader->size();
    if (is_start_iterator) {
      if (num_elements > 0) {
        current_element = reader->next();
      }
      current_element_counter = 0;
    } else {
      // one past the end
      current_element_counter = num_elements;
    }
  }

  /**
   * Advances the iterator to the next element
   */
  sarray_iterator& operator++() { 
    if (reader->has_next()) {
      current_element = reader->next();
      ++current_element_counter;
    } else {
      current_element_counter = num_elements;
      reader->clear();
    }
    return *this; 
  }

  /**
   * Equivalent to other operator++ for input iterators
   */
  void operator++(int) {
    /// forward to the other operator++
    ++(*this);
  }

  /**
   * Returns true if the iterators are identical (points to the same
   * element in the same sarray).
   */
  bool operator==(const sarray_iterator& other) const {
    return (reader == other.reader) && 
        (segmentid == other.segmentid) && 
        (current_element_counter == other.current_element_counter);
  }

  /**
   * Returns true if the iterators are different (points to different
   * elements, or in different sarrays)
   */
  bool operator!=(const sarray_iterator& other) const {
    return !((*this) == other);
  }

  /**
   * Returns the current element. Value will be invalid if the iterator
   * is past the end of the sarray (points to end)
   */
  const value_type& operator*() const {
    return current_element;
  }

  /**
   * \overload
   */
  value_type& operator*() {
    return current_element;
  }

  /**
   * Returns the distance between two iterators. Both iterators must be
   * from the same segment of the same sarray, otherwise result is undefined.
   */
  int operator-(const sarray_iterator& other) const {
    return (int)(current_element_counter) - (int)(other.current_element_counter);
  }

 private:
  /// The reader we are reading from
  sarray_reader_buffer<T>* reader;
  /// The segment we are reading from
  size_t segmentid;
  /// The last element read (returned by the deref operator)
  T current_element;
  /// Number of elements successfully read so far.
  size_t current_element_counter;
  /// Total number of elements
  size_t num_elements;
};



/**
 * The SArray reader provides a reading interface to 
 * an immutable, on disk, sequence of objects T.
 *
 * The SArray is an immutable sequence of objects of type T, and is internally 
 * represented as a collection of files. The sequence is cut up into a 
 * collection of segments (not necessarily of equal length), where each segment
 * covers a disjoint subset of the sequence. Each segment can then be read
 * in parallel. 
 *
 * To read from an sarray<T> use \ref sarray::get_reader():
 * \code
 * auto reader = array.get_reader();
 * \endcode
 * reader will be of type sarray_reader<T>
 * 
 * reader can then provide input iterators from segments via the begin()
 * and end() functions.
 */
template <typename T>
class sarray_reader: public siterable<sarray_iterator<T> > {
 public:
  /// The iterator type which \ref begin and \ref end returns
  typedef sarray_iterator<T> iterator;

  /// The value type the sarray stores
  typedef typename iterator::value_type value_type;

  /**
   * Default constructor. Does nothing. Use init() 
   */
  sarray_reader() = default;

  /// Deleted Copy constructor
  sarray_reader(const sarray_reader& other) = delete;

  /// Assignment operator
  sarray_reader& operator=(const sarray_reader& other) = delete;




  ~sarray_reader() {
    if (reader) delete reader;
  }


  /**
   * Attempts to construct an sarray_iterator which reads from 
   * an existing sarray.
   * If the index file cannot be opened, an exception is thrown.
   *
   * \param array The array to read
   * \param num_segments If num_segments == (size_t)(-1), the 
   *                     original file segmentation is used. Otherwise,
   *                     the array is cut into num_segments number of 
   *                     logical segments which distribute the rows uniformly.
   */
  void init(const sarray<T>& array, size_t num_segments = (size_t)(-1)) {
    ASSERT_MSG(!reader, "Reader already inited");
    open_format_reader(array);
    // row start and row end of each segment
    std::vector<std::pair<size_t, size_t> > segment_row_start_end;
    if (num_segments == (size_t)(-1)) {
      // we use the original sarray file layout
      auto index_info = array.get_index_info();
      // the cumulative sum across segment lengths
      size_t current_row_index = 0;
      // figure out the start and end position for each segment
      for (size_t i = 0;i < index_info.nsegments; ++i) {
        size_t row_start = current_row_index;
        size_t row_end = row_start + index_info.segment_sizes[i];
        segment_row_start_end.push_back({row_start, row_end});
        current_row_index = row_end;
      }
    } else {
      // we equally divide the data across the segments 
      // compute the segment lengths
      ASSERT_GT(num_segments, 0);
      size_t totallength = size();
      // figure out the start and end position for each segment
      for (size_t i = 0;i < num_segments; ++i) {
        size_t row_start = ((i * totallength) / num_segments);
        size_t row_end = (((i + 1) * totallength) / num_segments);
        segment_row_start_end.push_back({row_start, row_end});
      }
    }
    create_segment_read_buffers(segment_row_start_end);
    files_managed = array.files_managed;
  }

  /**
   * Attempts to construct an sarray_iterator which reads from 
   * an existing sarray and uses a segmentation defined by an argument.
   * If the index file cannot be opened, an exception is thrown.
   * If the sum of the lengths of all the segments do not add up to the 
   * length of the sarray, an exception is thrown
   *
   * \param array The array to read
   * \param segment_lengths An array describing the lengths of each segment.
   *                        This must sum up to the length of the array.
   */
  void init(const sarray<T>& array, const std::vector<size_t>& segment_lengths) {
    ASSERT_MSG(!reader, "Reader already inited");
    open_format_reader(array);
    // check that lengths add up
    size_t sum = 0;
    for(size_t s: segment_lengths) sum += s;
    ASSERT_EQ(sum, size());

    // row start and row end of each segment
    std::vector<std::pair<size_t, size_t> > segment_row_start_end;

    // the cumulative sum across segment lengths
    size_t current_row_index = 0;
    // figure out the start and end position for each segment
    for (size_t i = 0;i < segment_lengths.size(); ++i) {
      size_t row_start = current_row_index;
      size_t row_end = row_start + segment_lengths[i];
      segment_row_start_end.push_back({row_start, row_end});
      current_row_index = row_end;
    }
    create_segment_read_buffers(segment_row_start_end);
  }

  /** 
   * Return the number of segments in the collection. 
   * Will throw an exception if the sarray is invalid (there is an error
   * reading files)
   */
  size_t num_segments() const {
    ASSERT_NE(reader, NULL);
    return m_num_segments;
  }

  /** 
   * Return the number of rows in the segment.
   * Will throw an exception if the sarray is invalid (there is an error
   * reading files)
   */
  size_t segment_length(size_t segment) const {
    ASSERT_NE(reader, NULL);
    return m_segment_lengths[segment];
  }

  /**
   * Return the file prefix of the sarray (paramter on construction)
   */
  std::string get_index_file() const {
    ASSERT_NE(reader, NULL);
    return reader->get_index_file();
  }


  /**
   * Returns the collection of files storing the sarray.
   * For instance: [file_prefix].sidx, [file_prefix].0001, etc.
   */
  std::vector<std::string> get_file_names() const {
    ASSERT_NE(reader, NULL);
    return reader->get_index_info().segment_files;
  }

  /**
   * Reads the value of a key associated with the sarray.
   * Returns true on success, false on failure.
   */
  bool get_metadata(std::string key, std::string &val) const {
    ASSERT_NE(reader, NULL);
    if (reader == NULL) log_and_throw(std::string("Invalid sarray"));
    bool ret = false;
    std::tie(ret, val) = get_metadata(key);
    return ret;
  }

  /**
   * Reads the value of a key associated with the sarray.
   * Returns a pair of (true, value) on success, and (false, empty_string)
   * on failure.
   */
  std::pair<bool, std::string> get_metadata(std::string key) const {
    ASSERT_NE(reader, NULL);
    const index_file_information& index_info = reader->get_index_info();
    if (index_info.metadata.count(key)) {
      return std::pair<bool, std::string>(true, 
                                          index_info.metadata.at(key));
    } else {
      return std::pair<bool, std::string>(false, "");
    }
  }

  /**
   * Returns the number of elements in the SArray
   */
  size_t size() const {
    ASSERT_NE(reader, NULL);
    size_t total = 0;
    const index_file_information& info = reader->get_index_info();
    for (size_t i = 0;i < info.segment_sizes.size(); ++i) {
      total += info.segment_sizes[i];
    }
    return total;
  }


  /** 
   * Return the begin iterator of the segment.
   * The iterator (\ref sarray_iterator) is of the input iterator type and 
   * has value_type T. See \ref end() to get the end iterator of the segment.
   *
   * The iterator is invalid once the originating sarray is destroyed.
   * Accessing the iterator after the sarray is destroyed is undefined behavior.
   *
   * \code
   * // example to print segment 1 to screen
   * auto iter = sarr.begin(1);
   * auto enditer =sarr.end(1);
   * while(iter != enditer) {
   *   std::cout << *iter << "\n";
   *   ++iter;
   * }
   * \endcode
   *
   * Will throw an exception if the sarray is invalid (there is an error
   * reading files) Also segmentid must be a valid segment ID. Will throw an
   * exception otherwise.
   */
  iterator begin(size_t segmentid) const {
    std::lock_guard<mutex> lck(lock);
    if (opened_segments.count(segmentid) == 0) {
      opened_segments.insert(segmentid);
    } else {
      log_and_throw(std::string("Must reset sarray iterators!"));
    }
    if (reader == NULL) log_and_throw(std::string("Invalid sarray"));
    if (segmentid >= num_segments()) log_and_throw(std::string("Invalid segment ID"));
    return iterator(&(m_read_buffers[segmentid]), segmentid, true);
  }

  /** Return the end iterator of the segment.
   * The iterator (\ref sarray_iterator) is of the input iterator type and 
   * has value_type T. See \ref end() to get the end iterator of the segment.
   *
   * The iterator is invalid once the originating sarray is destroyed.
   * Accessing the iterator after the sarray is destroyed is undefined behavior.
   *
   * \code
   * // example to print segment 1 to screen
   * auto iter = sarr.begin(1);
   * auto enditer =sarr.end(1);
   * while(iter != enditer) {
   *   std::cout << *iter << "\n";
   *   ++iter;
   * }
   * \endcode
   *
   * Will throw an exception if the sarray is invalid (there is an error
   * reading files) Also segmentid must be a valid segment ID. Will throw an
   * exception otherwise.
   */
  iterator end(size_t segmentid) const {
    ASSERT_NE(reader, NULL);
    ASSERT_LT(segmentid, num_segments());
    return iterator(&(m_read_buffers[segmentid]), segmentid, false);
  }


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
   *
   * \note This function is not always efficient. Different file formats
   * implementations will have different characteristics.
   */
  size_t read_rows(size_t row_start, 
                   size_t row_end, 
                   std::vector<T>& out_obj) {
    DASSERT_NE(reader, NULL);
    return reader->read_rows(row_start, row_end, out_obj);
  }

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
  void reset_iterators() {
    std::lock_guard<mutex> lck(lock);
    opened_segments.clear();
    for (auto& buf: m_read_buffers) buf.clear();
  }


  /**
   * Returns the type of the SArray (as set by 
   * \ref swriter<flexible_type>::set_type). If the type of the SArray was
   * not set, this returns \ref flex_type_enum::UNDEFINED, in which case
   * each row can be of arbitrary type.
   *
   * This function should only be used for sarray<flexible_type> and
   * will fail fatally otherwise.
   */
  flex_type_enum get_type() const {
    if (!std::is_same<T, flexible_type>::value) {
      ASSERT_MSG(false, 
                  "Use of get_type() in SArray which "
                  "does not contain flexible_types");
    }
    ASSERT_NE(reader, NULL);
    const index_file_information& index_info = reader->get_index_info();
    if (index_info.metadata.count("__type__")) {
      return flex_type_enum::UNDEFINED;
    }
    return flex_type_enum(std::stoi(index_info.metadata.at("__type__")));
  }


 private:
  mutable sarray_format_reader<T>* reader = NULL;
  mutable mutex lock;
  size_t m_num_segments = 0;
  mutable std::set<size_t> opened_segments;
  std::vector<size_t> m_segment_lengths;
  // take a reference to all the handled files so that deletion of the sarray
  // does not cause the reader to become invalidated.
  std::vector<std::shared_ptr<fileio::file_ownership_handle>> files_managed;

  mutable std::vector<sarray_reader_buffer<T> > m_read_buffers;

  /**
   * Construct the format reader object from the sarray.
   * Only called by the init() functions.
   */
  void open_format_reader(const sarray<T>& array) {
    // redirect to the appropriate file reader
    // for now, we only have version 0
    size_t file_version = array.get_index_info().version;

    switch(file_version) {
     case 0:
       log_and_throw("Format version 0 deprecated");
       break;
     case 1:
       log_and_throw("Format version 1 deprecated");
       break;
     case 2:
       reader = new sarray_format_reader_v2<T>();
       reader->open(array.get_index_info());
       break;
     default:
       reader = NULL;
       log_and_throw("Invalid file format version");
       break;
    }
  }

  /**
   * Construct the format reader object from the sarray.
   * Only called by the init() functions. Also sets up the m_segment_lengths
   * and m_num_segments variables.
   */
  void create_segment_read_buffers(
      const std::vector<std::pair<size_t, size_t> >& segment_row_start_end) {
    // set up the read buffers
    m_num_segments = segment_row_start_end.size();
    m_segment_lengths.resize(m_num_segments);
    m_read_buffers.resize(m_num_segments);

    for (size_t i = 0;i < m_segment_lengths.size(); ++i) {
      m_segment_lengths[i] = 
          segment_row_start_end[i].second - segment_row_start_end[i].first;
      m_read_buffers[i].init(this,
                             segment_row_start_end[i].first, 
                             segment_row_start_end[i].second);
    }
  }
};

template <typename T>
inline size_t sarray_reader<T>::read_rows(size_t row_start, 
                                          size_t row_end, 
                                          sframe_rows& out_obj) {
  ASSERT_MSG(false, "read_rows() to sframe_rows not implemented for "
                    "non-flexible_type templatizations of sarray");
  return 0;
}


template <>
inline size_t sarray_reader<flexible_type>::read_rows(size_t row_start, 
                                                      size_t row_end, 
                                                      sframe_rows& out_obj) {
  DASSERT_NE(reader, NULL);
  return reader->read_rows(row_start, row_end, out_obj);
}

/// \}

} // namespace turi

namespace std {

// specialization of std::distance
template <typename T>
inline int distance(const turi::sarray_iterator<T>& begin,
                    const turi::sarray_iterator<T>& end) {
  return end - begin;
}

} // namespace std

#endif

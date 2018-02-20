/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SFRAME_SARRAY_HPP
#define TURI_UNITY_SFRAME_SARRAY_HPP
#include <set>
#include <iterator>
#include <type_traits>
#include <logger/logger.hpp>
#include <logger/assertions.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <sframe/output_iterator.hpp>
#include <serialization/iarchive.hpp>
#include <serialization/oarchive.hpp>
#include <parallel/pthread_tools.hpp>
#include <parallel/lambda_omp.hpp>
#include <sframe/swriter_base.hpp>
#include <sframe/sarray_file_format_v2.hpp>
#include <sframe/sarray_index_file.hpp>
#include <sframe/algorithm.hpp>
#include <sframe/sframe_config.hpp>
#include <flexible_type/flexible_type.hpp>
#include <fileio/fixed_size_cache_manager.hpp>
#include <fileio/file_ownership_handle.hpp>
#include <fileio/file_handle_pool.hpp>
#include <exceptions/error_types.hpp>
#include <sframe/sframe_constants.hpp>
#include <sframe/sarray_saving.hpp>


namespace turi {

template <typename T>
class sarray_reader;

namespace swriter_impl {
  // We define the iterator type here so as not to polute the outer namesapce
  //
template <typename T>
using output_iterator = turi::sframe_function_output_iterator<
              T,
              std::function<void(const T&)>, 
              std::function<void(T&&)>,
              std::function<void(const sframe_rows&)> >; 
} // namespace swriter_impl

/**
 * \ingroup sframe_physical
 * \addtogroup sframe_main Main SFrame Objects
 * \{
 */

/**
 * The SArray represents an immutable, on disk, sequence of objects T.
 *
 * The SArray is an immutable sequence of objects of type T, and is internally
 * represented as a collection of files. The sequence is cut up into a
 * collection of segments (not necessarily of equal length), where each segment
 * covers a disjoint subset of the sequence. Each segment can then be read
 * in parallel. SArray is referenced on disk by a single ".sidx" file, which
 * then has a list of file names, one file for each segment.
 *
 * The SArray is \b write-once, \b read-many. The SArray can be opened for
 * writing \b once, after which it is read-only.
 *
 * To open an existing sarray on disk for reading:
 * \code
 * sarray<int> array;
 * array.open_for_read("test.sidx");
 * \endcode
 * Note that the type of the array on disk is NOT checked.
 * (though, we probably should)
 *
 * To open an sarray for writing:
 * \code
 * sarray<int> array;
 * array.open_for_write(); // create an sarray backed with temporary files
 * //temporary files will be deleted when array goes out of scope
 * \endcode
 *
 * \code
 * sarray<int> array;
 * array.open_for_write("test.sidx"); // create an sarray backed by real files
 * \endcode
 *
 * When the array is opened for writing, it can written into using
 * \ref get_output_iterator() , to get an output iterator into each segment.
 * \code
 * // Gets the output iterator for the 3rd segment
 * auto iter = get_output_iterator(3);
 * // writes the value "5" into the segment
 * (*iter) = 5; ++iter;
 *
 * // when done,
 * close(); // closes the write.
 * \endcode
 *  The get_output_iterator() function can be called concurrently, but
 *  each individual output iterator is not concurrent.
 *  After close() is called, the sarray becomes a read-only array, and is
 *  equivalent to having called array.open_for_read(...)
 *
 * To read from the sarray, \ref get_reader() is used.
 * \code
 * auto reader = array.get_reader();
 * \endcode
 * Each reader provides read access to the SArray. Multiple readers can be
 * obtained, as each has its own distinct file handles which are closed as
 * the reader goes out of scope.
 * See the documentation for \ref sarray_reader for details.
 *
 * The sarray<flexible_type> has additional capabilities.
 *  - It has the functions set_type() and get_type() to set the run-time type
 *    of the stored values.
 *  - Writes to the sarray<flexible_type> type check against the type setted.
 *    The writes \b must match either the type set by set_type() or be
 *    UNDEFINED.
 *
 * \note The only guaranteed concurrent safe function is get_output_iterator.
 * All other mutating functions are not guaranteed to be safe.
 */
template <typename T>
class sarray : public swriter_base<swriter_impl::output_iterator<T> > {
 public:
  /// The reader type
  typedef sarray_reader<T> reader_type;

  /// The iterator type which \ref get_output_iterator returns
  typedef swriter_impl::output_iterator<T> iterator;

  /// The type contained in the sarray
  typedef T value_type;

  /**
   * default constructor; does nothing; use \ref open_for_read or
   * \ref open_for_write after construction to read/create an sarray.
   */
  sarray() = default;

  /// Move constructor
  sarray(sarray&& other):sarray() {
    (*this) = std::move(other);
  }

  /// Copy constructor
  sarray(const sarray& other) {
    (*this) = other;
  }

   /// Assignment operator
  sarray& operator=(const sarray& other) {
    if (other.inited && other.writing) {
      throw("Cannot copy an array which is writing");
    }
    if (writing) {
      throw("Cannot copy over an array which is writing");
    }
    index_info = other.index_info;
    index_file = other.index_file;
    files_managed = other.files_managed;
    inited = other.inited;
    writing = other.writing;
    return (*this);
  }


  /** Move assignment.
   * Moves other into this. Other will be cleared as if it is a newly
   * constructed sarray object.
   */
  sarray& operator=(sarray&& other) {
    index_info = std::move(other.index_info);
    index_file = std::move(other.index_file);
    writer = std::move(other.writer);
    files_managed = std::move(other.files_managed);
    inited = other.inited;
    writing = other.writing;

    other.index_info = index_file_information();
    other.index_file = "";
    other.writer = NULL;
    other.files_managed.clear();
    other.inited = false;
    other.writing = false;
    return *this;
  }

  ~sarray() {
    // the RAII deleter in files_managed will take care of the deletion
  }

  /**
   * Attempts to construct an sarray which reads from an sfrom the given file index file.
   * If the index cannot be opened, an exception is thrown.
   */
  explicit sarray(std::string sidx_or_directory) {
    open_for_read(sidx_or_directory);
  }

  /**
   * Create an sarray of given value and size.
   */
  sarray(const flexible_type& value, size_t size,
         size_t num_segments = SFRAME_DEFAULT_NUM_SEGMENTS,
         flex_type_enum type = flex_type_enum::UNDEFINED) {
    if (type == flex_type_enum::UNDEFINED) {
      type = value.get_type();
    }
    ASSERT_GT(num_segments, (size_t)0);
    open_for_write(num_segments);
    set_type(type);
    size_t size_per_segment = size / num_segments;
    parallel_for(0, num_segments, [&](size_t i) {
        auto out = get_output_iterator(i);
        size_t begin = i * size_per_segment;
        size_t end = (i + 1) * size_per_segment;
        if (i == num_segments -1) end = size;
        for (size_t iter = begin; iter < end; ++iter) {
          *out = value;
          ++out;
        }
    });
    close();
  }

  /**
   * Initializes the SArray with an index info.
   * If the SArray is already inited, this will throw an exception
   */
  void open_for_read(index_file_information info) {
    ASSERT_MSG(!inited, "Attempting to init an SArray "
               "which has already been inited");
    index_info = info;
    keep_array_file_ref();

    inited = true;
    writing = false;
    if (index_info.version == 0) {
      logprogress_stream << "Version 0 file format has been deprecated. "
                         << "Operations may not work as expected, or will be slow."
                         << "Please re-save the SFrame/SArray to update it to "
                         << "the latest version which has substantial "
                         << "performance optimizations\n";
    }
  }

  /**
   * Initializes the SArray with an index file.
   * If the SArray is already inited, this will throw an exception
   */
  void open_for_read(std::string sidx_file) {
    ASSERT_MSG(!inited, "Attempting to init an SArray "
               "which has already been inited");
    index_file = sidx_file;

    index_info = read_index_file(index_file);
    keep_array_file_ref();

    inited = true;
    writing = false;
    if (index_info.version == 0) {
      logprogress_stream << "Version 0 file format has been deprecated. "
                         << "Operations may not work as expected, or will be slow."
                         << "Please re-save the SFrame/SArray to update it to "
                         << "the latest version which has substantial "
                         << "performance optimizations\n";

    }
  }


  /**
   * Opens the Array for writing with an arbitrary temporary file.
   * The array must not already been inited.
   *
   * \param num_segments The number of segments in the array
   */
  void open_for_write(size_t num_segments = SFRAME_DEFAULT_NUM_SEGMENTS) {
    ASSERT_MSG(!inited, "Attempting to init an SArray "
               "which has already been inited");
    std::string sidx_file = fileio::fixed_size_cache_manager::get_instance().get_temp_cache_id(".sidx");
    index_file = sidx_file;
    writer = new sarray_group_format_writer_v2<T>;
    ASSERT_TRUE(writer != NULL);
    if (writer) writer->open(sidx_file, num_segments, 1);
    inited = true;
    writing = true;
    index_info = writer->get_index_info().columns[0];
  }


  /**
   * Opens the Array for writing with a location on disk.
   * The array must not already been inited.
   *
   * \param sidx_file If not specified, an argitrary temporary
   *                  file will be created. Otherwise, all frame
   *                  files will be written to the same location
   *                  as the frame_sidx_file. Must end in
   *                  ".sidx"
   * \param num_segments The number of segments in the array
   */
  void open_for_write(std::string sidx_file,
                      size_t num_segments = SFRAME_DEFAULT_NUM_SEGMENTS) {
    ASSERT_MSG(!inited, "Attempting to init an SArray "
        "which has already been inited");
    index_file = sidx_file;
    writer = new sarray_group_format_writer_v2<T>;
    // writer->open will test if the sidx file ends with .sidx
    if (writer) writer->open(sidx_file, num_segments, 1);
    inited = true;
    writing = true;
    index_info = writer->get_index_info().columns[0];
  }

  /**
   * Returns true if the Array is opened for reading.
   * i.e. get_reader() will succeed
   */
  bool is_opened_for_read() const {
    return (inited && !writing);
  }


  /**
   * Returns true if the Array is opened for writing.
   * i.e. get_output_iterator() will succeed
   */
  bool is_opened_for_write() const {
    return (inited && writing);
  }

  /**
   * Return the location of the index file of the sarray
   */
  std::string get_index_file() const {
    ASSERT_TRUE(inited);
    return index_file;
  }

  /**
   * Return the underlying writer of the sarray
   */
  sarray_group_format_writer<T>* get_writer() {
    ASSERT_MSG(inited, "Invalid SArray");
    ASSERT_MSG(writing, "SArray not opened for writing");
    ASSERT_NE(writer, NULL);
    return writer;
  }

  /**
   * Reads the value of a key associated with the sarray.
   * Returns true on success, false on failure.
   */
  bool get_metadata(std::string key, std::string &val) const {
    bool ret;
    std::tie(ret, val) = get_metadata(key);
    return ret;
  }

  /**
   * Reads the value of a key associated with the sarray.
   * Returns a pair of (true, value) on success, and (false, empty_string)
   * on failure.
   */
  std::pair<bool, std::string> get_metadata(std::string key) const {
    ASSERT_MSG(inited, "Invalid SArray");
    if (index_info.metadata.count(key)) {
      return std::pair<bool, std::string>(true, index_info.metadata.at(key));
    } else {
      return std::pair<bool, std::string>(false, "");
    }
  }

  /**
   * Returns the number of elements in the SArray
   */
  size_t size() const {
    if(!inited)
      return 0;
    size_t ret = 0;
    for (size_t i = 0;i < index_info.segment_sizes.size(); ++i) {
      ret += index_info.segment_sizes[i];
    }
    return ret;
  }

  /**
   * Gets an sarray reader object using the segmentation produced by the
   * actual file segments on disk.
   */
  std::unique_ptr<reader_type> get_reader() const {
    ASSERT_MSG(inited, "Invalid SArray");
    ASSERT_MSG(!writing, "Cannot open an SArraying which is still writing.");
    std::unique_ptr<reader_type> sarray_reader(new reader_type());
    sarray_reader->init(*this);
    return sarray_reader;
  }

  /**
   * Gets an sarray reader object with num_segments number of logical segments.
   */
  std::unique_ptr<reader_type> get_reader(size_t num_segments) const {
    ASSERT_MSG(inited, "Invalid SArray");
    ASSERT_MSG(!writing, "Cannot open an SArray which is still writing.");
    std::unique_ptr<reader_type> sarray_reader(new reader_type());
    sarray_reader->init(*this, num_segments);
    return sarray_reader;
  }

  /**
   * Gets an sarray reader object with a custom segment layout. segment_lengths
   * must sum up to the same length as the original array.
   */
  std::unique_ptr<reader_type> get_reader(const std::vector<size_t>& segment_lengths) const {
    ASSERT_MSG(inited, "Invalid SArray");
    ASSERT_MSG(!writing, "Cannot open an SArray which is still writing.");
    std::unique_ptr<reader_type> sarray_reader(new reader_type());
    sarray_reader->init(*this, segment_lengths);
    return sarray_reader;
  }


  /**
   * Return the number of segments in the array.
   */
  size_t num_segments() const {
    ASSERT_MSG(inited, "Invalid SArray");
    return index_info.nsegments;
  }


  /**
   * Return the length of segment i in the array.
   */
  size_t segment_length(size_t i) const {
    ASSERT_MSG(inited, "Invalid SArray");
    return index_info.segment_sizes[i];
  }

  /**
   * Returns all the index information of the array.
   */
  const index_file_information get_index_info() const {
    return index_info;
  }

  /**
   * Appends another SArray of the same type with the current SArray,
   * returning a new sarray.
   * without destroying the other array. Both SArrays can be empty, but
   * cannot be opened for writing.
   */
  sarray append(const sarray& other) const {
    // both cannot be writing
    ASSERT_EQ(writing, false);
    ASSERT_EQ(other.writing, false);
    // if one is inited, return the other
    if (!other.inited) return *this;
    if (!inited) return other;

    // cannot combine across format version
    ASSERT_EQ(index_info.version, other.index_info.version);
    ASSERT_EQ(index_info.block_size, other.index_info.block_size);

    sarray ret;
    ret.inited = true;
    ret.index_info = index_info;
    ret.files_managed = files_managed;

    ret.index_info.nsegments += other.index_info.nsegments;
    std::copy(other.index_info.segment_sizes.begin(), other.index_info.segment_sizes.end(),
              std::inserter(ret.index_info.segment_sizes, ret.index_info.segment_sizes.end()));
    std::copy(other.index_info.segment_files.begin(), other.index_info.segment_files.end(),
              std::inserter(ret.index_info.segment_files, ret.index_info.segment_files.end()));
    std::copy(other.files_managed.begin(), other.files_managed.end(),
              std::inserter(ret.files_managed, ret.files_managed.end()));
    return ret;
  }
  /**
   * Return a new sarray that contains a copy of the data in the current array.
   */
  sarray* clone() const {
    sarray* ret = new sarray();
    ret->open_for_write(num_segments());
    ret->set_type(get_type());
    auto reader = get_reader();
    parallel_for(0, num_segments(), [&](size_t segment_id) {
        auto iter = reader->begin(segment_id);
        auto end = reader->end(segment_id);
        auto out = ret->get_output_iterator(segment_id);
        while (iter != end) {
          *out = *iter;
          ++out;
          ++iter;
        }
    });
    ret->close();
    return ret;
  }

  /**
   * Sarray serializer. iarc must be associated with a directory.
   * Saves into a prefix inside the directory.
   */
  void save(oarchive& oarc) const {
    std::string prefix = oarc.get_prefix();
    save(prefix + ".sidx");
  }

  /**
   * SArray deserializer. iarc must be associated with a directory.
   * Loads from the next prefix inside the directory.
   */
  void load(iarchive& iarc) {
    std::string prefix = iarc.get_prefix();
    open_for_read(prefix + ".sidx");
  }

/**************************************************************************/
/*                                                                        */
/*                           Writing Functions                            */
/*                                                                        */
/**************************************************************************/
// These functions are only valid when the array is opened for writing

  /**
   * Sets the number of segments in the output.
   * Array must be first opened for writing.
   * If any writes has occured prior to this, those writes will be lost.
   * Returns true on sucess, false on failure.
   */
  bool set_num_segments(size_t numseg) {
    ASSERT_MSG(inited, "Invalid SArray");
    ASSERT_MSG(writing, "SArray not opened for writing");
    if (numseg == 0) return false;
    if (numseg != writer->num_segments()) {
      delete writer;
      writer = new sarray_group_format_writer_v2<T>;
      writer->open(index_file, numseg, 1);
      // restore metadata
      writer->get_index_info().columns[0].metadata = index_info.metadata;
      index_info = writer->get_index_info().columns[0];
      return true;
    }
    return false;
  }

  /**
   * Return an output iterator which can be used to write data to the segment.
   * Array must be first opened for writing.
   * The iterator (\ref iterator) is of the output iterator type and
   * has value_type T.
   *
   * The iterator is invalid once the segment is closed (See \ref close).
   * Accessing the iterator after the writer is destroyed is undefined
   * behavior.
   *
   * \code
   * // example to write a little array to segment 1
   * // say sw is of type sarray<int>
   * auto iter = sw.get_output_iterator(1);
   * std::vector<int> vals{1,2,3}
   * auto(int i, vals) {
   *   *iter = i;
   *   ++iter;
   * }
   * \endcode
   *
   * Will throw an exception if the array is invalid (there is an error
   * opening/writing files) Also segmentid must be a valid segment ID. Will
   * throw an exception otherwise.
   *
   * When T is a flexible_type, the output iterator performs type checking.
   */
  iterator get_output_iterator(size_t segmentid);

  /**
   * Closes the array.
   * Array must be first opened for writing.
   * close() also implicitly closes all segments.
   * After the writer is closed, no segments can be written.
   * Only once the array is closed, the SArray becomes readable with the
   * get_reader() function.
   */
  void close() {
    writer->close();
    writer->write_index_file();
    index_info = writer->get_index_info().columns[0];
    delete writer;
    writing = false;

    keep_array_file_ref();
  }

  /**
   * Adds meta data to the array.
   * Array must be first opened for writing.
   */
  bool set_metadata(std::string key, std::string val) {
    ASSERT_MSG(inited, "Invalid SArray");
    ASSERT_MSG(writing, "SArray not opened for writing");
    ASSERT_NE(writer, NULL);
    writer->get_index_info().columns[0].metadata[key] = val;
    index_info = writer->get_index_info().columns[0];
    return true;
  }

/**************************************************************************/
/*                                                                        */
/*                SArray<flexible_type> specific functions                */
/*                                                                        */
/**************************************************************************/

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
    ASSERT_MSG(inited, "Invalid SArray");
    if (!index_info.metadata.count("__type__")) {
      return flex_type_enum::UNDEFINED;
    }
    return flex_type_enum(std::stoi(index_info.metadata.at("__type__")));
  }


  /**
   * Sets the internal type of the flexible_type when written.
   * All writes will cast to this type.
   *
   * This function should only be used for sarray<flexible_type> and
   * will fail fatally otherwise.
   */
  void set_type(flex_type_enum type) {
    if (!std::is_same<T, flexible_type>::value) {
      ASSERT_MSG(false,
                  "Use of set_type() in SArray which "
                  "does not contain flexible_types");
    }
    ASSERT_MSG(inited, "Invalid SArray");
    ASSERT_MSG(writing, "SArray not opened for writing");
    set_metadata("__type__",
                 std::to_string(static_cast<int>(type)));
  }

  /**
   * Set the writer index_info for a given segment.
   * This function can be called, when the actual segment writing is done by other
   * logics.
   */
  void set_segment(size_t segmentid, const std::string& segment_file, size_t segment_size) {
    ASSERT_MSG(inited, "Invalid SArray");
    ASSERT_MSG(writing, "SArray not opened for writing");
    auto& index_info = writer->get_index_info().columns[0];
    index_info.segment_files[segmentid] = segment_file;
    index_info.segment_sizes[segmentid] = segment_size;
  }

  /**
   * Saves a copy of the current sarray into a different location.
   * Does not modify the current sarray.
   */
  void save(std::string index_file) const {
    ASSERT_TRUE(inited);
    ASSERT_FALSE(writing);
    std::string expected_ext(".sidx");
    if (!boost::algorithm::ends_with(index_file, expected_ext)) {
      log_and_throw("Index file must end with " + expected_ext);
    }
    sarray_save_blockwise(*this, index_file);
  }

  void delete_files_on_destruction() {
    for(auto &file : files_managed) {
      logstream(LOG_INFO) << "Will delete data file: " << file->m_file << std::endl;
      file->delete_on_destruction();
    }
  }

 private:
  // put all the files into the files_managed to manage their lifespan.
  // For non temp file, we also use a global handle pool to manage the lifespan
  // so that in normal case they are not going to be removed when the file is
  // in use
  void keep_array_file_ref() {
    std::vector<std::string> managed_files;
    for (const auto& file: index_info.segment_files) {
      managed_files.push_back(parse_v2_segment_filename(file).first);
    }
    if (!index_info.index_file.empty()) {
      managed_files.push_back(parse_v2_segment_filename(index_info.index_file).first);
    }
    if (!index_file.empty()) {
      managed_files.push_back(parse_v2_segment_filename(index_file).first);
    }
    for(auto& file : managed_files) {
      std::shared_ptr<fileio::file_ownership_handle> file_handle;
      file_handle = fileio::file_handle_pool::get_instance().register_file(file);

      files_managed.push_back(file_handle);
    }
  }

  // read when the sarray is opened for reading
  // synchronized against the writer class when used for writing
  index_file_information index_info;
  std::string index_file;
  sarray_group_format_writer<T>* writer = NULL;
  mutex lock;
  // this flag only matters on construction
  // it tells me whether I should insert the created segment files into the
  // "files_managed" RAII structure
  bool inited = false;
  bool writing = false;
  std::vector<std::shared_ptr<fileio::file_ownership_handle>> files_managed;

  friend class sarray_reader<T>;
};

/// \}
//
/*
 * When T is a regular type, the output iterator just writes directly
 */
template <typename T>
typename sarray<T>::iterator inline sarray<T>::get_output_iterator(size_t segmentid) {
  ASSERT_MSG(inited, "Invalid SArray");
  ASSERT_MSG(writing, "SArray not opened for writing");
  ASSERT_NE(writer, NULL);
  ASSERT_LT(segmentid, num_segments());
  // return an output iterator which when written to,
  // will write to the segment
  return iterator(
      [=](const T& val)->void {
        writer->write_segment(0, segmentid, val);
      },
      [=](T&& val)->void {
        writer->write_segment(0, segmentid, std::forward<T>(val));
      },
      [=](const sframe_rows&)->void {
        ASSERT_TRUE(false);
      } );
}


/*
 * When T is a flexible type, the output iterator performs type checking
 */
template <>
sarray<flexible_type>::iterator
inline sarray<flexible_type>::get_output_iterator(size_t segmentid) {
  ASSERT_NE(writer, NULL);
  ASSERT_LT(segmentid, num_segments());
  flex_type_enum stored_type = get_type();
  // return an output iterator which when written to,
  // will write to the segment
  return iterator(
      [=](const value_type& val)->void {
        if (val.get_type() == stored_type ||
            val.get_type() == flex_type_enum::UNDEFINED ||
            stored_type == flex_type_enum::UNDEFINED) {
              writer->write_segment(0, segmentid, val);
            } else if (flex_type_is_convertible(val.get_type(), stored_type)) {
              flexible_type res(stored_type);
              res.soft_assign(val);
              writer->write_segment(0, segmentid, res);
            } else {
              std::string message = "Cannot convert " + std::string(val) + " to " + flex_type_enum_to_name(stored_type);
              logstream(LOG_ERROR) <<  message << std::endl;
              throw(bad_cast(message));
            }
      },
      [=](value_type&& val)->void {
        if (val.get_type() == stored_type ||
            val.get_type() == flex_type_enum::UNDEFINED ||
            stored_type == flex_type_enum::UNDEFINED) {
              writer->write_segment(0, segmentid, std::forward<flexible_type>(val));
            } else if (flex_type_is_convertible(val.get_type(), stored_type)) {
              flexible_type res(stored_type);
              res.soft_assign(val);
              writer->write_segment(0, segmentid, std::move(res));
            } else {
              std::string message = "Cannot convert " + std::string(val) + " to " + flex_type_enum_to_name(stored_type);
              logstream(LOG_ERROR) <<  message << std::endl;
              throw(bad_cast(message));
            }
      },
      [=](const sframe_rows& sfr)->void {
        ASSERT_TRUE(sfr.num_columns() == 1);
        writer->write_segment(segmentid, sfr.type_check({stored_type}));
      });
}


} // namespace turi

#include <sframe/sarray_reader.hpp>

////////////////////////////////////////////////////////////////////////////////
// Implement serialization for
// std::shared_ptr<sarray<flexible_type> >

BEGIN_OUT_OF_PLACE_SAVE(arc, std::shared_ptr<turi::sarray<turi::flexible_type> >, m) {
  if(m == nullptr) {
    arc << false;
  } else {
    arc << true;
    arc << (*m);
  }
} END_OUT_OF_PLACE_SAVE()

BEGIN_OUT_OF_PLACE_LOAD(arc, std::shared_ptr<turi::sarray<turi::flexible_type> >, m) {
  bool is_not_nullptr;
  arc >> is_not_nullptr;
  if(is_not_nullptr) {
    m.reset(new turi::sarray<turi::flexible_type>);
    arc >> (*m);
  } else {
    m = std::shared_ptr<turi::sarray<turi::flexible_type> >(nullptr);
  }
} END_OUT_OF_PLACE_LOAD()


#endif

/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_LIB_SFRAME_HPP
#define TURI_UNITY_LIB_SFRAME_HPP
#include <iostream>
#include <algorithm>
#include <memory>
#include <vector>
#include <core/logging/logger.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_data/sarray.hpp>
#include <core/storage/sframe_data/dataframe.hpp>
#include <core/storage/sframe_data/sframe_index_file.hpp>
#include <core/storage/sframe_data/sframe_constants.hpp>
#include <core/storage/sframe_data/output_iterator.hpp>


namespace turi {
// forward declaration of th csv_line_tokenizer to avoid a
// circular dependency
struct csv_line_tokenizer;
class sframe_reader;
class csv_writer;

typedef turi::sframe_function_output_iterator<
    std::vector<flexible_type>,
    std::function<void(const std::vector<flexible_type>&)>,
    std::function<void(std::vector<flexible_type>&&)>,
    std::function<void(const sframe_rows&)> >
    sframe_output_iterator;


/**
 * \ingroup sframe_physical
 * \addtogroup sframe_main Main SFrame Objects
 * \{
 */

/**
 * The SFrame is an immutable object that represents a table with rows
 * and columns.  Each column is an \ref sarray<flexible_type>, which is a
 * sequence of an object T split into segments. The sframe writes an sarray
 * for each column of data it is given to disk, each with a prefix that extends
 * the prefix given to open. The SFrame is referenced on disk by a single
 * ".frame_idx" file which then has a list of file names, one file for each
 * column.
 *
 * The SFrame is \b write-once, \b read-many. The SFrame can be opened for
 * writing \b once, after which it is read-only.
 *
 * Since each column of the SFrame is an independent sarray, as an independent
 * shared_ptr<sarray<flexible_type> > object, columns can be added / removed
 * to form new sframes without problems. As such, certain operations
 * (such as the object returned by add_column) recan be "ephemeral" in that
 * there is no .frame_idx file on disk backing it. An "ephemeral" frame can be
 * identified by checking the result of get_index_file(). If this is empty,
 * it is an ephemeral frame.
 *
 * The interface for the SFrame pretty much matches that of the \ref sarray
 * as in the SArray's stored type is std::vector<flexible_type>. The SFrame
 * however, also provides a large number of other capabilities such as
 * csv parsing, construction from sarrays, etc.
 */
class sframe : public swriter_base<sframe_output_iterator> {
 public:

  /// The reader type
  typedef sframe_reader reader_type;

  /// The iterator type which \ref get_output_iterator returns
  typedef sframe_output_iterator iterator;

  /// The type contained in the sframe
  typedef std::vector<flexible_type> value_type;

  /**************************************************************************/
  /*                                                                        */
  /*                              Constructors                              */
  /*                                                                        */
  /**************************************************************************/
  /**
   * default constructor; does nothing; use \ref open_for_read or
   * \ref open_for_write after construction to read/create an sarray.
   */
  inline sframe() { }

  /**
   * Copy constructor.
   * If the source frame is opened for writing, this will throw
   * an exception. Otherwise, this will create a frame opened for reading,
   * which shares column arrays with the source frame.
   */
  sframe(const sframe& other);


  /**
   * Move constructor.
   */
  sframe(sframe&& other) : sframe() {
    (*this) = std::move(other);
  }


  /**
   * Assignment operator.
   * If the source frame is opened for writing, this will throw
   * an exception. Otherwise, this will create a frame opened for reading,
   * which shares column arrays with the source frame.
   */
  sframe& operator=(const sframe& other);


  /**
   * Move Assignment operator.
   * Moves other into this. Other will be cleared as if it is a newly
   * constructed sframe object.
   */
  sframe& operator=(sframe&& other);

  /**
   * Attempts to construct an sframe which reads from the given frame
   * index file. This should be a .frame_idx file.
   * If the index cannot be opened, an exception is thrown.
   */
  explicit inline sframe(std::string frame_idx_file) {
    auto frame_index_info = read_sframe_index_file(frame_idx_file);
    open_for_read(frame_index_info);
  }

  /**
   * Construct an sframe from sframe index information.
   */
  explicit inline sframe(sframe_index_file_information frame_index_info) {
    open_for_read(frame_index_info);
  };

  /**
   * Constructs an SFrame from a vector of Sarrays.
   *
   * \param columns List of sarrays to form as columns
   * \param column_names List of the name for each column, with the indices
   * corresponding with the list of columns. If the length of the column_names
   * vector does not match columns, the column gets a default name.
   * For example, if four columns are given and column_names = {id, num},
   * the columns will be named {"id, "num", "X3", "X4"}.  Entries that are
   * zero-length strings will also be given a default name.
   * \param fail_on_column_names If true, will throw an exception if any column
   * names are unique.  If false, will automatically adjust column names so
   * they are unique.
   *
   * Throws an exception if any column names are not unique (if
   * fail_on_column_names is true), or if the number of segments, segment
   * sizes, or total sizes of each sarray is not equal. The constructed SFrame
   * is ephemeral, and is not backed by a disk index.
   */
  explicit inline sframe(
      const std::vector<std::shared_ptr<sarray<flexible_type> > > &new_columns,
      const std::vector<std::string>& column_names = {},
      bool fail_on_column_names=true) {
    open_for_read(new_columns, column_names, fail_on_column_names);
  }

  /**
   * Constructs an SFrame from a csv file.
   *
   * All columns will be parsed into flex_string unless the column type is
   * specified in the column_type_hints.
   *
   * \param path The url to the csv file. The url can points to local
   * filesystem, hdfs, or s3.  \param tokenizer The tokenization rules to use
   * \param use_header If true, the first line will be parsed as column
   * headers. Otherwise, R-style column names, i.e. X1, X2, X3... will be used.
   * \param continue_on_failure If true, lines with parsing errors will be skipped.
   * \param column_type_hints A map from column name to the column type.
   * \param output_columns The subset of column names to output
   * \param row_limit If non-zero, the maximum number of rows to read
   * \param skip_rows If non-zero, the number of lines to skip at the start
   *                   of each file
   *
   * Throws an exception if IO error or csv parse failed.
   */
  std::map<std::string, std::shared_ptr<sarray<flexible_type>>> init_from_csvs(
      const std::string& path,
      csv_line_tokenizer& tokenizer,
      bool use_header,
      bool continue_on_failure,
      bool store_errors,
      std::map<std::string, flex_type_enum> column_type_hints,
      std::vector<std::string> output_columns = std::vector<std::string>(),
      size_t row_limit = 0,
      size_t skip_rows = 0);

  /**
   * Constructs an SFrame from dataframe_t.
   *
   * \note Throw an exception if the dataframe contains undefined values (e.g.
   * in sparse rows),
   */
  sframe(const dataframe_t& data);

  ~sframe();

  /**************************************************************************/
  /*                                                                        */
  /*                                Openers                                 */
  /*                                                                        */
  /**************************************************************************/
  /**
   * Initializes the SFrame with an index_information.
   * If the SFrame is already inited, this will throw an exception
   */
  inline void open_for_read(sframe_index_file_information frame_index_info) {
    Dlog_func_entry();
    ASSERT_MSG(!inited, "Attempting to init an SFrame "
        "which has already been inited.");
    inited = true;
    create_arrays_for_reading(frame_index_info);
  }

  /**
   * Initializes the SFrame with a collection of columns.  If the SFrame is
   * already inited, this will throw an exception. Will throw an exception if
   * column_names are not unique and fail_on_column_names is true.
   */
  void open_for_read(
      const std::vector<std::shared_ptr<sarray<flexible_type> > > &new_columns,
      const std::vector<std::string>& column_names = {},
      bool fail_on_column_names=true) {
    Dlog_func_entry();
    ASSERT_MSG(!inited, "Attempting to init an SFrame "
        "which has already been inited.");
    inited = true;
    create_arrays_for_reading(new_columns, column_names, fail_on_column_names);
  }

  /**
   * Opens the SFrame with an arbitrary temporary file.
   * The array must not already been inited.
   *
   * \param column_names The name for each column. If the vector is shorter
   * than column_types, or empty values are given, names are handled with
   * default names of "X<column id+1>".  Each column name must be unique.
   * This will let you write non-unique column names, but if you do that,
   * the sframe will throw an exception while constructing the output of
   * this class.
   * \param column_types The type of each column expressed as a
   * flexible_type.  Currently this is required to tell how many columns
   * are a part of the sframe. Throws an exception if this is an empty
   * vector.
   * \param nsegments The number of parallel output segments on each
   * sarray.  Throws an exception if this is 0.
   * \param frame_sidx_file If not specified, an argitrary temporary
   *                        file will be created. Otherwise, all frame
   *                        files will be written to the same location
   *                        as the frame_sidx_file. Must end in
   *                        ".frame_idx"
   * \param fail_on_column_names If true, will throw an exception if any column
   *                             names are unique.  If false, will
   *                             automatically adjust column names so they are
   *                             unique.
   */
  inline void open_for_write(const std::vector<std::string>& column_names,
                             const std::vector<flex_type_enum>& column_types,
                             const std::string& frame_sidx_file = "",
                             size_t nsegments = SFRAME_DEFAULT_NUM_SEGMENTS,
                             bool fail_on_column_names=true) {
    Dlog_func_entry();
    ASSERT_MSG(!inited, "Attempting to init an SFrame "
        "which has already been inited.");
    if (column_names.size() != column_types.size()) {
      log_and_throw(std::string("Names and Types array length mismatch"));
    }
    inited = true;
    create_arrays_for_writing(column_names, column_types,
                              nsegments, frame_sidx_file, fail_on_column_names);
  }

/**************************************************************************/
/*                                                                        */
/*                            Basic Accessors                             */
/*                                                                        */
/**************************************************************************/

  /**
   * Returns true if the Array is opened for reading.
   * i.e. get_reader() will succeed
   */
  inline bool is_opened_for_read() const {
    return (inited && !writing);
  }


  /**
   * Returns true if the Array is opened for writing.
   * i.e. get_output_iterator() will succeed
   */
  inline bool is_opened_for_write() const {
    return (inited && writing);
  }


  /**
   * Return the index file of the sframe
   */
  inline const std::string& get_index_file() const {
    ASSERT_TRUE(inited);
    return index_file;
  }


  /**
   * Reads the value of a key associated with the sframe
   * Returns true on success, false on failure.
   */
  inline bool get_metadata(const std::string& key, std::string &val) const {
    bool ret;
    std::tie(ret, val) = get_metadata(key);
    return ret;
  }


  /**
   * Reads the value of a key associated with the sframe
   * Returns a pair of (true, value) on success, and (false, empty_string)
   * on failure.
   */
  inline std::pair<bool, std::string> get_metadata(const std::string& key) const {
    ASSERT_MSG(inited, "Invalid SFrame");
    if (index_info.metadata.count(key)) {
      return std::pair<bool, std::string>(true, index_info.metadata.at(key));
    } else {
      return std::pair<bool, std::string>(false, "");
    }
  }


  /// Returns the number of columns in the SFrame. Does not throw.
  inline size_t num_columns() const {
    return index_info.ncolumns;
  }

  /// Returns the length of each sarray.
  inline size_t num_rows() const {
    return size();
  }


  /**
   * Returns the number of elements in the sframe. If the sframe was not initialized, returns 0.
   */
  inline size_t size() const {
    return inited ? index_info.nrows : 0;
  }

  /**
   * Returns the name of the given column.  Throws an exception if the
   * column id is out of range.
   */
  inline std::string column_name(size_t i) const {
    if(i >= index_info.column_names.size()) {
      log_and_throw("Column index out of range!");
    }

    return index_info.column_names[i];
  }

  /**
   * Returns the type of the given column.  Throws an exception if the
   * column id is out of range.
   */
  inline flex_type_enum column_type(size_t i) const {
    if (writing) {
      if(i >= group_writer->get_index_info().columns.size()) {
        log_and_throw("Column index out of range!");
      }
      return (flex_type_enum)
          (atoi(group_writer->get_index_info().columns[i].metadata["__type__"].c_str()));
    } else {
      if(i >= columns.size()) {
        log_and_throw("Column index out of range!");
      }
      return columns[i]->get_type();
    }
  }

  /**
   * Returns the type of the given column.  Throws an exception if the
   * column id is out of range.
   * \overload
   */
  inline flex_type_enum column_type(const std::string& column_name) const {
    return columns[column_index(column_name)]->get_type();
  }


  /** Returns the column names as a single vector.
   */
  inline const std::vector<std::string>& column_names() const {
    return index_info.column_names;
  }

  /** Returns the column types as a single vector.
   */
  inline std::vector<flex_type_enum> column_types() const {
    std::vector<flex_type_enum> tv(num_columns());
    for(size_t i = 0; i < num_columns(); ++i)
      tv[i] = column_type(i);

    return tv;
  }

  /**
   * Returns true if the sframe contains the given column.
   */
  inline bool contains_column(const std::string& column_name) const {
    Dlog_func_entry();
    auto iter = std::find(index_info.column_names.begin(),
                          index_info.column_names.end(),
                          column_name);
    return iter != index_info.column_names.end();
  }

  /**
   * Returns the number of segments that this SFrame will be
   * written with.  Never fails.
   */
  inline size_t num_segments() const {
    ASSERT_MSG(inited, "Invalid SFrame");
    if (writing) {
      return group_writer->num_segments();
    } else {
      if (index_info.ncolumns == 0) return 0;
      return columns[0]->num_segments();
    }
  }

  /**
   * Return the number of segments in the collection.
   * Will throw an exception if the writer is invalid (there is an error
   * opening/writing files)
   */
  inline size_t segment_length(size_t i) const {
    DASSERT_MSG(inited, "Invalid SFrame");
    if (index_info.ncolumns == 0) return 0;
    else return columns[0]->segment_length(i);
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
      log_and_throw(std::string("Column name " + column_name + " does not exist."));
    }
    __builtin_unreachable();
  }

  /**
   * Returns the current index info of the array.
   */
  inline const sframe_index_file_information get_index_info() const {
    return index_info;
  }

  /**
   * Merges another SFrame with the same schema with the current SFrame
   * returning a new SFrame.
   * Both SFrames can be empty, but cannot be opened for writing.
   */
  sframe append(const sframe& other) const;


  /**
   * Gets an sframe reader object with the segment layout of the first column.
   */
  std::unique_ptr<reader_type> get_reader() const;


  /**
   * Gets an sframe reader object with num_segments number of logical segments.
   */
  std::unique_ptr<reader_type> get_reader(size_t num_segments) const;


  /**
   * Gets an sframe reader object with a custom segment layout. segment_lengths
   * must sum up to the same length as the original array.
   */
  std::unique_ptr<reader_type> get_reader(const std::vector<size_t>& segment_lengths) const;

/**************************************************************************/
/*                                                                        */
/*                     Other SFrame Unique Accessors                      */
/*                                                                        */
/**************************************************************************/

  /**
   * Converts the sframe into a dataframe_t. Will reset iterators before
   * and after the operation.
   */
  dataframe_t to_dataframe();

  /**
   * Returns an sarray of the specific column.
   *
   * Throws an exception if the column does not exist.
   */
  std::shared_ptr<sarray<flexible_type> > select_column(size_t column_id) const;

  /**
   * Returns an sarray of the specific column by name.
   *
   * Throws an exception if the column does not exist.
   */
  std::shared_ptr<sarray<flexible_type> > select_column(const std::string &name) const;

  /**
   * Returns new sframe containing only the chosen columns in the same order.
   * The new sframe is "ephemeral" in that it is not backed by an index
   * on disk.
   *
   * Throws an exception if the column name does not exist.
   */
  sframe select_columns(const std::vector<std::string>& names) const;

  /**
   * Returns a new ephemeral SFrame with the new column added to the end.
   * The new sframe is "ephemeral" in that it is not backed by an index
   * on disk.
   *
   * \param sarr_ptr Shared pointer to the SArray
   * \param column_name The name to give this column.  If empty it will
   * be given a default name (X<column index>)
   *
   */
  sframe add_column(std::shared_ptr<sarray<flexible_type> > sarr_ptr,
                    const std::string& column_name=std::string("")) const;



  /**
   * Set the ith column name to name. This can be done when the
   * frame is open in either reading or writing mode. Changes are ephemeral,
   * and do not affect what is stored on disk.
   */
  void set_column_name(size_t column_id, const std::string& name);

  /**
   * Returns a new ephemeral SFrame with the column removed.
   * The new sframe is "ephemeral" in that it is not backed by an index
   * on disk.
   *
   * \param column_id The index of the column to remove.
   *
   */
  sframe remove_column(size_t column_id) const;


  /**
   * Returns a new ephemeral SFrame with two columns swapped.
   * The new sframe is "ephemeral" in that it is not backed by an index
   * on disk.
   *
   * \param column_1 The index of the first column.
   * \param column_2 The index of the second column.
   *
   */
  sframe swap_columns(size_t column_1, size_t column_2) const;

  /**
   * Replace the column of the given column name with a new sarray.
   * Return the new sframe with old column_name sarray replaced by the new sarray.
   */
  sframe replace_column(std::shared_ptr<sarray<flexible_type>> sarr_ptr,
                        const std::string& column_name) const;

/**************************************************************************/
  /*                                                                        */
/*                           Writing Functions                            */
/*                                                                        */
/**************************************************************************/
// These functions are only valid when the array is opened for writing

  /**
   * Sets the number of segments in the output.
   * Frame must be first opened for writing.
   * Once an output iterator has been obtained, the number of segments
   * can no longer be changed. Returns true on sucess, false on failure.
   */
  bool set_num_segments(size_t numseg);

  /**
   * Gets an output iterator for the given segment.  This can be used to
   * write data to the segment, and is currently the only supported way
   * to do so.
   *
   * The iterator is invalid once the segment is closed (See \ref close).
   * Accessing the iterator after the writer is destroyed is undefined
   * behavior.
   *
   * Cannot be called until the sframe is open.
   *
   * Example:
   * \code
   * // example to write the same vector to 7 rows of segment 1
   * // let's say the sframe has 5 columns of type FLEX_TYPE_ENUM::INTEGER
   * // and sfw is the sframe.
   * auto iter = sfw.get_output_iterator(1);
   * std::vector<flexible_type> vals{1,2,3,4,5}
   * for(int i = 0; i < 7; ++i) {
   *   *iter = vals;
   *   ++iter;
   * }
   * \endcode
   */
  iterator get_output_iterator(size_t segmentid);

  /**
   * Closes the sframe. close() also implicitly closes all segments.  After
   * the writer is closed, no segments can be written.
   * After the sframe is closed, it becomes read only and can be read
   * with the get_reader() function.
   */
  void close();

  /**
   * Flush writes for a particular segment
   */
  void flush_write_to_segment(size_t segment);

  /**
   * Saves a copy of the current sframe as a CSV file.
   * Does not modify the current sframe.
   *
   * \param csv_file target CSV file to save into
   * \param writer The CSV writer configuration
   */
  void save_as_csv(std::string csv_file,
                   csv_writer& writer);

  /**
   * Adds meta data to the frame.
   * Frame must be first opened for writing.
   */
  bool set_metadata(const std::string& key, std::string val);

  /**
   * Saves a copy of the current sframe into a different location.
   * Does not modify the current sframe.
   */
  void save(std::string index_file) const;

  /**
   * SFrame serializer. oarc must be associated with a directory.
   * Saves into a prefix inside the directory.
   */
  void save(oarchive& oarc) const;


  /**
   * Attempts to compact if the number of segments in the SArray
   * exceeds SFRAME_COMPACTION_THRESHOLD.
   */
  void try_compact();

  /**
   * SFrame deserializer. iarc must be associated with a directory.
   * Loads from the next prefix inside the directory.
   */
  void load(iarchive& iarc);

  bool delete_files_on_destruction();

  /**
   * Internal API.
   * Used to obtain the internal writer object.
   */
  inline
  std::shared_ptr<sarray_group_format_writer<flexible_type> >
  get_internal_writer() {
    return group_writer;
  }
 private:

  /**
   * Clears all internal structures. Used by \ref create_arrays_for_reading
   * and \ref create_arrays_for_writing to clear all the index information
   * and column information
   */
  void reset();

  /**
   * Internal function that actually writes the values to each SArray's
   * output iterator.  Used by the sframe_output_iterator.
   */
  void write(size_t segmentid, const std::vector<flexible_type>& t);


  /**
   * Internal function that actually writes the values to each SArray's
   * output iterator.  Used by the sframe_output_iterator.
   */
  void write(size_t segmentid, std::vector<flexible_type>&& t);


  /**
   * Internal function that actually writes the values to each SArray's
   * output iterator.  Used by the sframe_output_iterator.
   */
  void write(size_t segmentid, const sframe_rows& t);

  /**
   * Internal function. Given the index_information, this function
   * initializes each of the sarrays for reading; filling up
   * the columns array
   */
 void create_arrays_for_reading(sframe_index_file_information frame_index_info);

  /**
   * Internal function. Given a collection of sarray columns, this function
   * makes an sframe representing the combination of all the columns.  This
   * sframe does not have an index file (it is ephemeral), and get_index_file
   * will return an empty file. Will throw an exception if column_names are not
   * unique and fail_on_column_names is true.
   */
  void create_arrays_for_reading(
      const std::vector<std::shared_ptr<sarray<flexible_type> > > &columns,
      const std::vector<std::string>& column_names = {},
      bool fail_on_column_names=true);
  /**
   * Internal function. Given the index_file, this function initializes each of
   * the sarrays for writing; filling up the columns array.  Will throw an
   * exception if column_names are not unique and fail_on_column_names is true.
   */
  void create_arrays_for_writing(const std::vector<std::string>& column_names,
                                 const std::vector<flex_type_enum>& column_types,
                                 size_t nsegments,
                                 const std::string& frame_sidx_file,
                                 bool fail_on_column_names);

  void keep_array_file_ref();
  /**
   * Internal function. Resolve conflicts in column names.
   */
  std::string generate_valid_column_name(const std::string &column_name) const;

  sframe_index_file_information index_info;
  std::string index_file;
  std::vector<std::shared_ptr<fileio::file_ownership_handle> > index_file_handle;

  std::vector<std::shared_ptr<sarray<flexible_type> > > columns;
  std::shared_ptr<sarray_group_format_writer<flexible_type> > group_writer;

  mutex lock;

  bool inited = false;
  bool writing = false;
  friend class sframe_reader;

public:
  /**
   * For debug purpose, print the information about the sframe.
   */
  void debug_print();
};

/// \}
} // end of namespace
#endif


#include <core/storage/sframe_data/sframe_reader.hpp>

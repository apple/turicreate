/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_DATAFRAME_HPP
#define TURI_UNITY_DATAFRAME_HPP
#include <vector>
#include <iterator>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/serialization/serialization_includes.hpp>

namespace turi {

class flexible_type_registry;
typedef uint32_t field_id_type;


/**
 * \ingroup sframe_physical
 * \addtogroup csv_utils CSV Parsing and Writing
 * \{
 */

/**
 * Type that represents a Pandas-like dataframe:
 * A in memory column-wise representation of a table.
 * The dataframe_t is simply a map from column name to a column of records,
 * where every column is the same length, and all values within a column
 * have the same type. This is also the type used for transferring between
 * pandas dataframe objects and C++.
 *
 * Each cell in the dataframe is represented by a \ref flexible_type object,
 * while this technically allows every cell to be an arbitrary type, we do not
 * permit that behavior. We require and assume that every cell in a column be of
 * the same type. This is with the exception of empty cells (NaNs in Pandas)
 * which are of type UNDEFINED.
 */
struct dataframe_t {
  /// A vector storing the name of columns
  std::vector<std::string> names;

  /// A map from the column name to the type of the column
  std::map<std::string, flex_type_enum> types;

  /** A map from the column name to the values of the column.
   * Every column must have the same length, and all values within a column
   * must be of the same type. The UNDEFINED type is an exception to the rule
   * and may be used anywhere to designate an empty entry.
   */
  std::map<std::string, std::vector<flexible_type> > values;

  /**
   * Fill the dataframe with the content from a csv file.
   *
   * \param: path. Path to the csv file.
   * \param: delimiter. User defined csv sepearator.
   * \param: use_header. If true, the first line is treated as column header.
   *         Otherwise, X0, X1,... will be used.
   *
   * Note: This function will infer and unify the most proper type for each
   * column.
   */
  void read_csv(const std::string& path, char delimiter, bool use_header);

  /**
   * Returns the number of rows in the dataframe
   */
  inline size_t nrows() const {
    return (values.begin() == values.end()) ? 0
           : values.begin()->second.size();
  }

  /**
   * Returns true if the dataframe is empty.
   */
  inline bool empty() const {
    return ncols() == 0 || nrows() == 0;
  }

  /**
   * Convert the values in the column into the specified type.
   * Throws an exception if the column is not found, or the conversion
   * cannot be made.
   */
  void set_type(std::string key, flex_type_enum type);

  /// Returns the number of columns in the dataframe
  inline size_t ncols() const { return values.size(); }

  /**
   * Returns true if the dataframe contains a column with the given name.
   */
  bool contains(std::string key) const{
    return types.count(key);
  }

  /**
   * Returns true if the column contains undefined flexible_type value.
   */
  bool contains_nan(std::string key) const{
    if (!contains(key)) {
      log_and_throw(std::string("Column " + key + " does not exist."));
    }
    const std::vector<flexible_type>& col = values.at(key);
    bool ret = false;
    for (size_t i = 0; i < col.size(); ++i) {
        if (col[i].get_type() == flex_type_enum::UNDEFINED) {
          ret = true;
        }
    }
    return ret;
  }

  /**
   * Column index operator. Can be used to extract a column from the dataframe.
   * Returns a pair of (type, reference to column)
   */
  inline std::pair< flex_type_enum, std::vector<flexible_type>&>
      operator[](std::string key) {
      return {types.at(key), values.at(key)};
   }

  /**
   * Const column index operator. Can be used to extract a column from the
   * dataframe.  Returns a pair of (type, reference to column)
   */
  inline std::pair< flex_type_enum, const std::vector<flexible_type>&>
      operator[](std::string key) const {
      return {types.at(key), values.at(key)};
   }

  /**
   * Prints the contents of the dataframe to std::cerr
   */
  void print() const;

  /**
   * Sets the value of a column of the dataframe.
   */
  void set_column(std::string key,
                  const std::vector<flexible_type>& val,
                  flex_type_enum type);


  /**
   * Sets the value of a column of the dataframe, consuming the vector value
   */
  void set_column(std::string key,
                  std::vector<flexible_type>&& val,
                  flex_type_enum type);

  /**
   * Remove the column.
   */
  void remove_column(std::string key);

  /// Serializer
  inline void save(oarchive& oarc) const {
    oarc << names << types << values;
  }

  /// Deserializer
  inline void load(iarchive& iarc) {
    iarc >> names >> types >> values;
  }

  /// Clears the contents of the dataframe
  inline void clear() {
    names.clear();
    types.clear();
    values.clear();
  }
};

/**
 * \ingroup unity
 * The dataframe is a column-wise representation. This provides iteration over
 * the dataframe in a row-wise representation. Incrementing the iterator
 * advances the iterator element by element by across rows.
 *
 * Usage:
 * \code
 * // Performs a row-wise iteration over the entries of the dataframe
 * dataframe_row_iterator iter = dataframe_row_iterator::begin(df);
 * dataframe_row_iterator end = dataframe_row_iterator::end(df);
 * while (iter != end) {
 *   // do stuff with (*iter). It is a flexible_type
 *   ++iter;
 * }
 * \endcode
 *
 * \code
 * // Alternatively:
 * dataframe_row_iterator iter = dataframe_row_iterator::begin(df);
 * for (size_t row = 0; row < iter.row_size(); ++row) {
 *   for (size_t col = 0; col < iter.col_size(); ++col) {
 *     // do stuff with (*iter). It is a flexible_type
 *     // pointing to the cell in column 'col' and row 'row'
 *     ++iter;
 *   }
 * }
 * \endcode
 */
class dataframe_row_iterator {
 private:
  /// The names of each column of the dataframe
  std::vector<std::string> names;

  /// The types of each column of the dataframe
  std::vector<flex_type_enum> types;

  /// The list of iterators over each column
  std::vector<std::pair<std::vector<flexible_type>::const_iterator,
                        std::vector<flexible_type>::const_iterator> > iterators;
  /// Number of rows in the dataframe
  size_t num_rows;
  /// Number of columns in the dataframe
  size_t num_columns;
  ///The current column pointed to
  size_t current_column;
  /// The current row pointed to
  size_t current_row;
  /// The total number of entries: num_rows * num_column
  size_t num_el;
  /// The entry index pointed to.
  size_t idx;
 public:

  typedef flexible_type value_type;
  typedef int difference_type;
  typedef flexible_type* pointer;
  typedef flexible_type& reference;
  typedef std::forward_iterator_tag iterator_category;

  /// Creates a begin iterator to the dataframe
  static dataframe_row_iterator begin(const dataframe_t& dt);

  /// Creates an end iterator to the dataframe
  static dataframe_row_iterator end(const dataframe_t& dt);

  /**
   * Changes the column iterator ordering by swapping the indices
   * of columns 'a' and columns 'b'.
   * Should only be done on begin and end iterators. Iterators in the midst
   * of iterating are not safe to be swapped.
   */
  inline void swap_column_order(size_t a, size_t b) {
    log_func_entry();
    std::swap(iterators[a], iterators[b]);
    std::swap(names[a], names[b]);
    std::swap(types[a], types[b]);
  }

  /// pre-increments to the next entry of the dataframe row-wise
  inline dataframe_row_iterator& operator++() {
    ++iterators[current_column].first;
    ++current_column;
    if (current_column == num_columns) {
      current_column = 0;
      ++current_row;
    }
    ++idx;
    return *this;
  }

  /// post-increments to the next entry of the dataframe row-wise
  inline dataframe_row_iterator& operator++(int) {
    dataframe_row_iterator ret = (*this);
    ++ret;
    return *this;
  }

  /// Returns the index of the current row
  inline size_t row() const {
    return current_row;
  }

  /// Returns the index of the current column
  inline size_t column() const {
    return current_column;
  }

  /// Returns the number of columns
  inline size_t column_size() const {
    return num_columns;
  }

  /// Returns the number of rows
  inline size_t row_size() const {
    return num_rows;
  }

  /// Returns the name of the current column
  inline const std::string& column_name() const {
    return names[current_column];
  }

  /// Returns the name of an arbitrary column
  inline const std::string& column_name(size_t idx) const {
    return names[idx];
  }

  /// Returns the list of all column names
  inline const std::vector<std::string>& column_names() const {
    return names;
  }

  /// Returns the type of the current column
  inline flex_type_enum column_type() const {
    return types[current_column];
  }

  /// Returns the type of an arbitrary column
  inline flex_type_enum column_type(size_t idx) const {
    return types[idx];
  }

  /// Returns the list of all column types
  inline const std::vector<flex_type_enum>& column_types() const {
    return types;
  }

  /**
   * Advances the iterator by this number of rows.
   * Current column does not change. If the number of rows to skip causes
   * the iterator to go past the end of the dataframe, the resultant
   * iterator is equivalent to the end iterator of the dataframe.
   */
  void skip_rows(size_t num_rows_to_skip);

  /// Returns true if both iterators are equal
  inline bool operator==(const dataframe_row_iterator& other) {
    return num_el == other.num_el && idx < other.idx;
  }

  /// Returns true if both iterators are not equal
  inline bool operator!=(const dataframe_row_iterator& other) {
    return !((*this) == other);
  }

  /// Dereferences the iterator, returning a reference to the underlying flexible_type
  inline const flexible_type& operator*() {
    return *(iterators[current_column].first);
  }

  /// Dereferences the iterator, returning a reference to the underlying flexible_type
  inline const flexible_type& operator*() const {
    return *(iterators[current_column].first);
  }


  /// Dereferences the iterator, returning a reference to the underlying flexible_type
  inline const flexible_type* operator->() {
    return &(*(iterators[current_column].first));
  }

  /// Dereferences the iterator, returning a reference to the underlying flexible_type
  inline const flexible_type* operator->() const {
    return &(*(iterators[current_column].first));
  }
};



/**
 * Cuts up the provided begin iterator to a dataframe into rows, calling the
 * lambda with a new iterator and the range of rows it is meant to process.
 */
void parallel_dataframe_iterate(const dataframe_t& df,
                                std::function<void(dataframe_row_iterator& iter,
                                                   size_t startrow,
                                                   size_t endrow)> partialrowfn);
/// \}
} // namespace turi

#endif

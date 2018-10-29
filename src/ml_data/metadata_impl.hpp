/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DML_METADATA_IMPL_H_
#define TURI_DML_METADATA_IMPL_H_

namespace turi {

////////////////////////////////////////////////////////////////////////////////
// Implementations of the above

  /** Returns a pointer to the internal column metadata of column
   *  column_index.  Useful for dealing with the column_index
   */
inline ml_data_internal::column_metadata_ptr
  ml_metadata::get_column_metadata(size_t column_index) const {

  DASSERT_LT(column_index, num_columns());
  return columns[column_index];
}

/** Returns the index of the column matching column_name, or throws
 *  an error if it does not exist.
 */
inline size_t ml_metadata::column_index(const std::string& _column_name, bool max_on_error) const {
  auto it = _column_name_to_index_map.find(_column_name);

  bool is_present = (it != _column_name_to_index_map.end());
  
  if(max_on_error) {
    return LIKELY(is_present) ? it->second : size_t(-1);
  } else {
    if(UNLIKELY(!is_present)) {
      log_and_throw((std::string("Column ") + _column_name + " not found in model metadata.").c_str());
    }
    DASSERT_TRUE(column_name(it->second) == _column_name);
    return it->second; 
  }
}

/**
 * Returns true if the metadata contains the given column.
 *
 *  \param column_name The name of the column.
 */
inline bool ml_metadata::contains_column(const std::string& column_name) const {
  return (_column_name_to_index_map.find(column_name) != _column_name_to_index_map.end());
}


inline bool ml_metadata::has_target() const {
  return (target != nullptr);
}

////////////////////////////////////////////////////////////////////////////////
// The indexers

inline const std::shared_ptr<ml_data_internal::column_indexer>&
ml_metadata::indexer(size_t column_index) const {
  return get_column_metadata(column_index)->indexer;
}

inline const std::shared_ptr<ml_data_internal::column_indexer>&
ml_metadata::indexer(const std::string& column_name) const {
  return indexer(column_index(column_name));
}


////////////////////////////////////////////////////////////////////////////////
// Statistics

inline const std::shared_ptr<ml_data_internal::column_statistics>&
ml_metadata::statistics(size_t column_index) const {
  return get_column_metadata(column_index)->statistics;
}

inline const std::shared_ptr<ml_data_internal::column_statistics>&
ml_metadata::statistics(const std::string& column_name) const {
  return statistics(column_index(column_name));
}


inline size_t ml_metadata::num_columns(bool include_untranslated_columns) const {
  size_t nc = columns.size();

  if(!include_untranslated_columns)
    nc -= num_untranslated_columns();

  return nc;
}

inline size_t ml_metadata::num_untranslated_columns() const {
  DASSERT_NE(_num_untranslated_columns, size_t(-1));
  return _num_untranslated_columns;
}

/** Returns true if there are translated columns present, and false
 *  otherwise.
 */
inline bool ml_metadata::has_translated_columns() const {
  return num_untranslated_columns() != columns.size();
}

/** Returns true if there are untranslated columns present, and false
 *  otherwise.
 */
inline bool ml_metadata::has_untranslated_columns() const {
  return num_untranslated_columns() != 0;
}

/** Returns the number of distinct dimensions, including all
 *  categorical features.
 */
inline size_t ml_metadata::num_dimensions() const {
  DASSERT_NE(_num_dimensions, size_t(-1));
  return _num_dimensions;
}

/** Returns the size of the columns in the metadata that were
 * present at train time.
 *
 */
inline const std::string& ml_metadata::column_name(size_t column_index) const {
  return get_column_metadata(column_index)->name;
}

/** Returns the size of the columns in the metadata that were
 * present at train time.
 *
 */
inline const std::string& ml_metadata::target_column_name() const {
  DASSERT_TRUE(has_target());
  return target->name;
}

inline const std::shared_ptr<ml_data_internal::column_indexer>&
ml_metadata::target_indexer() const {
  DASSERT_TRUE(has_target());
  return target->indexer;
}

inline const std::shared_ptr<ml_data_internal::column_statistics>&
ml_metadata::target_statistics() const {
  DASSERT_TRUE(has_target());
  return target->statistics;
}

/** Returns the current index size of the columns in the metadata.
 */
inline size_t ml_metadata::column_size(size_t column_index) const {
  return get_column_metadata(column_index)->column_size();
}

/** Returns the current nd column shape of the columns
 */
inline const flex_nd_vec::index_range_type& ml_metadata::nd_column_shape(size_t column_index) const {
  return get_column_metadata(column_index)->nd_column_shape();
}

/** Returns the current nd column shape of the columns
 */
inline const flex_nd_vec::index_range_type& ml_metadata::nd_column_shape(const std::string& column_name) const {
  return nd_column_shape(column_index(column_name));
}


/** Returns the current index size of the columns in the metadata.
 */
inline size_t ml_metadata::target_column_size() const {
  DASSERT_TRUE(has_target());
  return target->column_size();
}

/** Returns the index size of the columns in the metadata that were
 *  present at train time.  Index size differs from column size in
 *  that column_size may grow on test, but index_size is constant.
 */
inline size_t ml_metadata::index_size(size_t column_index) const {
  return get_column_metadata(column_index)->index_size();
}

/** Returns the index size of the column in the metadata that were
 *  present at train time.  Index size differs from column size in
 *  that column_size may grow on test, but index_size is constant.
 *
 *  \overload
 *
 *  \param column_name The name of the column.
 */
inline size_t ml_metadata::index_size(const std::string& column_name) const {
  return index_size(column_index(column_name));
}

/** Returns the index size of the columns in the metadata that were
 *  present at train time.  Index size differs from column size in
 *  that column_size may grow on test, but index_size is constant.
 */
inline size_t ml_metadata::target_index_size() const {
  return target->index_size();
}


/** Returns the global index offset of the columns in the metadata
 *  that were present at train time.  This is fixed at setup time;
 *  global indices for the column c_idx are in the interval
 *  [global_index_offset(c_idx), global_index_offset(c_idx) + index_size(c_idx) - 1]
 */
inline size_t ml_metadata::global_index_offset(size_t column_index) const {
  return get_column_metadata(column_index)->global_index_offset();
}

/** Returns the global index offset of the columns in the metadata
 *  that were present at train time.  This is fixed at setup time;
 *  global indices for the column c_idx are in the interval
 *  [global_index_offset(c_idx), global_index_offset(c_idx) + index_size(c_idx) - 1]
 *
 *  \overload
 *
 *  \param column_name The name of the column.
 */
inline size_t ml_metadata::global_index_offset(const std::string& column_name) const {
  return global_index_offset(column_index(column_name));
}

/**  Returns the mode of the column.  See ml_data_column_modes.hpp
 *  for details on the column modes.
 *
 *  \param column_index The index of the column.
 */
inline ml_column_mode ml_metadata::column_mode(size_t column_index) const {
  return get_column_metadata(column_index)->mode;
}

/**  Returns the mode of the column.  See ml_data_column_modes.hpp
 *  for details on the column modes.
 *
 *  \overload
 *
 *  \param column_name The name of the column.
 */
inline ml_column_mode ml_metadata::column_mode(const std::string& column_name) const {
  return column_mode(column_index(column_name));
}

/**  Returns the mode of the target column.  See
 *  ml_data_column_modes.hpp for details on the column modes.
 */
inline ml_column_mode ml_metadata::target_column_mode() const {
  DASSERT_TRUE(has_target());
  return target->mode;
}

/** Returns the size of the columns in the metadata that were
 *  present at train time.
 *
 *  \param column_index The index of the column.
 */
inline flex_type_enum ml_metadata::column_type(size_t column_index) const {
  return get_column_metadata(column_index)->original_column_type;
}

/** Returns the size of the columns in the metadata that were
 *  present at train time.
 *
 *  \overload
 *
 *  \param column_name The name of the column.
 */
inline flex_type_enum ml_metadata::column_type(const std::string& column_name) const {
  return column_type(column_index(column_name));
}


/**  Returns the type of the target column.
 */
inline flex_type_enum ml_metadata::target_column_type() const {
  DASSERT_TRUE(has_target());
  return target->original_column_type;
}

/** Returns true if the underlying type is treated as a categorical
 *  variable, and false otherwise.
 *
 *  \param column_index The index of the column.
 */
inline bool ml_metadata::is_categorical(size_t column_index) const {
  ml_column_mode mode = get_column_metadata(column_index)->mode;
  return mode_is_categorical(mode);
}

/** Returns true if the underlying type is treated as a categorical
 *  variable, and false otherwise.
 *
 *  \overload
 *
 *  \param column_name The name of the column.
 */
inline bool ml_metadata::is_categorical(const std::string& column_name) const {
  return is_categorical(column_index(column_name));
}

/** Returns true if the underlying column type is indexed, and false
 *  otherwise.  This differs form the is_categorical in that
 *  dictionaries are not treated as pure categorical variables, as
 *  they have values associated with them, but they are indexed.
 *
 *  \param column_index The index of the column.
 */
inline bool ml_metadata::is_indexed(size_t column_index) const {
  ml_column_mode mode = get_column_metadata(column_index)->mode;
  return mode_is_indexed(mode);
}

/** Returns true if the underlying column type is indexed, and false
 *  otherwise.  This differs form the is_categorical in that
 *  dictionaries are not treated as pure categorical variables, as
 *  they have values associated with them, but they are indexed.
 *
 *  \overload
 *
 *  \param column_name The name of the column.
 */
inline bool ml_metadata::is_indexed(const std::string& column_name) const {
  return is_indexed(column_index(column_name));
}

/** Returns true if the underlying column type is untranslated.
 *  This means it will only be available as flexible_type later on.
 *
 *  \param column_index The index of the column.
 */
inline bool ml_metadata::is_untranslated_column(size_t column_index) const {
  return get_column_metadata(column_index)->is_untranslated_column();
}

/** Returns true if the underlying column type is untranslated.
 *  This means it will only be available as flexible_type later on.
 *
 *  \overload
 *
 *  \param column_name The name of the column.
 */
inline bool ml_metadata::is_untranslated_column(const std::string& column_name) const {
  return is_untranslated_column(column_index(column_name));
}


/** Returns true if the underlying type is treated as a categorical
 *  variable, and false otherwise.
 */
inline bool ml_metadata::target_is_categorical() const {
  DASSERT_TRUE(has_target());
  return mode_is_categorical(target->mode);
}

/** Returns true if the underlying type is indexed, and false
 *  otherwise.  This differs form the is_categorical in that
 *  dictionaries are not treated as pure categorical variables, as
 *  they have values associated with them, but they are indexed.
 */
inline bool ml_metadata::target_is_indexed() const {
  DASSERT_TRUE(has_target());
  return mode_is_indexed(target->mode);
}

}

#endif

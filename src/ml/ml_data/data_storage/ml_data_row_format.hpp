/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DML_DATA_ROW_FORMAT_H_
#define TURI_DML_DATA_ROW_FORMAT_H_

#include <ml/ml_data/ml_data_entry.hpp>
#include <ml/ml_data/ml_data_column_modes.hpp>
#include <ml/ml_data/data_storage/internal_metadata.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/util/code_optimization.hpp>
#include <core/util/dense_bitset.hpp>


/**
 * ml_data layout format
 * ================================================================================
 *
 * The rows are stored in segments of a vector of entry_value structs,
 * where entry_value encloses a union of an index_value and a
 * double_value.  Thus it is 8 bytes.  Each vector contains up to
 * row_block_size rows.  The data is stored as an sarray<vector<entry_value> >
 *
 * Raw data layout
 * ---------------
 *
 * Each vector is simply laid out as
 *
 * | row 1 | row 2 | ... | row n |
 *
 * where n <= row_block_size.
 *
 * Each row is laid out according to the "mode" of the columns,
 * determined by column_metadata::mode.  mode can be NUMERIC,
 * CATEGORICAL, NUMERIC_VECTOR, CATEGORICAL_VECTOR, and7 DICTIONARY.  The
 * last three of these can hold multiple values.
 *
 * ROW FORMAT (from ml_data.hpp)
 * ================================================================================
 *
 * ml_data loads data from an existing sframe, indexes it by mapping all
 * categorical values to unique indices in 0, 1,2,...,n, and records
 * statistics about the values.  It then puts it into an efficient
 * row-based data storage structure for use in regression and learning
 * algorithms. The row based storage structure is designed for fast
 * iteration through the rows and target.
 *
 *
 * The rows are stored in segments of a vector of entry_value structs,
 * where entry_value encloses a union of an index_value and a
 * double_value.  Thus it is 8 bytes.  Each vector contains up to
 * ROW_BLOCK_SIZE rows.  The data is stored as an sarray<vector<entry_value> >
 *
 * Raw data layout
 * ---------------
 *
 * Each vector is simply laid out as
 *
 * | <row_size> row 1 | <row_size> row 2 | ... | <row_size> row n |
 *
 * The row size variable is present if CATEGORICAL_VECTOR or
 * DICTIONARY are defined types, as these may induce variable sized
 * rows.  Otherwise, the row sizes are constant and determined from
 * the metadata, thus this does not need to be there.
 *
 * where n <= ROW_BLOCK_SIZE.
 *
 * Each row is laid out according to the "mode" of the columns,
 * determined by column_metadata::mode.  mode can be NUMERIC,
 * CATEGORICAL, NUMERIC_VECTOR, CATEGORICAL_VECTOR, and DICTIONARY.  The
 * last three of these can hold multiple values.
 *
 * Row layout
 * ----------
 *
 * Each row takes a block of data entries laid out according to mode:
 *
 * NUMERIC:
 *
 *   ... | value | ...
 *
 * CATEGORICAL:
 *
 *   ... | index | ...
 *
 * NUMERIC_VECTOR:
 *
 *   ... | value1 value2 ... | ...
 *
 * CATEGORICAL_VECTOR:
 *
 *   ... | length index1 index2 ... | ...
 *
 * DICTIONARY:
 *
 *   ... | length index1 value1 index2 value2 ... | ...
 *
 *   example: {0 : 2.1, 1 : 8.5} would be ... | 2 0 2.1 1 8.5 | ...
 *
 *
 * Since the number of columns, and the mode of each column is constant
 * across rows, the values are all laid out sequentially; for example, a
 * row with columns (numeric, numeric_vector, dictionary) would be
 *
 * ... || row_size | value | v1 v2 ... | length k1 v1 k2 v2 ... || ...
 *
 * Thus a row of [1.0, [2.0, 4.0], {0 : 2.1, 1 : 8.5}] would appear as
 *
 * ... || 8 | 1.0 | 2.0 4.0 | 2 0 2.1 1 8.5 || ...
 * = ... || 8 1.0 2.0 4.0 2 0 2.1 1 8.5 || ...
 *
 * Similarly, a row with columns (numeric, numeric_vector, categorical)
 * would be
 *
 * ... || value | v1 v2 ... | k1 || ...
 * * And a row of [1.0, [2.0, 4.0], 8] would appear as
 *
 * ... || 1.0 | 2 2.0 4.0 | 8 || ...
 * = ... || 1.0 2 2.0 4.0 8 || ...
 * Since ROW_BLOCK_SIZE are stored together in a vector<entry_value>,
 * iterating through rows is extremely fast.
 *
 *
 * target access
 * ======================================================================
 *
 * If a target column is present, it is stored as the LAST element in
 * exactly the same way as a regular numeric value.
 */
namespace turi { namespace ml_data_internal {

////////////////////////////////////////////////////////////////////////////////
//
//  Data type definitions.
//
////////////////////////////////////////////////////////////////////////////////

extern size_t ML_DATA_TARGET_ROW_BYTE_MINIMUM;

/** The raw data storage unit. Contains only 8 bytes; only one of
 *  these two values is used at any given point.
 */
struct entry_value : public turi::IS_POD_TYPE {
  union {
    size_t index_value;
    double double_value;
  };
};

typedef const entry_value* entry_value_iterator;


/**  The structure that holds the data for a given row.
 */
struct row_data_block {
  std::vector<entry_value> entry_data;
  std::vector<flexible_type> additional_data;

  void load(turi::iarchive& iarc) GL_HOT;
  void save(turi::oarchive& oarc) const GL_HOT;
};


////////////////////////////////////////////////////////////////////////////////
//
// Utility functions for management of internal data stuff
//
////////////////////////////////////////////////////////////////////////////////

/** Translates the raw flexible_type data in column_buffer into a
 *  block of rows, indexing it through the metadata classes.  The
 *  output format is described in ml_data.hpp.
 *
 *  If a target column is present, it is assumed to be the first
 *  column in metadata.
 *
 *  \param[out] row2data_idx_map A vector corresponding to the number
 *  of rows.  Will be giving the starting indices in block_output.
 *
 *  \param[out] block_output A destination vector of raw row storage.
 *
 *  \param[in] metadata A vector of column_metadata objects giving the
 *  metadata for each column.
 *
 *  \param[in] column_buffer A vector of vectors, with each inner
 *  vector holding one unindexed column from the raw source and
 *  corresponding to one of the column_metadata objects.  All the
 *  columns must be the same length.
 *
 *  \param[in] track_statistics If true then the statistics in
 *  metadata is set with the current values.
 *
 *  \param[in] immutable_metadata If false, then new categorical
 *  values are allowed and inserted into the metadata tracking.  If
 *  true, then new categories are mapped to size_t(-1) and a warning
 *  is printed at the end giving the number of previously unseen
 *  categories.
 *
 *  \param[in] index_remapping If given, map all indices through this
 *  vector.  This allows row blocks to be sorted or shuffled by the
 *  calling function.
 *  \param[in] none_action Can be set to one of ERROR/SKIP/IMPUTE
 *
 *  \return The maximum row size.
 *
 */
size_t fill_row_buffer_from_column_buffer(
    std::vector<size_t>& row2data_idx_map,
    row_data_block& block_output,
    const row_metadata& rm,
    const std::vector<std::vector<flexible_type> >& column_buffer,
    size_t thread_idx,
    bool track_statistics,
    bool immutable_metadata,
    ml_missing_value_action none_action) GL_HOT;

/** Truncates a row_data_block in place to n_rows.
 *
 */
void truncate_row_data_block(
    const row_metadata& rm,
    row_data_block& block_output,
    size_t n_rows);


/** Takes a row from position entry_value_iterator, appending it to
 *  output_block.
 */
void append_row_to_row_data_block(
    const row_metadata& rm,
    row_data_block& output_block,
    entry_value_iterator src_location);

/** Remap all the indices in a block, rewriting things back.
 */
void reindex_block(const row_metadata& rm, row_data_block& block,
                   const std::vector<std::vector<size_t> >& reindex_map) GL_HOT_FLATTEN;

/** Determines the number of ml_data_entry objects needed to fit a
 *  mapped buffer of flexible_type columns into.
 *
 *  \param[in] metadata A vector of column_metadata objects giving the
 *  metadata for each column.
 *
 *  \param[in] column_buffer A vector of vectors, with each inner
 *  vector holding one unindexed column from the raw source and
 *  corresponding to one of the column_metadata objects.  All the
 *  columns must be the same length.
 *
 *  \param[in] none_action Can be set to one of ERROR/SKIP/IMPUTE
 *
 *  \return The block size in number of ml_data_entry objects required.
 */
size_t estimate_num_data_entries(
    const column_metadata_ptr& m, const flexible_type& v);

/**
 * Returns the size of the row data block starting at block location
 * block_location.  If has_target is true, the first element is
 * assumed to be the target value.  The rest of the columns are given
 * by column_modes.
 *
 * BlockLocationIterator can be either a pointer or a vector iterator
 * to the start of the current row.
 *
 * The number of data elements in that row is returned.
 */
GL_HOT_INLINE_FLATTEN
static inline size_t get_row_data_size(
    const row_metadata& rm, entry_value_iterator block_location) {

  if(rm.data_size_is_constant)
    return rm.constant_data_size;
  else
    return block_location->index_value;
}


/** Returns the target value of the current row block.
 */
GL_HOT_INLINE_FLATTEN
static inline double get_target_value(
    const row_metadata& rm, entry_value_iterator block_location) {

  if(!rm.has_target || rm.target_is_indexed)
    return 1.0;
  else
    return (block_location + get_row_data_size(rm, block_location) - 1)->double_value;
}

/** Returns the target index of the current row block.
 */
GL_HOT_INLINE_FLATTEN
static inline size_t get_target_index(
    const row_metadata& rm, entry_value_iterator block_location) {

  if(!rm.has_target || !rm.target_is_indexed)
    return 0;
  else
    return (block_location + get_row_data_size(rm, block_location) - 1)->index_value;
}

/** Reads the rows of data out as a sequence of (column_index,
 *  feature_index, double value) tuples.
 *
 *  EntryWriteFunction out_function is a function taking 6 arguments:
 *
 *  (ml_column_mode mode,
 *   size_t column_index,
 *   size_t feature_index,
 *   double value,
 *   size_t index_size,
 *   size_t index_offset).
 *
 *  row_block_ptr is a pointer or iterator to the first entry_value of
 *  the row.
 *
 *  rm is the row_metadata structure associated with this row.
 */
template <
  typename EntryWriteFunction,
  typename ColumnAdvancementFunction>
GL_HOT_INLINE_FLATTEN
static inline void
read_ml_data_row(
    const row_metadata& rm,
    entry_value_iterator& row_block_ptr,
    EntryWriteFunction&& out_function,
    ColumnAdvancementFunction&& next_column) {

#ifndef NDEBUG
  entry_value_iterator original_row_block_ptr = row_block_ptr;
  entry_value_iterator row_block_end = row_block_ptr + get_row_data_size(rm, row_block_ptr);
#endif

  DASSERT_EQ(rm.num_x_columns + (rm.has_target ? 1 : 0), rm.total_num_columns);

  /** Skip the first entry if the row size is not constant, as this
   * contains the row data size. */
  if(!rm.data_size_is_constant)
    ++row_block_ptr;

  const size_t num_columns = rm.num_x_columns;

  for(size_t c_idx = 0; c_idx < num_columns; ++c_idx) {

    DASSERT_LT(c_idx, rm.metadata_vect.size());

    ////////////////////////////////////////////////////////////////////////////////
    // Step 1: Set up everything needed to read out values from the raw data.

    size_t index_size = rm.metadata_vect[c_idx]->index_size();
    size_t index_offset = rm.metadata_vect[c_idx]->global_index_offset();

    // The offset should always have been initialized.
    DASSERT_NE(index_offset, size_t(-1));

    /** Call this to read out an index.
     */
    auto read_index = [&]() GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
      DASSERT_LT(row_block_ptr, row_block_end);

      size_t index = row_block_ptr->index_value;
      ++row_block_ptr;
      return index;
    };

    /** Call this to read out a value.
     */
    auto read_value = [&]() GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
      DASSERT_LT(row_block_ptr, row_block_end);

      double value = row_block_ptr->double_value;
      ++row_block_ptr;
      return value;
    };

    /** Call this to read out a size.
     */
    auto read_size = [&]() GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
      size_t size = row_block_ptr->index_value;
      ++row_block_ptr;

      DASSERT_FALSE(rm.data_size_is_constant);
      DASSERT_LT(size, get_row_data_size(rm, original_row_block_ptr));

      return size;
    };

    ////////////////////////////////////////////////////////////////////////////////
    // Step 2:  Go through and read values based on the type.

    // For some of the modes, an important optimization is to set
    // all_columns_categorical to be true in the data profile, which
    // means that this branch / lookup is optimized out at compile
    // time.

    switch(rm.metadata_vect[c_idx]->mode) {

      case ml_column_mode::NUMERIC: {

        size_t index = 0;
        double value = read_value();

        out_function(ml_column_mode::NUMERIC, c_idx, index, value, index_size, index_offset);
        next_column(ml_column_mode::NUMERIC,  c_idx, index_size);

        break;
      }

      case ml_column_mode::CATEGORICAL: {

        // Just in case we use the all_columns_categorical override --
        // check to make sure it's valid.
        DASSERT_TRUE(rm.metadata_vect[c_idx]->mode == ml_column_mode::CATEGORICAL);

        size_t index = read_index();
        double value = 1.0;

        out_function(ml_column_mode::CATEGORICAL, c_idx, index, value, index_size, index_offset);
        next_column(ml_column_mode::CATEGORICAL,  c_idx, index_size);
        break;
      }

      case ml_column_mode::NUMERIC_VECTOR: {

        size_t nv = rm.metadata_vect[c_idx]->fixed_column_size();

        for(size_t k = 0; k < nv; ++k) {

          size_t index = k;
          double value = read_value();

          out_function(ml_column_mode::NUMERIC_VECTOR, c_idx, index, value, index_size, index_offset);
        }

        next_column(ml_column_mode::NUMERIC_VECTOR, c_idx, index_size);
        break;
      }

      case ml_column_mode::NUMERIC_ND_VECTOR: {

        size_t nv = rm.metadata_vect[c_idx]->fixed_column_size();

        for(size_t k = 0; k < nv; ++k) {

          size_t index = k;
          double value = read_value();

          out_function(ml_column_mode::NUMERIC_ND_VECTOR, c_idx, index, value, index_size, index_offset);
        }

        next_column(ml_column_mode::NUMERIC_ND_VECTOR, c_idx, index_size);
        break;
      }

      case ml_column_mode::CATEGORICAL_VECTOR: {

        size_t nv = read_size();

        for(size_t k = 0; k < nv; ++k) {

          size_t index = read_index();
          double value = 1.0;

          out_function(ml_column_mode::CATEGORICAL_VECTOR, c_idx, index, value, index_size, index_offset);
        }

        next_column(ml_column_mode::CATEGORICAL_VECTOR, c_idx, index_size);
        break;
      }

      case ml_column_mode::DICTIONARY: {

        size_t nv = read_size();

        for(size_t k = 0; k < nv; ++k) {

          size_t index = read_index();
          double value = read_value();

          out_function(ml_column_mode::DICTIONARY, c_idx, index, value, index_size, index_offset);
        }

        next_column(ml_column_mode::DICTIONARY, c_idx, index_size);
        break;
      }

      case ml_column_mode::UNTRANSLATED: {

        // Do nothing in this case.
        break;
      }
      default:
        DASSERT_TRUE(false);
    } // End switch
  }
}

}}

#endif /* _ML_DATA_ROW_FORMAT_H_ */

/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ML2_DATA_ROW_TRANSATION_H_
#define TURI_ML2_DATA_ROW_TRANSATION_H_

#include <unity/toolkits/ml_data_2/data_storage/ml_data_row_format.hpp>
#include <unity/toolkits/ml_data_2/ml_data_entry.hpp>
#include <unity/toolkits/ml_data_2/ml_data_column_modes.hpp>
#include <unity/toolkits/ml_data_2/data_storage/internal_metadata.hpp>
#include <flexible_type/flexible_type.hpp>
#include <util/code_optimization.hpp>
#include <unity/toolkits/ml_data_2/side_features.hpp>
#include <type_traits>

#include <Eigen/SparseCore>
#include <Eigen/Core>

#include <array>

namespace turi { namespace v2 { namespace ml_data_internal {

typedef Eigen::Matrix<double, Eigen::Dynamic,1>  DenseVector;
typedef Eigen::SparseVector<double> SparseVector;

////////////////////////////////////////////////////////////////////////////////
// Create a fill function that works for both vectors and arrays of
// entries

template <typename T, typename E>
static inline void _add_element(std::vector<T>& v, size_t element_idx, const E& element) {
  DASSERT_EQ(element_idx, v.size());
  v.push_back(element);
}

template <typename T, size_t n, typename E>
static inline void _add_element(std::array<T, n>& v, size_t element_idx, const E& element) {
  DASSERT_LT(element_idx, v.size());
  v[element_idx] = element;
}

/**
 *  Copies a row into a sequence of ml_data_entry structures sent to
 *  the output iterator MLDataEntryIterator x_out.  This provides fast
 *  translation between the row blocks and ml_data_entry structures.
 *
 *  \param[in,out] row_block_ptr Points to the beginning of the row in
 *  the data entry block.  Gets updated to the start of the next row
 *  or the end of the data block, if the data block is the last one on
 *  that row.
 *
 *  \param[in] column_modes The modes of each column.
 *
 *  \param[in] has_target Whether the row starts with the target value
 *  or not.
 *
 *  \param[in] column_index_offset When filling in the ml_data_entry
 *  structures, this gives the starting index that the column_index
 *  value is based off of.
 */
template <typename EntryContainer>
GL_HOT_INLINE_FLATTEN
static inline void copy_raw_into_ml_data_entry_row(
    EntryContainer& row,
    const row_metadata& rm,
    entry_value_iterator row_block_ptr,
    const std::shared_ptr<ml_data_side_features>& side_features) {

  typedef typename EntryContainer::value_type Entry;

  DASSERT_TRUE(row.empty());

  size_t write_index = 0;

  read_ml_data_row(
      rm,

      row_block_ptr,

      /** The function to write out the data to x.
       */
      [&](ml_column_mode mode, size_t column_index,
          size_t feature_index, double value,
          size_t index_size, size_t index_offset) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {

        size_t global_index = (LIKELY(feature_index < index_size)
                               ? index_offset + feature_index
                               : size_t(-1));

        Entry e;
        e = ml_data_full_entry{column_index, feature_index, global_index, value};
        _add_element(row, write_index, e);
        ++write_index;
      },

      // Nothing that we need to do at the end of each column.
      [&](ml_column_mode, size_t, size_t) {},

      side_features);
}

// The main function that implements all of the above filling
// techniques.
template <typename SparesOrDenseEigenVector>
GL_HOT_INLINE_FLATTEN
inline void copy_raw_into_eigen_array(
    SparesOrDenseEigenVector& x,
    const row_metadata& rm,
    const entry_value_iterator& row_block_ptr,
    const std::shared_ptr<ml_data_side_features>& side_features,
    bool use_reference) {

  size_t offset = 0;

  read_ml_data_row(
      /** The row metadata. **/
      rm,

      /** The pointer to the current location.
       */
      row_block_ptr,

      /** The function to write out the data to x.
       */
      [&](ml_column_mode mode, size_t column_index,
          size_t feature_index, double value,
          size_t index_size, size_t index_offset) {

        if(UNLIKELY(feature_index >= index_size))
          return;

        size_t idx = offset + feature_index;

        // Decrement the category if it isn't the reference category.
        if(use_reference && mode_is_categorical(mode)) {
          if (feature_index != 0) {
            idx -= 1;
          } else {
            return;
          }
        }
 
        DASSERT_GE(idx,  0);
        DASSERT_LT(idx, size_t(x.size()));
        x.coeffRef(idx) = value;

      },

      /** The function to advance the offset, called after each column
       *  is finished.
       */
      [&](ml_column_mode mode, size_t column_index, 
                      size_t index_size) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
        offset += (index_size
            - ((use_reference && mode_is_categorical(mode)) ? 1 : 0));
      },

      /** The pointer to the side features.  Possibly null.
       */
      side_features);
}

////////////////////////////////////////////////////////////////////////////////
// Translation routines to the basic ml_data_entry type

/** Translation from one row type to another
 */
std::vector<ml_data_entry> translate_row_to_ml_data_entry(
    const std::shared_ptr<ml_metadata>& metadata,
    const std::vector<ml_data_entry_global_index>& row);

/** Translation routines.
 */
std::vector<ml_data_entry> translate_row_to_ml_data_entry(
    const std::shared_ptr<ml_metadata>& metadata,
    const DenseVector& row);

/** Translates the original sparse row format to the ml_data_entry
 *  vector.
 */
std::vector<ml_data_entry> translate_row_to_ml_data_entry(
    const std::shared_ptr<ml_metadata>& metadata,
    const SparseVector& v);


////////////////////////////////////////////////////////////////////////////////
// translation routines to the original row type

std::vector<flexible_type> translate_row_to_original(
    const std::shared_ptr<ml_metadata>& metadata,
    const std::vector<ml_data_entry>& row);

/** Translates the original sparse row format to the original flexible
 *  types.
 */
std::vector<flexible_type> translate_row_to_original(
    const std::shared_ptr<ml_metadata>& metadata,
    const DenseVector& v);

/** Translates the original sparse row format to the original flexible
 *  types.
 */
std::vector<flexible_type> translate_row_to_original(
    const std::shared_ptr<ml_metadata>& metadata,
    const SparseVector& v);

/** Translate a vector of global indices to the next
 */
std::vector<flexible_type> translate_row_to_original(
    const std::shared_ptr<ml_metadata>& metadata,
    const std::vector<ml_data_entry_global_index>& row);


}}}


#endif /* _ML_DATA_ROW_FORMAT_H_ */

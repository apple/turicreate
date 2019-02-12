/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <logger/assertions.hpp>
#include <unity/toolkits/ml_data_2/iterators/composite_row_type.hpp> 
#include <unity/toolkits/ml_data_2/side_features.hpp> 

namespace turi { namespace v2 {


/** Constructor; requires a metadata object. 
 */
composite_row_specification::composite_row_specification(const std::shared_ptr<ml_metadata>& _metadata)
    : metadata(_metadata)
{
  sparse_spec.resize(metadata->num_columns());
  dense_spec.resize(metadata->num_columns());
}



/** Add in a sparse subrow.  Returns the index in the sparse_subrows
 *  attribute of the composite_row_container where this particular
 *  row will go.
 */
size_t composite_row_specification::add_sparse_subrow(
    const std::vector<size_t>& column_indices) { 

  ASSERT_MSG(!metadata->has_side_features(),
             "Side features are not supported with composite row types."); 

  size_t sparse_index = n_sparse_subrows;
  size_t sparse_index_size = 0;

  for(size_t c_idx : column_indices) {
    ASSERT_MSG(!metadata->is_untranslated_column(c_idx),
               "Untranslated column cannot be assigned to a sparse subrow.");
    sparse_spec[c_idx].push_back(sparse_index);
    sparse_index_size += metadata->index_size(c_idx);
  }
  
  sparse_spec_sizes.push_back(sparse_index_size); 
  
  ++n_sparse_subrows;
  
  return sparse_index; 
}

/** Add in a dense subrow.  Returns the index in the dense_subrows
 *  attribute of the composite_row_container where this particular
 *  row will go upon filling from the iterator.
 */
size_t composite_row_specification::add_dense_subrow(
    const std::vector<size_t>& column_indices) { 

  ASSERT_MSG(!metadata->has_side_features(),
             "Side features are not supported with composite row types."); 

  size_t dense_index = n_dense_subrows;
  size_t dense_index_size = 0;
  
  for(size_t c_idx : column_indices) {
    ASSERT_MSG(!metadata->is_untranslated_column(c_idx),
               "Untranslated column cannot be assigned to a dense subrow.");

    dense_spec[c_idx].push_back(dense_index);
    dense_index_size += metadata->index_size(c_idx); 
  }
  
  dense_spec_sizes.push_back(dense_index_size); 

  ++n_dense_subrows;
  
  return dense_index; 
}


/** Add in a flexible type subrow.  Returns the index in the
 *  flex_subrows attribute of the composite_row_container where this
 *  particular row will go upon filling from the iterator.
 */
size_t composite_row_specification::add_flex_type_subrow(
    const std::vector<size_t>& column_indices) {

  size_t flex_subrow_index = flex_subrow_spec_by_subrow.size();

  // The indices we store are according the vector of untranslated
  // columns, so we need to translate them from the column_indices to
  // how they would appear in the vector returned by
  // fill_untranslated_values.
  std::vector<size_t> contiguous_flextype_indices(column_indices.size()); 

  for(size_t i = 0; i < column_indices.size(); ++i) {
    size_t c_idx = column_indices[i];
    
    ASSERT_MSG(metadata->is_untranslated_column(c_idx),
               "Untranslated column required for flex_type_subrow.");

    size_t tr_idx = 0; 
    for(size_t j = 0; j < c_idx; ++j) {
      if(metadata->is_untranslated_column(j))
        ++tr_idx; 
    }

    contiguous_flextype_indices[i] = tr_idx;
  }

  flex_subrow_spec_by_subrow.push_back(contiguous_flextype_indices); 
  
  ++n_flex_subrows;
  
  return flex_subrow_index; 
}

/** The primary filling function for the composite type.
 *
 */
void composite_row_specification::fill(composite_row_container& crc,
                                       const ml_data_internal::row_metadata& rm,
                                       ml_data_internal::entry_value_iterator row_block_ptr,
                                       std::vector<flexible_type> flexible_type_row) {

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1: Set up the index offset tracker.  Each row has it's own
  // indexing here, so each is tracked locally.
    
  std::vector<size_t>& index_offsets = crc.buffer;
  index_offsets.assign(n_dense_subrows + n_sparse_subrows, 0);

  ////////////////////////////////////////////////////////////////////////////////
  // Step 2:  Clear everything out. 
    
  crc.dense_subrows.resize(n_dense_subrows);
  for(size_t i = 0; i < n_dense_subrows; ++i) {
    crc.dense_subrows[i].resize(dense_spec_sizes[i]);
    crc.dense_subrows[i].setZero();
  }

  crc.sparse_subrows.resize(n_sparse_subrows);
  for(size_t i = 0; i < n_sparse_subrows; ++i) {
    crc.sparse_subrows[i].resize(sparse_spec_sizes[i]);
    crc.sparse_subrows[i].setZero();
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3:  Translate the row. 
  
  read_ml_data_row(
      rm,
        
      row_block_ptr,

      /** The function to write out the data of an element to the
       *  proper locations in the composite row.
       */
      [&](ml_column_mode mode, size_t column_index,
          size_t feature_index, double value,
          size_t index_size, size_t index_offset) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {

        if(!dense_spec[column_index].empty()) {
          for(size_t dense_subrow_index : dense_spec[column_index]) {
            size_t idx = feature_index + index_offsets[dense_subrow_index];
            crc.dense_subrows[dense_subrow_index].coeffRef(idx) = value;
          }
        }

        if(!sparse_spec[column_index].empty()) {
          for(size_t sparse_subrow_index : sparse_spec[column_index]) {
            size_t idx = feature_index + index_offsets[n_dense_subrows + sparse_subrow_index];
            crc.sparse_subrows[sparse_subrow_index].coeffRef(idx) = value;
          }
        }
      },

      // The function for when we leave a column. When we leave a
      // column, increment the global index counter by the correct
      // amount.
      [&](ml_column_mode mode, size_t column_index, size_t index_size) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
          
        for(size_t dense_subrow_index : dense_spec[column_index])
          index_offsets[dense_subrow_index] += index_size;

        for(size_t sparse_subrow_index : sparse_spec[column_index])
          index_offsets[n_dense_subrows + sparse_subrow_index] += index_size;
      },

      std::shared_ptr<ml_data_side_features>());
  
  ////////////////////////////////////////////////////////////////////////////////
  // Step 4: Deal with the flexible_type subrows
    
  if(n_flex_subrows > 0) {

    crc.flex_subrows.resize(n_flex_subrows);

    for(size_t i = 0; i < n_flex_subrows; ++i) { 
      crc.flex_subrows[i].resize(flex_subrow_spec_by_subrow[i].size());

      for(size_t j = 0; j < flex_subrow_spec_by_subrow[i].size(); ++j)
        crc.flex_subrows[i][j] = flexible_type_row[flex_subrow_spec_by_subrow[i][j]]; 
    }
  }

  // And we're done!
}

}}

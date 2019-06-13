/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ML2_DATA_SIDE_FEATURES_TRANSATION_H_
#define TURI_ML2_DATA_SIDE_FEATURES_TRANSATION_H_

#include <toolkits/ml_data_2/data_storage/ml_data_row_format.hpp>
#include <toolkits/ml_data_2/ml_data_entry.hpp>
#include <toolkits/ml_data_2/ml_data_column_modes.hpp>
#include <toolkits/ml_data_2/data_storage/internal_metadata.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/util/code_optimization.hpp>
#include <type_traits>

namespace turi { namespace v2 {

class ml_data_side_features;

namespace ml_data_internal {

/** Write some of the information into a specified output iterator,
 *  with columns shifted by column_index_offset.  Meant to be used as
 *  the routine that the side features column uses.
 */
template <typename EntryType>
GL_HOT_INLINE_FLATTEN
static inline void append_raw_to_entry_row(
    const row_metadata& rm,
    entry_value_iterator row_block_ptr,
    std::vector<EntryType>& x_out,
    size_t column_index_offset) {

  read_ml_data_row(
      rm,

      /** The data pointer. */
      row_block_ptr,

      /** The function to write out the data to x.
       */
      [&](ml_column_mode mode, size_t local_column_index,
          size_t feature_index, double value, size_t index_size, size_t index_offset) {

        if(LIKELY(feature_index < index_size)) {

          size_t column_index = column_index_offset + local_column_index;

          size_t global_index = rm.metadata_vect[local_column_index]->global_index_offset() + feature_index;

          EntryType e;
          e = ml_data_full_entry{column_index,
                                 feature_index,
                                 global_index,
                                 value};

          x_out.push_back(e);
        }
      },

      // Nothing that we need to do at the end of each column.
      [&](ml_column_mode, size_t, size_t) {},

      /** No side information here. */
      std::shared_ptr<ml_data_side_features>());
}


}}}


#endif

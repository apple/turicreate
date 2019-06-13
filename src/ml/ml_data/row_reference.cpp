/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <ml/ml_data/ml_data.hpp>
#include <ml/ml_data/row_reference.hpp>

namespace turi {

/**
 * Fill an observation vector with the untranslated columns, if any
 * have been specified at setup time.  These columns are simply
 * mapped back to their sarray counterparts.
 */
void ml_data_row_reference::fill_untranslated_values(std::vector<flexible_type>& x) const {

  if(!has_untranslated_columns) {
    x.clear();
    return;
  }

  x.resize(data_block->untranslated_columns.size());

  for(size_t i = 0; i < data_block->untranslated_columns.size(); ++i) {
    x[i] = data_block->untranslated_columns[i][current_in_block_row_index];
  }

  DASSERT_TRUE(x.size() >= 1);
}


/** Create an ml_data_row_reference from a single sframe row reference.
 *
 *  Row must be in the format {column_name, value} and columns
 *  correspond to the columns in metadata.  Missing columns are
 *  treated as missing values.
 *
 *  Returns a single row reference.
 */
ml_data_row_reference ml_data_row_reference::from_row(
    const std::shared_ptr<ml_metadata>& metadata, const flex_dict& row,
    ml_missing_value_action none_action) {

  ////////////////////////////////////////////////////////////////////////////////
  // First, unpack the row vector with mappings.

  bool has_target = false;
  std::vector<size_t> col_indices(row.size());

  for(size_t i = 0; i < row.size(); ++i) {
    if(row[i].first.get_type() != flex_type_enum::STRING) {
      log_and_throw((std::string("Key type for column_name to value dictionary; expected string, got ")
                     + flex_type_enum_to_name(row[i].first.get_type())).c_str());
    }

    const flex_string& col_name = row[i].first.get<flex_string>();

    if(metadata->has_target() && col_name == metadata->target_column_name()) {
      col_indices[i] = metadata->num_columns();
      has_target = true;
    } else {
      col_indices[i] = metadata->column_index(col_name, true);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Next, build the data_block we're going to dump it in to.

  std::shared_ptr<ml_data_internal::ml_data_block> data_block(new ml_data_internal::ml_data_block);

  data_block->metadata = metadata;
  data_block->rm = (has_target
                    ? metadata->cached_rm_with_target
                    : metadata->cached_rm_without_target);

  ////////////////////////////////////////////////////////////////////////////////
  // Copy the data over
  std::vector<std::vector<flexible_type> > data(
      data_block->rm.total_num_columns, {FLEX_UNDEFINED});

  for(size_t i = 0; i < row.size(); ++i) {
    size_t col_idx = col_indices[i];
    if(col_idx == size_t(-1))
      continue;

    const auto& v = row[i].second;
    data[col_idx][0] = v;

    if(v.get_type() != flex_type_enum::UNDEFINED) {
      ml_data_internal::check_type_consistent_with_mode(
          data_block->rm.metadata_vect[col_idx]->name,
          v.get_type(),
          data_block->rm.metadata_vect[col_idx]->mode);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Handle any untranslated columns properly.

  bool has_untranslated_columns = metadata->has_untranslated_columns();

  if(has_untranslated_columns) {
    size_t num_utc = metadata->num_untranslated_columns();

    data_block->untranslated_columns.reserve(num_utc);

    for(size_t i = 0; i < data.size(); ++i) {
      if(metadata->is_untranslated_column(i)) {
        data_block->untranslated_columns.push_back(std::move(data[i]));
      }
    }
  }

  // Single use for the mapping of one item.
  static std::vector<size_t> row2data_idx_map = {0};

  // Do the unpacking.
  ml_data_internal::fill_row_buffer_from_column_buffer(
      row2data_idx_map,
      data_block->translated_rows,
      data_block->rm,
      data,
      /* thread_idx = */ 0,
      /* track_statistics = */ false,
      /* immutable_metadata = */ true,
      none_action);

  // Build the reference.
  ml_data_row_reference row_ref;
  row_ref.data_block = data_block;
  row_ref.current_in_block_index = 0;
  row_ref.current_in_block_row_index = 0;
  row_ref.has_translated_columns = metadata->has_translated_columns();
  row_ref.has_untranslated_columns = has_untranslated_columns;

  return row_ref;
}

}

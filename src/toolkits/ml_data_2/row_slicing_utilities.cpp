/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/ml_data_2/row_slicing_utilities.hpp>
#include <core/logging/assertions.hpp>
#include <string>

namespace turi { namespace v2 {

/** Constructor -- provide ml_metadata class and a subset of column
 *  indices to use in this particular row.  The _columns_to_pick must
 *  be in sorted order.
 *
 *  If the chosen columns are from untranslated columns, then they
 *  must be all untranslated columns.  In this case, only the
 *  flexible_type slice method below can be used.  Otherwise, none
 *  of the columns must be untranslated, and either the sparse or
 *  dense slicing methods must be used.
 */
row_slicer::row_slicer(const std::shared_ptr<ml_metadata>& metadata,
                       const std::vector<size_t>& _columns_to_pick) {

  if(_columns_to_pick.empty()) {
    pick_from_flexible_type = false;
    return;
  }

  if(!std::is_sorted(_columns_to_pick.begin(), _columns_to_pick.end())) {
    ASSERT_MSG(false, "Selected columns must be in sorted order.");
    return;
  }

  ////////////////////////////////////////////////////////////
  // First go through and get the types of the given columns.
  pick_from_flexible_type = metadata->is_untranslated_column(_columns_to_pick.front()) ;

  // Make sure this is consistent
  for(size_t c : _columns_to_pick) {
    ASSERT_MSG(pick_from_flexible_type == metadata->is_untranslated_column(c),
               (std::string("Cannot mix untranslated and translated columns in single slice. (")
                + metadata->column_name(c) + ")").c_str());
  }

  auto is_included = [&](size_t i) {
    for(size_t c : _columns_to_pick) {
      if(c == i) return true;
    }

    return false;
  };

  if(pick_from_flexible_type) {
    // Go through and get the ones that
    flex_type_columns_to_pick.clear();

    // The indexing on the untranslated columns is dependent on the
    // untranslated ordering and count, so we have to translate the indices.
    size_t untranslated_column_count = 0;
    for(size_t c_idx = 0; c_idx < metadata->num_columns(); ++c_idx) {

      if(is_included(c_idx))
        flex_type_columns_to_pick.push_back(untranslated_column_count);

      if(metadata->is_untranslated_column(c_idx))
        ++untranslated_column_count;
    }
  } else {
    // We can just copy the indices over.
    column_pick_mask.assign(metadata->num_columns(), false);

    for(size_t c : _columns_to_pick)
      column_pick_mask[c] = true;

    // Now build the index counts and offsets
    index_offsets.assign(metadata->num_columns(), 0);
    index_sizes.assign(metadata->num_columns(), 0);

    size_t cum_sum = 0;
    for(size_t i = 0; i < metadata->num_columns(); ++i) {
      if(column_pick_mask[i]) {
        index_sizes[i] = metadata->index_size(i);
        index_offsets[i] = cum_sum;
        cum_sum += index_sizes[i];
      }
    }

    // Finally, set the number of dimensions
    _num_dimensions = cum_sum;
  }
}


/**  Take a row, represented by a pair of translated and
 *   untranslated columns (either of which may be empty), and
 *   use it to fill an eigen sparse vector with the result.
 */
void row_slicer::slice(sparse_vector& dest,
                       const std::vector<ml_data_entry>& x_t, const std::vector<flexible_type>&) const {

  ASSERT_MSG(!pick_from_flexible_type, "Cannot be used for untranslated columns.");

  dest.resize(_num_dimensions);
  dest.setZero();

  for(const ml_data_entry& v : x_t) {
    DASSERT_LT(v.column_index, index_sizes.size());
    if(!column_pick_mask[v.column_index])
      continue;

    // Gracefully disregard new values
    if(v.index >= index_sizes[v.column_index])
      continue;


    dest.coeffRef(v.index + index_offsets[v.column_index]) = v.value;
  }
}



/**  Take a row, represented by a pair of translated and
 *   untranslated columns (either of which may be empty), and
 *   use it to fill an eigen dense vector with the result.
 */
void row_slicer::slice(dense_vector& dest,
                       const std::vector<ml_data_entry>& x_t, const std::vector<flexible_type>& x_u) const {

  ASSERT_MSG(!pick_from_flexible_type, "Cannot be used for untranslated columns.");

  dest.resize(_num_dimensions);
  dest.setZero();

  for(const ml_data_entry& v : x_t) {
    DASSERT_LT(v.column_index, index_sizes.size());

    if(!column_pick_mask[v.column_index])
      continue;

    // Gracefully disregard new values
    if(v.index >= index_sizes[v.column_index])
      continue;

    dest[v.index + index_offsets[v.column_index]] = v.value;
  }
}

/**  Take a row, represented by a pair of translated and
 *   untranslated columns (either of which may be empty), and
 *   use it to fill an untranslated row with the result.
 */
void row_slicer::slice(std::vector<flexible_type>& dest,
                       const std::vector<ml_data_entry>& x_t, const std::vector<flexible_type>& x_u) const {

  ASSERT_MSG(pick_from_flexible_type, "Can only be used for untranslated columns.");

  dest.resize(flex_type_columns_to_pick.size());

  for(size_t i = 0; i < flex_type_columns_to_pick.size(); ++i) {
    DASSERT_LT(flex_type_columns_to_pick[i], x_u.size());
    dest[i] = x_u[flex_type_columns_to_pick[i]];
  }
}

/** Serialization -- save.
 */
void row_slicer::save(turi::oarchive& oarc) const {

  size_t version = 0;

  std::map<std::string, variant_type> data;

  data["version"]                   = to_variant(version);
  data["pick_from_flexible_type"]   = to_variant(pick_from_flexible_type);
  data["flex_type_columns_to_pick"] = to_variant(flex_type_columns_to_pick);
  data["column_pick_mask"]          = to_variant(column_pick_mask);
  data["index_offsets"]             = to_variant(index_offsets);
  data["index_sizes"]               = to_variant(index_sizes);
  data["_num_dimensions"]           = to_variant(_num_dimensions);

  variant_deep_save(data, oarc);

}


/** Serialization -- load.
 */
void row_slicer::load(turi::iarchive& iarc) {

#define __EXTRACT(var) var = variant_get_value<decltype(var)>(data.at(#var));

  std::map<std::string, variant_type> data;
  variant_deep_load(data, iarc);

  __EXTRACT(pick_from_flexible_type);
  __EXTRACT(flex_type_columns_to_pick);
  __EXTRACT(column_pick_mask);
  __EXTRACT(index_offsets);
  __EXTRACT(index_sizes);
  __EXTRACT(_num_dimensions);

#undef __EXTRACT

}

}}

/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <ml/ml_data/data_storage/ml_data_row_format.hpp>
#include <ml/ml_data/data_storage/ml_data_row_translation.hpp>
#include <ml/ml_data/ml_data.hpp>
#include <ml/ml_data/metadata.hpp>

////////////////////////////////////////////////////////////////////////////////

namespace turi {

template <typename GlobalEntryType>
static std::vector<ml_data_entry> _translate_row_to_ml_data_entry(
    const std::shared_ptr<ml_metadata>& metadata,
    const std::vector<GlobalEntryType>& row) {

  std::vector<ml_data_entry> x;
  size_t col = 0;
  size_t shift = 0;

  x.reserve(row.size());

  size_t last_global_index = 0;

  for (size_t i = 0; i < row.size(); ++i) {

    size_t global_index = row[i].global_index;

    if(global_index == size_t(-1))
      continue;

    if(last_global_index >= global_index) {
      shift = 0;
      col = 0;
    }

    while(global_index >= metadata->index_size(col) + shift) {
      shift += metadata->index_size(col);
      col++;
      DASSERT_LT(col, metadata->num_columns());
    }

    DASSERT_LE(shift, global_index);

    size_t feature_index = global_index - shift;

    DASSERT_EQ(shift, metadata->global_index_offset(col));

    x.push_back(ml_data_entry{col, feature_index, row[i].value});

    last_global_index = global_index;
  }

  return x;
}


/** Translation routines.
 */
std::vector<ml_data_entry> translate_row_to_ml_data_entry(
    const std::shared_ptr<ml_metadata>& metadata,
    const std::vector<ml_data_entry_global_index>& row) {

  return _translate_row_to_ml_data_entry(metadata, row);
}



/** Translation routines.
 */
std::vector<ml_data_entry> translate_row_to_ml_data_entry(
    const std::shared_ptr<ml_metadata>& metadata,
    const DenseVector& v) {

  std::vector<ml_data_entry> x;
  x.reserve(v.size());
  size_t col = 0;
  size_t shift = 0;

  for (size_t i = 0; i < (size_t) v.rows(); i++) {
    if(v[i] == 0)
      continue;

    size_t global_index = i;

    while(global_index >= metadata->index_size(col) + shift ) {
      shift += metadata->index_size(col);
      col++;
      if (col >= metadata->num_columns())
        break;
    }
    x.push_back(ml_data_entry{ col, global_index - shift, v[i] });
  }

  return x;
}


/** Translates the original sparse row format to the ml_data_entry
 *  vector.
 */
std::vector<ml_data_entry> translate_row_to_ml_data_entry(
    const std::shared_ptr<ml_metadata>& metadata,
    const SparseVector& v) {


  std::vector<ml_data_entry> x;
  size_t col = 0;
  size_t shift = 0;

  x.reserve(v.nonZeros());

  for (SparseVector::InnerIterator it(v); it; ++it) {
    size_t global_index = it.index();

    while(global_index >= metadata->index_size(col) + shift ) {
      shift += metadata->index_size(col);
      col++;
      if (col >= metadata->num_columns())
        break;
    }

    x.push_back(ml_data_entry{col, global_index - shift, it.value()});
  }

  return x;
}

////////////////////////////////////////////////////////////////////////////////

/** Translates a row from the original row into the flexible type version
 */
std::vector<flexible_type> translate_row_to_original(
    const std::shared_ptr<ml_metadata>& metadata,
    const std::vector<ml_data_entry>& x) {

  std::vector<flexible_type> ret(metadata->num_columns());

  for(size_t c_idx = 0; c_idx < metadata->num_columns(); ++c_idx) {
    if(metadata->column_mode(c_idx) == ml_column_mode::UNTRANSLATED) {
      ret[c_idx] = flexible_type(flex_type_enum::UNDEFINED);
    } else if(metadata->column_mode(c_idx) == ml_column_mode::NUMERIC_VECTOR) {
      ret[c_idx] = flex_vec(metadata->column_size(c_idx), 0.0);
    } else if(metadata->column_mode(c_idx) == ml_column_mode::NUMERIC_ND_VECTOR) {
      ret[c_idx] = flex_nd_vec(metadata->nd_column_shape(c_idx), 0.0);
    } else {
      ret[c_idx] = flexible_type(metadata->column_type(c_idx));
    }
  }

  for(size_t i = 0; i < x.size(); ++i) {
    const ml_data_entry& v = x[i];
    size_t c_idx = v.column_index;

    const auto& m_idx = metadata->indexer(c_idx);

    switch(metadata->column_mode(c_idx)) {
      case ml_column_mode::NUMERIC: {
        if(metadata->column_type(c_idx) == flex_type_enum::INTEGER)
          ret[c_idx] = flex_int(v.value);
        else
          ret[c_idx] = v.value;
        break;
      }

      case ml_column_mode::CATEGORICAL:
      case ml_column_mode::CATEGORICAL_SORTED: {
        ret[c_idx] = m_idx->map_index_to_value(v.index);
        break;
      }

      case ml_column_mode::DICTIONARY: {

        flexible_type key = m_idx->map_index_to_value(v.index);

        flex_dict& dv = ret[c_idx].mutable_get<flex_dict>();
        dv.push_back(std::make_pair(key, flexible_type(v.value)));
        std::sort(dv.begin(), dv.end());

        break;
      }
      case ml_column_mode::NUMERIC_VECTOR: {

        flex_vec& vv = ret[c_idx].mutable_get<flex_vec>();

        DASSERT_EQ(vv.size(), metadata->column_size(c_idx));
        DASSERT_LT(v.index, vv.size());

        // Should always be true, but don't want to crash.
        if(v.index < vv.size())
          vv[v.index] = v.value;

        vv[v.index] = v.value;
        break;
      }

      case ml_column_mode::CATEGORICAL_VECTOR: {
        ret[c_idx].mutable_get<flex_list>().push_back(m_idx->map_index_to_value(v.index));

        std::sort(ret[c_idx].mutable_get<flex_list>().begin(),
                  ret[c_idx].mutable_get<flex_list>().end());

        break;
      }

      case ml_column_mode::UNTRANSLATED: {
        ret[c_idx] = flexible_type(flex_type_enum::UNDEFINED);
        break;
      }

      case ml_column_mode::NUMERIC_ND_VECTOR: {

        flex_nd_vec& vv = ret[c_idx].mutable_get<flex_nd_vec>();

        DASSERT_TRUE(vv.is_canonical());
        DASSERT_EQ(vv.num_elem(), metadata->column_size(c_idx));
        DASSERT_LT(v.index, vv.num_elem());

        // Should always be true, but don't want to crash.
        if(v.index < vv.num_elem()) {
          vv[v.index] = v.value;
        }

        vv[v.index] = v.value;
        break;
      }
    }
  }

  return ret;
}


/** Translates the original sparse row format to the original flexible
 *  types.
 */
std::vector<flexible_type> translate_row_to_original(
    const std::shared_ptr<ml_metadata>& metadata,
    const DenseVector& v) {

  return translate_row_to_original(metadata, translate_row_to_ml_data_entry(metadata, v));
}

/** Translates the original sparse row format to the original flexible
 *  types.
 */
std::vector<flexible_type> translate_row_to_original(
    const std::shared_ptr<ml_metadata>& metadata,
    const SparseVector& v) {

  return translate_row_to_original(metadata, translate_row_to_ml_data_entry(metadata, v));
}

/** Translate a vector of global indices to the next
 */
std::vector<flexible_type> translate_row_to_original(
    const std::shared_ptr<ml_metadata>& metadata,
    const std::vector<ml_data_entry_global_index>& row) {
  return translate_row_to_original(metadata, translate_row_to_ml_data_entry(metadata, row));
}

}

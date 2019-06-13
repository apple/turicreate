/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FACTORIZATION_FACTORS_TO_SFRAME_H_
#define TURI_FACTORIZATION_FACTORS_TO_SFRAME_H_

#include <toolkits/ml_data_2/ml_data.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <string>

namespace turi { namespace factorization {

/**  Fills a unity_sframe object with data from the features in the
 *   model.
 */
template <typename VectorType, typename EigenMatrixType>
sframe fill_linear_model_sframe_from_eigen_data(
    const std::shared_ptr<v2::ml_metadata>& metadata,

    size_t c_idx,

    size_t n,

    bool include_w_terms,
    size_t w_idx_offset,
    const std::string& w_name,
    const VectorType& w,

    bool include_V_terms,
    size_t V_idx_offset,
    const std::string& V_name,
    const EigenMatrixType& V) {

  bool is_categorical;
  size_t n_rows;

  switch(metadata->column_mode(c_idx)) {
    case turi::v2::ml_column_mode::CATEGORICAL:
    case turi::v2::ml_column_mode::CATEGORICAL_VECTOR:
    case turi::v2::ml_column_mode::DICTIONARY:
        is_categorical = true;
        n_rows = n;
        break;

    case turi::v2::ml_column_mode::NUMERIC:
        is_categorical = false;
        n_rows = 1;
        break;

    case turi::v2::ml_column_mode::NUMERIC_VECTOR:
        is_categorical = false;
        n_rows = metadata->column_size(c_idx);
        break;
      default:
        ASSERT_FALSE(true);
  };

  std::vector<std::string> names;
  std::vector<flex_type_enum> types;

  names.push_back(metadata->column_name(c_idx));

  // Decide on the type present; promote to string if there is an issue.
  {
    std::set<flex_type_enum> value_types_present = metadata->indexer(c_idx)->extract_key_types();

    // If undefined is in there, it is typically present with
    // other values.
    if(value_types_present.find(flex_type_enum::UNDEFINED) != value_types_present.end())
      value_types_present.erase(flex_type_enum::UNDEFINED);

    // If no data is present, then use undefined.
    if(value_types_present.size() == 0)
      value_types_present.insert(flex_type_enum::UNDEFINED);

    flex_type_enum out_type;

    if(value_types_present.size() == 1) {
      out_type = *value_types_present.begin();
    } else {
      logprogress_stream << "WARNING: Differing categorical key types present in list or "
                         << "dictionary on column " << metadata->column_name(c_idx)
                         << "; promoting all to string type." << std::endl;
      out_type = flex_type_enum::STRING;
    }

    types.push_back(out_type);
  }

  size_t w_col_idx = 0, V_col_idx = 0;

  if(include_w_terms) {
    DASSERT_LE(w_idx_offset + n, w.size());

    w_col_idx = names.size();
    names.push_back(w_name);
    types.push_back(flex_type_enum::FLOAT);
  }

  if(include_V_terms) {
    DASSERT_LE(V_idx_offset + n, V.rows());

    V_col_idx = names.size();
    names.push_back(V_name);
    types.push_back(flex_type_enum::VECTOR);
  }

  size_t num_columns = names.size();

  size_t num_segments = thread::cpu_count();

  sframe out;

  out.open_for_write(names, types, "", num_segments);

  size_t num_factors = V.cols();

  in_parallel([&](size_t thread_idx, size_t num_threads) {

      size_t start_idx = (thread_idx * n_rows) / num_threads;
      size_t end_idx   = ( (thread_idx + 1) * n_rows) / num_threads;

      auto it_out = out.get_output_iterator(thread_idx);

      std::vector<flexible_type> x(num_columns);

      flex_vec factors(num_factors);

      for(size_t i = start_idx; i < end_idx; ++i, ++it_out) {
        if(is_categorical) {
          x[0] = metadata->indexer(c_idx)->map_index_to_value(i);
        } else {
          x[0] = i;
        }

        if(include_w_terms)
          x[w_col_idx] = w[i + w_idx_offset];

        if(include_V_terms) {

          for(size_t j = 0; j < num_factors; ++j)
            factors[j] = V(i + V_idx_offset, j);

          x[V_col_idx] = factors;
        }

        *it_out = x;
      }
    });

  out.close();

  return out;
}

}}


#endif /* TURI_RECSYS_FILL_MODEL_SFRAME_H_ */

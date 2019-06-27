/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <ml/ml_data/metadata.hpp>
#include <model_server/lib/variant.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>

namespace turi {

/** Returns a vector giving the column names of all columns present.
 */
std::vector<std::string> ml_metadata::column_names() const {

  size_t n_columns = columns.size();

  std::vector<std::string> column_names(n_columns);

  for(size_t c_idx = 0; c_idx < n_columns; ++c_idx)
    column_names[c_idx] = column_name(c_idx);

  return column_names;
}

 /**
   * Returns the feature name of a specific feature present in the metadata.
   *
   * Numeric columns are represented by the column name.
   *
   * Categorical / Categorical List / Dictionary columns are represented by
   * "name[category]".
   *
   * Vectors are represented by "vector[index]", where index is numerical.
   *
   * ND vectors are represented by "nd_vector[idx1,idx2]" etc.
   *
   * \returns Names of features
   */
std::string ml_metadata::feature_name(size_t column_idx, size_t index, bool quote_string_values) const {
  const std::string& name = column_name(column_idx);

  switch (column_mode(column_idx)) {
    case ml_column_mode::NUMERIC:
    case ml_column_mode::UNTRANSLATED:
      DASSERT_EQ(index, 0);
      return name;

    case ml_column_mode::CATEGORICAL:
    case ml_column_mode::DICTIONARY:
    case ml_column_mode::CATEGORICAL_VECTOR:
    case ml_column_mode::CATEGORICAL_SORTED: {
      const flexible_type& v = indexer(column_idx)->map_index_to_value(index);

      bool quote = quote_string_values && (v.get_type() == flex_type_enum::STRING);

      return name + (quote ? "[\"" : "[") + v.to<std::string>() + (quote ? "\"]" : "]");
    }

    case ml_column_mode::NUMERIC_VECTOR:
      DASSERT_LT(index, column_size(column_idx));
      return name + "[" + std::to_string(index) + "]";
    case ml_column_mode::NUMERIC_ND_VECTOR: {
      const flex_nd_vec::index_range_type& shape = nd_column_shape(column_idx);

      // Need the product of all the remaining shape entries -- which is the
      // stride of the current dimension -- in order to calculate how much of
      // the index gets devoted to each dimension. We do this by starting with
      // the full product and dividing out each current dimension as we
      // encounter it.
      int64_t n = index_size(column_idx);

      std::ostringstream ss;
      ss << name << "[";

      int64_t remainder_idx = index;

      for (int64_t k = 0; k < int64_t(shape.size()); ++k) {
        DASSERT_EQ(n % shape[k], 0);
        n /= shape[k];

        auto r = std::div(remainder_idx, n);
        ss << r.quot;
        remainder_idx = r.rem;

        if (size_t(k) + 1 != shape.size()) {
          ss << ',';
        } else {
          DASSERT_EQ(n, 1);
        }
      }

      ss << "]";

      return ss.str();
    }
  }
  // Should be unreachable
  ASSERT_UNREACHABLE();
}

/**
 * Returns a list of all the feature names present in the metadata.
 *
 * Numeric columns are represented by the column name.
 *
 * Categorical / Categorical List / Dictionary columns are represented by
 * "name[category]".
 *
 * Vectors are represented by "vector[index]", where index is numerical.
 *
 * ND vectors are represented by "nd_vector[idx1,idx2]" etc.
 *
 * If unpack_categorical_columns is false, then purely categorical columns (not
 * lists or dictionaries) are called out only by their column name instead of
 * their categories.
 *
 * \returns Names of features
 */
std::vector<std::string> ml_metadata::feature_names(bool unpack_categorical_columns) const {

  std::vector<std::string> feature_names;
  feature_names.reserve(num_dimensions());

  for(size_t i = 0; i < num_columns(); ++i) {
    if (column_mode(i) == ml_column_mode::CATEGORICAL &&
        !unpack_categorical_columns) {
      feature_names.push_back(column_name(i));
    } else {
      for (size_t j = 0; j < index_size(i); ++j) {
        feature_names.push_back(feature_name(i, j));
      }
    }
  }

  return feature_names;
}

void ml_metadata::set_training_index_sizes_to_current_column_sizes() {

  for(size_t c_idx = 0; c_idx < num_columns(); ++c_idx)
    get_column_metadata(c_idx)->set_training_index_size();

  if(has_target()) {
    target->set_training_index_size();
  }

  // Set the global index offsets; the last bit of the ride.
  size_t cum_sum = 0;
  for(size_t c_idx = 0; c_idx < num_columns(); ++c_idx) {
    auto cm = get_column_metadata(c_idx);
    cm->set_training_index_offset(cum_sum);
    cum_sum += cm->index_size();
  }

  setup_cached_values();
}

/** Some of the data statistics are cached.  This function computes
 *  these, making it possible to use nearly all the metadata functions
 *  in the inner loop of something with no concerns about speed.
 */
void ml_metadata::setup_cached_values() {

  ////////////////////////////////////////
  // The number of untranslated columns.
  _num_untranslated_columns = 0;

  for(size_t i = 0; i < columns.size(); ++i)
    if(is_untranslated_column(i))
      ++_num_untranslated_columns;

  ////////////////////////////////////////
  // The total number of dimensions present

  _num_dimensions = 0;

  for(size_t c_idx = 0; c_idx < num_columns(); ++c_idx)
    _num_dimensions += get_column_metadata(c_idx)->index_size();

  ////////////////////////////////////////
  // The map of column names to indices

  _column_name_to_index_map.clear();

  for(size_t c_idx = 0; c_idx < num_columns(); ++c_idx) {
    _column_name_to_index_map[column_name(c_idx)] = c_idx;
  }

  ////////////////////////////////////////
  // Build the row metadata objects

  cached_rm_without_target.setup(columns, false);

  if(has_target()) {
    std::vector<ml_data_internal::column_metadata_ptr> full_columns_with_target;
    full_columns_with_target.reserve(columns.size() + 1);
    full_columns_with_target.assign(columns.begin(), columns.end());
    full_columns_with_target.push_back(target);

    cached_rm_with_target.setup(full_columns_with_target, true);
  } else {
    cached_rm_with_target = cached_rm_without_target;
  }
}

/** Serialization -- save.
 */
void ml_metadata::save(turi::oarchive& oarc) const {

  oarc << get_version();

  std::map<std::string, variant_type> data;

  data["original_column_names"] = to_variant(original_column_names);

  variant_deep_save(data, oarc);

  oarc << columns << target;
}

/** Serialization -- load.
 */
void ml_metadata::load(turi::iarchive& iarc) {

  size_t version = 0;

  iarc >> version;

  std::map<std::string, variant_type> data;
  variant_deep_load(data, iarc);

#define __EXTRACT(var) var = variant_get_value<decltype(var)>(data.at(#var));

  __EXTRACT(original_column_names);

#undef __EXTRACT

  iarc >> columns >> target;

  if(version == 2) {
    bool junk_from_old_side_info;
    iarc >> junk_from_old_side_info;
  }

  ////////////////////////////////////////////////////////////
  // Set the global index offsets.  Annoying to do it here, but need
  // to for backwards compatibility of model serialization.  If the
  // individual global offsets are size_t(-1), which is what happens
  // when the model is loaded, this sets them to the correct value.

  size_t cum_sum = 0;
  for(size_t c_idx = 0; c_idx < num_columns(); ++c_idx) {
    auto cm = get_column_metadata(c_idx);
    cm->set_training_index_offset(cum_sum);
    cum_sum += cm->index_size();
  }

  // Finalize by setting up all the cached values now that everything
  // is present.
  setup_cached_values();
}

#ifndef NDEBUG
void ml_metadata::_debug_is_equal(const std::shared_ptr<ml_metadata>& m) const {
  DASSERT_EQ(columns.size(), m->columns.size());

  for(size_t i = 0; i < columns.size(); ++i) {
    columns[i]->_debug_is_equal(*(m->columns[i]));
  }

  if(target != nullptr) {
    DASSERT_TRUE(m->target != nullptr);
    target->_debug_is_equal(*(m->target));
  } else {
    DASSERT_TRUE(m->target == nullptr);
  }

  DASSERT_TRUE(original_column_names == m->original_column_names);
  DASSERT_EQ(_num_dimensions, m->_num_dimensions);
  DASSERT_EQ(_num_untranslated_columns, m->_num_untranslated_columns);
  DASSERT_TRUE(_column_name_to_index_map == m->_column_name_to_index_map);
}
#endif

}

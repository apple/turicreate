/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/toolkits/ml_data_2/metadata.hpp>
#include <unity/lib/variant_deep_serialize.hpp>

namespace turi { namespace v2 {


/** Returns a vector giving the column names of all columns present.
 */
std::vector<std::string> ml_metadata::column_names(bool include_side_columns_if_present) const {

  size_t n_columns = include_side_columns_if_present ? num_columns() : columns.size();

  std::vector<std::string> column_names(n_columns);

  for(size_t c_idx = 0; c_idx < n_columns; ++c_idx)
    column_names[c_idx] = column_name(c_idx);

  return column_names;
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
   * \returns Names of features
   */
std::string ml_metadata::feature_name(size_t column_idx, size_t index) const {
  const std::string& name = column_name(column_idx);

  switch (column_mode(column_idx)) {
    case ml_column_mode::NUMERIC:
    case ml_column_mode::UNTRANSLATED:
      DASSERT_EQ(index, 0);
      return name;

    case ml_column_mode::CATEGORICAL:
    case ml_column_mode::DICTIONARY:
    case ml_column_mode::CATEGORICAL_VECTOR:
      return name + "[" +
             (indexer(column_idx)
                  ->map_index_to_value(index)
                  .template to<std::string>()) +
             "]";

    case ml_column_mode::NUMERIC_VECTOR:
      DASSERT_LT(index, column_size(column_idx));
      return name + "[" + std::to_string(index) + "]";
    default:
      return name; 
  }
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
}

/** Serialization -- save.
 */
void ml_metadata::save(turi::oarchive& oarc) const {

  oarc << get_version();

  std::map<std::string, variant_type> data;

  data["original_column_names"] = to_variant(original_column_names);
  data["options"]               = to_variant(options);

  variant_deep_save(data, oarc);

  oarc << columns << target;

  // Finally, save the side data
  if(side_features != nullptr) {
    oarc << bool(true);
    side_features->save_without_metadata(oarc);
  } else {
    oarc << bool(false);
  }
}

/** Serialization -- load.
 */
void ml_metadata::load(turi::iarchive& iarc) {

  size_t version = 0;

  iarc >> version;

  ASSERT_EQ(version, 2);

  std::map<std::string, variant_type> data;

  variant_deep_load(data, iarc);

#define __EXTRACT(var) var = variant_get_value<decltype(var)>(data.at(#var));

  __EXTRACT(original_column_names);
  __EXTRACT(options);

#undef __EXTRACT

  iarc >> columns >> target;

  ////////////////////////////////////////////////////////////////////////////////
  // Now load the side features
  bool _has_side_features;
  iarc >> _has_side_features;

  if(_has_side_features) {
    side_features.reset(new ml_data_side_features(columns));

    side_features->load_with_metadata_present(iarc);
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

/** Create a new metadata object that shares the same indexing as
 *  the previous one, but has possibly different and possibly
 *  subsetted columns.
 */
std::shared_ptr<ml_metadata> ml_metadata::select_columns(
    const std::vector<std::string>& new_columns, bool include_target) const {

  if(std::set<std::string>(new_columns.begin(), new_columns.end()).size() != new_columns.size()) {
    ASSERT_MSG(false, "Duplicates in the column selection not allowed.");
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1.  Deal with the columns.

  // Go through and copy over all the individual column_metadata pointers.
  auto m = std::make_shared<ml_metadata>();

  m->columns.resize(new_columns.size());

  for(size_t i = 0; i < new_columns.size(); ++i) {
    const std::string& c = new_columns[i];

      // Find that column
    size_t column_idx = size_t(-1);
    for(size_t j = 0; j < columns.size(); ++j) {
      if(columns[j]->name == c) {
        column_idx = j;
        break;
      }
    }

    if(column_idx == size_t(-1))
      ASSERT_MSG(false, (std::string("Column ") + new_columns[i] + " not found.").c_str());

    m->columns[i] = columns[column_idx];
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 2.  Deal with the target

  if(include_target)
    m->target = target;

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3: Deal with the original_column_names.  This may be in a
  // different order than the columns above (e.g. user and items are
  // moved to index 0 and 1 in the recommender).  Here, we just choose
  // the subset based on the previous columns.

  m->original_column_names.clear();
  m->original_column_names.reserve(new_columns.size());

  for(size_t i = 0; i < original_column_names.size(); ++i) {
    const std::string& c = original_column_names[i];

    // See if that column is in the new set
    size_t column_idx = size_t(-1);
    for(size_t j = 0; j < new_columns.size(); ++j) {
      if(new_columns[j] == c) {
        column_idx = j;
        break;
      }
    }
    if(column_idx != size_t(-1))
      m->original_column_names.push_back(c);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 4: deal with the side data

  if(side_features != nullptr)
    m->side_features = side_features->copy_with_new_main_columns(m->columns);

  ////////////////////////////////////////////////////////////////////////////////
  // Step 5: Other details.

  m->options = options;

  // Set the cached values 
  m->setup_cached_values(); 

  return m;
}



}}

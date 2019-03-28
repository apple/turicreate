/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/toolkits/ml_data_2/indexing/column_unique_indexer.hpp>
#include <serialization/serialization_includes.hpp>
#include <sframe/sframe.hpp>
#include <unity/lib/variant.hpp>
#include <unity/lib/variant_deep_serialize.hpp>

namespace turi { namespace v2 { namespace ml_data_internal {

column_unique_indexer::column_unique_indexer()
{}

/** Initialize the index mapping and setup.  There are certain
 *  internal parallel things that need to be set up before
 *  map_value_to_index works.  Call this before looping over
 *  map_value_to_index, then call finish_indexing() when done.
 */
void column_unique_indexer::initialize() {
  index_modification_lock.lock();

  DASSERT_TRUE(values_by_index_threadlocal_accumulator.empty());

  // Init the lookup tables.
  index_by_values_lookup.resize(
      1 << _column_unique_indexer_first_level_lookup_size_n_bits);

  size_t num_threads = thread::cpu_count();

  // Initialize the value trackers
  values_by_index_threadlocal_accumulator.resize(num_threads);

  for(auto& v : values_by_index_threadlocal_accumulator)
    v.clear();
}


/** Call this when all calls to map_value_to_index are completed.
 */
void column_unique_indexer::finalize() {

  DASSERT_FALSE(values_by_index_threadlocal_accumulator.empty());

  values_by_index_lookup.resize(_column_size);

  // Copy all the flexible type values over to the main
  // values_by_index lookup.
  for(const auto& vv : values_by_index_threadlocal_accumulator) {
    for(const auto& p : vv) {
      values_by_index_lookup[p.first] = std::move(p.second);
    }
  }

  values_by_index_threadlocal_accumulator.clear();
  index_modification_lock.unlock();
}

/** Returns the index associated with the "feature" value.
 *
 * \note Only used if is_categorical is true.
 *
 * If the value in the feature column was already seen, then the index
 * already associated with that value is returned.  If not, a new unique
 * index is added and associated with this feature value.
 *
 * This method is completely threadsafe and is meant to be called by
 * multiple threads in contention.
 *
 * \param[in] feature  The value in the feature column to map to the index.
 * \return An index (possibly new) associated with the given value.
 */
size_t column_unique_indexer::map_value_to_index(size_t thread_idx, const flexible_type& feature) {
  DASSERT_TRUE(
      mode == ml_column_mode::CATEGORICAL
      || mode == ml_column_mode::CATEGORICAL_VECTOR
      || mode == ml_column_mode::DICTIONARY);

  DASSERT_FALSE(values_by_index_threadlocal_accumulator.empty());
  DASSERT_LT(thread_idx, values_by_index_threadlocal_accumulator.size());

  // Check value
  if( ! (feature.get_type() == flex_type_enum::STRING
         || feature.get_type() == flex_type_enum::INTEGER
         || feature.get_type() == flex_type_enum::UNDEFINED) ) {

    log_and_throw(std::string("Value encountered in column '")
                  + column_name + "' is of type '"
                  + flex_type_enum_to_name(feature.get_type()) +
                  "' cannot be mapped to a categorical value." +
                  " Categorical values must be integer, strings, or None.");
  }

  hash_value wt(feature);

  // Lock the first level
  size_t first_index = wt.n_bit_index(_column_unique_indexer_first_level_lookup_size_n_bits);
  DASSERT_LT(first_index, index_by_values_lookup.size());
  auto& lock_ht_pair = index_by_values_lookup[first_index];

  lock_ht_pair.first.lock();
  auto it = lock_ht_pair.second.find(wt);

  size_t index;

  if(it == lock_ht_pair.second.end()) {
    index = (++_column_size) - 1;
    values_by_index_threadlocal_accumulator[thread_idx].push_back({index, feature});
    lock_ht_pair.second[wt] = index;
  } else {
    index = it->second;
  }

  lock_ht_pair.first.unlock();

  return index;
}

/** Returns the index associated with the "feature" value.
 *
 * \note Only used if is_categorical is true.
 *
 * If the value in the feature column was already seen, then the index
 * already associated with that value is returned.  If not, it returns
 * size_t(-1).
 *
 * \param[in] feature  The value in the feature column to map to the index.
 *
 * \return An index associated with the given value. If the index is not
 * present. We return size_t(-1).
 */
size_t column_unique_indexer::immutable_map_value_to_index(
    const flexible_type& feature) const {

  DASSERT_TRUE(
      mode == ml_column_mode::CATEGORICAL
      || mode == ml_column_mode::CATEGORICAL_VECTOR
      || mode == ml_column_mode::DICTIONARY);

  // Check value
  if( ! (feature.get_type() == flex_type_enum::STRING
         || feature.get_type() == flex_type_enum::INTEGER
         || feature.get_type() == flex_type_enum::UNDEFINED) ) {

    log_and_throw(std::string("Value encountered in column '")
                  + column_name + "' is of type '"
                  + flex_type_enum_to_name(feature.get_type()) +
                  "' cannot be mapped to a categorical value." +
                  " Categorical values must be strings, or None.");
  }

  hash_value wt(feature);

  // Lock the first level
  size_t first_index = wt.n_bit_index(_column_unique_indexer_first_level_lookup_size_n_bits);
  DASSERT_LT(first_index, index_by_values_lookup.size());
  auto& lock_ht_pair = index_by_values_lookup[first_index];

  auto it = lock_ht_pair.second.find(wt);

  if(it == lock_ht_pair.second.end()) {

    // Value not found.
    return (size_t)(-1);

  } else {

    // Value found. Returning the index.
    return it->second;
  }
}

/**
 * Some of the ml_data tests currently depend on the order of
 * insertion into the index, which is now done in parallel and thus
 * not deterministic.  This function allows the user to remove that
 * randomness by inserting all indices in a specified order.  It must
 * be called from only one thread.
 *
 * \note Missing values will be ignored.
 */
void column_unique_indexer::insert_values_into_index(
    const std::vector<flexible_type>& fv){

  switch(mode) {
    case ml_column_mode::NUMERIC:
      return;

    case ml_column_mode::CATEGORICAL:
      for(const flexible_type& feature : fv)
        map_value_to_index(0, feature);
      return;

    case ml_column_mode::CATEGORICAL_VECTOR:
      {
        for(const flexible_type& feature : fv) {
          if (feature.get_type() == flex_type_enum::UNDEFINED)
            continue;
          const flex_list& vv = feature.get<flex_list>();
          for(const auto& v : vv)
            map_value_to_index(0, v);
        }
      }
      return;

    case ml_column_mode::NUMERIC_VECTOR:
      return;

    case ml_column_mode::DICTIONARY: {
      for(const flexible_type& feature : fv) {
        if (feature.get_type() == flex_type_enum::UNDEFINED)
          continue;
        if (feature.get_type() == flex_type_enum::DICT) {
          for(const auto& kv : feature.get<flex_dict>())
            map_value_to_index(0, kv.first);
        } else {
          map_value_to_index(0, feature); 
        }
      }
      return;
    }
      
    case ml_column_mode::UNTRANSLATED:
      return;
  }
}

/** Returns the feature "value" associated an index.
 *
 * \note Only used if is_categorical is true.
 *
 * \param[\in] idx  Index associated with the feature value.
 * \return The "value" in the original data associated with the given id.
 */
flexible_type column_unique_indexer::map_index_to_value(size_t idx) const {

  DASSERT_TRUE(mode == ml_column_mode::CATEGORICAL
               || mode == ml_column_mode::CATEGORICAL_VECTOR
               || mode == ml_column_mode::DICTIONARY);

  DASSERT_MSG(idx != size_t(-1),
              "Index not tracked in metadata table!");

  DASSERT_MSG(idx < values_by_index_lookup.size(),
              "Index not in metadata table; using correct metadata?");

  flexible_type value = values_by_index_lookup[idx];
  return value;
}

////////////////////////////////////////////////////////////////////////////////
//
std::set<flex_type_enum> column_unique_indexer::extract_key_types() const {

  if(values_by_index_lookup.size() == 0)
    return std::set<flex_type_enum>{};

  flex_type_enum primary_deduced_type = values_by_index_lookup[0].get_type();

  std::set<flex_type_enum> values_present{primary_deduced_type};

  for(const flexible_type& v : values_by_index_lookup) {
    if(v.get_type() != primary_deduced_type)
      values_present.insert(v.get_type());
  }

  return values_present;
}

////////////////////////////////////////////////////////////////////////////////
// Serialization routines

size_t column_unique_indexer::get_version() const {
  return 1;
}

/**
 * Save metadata.
 */
void column_unique_indexer::save_impl(turi::oarchive& oarc) const {

  std::map<std::string, variant_type> data;

  data["values_by_index_lookup"] = to_variant(values_by_index_lookup);
  data["column_size"] = to_variant(size_t(_column_size));

  variant_deep_save(to_variant(data), oarc);
}

/**
 * Load metadata.
 */
void column_unique_indexer::load_version(turi::iarchive& iarc, size_t version) {

  ASSERT_TRUE(version == 1);

  variant_type data_v;
  variant_deep_load(data_v, iarc);

  std::map<std::string, variant_type> data;
  data = variant_get_value<decltype(data)>(data_v);

  set_values(variant_get_value<std::vector<flexible_type> >(
          data["values_by_index_lookup"]));

  _column_size = variant_get_value<size_t>(data["column_size"]);
}

/** Returns a lambda function that can be used as a lambda function for indexing
 *  a column.
 *
 *  Does not add any new index values.
 */
std::function<flexible_type(const flexible_type&)> column_unique_indexer::deindexing_lambda() const {
  return [this](const flexible_type& v) -> flexible_type {
    DASSERT_EQ(v.get_type(), flex_type_enum::INTEGER);
    return flexible_type(this->map_index_to_value(v.get<flex_int>()));
  };
}

/** Returns a lambda function that can be used as a lambda function for indexing
 *  a column.
 *
 *  Does not add any new index values.
 */
std::function<flexible_type(const flexible_type&)> column_unique_indexer::indexing_lambda() const {
  return [this](const flexible_type& v) -> flexible_type {
    return flexible_type(flex_int(this->immutable_map_value_to_index(v)));
  };
}



// Reset and return all the values in the index.
std::vector<flexible_type> column_unique_indexer::reset_and_return_values() {
  // Clear out the hash indexing.
  index_by_values_lookup.clear();

  std::vector<flexible_type> ret;
  ret.swap(values_by_index_lookup);

  return ret;
}


// Set the values from a prior index.
void column_unique_indexer::set_values(std::vector<flexible_type>&& values) {

  values_by_index_lookup = values;

  // Now, we need to rebuild the index.
  if(mode == ml_column_mode::CATEGORICAL
     || mode == ml_column_mode::CATEGORICAL_VECTOR
     || mode == ml_column_mode::DICTIONARY) {

    // Set the first level of the index_by_values hash lookup
    index_by_values_lookup.clear();
    index_by_values_lookup.resize(1 << _column_unique_indexer_first_level_lookup_size_n_bits);


    // Fill the hash table map with the loaded list of values
    in_parallel([&](size_t thread_idx, size_t num_threads) {
        size_t start_idx = (thread_idx * values_by_index_lookup.size()) / num_threads;
        size_t end_idx = ((thread_idx + 1) * values_by_index_lookup.size()) / num_threads;

        for(size_t i = start_idx; i < end_idx; ++i) {
          hash_value wt(values_by_index_lookup[i]);

          // Lock the first level
          size_t first_index = wt.n_bit_index(_column_unique_indexer_first_level_lookup_size_n_bits);
          DASSERT_LT(first_index, index_by_values_lookup.size());
          auto& lock_ht_pair = index_by_values_lookup[first_index];

          lock_ht_pair.first.lock();
          lock_ht_pair.second[wt] = i;
          lock_ht_pair.first.unlock();
        }
      });
  }
}

  /** Create a copy with the index cleared.
   */
  std::shared_ptr<column_indexer> column_unique_indexer::create_cleared_copy() const {
    auto ret = std::make_shared<column_unique_indexer>(*this); 
    ret->set_values({});
    return ret; 
  } 
 
}}}

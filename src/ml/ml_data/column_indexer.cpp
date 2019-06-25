/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <ml/ml_data/column_indexer.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <model_server/lib/variant.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>

namespace turi { namespace ml_data_internal {

/**
 *  Default constructor; does nothing;
 *  Initialize from a serialization stream
 */
column_indexer::column_indexer(std::string _column_name,
                 ml_column_mode _mode,
                 flex_type_enum _original_column_type)
    : column_name(_column_name)
    , mode(_mode)
    , original_column_type(_original_column_type)
{}

/** Initialize the index mapping and setup.  There are certain
 *  internal parallel things that need to be set up before
 *  map_value_to_index works.  Call this before looping over
 *  map_value_to_index, then call finish_indexing() when done.
 */
void column_indexer::initialize() {
  index_modification_lock.lock();

  DASSERT_TRUE(values_by_index_threadlocal_accumulator.empty());

  // Init the lookup tables.
  index_by_values_lookup.resize(
      1 << _column_indexer_first_level_lookup_size_n_bits);

  size_t num_threads = thread::cpu_count();

  // Initialize the value trackers
  values_by_index_threadlocal_accumulator.resize(num_threads);

  for(auto& v : values_by_index_threadlocal_accumulator)
    v.clear();
}


/** Call this when all calls to map_value_to_index are completed.
 */
void column_indexer::finalize() {

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

/**
 * Some of the ml_data tests currently depend on the order of
 * insertion into the index, which is now done in parallel and thus
 * not deterministic.  This function allows the user to remove that
 * randomness by inserting all indices in a specified order.  It must
 * be called from only one thread.
 *
 * \note Missing values will be ignored.
 */
void column_indexer::insert_values_into_index(
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

    default:
      return;
  }
}

////////////////////////////////////////////////////////////////////////////////
//
std::set<flex_type_enum> column_indexer::extract_key_types() const {

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

  /** Purges and returns all the values; The result is an indexer that
   *  contains no values, but metadata like name, mode, and type are
   *  preserved.
   */

std::vector<flexible_type> column_indexer::reset_and_return_values() {
  std::lock_guard<mutex> lg(index_modification_lock);
  index_by_values_lookup.clear();
  values_by_index_threadlocal_accumulator.clear();
  _column_size = 0;

  return std::move(values_by_index_lookup);
}

void column_indexer::set_indices(std::vector<flexible_type>&& values) {

  if(values.empty()) {
    values_by_index_lookup.clear();
    return;
  }

  ASSERT_TRUE(mode == ml_column_mode::CATEGORICAL
              || mode == ml_column_mode::CATEGORICAL_VECTOR
              || mode == ml_column_mode::DICTIONARY);

  values_by_index_lookup = std::move(values);
  _column_size = values_by_index_lookup.size();

  // Now, we need to rebuild the index.

  // Set the first level of the index_by_values hash lookup
  index_by_values_lookup.resize(1 << _column_indexer_first_level_lookup_size_n_bits);

  // Fill the hash table map with the loaded list of values
  in_parallel([&](size_t thread_idx, size_t num_threads) {
      size_t start_idx = (thread_idx * values_by_index_lookup.size()) / num_threads;
      size_t end_idx = ((thread_idx + 1) * values_by_index_lookup.size()) / num_threads;

      for(size_t i = start_idx; i < end_idx; ++i) {
        hash_value wt(values_by_index_lookup[i]);

        // Lock the first level
        size_t first_index = wt.n_bit_index(_column_indexer_first_level_lookup_size_n_bits);
        DASSERT_LT(first_index, index_by_values_lookup.size());
        auto& lock_ht_pair = index_by_values_lookup[first_index];

        {
          std::lock_guard<simple_spinlock> wg(lock_ht_pair.first);
          lock_ht_pair.second[wt] = i;
        }
      }
    });
}

void column_indexer::debug_check_is_internally_consistent() const {
#ifndef NDEBUG
  ASSERT_EQ(_column_size, values_by_index_lookup.size());

  for(size_t i = 0; i < values_by_index_lookup.size(); ++i) {
    size_t index = immutable_map_value_to_index(values_by_index_lookup[i]);
    ASSERT_EQ(index, i);
  }
#endif
}


  /** Checks that the indices are equal across machines.
   */
void column_indexer::debug_check_is_equal(std::shared_ptr<column_indexer> other) const {
#ifndef NDEBUG
  debug_check_is_internally_consistent();
  other->debug_check_is_internally_consistent();

  ASSERT_TRUE(mode == other->mode);
  ASSERT_TRUE(column_name == other->column_name);
  ASSERT_TRUE(original_column_type == other->original_column_type);
  ASSERT_TRUE(_column_size == other->_column_size);

  for(size_t i = 0; i < values_by_index_lookup.size(); ++i) {
    DASSERT_TRUE(values_by_index_lookup[i] == other->values_by_index_lookup[i]);
  }
#endif
}


////////////////////////////////////////////////////////////////////////////////
// Serialization routines

size_t column_indexer::get_version() const {
  return 2;
}

/**
 * Save metadata.
 */
void column_indexer::save_impl(turi::oarchive& oarc) const {
  oarc << column_name << mode << original_column_type;
  oarc << values_by_index_lookup << _column_size;
}

/**
 * Load metadata.
 */
void column_indexer::load_version(turi::iarchive& iarc, size_t version) {

  if(version == 2) {

    std::vector<flexible_type> values;

    iarc >> column_name >> mode >> original_column_type;
    iarc >> values >> _column_size;

    if(mode == ml_column_mode::CATEGORICAL
       || mode == ml_column_mode::CATEGORICAL_VECTOR
       || mode == ml_column_mode::DICTIONARY) {

      ASSERT_EQ(_column_size, values.size());

      set_indices(std::move(values));
    }
  } else if(version == 1) {

    // First load the creation options
    std::map<std::string, variant_type> creation_options;
    variant_deep_load(creation_options, iarc);

    std::string indexer_type = variant_get_value<std::string>(creation_options.at("indexer_type"));

    ASSERT_TRUE(indexer_type == "unique");

#define __EXTRACT(var)                                                  \
    var = variant_get_value<decltype(var)>(creation_options.at(#var));

    __EXTRACT(column_name);
    __EXTRACT(mode);
    __EXTRACT(original_column_type);

#undef __EXTRACT

    variant_type data_v;
    variant_deep_load(data_v, iarc);

    std::map<std::string, variant_type> data;
    data = variant_get_value<decltype(data)>(data_v);

    std::vector<flexible_type> values;

    values = variant_get_value<decltype(values)>(data["values_by_index_lookup"]);

    _column_size = variant_get_value<size_t>(data["column_size"]);

    set_indices(std::move(values));
  } else {
    log_and_throw("Incompatable versioning here.");
  }
}

}}

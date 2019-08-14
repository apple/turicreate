/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/serialization/serialization_includes.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <model_server/lib/variant.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>

#include <toolkits/feature_engineering/statistics_tracker.hpp>

namespace turi {

/**
 * Utility to check the value types.
 */
void statistics_tracker::valdidate_types(const flexible_type & value) const {
  if( ! (value.get_type() == flex_type_enum::INTEGER ||
         value.get_type() == flex_type_enum::FLOAT   ||
         value.get_type() == flex_type_enum::UNDEFINED) ) {

    log_and_throw(std::string("Value encountered in column '")
                  + column_name + "' is of type '"
                  + flex_type_enum_to_name(value.get_type()) +
                  "' cannot have mean calculation." +
                  " Values must be integer, floats, or None.");
  }
}

/**
 * Initialize the index mapping and setup.
 */
void statistics_tracker::initialize() {
  DASSERT_TRUE(threadlocal_accumulator.count.empty());
  DASSERT_TRUE(threadlocal_accumulator.mean.empty());
  DASSERT_TRUE(threadlocal_accumulator.missing.empty());
  DASSERT_TRUE(threadlocal_accumulator.key_index.empty());

  // Initialize the count trackers
  size_t num_threads = thread::cpu_count();
  threadlocal_accumulator.count.resize(num_threads);
  threadlocal_accumulator.mean.resize(num_threads);
  threadlocal_accumulator.missing.resize(num_threads);
  threadlocal_accumulator.key_index.resize(num_threads);
  for(size_t i=0; i<num_threads; i++){
    threadlocal_accumulator.count[i].clear();
    threadlocal_accumulator.mean[i].clear();
    threadlocal_accumulator.missing[i].clear();
    threadlocal_accumulator.key_index[i].clear();
  }
}

/**
 * Call this when all calls to map_value_to_index are completed.
 */
void statistics_tracker::finalize(size_t num_examples) {
  DASSERT_FALSE(threadlocal_accumulator.count.empty());
  DASSERT_FALSE(threadlocal_accumulator.mean.empty());
  DASSERT_FALSE(threadlocal_accumulator.missing.empty());
  DASSERT_FALSE(threadlocal_accumulator.key_index.empty());
  DASSERT_TRUE(counts.empty());
  DASSERT_TRUE(means.empty());
  DASSERT_TRUE(missing.empty());
  DASSERT_TRUE(keys.empty());

  // Copy all the flexible type values over to the main
  // values_by_index lookup.
  size_t global_index = 0;
  for(size_t i=0; i<thread::cpu_count(); i++ ) {
    auto& count_hash_table = threadlocal_accumulator.count[i];
    auto& mean_hash_table = threadlocal_accumulator.mean[i];
    auto& missing_hash_table = threadlocal_accumulator.missing[i];
    auto& key_hash_table = threadlocal_accumulator.key_index[i];
    for(auto& kvp : key_hash_table) {
      auto it = index_lookup.find(kvp.first);
      if (it == index_lookup.end()) {
        index_lookup[kvp.first] = global_index;
        keys.push_back(key_hash_table[kvp.first]);
        counts.push_back(count_hash_table[kvp.first]);
        means.push_back(mean_hash_table[kvp.first]);
        missing.push_back(missing_hash_table[kvp.first]);
        global_index++;
      } else {
        means[it->second] = means[it->second] * ((flex_float)counts[it->second]/(counts[it->second] + count_hash_table[kvp.first]))
         + mean_hash_table[kvp.first] * ((flex_float)count_hash_table[kvp.first]/(counts[it->second] + count_hash_table[kvp.first]));
        counts[it->second] += count_hash_table[kvp.first];
        missing[it->second] += missing_hash_table[kvp.first];
      }
    }
  }
  // Merge implicit 0's into mean and count
  for(const auto& k : keys) {
    auto it = index_lookup.find(k);
    size_t num_implicit_zeros = num_examples - missing[it->second] - counts[it->second];
    if (missing[it->second] == num_examples){
      log_and_throw("At least one feature in " + column_name + " is all None's."
                    " There must be at least one non-None value for mean imputation.");
    }
    means[it->second] = means[it->second] * ((flex_float)counts[it->second]/(counts[it->second] + num_implicit_zeros));
    counts[it->second] += num_implicit_zeros;
  }
}


/**
 * Returns the index associated with the "value" value.
 *
 */
void statistics_tracker::insert_or_update(const flexible_type& key,
                            flexible_type value, size_t thread_idx) {
  DASSERT_FALSE(threadlocal_accumulator.count.empty());
  DASSERT_FALSE(threadlocal_accumulator.mean.empty());
  DASSERT_FALSE(threadlocal_accumulator.missing.empty());
  DASSERT_FALSE(threadlocal_accumulator.key_index.empty());
  DASSERT_LT(thread_idx, threadlocal_accumulator.mean.size());
  DASSERT_LT(thread_idx, threadlocal_accumulator.count.size());
  DASSERT_LT(thread_idx, threadlocal_accumulator.missing.size());
  DASSERT_LT(thread_idx, threadlocal_accumulator.key_index.size());



  // Check types.
  valdidate_types(value);

  // Get the hash_table
  const  hash_value wt(key);
  auto& count_hash_table = threadlocal_accumulator.count[thread_idx];
  auto& mean_hash_table = threadlocal_accumulator.mean[thread_idx];
  auto& missing_hash_table = threadlocal_accumulator.missing[thread_idx];
  auto& key_hash_table = threadlocal_accumulator.key_index[thread_idx];

  // Search
  auto it = key_hash_table.find(wt);
  if (value.get_type() != flex_type_enum::UNDEFINED){
    if(it == key_hash_table.end()) {
      key_hash_table[wt] = key;
      count_hash_table[wt] = 1;
      mean_hash_table[wt] = value;
    } else {
      count_hash_table[wt] ++;
      mean_hash_table[wt] += ((flex_float)value - mean_hash_table[wt])/(count_hash_table[wt]);
    }
  } else {
    if(it == key_hash_table.end()) {
      key_hash_table[wt] = key;
      missing_hash_table[wt] = 1;
    } else {
      missing_hash_table[wt] ++;
    }

  }
}

/**
 * Returns the index associated with the value.
 */
size_t statistics_tracker::lookup(const flexible_type& value) const {

  // Get the hash_table
  hash_value wt(value);
  auto it = index_lookup.find(wt);
  if(it == index_lookup.end()) {
    return (size_t)-1;
  } else {
    return it->second;
  }
}

/**
 * Returns the counts associated with the value.
 */
size_t statistics_tracker::lookup_counts(const flexible_type& value) const {


  // Get the hash_table
  hash_value wt(value);
  auto it = index_lookup.find(wt);
  if(it == index_lookup.end()) {
    return 0;
  } else {
    return counts[(it->second)];
  }
}

/**
 * Returns the means associated with the value.
 */
flex_float statistics_tracker::lookup_means(const flexible_type& value) const {


  // Get the hash_table
  hash_value wt(value);
  auto it = index_lookup.find(wt);
  if(it == index_lookup.end()) {
    log_and_throw("No mean associated with this value");
  } else {
    return means[(it->second)];
  }
}

/**
 * Returns the value "value" associated an index.
 */
flexible_type statistics_tracker::inverse_lookup(size_t idx) const {
  DASSERT_TRUE(idx != size_t(-1));
  DASSERT_TRUE(idx < keys.size());
  return keys.at(idx);
}


// Serialization routines
size_t statistics_tracker::get_version() const {
  return 1;
}

/**
 * Save metadata.
 */
void statistics_tracker::save_impl(turi::oarchive& oarc) const {
  oarc << keys
       << counts
       << means
       << missing
       << column_name;
}

/**
 * Load metadata.
 */
void statistics_tracker::load_version(turi::iarchive& iarc, size_t version) {
  ASSERT_TRUE(version == 1);
  iarc >> keys
       >> counts
       >> means
       >> missing
       >> column_name;

  // Rebuild the hash-table.
  size_t index = 0;
  for(auto& k : keys) {
    hash_value wt(k);
    index_lookup[wt] = index;
    index++;
  }
}

} // Turi

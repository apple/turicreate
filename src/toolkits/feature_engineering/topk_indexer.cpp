/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/serialization/serialization_includes.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <model_server/lib/variant.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>

#include <toolkits/feature_engineering/topk_indexer.hpp>

namespace turi {

/**
 * Utility to check the value types.
 */
void topk_indexer::valdidate_types(const flexible_type & value) const {
  if( ! (value.get_type() == flex_type_enum::STRING
         || value.get_type() == flex_type_enum::INTEGER
         || value.get_type() == flex_type_enum::UNDEFINED) ) {

    log_and_throw(std::string("Value encountered in column '")
                  + column_name + "' is of type '"
                  + flex_type_enum_to_name(value.get_type()) +
                  "' cannot be mapped to a categorical value." +
                  " Categorical values must be integer, strings, or None.");
  }
}

/**
 * Initialize the index mapping and setup.
 */
void topk_indexer::initialize() {
  DASSERT_TRUE(threadlocal_accumulator.empty());

  // Initialize the count trackers
  size_t num_threads = thread::cpu_count();
  threadlocal_accumulator.resize(num_threads);
  for(auto& v : threadlocal_accumulator){
    v.clear();
  }
}

/**
 * Call this when all calls to map_value_to_index are completed.
 */
void topk_indexer::finalize() {
  DASSERT_FALSE(threadlocal_accumulator.empty());
  DASSERT_TRUE(values.empty());
  DASSERT_TRUE(counts.empty());

  // Copy all the flexible type values over to the main
  // values_by_index lookup.
  size_t global_index = 0;
  for(const auto& hash_table : threadlocal_accumulator) {
    for(const auto& kvp : hash_table) {
      auto it = index_lookup.find(kvp.first);
      if (it == index_lookup.end()) {
        index_lookup[kvp.first] = global_index;
        values.push_back(kvp.second.first);
        counts.push_back(kvp.second.second);
        global_index++;
      } else {
        counts[it->second] += kvp.second.second;
      }
    }
  }

  retain_only_top_k_values();
  retain_min_count_values();
  delete_min_count_values();
  delete_all_marked();

}


/**
 * Returns the index associated with the "value" value.
 *
 */
void topk_indexer::insert_or_update(const flexible_type& value,
                            size_t thread_idx, size_t count) {
  DASSERT_FALSE(threadlocal_accumulator.empty());
  DASSERT_LT(thread_idx, threadlocal_accumulator.size());

  // Check types.
  valdidate_types(value);

  // Get the hash_table
  hash_value wt(value);
  auto& hash_table = threadlocal_accumulator[thread_idx];

  // Search
  auto it = hash_table.find(wt);
  if(it == hash_table.end()) {
    hash_table[wt] = std::make_pair(value, count);
  } else {
    (it->second.second)++;
  }
}

/**
 * Returns the index associated with the value.
 */
size_t topk_indexer::lookup(const flexible_type& value) const {
  // Check types.
  valdidate_types(value);

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
 * Returns the index associated with the value.
 */
size_t topk_indexer::lookup_counts(const flexible_type& value) const {

  // Check types.
  valdidate_types(value);

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
 * Returns the value "value" associated an index.
 */
flexible_type topk_indexer::inverse_lookup(size_t idx) const {
  DASSERT_TRUE(idx != size_t(-1));
  DASSERT_TRUE(idx < values.size());
  return values.at(idx);
}

/**
 * Delete by index.
 */
void topk_indexer::mark_for_deletion(size_t index) {
  DASSERT_TRUE(index < size());
  counts[index] = 0;
}

/**
 * Delete by index.
 */
void topk_indexer::delete_all_marked() {

  // Make a copy and clear out the old ones.
  // Define struct
  typedef struct cv_pair {
    size_t count;
    flexible_type value;
  } cv_pair;
  std::vector<cv_pair> cv_pair_array;
  // Now add back the ones that are not marked for deletion.
  for(size_t i = 0; i < counts.size(); i++){
    hash_value wt(values[i]);
    if (counts[i] == 0){
      index_lookup.erase(wt);
    } else {
      cv_pair current_pair = {counts[i], values[i]};
      cv_pair_array.push_back(current_pair);
    }
  }
  // Sort the thing
  std::sort(cv_pair_array.begin(),cv_pair_array.end(),
  [&](const cv_pair& a, const cv_pair& b){
    if (a.count == b.count){
      hash_value hash_a(a.value);
      hash_value hash_b(b.value);
      return hash_a < hash_b;
    } else {
      return a.count < b.count;
    }
  });

  counts.clear();
  values.clear();
  size_t global_index = 0;
  // Rebuild global index map
  for (size_t i=0 ; i < cv_pair_array.size(); i++){
    hash_value wt(cv_pair_array[i].value);
    index_lookup[wt] = global_index;
    counts.push_back(cv_pair_array[i].count);
    values.push_back(cv_pair_array[i].value);
    global_index++;
  }
}

/**
 * Retain only top-k.
 */
void topk_indexer::retain_only_top_k_values() {
  DASSERT_TRUE(topk >= 0);
  // k = max(size() - topk, 0)
  size_t botk = topk < size() ? size() - topk : 0;

  // Retain top-k
  if (botk > 0) {

    // Sort the indices
    std::vector<size_t> indices;
    for(size_t i=0; i < values.size(); i++){
      indices.push_back(i);
    }
    std::nth_element(indices.begin(), indices.begin() + botk, indices.end(),
    [&](size_t a, size_t b) {
     if (counts[a] == counts[b]){
       hash_value hash_a(values[a]);
       hash_value hash_b(values[b]);
       return hash_a < hash_b;
     } else {
       return counts[a] < counts[b];
     }
    });
    indices.resize(botk);

    // Remove from all structures.
    for (size_t i = 0; i < indices.size(); i++){
      mark_for_deletion(indices[i]);
    }
  }
}

/**
 * Retain only values with count >= threshold
 */
void topk_indexer::retain_min_count_values() {
  DASSERT_TRUE(threshold >= 0);
  if (threshold > 1) {
    for(size_t i=0; i < size(); i++){
      if (counts[i] < threshold){
        mark_for_deletion(i);
      }
    }
  }
}

void topk_indexer::delete_min_count_values() {
  DASSERT_TRUE(max_threshold >= 0);
  if (threshold < (size_t) -1) {
    for(size_t i=0; i < size(); i++){
      if (counts[i] > max_threshold){
        mark_for_deletion(i);
      }
    }
  }
}


// Serialization routines
size_t topk_indexer::get_version() const {
  return 1;
}

/**
 * Save metadata.
 */
void topk_indexer::save_impl(turi::oarchive& oarc) const {
  oarc << values
       << counts
       << column_name
       << topk
       << threshold
       << max_threshold;
}

/**
 * Load metadata.
 */
void topk_indexer::load_version(turi::iarchive& iarc, size_t version) {
  ASSERT_TRUE(version == 1);
  iarc >> values
       >> counts
       >> column_name
       >> topk
       >> threshold
       >> max_threshold;

  // Rebuild the hash-table.
  for(size_t i = 0; i < values.size(); i++) {
    hash_value wt(values[i]);
    index_lookup[wt] = i;
  }
}

} // Turi

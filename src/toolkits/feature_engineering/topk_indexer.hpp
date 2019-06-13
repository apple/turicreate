/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TOPK_COLUMN_INDEXER_H_
#define TURI_TOPK_COLUMN_INDEXER_H_

#include <core/data/flexible_type/flexible_type.hpp>
#include <core/util/hash_value.hpp>
#include <core/logging/assertions.hpp>
#include <core/util/bitops.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
#include <core/generics/hopscotch_map.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/export.hpp>

namespace turi {

/**
 *
 * Parallel top-k indexer for categorical variables (uses one-hot-encoding)
 *
 * Note: This implementation is intended to be general and will be moved to some
 *       place more general later.
 *
 * Construction
 * -------------
 *
 * // Construct the indexer with the arguments.
 * auto indexer = topk_indexer(10, 1, "column_name_for_error_messages");
 * indexer.initialize();
 *
 * // Insert flexible types into the indexer
 * for (const flexible_type& v: sa.range_iterator() {
 *   indexer.insert_or_update(v);
 * }
 *
 * // Finalize mapping (drops elements by frequency/threshold)
 * indexer.finalize();
 *
 * Lookups
 * --------
 *  size_t index = indexer.lookup(v);        // Returns (size_t) -1 if not present.
 *
 *  size_t counts = indexer.lookup_counts(v); // Returns 0 if not present.
 *
 *  flexible_type v = indexer.inverse_lookup(1) // Fails if index doesn't exist.
 *
 * Parallel construction
 * -----------------------
 *
 * // Initialize
 * indexer.initialize();
 *
 *  // Perform the indexing.
 *  in_parallel([&](size_t thread_idx, size_t num_threads) {
 *
 *    size_t start_idx = src_size * thread_idx / num_threads;
 *    size_t end_idx = src_size * (thread_idx + 1) / num_threads;
 *
 *    for (const flexible_type& v: sa.range_iterator(start_idx, end_idx) {
 *      indexer.insert_or_update(v, thread_id);
 *    }
 *
 * // Finalize
 * indexer.finalize();
 *
 *
 */
class EXPORT topk_indexer {

 public:

  /**
   *  Default constructor
   *
   * \param[in] topk        Topk to retain (by counts)
   * \param[in] threshold   Min count threshold to retain.
   * \param[in] column_name Column name for display.
   *
   */
  topk_indexer(const size_t& _topk = (size_t)-1,
               const size_t& _threshold = 1,
               const size_t& _max_threshold = (size_t) -1,
               const std::string _column_name = "") : topk(_topk),
                    threshold(_threshold), max_threshold(_max_threshold),column_name(_column_name) {
  }


  /**
   * Copy constructor: Don't want to risk making copies of this.
   */
  topk_indexer(const topk_indexer&) = delete;


  /**
   * Initialize the index mapping and setup. Should be called before
   * starting the map.
   */
  void initialize();

  /**
   * Insert
   *
   * \param[in] value       Flexible type.
   * \param[in] thread_idx  Thread id  (For parallel insertion).
   * \param[in] count       Amount to increment for this value.
   *
   */
  void insert_or_update(const flexible_type& value, size_t thread_idx = 0,
                        size_t count = 1) GL_HOT;

  /**
   * Returns the index associated with the value.
   *
   *  \param[in] value Search for the value.
   *  \returns The index. (Returns size_t(-1) if not present).
   */
  size_t lookup(const flexible_type& value) const;

  /**
   * Returns the counts associated with the value.
   *
   *  \param[in] value Search for the value.
   *  \returns Counts (Returns 0 if not present).
   */
  size_t lookup_counts(const flexible_type& value) const;


  /**
   * Finalize by dropping indices that dont meet
   * - Count requirement i.e count >= threshold.
   * - Topk requirement.
   */
  void finalize();

  /**
   * Returns the "value" associated with the index.
   *
   * \param[\in] idx  Index associated with the feature value.
   * \return The "value" in the original data associated with the given id.
   */
  flexible_type inverse_lookup(size_t idx) const;

  /**
   * Returns the values (ordered by indices)
   */
  std::vector<flexible_type> get_values() const {
    return values;
  }


  /** Returns the number of categorical variables.
   *
   * \return Column size.
   */
  inline size_t size() const {
    return index_lookup.size();
  }

  /**
   * Returns the current version used for the serialization.
   */
  size_t get_version() const;

  /**
   *  Serialize the object (save).
   */
  void save_impl(turi::oarchive& oarc) const;

  /**
   *  Load the object.
   */
  void load_version(turi::iarchive& iarc, size_t version);

 private:

  // Private members.
  size_t topk = (size_t) (-1);
  size_t threshold = 0;
  size_t max_threshold = (size_t) (-1);
  std::string column_name = "";

  // List of Map(hash : (value, count)) per thread.
  std::vector<hopscotch_map<hash_value, std::pair<flexible_type, size_t>>>
                                                    threadlocal_accumulator;

  // Index -> value/cound
  std::vector<flexible_type> values;
  std::vector<size_t> counts;

  // Map(value : index)
  hopscotch_map<hash_value, size_t> index_lookup;

  // Private helper functions.
  // ------------------------------------------------------------------------

  /**
   * Retain only top_k values (by counts). Only call after finalize.
   */
  void retain_only_top_k_values();

  /**
   * Retain values with counts >= threshold. Only call after finalize.
   */
  void retain_min_count_values();

  /**
   * Retain values with count <= max_threshold. Only call after finalize.
   */
  void delete_min_count_values();
  /**
   * Delete everything associated with an index in the lookup.
   */
  void mark_for_deletion(size_t index);
  /**
   * Delete everything associated with an index in the lookup.
   */
  void delete_all_marked();

  /**
   * Validate feature types.
   */
  void valdidate_types(const flexible_type& value) const;
};
} // turicreate


// Implement serialization for std::shared_ptr
BEGIN_OUT_OF_PLACE_SAVE(arc, std::shared_ptr<topk_indexer>, m) {
  if(m == nullptr) {
    arc << false;
  } else {
    arc << true;

    // Save the version number
    size_t version = m->get_version();
    arc << version;
    // Save the object.
    m->save_impl(arc);
  }
} END_OUT_OF_PLACE_SAVE()


// Implement deserialization
BEGIN_OUT_OF_PLACE_LOAD(arc, std::shared_ptr<topk_indexer>, m) {
  bool is_not_nullptr;
  arc >> is_not_nullptr;
  if(is_not_nullptr) {

    // Load version
    size_t version;
    arc >> version;

    // Load object.
    m.reset(new topk_indexer(0,1,0,""));
    m->load_version(arc, version);

  } else {
    m = std::shared_ptr<topk_indexer>(nullptr);
  }
} END_OUT_OF_PLACE_LOAD()
#endif

/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TOPK_STATISTICS_TRACKER_H_
#define TURI_TOPK_STATISTICS_TRACKER_H_

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
 * Parallel statistics(mean) tracker
 *
 * Note: This implementation is intended to be general and will be moved to some
 *       place more general later.
 *
 * Construction
 * -------------
 *
 * // Construct the tracker with the arguments.
 * auto tracker = statistics_tracker(10, 1, "column_name_for_error_messages");
 * tracker.initialize();
 *
 * // Insert flexible types into the tracker
 * for (const flexible_type& v: sa.range_iterator() {
 *   tracker.insert_or_update(v);
 * }
 *
 * // Finalize mapping
 * tracker.finalize();
 *
 * Lookups
 * --------
 *  size_t index = tracker.lookup(v);        // Returns (size_t) -1 if not present.
 *
 *  size_t counts = tracker.lookup_counts(v); // Returns 0 if not present.
 *
 *  flexible_type v = tracker.inverse_lookup(1) // Fails if index doesn't exist.
 *
 * Parallel construction
 * -----------------------
 *
 * // Initialize
 * tracker.initialize();
 *
 *  // Perform the indexing.
 *  in_parallel([&](size_t thread_idx, size_t num_threads) {
 *
 *    size_t start_idx = src_size * thread_idx / num_threads;
 *    size_t end_idx = src_size * (thread_idx + 1) / num_threads;
 *
 *    // Inserts value of 1 for each key k
 *    for (const flexible_type& k: sa.range_iterator(start_idx, end_idx) {
 *      tracker.insert_or_update(k,1,thread_id);
 *    }
 *
 * // Finalize
 * tracker.finalize();
 *
 *
 */
class EXPORT statistics_tracker {

 public:

  /**
   *  Default constructor
   *
   * \param[in] column_name Column name for display.
   *
   */
  statistics_tracker( const std::string _column_name = "") :
                    column_name(_column_name) {
  }


  /**
   * Copy constructor: Don't want to risk making copies of this.
   */
  statistics_tracker(const statistics_tracker&) = delete;


  /**
   * Initialize the index mapping and setup. Should be called before
   * starting the map.
   */
  void initialize();

  /**
   * Insert
   *
   * \param[in] key       Flexible type.
   * \param[in] value     Flexible type.
   * \param[in] thread_idx  Thread id  (For parallel insertion).
   *
   */
  void insert_or_update(const flexible_type& key, flexible_type value,size_t thread_idx = 0) GL_HOT;

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
   * Returns the counts associated with the value.
   *
   *  \param[in] value Search for the value.
   *  \returns Counts (Returns 0 if not present).
   */
  flex_float lookup_means(const flexible_type& value) const;

  /**
   * Finalize by dropping indices that dont meet
   * - Count requirement i.e count >= threshold.
   * - Topk requirement.
   */
  void finalize(size_t num_examples);

  /**
   * Returns the "value" associated with the index.
   *
   * \param[\in] idx  Index associated with the feature value.
   * \return The "value" in the original data associated with the given id.
   */
  flexible_type inverse_lookup(size_t idx) const;



  /** Returns the number of categorical variables.
   *
   * \return Column size.
   */
  inline size_t size() const {
    return index_lookup.size();
  }

  /** Returns the number of categorical variables.
   *
   * \return Column size.
   */
  inline std::vector<flexible_type> get_keys() const {
    return keys;
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
  std::string column_name = "";

 // List of Map(hash : (value, count)) per thread.
 struct threadlocal_accumulator{
  std::vector<hopscotch_map<hash_value, size_t>> count;
  std::vector<hopscotch_map<hash_value, flex_float>> mean;
  std::vector<hopscotch_map<hash_value, size_t>> missing;
  std::vector<hopscotch_map<hash_value, flexible_type>> key_index;
 } threadlocal_accumulator;
  // Index -> value/cound
  std::vector<size_t> counts;
  std::vector<flex_float> means;
  std::vector<size_t> missing;
  std::vector<flexible_type> keys;

  // Map(value : index)
  hopscotch_map<hash_value, size_t> index_lookup;


  // Private helper functions.
  // ------------------------------------------------------------------------

  /**
   * Validate feature types.
   */
  void valdidate_types(const flexible_type& value) const;
};
} // turicreate


// Implement serialization for std::shared_ptr
BEGIN_OUT_OF_PLACE_SAVE(arc, std::shared_ptr<statistics_tracker>, m) {
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
BEGIN_OUT_OF_PLACE_LOAD(arc, std::shared_ptr<statistics_tracker>, m) {
  bool is_not_nullptr;
  arc >> is_not_nullptr;
  if(is_not_nullptr) {

    // Load version
    size_t version;
    arc >> version;

    // Load object.
    m.reset(new statistics_tracker(""));
    m->load_version(arc, version);

  } else {
    m = std::shared_ptr<statistics_tracker>(nullptr);
  }
} END_OUT_OF_PLACE_LOAD()
#endif

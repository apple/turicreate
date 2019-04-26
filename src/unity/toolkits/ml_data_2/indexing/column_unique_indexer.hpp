/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ML2_DATA_UNIQUE_COLUMN_INDEXER_H_
#define TURI_ML2_DATA_UNIQUE_COLUMN_INDEXER_H_

#include <flexible_type/flexible_type.hpp>
#include <util/hash_value.hpp>
#include <logger/assertions.hpp>
#include <util/bitops.hpp>
#include <serialization/serialization_includes.hpp>
#include <generics/hopscotch_map.hpp>
#include <parallel/pthread_tools.hpp>
#include <unity/toolkits/ml_data_2/indexing/column_indexer.hpp>

namespace turi { namespace v2 { namespace ml_data_internal {

/**  Use a two-level hash table to store the index mappings.  The
 *  first level is constant size and unlocked, determined by an n-bit
 *  hash.  Each leaf in this one contains a hash table and lock.  This
 *  significantly reduces lock contention.  This
 *  _column_metadata_first_level_lookup_size_n_bits gives the number
 *  of bits used for this first lookup.
 */
static constexpr int _column_unique_indexer_first_level_lookup_size_n_bits = 8;

/**
 * column_metadata contains "meta data" concerning indexing of a single column
 * of an SFrame. A collection of meta_data column objects is "all" the
 * metadata required in the ml_data container.
 */
class column_unique_indexer final : public column_indexer {

 public:

  /**
   *  Default constructor; does nothing;
   *  Initialize from a serialization stream
   */
  column_unique_indexer();

  /** Initialize the index mapping and setup.  There are certain
   *  internal parallel things that need to be set up before
   *  map_value_to_index works.  Call this before looping over
   *  map_value_to_index, then call finalize() when done.
   */
  void initialize();

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
  size_t map_value_to_index(size_t thread_idx, const flexible_type& feature) GL_HOT;

  /** Returns the index associated with the "feature" value.
   *
   * \note Only used if is_categorical is true.
   *
   * If the value in the feature column was already seen, then the
   * index already associated with that value is returned.  If not,
   * size_t(-1) is returned.
   *
   * \param[in] feature  The value in the feature column to map to the index.
   * \return An index associated with the given value. If the index is not
   * present. We return size_t(-1).
   */
  size_t immutable_map_value_to_index(const flexible_type& feature) const;

  /**  Some of the ml_data tests currently depend on the order of
   *   insertion into the index, which is now done in parallel and
   *   thus not deterministic.  This function allows the user to
   *   remove that randomness by inserting all indices in a specified
   *   order.
   *
   *   NOTE: This function is not thread safe; only call it from one
   *   thread.
   */
  void insert_values_into_index(const std::vector<flexible_type>& features);

  /**  When a new value is encountered in translating the data, it
   *   should be dealt with through map_value_to_index above if it is
   *   categorical, or through register_real_value below if it is
   *   numeric.  This function handles things like checking the size
   *   of the numeric vectors (all must be the same) and setting the
   *   column size.
   *
   *   Note that the statistics collection functions below are not
   *   always called; hence the error checks in register_real_value
   *   can't go there.
   */
  void register_real_value(const flexible_type& feature);


  /** Call this when all calls to map_value_to_index are completed.
   */
  void finalize();

  /** Returns the feature "value" associated an index.
   *
   * \note Only used if is_categorical is true.
   *
   * \param[\in] idx  Index associated with the feature value.
   * \return The "value" in the original data associated with the given id.
   */
  flexible_type map_index_to_value(size_t idx) const;

  /**  Calculates the type of the values held in the index.  This may
   *  be different from original_column_type -- if the
   *  original_column_type is a DICT or LIST, this will return a set
   *  of the actual types present.  If the values are
   *  inconsistent, then an error is raised.
   *
   *  This method is useful when a metadata built with a dictionary is
   *  also used to map simple categorical variables.
   */
  std::set<flex_type_enum> extract_key_types() const;

  /** Returns the size of the column.
   *
   * Numeric     : 1
   * Categorical : # Unique categories
   * Vector      : Size of the vector.
   *
   * \return Column size.
   */
  inline size_t indexed_column_size() const {
    return _column_size;
  }

  /** Returns the current version used for the serialization.
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

  /** Set data directly.
   *
   */
  void set_values(std::vector<flexible_type>&& values);

  std::vector<flexible_type> reset_and_return_values();
  
  
  /** Create a copy with the index cleared.
   */
  std::shared_ptr<column_indexer> create_cleared_copy() const; 

  /** Returns a lambda function that can be used as a lambda function for deindexing
   *  a column.
   */
  std::function<flexible_type(const flexible_type&)> deindexing_lambda() const;

  /** Returns a lambda function that can be used as a lambda function for indexing
   *  a column.
   *
   *  Does not add any new index values.
   */
  std::function<flexible_type(const flexible_type&)> indexing_lambda() const;
  
 private:

  std::vector<std::pair<simple_spinlock, hopscotch_map<hash_value, size_t> > >
  index_by_values_lookup;

  std::vector<std::vector<std::pair<size_t, flexible_type> > >
  values_by_index_threadlocal_accumulator;

  std::vector<flexible_type> values_by_index_lookup;

  // If we have numeric
  atomic<size_t> _column_size = 0;

  mutex index_modification_lock;
};

}}}

#endif

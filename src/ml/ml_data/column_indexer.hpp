/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DML_DATA_COLUMN_INDEXER_H_
#define TURI_DML_DATA_COLUMN_INDEXER_H_

#include <core/data/flexible_type/flexible_type.hpp>
#include <core/util/hash_value.hpp>
#include <core/logging/assertions.hpp>
#include <core/util/bitops.hpp>
#include <ml/ml_data/ml_data_column_modes.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
#include <core/generics/hopscotch_map.hpp>
#include <core/parallel/pthread_tools.hpp>

namespace turi {

namespace ml_data_internal {

/**
 * \ingroup mldata
 * \internal
 * \addtogroup mldata ML Data Internal
 * \{
 */


/**  Use a two-level hash table to store the index mappings.  The
 *  first level is constant size and unlocked, determined by an n-bit
 *  hash.  Each leaf in this one contains a hash table and lock.  This
 *  significantly reduces lock contention.  This
 *  _column_metadata_first_level_lookup_size_n_bits gives the number
 *  of bits used for this first lookup.
 */
static constexpr int _column_indexer_first_level_lookup_size_n_bits = 8;

/**
 * column_metadata contains "meta data" concerning indexing of a single column
 * of an SFrame. A collection of meta_data column objects is "all" the
 * metadata required in the ml_data container.
 */
class column_indexer {

 public:

  column_indexer() {}

  /**
   *  Default constructor; does nothing;
   */
  column_indexer(std::string column_name,
                 ml_column_mode mode,
                 flex_type_enum original_column_type);

  /**
   * Copy constructor: Don't want to risk making copies of this.
   */
  column_indexer(const column_indexer&) = delete;


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
  size_t map_value_to_index(size_t thread_idx, const flexible_type& feature) GL_HOT {
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

      auto throw_error = [&]() GL_GCC_ONLY(GL_COLD_NOINLINE) {
        log_and_throw(std::string("Value encountered in column '")
                      + column_name + "' is of type '"
                      + flex_type_enum_to_name(feature.get_type()) +
                      "' cannot be mapped to a categorical value." +
                      " Categorical values must be integer, strings, or None.");
      };
      throw_error();
    }

    hash_value wt(feature);

    // Lock the first level
    size_t first_index = wt.n_bit_index(_column_indexer_first_level_lookup_size_n_bits);
    DASSERT_LT(first_index, index_by_values_lookup.size());
    auto& lock_ht_pair = index_by_values_lookup[first_index];

    std::lock_guard<simple_spinlock> lg(lock_ht_pair.first);
    auto it = lock_ht_pair.second.find(wt);

    size_t index;

    if(it == lock_ht_pair.second.end()) {
      index = (++_column_size) - 1;
      values_by_index_threadlocal_accumulator[thread_idx].push_back({index, feature});
      lock_ht_pair.second[wt] = index;
    } else {
      index = it->second;
    }

    return index;
  }

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
  size_t immutable_map_value_to_index(const flexible_type& feature) const {

    DASSERT_TRUE(
        mode == ml_column_mode::CATEGORICAL
        || mode == ml_column_mode::CATEGORICAL_VECTOR
        || mode == ml_column_mode::DICTIONARY);

    // Check value
    if( ! (feature.get_type() == flex_type_enum::STRING
           || feature.get_type() == flex_type_enum::INTEGER
           || feature.get_type() == flex_type_enum::UNDEFINED) ) {

      auto throw_error = [&]() GL_GCC_ONLY(GL_COLD_NOINLINE) {
        log_and_throw(std::string("Value encountered in column '")
                      + column_name + "' is of type '"
                      + flex_type_enum_to_name(feature.get_type()) +
                      "' cannot be mapped to a categorical value." +
                      " Categorical values must be integer, strings, or None.");
      };
      throw_error();
    }

    hash_value wt(feature);

    // Lock the first level
    size_t first_index = wt.n_bit_index(_column_indexer_first_level_lookup_size_n_bits);
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
  const flexible_type& map_index_to_value(size_t idx) const {

    DASSERT_TRUE(mode == ml_column_mode::CATEGORICAL
                 || mode == ml_column_mode::CATEGORICAL_VECTOR
                 || mode == ml_column_mode::DICTIONARY);

    DASSERT_MSG(idx != size_t(-1),
                "Index not tracked in metadata table!");

    DASSERT_MSG(idx < values_by_index_lookup.size(),
                "Index not in metadata table; using correct metadata?");

    return values_by_index_lookup[idx];
  }

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

  /** Purges and returns all the values; The result is an indexer that
   *  contains no values, but metadata like name, mode, and type are
   *  preserved.
   */
  std::vector<flexible_type> reset_and_return_values();

  /** Sets the indices and creates all the index maps.
   */
  void set_indices(std::vector<flexible_type>&& values);

  /** Checks that the indices are equal across machines.
   */
  void debug_check_is_internally_consistent() const;

  void debug_check_is_equal(std::shared_ptr<column_indexer> other) const;

  const std::string& name() const { return column_name; }

  const ml_column_mode& column_mode() const { return mode;}

  const flex_type_enum& column_type() const { return original_column_type;}
 private:

  /**  The name of the column.
   */
  std::string column_name;

  /** The mode of the column;
   */
  ml_column_mode mode;

  /** Original column type
   */
  flex_type_enum original_column_type;

  std::vector<std::pair<simple_spinlock, hopscotch_map<hash_value, size_t> > >
  index_by_values_lookup;

  std::vector<std::vector<std::pair<size_t, flexible_type> > >
  values_by_index_threadlocal_accumulator;

  std::vector<flexible_type> values_by_index_lookup;

  // If we have numeric
  atomic<size_t> _column_size = 0;

  mutex index_modification_lock;
};

/// \}
}}

BEGIN_OUT_OF_PLACE_SAVE(arc, std::shared_ptr<ml_data_internal::column_indexer>, m) {
  if(m == nullptr) {
    arc << false;
  } else {
    arc << true;

    // Save the version number
    size_t version = m->get_version();
    arc << version;

    m->save_impl(arc);
  }

} END_OUT_OF_PLACE_SAVE()


BEGIN_OUT_OF_PLACE_LOAD(arc, std::shared_ptr<ml_data_internal::column_indexer>, m) {
  bool is_not_nullptr;
  arc >> is_not_nullptr;
  if(is_not_nullptr) {
    m.reset(new ml_data_internal::column_indexer);

    size_t version;
    arc >> version;
    m->load_version(arc, version);

  } else {
    m = std::shared_ptr<ml_data_internal::column_indexer>(nullptr);
  }
} END_OUT_OF_PLACE_LOAD()

#endif

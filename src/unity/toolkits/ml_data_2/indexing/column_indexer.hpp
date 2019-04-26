/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ML2_DATA_COLUMN_INDEXER_H_
#define TURI_ML2_DATA_COLUMN_INDEXER_H_

#include <string>
#include <map>
#include <unity/lib/variant.hpp>
#include <flexible_type/flexible_type.hpp>
#include <logger/assertions.hpp>
#include <serialization/serialization_includes.hpp>
#include <unity/toolkits/ml_data_2/ml_data_column_modes.hpp>
#include <unity/lib/variant_deep_serialize.hpp>
#include <functional>

namespace turi { namespace v2 { namespace ml_data_internal {

/** COMMENT.
 *
 * column_metadata contains "meta data" concerning indexing of a single column
 * of an SFrame. A collection of meta_data column objects is "all" the
 * metadata required in the ml_data container.
 */
class column_indexer {
 public:

  /**
   * Default constructor.
   */
  column_indexer() {}

  /** Initialize the index mapping and setup.  There are certain
   *  internal parallel things that need to be set up before
   *  map_value_to_index works.  Call this before looping over
   *  map_value_to_index, then call finish_indexing() when done.
   */
  virtual void initialize() = 0;

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
  virtual size_t map_value_to_index(size_t thread_idx, const flexible_type& feature ) = 0;

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
  virtual size_t immutable_map_value_to_index(const flexible_type& feature) const = 0;

  /**  Some of the ml_data tests currently depend on the order of
   *   insertion into the index, which is now done in parallel and
   *   thus not deterministic.  This function allows the user to
   *   remove that randomness by inserting all indices in a specified
   *   order.
   *
   *   NOTE: This function is not thread safe; only call it from one
   *   thread.
   */
  virtual void insert_values_into_index(const std::vector<flexible_type>& features) {};

  /** Call this when all calls to map_value_to_index are completed.
   */
  virtual void finalize() = 0;

  /** Returns the feature "value" associated an index.
   *
   * \note Only used if is_categorical is true.
   *
   * \param[\in] idx  Index associated with the feature value.
   * \return The "value" in the original data associated with the given id.
   */
  virtual flexible_type map_index_to_value(size_t idx) const {
    ASSERT_MSG(false, "Indexing not reversable with this indexer.");
    return 0;
  }

  /**  Calculates the type of the values held in the index.  This may
   *  be different from original_column_type -- if the
   *  original_column_type is a DICT or LIST, this will return
   *  the actual type of the values.  If the values are inconsistent,
   *  then an error is raised.
   *
   *  This method is useful when a metadata built with a dictionary is
   *  also used to map simple categorical variables.
   */
  virtual std::set<flex_type_enum> extract_key_types() const {
    ASSERT_MSG(false, "Indexing not reversable with this indexer.");
    return {flex_type_enum::UNDEFINED};
  }

  /** Returns the size of the column -- e.g. the number of distinct
   *  categories, or the size of the hash space.  Only called if the
   *  column is indeed indexed, i.e. if mode_is_indexed(mode) is true.
   *
   *  Categorical : # Unique categories
   *
   * \return Column size.
   */
  virtual size_t indexed_column_size() const = 0;

  ////////////////////////////////////////////////////////////////////////////////
  // Methods for creation and serialization

  /** Returns the current version used for the serialization.
   */
  virtual size_t get_version() const = 0;

  /** Serialize the object (save).
   */
  virtual void save_impl(turi::oarchive& oarc) const = 0;

  /** Load the object.
   */
  virtual void load_version(turi::iarchive& iarc, size_t version) = 0;

  /** Returns a lambda function that can be used as a lambda function for deindexing
   *  a column.
   */
  virtual std::function<flexible_type(const flexible_type&)> deindexing_lambda() const = 0;

  /** Returns a lambda function that can be used as a lambda function for indexing
   *  a column.
   *
   *  Does not add any new index values.
   */
  virtual std::function<flexible_type(const flexible_type&)> indexing_lambda() const = 0;

  /** Create a copy with the index cleared.
   */
  virtual std::shared_ptr<column_indexer> create_cleared_copy() const = 0; 

  /**  The factory method for loading and instantiating the proper class
   */
  static std::shared_ptr<column_indexer> factory_create(
      const std::map<std::string, variant_type>& creation_options);

  const std::map<std::string, variant_type>& get_serialization_parameters() const {
    return creation_options;
  }

  /** Set data directly.
   *
   */
  virtual void set_values(std::vector<flexible_type>&& values) = 0;
  virtual std::vector<flexible_type> reset_and_return_values() = 0;


 public:

  /**  The name of the column.
   */
  std::string column_name;

  /** The mode of the column;
   */
  ml_column_mode mode;

  /** Original column type
   */
  flex_type_enum original_column_type;

  /**  A map of the options passed in to ml_data.  May include options
   *   for the indexers.
   */
  std::map<std::string, flexible_type> options;

 private:

  /** A snapshot of the options needed for creating the class.
   */
  std::map<std::string, variant_type> creation_options;

};

}}}

////////////////////////////////////////////////////////////////////////////////
// Implement serialization for vector<std::shared_ptr<column_indexer>
// > and std::shared_ptr<column_indexer>

BEGIN_OUT_OF_PLACE_SAVE(arc, std::shared_ptr<v2::ml_data_internal::column_indexer>, m) {
  if(m == nullptr) {
    arc << false;
  } else {
    arc << true;

    // Save the version number
    size_t version = m->get_version();
    arc << version;

    // Save the model parameters as a map
    std::map<std::string, variant_type> serialization_parameters =
        m->get_serialization_parameters();

    // Save the version along with the creation options.
    serialization_parameters["version"] = to_variant(m->get_version());

    variant_deep_save(serialization_parameters, arc);

    m->save_impl(arc);
  }

} END_OUT_OF_PLACE_SAVE()


BEGIN_OUT_OF_PLACE_LOAD(arc, std::shared_ptr<v2::ml_data_internal::column_indexer>, m) {
  bool is_not_nullptr;
  arc >> is_not_nullptr;
  if(is_not_nullptr) {

    size_t version;
    arc >> version;

    std::map<std::string, variant_type> creation_options;
    variant_deep_load(creation_options, arc);

    m = v2::ml_data_internal::column_indexer::factory_create(creation_options);

    m->load_version(arc, version);

  } else {
    m = std::shared_ptr<v2::ml_data_internal::column_indexer>(nullptr);
  }
} END_OUT_OF_PLACE_LOAD()


#endif

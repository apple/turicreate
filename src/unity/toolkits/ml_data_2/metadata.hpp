/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ML2_METADATA_H_
#define TURI_ML2_METADATA_H_

#include <unity/toolkits/ml_data_2/ml_data_column_modes.hpp>
#include <unity/toolkits/ml_data_2/data_storage/internal_metadata.hpp>
#include "export.hpp"

namespace turi {

struct metadata_load;

namespace v2 {
class ml_data;
class ml_data_side_features;

class EXPORT ml_metadata {
 public:

  ml_metadata(){}
  ml_metadata(const ml_metadata&) = delete;

  /** Returns true if there is a target column present and false
   *  otherwise.
   */
  inline bool has_target() const;

  ////////////////////////////////////////////////////////////////////////////////
  // Accessing the indexers 

  /** Returns true if the underlying column type is indexed, and false
   *  otherwise.  This differs form the is_categorical in that
   *  dictionaries are not treated as pure categorical variables, as
   *  they have values associated with them, but they are indexed.
   *
   *  \param column_index The index of the column. 
   */
  inline bool is_indexed(size_t column_index) const;

  /** Returns true if the underlying column type is indexed, and false
   *  otherwise.  This differs form the is_categorical in that
   *  dictionaries are not treated as pure categorical variables, as
   *  they have values associated with them, but they are indexed.
   *
   *  \overload
   *
   *  \param column_name The name of the column. 
   */
  inline bool is_indexed(const std::string& column_name) const;
  
  /** Returns the metadata for a particular column, even if that
   *  column is a side feature.
   *
   *  \param column_index The index of the column. 
   */
  inline const std::shared_ptr<ml_data_internal::column_indexer>&
  indexer(size_t column_index) const;

  /** Returns the metadata for a particular column, even if that
   *  column is a side feature.
   *
   *  \overload
   *
   *  \param column_name The name of the column. 
   */
  inline const std::shared_ptr<ml_data_internal::column_indexer>&
  indexer(const std::string& column_name) const;

  /** Returns true if the underlying target type is indexed, and false
   *  otherwise.  This differs form the is_categorical in that
   *  dictionaries are not treated as pure categorical variables, as
   *  they have values associated with them, but they are indexed.
   */
  inline bool target_is_indexed() const;

  /** Returns the metadata for a particular column, even if that
   *  column is a side feature.
   */
  inline const std::shared_ptr<ml_data_internal::column_indexer>& target_indexer() const;

  ////////////////////////////////////////////////////////////////////////////////
  // Accessing the statistics
  
  /** Returns the metadata for a particular column, even if that
   *  column is a side feature.
   *
   *  \param column_index The index of the column. 
   */
  inline const std::shared_ptr<ml_data_internal::column_statistics>&
  statistics(size_t column_index) const;

  /** Returns the metadata for a particular column, even if that
   *  column is a side feature.
   *
   *  \overload
   *
   *  \param column_name The name of the column. 
   */
  inline const std::shared_ptr<ml_data_internal::column_statistics>&
  statistics(const std::string& column_name) const;

  /** Returns the metadata for a particular column, even if that
   *  column is a side feature.
   */
  inline const std::shared_ptr<ml_data_internal::column_statistics>& target_statistics() const;


  ////////////////////////////////////////////////////////////////////////////////
  // Aggregate statistics of the columns 

  /** Returns the number of columns present, including any possible
   *  side columns.
   */
  inline size_t num_columns(bool include_side_columns_if_present = true,
                            bool include_untranslated_columns = true) const; 

  /** Returns the number of untranslated columns present.
   */
  inline size_t num_untranslated_columns() const; 
  
  /** Returns true if there are translated columns present, and false
   *  otherwise.
   */
  inline bool has_translated_columns() const;

  /** Returns true if there are untranslated columns present, and false
   *  otherwise.
   */
  inline bool has_untranslated_columns() const;
  
  /** Returns the name of the column at column_index.
   *
   *  \param column_index The index of the column. 
   */
  inline const std::string& column_name(size_t column_index) const;

  /** Returns all column names as a vector.
   */
  std::vector<std::string> column_names(bool include_side_columns_if_present = true) const;

  /** Returns the index of the column matching column_name, or throws
   *  an error if it does not exist.
   *
   *  \param column_name The name of the column. 
   */
  inline size_t column_index(const std::string& column_name) const;

  /**
   * Returns true if the metadata contains the given column.
   *
   *  \param column_name The name of the column. 
   */
  inline bool contains_column(const std::string& column_name) const;
  
  /** Returns the name of the column at column_index.
   */
  inline const std::string& target_column_name() const;

  /** Returns the current index size of the columns in the metadata.
   *
   *  \param column_index The index of the column. 
   */
  inline size_t column_size(size_t column_index) const;

  /** Returns the current index size of the columns in the metadata.
   */
  inline size_t target_column_size() const;

  ////////////////////////////////////////////////////////////////////////////////
  // Index sizes
  
  /** Returns the index size of the column in the metadata that were
   *  present at train time.  Index size differs from column size in
   *  that column_size may grow on test, but index_size is constant.
   *
   *  \param column_index The index of the column. 
   */
  inline size_t index_size(size_t column_index) const;

  /** Returns the index size of the column in the metadata that were
   *  present at train time.  Index size differs from column size in
   *  that column_size may grow on test, but index_size is constant.
   *
   *  \overload
   *
   *  \param column_name The name of the column. 
   */
  inline size_t index_size(const std::string& column_name) const;

  
  /** Returns the global index offset of the columns in the metadata
   *  that were present at train time.  This is fixed at setup time;
   *  global indices for the column c_idx are in the interval
   *  [global_index_offset(c_idx), global_index_offset(c_idx) + index_size(c_idx) - 1]
   *
   *  \param column_index The index of the column.
   */
  inline size_t global_index_offset(size_t column_index) const;
  
  /** Returns the global index offset of the columns in the metadata
   *  that were present at train time.  This is fixed at setup time;
   *  global indices for the column c_idx are in the interval
   *  [global_index_offset(c_idx), global_index_offset(c_idx) + index_size(c_idx) - 1]
   *
   *  \overload
   *
   *  \param column_name The name of the column. 
   */
  inline size_t global_index_offset(const std::string& column_name) const;

  /** Returns the index size of the columns in the metadata that were
   *  present at train time.  Index size differs from column size in
   *  that column_size may grow on test, but index_size is constant.
   */
  inline size_t target_index_size() const;

  /** Returns the number of distinct dimensions, including all
   *  categorical features, side features, etc.
   */
  inline size_t num_dimensions() const;

  ////////////////////////////////////////////////////////////////////////////////
  // Accessing flags of the columns 
  
  /** Returns true if the underlying type is treated as a categorical
   *  variable, and false otherwise.
   *
   *  \param column_index The index of the column. 
   */
  inline bool is_categorical(size_t column_index) const;

  /** Returns true if the underlying type is treated as a categorical
   *  variable, and false otherwise.
   *
   *  \overload
   *
   *  \param column_name The name of the column. 
   */
  inline bool is_categorical(const std::string& column_name) const;
  
  /** Returns true if the underlying target type is treated as a
   *  categorical variable, and false otherwise.
   *
   *  \overload
   *
   *  \param column_name The name of the column. 
   */
  inline bool target_is_categorical() const;
  
  /** Returns true if the underlying column is a side column handled
   *  by an implicit join, and false otherwise.  
   *
   *  \param column_index The index of the column. 
   */
  inline bool is_side_column(size_t column_index) const;

  /** Returns true if the underlying column is a side column handled
   *  by an implicit join, and false otherwise.  
   *
   *  \overload
   *
   *  \param column_name The name of the column. 
   */
  inline bool is_side_column(const std::string& column_name) const;
  
  /** Returns true if the underlying column type is untranslated.
   *  This means it will only be available as flexible_type later on.
   *
   *  \param column_index The index of the column. 
   */
  inline bool is_untranslated_column(size_t column_index) const;

  /** Returns true if the underlying column type is untranslated.
   *  This means it will only be available as flexible_type later on.
   *
   *  \overload
   *
   *  \param column_name The name of the column. 
   */
  inline bool is_untranslated_column(const std::string& column_name) const;
  
  /**  Returns the mode of the column.  See ml_data_column_modes.hpp
   *  for details on the column modes.
   *
   *  \param column_index The index of the column. 
   */
  inline ml_column_mode column_mode(size_t column_index) const;

  /**  Returns the mode of the column.  See ml_data_column_modes.hpp
   *  for details on the column modes.
   *
   *  \overload
   *
   *  \param column_name The name of the column. 
   */
  inline ml_column_mode column_mode(const std::string& column_name) const;

  /**  Returns the mode of the target column.  See
   *  ml_data_column_modes.hpp for details on the column modes.
   */
  inline ml_column_mode target_column_mode() const;

  /** Returns the type of the column in the metadata
   *  present at train time.
   *
   *  \param column_index The index of the column. 
   */
  inline flex_type_enum column_type(size_t column_index) const;

  /** Returns the type of the column in the metadata 
   *  present at train time.
   *
   *  \overload
   *
   *  \param column_name The name of the column. 
   */
  inline flex_type_enum column_type(const std::string& column_name) const;
  
  /**  Returns the mode of the target column.  See
   *  ml_data_column_modes.hpp for details on the column modes.
   */
  inline flex_type_enum target_column_type() const;

  ////////////////////////////////////////////////////////////////////////////////
  // Other information. 
  
  /** Returns the current options.
   */
  inline const std::map<std::string, flexible_type>& get_current_options() const;

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
  std::string feature_name(size_t column_idx, size_t index) const;

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
   * \returns Names of features
   */
  std::vector<std::string> feature_names(bool unpack_categorical_columns = true) const;


  /** Serialization version.
   */
  size_t get_version() const { return 2; }

  /** Serialization -- save.
   */
  void save(turi::oarchive& oarc) const;

  /** Serialization -- load.
   */
  void load(turi::iarchive& iarc);

  /** Returns true if there is side data and false otherwise.
   */
  inline bool has_side_features() const;

  /** 
   * Set the missing value action on predict time.
   */
  inline void set_missing_value_on_predict(const std::string& missing_value_action) {
    options["missing_value_action_on_predict"] = missing_value_action;
  }

  /** Returns the side feature storage class.
   */
  inline std::shared_ptr<ml_data_side_features> get_side_features() const;

  /** Sets the values of all future calls to index_size() to return
   *  the column_size values currently present in the indexers.  This
   *  is done automatically at the end of fill(), but it can be useful
   *  if more is done to the indexers after that that is still
   *  considered part of training.
   */
  void set_training_index_sizes_to_current_column_sizes();

  /** Create a new metadata object that shares the same indexing as
   *  the previous one, but has possibly different and possibly
   *  subsetted columns.
   *
   *  The indexing on the new columns is preserved from the original
   *  metadata.  Thus all the index_size, global_index_offset,
   *  etc. remain the same.  The indexer classes are shared between
   *  the two metadata objects.
   *
   *  Example:
   *
   *   ml_data data_user_item({{"sort_by_first_two_columns_on_train", true}});
   *   data_user_item.fill(X);
   *
   *   // data_user_item is now sorted by user, then by item.
   *
   *   ml_data data_item_user(data_user_item.metadata()->select_columns({"item", "user"});
   *   data_item_user.fill(X);
   *
   *   // data_item_user is now sorted by item, then by user.
   *
   */
  std::shared_ptr<ml_metadata> select_columns(
      const std::vector<std::string>& columns, bool include_target = true) const;


 private:

  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Internal routines for the data stuff


  /** Returns a pointer to the internal column metadata of column
   *  column_index.  Useful for dealing with the column_index
   */
  inline ml_data_internal::column_metadata_ptr get_column_metadata(size_t column_index) const;

  ////////////////////////////////////////////////////////////////////////////////
  // Data

  friend class ml_data;
  friend struct turi::metadata_load;

  // column-specific metadata
  std::vector<ml_data_internal::column_metadata_ptr> columns;
  ml_data_internal::column_metadata_ptr target;

  // Side features
  std::shared_ptr<ml_data_side_features> side_features;

  // The original names of the columns.  This may be in a different
  // order than the columns above (e.g. user and items are moved to
  // index 0 and 1 in the recommender).  this allows us to reorder the
  // columns as needed
  std::vector<std::string> original_column_names;
  
  // The options the model was created with.
  std::map<std::string, flexible_type> options;
  
  // Cached values; this is a small optimization here to allow
  // statistics to be used in time-sensitive places.  The
  // setup_cached_values function prepares these from the current
  // metadata.  This is only called by the load and creation routines.
  void setup_cached_values();

  size_t _num_dimensions = size_t(-1);
  size_t _num_untranslated_columns = size_t(-1);
  std::map<std::string, size_t> _column_name_to_index_map;
};

}}

// Include the implementations of the above inline functions.
#include <unity/toolkits/ml_data_2/metadata_impl.hpp>

////////////////////////////////////////////////////////////////////////////////
// Implement serialization for
// std::shared_ptr<ml_metadata>

BEGIN_OUT_OF_PLACE_SAVE(arc, std::shared_ptr<turi::v2::ml_metadata>, m) {
  if(m == nullptr) {
    arc << false;
  } else {
    arc << true;
    arc << (*m);
  }
} END_OUT_OF_PLACE_SAVE()

BEGIN_OUT_OF_PLACE_LOAD(arc, std::shared_ptr<turi::v2::ml_metadata>, m) {
  bool is_not_nullptr;
  arc >> is_not_nullptr;
  if(is_not_nullptr) {
    m.reset(new turi::v2::ml_metadata);
    arc >> (*m);
  } else {
    m = std::shared_ptr<turi::v2::ml_metadata>(nullptr);
  }
} END_OUT_OF_PLACE_LOAD()

#endif

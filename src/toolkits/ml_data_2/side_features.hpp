/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ML2_DATA_SIDE_FEATURES_H_
#define TURI_ML2_DATA_SIDE_FEATURES_H_

#include <toolkits/ml_data_2/data_storage/ml_data_row_format.hpp>
#include <toolkits/ml_data_2/data_storage/ml_data_side_feature_translation.hpp>
#include <toolkits/ml_data_2/data_storage/internal_metadata.hpp>
#include <toolkits/ml_data_2/ml_data_entry.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
#include <vector>
#include <map>
#include <memory>

namespace turi {

class sframe;

namespace v2 {

/** A class to manage possible sources of side information.
 */
class ml_data_side_features {

 private:
  friend class ml_data;

  /** Main constructor.  To be constructed only from winithin ml_data.
   */
  ml_data_side_features(const std::vector<ml_data_internal::column_metadata_ptr>& main_metadata);

  /** Should not be assigning things; use the copy constructor.
   */
  const ml_data_side_features& operator=(const ml_data_side_features&) = delete;

  /** Add in a new source of side information.  This may be called
   *  many times to include new information.
   *
   * Joining is done by selecting a column with name the same as the
   * name of the column in the main_metadata provided to the
   * constructor of this class.  If no column is found, or if there
   * are multiple columns matching, an error is thrown.
   *
   * The new side information is indexed using the metadata indexer,
   * with the column on which the join is performed indexed with the
   * same metadata as the original column.
   *
   * It is possible to call this function multiple times.  If the join
   * column is new, it adds a block of column indices matched to that
   * side information.  When the observation vector is filled in with
   * side information -- i.e. the join is performed -- these column
   * indices have their own unique block.  The metadata for that block
   * can be accessed through get_full_metadata();
   *
   * If the join column is the same as a previous one, any new entries
   * replace the previous entries.  Only one table of side information
   * is allowed per join column, so the schemas must match up.  In
   * this case, the column_is_categorical parameter can be ommitted.
   *
   */
  void add_and_index_side_data(sframe unindexed_side_sframe,
                               const std::map<std::string, ml_column_mode>& mode_override,
                               const std::map<std::string, flexible_type>& options,
                               bool training_mode,
                               bool immutable_metadata,
                               const std::string& forced_join_column = "");

 public:

  struct side_feature_info {
    size_t column_offset;
    const ml_data_internal::row_metadata& rm;
    ml_data_internal::entry_value_iterator row_block_ptr;
  };

  ////////////////////////////////////////////////////////////////////////////////
  // Versions of this with only the main index given.

  /** Returns a pointer to the raw location
   *
   */
  side_feature_info get_side_feature_block(
      size_t main_column_index, size_t main_feature_index) const GL_HOT_INLINE_FLATTEN {

    DASSERT_LT(main_column_index, side_lookups.size());

    const column_side_info& csi = side_lookups[main_column_index];

    // Get the pointer to the row of entry values.  If it's out of
    // range or the lookup resolves to the null pointer, then there is
    // no side information for this value.

    ml_data_internal::entry_value_iterator block_ptr =
        ( (main_feature_index < csi.data_lookup_map.size())
          ? csi.data_lookup_map[main_feature_index]
          : ml_data_internal::entry_value_iterator(nullptr));

    return {csi.column_index_start, csi.rm, block_ptr};
  }


  /// Overload of the above. Appends the side features associated with
  /// exactly one of the main columns to the observation vector x.
  template <typename EntryType>
  void add_partial_side_features_to_row(
      std::vector<EntryType>& x, size_t main_column_index, size_t feature_index) const {

    DASSERT_LT(main_column_index, side_lookups.size());

    const column_side_info& csi = side_lookups[main_column_index];

    // Get the pointer to the row of entry values.  If it's out of
    // range or the lookup resolves to the null pointer, then there is
    // no side information for this value.
    if(feature_index >= csi.data_lookup_map.size())
      return;

    const ml_data_internal::entry_value* block_ptr = csi.data_lookup_map[feature_index];

    if(block_ptr == nullptr)
      return;

    ml_data_internal::append_raw_to_entry_row(csi.rm, block_ptr, x, csi.column_index_start);
  }

  /// Dummy overload to make a number of compiler issues easier.
  template <typename EntryType, size_t n>
  void add_partial_side_features_to_row(
      std::array<EntryType, n>& x, size_t main_column_index, size_t feature_index) const {
    ASSERT_MSG(false, "Programming Error: arrays not compatible with side features.");
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Versions of this with only the main index given.

  /// Appends the side features associated with exactly one of the
  /// main columns to the observation vector x.
  void add_partial_side_features_to_row(
      std::vector<ml_data_entry>& x, size_t main_column_index) const GL_HOT {
    add_partial_side_features_to_row(x, main_column_index, x[main_column_index].index);
  }

  /// Appends the side features associated with exactly one of the
  /// main columns to the observation vector x.
  template <typename GlobalEntryType>
  void add_partial_side_features_to_row(
      std::vector<GlobalEntryType>& x, size_t main_column_index) const {
    add_partial_side_features_to_row(
        x, main_column_index,
        x[main_column_index].global_index - _full_metadata[main_column_index]->global_index_offset());
  }

  /// Dummy overload to make a number of compiler issues easier.
  template <typename EntryType, size_t n>
  void add_partial_side_features_to_row(
      std::array<EntryType, n>& x, size_t main_column_index) const {
    ASSERT_MSG(false, "Programming Error: arrays not compatible with side features.");
  }

  /** Returns the bounds on the column indices of data
   *  associated with a particular column.
   *
   *  In a full std::vector<ml_data_entry> observation, the entries
   *  with column indices between these two values will be from the
   *  side data associated with main_column_index.
   */
  std::pair<size_t, size_t> column_indices_of_side_information_block(size_t main_column_index) const {
    DASSERT_LT(main_column_index, side_lookups.size());
    const column_side_info& csi = side_lookups[main_column_index];

    return std::make_pair(csi.column_index_start,
                          csi.column_index_start + csi.rm.metadata_vect.size());
  }

  /** Returns the bounds on the global indices of data associated with
   *  a particular column.
   *
   *  In a full std::vector<ml_data_entry> observation, the entries
   *  with column indices between these two values will be from the
   *  side data associated with main_column_index.
   */
  std::pair<size_t, size_t> global_indices_of_side_information_block(size_t main_column_index) const {
    DASSERT_LT(main_column_index, side_lookups.size());
    const column_side_info& csi = side_lookups[main_column_index];

    size_t start_idx = csi.column_index_start;
    size_t end_idx = csi.column_index_start + csi.rm.metadata_vect.size();

    if(start_idx == end_idx)
      return {0,0};

    return std::make_pair(_full_metadata[start_idx]->global_index_offset(),
                          (_full_metadata[end_idx - 1]->global_index_offset()
                           + _full_metadata[end_idx - 1]->index_size()));
  }

  /** Appends all available side information to the vector x based on
   * current values in x.
   */
  inline void add_side_features_to_row(std::vector<ml_data_entry>& x) const {

    DASSERT_EQ(x.size(), main_metadata.size());

    const size_t x_size = x.size();

    for(size_t i = 0; i < x_size; ++i) {
      add_partial_side_features_to_row(x, i);
    }
  }

  /** Appends all available side information to the vector x based on
   * current values in x.
   */
  template <typename GlobalEntryType>
  inline void add_side_features_to_row(std::vector<GlobalEntryType>& x) const {

    DASSERT_EQ(x.size(), main_metadata.size());

    const size_t x_size = x.size();

    for(size_t i = 0; i < x_size; ++i) {
      add_partial_side_features_to_row(x, i, x[i].global_index - _full_metadata[i]->global_index_offset());
    }
  }

  /** Overload of above; this case shouldn't ever be called -- i.e.,
   *  if you are using side features, you should be using vectors, not arrays.
   */
  template <typename EntryType, size_t n>
  inline void add_side_features_to_row(std::array<EntryType, n>& x) const {
    ASSERT_MSG(false, "Programming Error: arrays not compatible with side features.");
  }


  /** Strips out the side features in the row associated with main_column_index.
   */
  inline void strip_side_features_from_row(size_t main_column_index, std::vector<ml_data_entry>& x) const {

    size_t lb, ub;
    std::tie(lb, ub) = column_indices_of_side_information_block(main_column_index);

    auto new_end = std::remove_if(x.begin(), x.end(),
                                  [&](const ml_data_entry& v) {
                                    return (lb <= v.column_index) && (v.column_index < ub);
                                  });

    x.resize(new_end - x.begin());
  }

  /** Strips out the side features in the row associated with
   * main_column_index --
   */
  template <typename GlobalEntryType>
  inline void strip_side_features_from_row(size_t main_column_index, std::vector<GlobalEntryType>& x) const {

    size_t lb, ub;
    std::tie(lb, ub) = global_indices_of_side_information_block(main_column_index);

    auto new_end = std::remove_if(x.begin(), x.end(),
                                  [&](const GlobalEntryType& v) {
                                    return (lb <= v.global_index) && (v.global_index < ub);
                                  });

    x.resize(new_end - x.begin());
  }

  /** Can't strip out arrays.
   */
  template <typename EntryType, size_t n>
  inline void strip_side_features_from_row(size_t main_column_index, std::array<EntryType, n>& x) const {
    ASSERT_MSG(false, "Programming Error: arrays not compatible with side features.");
  }

  /** Returns the number of columns joined off of column
   *  main_column_index in the main data.
   */
  size_t num_columns(size_t main_column_index) const;

  ////////////////////////////////////////////////////////////////////////////////

 private:
  friend class ml_metadata;

  /** Uniquify the side column names.
   */
  void uniquify_side_column_names(
      sframe& side_sframe,
      std::map<std::string, std::string>& column_name_map,
      const std::string& join_name) const;

  /// Serialization -- save and load functions.  NOTE: these assume
  /// that the correct metadata has already been set.
  void save_without_metadata(turi::oarchive& oarc) const;
  void load_with_metadata_present(turi::iarchive& iarc);


  // The main side column information is intended to be accessed from
  // the metadata class, not directly...

  /** Returns the full metadata for all columns, including side
   * information.  The full metadata contains the metadata for all the
   * columns concatenated, as opposed to just the metadata of the main
   * observation sframe. If you have just user-items in the main data,
   * and 2 additional columns joined on user, main_metadata will be
   * length 2 and get_full_metadata() will be length 4.
   */
  const std::vector<ml_data_internal::column_metadata_ptr>& get_full_column_metadata() const {
    return _full_metadata;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Additional accessor functions to make accessing parts of the
  // metadata and side features easier.

  /** Returns the metadata in the main_column_index with
   * side_column_index.  The main_column_index determines which side
   * table we are referring to. Within that side table, each column
   * has metadata associated with it.
   */
  const ml_data_internal::column_metadata& get_column_metadata(
      size_t main_column_index, size_t side_column_index) const;

  /**  Returns the maximum row size that is added on by the side data.
   *  This is the sum of the maximum possible sizes of the rows in
   *  each column.
   */
  size_t max_additional_row_size() const {
    size_t _max_additional_row_size = 0;

    for(const column_side_info& csi : side_lookups) {
      _max_additional_row_size += csi.max_row_size;
    }

    return _max_additional_row_size;
  }

  /** This function is needed to remap things for the metadata
   *  select_columns function.  When selecting a subset of columns,
   *  this copies over the metadat in order to make it worthwhile.
   */
  std::shared_ptr<ml_data_side_features> copy_with_new_main_columns(
      const std::vector<ml_data_internal::column_metadata_ptr>& new_columns) const;

 private:

  /// The main metadata
  const std::vector<ml_data_internal::column_metadata_ptr> main_metadata;
  std::map<std::string, size_t> main_column_name_lookup;

  struct column_side_info {
    size_t column_index_start = 0;
    size_t max_row_size = 0;

    ml_data_internal::row_metadata rm;

    // A map to track column uniquify renamings.
    std::map<std::string, std::string> column_name_map;

    // A map of pointers into the raw data below.  If a pointer is null,
    // there are no side features for that column / value.
    std::vector<ml_data_internal::entry_value_iterator> data_lookup_map;
  };

  std::vector<column_side_info> side_lookups;

  ////////////////////////////////////////////////////////////
  // Some utility functions to govern how

  /// All the rows of side information are stored somewhere in the
  /// vectors in raw_row_storage as a block of entry_values.  Indexing
  /// into the raw storage is provided by the data_lookup_map in the
  /// column_side_info structure.  To access the side row pointed to
  /// by index j of column k, you would read the row starting at the
  /// pointer in side_lookups[k].data_lookup_map[j].  If that pointer
  /// is null, there is no side information provided for that row.
  ///
  /// Thus the data layout is simply some raw storage area in which
  /// all the column information is dumped. It's completely unordered
  /// and has no organizational structure -- it's designed entirely so
  /// that the data_lookup_map can store pointers into it. Because
  /// it's a shared_ptr to a vector, the memory inside it will never
  /// be moved (unless this class is destroyed). Thus the pointers in
  /// data_lookup_map give the raw address of the data to copy into
  /// the observation.
  ///
  /// The number of columns to read is given by
  /// side_lookups[k].metadata.size(), with a lookup of how to
  /// interpret them given by the value of
  /// side_lookups[k].column_types[c_idx].  Depending on the column
  /// types, the data layout is different.
  ///
  /// - If it's numeric, then that contribution is a single entry_value
  ///   with double_value filled to that entry.
  ///
  /// - If it's categorical, it's contribution is a single entry_value
  ///   with index_value filled to the index of its categorical
  ///   variable.
  ///
  /// - If it's a vector, then the first entry_value::index_value
  ///   gives the number of values after that go into that value.  For
  ///   example, if it's [3, 4.0, 1.5, 2.0, x, ...], then 3 would say
  ///   it's a vector of length 3, which would be values 4.0, 1.5,
  ///   and 2.0.  x would be the start of the next column.
  ///
  /// - If it's a dictionary, then the first entry_value::index_value
  ///   gives the number of pairs of index, value triplets after that
  ///   go into that column.  For example, if it's [2, 32, 2.0, 16,
  ///   3.0, x, ...], then 2 would say there are 2 index-value pairs
  ///   following it, (32, 2.0) and (16, 3.0). x would be the start of
  ///   the next column.

  std::vector<std::shared_ptr<const ml_data_internal::row_data_block> > raw_row_storage;

  /// The column index telling us where to put new side information
  /// blocks
  size_t current_column_index;

  std::vector<ml_data_internal::column_metadata_ptr> _full_metadata;
};

}}

#endif /* TURI_ML2_DATA_SIDE_FEATURES_H_ */

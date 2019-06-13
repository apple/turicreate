/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DML_DATA_H_
#define TURI_DML_DATA_H_

#include <vector>
#include <memory>
#include <core/storage/sframe_data/sframe.hpp>
#include <ml/ml_data/metadata.hpp>
#include <ml/ml_data/ml_data_entry.hpp>
#include <ml/ml_data/ml_data_column_modes.hpp>

#include <Eigen/SparseCore>
#include <Eigen/Core>

namespace turi {

class ml_data_iterator;

namespace ml_data_internal {
class ml_data_block_manager;
struct row_data_block;
class ml_data_reconciler;
}

/**
 * \defgroup mldata ML Data
 * Data Normalization.
 * See \ref ml_data for details
 */

/**
 * \ingroup mldata
 * Row based, SFrame-Like Data storage for Learning and Optimization tasks.
 *
 *  `ml_data` is a data normalization datastructure that translates user input tables
 *  (which can contain arbitrary types like strings, lists, dictionaries, etc) into
 *  sparse and dense numeric vectors. This allows toolkits to be implemented in a
 *  way that operates on fully mathematical, numeric assumptions, but support a
 *  much richer surface area outside.
 *
 *  To support this, `ml_data` is kind of a complicated datastructure that
 *  performs several things.
 *   - interpret string columns as categorical onto a sparse vector representation,
 *     using either one-hot encoding or reference encoding.
 *   - map list columns onto a sparse vector representation.
 *   - map dictionary columns onto a sparse vector representation.
 *   - map dense numeric arrays onto a dense vector representation.
 *   - etc.
 *  Each row of a user input table is hence translated into a mixed
 *  dense-sparse vector. This vector then has to be materialized as an SFrame
 *  (allowing it to scale to datasets larger than memory).
 *
 *  This can then be used to train other Machine Learning models with.
 *
 *  Finally, the `ml_data` datastructure has to remember and store the translation
 *  mappings so that the exact procedure can be performed later on new data
 *  (when using the trained model)
 *
 *  Additionally `ml_data` also implement strategies for automatic imputation of missing
 *  data. For instance, missing numeric columns can be imputed with the mean,
 *  missing categorical columns can be imputed with the most common value, etc.
 *
 *
 * ml_data loads data from an existing sframe, indexes it by mapping
 * all categorical values to unique indices in 0, 1,2,...,n, and
 * records statistics about the values.  It then puts it into an
 * efficient row-based data storage structure for use in learning
 * algorithms that need fast row-wise iteration through the features
 * and target. The row based storage structure is designed for fast
 * iteration through the rows and target.  ml_data also speeds up data
 * access via caching and a compact layout.
 *
 *
 * Illustration of the API
 * =======================
 *
 * Using ml_data
 * -------------
 *
 * There are a number of use cases for ml_data.  The following should
 * address the current use cases.
 *
 * ### To construct the data at train time:
 * \code
 *     // Constructs an empty ml_data object
 *     ml_data data;
 *
 *     // Sets the data source from X, with target_column_name being the
 *     // target column.  (Alternatively, target_column_name may be a
 *     // single-column SFrame giving the target.  "" denotes no target
 *     // column present).
 *     data.fill(X, target_column_name);
 *
 *     // After filling, a serializable shared pointer to the metadata
 *     // can be saved for the predict stage.  this->metadata is of type
 *     // std::shared_ptr<ml_metadata>.
 *     this->metadata = data.metadata();
 * \endcode
 *
 *
 * ### To iterate through the data, single threaded.
 * \code
 *     for(auto it = data.get_iterator(); !it.done(); ++it) {
 *        ....
 *        it->target_value();
 *        it->fill(...);
 *     }
 * \endcode
 *
 *
 * ### To iterate through the data, threaded.
 *
 * \code
 *     in_parallel([&](size_t thread_idx, size_t num_threads) {
 *
 *       for(auto it = data.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
 *          ....
 *          it->target_value();
 *          it->fill(...);
 *       }
 *     });
 * \endcode
 *
 *
 * ### To construct the data at predict time:
 *
 * \code
 *     // Constructs an empty ml_data object, takes construction options
 *     // from original ml_data.
 *     ml_data data(this->metadata);
 *
 *     // Sets the data source from X, with no target column.
 *     data.fill(X);
 * \endcode
 *
 *
 * ### To serialize the metadata for model serialization
 *
 * \code
 *     // Type std::shared_ptr<ml_metadata> is fully serializable.
 *     oarc << this->metadata;
 *
 *     iarc >> this->metadata;
 * \endcode
 *
 * ### To access statistics at train/predict time.
 *
 * Statistics about each of the columns is fully accessible at any point
 * after training time, and does not change.  This is stored with the
 * metadata.
 *
 *
 * \code
 *     // The number of columns.  column_index
 *     // below is between 0 and this value.
 *     this->metadata->num_columns();
 *
 *     // This gives the number of index value at train time.  Will never
 *     // change after training time.  For categorical types, it gives
 *     // the number of categories at train time.  For numerical it is 1
 *     // if scalar and the width of the vector if numeric.  feature_idx
 *     // below is between 0 and this value.
 *     this->metadata->index_size(column_index);
 *
 *     // The number of rows having this feature.
 *     this->metadata->statistics(column_index)->count(feature_idx);
 *
 *     // The mean of this feature.  Missing is counted as 0.
 *     this->metadata->statistics(column_index)->mean(idx);
 *
 *     // The std dev of this feature.  Missing is counted as 0.
 *     this->metadata->statistics(column_index)->stdev(idx);
 *
 *     // The number of rows in which the value of this feature is
 *     // strictly greater than 0.
 *     this->metadata->statistics(column_index)->num_positive(idx);
 *
 *     // The same methods above, but for the target.
 *     this->metadata->target_statistics()->count();
 *     this->metadata->target_statistics()->mean();
 *     this->metadata->target_statistics()->stdev();
 * \endcode
 *
 *
 *
 * ### Forcing certain column modes.
 *
 * The different column modes control the behavior of each column.  These
 * modes are defined in ml_data_column_modes as an enum and currently
 * allow NUMERIC, NUMERIC_VECTOR, CATEGORICAL, CATEGORICAL_VECTOR,
 * DICTIONARY.
 *
 * In most cases, there is an obvious default.  However, to force some
 * columns to be set to a particular mode, a mode_override parameter is
 * available to the set_data and add_side_data functions as a map from
 * column name to column_mode.  This overrides the default choice.  The
 * main use case for this is recsys, where user_id and item_id will
 * always be categorical:
 *
 * \code
 *      data.fill(recsys_data, "rating",
 *                    {{"user_id", column_mode::CATEGORICAL},
 *                     {"item_id", column_mode::CATEGORICAL}});
 * \endcode
 *
 * Untranslated Columns
 * ----------------------------------------
 *
 * Untranslated columns can be specified with the set_data(...)
 * method.  The untranslated columns are tracked alongside the regular
 * ones, but are not themselves translated, indexed, or even loaded
 * until iteration.  These additional columns are then available using
 * the iterator's fill_untranslated_values function.
 *
 * The way to mark a column as untranslated is to manually specify its
 * type as ml_column_mode::UNTRANSLATED using the mode_overrides
 * parameter in the set_data method.  The example code below
 * illustrates this:
 *
 *
 * \code
 *     sframe X = make_integer_testing_sframe( {"C1", "C2"}, { {0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4} } );
 *
 *     ml_data data;
 *
 *     data.set_data(X, "", {}, { {"C2", ml_column_mode::UNTRANSLATED} });
 *
 *     data.fill();
 *
 *
 *     std::vector<ml_data_entry> x_d;
 *     std::vector<flexible_type> x_f;
 *
 *     ////////////////////////////////////////
 *
 *     for(auto it = data.get_iterator(); !it.done(); ++it) {
 *
 *       it->fill(x_d);
 *
 *       ASSERT_EQ(x_d.size(), 1);
 *       ASSERT_EQ(x_d[0].column_index, 0);
 *       ASSERT_EQ(x_d[0].index, 0);
 *       ASSERT_EQ(x_d[0].value, it.row_index());
 *
 *       it->fill_untranslated(x_f);
 *
 *       ASSERT_EQ(x_f.size(), 1);
 *       ASSERT_TRUE(x_f[0] == it.row_index());
 *     }
 * \endcode
 *
 */
class ml_data {
 public:

  ml_data(const ml_data&);
  const ml_data& operator=(const ml_data&);
  ml_data& operator=(ml_data&&) = default;

  ml_data(ml_data&&) = default;

  /**
   *   Construct an ml_data object based current options.
   */
  ml_data();


  /**
   *  Construct an ml_data object based on previous ml_data metadata.
   */
  explicit ml_data(const std::shared_ptr<ml_metadata>& metadata);

  /// This is here to get around 2 clang bugs!
  typedef std::map<std::string, ml_column_mode> column_mode_map;

  /*********************************************************************************
   *
   * Missing Value Action
   * ================================================================================
   *
   * IMPUTE
   * ------
   * Imputes the data with the mean. Do not use this during creation time because
   * the means will change over time. Imnputation only makes sense when you
   * do it during predict/evaluate time.
   *
   *
   * ERROR
   * ------
   * Error out when a missing value occurs in a numeric columns. Keys (categorical
   * variables of dictionary keys) can accept missing values.
   *
   *
   * USE_NAN
   * ------
   * Use NAN as value for missing values.
   */

  /** Fills the data from an SFrame.
   *
   *  \param data The data sframe.
   *
   *  \param target_column If not reusing metadat, specifies the
   *  target column.  If no target column is present, then use "".
   *
   *  \param mode_overrides A dictionary of column-name to
   *  ml_column_mode mode overrides.  These will be used instead of
   *  the default flex_type_enum -> ml_column_mode mappings.  The main
   *  use is to specify integers as categorical or designate some
   *  columns as untranslated.
   *
   *  \param immutable_metadata If true, then any new values in
   *  categorical columns will be mapped to size_t(-1) and not
   *  indexed.
   *
   *  \param mva The behavior when missing values are present.
   */
  void fill(const sframe& data,
            const std::string& target_column = "",
            const column_mode_map mode_overrides = column_mode_map(),
            bool immutable_metadata = false,
            ml_missing_value_action mva = ml_missing_value_action::ERROR);

  /** Fills the data from an SFrame.
   *
   *  \param data The data sframe.
   *
   *  \param row_bounds The (lower, upper) bounds on which rows from
   *  the original data sframe are considered.  It is as if the
   *  original sframe has only these rows.
   *
   *  \param target_column If not reusing metadat, specifies the
   *  target column.  If no target column is present, then use "".
   *
   *  \param mode_overrides A dictionary of column-name to
   *  ml_column_mode mode overrides.  These will be used instead of
   *  the default flex_type_enum -> ml_column_mode mappings.  The main
   *  use is to specify integers as categorical or designate some
   *  columns as untranslated.
   *
   *  \param immutable_metadata If true, then any new values in
   *  categorical columns will be mapped to size_t(-1) and not
   *  indexed.
   *
   *  \param mva The behavior when missing values are present.
   */
  void fill(const sframe& data,
            const std::pair<size_t, size_t>& row_bounds,
            const std::string& target_column = "",
            const column_mode_map mode_overrides = column_mode_map(),
            bool immutable_metadata = false,
            ml_missing_value_action _mva = ml_missing_value_action::ERROR);


  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Metadata access
  //
  ////////////////////////////////////////////////////////////////////////////////

  /** Direct access to the metadata.
   */
  inline const std::shared_ptr<ml_metadata>& metadata() const {
    return _metadata;
  }

  /** Returns the number of columns present.
   */
  inline size_t num_columns() const {
    return _metadata->num_columns();
  }

  /** The number of rows present.
   */
  inline size_t num_rows() const {
    return _row_end - _row_start;
  }

  /** The number of rows present.
   */
  inline size_t size() const {
    return num_rows();
  }

  /** Returns true if there is no data in the container.
   */
  inline bool empty() const {
    return _row_start == _row_end;
  }

  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Iteration Access
  //
  ////////////////////////////////////////////////////////////////////////////////

  /** Return an iterator over part of the data.  See
   *  iterators/ml_data_iterator.hpp for documentation on the returned
   *  iterator.
   */
  ml_data_iterator get_iterator(size_t thread_idx=0, size_t num_threads=1) const;


  /**  Returns true if a target column is present, and false otherwise.
   */
  bool has_target() const {
    return rm.has_target;
  }

  /**  Returns true if there are untranslated columns present, and
   *   false otherwise.
   */
  bool has_untranslated_columns() const {
    return (!untranslated_columns.empty());
  }

  /**  Returns true if any of the non-target columns are translated.
   */
  bool has_translated_columns() const {
    return (untranslated_columns.size() != metadata()->num_columns(false));
  }

  /**
   * Returns the maximum row size present in the data.  This information is
   * calculated when the data is indexed and the ml_data structure is filled.
   * A buffer sized to this is guaranteed to hold any row encountered while
   * iterating through the data.
   */
  size_t max_row_size() const {
    return _max_row_size;
  }

  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Utility routines to convert ml_data to other formats.
  //
  ////////////////////////////////////////////////////////////////////////////////

  /**
   * Create a subsampled copy of the current ml_data structure.  This
   * allows us quickly create a subset of the data to be used for things
   * like sgd, etc.
   *
   * If n_rows < size(), exactly n_rows are sampled IID from the
   * dataset.  Otherwise, a copy of the current ml_data is returned.
   */
  ml_data create_subsampled_copy(size_t n_rows, size_t random_seed) const;

  /**
   *  Create a copy of the current ml_data structure, selecting the rows
   *  given by selection_indices.
   *
   *  \param selection_indices A vector of row indices that must be in
   *  sorted order.  Duplicates are allowed.  The returned ml_data
   *  contains all the rows given by selection_indices.
   *
   *  \return A new ml_data object with containing only the rows given
   *  by selection_indices.
   */
  ml_data select_rows(const std::vector<size_t>& selection_indices) const;

  /**
   *  Create a sliced copy of the current ml_data structure.  This
   *  copy is cheap.
   */
  ml_data slice(size_t start_row, size_t end_row) const;

  ////////////////////////////////////////////////////////////////////////////////
  // Serialization routines

  /** Get the current serialization format.
   */
  size_t get_version() const { return 1; }

  /** Remap all the block indices.
   */
  void _reindex_blocks(const std::vector<std::vector<size_t> >& reindex_maps);

 private:

  friend class ml_data_iterator;
  friend class ml_data_internal::ml_data_reconciler;
  friend void reconcile_distributed_ml_data(ml_data& data, const std::vector<std::string>&);

  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Internal data
  //
  ////////////////////////////////////////////////////////////////////////////////

  std::shared_ptr<ml_metadata> _metadata = nullptr;

  size_t _row_start                      = 0;
  size_t _row_end                        = 0;
  size_t _original_num_rows              = 0;
  size_t _max_row_size                   = 0;


  /** The row metadata.  This is what is needed to interact with the
   *  raw data contained in this data set, and gives the schema for
   *  the data laid out in the data_blocks variable below.
   */
  ml_data_internal::row_metadata rm;

  // The row block size.  Set so that each row is at least 64K.  This
  // balances the buffering and sorting speed with not using too much
  // memory at once.  This value is set initially on fill.
  size_t row_block_size = size_t(-1);

  /** The main storage container for the indexed, compactly
   *  represented rows.
   */
  std::shared_ptr<sarray<ml_data_internal::row_data_block> > data_blocks;

  /** The main storage container for untranslated columns.  These
   *  columns are not put through the indexer or anything else.
   */
  std::vector<std::shared_ptr<sarray<flexible_type> > > untranslated_columns;

  /** The block manager -- holds the readers, as well as a cache of
   *  currently referenced blocks.  Each block holds both the
   *  translated.
   */
  std::shared_ptr<ml_data_internal::ml_data_block_manager> block_manager;

  /** Convenience function to create the block manager given the
   *  current data in the model.
   */
  void _reset_block_manager();

  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Internal routines for setting up and filling the ml_data.  These
  //  are defined in ml_data_setup.cpp.
  //
  ////////////////////////////////////////////////////////////////////////////////

  /**  Sets the ml metadata for the whole class based on the options
   *   given.
   */
  void _setup_ml_metadata(const sframe& data,
                          const std::string& target_column_name,
                          const column_mode_map& mode_overrides);

  /**
   * Fill the ml_data structure with the raw data in raw_data.
   *
   * Only call this function when metadata and optionally target metadata are
   * set.  It uses them to import the data.
   *
   * \param[in] raw_data                     Input SFrame (with target column)
   * \param[in] track_statistics             Tracks stats (mean, variance etc.)
   * \param[in] allow_new_catgorical_values  Modify metadata for new categories?
   * \param[in] none_action                  Missing value action?
   *
   *
   * \note track_statistics and allow_new_catgorical_values when set to true
   * will modify the underlying metadata.
   *
   */
  void _fill_data_blocks(const sframe& raw_data,
                         bool immutable_metadata,
                         bool track_statistics,
                         ml_missing_value_action mva,
                         const std::pair<size_t, size_t>& row_bounds,
                         const std::set<std::string>& sorted_columns);

  /** Sets up the untranslated columns and column readers.
   */
  void _setup_untranslated_columns(const sframe& original_data, size_t row_lb, size_t row_ub);

  /**  Set up the untranslated column readers.
   */
  void _setup_untranslated_column_readers();

  ////////////////////////////////////////////////////////////////////////////////
  // Stuff for reconciling the ml_data stuff

  friend class ml_data_reconciler;


};

}

////////////////////////////////////////////////////////////////////////////////
// Implement serialization for
// std::shared_ptr<std::vector<sarray<ml_data_internal::entry_value> > >

BEGIN_OUT_OF_PLACE_SAVE(arc, std::shared_ptr<sarray<turi::ml_data_internal::row_data_block> >, m) {
  if(m == nullptr) {
    arc << false;
  } else {
    arc << true;
    arc << (*m);
  }
} END_OUT_OF_PLACE_SAVE()

BEGIN_OUT_OF_PLACE_LOAD(arc, std::shared_ptr<sarray<turi::ml_data_internal::row_data_block> >, m) {
  bool is_not_nullptr;
  arc >> is_not_nullptr;
  if(is_not_nullptr) {
    m.reset(new sarray<turi::ml_data_internal::row_data_block>);
    arc >> (*m);
  } else {
    m = std::shared_ptr<sarray<turi::ml_data_internal::row_data_block> >(nullptr);
  }
} END_OUT_OF_PLACE_LOAD()

// A few includes for convenience.

#include <ml/ml_data/ml_data_iterator.hpp>

#endif

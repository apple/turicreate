/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ML2_DATA_H_
#define TURI_ML2_DATA_H_

#include <vector>
#include <memory>
#include <sframe/sframe.hpp>
#include <unity/lib/extensions/option_manager.hpp>
#include <unity/toolkits/ml_data_2/metadata.hpp>
#include <unity/toolkits/ml_data_2/ml_data_entry.hpp>
#include <unity/toolkits/ml_data_2/ml_data_column_modes.hpp>
#include <unity/toolkits/ml_data_2/data_storage/ml_data_row_format.hpp>
#include <unity/toolkits/ml_data_2/side_features.hpp>

#include <Eigen/SparseCore>
#include <Eigen/Core>

namespace turi { namespace v2 {

class ml_data_iterator;
class ml_data_block_iterator;
class ml_data_iterator_base;

namespace ml_data_internal { 
class ml_data_block_manager;
}

/*********************************************************************************
 *
 * Row based, SFrame-Like Data storage for Learning and Optimization tasks.
 * ================================================================================
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
 * ml_data v2 design.
 *
 * ml_data loads data from an existing sframe, indexes it by mapping all
 * categorical values to numerical index values, and records statistics
 * about the values. It then puts it into an efficient row-based data
 * storage structure for use in learning algorithms that need fast
 * row-wise iteration through the features and target. The row based
 * storage structure is designed for fast iteration through the rows and
 * target.  ml_data also speeds up data access via caching and a compact
 * layout.
 *
 * Since ml_data is now used extensively in the different toolkits, a
 * redesign of the interface is needed.
 *
 * The current ml_data design have a number of issues:
 * - Confusing to construct.
 * - Metadata is confusing to work with.
 * - Not easily extendible (e.g. with other indexing strategies).
 * - The code is difficult to dive into.
 *
 * The new design addresses some of these:
 *
 * - API for construction is greatly simplified.
 * - API for saving and working with the metadata is greatly simplified.
 * - Indexing and Statistics tracking are easy to extend.
 *
 * Illustration of the new API:
 * ================================================================================
 *
 * Using ml_data.
 * --------------------------------------------------------------------------------
 *
 * There are a number of use cases for ml_data.  The following should
 * address the current use cases.
 *
 * To construct the data at train time:
 * ++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 *     // Constructs an empty ml_data object
 *     ml_data data(options);
 *
 *     // Sets the data source from X, with target_column_name being the
 *     // target column.  (Alternatively, target_column_name may be a
 *     // single-column SFrame giving the target.  "" denotes no target
 *     // column present).
 *     data.set_data(X, target_column_name);
 *
 *     // Finalize the filling.
 *     data.fill();
 *
 *     // After filling, a serializable shared pointer to the metadata
 *     // can be saved for the predict stage.  this->metadata is of type
 *     // std::shared_ptr<ml_metadata>.
 *     this->metadata = data.metadata();
 *
 *
 * To iterate through the data, single threaded.
 * ++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 *     for(auto it = data.get_iterator(); !it.done(); ++it) {
 *        ....
 *        it.target_value();
 *        it.fill_observation(...);
 *     }
 *
 *
 * To iterate through the data, threaded.
 * ++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 *     in_parallel([&](size_t thread_idx, size_t num_threads) {
 *
 *       for(auto it = data.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
 *          ....
 *          it.target_value();
 *          it.fill_observation(...);
 *       }
 *     });
 *
 *
 * To construct the data at predict time:
 * ++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 *     // Constructs an empty ml_data object, takes construction options
 *     // from original ml_data.
 *     ml_data data(this->metadata);
 *
 *     // Sets the data source from X, with no target column.
 *     data.set_data(X, "");
 *
 *     // Finalize the filling.
 *     data.fill();
 *
 * To construct the data at predict time, with tracking of new
 * categorical variables.
 * ++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * There is currently no use case for the data statistics (column means,
 * std dev, count, etc.) to change after training.  However, some models,
 * e.g. recsys, need to change parts of the metadata -- e.g. track new
 * categories.  Thus we allow this part of the metadata to change.
 *
 *     // Constructs an empty ml_data object, takes construction options
 *     // from original ml_data.  The "true" here says that the metadata
 *     // indexing should be mutable, allowing new categories to be
 *     // tracked (this is needed for recsys).
 *     ml_data data(this->metadata, true);
 *
 *     // Sets the data source from X, with no target column.
 *     data.set_data(X, "");
 *
 *     // Finalize the filling.
 *     data.fill();
 *
 *
 * To serialize the metadata for model serialization
 * ++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 *     // Type std::shared_ptr<ml_metadata> is fully serializable.
 *     oarc << this->metadata;
 *
 *     iarc >> this->metadata;
 *
 * To add side data at construction
 * ++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 *     // Constructs an empty ml_data object
 *     ml_data data(options);
 *
 *     // Sets the data source from X, with target_column_name being the
 *     // target column.
 *     data.set_data(X, target_column_name);
 *
 *     // Sets the data source from X2
 *     data.add_side_data(X2);
 *
 *     // Finalize the filling.
 *     data.fill();
 *
 *     // After filling, a serializable shared pointer to the metadata
 *     // can be saved for the predict stage.  This metadata contains the
 *     // side features.
 *     this->metadata = data.metadata();
 *
 *
 * To access statistics at train/predict time.
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Statistics about each of the columns is fully accessible at any point
 * after training time, and does not change.  This is stored with the
 * metadata.
 *
 *
 *     // The number of columns, including side features.  column_index
 *     // below is between 0 and this value.
 *     this->metadata->num_columns();
 *
 *     // This gives the size of the column at train time.  Will never
 *     // change after training time.  For categorical types, it gives
 *     // the number of categories at train time.  For numerical it is 1
 *     // if scalar and the width of the vector if numeric.  feature_idx
 *     // below is between 0 and this value.
 *     this->metadata->column_size(column_index);
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
 *
 * Forcing the ordering of certain columns
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * As the SFrame is intended to work with column names, ml_data may
 * reorder the columns in the original SFrame for optimization or
 * convenience reasons.  This ordering will always be consistent, even if
 * the column orderings in the data SFrame change between train and
 * test. To force ml_data to put some columns at the start, a partial
 * column ordering may be passed to set_data(...) to force certain
 * columns to come first.  For example, to force the "user_id" column to
 * come first, and the "item_id" column to come second, do
 *
 *      data.set_data(recsys_data, "rating", {"user_id", "item_id"});
 *
 * These columns are guaranteed to be first.
 *
 *
 * Forcing certain column modes.
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
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
 *      data.set_data(recsys_data, "rating",
 *                    {"user_id", "item_id"},
 *                    {{"user_id", column_mode::CATEGORICAL},
 *                     {"item_id", column_mode::CATEGORICAL}});
 *
 *
 * Customizing the behavior of ml_data
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * The options parameter of the constructor provides a set of possible
 * options that can get passed in to the ml_data class and govern how the
 * model is created, which in turn control the functionality available
 * later on.
 *
 *
 * Separating out Train and Predict Modes
 * ---------------------------------------
 *
 * In practical use of the current ml_data, it seems wise to distinguish
 * between “train” and “predict” modes.  Train mode is when the ml_data
 * class creates the metadata class as part of it’s construction/filling.
 * Predict mode is when the ml_data class uses an existing ml_metadata
 * class obtained from the ml_data structure after it was filled.  This
 * ml_metadata class can be saved/loaded or used for multiple training.
 *
 * The reason it is important to distinguish between these two cases is
 * based on the following observations about the current usage and the
 * design decision.
 *
 * First, the main practical way the training mode and predict mode are
 * different is that in predict mode, all the rows in the original SFrame
 * are expected to be represented in the output SFrame in the same order
 * as the original SFrame.  Thus the ml_data structure must also preserve
 * this ordering.  However, reordering rows at train time is often
 * needed.  SGD needs the data shuffled, and recsys needs it sorted by
 * user/item.
 *
 * Second, for simplicity, the options are set once at ml_data creation,
 * at train time.  Following that, the current options for the ml_data
 * structure are stored with the metadata.  Practically, this means that
 * the options for setting up the ml_data class are consolidated into one
 * place, but has the side effect that some options are specific for the
 * training time and others for the predict time, as noted in the first
 * point.
 *
 * Thus, some of the options apply only at train time and some only at
 * predict time.  Options labeled with _on_train or _on_predict only
 * apply at train or predict time -- the rest apply to both modes.
 *
 * Data ordering options
 * ----------------------------------------
 *
 * - "sort_by_first_two_columns_on_train":
 *
 *   If true (default = false), then for the training data set, sort the
 *   rows by the feature indices of the first two columns.  The first two
 *   columns must be categorical.  This ensures that all rows with equal
 *   first column are in a group.  (Used by recsys, matrix factorization
 *   for ranking, etc.).
 *
 *   This option is only relevant at train time; data for predict/test is
 *   not reordered.
 *
 * - "sort_by_first_two_columns":
 *
 *   If true (default = false), then always sort the data by the first
 *   two columns in similar fashion to that above.
 *
 * - "shuffle_rows_on_train":
 *
 *   If true (default = false), then for the training data set, do a
 *   simple random shuffle of the input rows. If sort is also on, then
 *   the order of the index mapping is random.
 *
 *   This option is only relevant at train time; data for predict/test is
 *   not reordered.
 *
 * - "shuffle_rows":
 *
 *   If true (default = false), then always do a simple random shuffle of
 *   the input rows. If sort is also on, then the order of the index
 *   mapping is random.
 *
 *
 * Indexing options
 * ----------------------------------------
 *
 * - "column_indexer_type".
 *
 *   Gives the type of the indexer to use on the columns (default =
 *   "unique").  Currently, only "unique" is available, but "hash" will
 *   be supported in the future.  (See Extending Column Indexing below to
 *   create your own indexer).
 *
 * - "target_column_indexer_type".
 *
 *   Gives the type of the indexer to use on the target columns (default = "unique").
 *
 * - "integer_columns_categorical_by_default".
 *
 *   By default, integer columns are treated as numeric.  If this option
 *   is true (default = false), then they are treated as categorical.
 *
 * Missing value options
 * ----------------------------------------
 *
 * - "missing_value_action_on_train"
 *
 *   This option controls what the default missing value behavior will be
 *   at training time (default = "error").  Currently, only "error" is
 *   supported at train time, but other options, e.g. "NAN", will be
 *   supported in the future.
 *
 * - "missing_value_action_on_predict",
 *
 *   This option controls what the action on missing value after the
 *   train stage should be (default = "impute").  Currently, only
 *   "impute" and "error" are supported.
 *
 * Error checking options
 * ----------------------------------------
 *
 * - "target_column_always_numeric"
 *
 *   If true (default), then the target column must be a numeric scalar
 *   column.  If not, then an error is raised.
 *
 * Extending Indexing and Statistics
 * =============================================================================
 *
 * The current design is set up to make extending the indexer and the
 * statistics trackers easy.
 *
 * To extend the indexer:
 *
 *  1. Subclass from column_indexer, given in indexing/column_indexer.hpp.
 *     Implement the appropriate virtual functions.
 *
 *  2. Register the class by adding a line to
 *     indexing/column_indexer_factory.cpp so it can get instantiated by
 *     name.
 *
 * The same can be done with statistics -- inherit column_statistics, as
 * given in statistics/column_statistics.hpp, and modify
 * statistics/column_statistics_factory.cpp.
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
 *     sframe X = make_integer_testing_sframe( {"C1", "C2"}, { {0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4} } );
 *
 *     v2::ml_data data;
 *
 *     data.set_data(X, "", {}, { {"C2", v2::ml_column_mode::UNTRANSLATED} });
 *
 *     data.fill();
 *
 *
 *     std::vector<v2::ml_data_entry> x_d;
 *     std::vector<flexible_type> x_f;
 *
 *     ////////////////////////////////////////
 *
 *     for(auto it = data.get_iterator(); !it.done(); ++it) {
 *
 *       it.fill_observation(x_d);
 *
 *       ASSERT_EQ(x_d.size(), 1);
 *       ASSERT_EQ(x_d[0].column_index, 0);
 *       ASSERT_EQ(x_d[0].index, 0);
 *       ASSERT_EQ(x_d[0].value, it.row_index());
 *
 *       it.fill_untranslated_values(x_f);
 *
 *       ASSERT_EQ(x_f.size(), 1);
 *       ASSERT_TRUE(x_f[0] == it.row_index());
 *     }
 *
 */
class ml_data {
 public:

  // ml_data is cheap to copy.  However, it cannot be copied before
  // fill() is called.

  ml_data(const ml_data&);
  const ml_data& operator=(const ml_data&);
  ml_data& operator=(ml_data&&) = default;

  ml_data(ml_data&&) = default;

  /** Default option list.  See above for explanation.
   */
  static std::map<std::string, flexible_type> default_options() {
    return {
    /**
     *
     */
      {"sort_by_first_two_columns_on_train",     false},
      {"sort_by_first_two_columns",              false},

      {"shuffle_rows_on_train",                  false},
      {"shuffle_rows",                           false},

      {"column_indexer_type",                    "unique"},
      {"column_statistics_type",                 "basic-dense"},

      {"missing_value_action_on_train",          "error"},
      {"missing_value_action_on_predict",        "impute"},

      {"integer_columns_categorical_by_default", false},

      {"target_column_always_numeric",           false},
      {"target_column_always_categorical",       false},

      {"target_column_indexer_type",             "unique"},

      {"target_column_statistics_type",          "basic-dense"},

      {"uniquify_side_column_names",             false},

      {"ignore_new_columns_after_train",         false}

    };
  }

  /**
   *  Construct an ml_data object based on previous ml_data metadata.
   */
  explicit ml_data(const std::shared_ptr<ml_metadata>& metadata,
                   bool immutable_metadata = false);

  /// STUPID CLANG PARSING BUG
  typedef std::map<std::string, flexible_type> flex_map; 
  
  /**
   *   Construct an ml_data object based current options.
   */
  explicit ml_data(const std::map<std::string, flexible_type>& options = flex_map());

  /**
   *   Special case the explicit initializer list to overload to the map of options.
   */
  explicit ml_data(std::initializer_list<std::map<std::string, flexible_type>::value_type> l)
      : ml_data(std::map<std::string, flexible_type>(l))
  {}

  /// This is here to get around 2 clang bugs!
  typedef std::map<std::string, ml_column_mode> column_mode_map; 
  
  /**  Sets the data source.
   *
   *   If target_column is null, then there is no target column.
   */
  void set_data(const sframe& data,
                const std::string& target_column = "",
                const std::vector<std::string>& partial_column_ordering = std::vector<std::string>(),
                const column_mode_map mode_overrides = column_mode_map());

  /**  Sets the data source.
   *
   *   An overload of the previous one.  Here, the target is supplied separately
   *   as a one-column sframe.
   *
   */
  void set_data(const sframe& data,
                const sframe& target,
                const std::vector<std::string>& partial_column_ordering = std::vector<std::string>(),
                const column_mode_map mode_overrides = column_mode_map());

  /**  Add in the side data to the mix.  If forced_join_column is
   *   given, that column must be present and the one to join on.
   *   Otherwise, there must be exactly one column in common between
   *   the main data and the side data.
   */
  void add_side_data(const sframe& data,
                     const std::string& forced_join_column = "",
                     const column_mode_map mode_overrides = column_mode_map());


  /** Convenience function -- short for calling set_data(data,
   * target_column), then fill().
   */
  void fill(const sframe& data,
            const std::string& target_column = "");


  /** Convenience function -- short for calling set_data(data,
   *  target), then fill().
   */
  void fill(const sframe& data,
            const sframe& target);


  /** Call this function when all the data is added.  This executes
   *  the filling process based on everything given.
   */
  void fill();


  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Metadata access
  //
  ////////////////////////////////////////////////////////////////////////////////

  /** Returns True if the ml_data structure has been created
   *  completely and is ready to use.
   */
  inline bool creation_complete() const { return incoming_data == nullptr; }

  /** Direct access to the metadata.
   */
  inline const std::shared_ptr<ml_metadata>& metadata() const {
    return _metadata;
  }

  /** Returns the number of columns present, including any possible
   * side columns.
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

  /**
   * Returns the maximum row size present in the data.  This information is
   * calculated when the data is indexed and the ml_data structure is filled.
   * A buffer sized to this is guaranteed to hold any row encountered while
   * iterating through the data.
   */
  size_t max_row_size() const;

  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Iteration Access
  //
  ////////////////////////////////////////////////////////////////////////////////

  /** Return an iterator over part of the data.  See
   *  iterators/ml_data_iterator.hpp for documentation on the returned
   *  iterator.
   */
  ml_data_iterator get_iterator(
      size_t thread_idx=0, size_t num_threads=1,
      bool add_side_information_if_present = true,
      bool use_reference_encoding = false) const;


  /** Return a block iterator over part of the data.  See
   *  iterators/ml_data_block_iterator.hpp for documentation on the returned
   *  iterator.
   */
  ml_data_block_iterator get_block_iterator(
      size_t thread_idx=0, size_t num_threads=1,
      bool add_side_information_if_present = true,
      bool use_reference_encoding = false) const;

  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Utility routines to handle side data
  //
  ////////////////////////////////////////////////////////////////////////////////

  /**  Returns the current side features that work with this class.
   */
  std::shared_ptr<ml_data_side_features> get_side_features() const {

    DASSERT_TRUE(side_features != nullptr);
    return side_features;
  }

  /**  Returns true if a target column is present, and false otherwise.
   */
  bool has_target() const {
    return rm.has_target;
  }

  /**  Returns true if there are side features, and false otherwise
   */
  bool has_side_features() const {
    return (side_features != nullptr);
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

  typedef Eigen::Matrix<double, Eigen::Dynamic,1>  DenseVector;
  typedef Eigen::SparseVector<double> SparseVector;

  /** Translates the ml_data_entry row format to the original flexible
   *  types.
   */
  std::vector<flexible_type> translate_row_to_original(const std::vector<ml_data_entry>& v) const;

  /** Translates the ml_data_entry_global_index row format to the original flexible
   *  types.
   */
  std::vector<flexible_type> translate_row_to_original(const std::vector<ml_data_entry_global_index>& v) const;
  
  /** Translates the original dense row format to the original flexible
   *  types.
   */
  std::vector<flexible_type> translate_row_to_original(const DenseVector& v) const;

  /** Translates the original dense row format to the original flexible
   *  types.
   */
  std::vector<flexible_type> translate_row_to_original(const SparseVector& v) const;


  ////////////////////////////////////////////////////////////////////////////////
  // Direct access to creating and working with the indexers


  typedef std::shared_ptr<ml_data_internal::column_indexer> indexer_type;

  /** Occasionally, we need to create a tempororay indexer for a
   *  specific column.  This allows us to do just that.
   *
   */
  static indexer_type create_indexer(
      const std::string& column_name,
      ml_column_mode mode,
      flex_type_enum column_type,
      const std::string& indexer_type = "unique",
      const std::map<std::string, flexible_type>& options = flex_map());


 private:
  void _check_is_iterable() const;

 public:


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

  /**
   *  Create a sliced copy of the current ml_data structure, with the
   *  slice indices referenced from the original structure
   */
  ml_data absolute_slice(size_t start_row, size_t end_row) const;


  ////////////////////////////////////////////////////////////////////////////////
  // Serialization routines

  /** Get the current serialization format.
   */
  size_t get_version() const { return 1; }

  /**
   * Serialize the object (save).
   */
  void save(turi::oarchive& oarc) const;

  /**
   * Load the object.
   */
  void load(turi::iarchive& iarc);

 private:

  friend class ml_data_iterator_base;

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

  /** The current side features.  This may be different from the
   *  original side features if additional data has been provided.
   *
   */
  std::shared_ptr<ml_data_side_features> side_features = nullptr;


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
   *  translated and untranslated columns.
   */
  std::shared_ptr<ml_data_internal::ml_data_block_manager> block_manager;

  /** Convenience function to create the block manager given the
   *  current data in the model.
   */
  void _create_block_manager(); 
  
  
  
  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Temporary variables to hold the filling parameters.
  //
  ////////////////////////////////////////////////////////////////////////////////

  struct _data_for_filling {

    // This is moved to the metadata at creation time.
    std::map<std::string, flexible_type> options;

    bool immutable_metadata;

    sframe data;
    std::string target_column_name;

    /** Column ordering holds a partial ordering of the incoming
     * columns.  Can be empty, in which case the columns are chosen
     * arbitrarily.
     */
    std::vector<std::string> column_ordering;


    typedef std::map<std::string, ml_column_mode> mode_override_map;

    mode_override_map mode_overrides;

    struct incoming_side_feature {
      sframe data;
      std::string forced_join_column;
      mode_override_map mode_overrides;
    };

    std::vector<incoming_side_feature> incoming_side_features;
  };

  std::unique_ptr<_data_for_filling> incoming_data = nullptr;


  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Internal routines for setting up and filling the ml_data.  These
  //  are defined in ml_data_setup.cpp.
  //
  ////////////////////////////////////////////////////////////////////////////////

  /**  Sets the ml metadata for the whole class based on the options
   *   given.
   *
   */
  void _setup_ml_metadata();

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
  void _fill_data_blocks(bool in_training_mode);

  /** Sets up the untranslated columns and column readers.
   */
  void _setup_untranslated_columns(const sframe& original_data);

  /**  Set up the untranslated column readers.
   */
  void _setup_untranslated_column_readers();

  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Internal routines for sorting the ml_data.  These are defined in
  //  ml_data_sorting.cpp.
  //
  ////////////////////////////////////////////////////////////////////////////////

  std::unique_ptr<ml_data_iterator> _merge_sorted_ml_data_sources(
      const std::vector<std::unique_ptr<ml_data_iterator> >& sources);

  void _sort_user_item_data_blocks();
};

}}

////////////////////////////////////////////////////////////////////////////////
// Implement serialization for
// std::shared_ptr<std::vector<sarray<ml_data_internal::entry_value> > >

BEGIN_OUT_OF_PLACE_SAVE(arc, std::shared_ptr<sarray<turi::v2::ml_data_internal::row_data_block> >, m) {
  if(m == nullptr) {
    arc << false;
  } else {
    arc << true;
    arc << (*m);
  }
} END_OUT_OF_PLACE_SAVE()

BEGIN_OUT_OF_PLACE_LOAD(arc, std::shared_ptr<sarray<turi::v2::ml_data_internal::row_data_block> >, m) {
  bool is_not_nullptr;
  arc >> is_not_nullptr;
  if(is_not_nullptr) {
    m.reset(new sarray<turi::v2::ml_data_internal::row_data_block>);
    arc >> (*m);
  } else {
    m = std::shared_ptr<sarray<turi::v2::ml_data_internal::row_data_block> >(nullptr);
  }
} END_OUT_OF_PLACE_LOAD()


#endif

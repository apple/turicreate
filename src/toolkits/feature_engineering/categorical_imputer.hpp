/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef categorical_imputer_INDEXER_H_
#define categorical_imputer_INDEXER_H_
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <core/export.hpp>

#include <model_server/lib/toolkit_class_macros.hpp>
#include <toolkits/feature_engineering/transformer_base.hpp>

namespace turi {
namespace sdk_model {
namespace feature_engineering {

class EXPORT categorical_imputer : public transformer_base {

  // Version of the imputer
  static constexpr size_t CATEGORICAL_IMPUTER_VERSION = 0;
  static constexpr char CLUSTER_ID[] = "__internal__cluster_id";
  static constexpr char CLUSTER_DISTANCE[] = "__internal__cluster_centroid_distance";
  static constexpr char LABEL_COUNT[] = "__internal__label_count";
  static constexpr char MAX_LABEL[] = "__internal__max_label";
  static constexpr char FIXED_LABEL[] = "__internal__fixed_label";
  static constexpr char COUNT_OF_LABELS[] = "__internal__count_of_labels";
  static constexpr char MAX_OF_LABELS[] = "__internal__max_of_labels";
  static constexpr char PREDICTED_COLUMN_PREFIX[] = "predicted_feature_";
  static constexpr char PROBABILITY_COLUMN_PREFIX[] = "feature_probability_";

  // Map from internal-label to user-label
  std::unordered_map<int64_t, flexible_type> label_map;

  // Map from user-label to internal-label
  std::unordered_map<flexible_type, int64_t> reverse_label_map;

  // Features valid for label propagation
  std::unordered_set<std::string> label_propagation_features_set;

  // Was fit() called?
  bool fitted = false;

  // User-provided inputs
  flexible_type dependent_feature_columns; // Columns to use as features
  std::string feature_column;              // Column to impute
  flex_type_enum feature_column_type;      // Type of the column to impute
  bool exclude = false;                    // Are some columns to be excluded?
  bool verbose = false;                    // Verbose output?

  /**
   * Utility method to convert an sframt into a gl_sframe
   *
   * \param sframe An SFrame
   *
   * \returns a gl_sframe wrapping the SFrame
   */
  gl_sframe from_sframe(const sframe& sframe);

  /**
   * Utility method to retrieve the index of a column in an SFrame
   *
   * \param sfram An SFrame
   * \param column_name Column to get the index of
   * \returns The index of the column_name column if found in the SFrame,
   *          otherwise (size_t)(-1).
   */
  size_t get_column_index(const gl_sframe& sframe,
                          const std::string& column_name);

  /**
   * Calls the kmeans toolkit and assigns a cluster ID to each user-provided
   * row of data.
   *
   * \param data The user-supplied SFrame of data, containing all the columns
   * \param use_centroids Use the provided centroids as starting points
   * \param gl_clustered_user_data (output) The user data with cluster IDs
   * \param gl_centroids (output) Centroid IDs and features
   */
  void call_kmeans(
      gl_sframe data,
      bool use_centroids,
      gl_sframe* gl_clustered_user_data,
      gl_sframe* gl_centroids);

  /**
   * Use ARGMAX to assign a label to each cluster computed by kmeans
   *
   * \param gl_clustered_user_data User-provided data with cluster_id column
   * \param gl_centroids Centroids with features
   * \param gl_centroid_with_label (output) Centroids with computed label
   */
  void compute_cluster_argmax_label(
      gl_sframe gl_clustered_user_data,
      gl_sframe gl_centroids,
      gl_sframe* gl_centroid_with_label);

  /**
   * Returns wether all the centroids have an assigned label. If they
   * all do, we don't need to perform label propagation.
   *
   * \param gl_centroid_with_label Centroids marked with labels
   * \returns True if all centroids have an assigned label
   */
  bool all_centroids_labeled(gl_sframe gl_centroid_with_label);

  /**
   * Renames the cluster labels from the user-provided labels to
   * numbers from [0, N) as required by the label_propagation toolkit
   *
   * \param gl_centroid_with_label A new column "fixed_label" is added
   */
  void rename_labels(gl_sframe* gl_centroid_with_label);

  /**
   * Builds the distance graph between every centroid, allowing us to run
   * label propagation between the vertices
   *
   * \param gl_centroid_with_label Centroids with fixed labels
   * \returns An SGraph with every centroid-centroid distances computed
   */
  gl_sgraph build_distance_graph(gl_sframe gl_centroid_with_label);

  /**
   * Calls the label_propagation toolkit to fill in the missing labels
   * for all centroids.
   *
   * \param centroid_graph Centroid graph, output of build_distance_graph
   * \returns An SFrame with centroid and propagated labels
   */
  gl_sframe call_label_propagation(gl_sgraph centroid_graph);

  /**
   * The output of the label propagation module is one probability for
   * every possible label for each row; we really just want the probability
   * for the selected label. This method generates that column with the
   * probability of the chosen label.
   *
   * \param label_propagation_output Output of label propagation, a new
   *        column (label_probability) will be added
   * \returns An SFrame with Cluster Id, Predicted Label and Label Probability
   */
  gl_sframe get_prediction_probability(gl_sframe* label_propagation_output);

  /**
   * Joins the user provided data with the computed labels, making sure to
   * keep the user labels instead of centroid labels as well as replacing the
   * labels back into user-provided label-space (instead of [0, N) ).
   *
   * \param gl_clustered_user_data User data with Cluster ID column
   * \param clusters_with_predictions Output of get_prediction_probability
   * \returns The User's SFrame with the predicted_label and label_probability
   *          columns added.
   */
  gl_sframe join_user_data_and_predictions(
      gl_sframe gl_clustered_user_data,
      gl_sframe clusters_with_predictions);

  /**
   * In the case where every centroid has a label attached, we can skip the
   * label propagation step.
   *
   * \param gl_clustered_user_data User data with Cluster IDs
   * \param gl_centroid_with_label Cluster IDs with labels
   * \returns The User's SFrame with the predicted_label and label_probability
   *          columns added.
   */
  gl_sframe join_user_data_and_kmeans_output(
      gl_sframe gl_clustered_user_data,
      gl_sframe gl_centroid_with_label);

  public:

  /**
   * Methods that must be implemented in a new transformer model.
   * -------------------------------------------------------------------------
   */

  virtual inline ~categorical_imputer() {}

  /**
   * Set one of the options in the model. Use the option manager to set
   * these options. If the option does not satisfy the conditions that the
   * option manager has imposed on it. Errors will be thrown.
   *
   * \param[in] options Options to set
   */
  void init_options(const std::map<std::string, flexible_type>&_options) override;

  /**
   * Get a version for the object.
   */
  size_t get_version() const override;

  /**
   * Save the object using Turi's oarc.
   */
  void save_impl(turi::oarchive& oarc) const override;

  /**
   * Load the object using Turi's iarc.
   */
  void load_version(turi::iarchive& iarc, size_t version) override;


  /**
   * Initialize the transformer.
   */
  void init_transformer(const std::map<std::string, flexible_type>& _options) override;

  /**
   * Set constant.
   *
   * \param[in] data  (SFrame of data)
   */
  void fit(gl_sframe data) override;

  /**
   * Transform the given data.
   *
   * \param[in] data  (SFrame of data)
   *
   * Python side interface
   * ------------------------
   * This function directly interfaces with "transform" in python.
   *
   */
  gl_sframe transform(gl_sframe data) override;

  /**
   * Fit and transform the given data. Intended as an optimization because
   * fit and transform are usually always called together. The default
   * implementaiton calls fit and then transform.
   *
   * \param[in] data  (SFrame of data)
   */
  gl_sframe fit_transform(gl_sframe data) {
     data.materialize();
     fit(data);
     return transform(data);
  }

  // Functions that all transformers need to register. Can be copied verbatim
  // for other classes.
  // --------------------------------------------------------------------------
  BEGIN_CLASS_MEMBER_REGISTRATION("_CategoricalImputer")
  REGISTER_CLASS_MEMBER_FUNCTION(categorical_imputer::init_transformer, "_options");
  REGISTER_CLASS_MEMBER_FUNCTION(categorical_imputer::fit, "data");
  REGISTER_CLASS_MEMBER_FUNCTION(categorical_imputer::fit_transform, "data");
  REGISTER_CLASS_MEMBER_FUNCTION(categorical_imputer::transform, "data");
  REGISTER_CLASS_MEMBER_FUNCTION(categorical_imputer::get_current_options);
  REGISTER_CLASS_MEMBER_FUNCTION(categorical_imputer::list_fields);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("_get_default_options",
      categorical_imputer::get_default_options);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("get",
      categorical_imputer::get_value_from_state, "key");
  END_CLASS_MEMBER_REGISTRATION

};


} // feature_engineering
} // sdk_model
} // turicreate
#endif

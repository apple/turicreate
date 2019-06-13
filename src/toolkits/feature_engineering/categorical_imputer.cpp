/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>

#include <sgraph/sgraph.hpp>

#include <unity/lib/gl_sgraph.hpp>
#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/unity_sgraph.hpp>
#include <unity/lib/variant_deep_serialize.hpp>

#include <unity/toolkits/clustering/kmeans.hpp>

#include <unity/toolkits/feature_engineering/transform_utils.hpp>
#include <unity/toolkits/feature_engineering/categorical_imputer.hpp>


/***************************************************************************/
/*                                                                         */
/* Forward declaration of stuff                                            */
/*                                                                         */
/***************************************************************************/

/**
 * This class relies on the label propagation framework, which is not
 * directly shared. This little hack allows us to access its internal
 * methods ;)
 */
namespace turi {
namespace label_propagation {

/**
 * Setup the label propagation framework.
 *
 * \param params A dictionary of properties to pass to the framework
 * (WILL be copied to global variables interanlly, _not_ thread safe)
 */
void setup(variant_map_type& params);

/**
 * Runs the label propagation on a given graph
 *
 * \param g The graph to be used; will be modified by the end of the computation
 *
 * \param num_iter The number of iterations that the model has run
 *
 * \param average_l2_delta Average L2 Normalization delta
 */
template<typename FLOAT_TYPE>
  void run(sgraph& g, size_t& num_iter, double& average_l2_delta);

} // namespace pagerank
} // namespace turi


/***************************************************************************/
/*                                                                         */
/* Actual Class                                                            */
/*                                                                         */
/***************************************************************************/

namespace turi {
namespace sdk_model {
namespace feature_engineering {

/***************************************************************************/
/*                                                                         */
/* Private  Methods                                                        */
/*                                                                         */
/***************************************************************************/

constexpr char categorical_imputer::CLUSTER_ID[];
constexpr char categorical_imputer::CLUSTER_DISTANCE[];
constexpr char categorical_imputer::LABEL_COUNT[];
constexpr char categorical_imputer::MAX_LABEL[];
constexpr char categorical_imputer::FIXED_LABEL[];
constexpr char categorical_imputer::COUNT_OF_LABELS[];
constexpr char categorical_imputer::MAX_OF_LABELS[];
constexpr char categorical_imputer::PREDICTED_COLUMN_PREFIX[];
constexpr char categorical_imputer::PROBABILITY_COLUMN_PREFIX[];

/**
 * Utility method to convert an sframt into a gl_sframe
 */
gl_sframe categorical_imputer::from_sframe(const sframe& sframe) {
  std::shared_ptr<unity_sframe> usf = std::make_shared<unity_sframe>();
  usf->construct_from_sframe(sframe);
  return gl_sframe(usf);
}

/**
 * Utility method to retrieve the position of a column in an SFrame,
 * this is an O(n) operation in the number of columns.
 */
size_t categorical_imputer::get_column_index(
    const gl_sframe& sframe, const std::string& column_name) {
  for (size_t i = 0; i < sframe.column_names().size(); ++i) {
    if (sframe.column_names()[i] == column_name) {
      return i;
    }
  }

  return (size_t)(-1);
}

/**
 * Runs the KMeans model on the appropriate columns of the data.
 */
void categorical_imputer::call_kmeans(
    gl_sframe data,
    bool use_centroids,
    gl_sframe* gl_clustered_user_data,
    gl_sframe* gl_centroids) {
  // Retrieve the list of features to use for KMeans
  std::vector<std::string> kmeans_features =
            variant_get_value<std::vector<std::string>>(state.at("reference_features"));

  // Prepare the inputs to the k-means module; number of clusters is
  // computed totally arbitrarily based on common knowledge
  std::map<std::string, flexible_type> kmeans_options;
  kmeans_options["verbose"] = verbose;
  kmeans_options["num_clusters"] = (int)(std::min(1 + (int)sqrt(data.size() / 2), 5000));
  kmeans_options["max_iterations"] = 10;
  kmeans_options["method"] = "minibatch";
  kmeans_options["batch_size"] = (int)std::min(1 + data.size() / 4, (size_t)2000);

  sframe user_data = *data.get_proxy()->get_underlying_sframe().get();
  sframe user_data_kmeans_subset = user_data.select_columns(kmeans_features);

  sframe initialcenters;
  if (use_centroids) {
    initialcenters = *gl_centroids->get_proxy()->get_underlying_sframe().get();
    kmeans_options["num_clusters"] = initialcenters.size();
    kmeans_options["max_iterations"] = 0;
  }

  kmeans::kmeans_model kmmodel;
  kmmodel.init_options(kmeans_options);

  kmmodel.train(user_data_kmeans_subset, initialcenters, "minibatch", true);

  sframe clusters = kmmodel.get_cluster_assignments();
  sframe centroids = kmmodel.get_cluster_info();

  // This is the user data
  sframe clustered_user_data =
      user_data.add_column(clusters.select_column("cluster_id"), CLUSTER_ID);

  clustered_user_data =
      clustered_user_data.add_column(
            clusters.select_column("distance"), CLUSTER_DISTANCE);

  /**
   * At this point, we have successfuly run k-means on the data and
   * assigned clusters to each row of the original user data
   */

  *gl_clustered_user_data = from_sframe(clustered_user_data);
  *gl_centroids = from_sframe(centroids);

  gl_centroids->rename({{"cluster_id", CLUSTER_ID}});
}

/**
 * Assigns a label to each centroid based on argmax on the original
 * user-provided data
 */
void categorical_imputer::compute_cluster_argmax_label(
    gl_sframe gl_clustered_user_data,
    gl_sframe gl_centroids,
    gl_sframe* gl_centroid_with_label) {

  /**
   * We find, for each cluster, the most common label, and assign it to
   * the whole cluster (excluding None, since it will likely be the
   * most common everywhere on sparse data)
   */
  gl_sframe cluster_with_label_count =
      gl_clustered_user_data.groupby(
          {CLUSTER_ID, feature_column},
          {{LABEL_COUNT, aggregate::COUNT()}});

  /**
   * Drop  the NA from the table, since we don't want the to be part of the
   * argmax computation
   */
  gl_sframe cluster_with_label_count_no_na =
      cluster_with_label_count.dropna({feature_column});

  // Cluster ID, Label, Count
  gl_sframe cluster_with_argmax_label =
      cluster_with_label_count_no_na.groupby(
          {CLUSTER_ID},
          {{MAX_LABEL, aggregate::ARGMAX(LABEL_COUNT, feature_column)}});

  // Cluster ID, Sum of Count
  gl_sframe  cluster_with_counts_of_labels =
      cluster_with_label_count_no_na.groupby(
          {CLUSTER_ID},
          {{COUNT_OF_LABELS, aggregate::SUM(LABEL_COUNT)}});

  // Cluster ID, Max of Count
  gl_sframe  cluster_with_max_of_labels =
      cluster_with_label_count_no_na.groupby(
          {CLUSTER_ID},
          {{MAX_OF_LABELS, aggregate::MAX(LABEL_COUNT)}});

  *gl_centroid_with_label =
      gl_centroids
        .join(cluster_with_argmax_label, {CLUSTER_ID}, "left")
        .join(cluster_with_counts_of_labels, {CLUSTER_ID}, "left")
        .join(cluster_with_max_of_labels, {CLUSTER_ID}, "left");
}

/**
 * Returns wether all the centroids have an assigned label. If they
 * all do, we don't need to perform label propagation.
 */
bool categorical_imputer::all_centroids_labeled(
    gl_sframe gl_centroid_with_label) {
  size_t columnindex = get_column_index(gl_centroid_with_label, MAX_LABEL);
  if (columnindex == (size_t)(-1)) {
    log_and_throw("No label column could be computed");
  }

  for (const auto& iter: gl_centroid_with_label.range_iterator()) {
    if (iter[columnindex].is_na()) {
      // At least one unlabeled centroid
      return false;
    }
  }

  // All labeled
  return true;
}

/**
 * Renames the cluster labels from the user-provided labels to
 * numbers from [0, N) as required by the label_propagation toolkit
 */
void categorical_imputer::rename_labels(
    gl_sframe* gl_centroid_with_label) {

  reverse_label_map.clear();
  label_map.clear();
  size_t max_label_column_index =
      get_column_index(*gl_centroid_with_label, MAX_LABEL);
  std::vector<flexible_type> new_labels;

  int64_t current = 0;
  for (const auto& iter: gl_centroid_with_label->range_iterator()) {
    const flexible_type& rowlabel = iter[max_label_column_index];
    if (rowlabel.is_na()) {
      new_labels.push_back(rowlabel);
      continue;
    }

    if (reverse_label_map.find(rowlabel) == reverse_label_map.end()) {
      new_labels.push_back(flexible_type(current));
      label_map[current] = rowlabel;
      reverse_label_map[rowlabel] = current;
      ++current;
    }
  }

  gl_centroid_with_label->add_column(gl_sarray(new_labels), FIXED_LABEL);
}

/**
 * Builds the distance graph between every centroid, allowing us to run
 * label propagation between the vertices
 */
gl_sgraph categorical_imputer::build_distance_graph(
    gl_sframe gl_centroid_with_label) {

  std::vector<flexible_type> srcVertex;
  std::vector<flexible_type> dstVertex;
  std::vector<flexible_type> wgtVertex;

  size_t cluster_id_index =
      get_column_index(gl_centroid_with_label, CLUSTER_ID);

  std::vector<size_t> feature_idx_for_distance;
  for (const std::string& column: label_propagation_features_set) {
    feature_idx_for_distance.push_back(
        get_column_index(gl_centroid_with_label, column));
  }

  for (const auto& xiter: gl_centroid_with_label.range_iterator()) {
    for (const auto& yiter: gl_centroid_with_label.range_iterator()) {
      if (xiter[cluster_id_index] >= yiter[cluster_id_index]) {
        continue;
      }

      srcVertex.push_back(xiter[cluster_id_index]);
      dstVertex.push_back(yiter[cluster_id_index]);

      double distance = 0;
      // The columns before the cluster IDs are the features of the
      // centroid, we used this to compute the eucledian distance
      for (size_t k: feature_idx_for_distance) {
        distance +=
            (xiter[k].to<double>() - yiter[k].to<double>()) *
            (xiter[k].to<double>() - yiter[k].to<double>());
      }
      wgtVertex.push_back(sqrt(distance));
    }
  }

  gl_sarray raw_weights = gl_sarray(wgtVertex);
  double stdev = raw_weights.std().to<double>();
  double variance = stdev * stdev;
  for (size_t i = 0; i < wgtVertex.size(); ++i) {
    if (variance == 0.0) {
      wgtVertex[i] = 0.0;
    } else {
      wgtVertex[i] = exp(double(-1.0 * (wgtVertex[i]*wgtVertex[i]) / variance));
    }
  }

  // Build the edges of the graph, centroid_with_label are used as vertices
  gl_sframe edges;
  edges.add_column(gl_sarray(srcVertex), "src");
  edges.add_column(gl_sarray(dstVertex), "dst");
  edges.add_column(gl_sarray(wgtVertex), "weight");

  // gl_centroid_with_label is our vertices
  // edges are our edges
  // cluster_id is the vertex ID
  return gl_sgraph(gl_centroid_with_label, edges, CLUSTER_ID, "src", "dst");
}

/**
 * Calls the label_propagation toolkit to fill in the missing labels
 * for all centroids.
 */
gl_sframe categorical_imputer::call_label_propagation(
    gl_sgraph centroid_graph) {

  sgraph graph_to_run = centroid_graph.get_proxy()->get_graph();
  size_t nbCentroids = centroid_graph.vertices().size();

  // Prepare the label_propagation toolkit
  variant_map_type params;
  params["label_field"] = FIXED_LABEL;
  params["weight_field"] = "weight";
  params["threshold"] = nbCentroids < 1000 ? 0 : 1e-3;
  params["self_weight"] = 1.0;
  params["undirected"] = true;
  params["max_iterations"] = 100;

  turi::label_propagation::setup(params);

  size_t num_iter;
  double average_l2_del;
  turi::label_propagation::run<double>(
      graph_to_run, num_iter, average_l2_del);

  std::shared_ptr<unity_sgraph> label_propagated_graph(
      new unity_sgraph(std::make_shared<sgraph>(graph_to_run)));

  /**
   * At this point, label propagation is complete, and we now have a
   * prediction for every centroid as to which label it should have
   * (not forgetting that the predicted labels are _NOT_ in the original label
   * space)
   */
  return gl_sframe(label_propagated_graph->get_vertices());
}

/**
 * The output of the label propagation module is one probability for
 * every possible label for each row; we really just want the probability
 * for the selected label. This method generates that column with the
 * probability of the chosen label.
 */
gl_sframe categorical_imputer::get_prediction_probability(
    gl_sframe* propagation_output) {
  // First, we build a map between probability column and label (the
  // probability columns are called P0, P1, P2,... where 0, 1, 2,... are
  // the labels, and P is a P.
  size_t labelindex = get_column_index(*propagation_output, "predicted_label");

  // Map between a label Id [0, N) and an index in the SFrame
  std::unordered_map<int64_t, size_t> column_map;
  for (size_t i = labelindex; i < propagation_output->column_names().size(); ++i) {
    long long int labelid;
    if (sscanf(propagation_output->column_names()[i].c_str(),
        "P%lld", &labelid) ==  1) {
      column_map[(int64_t)labelid] = i;
    }
  }

  // Now, we build a single column that will contain the probability of
  // the centroids.
  std::vector<flexible_type> probabilities;
  for (const auto& iter: propagation_output->range_iterator()) {
    int64_t predicted_feature = iter[labelindex].to<int64_t>();
    probabilities.push_back(iter[column_map[predicted_feature]]);
  }
  propagation_output->add_column(gl_sarray(probabilities), "feature_probability");

  // Now, we have a table with centroid IDs, Predicted Label and Probabilities
  gl_sframe clusters_with_predictions =
      propagation_output->select_columns(
          {"__id", "predicted_label", "feature_probability"});
  clusters_with_predictions.rename({{"__id", CLUSTER_ID}});
  return clusters_with_predictions;
}

gl_sframe categorical_imputer::join_user_data_and_predictions(
    gl_sframe gl_clustered_user_data,
    gl_sframe clusters_with_predictions) {

  // We can now join the original user data with these, and remove the
  // intermediate cluster_id column
  gl_sframe original_with_prob =
      gl_clustered_user_data.join(clusters_with_predictions, {CLUSTER_ID});
  original_with_prob.remove_column(CLUSTER_ID);

  /**
   * As this point, we pretty much have the final data, except that when
   * we did the clustering, we overrode the original labels of a bunch of
   * data points. For the final output to make sense, we must make sure
   * to restore them as they should. We also cannot load the whole thing
   * in RAM since this is an operation on the entire user-supplied data
   * (so we  rely on the Apply method).
   */
  size_t labelcolumnId = get_column_index(original_with_prob, feature_column);
  size_t predicted_feature_column = get_column_index(original_with_prob, "predicted_label");
  size_t pLabelColumn = get_column_index(original_with_prob, "feature_probability");

  gl_sarray predictedFinal =
      original_with_prob.apply([=](const sframe_rows::row& row) {
    if (row[labelcolumnId].is_na()) {
      auto kv = label_map.find(row[predicted_feature_column].to<int64_t>());
      if (kv != label_map.end()) {
        return (const flexible_type)kv->second;
      }
      return (const flexible_type)row[labelcolumnId]; // Return the None back...
    }
    return row[labelcolumnId];
  }, label_map.begin()->second.get_type());

  gl_sarray probFinal =
      original_with_prob.apply([=](const sframe_rows::row& row) {
    if (row[labelcolumnId].is_na()) {
      return row[pLabelColumn];
    }
    const flexible_type ret = 1.0;
    return ret;
  }, flex_type_enum::FLOAT);

  // Finally, we remove the "bad" columns (which are in [0, N) space)
  original_with_prob.remove_column("predicted_label");
  original_with_prob.remove_column("feature_probability");
  original_with_prob.remove_column(CLUSTER_DISTANCE);

  // And replace them by the ones computed by the Apply method calls
  original_with_prob.add_column(predictedFinal, std::string(PREDICTED_COLUMN_PREFIX) + feature_column);
  original_with_prob.add_column(probFinal, std::string(PROBABILITY_COLUMN_PREFIX) + feature_column);

  return original_with_prob;
}

/**
 * In the case where every centroid has a label attached, we can skip the
 * label propagation step.
 */
gl_sframe categorical_imputer::join_user_data_and_kmeans_output(
    gl_sframe gl_clustered_user_data,
    gl_sframe gl_centroid_with_label) {

  gl_sframe cluster_labels =
      gl_centroid_with_label.select_columns(
          {CLUSTER_ID, MAX_LABEL, COUNT_OF_LABELS, MAX_OF_LABELS});

  // We can now join the original user data with these, and remove the
  // intermediate cluster_id column
  gl_sframe original_with_prob =
      gl_clustered_user_data.join(cluster_labels, {CLUSTER_ID});

  size_t labelcolumnId = get_column_index(original_with_prob, feature_column);
  size_t predicted_label_column = get_column_index(original_with_prob, MAX_LABEL);
  size_t count_of_labels_column = get_column_index(original_with_prob, COUNT_OF_LABELS);
  size_t max_of_labels_column = get_column_index(original_with_prob, MAX_OF_LABELS);

  gl_sarray predictedFinal =
      original_with_prob.apply([=](const sframe_rows::row& row) {
    if (row[labelcolumnId].is_na()) {
      return row[predicted_label_column];
    }
    return row[labelcolumnId];
  }, feature_column_type);

  gl_sarray probFinal =
      original_with_prob.apply([=](const sframe_rows::row& row) {
    if (row[labelcolumnId].is_na()) {
      double probability =
          row[count_of_labels_column] > 0 ?
              row[max_of_labels_column].to<double>() / row[count_of_labels_column].to<double>() :
              0.0;
      return (flexible_type)probability;
    }
    return (flexible_type)1.0;
  }, flex_type_enum::FLOAT);

  // Finally, we remove the "bad" columns (which are in [0, N) space)
  original_with_prob.remove_column(MAX_LABEL);
  original_with_prob.remove_column(COUNT_OF_LABELS);
  original_with_prob.remove_column(MAX_OF_LABELS);
  original_with_prob.remove_column(CLUSTER_ID);
  original_with_prob.remove_column(CLUSTER_DISTANCE);

  // And replace them by the ones computed by the Apply method calls
  original_with_prob.add_column(predictedFinal, std::string(PREDICTED_COLUMN_PREFIX) + feature_column);
  original_with_prob.add_column(probFinal, std::string(PROBABILITY_COLUMN_PREFIX) + feature_column);

  return original_with_prob;
}


/***************************************************************************/
/*                                                                         */
/* Interface Methods                                                       */
/*                                                                         */
/***************************************************************************/

/**
 * Initialize the options
 */
void categorical_imputer::init_options(const std::map<std::string,
                                                   flexible_type>&_options){
  // Can only be called once.
  DASSERT_TRUE(options.get_option_info().empty());

  options.create_string_option("feature", "Column to impute", "feature");
  options.create_boolean_option("verbose",
      "Should the transformer output more status during processing", false);

  // Set options!
  options.set_options(_options);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

/**
 * Get a version for the object.
 */
size_t categorical_imputer::get_version() const {
  return CATEGORICAL_IMPUTER_VERSION;
}

/**
 * Save the object using Turi's oarc.
 */
void categorical_imputer::save_impl(turi::oarchive& oarc) const {
  // Save state
  variant_deep_save(state, oarc);

  // Everything else
  oarc << options
       << label_map
       << reverse_label_map
       << label_propagation_features_set
       << dependent_feature_columns
       << fitted
       << feature_column
       << exclude
       << verbose;
}

/**
 * Load the object using Turi's iarc.
 */
void categorical_imputer::load_version(turi::iarchive& iarc, size_t version){
  // State
  variant_deep_load(state, iarc);

  // Everything else
  iarc >> options
       >> label_map
       >> reverse_label_map
       >> label_propagation_features_set
       >> dependent_feature_columns
       >> fitted
       >> feature_column
       >> exclude
       >> verbose;
}


/**
 * Initialize the transformer.
 *
 * In this case, we pretty much just parse user-provided options.
 */
void categorical_imputer::init_transformer(
    const std::map<std::string, flexible_type>& _options){

  DASSERT_TRUE(options.get_option_info().empty());

  // Copy over the options (exclude features)
  std::map<std::string, flexible_type> opts;
  for (const auto& k: _options){
    if (k.first != "reference_features"){
      opts[k.first] = variant_get_value<flexible_type>(k.second);
    }
  }

  init_options(opts);

  // Set features
  dependent_feature_columns = _options.at("reference_features");
  exclude = false;
  verbose = _options.at("verbose");
  feature_column = _options.at("feature").to<std::string>();

  state["reference_features"] = to_variant(dependent_feature_columns);
  state["excluded_features"] = to_variant(FLEX_UNDEFINED);
}

/**
 * Fit the data.
 *
 * In this case, we find which columns are valid to play with
 */
void categorical_imputer::fit(gl_sframe data){
  //  Empty SFrame provided
  if (data.size() == 0) {
    log_and_throw("The input data is empty.");
  }

  DASSERT_TRUE(state.count("reference_features") > 0);
  DASSERT_TRUE(!options.get_option_info().empty());

  // Get the set of reference_features to work with for the KMeans step
  std::vector<std::string> kmeans_features =
      transform_utils::get_column_names(data, exclude, dependent_feature_columns);
  transform_utils::validate_feature_columns(
      data.column_names(), kmeans_features);

  // Remove the label column from the set of features!
  kmeans_features.erase(
      std::remove(
          kmeans_features.begin(), kmeans_features.end(), feature_column),
          kmeans_features.end());

  // Select the features of the right type. -- Throws an exception if no
  // features are found!
  kmeans_features =
      transform_utils::select_valid_features(data, kmeans_features,
          {flex_type_enum::FLOAT,
           flex_type_enum::VECTOR,
           flex_type_enum::INTEGER,
           flex_type_enum::DICT,
           flex_type_enum::STRING});

  // Get the set of features to work with for the label propagation step
  std::vector<std::string> label_prop_features =
      transform_utils::select_valid_features_nothrow(data, kmeans_features,
          {flex_type_enum::FLOAT,
           flex_type_enum::INTEGER},
           false);

  label_propagation_features_set.clear();
  label_propagation_features_set.insert(
      label_prop_features.begin(), label_prop_features.end());

  if (verbose && label_prop_features.empty()) {
    logprogress_stream << "No FLOAT or INTEGER columns specified as features, "
        << "the imputer will not be able to use the full extent of "
        << "label propagation to infer labels.";
  }

  state["reference_features"] = to_variant(kmeans_features);
  state["label_prop_features"] = to_variant(label_prop_features);

  // Make sure the column to impute is there
  size_t feature_column_index = get_column_index(data, feature_column);
  if (feature_column_index == (size_t)(-1)) {
    log_and_throw("Feature column not present in input SFrame");
  }

  // Get the type of the column to impute
  feature_column_type = data.column_types()[feature_column_index];

  // Build the KMeans model
  gl_sframe gl_centroids;
  gl_sframe gl_clustered_user_data;
  gl_sframe innerdata = data;
  call_kmeans(innerdata, false, &gl_clustered_user_data, &gl_centroids);

  // Compute a label for each centroid
  gl_sframe gl_centroid_with_label;
  compute_cluster_argmax_label(
      gl_clustered_user_data,
      gl_centroids,
      &gl_centroid_with_label);

  // Now that we have joined the centroids to get labels, we must remove
  // additional columns such that they can be used in the transform phase
  // (kmeans when using centroids assumes that there are only feature columns)
  gl_centroids.remove_column(CLUSTER_ID);
  gl_centroids.remove_column("size");
  gl_centroids.remove_column("sum_squared_distance");

  state["gl_centroids"] = to_variant(gl_centroids);
  state["gl_centroid_with_label"] = to_variant(gl_centroid_with_label);

  fitted = true;
}

/**
 * Transform the given data.
 *
 * This is the meat of the problem! MEAT!!!
 */
gl_sframe categorical_imputer::transform(gl_sframe data){
  // Empty sframe provided
  if (data.size() == 0) {
    log_and_throw("The input data is empty.");
  }

  DASSERT_TRUE(!options.get_option_info().empty());
  if (!fitted) {
    log_and_throw("The CategoricalImputer must be fitted before .transform() is called.");
  }

  size_t feature_column_index = get_column_index(data, feature_column);
  if (feature_column_index == (size_t)(-1)) {
    log_and_throw("Feature column not present in input SFrame");
  }

  if (data.column_types()[feature_column_index] != feature_column_type) {
    log_and_throw(std::string("Feature column type for column ")
      + feature_column + std::string(" does not match between fit and transform"));
  }

  /**
   * First, we call KMeans, which will assign cluster IDs to every row of user-
   * provided data.
   *
   * Clustered user_data is the user data with a centroid_id column added
   * Centroids is a Feature Vector, Centroid ID and Weight for each cluster
   */

  gl_sframe gl_centroids =
            variant_get_value<gl_sframe>(state.at("gl_centroids"));

  gl_sframe gl_clustered_user_data;
  call_kmeans(data, true, &gl_clustered_user_data, &gl_centroids);

  /**
   * Compute a label for each centroid
   *
   * Centroid with Labels is a row per centroid, with an ARGMAX label column
   */
  gl_sframe gl_centroid_with_label =
            variant_get_value<gl_sframe>(state.at("gl_centroid_with_label"));

  std::vector<std::string> label_prop_features =
     variant_get_value<std::vector<std::string>>(state.at("label_prop_features"));

  // If Not all centroids have labels, we need to use label_propagation,
  // but only if we have valid features!
  bool allHaveLabels = all_centroids_labeled(gl_centroid_with_label);
  if (!allHaveLabels && !label_prop_features.empty()) {

    // First, we need to rename the centroids in [0, N) space
    rename_labels(&gl_centroid_with_label);

    gl_sgraph centroid_graph = build_distance_graph(gl_centroid_with_label);

    gl_sframe label_propagation_output = call_label_propagation(centroid_graph);

    gl_sframe clusters_with_predictions =
        get_prediction_probability(&label_propagation_output);

    gl_sframe final_data = join_user_data_and_predictions(
      gl_clustered_user_data,
      clusters_with_predictions);

    return final_data;

  } else {

    // Join cluster IDs and produce user-friendly output
    gl_sframe final_data = join_user_data_and_kmeans_output(
        gl_clustered_user_data,
        gl_centroid_with_label);

    return final_data;
  }
}

} // feature_engineering
} // sdk_model
} // turicreate

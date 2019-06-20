/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <string>
#include <cmath>
#include <cfloat>
#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>

#include <toolkits/feature_engineering/transform_utils.hpp>
#include <toolkits/feature_engineering/feature_binner.hpp>
#include <toolkits/feature_engineering/topk_indexer.hpp>
#include <ml/sketches/quantile_sketch.hpp>

namespace turi {
namespace sdk_model {
namespace feature_engineering {


/**
 * Compute the left and right endpoints for bins defined by
 * the provided breakpoints.
 *
 * Example:
 * A vector of breakpoints {0, 1, 20} returns a set of pairs
 * (-Inf, 0), (0, 1), (1, 20), and (20, Inf)
 */
std::vector<bin> compute_bins(std::vector<double> breaks) {

  // Start with one bin: (-Inf, Inf).
  auto column_bins = std::vector<bin>();
  double left = std::numeric_limits<double>::lowest();
  double right = std::numeric_limits<double>::max();
  size_t bin_id = 0;

  // For each breakpoint add the left bin to the current list.
  for (auto& b: breaks) {
    column_bins.emplace_back(bin{left, b, bin_id});
    left = b;
    bin_id++;
  }

  // Add the right-most bin
  column_bins.emplace_back(bin{left, right, bin_id});

  return column_bins;
}

/**
 * This function compute quantiles for all floats in a column, whether it's
 * an SArray of floats, a dict of floats, or a list of floats.
 */
std::vector<double> compute_quantiles(gl_sarray& column,
                                      flex_type_enum type,
                                      size_t num_bins) {
  auto breakpoints = std::vector<double>();
  if ((type != flex_type_enum::FLOAT) &&
      (type != flex_type_enum::INTEGER)) {
    return breakpoints;
  }

  auto s = sketches::quantile_sketch<double>(column.size());

  // TODO: This next section could benefit from an iterator over floats,
  // regardless of the column type.
  if (type == flex_type_enum::FLOAT || type == flex_type_enum::INTEGER) {
    for (const auto& x: column.range_iterator()) {
      s.add(x);
    }
  } else {
    // Do nothing
  }
  s.finalize();

  // Copy quantile values over to a vector
  DASSERT_TRUE(num_bins > 0);
  for (size_t i=0; i < (num_bins-1); ++i) {
    double b = s.fast_query_quantile(1.0 * i / num_bins);
    breakpoints.push_back(b);
  }
  return breakpoints;
}


/**
 * Make an SFrame describing the bins
 */
gl_sframe get_bins_sframe(const std::map<std::string, std::vector<bin>>& breaks) {
  auto names = std::vector<std::string>({"column", "name", "left", "right"});
  auto types = std::vector<flex_type_enum>({flex_type_enum::STRING,
                                            flex_type_enum::STRING,
                                            flex_type_enum::FLOAT,
                                            flex_type_enum::FLOAT});

  gl_sframe_writer writer(names, types, 1);
  for (const auto& kvp: breaks) {
    auto column_name = kvp.first;
    auto column_bins = kvp.second;
    for (const auto& b : column_bins) {
      std::vector<flexible_type> row;
      row.push_back(column_name);
      row.push_back(column_name + "_" + std::to_string(b.bin_id));
      row.push_back(b.left);
      row.push_back(b.right);
      writer.write(row, 0);
    }
  }
  return writer.close();
}

/**
 * Maps a numerical value to a bin, turning it into a categorical variable.
 *
 * \param[in] input      A flexible type
 *
 * \returns  output The name of the appropriate bin.
 *
 */
flexible_type feature_binner_apply_element(const flexible_type& input,
                                           const std::vector<bin>& column_bins,
                                           const std::string& column_name) {
  for (const auto& b: column_bins) {
    if ((b.left < input) && (input <= b.right)) {
      return column_name + "_" + std::to_string(b.bin_id);
    }
  }
  return FLEX_UNDEFINED;
}

/**
 * Apply the binning operation to a single input value.
 */
flexible_type feature_binner_apply(const flexible_type& input,
                                   const std::vector<bin>& column_bins,
                                   const std::string& column_name) {

  flex_type_enum run_mode = input.get_type();
  DASSERT_TRUE(column_bins.size() > 0);
  DASSERT_TRUE(run_mode == flex_type_enum::INTEGER
               || run_mode == flex_type_enum::FLOAT
               || run_mode == flex_type_enum::UNDEFINED);

  // Go through all the cases.
  flexible_type output;
  switch(run_mode) {
    // Categorical cols (int | str)
    case flex_type_enum::INTEGER:
    case flex_type_enum::FLOAT: {
      output = feature_binner_apply_element(input, column_bins, column_name);
      break;
    }
    case flex_type_enum::UNDEFINED: {
      // No transform is made to undefined values.
      output = input;
      break;
    }
    // Should never happen here.
    default:
      log_and_throw("Invalid type. Column must be of type int or float.");
      break;
  }
  return output;
}


/**
 * Initialize the options
 */
void feature_binner::init_options(const std::map<std::string,
                                  flexible_type>&_options){
  // Can only be called once.
  DASSERT_TRUE(options.get_option_info().size() == 0);

  options.create_flexible_type_option(
      "output_column_prefix",
      "The prefix to use for the column name of each transformed column.",
      FLEX_UNDEFINED,
      false);

  options.create_categorical_option(
      "strategy",
      "Default strategy to use for defining bins. Options include 'quantiles', 'logarithmic'.",
      "logarithmic",
      {flexible_type("logarithmic"), flexible_type("quantile")},
      false);

  options.create_integer_option(
      "num_bins",
      "Number of bins to use.",
      10,
      1,
      std::numeric_limits<int>::max(),
      false);

/*  options.create_flexible_type_option(
      "breaks",
      "Dictionary of breakpoints that define bins, where each key is a column name and each value is a list of integers. This overrides any setting of num_bins and the default strategy.",
      FLEX_UNDEFINED,
      false);
*/

  // Set options!
  options.set_options(_options);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

/**
 * Get a version for the object.
 */
size_t feature_binner::get_version() const {
  return FEATURE_BINNER_VERSION;
}

/**
 * Save the object using Turi's oarc.
 */
void feature_binner::save_impl(turi::oarchive& oarc) const {
  // Save state
  variant_deep_save(state, oarc);

  // Everything else
  oarc << options
       << feature_columns
       << feature_types
       << bins
       << fitted
       << unprocessed_features
       << exclude;
}

/**
 * Load the object using Turi's iarc.
 */
void feature_binner::load_version(turi::iarchive& iarc, size_t version){
  // State
  variant_deep_load(state, iarc);

  // Everything else
  iarc >> options
       >> feature_columns
       >> feature_types
       >> bins
       >> fitted
       >> unprocessed_features
       >> exclude;
}


/**
 * Initialize the transformer.
 */
void feature_binner::init_transformer(const std::map<std::string,
                      flexible_type>& _options){
  std::map<std::string, flexible_type> opts;
  for(const auto& k: _options){
    if (k.first != "features" && k.first != "exclude"){
      opts[k.first] = variant_get_value<flexible_type>(k.second);
    }
  }
  init_options(opts);

  unprocessed_features = _options.at("features");
  exclude = _options.at("exclude");
  if ((int) exclude == 1) {
    state["features"] = to_variant(FLEX_UNDEFINED);
    state["excluded_features"] = to_variant(unprocessed_features);
  } else {
    state["features"] = to_variant(unprocessed_features);
    state["excluded_features"] = to_variant(FLEX_UNDEFINED);
  }

  // Initialize all of the bins
  bins = std::map<std::string, std::vector<bin>>();
  state["bins"] = to_variant(FLEX_UNDEFINED);
}

/**
 * Fit the data.
 */
void feature_binner::fit(gl_sframe data){
  DASSERT_TRUE(state.count("features") > 0);
  DASSERT_TRUE(options.get_option_info().size() > 0);

  // Get the set of features to work with.
  feature_columns = transform_utils::get_column_names(
                            data, exclude, unprocessed_features);

  // Select the features of the right type.
  feature_columns = transform_utils::select_valid_features(data, feature_columns,
                      {flex_type_enum::FLOAT,
                       flex_type_enum::INTEGER});

  transform_utils::validate_feature_columns(data.column_names(), feature_columns);

  // Store feature types and cols.
  feature_types.clear();
  for (const auto& f: feature_columns) {
    feature_types[f] = data.select_column(f).dtype();
  }

  // Update state based on the new set of features.
  state["features"] = to_variant(feature_columns);

  size_t num_bins = options.value("num_bins");


  //TODO: Break this into its own function
  // Get the set of columns that have manual overrides in the breaks argument.

  for (const auto& f: feature_columns) {
    if (options.value("strategy") == "logarithmic") {

      auto breakpoints = std::vector<double>();
      double base = 10.0;

      // Until we reach num_bins, add a breakpoint
      DASSERT_TRUE(num_bins > 0);
      for (size_t b=0; b < num_bins; ++b) {
        if (b+1 < num_bins) {
          breakpoints.push_back(std::pow(base, b));
        }
      }

      bins[f] = compute_bins(breakpoints);

    } else if (options.value("strategy") == "quantile") {

      auto col = data.select_column(f);
      auto breakpoints = compute_quantiles(col, feature_types[f], num_bins);
      bins[f] = compute_bins(breakpoints);
    } else {
      // Do nothing. Uses (-Inf, Inf] by default.
    }
  }

  // Save bins data for each feature column to a pretty SFrame.
  state["bins"] = get_bins_sframe(bins);

  fitted = true;
}

/**
 * Transform the given data.
 */
gl_sframe feature_binner::transform(gl_sframe data){
  DASSERT_TRUE(options.get_option_info().size() > 0);
  DASSERT_TRUE(bins.size() > 0);

  //Check if fitting has already ocurred.
  if (!fitted){
    log_and_throw("The FeatureBinner must be fitted before .transform() is called.");
  }

  // Validated column names
  std::vector<std::string> subset_columns;
  subset_columns = transform_utils::select_feature_subset(data, feature_columns);
  transform_utils::validate_feature_types(subset_columns, feature_types, data);

  // Get the set of features to work with.
  std::vector<std::string> transform_features =
            subset_columns;

  // Setup
  gl_sframe ret_sf = data;
  std::vector<std::string> cols = data.column_names();
  flexible_type output_column_prefix =
      variant_get_value<flexible_type>(state.at("output_column_prefix"));
  if (output_column_prefix == FLEX_UNDEFINED) {
    output_column_prefix = "";
  } else {
    output_column_prefix = output_column_prefix + ".";
  }

  // Loop through features.
  for (size_t i = 0; i < transform_features.size(); i++){
    std::string col_name = transform_features[i];

    // Get the output column name.
    flex_string output_column_name = output_column_prefix + col_name;

    // Skip if the column isn't there.
    if (std::find(cols.begin(), cols.end(), col_name) == cols.end()) {
      continue;
    }

    // Check that the types are the same.
    if (feature_types[col_name] != data[col_name].dtype()){
        log_and_throw("Column type mismatch (in column '" + col_name + "')"
                      " between fit() and transform() modes.");
    }
    const auto& column_bins = bins[col_name];

    // Decide return type of applying the transformation.
    auto ret_type = flex_type_enum::STRING;

    // Error throwing mode.
    data[col_name].head(10).apply([column_bins, col_name]
           (const flexible_type& x) {
               return feature_binner_apply(x, column_bins, col_name);
           }, ret_type, false).materialize();

    // Tranform mode.
    ret_sf[output_column_name] = data[col_name].apply([column_bins, col_name]
           (const flexible_type& x) {
               return feature_binner_apply(x, column_bins, col_name);
           }, ret_type, false);
  }
  return ret_sf;
}


} // feature_engineering
} // sdk_model
} // turicreate

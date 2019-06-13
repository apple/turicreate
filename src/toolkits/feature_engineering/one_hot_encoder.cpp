/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <string>
#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>

#include <toolkits/feature_engineering/transform_utils.hpp>
#include <toolkits/feature_engineering/one_hot_encoder.hpp>
#include <toolkits/feature_engineering/topk_indexer.hpp>

namespace turi {
namespace sdk_model {
namespace feature_engineering {

/**
 * Map a collection of categorical types to a sparse indexed representation.
 *
 * \param[in] input          Row of flexible types.
 * \param[in] index_map       index_map for each row.
 * \param[in] start_indices  Start index for each of the columns.
 *
 * \returns  output Indexed flexible type (as dictionary).
 *
 */
flexible_type one_hot_encoder_apply(const sframe_rows::row& inputs,
            const std::vector<std::shared_ptr<topk_indexer>>& index_map,
            const std::vector<size_t>& start_indices) {
  DASSERT_EQ(index_map.size(), inputs.size());
  DASSERT_EQ(index_map.size(), start_indices.size());

  // Go through all the cases.
  flex_dict output = {};
  size_t index = 0;
  for (size_t i = 0; i < index_map.size(); i++) {
    size_t start_index = start_indices[i];
    const auto& indexer = index_map[i];
    const auto& input = inputs[i];
    flex_type_enum run_mode = input.get_type();
    DASSERT_TRUE(indexer != NULL);
    DASSERT_TRUE(run_mode == flex_type_enum::INTEGER
                 || run_mode == flex_type_enum::STRING
                 || run_mode == flex_type_enum::LIST
                 || run_mode == flex_type_enum::UNDEFINED
                 || run_mode == flex_type_enum::DICT);

    switch(run_mode) {
      // Categorical cols (int | str | None)
      case flex_type_enum::INTEGER:
      case flex_type_enum::UNDEFINED:
      case flex_type_enum::STRING: {
        index = indexer->lookup(input);
        if (index != (size_t)-1) {
          output.push_back(std::make_pair(start_index + index, 1));
        }
        break;
      }

      // Categorical vector
      case flex_type_enum::LIST: {
        const flex_list& vv = input.get<flex_list>();
        size_t n_values = vv.size();
        for(size_t k = 0; k < n_values; ++k) {
          index = indexer->lookup(vv[k]);
          if (index != (size_t)-1) {
            output.push_back(std::make_pair(start_index + index, 1));
          }
        }
        break;
      }

      // Dictionary
      case flex_type_enum::DICT: {
        const flex_dict& dv = input.get<flex_dict>();
        size_t n_values = dv.size();
        for(size_t k = 0; k < n_values; ++k) {
          const std::pair<flexible_type, flexible_type>& kvp = dv[k];
          flexible_type out_key =
                    flex_string(kvp.first) + ":" + flex_string(kvp.second);
          index = indexer->lookup(out_key);
          if (index != (size_t)-1) {
            output.push_back(std::make_pair(start_index + index, 1));
          }
        }
        break;
      }

      // Should never happen here.
      default:
        log_and_throw("Invalid type. Column must be of type int, string,"
                          " list or dictionary.");
        break;
    }
  }
  return output;
}


/**
 * Initialize the options
 */
void one_hot_encoder::init_options(const std::map<std::string,
                                                   flexible_type>&_options){
  // Can only be called once.
  DASSERT_TRUE(options.get_option_info().size() == 0);

  options.create_string_option(
      "output_column_name",
      "The column in the output SFrame where the encoded features are present.",
      "encoded_features");

  options.create_integer_option(
      "max_categories",
      "Maximum categories per column (ordered by occurrence in the training set).",
      FLEX_UNDEFINED,
      1,
      std::numeric_limits<int>::max(),
      false);

  // Set options!
  options.set_options(_options);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

/**
 * Get a version for the object.
 */
size_t one_hot_encoder::get_version() const {
  return ONE_HOT_ENCODER_VERSION;
}

/**
 * Save the object using Turi's oarc.
 */
void one_hot_encoder::save_impl(turi::oarchive& oarc) const {
  // Save state
  variant_deep_save(state, oarc);

  //// Everything else
  oarc << options
       << feature_columns
       << feature_types
       << index_map
       << start_index_map
       << exclude;
}

/**
 * Load the object using Turi's iarc.
 */
void one_hot_encoder::load_version(turi::iarchive& iarc, size_t version){
  // State
  variant_deep_load(state, iarc);

  // Everything else
  iarc >> options
       >> feature_columns
       >> feature_types
       >> index_map
       >> start_index_map
       >> exclude;
}


/**
 * Initialize the transformer.
 */
void one_hot_encoder::init_transformer(const std::map<std::string,
                      flexible_type>& _options){
  DASSERT_TRUE(options.get_option_info().size() == 0);

  std::map<std::string, flexible_type> opts;
  for(const auto& k: _options){
    if (k.first != "features" && k.first != "exclude"){
      opts[k.first] = variant_get_value<flexible_type>(k.second);
    }
  }

  // If max_categories = None, set it to inf
  if (opts.count("max_categories") && (opts.at("max_categories") == FLEX_UNDEFINED)) {
    opts["max_categories"] = std::numeric_limits<int>::max();
  }
  init_options(opts);

  // Set features
  feature_columns = _options.at("features");
  exclude = _options.at("exclude");
  if ((int) exclude == 1) {
    state["features"] = to_variant(FLEX_UNDEFINED);
    state["excluded_features"] = to_variant(feature_columns);
  } else {
    state["features"] = to_variant(feature_columns);
    state["excluded_features"] = to_variant(FLEX_UNDEFINED);
  }
}

/**
 * Fit the data.
 */
void one_hot_encoder::fit(gl_sframe data){
  DASSERT_TRUE(options.get_option_info().size() > 0);

  // Get the feature names.
  std::vector<std::string> fit_features = transform_utils::get_column_names(
                            data, exclude, feature_columns);

  // Select features of the right type.
  fit_features = transform_utils::select_valid_features(data, fit_features,
                      {flex_type_enum::STRING,
                       flex_type_enum::INTEGER,
                       flex_type_enum::LIST,
                       flex_type_enum::DICT});

  // Validate the features.
  transform_utils::validate_feature_columns(data.column_names(), fit_features);
  state["features"] = to_variant(fit_features);

  // Store feature types.
  feature_types.clear();
  for (const auto& f: fit_features) {
    feature_types[f] = data.select_column(f).dtype();
  }

  // Learn the index mapping.
  start_index_map.clear();
  index_map.clear();

  size_t global_index = 0;
  for (const auto& feat: fit_features) {

    index_map[feat].reset(new topk_indexer(
                     variant_get_value<size_t>(state.at("max_categories")),
                     1, std::numeric_limits<int>::max(),
                     feat));

    transform_utils::create_topk_index_mapping(data[feat], index_map[feat]);
    start_index_map[feat] = global_index;
    global_index += index_map[feat]->size();
    DASSERT_TRUE(index_map[feat] != NULL);
  }

  gl_sframe_writer feature_encoding({"feature", "category", "index"},
                             {flex_type_enum::STRING, flex_type_enum::STRING,
                             flex_type_enum::INTEGER}, 1);
  size_t i = 0;
  for (const auto& f:fit_features) {
    const auto& ind = index_map[f];
    for (const auto& val: ind->get_values()) {
      if (val != FLEX_UNDEFINED) {
        feature_encoding.write({f, flex_string(val), i}, 0);
      } else {
        feature_encoding.write({f, val, i}, 0);
      }
      i++;
    }
  }
  state["feature_encoding"] = to_variant(feature_encoding.close());
}

/**
 * Transform the given data.
 */
gl_sframe one_hot_encoder::transform(gl_sframe data){
  DASSERT_TRUE(options.get_option_info().size() > 0);
  if (index_map.size() == 0) {
    log_and_throw("The OneHotEncoder must be fitted before .transform() is called.");
  }

  // Select and validate features.
  std::vector<std::string> transform_features =
            variant_get_value<std::vector<std::string>>(state.at("features"));
  transform_features = transform_utils::select_feature_subset(
                                   data, transform_features);
  transform_utils::validate_feature_types(
                                   transform_features, feature_types, data);
  // Original data
  gl_sframe ret_sf = data;
  if (transform_features.size() > 0) {

    // Select subset of index_map, global indexes, etc.
    gl_sframe selected_sf = data.select_columns(transform_features);
    std::vector<size_t> selected_start_indices;
    std::vector<std::shared_ptr<topk_indexer>> selected_indexers;
    for (auto &f : transform_features){
      ret_sf.remove_column(f);
      selected_start_indices.push_back(start_index_map[f]);
      selected_indexers.push_back(index_map[f]);
    }

    // Check if output name exists. If not, get a valid one.
    std::string output_name = (std::string) options.value("output_column_name");
    output_name = transform_utils::get_unique_feature_name(ret_sf.column_names(),
                                                             output_name);

    // Error checking mode.
    selected_sf.head(10).apply([selected_indexers, selected_start_indices]
                             (const sframe_rows::row& x)->flexible_type{
        return one_hot_encoder_apply(x, selected_indexers, selected_start_indices);
      }, flex_type_enum::DICT).materialize();

    ret_sf[output_name] =
          selected_sf.apply([selected_indexers, selected_start_indices]
                            (const sframe_rows::row& x)->flexible_type{
        return one_hot_encoder_apply(x, selected_indexers, selected_start_indices);
    }, flex_type_enum::DICT);
  }

  return ret_sf;
}


} // namespace feature_engineering
} // namespace sdk_model
} // namespace turi

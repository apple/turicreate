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
#include <toolkits/feature_engineering/count_thresholder.hpp>
#include <toolkits/feature_engineering/topk_indexer.hpp>

namespace turi {
namespace sdk_model {
namespace feature_engineering {

/**
 * Map infrequent categorical variables into a Junk bin.
 *
 * \param[in] input    A flexible type
 * \param[in] indexer  An already created column_indexer.
 * \param[in] junk     The value for the junk bin.
 *
 * \returns  output Indexed flexible type.
 *
 */
flexible_type count_thresholder_apply(const flexible_type& input,
            const std::shared_ptr<topk_indexer>& indexer,
            const flexible_type& junk) {

  flex_type_enum run_mode = input.get_type();
  DASSERT_TRUE(indexer != NULL);
  DASSERT_TRUE(run_mode == flex_type_enum::INTEGER
               || run_mode == flex_type_enum::STRING
               || run_mode == flex_type_enum::UNDEFINED
               || run_mode == flex_type_enum::LIST
               || run_mode == flex_type_enum::DICT);

  // Go through all the cases.
  flexible_type output;
  size_t index = 0;
  switch(run_mode) {
    // Categorical cols (int | str | None)
    case flex_type_enum::UNDEFINED:
    case flex_type_enum::INTEGER:
    case flex_type_enum::STRING: {
      index = indexer->lookup(input);
      if (index == (size_t)-1) {
        output = junk;
      } else {
        output = input;
      }
      break;
    }

    // Categorical vector
    case flex_type_enum::LIST: {
      flex_list out_list;
      const flex_list& vv = input.get<flex_list>();
      size_t n_values = vv.size();
      out_list.resize(n_values);
      for(size_t k = 0; k < n_values; ++k) {
        index = indexer->lookup(vv[k]);
        if (index == (size_t)-1) {
          out_list[k] = junk;
        } else {
          out_list[k] = vv[k];
        }
      }
      output = out_list;
      break;
    }

    // Dictionary
    case flex_type_enum::DICT: {
      flex_dict out_dict = {};
      const flex_dict& dv = input.get<flex_dict>();
      size_t n_values = dv.size();
      out_dict.resize(n_values);
      for(size_t k = 0; k < n_values; ++k) {
        const std::pair<flexible_type, flexible_type>& kvp = dv[k];
        flexible_type out_key = flex_string(kvp.first) + ":" + flex_string(kvp.second);
        index = indexer->lookup(out_key);
        if (index == (size_t)-1) {
          out_dict[k] = std::make_pair(kvp.first, junk);
        } else {
          out_dict[k] = kvp;
        }
      }
      output = out_dict;
      break;
    }

    // Should never happen here.
    default:
      log_and_throw("Invalid type. Column must be of type int, string,"
                        " list or dictionary.");
      break;
  }
  return output;
}


/**
 * Initialize the options
 */
void count_thresholder::init_options(const std::map<std::string,
                                                   flexible_type>&_options){
  // Can only be called once.
  DASSERT_TRUE(options.get_option_info().size() == 0);

  options.create_flexible_type_option(
      "output_column_prefix",
      "The prefix to use for the column name of each transformed column.",
      FLEX_UNDEFINED,
      false);

  options.create_flexible_type_option(
      "output_category_name",
      "The name of the category, where infrequent categories are mapped to, in "
                                              "the transformed column.",
      FLEX_UNDEFINED,
      true);

  options.create_integer_option(
      "threshold",
      "Limit the categories to ones that occur atleast \'thresold\' times.",
      1,
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
size_t count_thresholder::get_version() const {
  return COUNT_THRESHOLDER_VERSION;
}

/**
 * Save the object using Turi's oarc.
 */
void count_thresholder::save_impl(turi::oarchive& oarc) const {
  // Save state
  variant_deep_save(state, oarc);

  //// Everything else
  oarc << options
       << feature_columns
       << feature_types
       << index_map
       << exclude;
}

/**
 * Load the object using Turi's iarc.
 */
void count_thresholder::load_version(turi::iarchive& iarc, size_t version){
  // State
  variant_deep_load(state, iarc);

  // Everything else
  iarc >> options
       >> feature_columns
       >> feature_types
       >> index_map
       >> exclude;
}


/**
 * Initialize the transformer.
 */
void count_thresholder::init_transformer(const std::map<std::string,
                      flexible_type>& _options){
  DASSERT_TRUE(options.get_option_info().size() == 0);

  // Copy over the options (exclude features)
  std::map<std::string, flexible_type> opts;
  for(const auto& k: _options){
    if (k.first != "features" && k.first != "exclude"){
      opts[k.first] = variant_get_value<flexible_type>(k.second);
    }
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
void count_thresholder::fit(gl_sframe data){
  DASSERT_TRUE(state.count("features") > 0);
  DASSERT_TRUE(options.get_option_info().size() > 0);

  // Get the set of features to work with.
  std::vector<std::string> fit_features = transform_utils::get_column_names(
                            data, exclude, feature_columns);
  transform_utils::validate_feature_columns(data.column_names(), fit_features);

  // Select the features of the right type.
  fit_features = transform_utils::select_valid_features(data, fit_features,
                      {flex_type_enum::STRING,
                       flex_type_enum::INTEGER,
                       flex_type_enum::LIST,
                       flex_type_enum::DICT});

  // Validate the features.
  transform_utils::validate_feature_columns(data.column_names(), fit_features);
  feature_types.clear();
  for (const auto& f: fit_features) {
    feature_types[f] = data.select_column(f).dtype();
  }
  state["features"] = to_variant(fit_features);

  // Learn the index mapping.
  index_map.clear();
  for (const auto& feat: fit_features) {
    index_map[feat].reset(new topk_indexer(
                     std::numeric_limits<int>::max(),
                     variant_get_value<size_t>(state.at("threshold")),std::numeric_limits<int>::max(),
                     feat));
    transform_utils::create_topk_index_mapping(data[feat], index_map[feat]);
    DASSERT_TRUE(index_map[feat] != NULL);
  }

  gl_sframe_writer feature_encoding({"feature", "category"},
                  {flex_type_enum::STRING, flex_type_enum::STRING}, 1);
  for (const auto& f:fit_features) {
    const auto& ind = index_map[f];
    for (const auto& val: ind->get_values()) {
      if (val != FLEX_UNDEFINED) {
        feature_encoding.write({f, flex_string(val)}, 0);
      } else {
        feature_encoding.write({f, val}, 0);
      }
    }
  }
  state["categories"] = to_variant(feature_encoding.close());
}

/**
 * Transform the given data.
 */
gl_sframe count_thresholder::transform(gl_sframe data){
  DASSERT_TRUE(options.get_option_info().size() > 0);
  if (index_map.size() == 0) {
    log_and_throw("The CountThresholder must be fitted before .transform() is called.");
  }

  // Select and validate features.
  std::vector<std::string> transform_features =
            variant_get_value<std::vector<std::string>>(state.at("features"));
  transform_features = transform_utils::select_feature_subset(
                                   data, transform_features);
  transform_utils::validate_feature_types(
                                   transform_features, feature_types, data);
  flexible_type output_category_name =
      variant_get_value<flexible_type>(state.at("output_category_name"));

  // Original data
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
  for (const auto& feat: transform_features) {

    // Get the output column name.
    flex_string output_column_name = output_column_prefix + feat;

    // Get the output type. Cast to string if not castable.
    flex_type_enum output_type = feature_types[feat];
    if (output_type != output_category_name.get_type()
       && output_category_name != FLEX_UNDEFINED) {
      output_type = flex_type_enum::STRING;
    }
    const auto& ind = index_map[feat];

    // Error throwing mode.
    data[feat].head(10).apply([ind, output_category_name]
      (const flexible_type& x) {
          return count_thresholder_apply(x, ind, output_category_name);
      }, output_type, false).materialize();

    // Tranform mode.
    ret_sf[output_column_name] = data[feat].apply([ind, output_category_name]
      (const flexible_type& x) {
          return count_thresholder_apply(x, ind, output_category_name);
      }, output_type, false);
  }
  return ret_sf;
}

} // feature_engineering
} // sdk_model
} // turicreate

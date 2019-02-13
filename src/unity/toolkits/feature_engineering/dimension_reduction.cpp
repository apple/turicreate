/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/feature_engineering/dimension_reduction.hpp>
#include <unity/toolkits/feature_engineering/transform_utils.hpp>

#include <unity/lib/variant_deep_serialize.hpp>
#include <random/random.hpp>
#include <unity/toolkits/supervised_learning/supervised_learning_utils-inl.hpp>
#include <table_printer/table_printer.hpp>
#include <sstream>

namespace turi {
namespace sdk_model {
namespace feature_engineering {

#ifdef NDEBUG
#define GAUSSIAN_PROJECTION_MAX_THREAD_MEMORY 1024 * 1024 * 512 // 512 MB
#else
#define GAUSSIAN_PROJECTION_MAX_THREAD_MEMORY 1024 * 128 // 125KB (small enough to test)
#endif


/**********************
 ** Helper functions **
 **********************/

/*
 * Fill an Eigen dense matrix by drawing each entry from a Gaussian
 * distribution. The Gaussian has mean 0 and variance 1, but the projection
 * needs to be scaled by 1 / sqrt(embedding_dimension).  Note that multiplying a
 * standard Gaussian by 1/sqrt(k) is equivalent to drawing a random variable
 * from N(0, 1/k), which is Scikit-learn's strategy. I think this code is more
 * readable.
 *
 * \param projection_matrix Shared pointer to an Eigen dense_matrix. Should be
 *                          sized correctly, but empty.
 * \param embedding_dimension Required here for the scale factor on the
 *                            projection matrix entries.
 * \param random seed
 */
void fill_gaussian_projection_matrix(std::shared_ptr<dense_matrix> projection_matrix,
                                     const size_t embedding_dimension,
                                     const size_t random_seed) {

  double scale = 1 / std::sqrt(embedding_dimension);
  random::seed(random_seed);

  *projection_matrix = projection_matrix->unaryExpr([&](double x) {
    return scale * random::normal(0, 1);
  });
}


/*
 * Figure out how many blocks of data to use for in-memory computation.
 *
 * Each thread loads into memory (sizeof(double) * dimension * num_block_rows)
 * bytes. This function figures out the upper bound on num_block_rows, given an
 * upper bound on the amount of memory each thread should use. NOTE: for now,
 * ignore the size of the projection matrix and the output matrix. These should
 * be accounted for but are typically much smaller than the original data.
 *
 * \param num_examples size_t Number of total observations.
 * \param dimension size_t Number of unpacked features in each data point.
 * \param max_thread_memory size_t Max memory to use in each thread (for the
 *                                 input data).
 *
 * \return num_blocks The ideal number of data blocks to use.
 */
size_t calculate_num_blocks(const size_t num_examples, const size_t dimension,
                            const size_t max_thread_memory) {

  double max_mem_rows = max_thread_memory / (sizeof(double) * dimension);
  size_t max_block_rows = static_cast<size_t>(std::max((double) 1,
                                                        max_mem_rows));

  logprogress_stream << "Max rows per data block: " << max_block_rows
                     << std::endl;

  size_t num_blocks = ((num_examples - 1) / max_block_rows) + 1;
  return num_blocks;
}


/*
 * Determine the unpacked dimension of a dataset.
 *
 * Only applies to integer, float, and array data. For arrays, the dimension is
 * the length of the first non-missing entry, up to `index_limit`. If
 * `index_limit` is reached without finding a non-missing-value, we throw an
 * error.
 */
size_t get_unpacked_dimension(const gl_sframe& data,
                              const std::vector<std::string>& feature_columns,
                              const size_t index_limit) {

  size_t dimension = 0;

  for (const auto& col_name : feature_columns) {

    flex_type_enum feature_type = data[col_name].dtype();

    if ((feature_type == flex_type_enum::INTEGER) ||
        (feature_type == flex_type_enum::FLOAT)) {

      dimension += 1;

    } else if (feature_type == flex_type_enum::VECTOR) {
      size_t array_length = 0;
      flex_type_enum array_value_type = flex_type_enum::UNDEFINED;
      size_t idx = 0;

      while (array_value_type == flex_type_enum::UNDEFINED && idx < index_limit) {
        array_length = data[col_name][idx].size();
        array_value_type = data[col_name][idx].get_type();
        idx++;
      }

      if (array_value_type == flex_type_enum::UNDEFINED) {
        std::stringstream ss;
        ss << "The dimension could not be determined for column '" << col_name
           << "' because the first several values are missing.";
        log_and_throw(ss.str());
      }

      dimension += array_length;

    } else {
      std::stringstream ss;
      ss << "Column '" << col_name << "' has an inappropriate type. "
         << "Columns must contain integers, floats, or arrays.";
      log_and_throw(ss.str());
    }
  }

  return dimension;
}


flex_vec random_projection_apply(const sframe_rows::row& row,
                                 const size_t original_dimension,
                                 const std::shared_ptr<dense_matrix> projection_matrix_ptr) {

  // Read data into an eigen dense vector.
  dense_vector x(original_dimension);
  size_t idx = 0;

  for (size_t j = 0; j < row.size(); ++j) {
    flex_type_enum col_type = row[j].get_type();

    // Check for missing values.
    if (col_type == flex_type_enum::UNDEFINED) {
      std::stringstream ss;
      ss << "A missing value has been found in the data to be transformed. "
         << "Missing values are not allowed in the transform data; consider "
         << "filling these values or dropping the rows with either "
         << "`SFrame.fillna` or `SFrame.dropna`.";
      log_and_throw(ss.str());
    }

    else if ((col_type == flex_type_enum::INTEGER) ||
             (col_type == flex_type_enum::FLOAT)) {

      x.coeffRef(idx) = row[j];
      idx++;

    } else if (col_type == flex_type_enum::VECTOR) {
      for (size_t k = 0; k < row[j].size(); ++k) {
        x.coeffRef(idx) = row[j][k];
        idx++;
      }
    }
  }

  // If the row isn't full, we've got a problem for the matrix multiplication in
  // the next step.
  if (idx != original_dimension) {
    std::stringstream ss;
    ss << "The dimension of the transform data does not match the "
       << "transformer's 'original_dimension' field, which was determined in "
       << "the `fit` method. Please ensure the number of features is the same "
       << "for all rows of data, including the number of entries in array-type "
       << "columns.";
    log_and_throw(ss.str());
  }

  // Embed the row by multiplying by the projection matrix.
  dense_vector y = x.transpose() * (*projection_matrix_ptr);

  // Convert the result back to a flex_vec.
  flex_vec row_out(y.size());

  for (size_t i = 0; i < static_cast<size_t>(y.size()); ++i) {
    row_out[i] = y[i];
  }

  return row_out;
}


/************************************
 ** RandomProjection class methods **
 ************************************/

/**
 * Define the options manager and set the initial options.
 *
 * \param _options
 */
void random_projection::init_options(
                         const std::map<std::string, flexible_type>& user_opts) {

  DASSERT_TRUE(options.get_option_info().size() == 0);

  options.create_string_option("output_column_name",
                               "Name of the embedded data in the output SFrame.",
                               "embedded_features",
                               true);

  options.create_integer_option("random_seed",
                                "Random seed for generating the projection matrix",
                                FLEX_UNDEFINED,
                                0,
                                std::numeric_limits<int>::max(),
                                true);

  options.create_integer_option("embedding_dimension",
                                "Dimension of the output data",
                                2,
                                1,
                                std::numeric_limits<int>::max(),
                                true);

  std::map<std::string, flexible_type> valid_opts;

  for (const auto& k: user_opts) {
    if (options.is_option(k.first)) {
      valid_opts[k.first] = variant_get_value<flexible_type>(k.second);
    }
  }

  options.set_options(valid_opts);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}


/**
 * Initialize the transformer.
 *
 * \param user_opts
 */
void random_projection::init_transformer(
                         const std::map<std::string, flexible_type>& user_opts) {

  // Initialize the options and set them in the model's state.
  init_options(user_opts);

  // Preprocessing of the features lists.
  unprocessed_features = user_opts.at("features");
  exclude = user_opts.at("exclude");

  if ((int) exclude == 1) {
    state["features"] = to_variant(FLEX_UNDEFINED);
    state["excluded_features"] = to_variant(unprocessed_features);

  } else {
    state["features"] = to_variant(unprocessed_features);
    state["excluded_features"] = to_variant(FLEX_UNDEFINED);
  }

  state["original_dimension"] = to_variant(FLEX_UNDEFINED);
  state["is_fitted"] = to_variant(false);
}


/**
 * Fit the random projection, based on the dimension of the input data.
 *
 * \param data
 */
void random_projection::fit(gl_sframe data) {

  DASSERT_TRUE(options.get_option_info().size() > 0);

  // Feature preprocessing
  // *********************

  // Get the set of features to work with. `feature_columns` is a vector of
  // strings.
  feature_columns = transform_utils::get_column_names(data, exclude,
                                                      unprocessed_features);

  // Select the features of the right type.
  std::vector<flex_type_enum> valid_feature_types = {flex_type_enum::FLOAT,
                                                     flex_type_enum::INTEGER,
                                                     flex_type_enum::VECTOR};

  feature_columns = transform_utils::select_valid_features(data,
                                                           feature_columns,
                                                           valid_feature_types);

  // Error out if specified features are missing.
  transform_utils::validate_feature_columns(data.column_names(),
                                            feature_columns);

  // Figure out the type of each input feature.
  feature_types.clear();

  for (auto& col_name : feature_columns){
    feature_types[col_name] = data[col_name].dtype();
  }

  // Figure out the original (i.e. ambient) dimension of the data.
  original_dimension = get_unpacked_dimension(data, feature_columns, 30);


  // Create the projection matrix
  // ****************************

  // Generate random entries in the projection matrix.
  size_t embedding_dimension
    = static_cast<size_t>(options.value("embedding_dimension"));

  flexible_type random_seed_option = options.value("random_seed");
  size_t random_seed;

  if (random_seed_option == FLEX_UNDEFINED) {
    random_seed = std::time(NULL);
    options.set_option("random_seed", random_seed);
  } else {
    random_seed = static_cast<size_t>(random_seed_option);
  }

  projection_matrix = std::make_shared<dense_matrix>();
  projection_matrix->resize(original_dimension, embedding_dimension);
  fill_gaussian_projection_matrix(projection_matrix, embedding_dimension,
                                  random_seed);
  fitted = true;


  // Update the model attributes visible to the Python user.
  state["random_seed"] = random_seed;
  state["is_fitted"] = to_variant(true);
  state["original_dimension"] = original_dimension;
  state["features"] = to_variant(feature_columns);
}


/**
 * Transform data into a low-dimensional space.
 *
 * \param data
 */
gl_sframe random_projection::transform(gl_sframe data) {

  //Check if fitting has already ocurred.
  if (!fitted) {
    std::stringstream ss;
    ss << "The RandomProjection object does not yet have a projection "
       << "matrix. Please use the 'fit' method to create one, or "
       << "use 'fit_transform' to create and apply the projection matrix "
       << "all at once.";
    log_and_throw(ss.str());
  }

  // Split the input data into feature columns and unprocessed columns. Also
  // ensures the input data has all of the columns specified in `fit`.
  gl_sframe transform_data = data.select_columns(feature_columns);

  gl_sframe output_data = data;
  for (const auto& col_name : feature_columns) {
    output_data.remove_column(col_name);
  }

  // Make sure the input data features have the right types.
  transform_utils::validate_feature_types(feature_columns, feature_types,
                                          transform_data);

  // Make sure the input data has the right dimension.
  size_t dimension_check = get_unpacked_dimension(transform_data,
                                                  feature_columns,
                                                  100);

  if (dimension_check != original_dimension) {
    std::stringstream ss;
    ss << "The original dimension of the transform does not match the created "
       << "projection matrix. Please re-fit with the data whose dimension is "
       << "correct, or simply use `fit_transform` with the current data.";
    log_and_throw(ss.str());
  }

  // Make sure the output column name is unique, or get a unique one.
  std::string output_name
    = transform_utils::get_unique_feature_name(output_data.column_names(),
                                           options.value("output_column_name"));
  state["output_column_name"] = to_variant(output_name);

  // Apply the projection to the transform features.
  size_t local_original_dimension = original_dimension;
  std::shared_ptr<dense_matrix> local_projection_matrix = projection_matrix;

  output_data[output_name] = transform_data.apply(
    [=](const sframe_rows::row& x) {
      return random_projection_apply(x, local_original_dimension,
                                     local_projection_matrix);
    },
    flex_type_enum::VECTOR);

  return output_data;
}



/**
 * Get the version number for a `random_projection` object.
 */
size_t random_projection::get_version() const {
  return RANDOM_PROJECTION_VERSION;
}


/**
 * Save a `random_projection` object using Turi's oarc.
 *
 * \param oarc
 */
void random_projection::save_impl(oarchive& oarc) const {
  variant_deep_save(state, oarc);
  oarc << options
       << unprocessed_features
       << feature_columns
       << feature_types
       << original_dimension
       << fitted
       << exclude;

  if (fitted) {
    oarc << *projection_matrix;
  }
}


/**
 * Load a `random_projection` object using Turi's iarc.
 *
 * \param iarc
 * \param version
 */
void random_projection::load_version(iarchive& iarc, size_t version) {
  variant_deep_load(state, iarc);
  iarc >> options
       >> unprocessed_features
       >> feature_columns
       >> feature_types
       >> original_dimension
       >> fitted
       >> exclude;

  if (fitted) {
    projection_matrix = std::make_shared<dense_matrix>();
    iarc >> *projection_matrix;
  }
}




} // namespace feature_engineering
} // namespace sdk_model
} // namespace turi


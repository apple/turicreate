/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DIMENSION_REDUCTION_H_
#define TURI_DIMENSION_REDUCTION_H_
#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/toolkits/feature_engineering/transformer_base.hpp>
#include <Eigen/Core>
#include <export.hpp>

namespace turi {
namespace sdk_model {
namespace feature_engineering {

typedef Eigen::MatrixXd dense_matrix;
typedef Eigen::Matrix<double, Eigen::Dynamic, 1> dense_vector;


/**
 * Create a random projection matrix if the dimenions were already set in the
 * constructor, in which case no data is needed.
 *
 * The Gaussian random projection is Y = (1 / \sqrt(k)) X * R, where:
 *   - X is the original data (n x d)
 *   - R is the projection matrix (d x k)
 *   - Y is the output data (n x k)
 *   - k is the embedding dimension, i.e. `embedding_dim`.
 *
 * See Achlioptas (2003) and Li, Hastie, and Church (2006) for details. Our
 * naming convention is to call d the "ambient dimension" and k the "embedded
 * dimension".
 * -----------------------------------------------------------------------------
 *
 * The private members of a random_projection instance are:
 *
 * unprocessed_features:
 *   Column names before feature validation and preprocessing, particularly
 *   whether the names are to be included or excluded.
 *
 * feature_columns:
 *   Feature column names after validation and pre-processing. These are the
 *   actual columns that we will work with.
 *
 * original_dimension
 *   Dimension of the data input to the `transform` function, as determined by
 *   data passed to the `fit` function.
 *
 * projection_matrix:
 *   Where the rubber meets the road. This is post-multiplied by the data to
 *   produce the output data.
 *
 * fitted:
 *   Indicates if the model has been fitted yet.
 *
 * exclude:
 *   Indicates if the `unprocessed_features` should be included or excluded.
 * -----------------------------------------------------------------------------
 *
 * Several items are added to the model's state (in addition to the options
 * defined in `init_options`) so they will be visible to the Python user. More
 * information about these can be found in the Python documentation. The items
 * are:
 *
 * - original_dimension: dimension of the input data, unpacked.
 * - features: list of column names to project.
 * - excluded_features: list of column names to exclude.
 * - random_seed: seed for generating the projection matrix.
 */
class EXPORT random_projection : public transformer_base {

  static constexpr size_t RANDOM_PROJECTION_VERSION = 0;

  flexible_type unprocessed_features;
  std::vector<std::string> feature_columns;
  std::map<std::string, flex_type_enum> feature_types;

  size_t original_dimension = 0;
  std::shared_ptr<dense_matrix> projection_matrix;

  bool fitted = false;
  bool exclude = false;

 public:

  virtual inline ~random_projection() {}

  /**
   * Define the options manager and set the initial options.
   */
  void init_options(const std::map<std::string, flexible_type>& user_opts) override;

  /**
   * Get the version number for a `random_projection` object.
   */
  size_t get_version() const override;

  /**
   * Save a `random_projection` object using Turi's oarc.
   */
  void save_impl(oarchive& iarc) const override;

  /**
   * Load a `random_projection` object using Turi's iarc.
   */
  void load_version(iarchive & iarc, size_t version) override;

  /**
   * Initialize the transformer. This is the primary entry points for C++ users.
   */
  void init_transformer(const std::map<std::string, flexible_type>& user_opts) override;

  /**
   * Fit the random projection. There is no real logic to write here, right?
   */
  void fit(gl_sframe data) override;

  /**
   * Transform data into a low-dimensional space.
   */
  gl_sframe transform(gl_sframe data) override;

  /**
   * Fit and transform the given data. Intended as an optimization because
   * fit and transform are usually always called together. The default
   * implementaiton calls fit and then transform.
   *
   * \param data
   */
  gl_sframe fit_transform(gl_sframe data) {
    data.materialize();
    fit(data);
    return transform(data);
  }

  BEGIN_CLASS_MEMBER_REGISTRATION("_RandomProjection")
  REGISTER_CLASS_MEMBER_FUNCTION(random_projection::init_transformer, "user_opts")
  REGISTER_CLASS_MEMBER_FUNCTION(random_projection::fit, "data")
  REGISTER_CLASS_MEMBER_FUNCTION(random_projection::transform, "data")
  REGISTER_CLASS_MEMBER_FUNCTION(random_projection::fit_transform, "data")
  REGISTER_CLASS_MEMBER_FUNCTION(random_projection::get_current_options);
  REGISTER_CLASS_MEMBER_FUNCTION(random_projection::list_fields);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("_get_default_options",
                                       random_projection::get_default_options);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("get",
                                       random_projection::get_value_from_state,
                                       "key");

  END_CLASS_MEMBER_REGISTRATION
};

} //namespace feature_engineering
} //namespace sdk_model
} // namespace turi
#endif

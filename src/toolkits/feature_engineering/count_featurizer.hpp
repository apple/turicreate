/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FEATURE_ENGINEERING_COUNT_FEATURIZER_HPP
#define TURI_FEATURE_ENGINEERING_COUNT_FEATURIZER_HPP
#include <model_server/lib/toolkit_class_macros.hpp>
#include <toolkits/feature_engineering/transformer_base.hpp>
#include <ml/sketches/countmin.hpp>
#include <core/export.hpp>
using namespace turi;


namespace turi {
namespace sdk_model {
namespace feature_engineering {

/**
 * An approximate, limited memory implementation of the feature engineering
 * mechanism by Misha Bilenko in
 * https://blogs.technet.microsoft.com/machinelearning/2015/02/17/big-learning-made-easy-with-counts/
 * .
 *
 * For some k-ary classification task assuming we are going to try to predict
 * column Y. Then for every column, X, we replace it with 2k-1 numeric features:
 * Assuming the value X in row i is x_i.
 * We replace it two columns:
 * count_X: a list of the following values
 *    - #(Y = 0 & X = x_i)   the number of times Y = 0 when X has the value x_i
 *    - #(Y = 1 & X = x_i)
 *    - ...
 *    - #(Y = k-1 & X = x_i)
 * prob_X: a list of the following values
 *    - P(Y = 0 | X = x_i)   the probability Y = 0 when X has the value x_i
 *    - P(Y = 1 | X = x_i)
 *    - ...
 *    - P(Y = k-2 | X = x_i)
 *
 * This procedure is generally quite memory intensive, requiring the
 * count table #(Y=y,  X=x) to be built up for every column, requiring
 * O(k * N(X)) memory per column where N(X) is the number of unique values of X.
 *
 * Instead, we will approximate the count table this way:
 * For each input column X,
 *  - We maintain k count tables. i.e. one count for each Y value, where
 *    each count table is represented by a count-min-sketch.
 *  http://www.cs.rutgers.edu/~muthu/cmz-sdm.pdf provides some analysis on the
 *  use of the count-min sketch and empirical validation as to why the
 *  count-min sketch is preferred over the count sketch in the power-law
 *  distribution setting.
 *
 *  This provides counts which are inherently upper bounds on the actual
 *  counts. Therefore, estimating
 *  P(Y = 0 | X = x_i) with #(Y = 0 & X = x_i) / #(X = x_i)
 *  is problematic since if #(X = x_i) is estimated indepedendently,
 *  the probabilities may not sum to 1.
 *  Instead #(X = x_i) should be estimated with \sum_y #(Y = y & X = x_i)
 */
class EXPORT count_featurizer : public transformer_base {
 public:
  static constexpr size_t count_featurizer_VERSION = 0;
  static constexpr char COUNTS_PREFIX[] = "count_";
  static constexpr char PROBABILITY_PREFIX[] = "prob_";

  count_featurizer() = default;

  // serializers
  size_t get_version() const override;
  void save_impl(oarchive& oarc) const override;
  void load_version(iarchive& iarc, size_t version) override;

  // transformer
  void init_options(const std::map<std::string,
                    flexible_type>& _options) override;
  void init_transformer(const std::map<std::string,
                        flexible_type>& _options) override;
  void fit(gl_sframe data) override;
  gl_sframe transform(gl_sframe data) override;
  gl_sframe fit_transform(gl_sframe data);

  // collect the state in a single shared pointer so that this can be
  // shared across to the lazy apply.
  struct transform_state {
    // random seed
    size_t seed = 0;
    double laplace_smearing = 0.0;
    std::string count_column_prefix;
    std::string prob_column_prefix;
    std::vector<flexible_type> y_values;
    // counters[i][colnumber] contains the sketch for column colnumber
    // and y value y_values[i].
    std::vector<
          std::vector<sketches::countmin<flexible_type>>
          > counters;
  };

  BEGIN_CLASS_MEMBER_REGISTRATION("_CountFeaturizer")
  REGISTER_CLASS_MEMBER_FUNCTION(count_featurizer::init_transformer, "_options")
  REGISTER_CLASS_MEMBER_FUNCTION(count_featurizer::fit, "data")
  REGISTER_CLASS_MEMBER_FUNCTION(count_featurizer::transform, "data")
  REGISTER_CLASS_MEMBER_FUNCTION(count_featurizer::fit_transform, "data")
  REGISTER_CLASS_MEMBER_FUNCTION(count_featurizer::get_current_options);
  REGISTER_CLASS_MEMBER_FUNCTION(count_featurizer::list_fields);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("_get_default_options",
                             count_featurizer::get_default_options);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("get",
                             count_featurizer::get_value_from_state,
                             "key");

  END_CLASS_MEMBER_REGISTRATION
 private:
  std::vector<std::string> feature_columns;
  flexible_type unprocessed_features;
  bool exclude = false;
  bool fitted = false;

  turi::mutex m_lock;

  std::shared_ptr<transform_state> m_state;
};

} // namespace feature_engineering
} // namespace sdk_model
} // namespace turi
#endif

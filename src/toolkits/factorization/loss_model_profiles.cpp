/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cmath>
#include <string>
#include <toolkits/factorization/loss_model_profiles.hpp>

namespace turi { namespace factorization {

void _logistic_loss_value_is_bad(double v) {
  log_and_throw(std::string("Training with binary_target=True requires targets to be 0 or 1; (")
                + std::to_string(v) + " invalid).");
}

/// A quick wrapper function to retrieve the correct profile by name.
std::shared_ptr<loss_model_profile> get_loss_model_profile(
    const std::string& name) {

  std::shared_ptr<loss_model_profile> ret;

  if(name == loss_squared_error::name())
    ret.reset(new loss_squared_error);
  else if (name == loss_logistic::name())
    ret.reset(new loss_logistic);
  else if (name == loss_ranking_hinge::name())
    ret.reset(new loss_ranking_hinge);
  else if (name == loss_ranking_logit::name())
    ret.reset(new loss_ranking_logit);
  else
    ASSERT_FALSE(true);

  return ret;
}

}}

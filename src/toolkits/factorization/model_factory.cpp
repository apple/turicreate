/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/factorization/model_factory.hpp>
#include <toolkits/sgd/basic_sgd_solver.hpp>
#include <toolkits/sgd/sgd_interface.hpp>
#include <toolkits/factorization/ranking_sgd_solver_explicit.hpp>
#include <toolkits/factorization/ranking_sgd_solver_implicit.hpp>

#include <string>
#include <cstdlib>

// This macros suppress all the solvers.  They are reinstantiated by 4
// other files given in factory/etc.
SUPPRESS_SOLVERS();

namespace turi { namespace factorization {

std::pair<std::shared_ptr<factorization_model>,
          std::shared_ptr<sgd::sgd_solver_base> >
create_model_and_solver(const v2::ml_data& train_data,
                        std::map<std::string, flexible_type> options,
                        const std::string& loss_type,
                        const std::string& solver_class,
                        const std::string& regularization_type,
                        const std::string& factor_mode,
                        flex_int num_factors) {

  // Work with a couple of the options to handle dependencies.
  if(options.at("solver") == "auto") {
    options["solver"] = "adagrad";
  }

  if(options.at("solver") == "adagrad") {
    // Turn off the sgd step size decrease.
    options["step_size_decrease_rate"] = 0;
    options["sgd_step_adjustment_interval"] = 0;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // FINALLY, actually instantiate the above.

  CREATE_RETURN_SOLVER(train_data, options);
}

}}

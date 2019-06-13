/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGD_SGD_INTERFACE_BASE_H_
#define TURI_SGD_SGD_INTERFACE_BASE_H_

#include <toolkits/sgd/sgd_interface.hpp>

namespace turi { namespace sgd {


/** The base class for the ranking SGD interfaces.  This interface
 *  governs all the interactions between the sgd solvers and the
 *  model.
 *
 *  To use the ranking sgd solver, implement the following options.
 */
class sgd_ranking_interface : sgd_interface_base {
 public:

  /** Apply two sgd steps to the code to increase the predicted value
   *  of x_positive and decrease the predicted value of x_negative.
   */
  virtual double apply_pairwise_sgd_step(
      size_t thread_idx,
      const std::vector<ml_data_entry>& x_positive,
      const std::vector<ml_data_entry>& x_negative,
      double step_size) = 0;
};

}}

#endif /* TURI_SGD_SGD_INTERFACE_BASE_H_ */

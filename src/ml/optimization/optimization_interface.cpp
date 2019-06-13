/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <ml/optimization/optimization_interface.hpp>

namespace turi {

namespace optimization {

/**************************************************************************/
/*                                                                        */
/*                First Order Optimization Interface                      */
/*                                                                        */
/**************************************************************************/

/**
 * Desctuctor
 */
first_order_opt_interface::~first_order_opt_interface(){
}


/**
 *
 * Default implementation of "dense" compute_gradient.
 *
 * Computes dense gradient by first calling "sparse" compute_gradient and
 * casting the result to a dense vector.
 *
 * \warning Not efficient. Requires an overwrite by the model writer.
 */
void first_order_opt_interface::compute_gradient(const DenseVector& point,
    DenseVector& gradient, const size_t mbStart, const size_t mbSize){
    double func = 0.0;
    compute_first_order_statistics(point, gradient, func);
}

/**
 *
 * Default implementation of computing function value.
 *
 * \warning Not efficient. Requires an overwrite by the model writer.
 */
double first_order_opt_interface::compute_function_value(
    const DenseVector& point, const size_t mbStart, const size_t mbSize){
    double func = 0.0;
    DenseVector gradient = point;
    compute_first_order_statistics(point, gradient, func);
    return func;
}


/**
 * Reset the state of the model's "randomness" source.
 *
 * \note Default implementation does nothing.
 *
 */
void first_order_opt_interface::reset(int seed) {
}

/**
 * Get strings needed to print the header for the progress table.
 */
std::vector<std::pair<std::string, size_t>>
  first_order_opt_interface::get_status_header(const std::vector<std::string>& stats) {
  auto header = std::vector<std::pair<std::string, size_t>>();
  for (const auto& s : stats) {
    header.push_back({s, 8});
  }
  return header;
}

/**
 * Get strings needed to print a row of the progress table.
 */
std::vector<std::string>
  first_order_opt_interface::get_status(const DenseVector& coefs,
                                        const std::vector<std::string>& stats) {
  return stats;
}


/**************************************************************************/
/*                                                                        */
/*                Second Order Optimization Interface                     */
/*                                                                        */
/**************************************************************************/

/**
 * Destructor
 */
second_order_opt_interface::~second_order_opt_interface(){
}

/**
 *
 * Default implementation of "dense" compute_hessian.
 *
 * \note The default implementation computes the statistics by making separate
 * passes over the data. This is not efficient
 *
 * \warning Not efficient. Requires an overwrite by the model writer.
 */
void second_order_opt_interface::compute_hessian(
    const DenseVector& point, DenseMatrix& hessian) {
    DenseVector gradient = point;
    double func = 0.0;
    compute_second_order_statistics(point, hessian, gradient, func);
}


} // optimization

} // turicreate

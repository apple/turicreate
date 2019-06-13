/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SFRAME_UTILS_H_
#define TURI_UNITY_SFRAME_UTILS_H_

#include <functional>
#include <string>
#include <core/storage/sframe_data/sarray.hpp>
#include <core/data/flexible_type/flexible_type_base_types.hpp>
#include <Eigen/Core>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/parallel/pthread_tools.hpp>

namespace turi {

class ml_data;
class unity_graph;

/**
 * \ingroup toolkit_util
 * Filters the rows of an sframe into the first (false) or second
 * (true) sframe by a switch function.
 */
 std::pair<sframe,sframe> split_sframe_on_index(const sframe& src, std::function<bool(size_t)> switch_function);

 /**
 * \ingroup toolkit_util
  * Create an SArray of vector type, where each element is a row of the
  * provided matrix.
  */
 std::shared_ptr<sarray<flexible_type>> matrix_to_sarray(const Eigen::MatrixXd& m);

/**
 * \ingroup toolkit_util
 * Generate an SFrame from a transform function that takes an index
 *   and fills a vector of flexible type.  The signature of the
 *   transform function should be:
 *
 *     gen_fill_func(size_t row_index, std::vector<flexible_type>& out_values);
 *
 *   Access there is done in parallel.
 *
 s*/
template <typename GenFunction>
sframe sframe_from_ranged_generator(const std::vector<std::string>& column_names,
                                    const std::vector<flex_type_enum>& column_types,
                                    size_t num_rows,
                                    GenFunction&& generator_function) {

  sframe out;

  out.open_for_write(column_names, column_types, "", thread::cpu_count());

    // Really inefficient due to the transpose.
  in_parallel([&](size_t thread_index, size_t num_threads) {
      std::vector<flexible_type> out_values(column_names.size());

      size_t start_idx = (num_rows * thread_index) / num_threads;
      size_t end_idx = (num_rows * (thread_index + 1)) / num_threads;

      auto it_out = out.get_output_iterator(thread_index);

      for(size_t i = start_idx; i < end_idx; ++i, ++it_out) {
        generator_function(i, out_values);
        DASSERT_EQ(column_names.size(), out_values.size());
        *it_out = out_values;
      }
    });

  out.close();

  return out;
}



}

#endif /* TURI_SFRAME_UTILS_H_ */

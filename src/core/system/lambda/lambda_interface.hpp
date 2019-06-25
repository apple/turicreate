/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_LAMBDA_LAMBDA_INTERFACE_HPP
#define TURI_LAMBDA_LAMBDA_INTERFACE_HPP
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/system/cppipc/cppipc.hpp>
#include <core/storage/sframe_data/sframe_rows.hpp>
#include <core/system/cppipc/magic_macros.hpp>

namespace turi {

namespace lambda {

enum class bulk_eval_serialized_tag:char {
  BULK_EVAL_ROWS = 0,
  BULK_EVAL_DICT_ROWS = 1,
};

GENERATE_INTERFACE_AND_PROXY(lambda_evaluator_interface, lambda_evaluator_proxy,
      (size_t, make_lambda, (const std::string&))
      (void, release_lambda, (size_t))
      (std::vector<flexible_type>, bulk_eval, (size_t)(const std::vector<flexible_type>&)(bool)(int))
      (std::vector<flexible_type>, bulk_eval_rows, (size_t)(const sframe_rows&)(bool)(int))
      (std::vector<flexible_type>, bulk_eval_dict, (size_t)(const std::vector<std::string>&)(const std::vector<std::vector<flexible_type>>&)(bool)(int))
      (std::vector<flexible_type>, bulk_eval_dict_rows, (size_t)(const std::vector<std::string>&)(const sframe_rows&)(bool)(int))
      (std::string, initialize_shared_memory_comm, )
    )
} // namespace lambda
} // namespace turi

#endif

/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_TOOLKIT_RESPONSE_TYPE_HPP
#define TURI_UNITY_TOOLKIT_RESPONSE_TYPE_HPP
#include <string>
#include <model_server/lib/variant.hpp>
#include <core/storage/serialization/serialization_includes.hpp>

namespace turi {

/**
 * \ingroup unity
 * The response from a toolkit
 */
struct toolkit_function_response_type {
  /**
   * Whether the toolkit was executed successfully
   */
  bool success = true;

  /**
   * Any other messages to be printed.
   */
  std::string message;

  /**
   * The returned parameters. (Details will vary from toolkit to toolkit)
   */
  variant_map_type params;

  void save(oarchive& oarc) const {
    log_func_entry();
    oarc << success << message << params;
  }

  void load(iarchive& iarc) {
    log_func_entry();
    iarc >> success >> message >> params;
  }
};



} // turicreate

#endif // TURI_UNITY_TOOLKIT_RESPONSE_TYPE_HPP

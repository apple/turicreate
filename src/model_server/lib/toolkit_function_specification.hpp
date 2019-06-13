/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TOOLKIT_FUNCTION_SPECIFICATION_HPP
#define TURI_TOOLKIT_FUNCTION_SPECIFICATION_HPP
#include <string>
#include <functional>
#include <model_server/lib/toolkit_function_response.hpp>
#include <model_server/lib/toolkit_function_invocation.hpp>
namespace turi {


/**
 * \ingroup unity
 * Each toolkit is specified by filling in \ref toolkit_function_specification struct.
 * The contents of the struct describe user-facing documentation and default
 * options, as well as a callback to actual toolkit execution.
 */
struct toolkit_function_specification {
  /**
   * A short name used to identify this toolkit. For instance,
   * LDA, or PageRank.
   */
  std::string name;

  /**
   * A list of required configurable parameters and their default values.
   */
  variant_map_type default_options;

  /**
   * Toolkit properties.
   * The following keys are recognized:
   *  - "arguments": value must a flex_list containing a list of
   *                 the argument names.
   *  - "file": The file which the toolkit was loaded from
   *  - "documentation": A documentation string
   */
  std::map<std::string, flexible_type> description;

  /**
   * A pointer to the actual execution function. All parameters to the
   * execution are passed in the \ref toolkit_function_invocation struct.
   * Returns an std::pair<bool, options_map> with status results.
   *
   * \note this can be generated easily using toolkit_function_wrapper_impl::make_spec
   */
  std::function<toolkit_function_response_type(toolkit_function_invocation&)> toolkit_execute_function;

  /**
   * A pointer to a simple version of the toolkit execution function which can be
   * executed natively without a toolkit_function_invocation. It will not have
   * some of the error management/reporting capabilities of the invocation object,
   * and does not have named parameters. But it is much simpler.
   */
  std::function<variant_type(const std::vector<variant_type>& invoke)> native_execute_function;
};

} // namespace turi

#endif

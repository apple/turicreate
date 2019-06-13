/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_TOOLKIT_INVOCATION_HPP
#define TURI_UNITY_TOOLKIT_INVOCATION_HPP
#include <string>
#include <functional>
#include <model_server/lib/variant.hpp>
#include <model_server/lib/toolkit_class_registry.hpp>
namespace turi {

/**
 * \ingroup unity
 * The arguments used to invoke the toolkit execution.
 * See \ref toolkit_function_specification for details.
 */
struct toolkit_function_invocation {
  toolkit_function_invocation() {
    progress = [=](std::string s) {
      logstream(LOG_INFO) << "PROGRESS: " << s << std::endl;
    };
  }
  /**
   * The parameters passed to the toolkit from the user.
   * The options set will be cleaned: every option in
   * \ref toolkit_function_specification::default_options will show appear here,
   * and there will not be extraneous options.
   */
  variant_map_type params;

  /**
   * A pointer to a function which prints execution progress.
   */
  std::function<void(std::string)> progress;

  /**
   * A pointer to the class registry.
   */
  toolkit_class_registry* classes;
};

}

#endif

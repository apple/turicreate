/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SERVER_UNITY_SERVER_INIT_HPP
#define TURI_UNITY_SERVER_UNITY_SERVER_INIT_HPP

#include <model_server/lib/toolkit_class_registry.hpp>
#include <model_server/lib/toolkit_function_registry.hpp>

namespace turi {

class EXPORT unity_server_initializer {
 public:

  virtual ~unity_server_initializer();

  /**
   * Fill the registry of internal toolkits
   */
  virtual void init_toolkits(toolkit_function_registry&) const;

  /**
   * Fill the registry of internal models
   */
  virtual void init_models(toolkit_class_registry&) const;

  /**
   * Load external extensions into unity global
   */
  virtual void init_extensions(const std::string& root_path, std::shared_ptr<unity_global> unity_global_ptr) const;

};

}
#endif

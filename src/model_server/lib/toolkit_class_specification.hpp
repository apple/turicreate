/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_LIB_TOOLKIT_CLASS_SPECIFICATION_HPP
#define TURI_UNITY_LIB_TOOLKIT_CLASS_SPECIFICATION_HPP

#include <string>
#include <core/data/flexible_type/flexible_type.hpp>
namespace turi {
class model_base;
/**
 * \ingroup unity
 * Each model is specified by filling in \ref toolkit_model_specification struct.
 * The contents of the struct describe user-facing documentation and default
 * options, as well as a callback to actual toolkit execution.
 */
struct toolkit_class_specification {
  /**
   * A short name used to identify this toolkit. For instance,
   * LDA, or PageRank.
   */
  std::string name;

  /**
   * Model properties.
   * The following keys are recognized.
   *  - "functions": A dictionary with key: function name, and value,
   *                     a list of input parameters.
   *  - "get_properties": The list of all readable properties of the model
   *  - "set_properties": The list of all writable properties of the model
   *  - "file": The file which the toolkit was loaded from
   *  - "documentation": A documentation string
   */
  std::map<std::string, flexible_type> description;

  /**
   * A callback function to call to construct a model
   */
  model_base* (*constructor)();
};

} // namespace turi
#endif // TURI_UNITY_LIB_TOOLKIT_MODEL_SPECIFICATION_HPP

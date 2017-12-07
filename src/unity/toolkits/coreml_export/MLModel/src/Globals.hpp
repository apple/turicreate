/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef MLMODEL_GLOBALS_HPP
#define MLMODEL_GLOBALS_HPP

#include <memory>
#include <string>
#include <vector>

#include "DataType.hpp"
#include "Export.hpp"

namespace CoreML {

  typedef std::vector<std::pair<std::string, FeatureType>> SchemaType;
  static const int32_t MLMODEL_SPECIFICATION_VERSION = 1;
}

#endif

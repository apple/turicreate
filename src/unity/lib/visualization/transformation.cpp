/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "transformation.hpp"
#include "summary_view.hpp"

using namespace turi;
using namespace turi::visualization;

std::shared_ptr<summary_view_transformation> transformation_collection::fuse(std::vector<std::string> column_names, std::vector<flex_type_enum> column_types, size_t size) {
  auto fused_output = std::make_shared<summary_view_transformation>(*this);
  fused_output->column_names = column_names;
  fused_output->column_types = column_types;
  fused_output->size = size;
  return fused_output;
}

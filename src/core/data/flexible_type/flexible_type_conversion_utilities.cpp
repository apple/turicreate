/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <core/data/flexible_type/flexible_type_conversion_utilities.hpp>

using namespace turi;
using namespace turi::flexible_type_internals;

type_conversion_error::type_conversion_error(const std::string& message) : std::runtime_error(message) {}
type_conversion_error::type_conversion_error(const char* message) : std::runtime_error(message) {}

void flexible_type_internals::throw_type_conversion_error(const flexible_type& val, const char *type) {
  std::ostringstream ss;
  ss << "Type conversion failure in flexible_type converter: expected "
     << type << "; got " << flex_type_enum_to_name(val.get_type());

  throw type_conversion_error(ss.str());
}

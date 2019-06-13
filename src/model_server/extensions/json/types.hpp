/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <string>

namespace turi {
  namespace JSON {
    namespace types {
      static const std::string INTEGER = flex_type_enum_to_name(flex_type_enum::INTEGER);
      static const std::string FLOAT = flex_type_enum_to_name(flex_type_enum::FLOAT);
      static const std::string STRING = flex_type_enum_to_name(flex_type_enum::STRING);
      static const std::string VECTOR = flex_type_enum_to_name(flex_type_enum::VECTOR);
      static const std::string LIST = flex_type_enum_to_name(flex_type_enum::LIST);
      static const std::string DICT = flex_type_enum_to_name(flex_type_enum::DICT);
      static const std::string DATETIME = flex_type_enum_to_name(flex_type_enum::DATETIME);
      static const std::string UNDEFINED = flex_type_enum_to_name(flex_type_enum::UNDEFINED);
      static const std::string IMAGE = flex_type_enum_to_name(flex_type_enum::IMAGE);
      static const std::string SARRAY = "SArray";
      static const std::string SFRAME = "SFrame";
      static const std::string SGRAPH = "SGraph";
    };
  }
}

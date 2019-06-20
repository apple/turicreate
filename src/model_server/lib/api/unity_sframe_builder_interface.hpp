/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SFRAME_BUILDER_INTERFACE_HPP
#define TURI_UNITY_SFRAME_BUILDER_INTERFACE_HPP
#include <vector>
#include <string>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/system/cppipc/magic_macros.hpp>

namespace turi {

class unity_sframe_base;

GENERATE_INTERFACE_AND_PROXY(unity_sframe_builder_base, unity_sframe_builder_proxy,
      (void, init, (size_t)(size_t)(std::vector<std::string>)(std::vector<flex_type_enum>)(std::string))
      (void, append, (const std::vector<flexible_type>&)(size_t))
      (void, append_multiple, (const std::vector<std::vector<flexible_type>>&)(size_t))
      (std::vector<std::string>, column_names, )
      (std::vector<flex_type_enum>, column_types, )
      (std::vector<std::vector<flexible_type>>, read_history, (size_t)(size_t))
      (std::shared_ptr<unity_sframe_base>, close, )
    )

} // namespace turi

#endif //TURI_UNITY_SFRAME_BUILDER_INTERFACE_HPP
#include <model_server/lib/api/unity_sframe_interface.hpp>

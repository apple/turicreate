/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SKETCH_INTERFACE_HPP
#define TURI_UNITY_SKETCH_INTERFACE_HPP
#include <memory>
#include <vector>
#include <map>
#include <string>

#include <core/data/flexible_type/flexible_type.hpp>
#include <model_server/lib/api/unity_sarray_interface.hpp>
#include <core/system/cppipc/magic_macros.hpp>

namespace turi {
class unity_sketch_base;

typedef std::pair<flexible_type, size_t> item_count;
typedef std::map<flexible_type, std::shared_ptr<unity_sketch_base>> sub_sketch_map;

GENERATE_INTERFACE_AND_PROXY(unity_sketch_base, unity_sketch_proxy,
    (void, construct_from_sarray, (std::shared_ptr<unity_sarray_base>)(bool)(const std::vector<flexible_type>&))
    (double, get_quantile, (double))
    (double, frequency_count, (flexible_type))
    (std::vector<item_count>, frequent_items, )
    (double, num_unique, )
    (double, mean, )
    (double, max, )
    (double, min, )
    (double, var, )
    (size_t, size, )
    (double, sum, )
    (size_t, num_undefined, )
    (bool, sketch_ready, )
    (size_t, num_elements_processed, )
    (std::shared_ptr<unity_sketch_base>, element_summary, )
    (std::shared_ptr<unity_sketch_base>, element_length_summary, )
    (std::shared_ptr<unity_sketch_base>, dict_key_summary, )
    (std::shared_ptr<unity_sketch_base>, dict_value_summary, )
    (sub_sketch_map, element_sub_sketch, (const std::vector<flexible_type>&))
    (void, cancel, )
)

} // namespace turi
#endif // TURI_UNITY_GRAPH_INTERFACE_HPP

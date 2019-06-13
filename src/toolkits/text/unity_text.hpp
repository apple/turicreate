/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_TEXT_H
#define TURI_UNITY_TEXT_H

#include <model_server/lib/toolkit_function_specification.hpp>
#include <model_server/lib/variant.hpp>

namespace turi {
namespace text {

/**
 *
 */
variant_map_type init(variant_map_type& params);

/**
 */
variant_map_type train(variant_map_type& params);

/**
 */
variant_map_type get_topic(variant_map_type& params);

/**
 *
 */
variant_map_type predict(variant_map_type& params);

/**
 */
variant_map_type summary(variant_map_type& params);

/**
 */
variant_map_type get_training_stats(variant_map_type& params);


std::vector<toolkit_function_specification> get_toolkit_function_registration();

}
}

#endif /* TURI_UNITY_TEXT_H */

/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <core/storage/serialization/serialization_includes.hpp>
#include <ml/neural_net/float_array.hpp>

namespace turi {

// To allow the neural_net library to stand alone, the serialization code that
// depends on core/storage/serialization lives here.
// TODO: Consider converting float_array values to flex_nd_vec, but we must
// support at least importing this serialization that released with TC 6.0.

void save_float_array_map(const neural_net::float_array_map& weights,
                          oarchive& oarc);
neural_net::float_array_map load_float_array_map(iarchive& iarc);

}  // namespace turi

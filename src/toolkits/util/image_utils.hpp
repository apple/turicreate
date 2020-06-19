/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <core/data/image/image_type.hpp>
#include <ml/neural_net/Image.hpp>

namespace turi {

/// Converts from the Turi image_type to the neural_net Image type.
std::shared_ptr<neural_net::Image> wrap_image(const image_type& image);

}  // namespace turi

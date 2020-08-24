/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/util/image_utils.hpp>

#include <ml/neural_net/PortableImage.hpp>

namespace turi {

/// Converts from the Turi image_type to the neural_net Image type.
std::shared_ptr<neural_net::Image> wrap_image(const image_type& image)
{
  return std::make_shared<neural_net::PortableImage>(image);
}

}  // namespace turi

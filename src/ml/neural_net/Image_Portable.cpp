/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/Image.hpp>

#include <ml/neural_net/PortableImage.hpp>

namespace turi {
namespace neural_net {

// static
std::unique_ptr<Image> Image::CreateFromPath(const std::string& path)
{
  return std::unique_ptr<PortableImage>(new PortableImage(path));
}

Image::~Image() = default;

}  // namespace neural_net
}  // namespace turi

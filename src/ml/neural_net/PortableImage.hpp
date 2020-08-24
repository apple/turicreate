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
namespace neural_net {

/// Concrete implementation of the Image interface that wraps the portable
/// image_type class.
class PortableImage : public Image {
 public:
  explicit PortableImage(image_type impl);
  explicit PortableImage(const std::string& path);

  // Copyable and movable.
  PortableImage(const PortableImage&);
  PortableImage(PortableImage&&);
  PortableImage& operator=(const PortableImage&);
  PortableImage& operator=(PortableImage&&);

  ~PortableImage() override;

  size_t Height() const override;
  size_t Width() const override;
  void WriteCHW(Span<float> buffer) const override;
  void WriteHWC(Span<float> buffer) const override;

 private:
  image_type impl_;
};

}  // namespace neural_net
}  // namespace turi

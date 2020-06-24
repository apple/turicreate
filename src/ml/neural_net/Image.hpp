/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <memory>
#include <string>

#include <core/util/Span.hpp>

namespace turi {
namespace neural_net {

/// Abstract interface for images that a training pipeline can consume.
class Image {
 public:
  static std::unique_ptr<Image> CreateFromPath(const std::string& path);

  virtual ~Image();

  /// The number of rows of pixels.
  virtual size_t Height() const = 0;

  /// The number of columns of pixels.
  virtual size_t Width() const = 0;

  /// The size (in elements) of a float buffer large enough to contain this
  /// image. At present, all images are assumed RGB.
  size_t Size() const { return 3 * Height() * Width(); }

  /// Writes the image in CHW order to the provided span or throws if the span does not have the
  /// expected size.
  virtual void WriteCHW(Span<float> buffer) const = 0;

  /// Writes the image in HWC order to the provided span or throws if the span does not have the
  /// expected size.
  virtual void WriteHWC(Span<float> buffer) const = 0;
};

}  // namespace neural_net
}  // namespace turi

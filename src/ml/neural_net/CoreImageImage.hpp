/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <ml/neural_net/Image.hpp>

#import <CoreImage/CoreImage.h>

namespace turi {
namespace neural_net {

/// Concrete implementation of the Image interface that wraps a CIImage.
class CoreImageImage : public Image {
 public:
  explicit CoreImageImage(CIImage* impl);
  explicit CoreImageImage(const std::string& path);

  // Copyable and movable.
  CoreImageImage(const CoreImageImage&);
  CoreImageImage(CoreImageImage&&);
  CoreImageImage& operator=(const CoreImageImage&);
  CoreImageImage& operator=(CoreImageImage&&);

  ~CoreImageImage() override;

  CIImage* AsCIImage() const { return impl_; }

  size_t Height() const override;
  size_t Width() const override;
  void WriteCHW(Span<float> buffer) const override;
  void WriteHWC(Span<float> buffer) const override;

 private:
  CIImage* impl_ = nil;
};

}  // namespace neural_net
}  // namespace turi

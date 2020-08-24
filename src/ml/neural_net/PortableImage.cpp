/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/PortableImage.hpp>

#include <core/data/image/io.hpp>
#include <core/util/Verify.hpp>
#include <model_server/lib/image_util.hpp>

namespace turi {
namespace neural_net {

PortableImage::PortableImage(image_type impl)
  : impl_(std::move(impl))
{
}

PortableImage::PortableImage(const std::string& path)
  : PortableImage(read_image(path, /* format_hint */ ""))
{
}

// Copyable and movable.
PortableImage::PortableImage(const PortableImage&) = default;
PortableImage::PortableImage(PortableImage&&) = default;
PortableImage& PortableImage::operator=(const PortableImage&) = default;
PortableImage& PortableImage::operator=(PortableImage&&) = default;

PortableImage::~PortableImage() = default;

size_t PortableImage::Height() const { return impl_.m_height; }

size_t PortableImage::Width() const { return impl_.m_width; }

void PortableImage::WriteCHW(Span<float> buffer) const
{
  VerifyIsTrue(buffer.Size() == Size(), TuriErrorCode::InvalidBufferLength);

  // Copy the image, resulting in each element having a channel value in [0.f,
  // 255.f].
  image_util::copy_image_to_memory(
      /* input */ impl_, /* outptr */ buffer.Data(),
      /* outstrides */ {Height() * Width(), Width(), 1},
      /* outshape */ {3, Height(), Width()}, /* channel_last */ false);

  // Normalize.
  std::transform(buffer.begin(), buffer.end(), buffer.begin(),
                 [](float raw_value) -> float { return raw_value / 255.0f; });
}

void PortableImage::WriteHWC(Span<float> buffer) const
{
  VerifyIsTrue(buffer.Size() == Size(), TuriErrorCode::InvalidBufferLength);

  // Copy the image, resulting in each element having a channel value in [0.f,
  // 255.f].
  image_util::copy_image_to_memory(
      /* input */ impl_, /* outptr */ buffer.Data(),
      /* outstrides */ {Width() * 3, 3, 1},
      /* outshape */ {Height(), Width(), 3}, /* channel_last */ true);

  // Normalize.
  std::transform(buffer.begin(), buffer.end(), buffer.begin(),
                 [](float raw_value) -> float { return raw_value / 255.0f; });
}

}  // namespace neural_net
}  // namespace turi

/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE PortableImageTests

#include "ml/neural_net/PortableImage.hpp"

#include <tuple>

#include <boost/test/unit_test.hpp>
#include <core/data/image/image_type.hpp>
#include <core/util/test_macros.hpp>
#include <model_server/lib/image_util.hpp>

using turi::MakeSpan;
using turi::Span;
using turi::neural_net::PortableImage;

namespace turi {
namespace neural_net {
namespace {

using rgb_pixel_type = std::tuple<uint8_t, uint8_t, uint8_t>;

constexpr float kEpsilon = 0.001f;

image_type CreateImage(size_t height, size_t width,
                       std::function<rgb_pixel_type(size_t x, size_t y)> rgb_generator)
{
  size_t size = height * width * 3;
  std::unique_ptr<uint8_t[]> buffer(new uint8_t[size]);
  for (size_t j = 0; j < height; ++j) {
    for (size_t i = 0; i < width; ++i) {
      uint8_t* pixel = buffer.get() + j * width * 3 + i * 3;
      std::tie(pixel[0], pixel[1], pixel[2]) = rgb_generator(i, j);
    }
  }

  flex_image raw_image(reinterpret_cast<char*>(buffer.get()), height, width, 3, size,
                       IMAGE_TYPE_CURRENT_VERSION, static_cast<int>(Format::RAW_ARRAY));
  return image_util::encode_image(raw_image);
}

BOOST_AUTO_TEST_CASE(TestHeightAndWidth)
{
  // Create an image with arbitrary size, leaving the pixel values default initialized.
  constexpr size_t kTestHeight = 31;
  constexpr size_t kTestWidth = 37;
  image_type impl = CreateImage(kTestHeight, kTestWidth,
                                [](size_t x, size_t y) { return rgb_pixel_type(0, 0, 0); });
  PortableImage image(impl);
  TS_ASSERT_EQUALS(image.Height(), kTestHeight);
  TS_ASSERT_EQUALS(image.Width(), kTestWidth);
}

BOOST_AUTO_TEST_CASE(TestWriteWithIncorrectSpanSize)
{
  // Create an image with arbitrary size, leaving the pixel values default initialized.
  constexpr size_t kTestHeight = 31;
  constexpr size_t kTestWidth = 37;
  image_type impl = CreateImage(kTestHeight, kTestWidth,
                                [](size_t x, size_t y) { return rgb_pixel_type(0, 0, 0); });
  PortableImage image(impl);

  std::vector<float> buffer(image.Size() - 1);
  TS_ASSERT_THROWS_ANYTHING(image.WriteCHW(MakeSpan(buffer)));
}

BOOST_AUTO_TEST_CASE(TestWriteCHW)
{
  // Create an image encoding the row number in the red channel, the column number in the green
  // channel, and 7 in the blue channel.
  auto pixel_generator = [](size_t x, size_t y) {
    return rgb_pixel_type(static_cast<uint8_t>(y), static_cast<uint8_t>(x), 7);
  };
  image_type impl = CreateImage(256, 256, pixel_generator);
  PortableImage image(impl);

  // Write the image to a buffer.
  std::vector<float> buffer(image.Size());
  image.WriteCHW(MakeSpan(buffer));

  // Compute strides for CHW ordering.
  size_t channel_stride = 256 * 256;
  size_t row_stride = 256;
  size_t column_stride = 1;

  // The first channel encodes the row number.
  for (size_t y = 0; y < 256; ++y) {
    for (size_t x = 0; x < 256; ++x) {
      float value = buffer[0 * channel_stride + y * row_stride + x * column_stride];
      TS_ASSERT_DELTA(value, static_cast<float>(y) / 255.f, kEpsilon);
    }
  }

  // The second channel encodes the column number.
  for (size_t y = 0; y < 256; ++y) {
    for (size_t x = 0; x < 256; ++x) {
      float value = buffer[1 * channel_stride + y * row_stride + x * column_stride];
      TS_ASSERT_DELTA(value, static_cast<float>(x) / 255.f, kEpsilon);
    }
  }

  // The third channel is just 7/255.
  for (size_t y = 0; y < 256; ++y) {
    for (size_t x = 0; x < 256; ++x) {
      float value = buffer[2 * channel_stride + y * row_stride + x * column_stride];
      TS_ASSERT_DELTA(value, 7 / 255.f, kEpsilon);
    }
  }
}

BOOST_AUTO_TEST_CASE(TestWriteHWC)
{
  // Create an image encoding the row number in the red channel, the column number in the green
  // channel, and 7 in the blue channel.
  auto pixel_generator = [](size_t x, size_t y) {
    return rgb_pixel_type(static_cast<uint8_t>(y), static_cast<uint8_t>(x), 7);
  };
  image_type impl = CreateImage(256, 256, pixel_generator);
  PortableImage image(impl);

  // Write the image to a buffer.
  std::vector<float> buffer(image.Size());
  image.WriteHWC(MakeSpan(buffer));

  // Compute strides for HWC ordering.
  size_t row_stride = 256 * 3;
  size_t column_stride = 3;
  size_t channel_stride = 1;

  // The first channel encodes the row number.
  for (size_t y = 0; y < 256; ++y) {
    for (size_t x = 0; x < 256; ++x) {
      float value = buffer[0 * channel_stride + y * row_stride + x * column_stride];
      TS_ASSERT_DELTA(value, static_cast<float>(y) / 255.f, kEpsilon);
    }
  }

  // The second channel encodes the column number.
  for (size_t y = 0; y < 256; ++y) {
    for (size_t x = 0; x < 256; ++x) {
      float value = buffer[1 * channel_stride + y * row_stride + x * column_stride];
      TS_ASSERT_DELTA(value, static_cast<float>(x) / 255.f, kEpsilon);
    }
  }

  // The third channel is just 7/255.
  for (size_t y = 0; y < 256; ++y) {
    for (size_t x = 0; x < 256; ++x) {
      float value = buffer[2 * channel_stride + y * row_stride + x * column_stride];
      TS_ASSERT_DELTA(value, 7 / 255.f, kEpsilon);
    }
  }
}

}  // namespace
}  // namespace neural_net
}  // namespace turi

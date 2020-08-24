/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/CoreImageImage.hpp>

#include <core/util/Verify.hpp>

#import <Accelerate/Accelerate.h>

namespace turi {
namespace neural_net {

namespace {

CIContext *GetCIContext()
{
  NSDictionary *contextOptions = @{
    // Assume that most image operations are part of already parallelized batch operations.
    kCIContextUseSoftwareRenderer : @true,

    // Disable any color management.
    kCIContextWorkingColorSpace : [NSNull null],
  };
  return [CIContext contextWithOptions:contextOptions];
}

// Render the image into a bitmap. CoreImage supports RGBA but not RGB...
void RenderRgba(CIImage *image, Span<float> buffer)
{
  const size_t height = static_cast<size_t>(image.extent.size.height);
  const size_t width = static_cast<size_t>(image.extent.size.width);
  VerifyIsTrue(buffer.Size() == height * width * 4, TuriErrorCode::InvalidBufferLength);

  CIContext *context = GetCIContext();
  [context render:image
         toBitmap:buffer.Data()
         rowBytes:width * 4 * sizeof(float)
           bounds:image.extent
           format:kCIFormatRGBAf
       colorSpace:nil];
}

vImage_Buffer WrapSpan(Span<float> span, size_t height, size_t width, size_t num_channels)
{
  VerifyIsTrue(span.Size() == height * width * num_channels, TuriErrorCode::InvalidBufferLength);

  vImage_Buffer buffer;
  buffer.data = span.Data();
  buffer.width = width;
  buffer.height = height;
  buffer.rowBytes = width * num_channels * sizeof(float);
  return buffer;
};

}  // namespace

CoreImageImage::CoreImageImage(CIImage *impl)
  : impl_(impl)
{
}

CoreImageImage::CoreImageImage(const std::string &path)
{
  @autoreleasepool {
    NSURL *url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
    impl_ = [CIImage imageWithContentsOfURL:url];
  }
}

CoreImageImage::CoreImageImage(const CoreImageImage &) = default;
CoreImageImage::CoreImageImage(CoreImageImage &&) = default;
CoreImageImage &CoreImageImage::operator=(const CoreImageImage &) = default;
CoreImageImage &CoreImageImage::operator=(CoreImageImage &&) = default;
CoreImageImage::~CoreImageImage() = default;

size_t CoreImageImage::Height() const
{
  @autoreleasepool {
    return static_cast<size_t>(impl_.extent.size.height);
  }
}

size_t CoreImageImage::Width() const
{
  @autoreleasepool {
    return static_cast<size_t>(impl_.extent.size.width);
  }
}

void CoreImageImage::WriteCHW(Span<float> output) const
{
  @autoreleasepool {
    // Verify the output buffer is big enough. (Avoid the cost of additional autorelease pools that
    // e.g. GetHeight() would incur.)
    const size_t height = static_cast<size_t>(impl_.extent.size.height);
    const size_t width = static_cast<size_t>(impl_.extent.size.width);
    VerifyIsTrue(output.Size() == 3 * height * width, TuriErrorCode::InvalidBufferLength);

    // Render to RGBAf, the only relevant floating-point format CoreImage supports.
    size_t rgba_size = height * width * 4;
    std::vector<float> rgba(rgba_size);
    RenderRgba(impl_, MakeSpan(rgba));

    // Allocate a buffer for the alpha channel that Accelerate will write, but that we will discard.
    size_t alpha_size = height * width;
    std::vector<float> alpha(alpha_size);

    // Convert spans to Accelerate image buffers.
    vImage_Buffer rgba_buffer = WrapSpan(MakeSpan(rgba), height, width, 4);
    std::array<vImage_Buffer, 4> channel_buffers;
    auto it = channel_buffers.begin();
    for (Span<float> channel_span : output.IterateByDimension(3)) {
      *it++ = WrapSpan(channel_span, height, width, 1);
    }
    *it++ = WrapSpan(MakeSpan(alpha), height, width, 1);
    VerifyDebugIsTrueWithMessage(it == channel_buffers.end(), TuriErrorCode::LogicError,
                                 "must initialize all channel buffers");

    // Invoke Accelerate.
    vImage_Error error =
        vImageConvert_RGBAFFFFtoPlanarF(&rgba_buffer, &channel_buffers[0], &channel_buffers[1],
                                        &channel_buffers[2], &channel_buffers[3], kvImageDoNotTile);
    VerifyIsTrueWithMessage(error == kvImageNoError, TuriErrorCode::ImageConversionFailure,
                            "unexpected error converting RGBA bitmap to planar RGB");
  }  // @autoreleasepool
}

void CoreImageImage::WriteHWC(Span<float> output) const
{
  @autoreleasepool {
    // Verify the output buffer is big enough. (Avoid the cost of additional autorelease pools that
    // e.g. GetHeight() would incur.)
    const size_t height = static_cast<size_t>(impl_.extent.size.height);
    const size_t width = static_cast<size_t>(impl_.extent.size.width);
    VerifyIsTrue(output.Size() == 3 * height * width, TuriErrorCode::InvalidBufferLength);

    // Render to RGBAf, the only relevant floating-point format CoreImage supports.
    size_t rgba_size = height * width * 4;
    std::vector<float> rgba(rgba_size);
    RenderRgba(impl_, MakeSpan(rgba));
    vImage_Buffer rgba_buffer = WrapSpan(MakeSpan(rgba), height, width, 4);

    // Copy the RGB portion to the output buffer using Accelerate.
    vImage_Buffer out_buffer = WrapSpan(output, height, width, 3);
    vImage_Error error =
        vImageConvert_RGBAFFFFtoRGBFFF(&rgba_buffer, &out_buffer, kvImageDoNotTile);
    VerifyIsTrueWithMessage(error == kvImageNoError, TuriErrorCode::ImageConversionFailure,
                            "converting RGBA bitmap to RGB");
  }  // @autoreleasepool
}

}  // namespace neural_net
}  // namespace turi

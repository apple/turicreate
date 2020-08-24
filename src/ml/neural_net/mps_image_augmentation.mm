/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/mps_image_augmentation.hpp>

#include <random>

#include <core/util/Verify.hpp>
#include <ml/neural_net/CoreImageImage.hpp>
#include <ml/neural_net/TaskQueue.hpp>

#import <Accelerate/Accelerate.h>

namespace turi {
namespace neural_net {

namespace {

/**
 * Returns a transform that converts a normalized rect with origin at top-left
 * to a rect in Core Image coordinates (with origin at bottom left).
 */
CGAffineTransform transform_to_core_image(CGSize size) {
  return CGAffineTransformConcat(
      CGAffineTransformMakeScale(size.width, -size.height),
      CGAffineTransformMakeTranslation(0, size.height));
}

/**
 * Returns the inverse of the transform returned by transform_to_core_image.
 */
CGAffineTransform transform_from_core_image(CGSize size) {
  return CGAffineTransformInvert(transform_to_core_image(size));
}

CIImage *convert_to_core_image(const Image &source)
{
  // On platforms where we use the image_augmenter implementation defined in this file, we expect
  // the Image to actually be a CoreImageImage anyway. Try downcasting and getting the underlying
  // CIImage.
  auto *core_image_source = dynamic_cast<const CoreImageImage *>(&source);
  if (core_image_source) {
    return core_image_source->AsCIImage();
  }

  // Otherwise, fall back to rendering the image into a bitmap and loading that into CoreImage.

  // Render to a bitmap.
  NSMutableData *rgbBitmap = [NSMutableData dataWithLength:source.Size() * sizeof(float)];
  Span<float> image_span(reinterpret_cast<float *>(rgbBitmap.mutableBytes), source.Size());
  source.WriteHWC(image_span);

  // Convert from RGB to RGBA.
  auto wrapBitmap = [&source](NSMutableData *bitmap) {
    vImage_Buffer buffer;
    buffer.data = bitmap.mutableBytes;
    buffer.width = source.Width();
    buffer.height = source.Height();
    buffer.rowBytes = bitmap.length / source.Height();
    return buffer;
  };
  NSMutableData *rgbaBitmap =
      [NSMutableData dataWithLength:source.Height() * source.Width() * 4 * sizeof(float)];
  vImage_Buffer rgbaBuffer = wrapBitmap(rgbaBitmap);
  vImage_Buffer rgbBuffer = wrapBitmap(rgbBitmap);
  vImage_Error error =
      vImageConvert_RGBFFFtoRGBAFFFF(&rgbBuffer, /* alpha buffer */ NULL, /* alpha value */ 1.0f,
                                     &rgbaBuffer, /* premultiply */ true, kvImageDoNotTile);
  VerifyIsTrueWithMessage(error == kvImageNoError, TuriErrorCode::ImageConversionFailure,
                          "converting RGB bitmap to RGBA");

  // Pass the bitmap to CoreImage.
  return [CIImage imageWithBitmapData:rgbaBitmap
                          bytesPerRow:rgbaBitmap.length / source.Height()
                                 size:CGSizeMake(source.Width(), source.Height())
                               format:kCIFormatRGBAf
                           colorSpace:nil];
}

API_AVAILABLE(macos(10.13))
NSArray<TCMPSImageAnnotation *> *convert_to_core_image(
    const std::vector<image_annotation>& source, CGSize size) {

  NSMutableArray<TCMPSImageAnnotation *> *result =
      [NSMutableArray arrayWithCapacity:source.size()];
  for (const image_annotation& src_annotation : source) {

    TCMPSImageAnnotation *resultAnnotation = [TCMPSImageAnnotation new];

    resultAnnotation.identifier = src_annotation.identifier;
    resultAnnotation.confidence = src_annotation.confidence;

    // Convert to CoreImage coordinates.
    CGRect sourceBoundingBox = CGRectMake(
        src_annotation.bounding_box.x, src_annotation.bounding_box.y,
        src_annotation.bounding_box.width, src_annotation.bounding_box.height);
    resultAnnotation.boundingBox = CGRectApplyAffineTransform(
        sourceBoundingBox, transform_to_core_image(size));

    [result addObject:resultAnnotation];
  }

  return result;
}

API_AVAILABLE(macos(10.13))
TCMPSLabeledImage *convert_to_core_image(const labeled_image& source) {

  TCMPSLabeledImage *result = [TCMPSLabeledImage new];

  CIImage *sourceImage = convert_to_core_image(*source.image);

  // For intermediate values, use an image with infinite extent, for smoother
  // sampling behavior at the intended image boundaries.
  result.image = [sourceImage imageByClampingToExtent];

  // The (integral) extent of the original CIImage is the original image size.
  result.bounds = sourceImage.extent;

  result.annotations = convert_to_core_image(source.annotations,
                                             result.bounds.size);

  return result;
}

void convert_from_core_image(CIImage *source, CIContext *context, size_t output_width,
                             size_t output_height, Span<float> output)
{
  VerifyIsTrue(output_width * output_height * 3 == output.Size(),
               TuriErrorCode::InvalidBufferLength);

  // Render the image into a bitmap. CoreImage supports RGBA but not RGB...
  size_t rgba_data_size = output_height * output_width * 4;
  std::unique_ptr<float[]> rgba_data(new float[rgba_data_size]);
  [context render: source
         toBitmap: rgba_data.get()
         rowBytes: output_width * 4 * sizeof(float)
           bounds: CGRectMake(0.f, 0.f, output_width, output_height)
           format: kCIFormatRGBAf
       colorSpace: nil];

  // Copy the RGB portion to the output buffer using Accelerate.
  auto wrap_float_data = [=](Span<float> span) {
    vImage_Buffer buffer;
    buffer.data = span.Data();
    buffer.width = output_width;
    buffer.height = output_height;
    buffer.rowBytes = span.Size() * sizeof(float) / output_height;
    return buffer;
  };
  vImage_Buffer rgba_buffer = wrap_float_data(Span<float>(rgba_data.get(), rgba_data_size));
  vImage_Buffer out_buffer = wrap_float_data(output);
  vImage_Error err = vImageConvert_RGBAFFFFtoRGBFFF(
      &rgba_buffer, &out_buffer, kvImageDoNotTile);
  VerifyIsTrueWithMessage(err == kvImageNoError, TuriErrorCode::ImageConversionFailure,
                          "converting RGBA bitmap to RGB");
}

API_AVAILABLE(macos(10.13))
std::vector<image_annotation> convert_from_core_image(
    NSArray<TCMPSImageAnnotation *> *source, CGSize size) {

  std::vector<image_annotation> result;
  result.reserve(source.count);
  for (TCMPSImageAnnotation *sourceAnnotation : source) {

    image_annotation dest_annotation;
    dest_annotation.identifier = static_cast<int>(sourceAnnotation.identifier);
    dest_annotation.confidence = sourceAnnotation.confidence;

    // The input annotation is normalized with origin at top-left.
    // Convert from CoreImage coordinates.
    CGRect boundingBox = CGRectApplyAffineTransform(
        sourceAnnotation.boundingBox, transform_from_core_image(size));
    dest_annotation.bounding_box.x = boundingBox.origin.x;
    dest_annotation.bounding_box.y = boundingBox.origin.y;
    dest_annotation.bounding_box.width = boundingBox.size.width;
    dest_annotation.bounding_box.height = boundingBox.size.height;

    result.push_back(dest_annotation);
  }
  return result;
}

API_AVAILABLE(macos(10.13))
void convert_from_core_image(TCMPSLabeledImage *source, CIContext *context, size_t output_width,
                             size_t output_height, Span<float> output,
                             std::vector<image_annotation> *annotations_out)
{
  // Render the CIImage into the provided float buffer.
  convert_from_core_image(source.image, context, output_width, output_height, output);

  // Convert the annotations.
  *annotations_out = convert_from_core_image(
      source.annotations, CGSizeMake(output_width, output_height));
}

NSArray<TCMPSUniformRandomNumberGenerator> *create_rng_batch(
    size_t size, int random_seed)
{
  NSMutableArray<TCMPSUniformRandomNumberGenerator> *result =
      [[NSMutableArray alloc] initWithCapacity:(NSUInteger)size];

  // Create `size` random number generators
  for (size_t i = 0; i < size; ++i) {
    // Seed each rng with a combination of the user's seed and the index of the
    // rng in the batch.
    std::seed_seq seed_seq = {random_seed, static_cast<int>(i)};
    __block std::mt19937 engine(seed_seq);

    // Wrap the random engine in a block yielding uniformly distributed values.
    TCMPSUniformRandomNumberGenerator uniform =
        ^(CGFloat lower, CGFloat upper) {
      std::uniform_real_distribution<CGFloat> dist(lower, upper);
      return dist(engine);
    };
    [result addObject:uniform];
  }
  return result;
}

NSArray<TCMPSUniformRandomNumberGenerator> *create_rng_batch(
    size_t size,
    std::function<float(float lower_bound, float upper_bound)> rng)
{
  // Create an Objective-C wrapper around the provided function yielding
  // uniformly distributed values.
  TCMPSUniformRandomNumberGenerator uniform = ^(CGFloat lower, CGFloat upper) {
    return static_cast<CGFloat>(rng(lower, upper));
  };

  // Return an array with `size` copies of the block.
  NSMutableArray<TCMPSUniformRandomNumberGenerator> *result =
      [[NSMutableArray alloc] initWithCapacity:(NSUInteger)size];
  for (size_t i = 0; i < size; ++i) {
    [result addObject:uniform];
  }
  return result;
}

}  // namespace

mps_image_augmenter::mps_image_augmenter(const options& opts)
    : mps_image_augmenter(opts, create_rng_batch(opts.batch_size,
                                                 opts.random_seed))
{}

mps_image_augmenter::mps_image_augmenter(
    const options& opts,
    std::function<float(float lower_bound, float upper_bound)> rng)
    : mps_image_augmenter(opts, create_rng_batch(opts.batch_size, rng))
{}

mps_image_augmenter::mps_image_augmenter(
    const options& opts, NSArray<TCMPSUniformRandomNumberGenerator> *rng_batch)
    : opts_(opts), rng_batch_(rng_batch)
{

  @autoreleasepool {

  NSDictionary *contextOptions = @{

    // Assume for now that we can keep the GPU busy with the actual NN training.
    kCIContextUseSoftwareRenderer : @true,

    // Disable any color management.
    kCIContextWorkingColorSpace   : [NSNull null],
  };
  context_ = [CIContext contextWithOptions:contextOptions];

  NSMutableArray<id <TCMPSImageAugmenting>> *augmentations =
      [[NSMutableArray alloc] init];

  if (opts.crop_prob > 0.f) {

    // Enable random crops.
    TCMPSRandomCropAugmenter *cropAug = [[TCMPSRandomCropAugmenter alloc] init];
    cropAug.skipProbability = 1.f - opts.crop_prob;
    cropAug.minAspectRatio = opts.crop_opts.min_aspect_ratio;
    cropAug.maxAspectRatio = opts.crop_opts.max_aspect_ratio;
    cropAug.minAreaFraction = opts.crop_opts.min_area_fraction;
    cropAug.maxAreaFraction = opts.crop_opts.max_area_fraction;
    cropAug.minObjectCovered = opts.crop_opts.min_object_covered;
    cropAug.maxAttempts = opts.crop_opts.max_attempts;
    cropAug.minEjectCoverage = opts.crop_opts.min_eject_coverage;

    [augmentations addObject:cropAug];
  }

  if (opts.pad_prob > 0.f) {

    // Enable random padding.
    TCMPSRandomPadAugmenter *padAug = [[TCMPSRandomPadAugmenter alloc] init];
    padAug.skipProbability = 1.f - opts.pad_prob;
    padAug.minAspectRatio = opts.pad_opts.min_aspect_ratio;
    padAug.maxAspectRatio = opts.pad_opts.max_aspect_ratio;
    padAug.minAreaFraction = opts.pad_opts.min_area_fraction;
    padAug.maxAreaFraction = opts.pad_opts.max_area_fraction;
    padAug.maxAttempts = opts.pad_opts.max_attempts;

    [augmentations addObject:padAug];
  }

  if (opts.horizontal_flip_prob > 0.f) {

    // Enable mirror images.
    TCMPSHorizontalFlipAugmenter *horizontalFlipAug =
        [[TCMPSHorizontalFlipAugmenter alloc] init];
    horizontalFlipAug.skipProbability = 1.f - opts.horizontal_flip_prob;
    [augmentations addObject:horizontalFlipAug];
  }

  if (opts.brightness_max_jitter > 0.f ||
      opts.contrast_max_jitter > 0.f ||
      opts.saturation_max_jitter > 0.f) {

    // Enable color controls.
    TCMPSColorControlAugmenter *colorAug =
        [[TCMPSColorControlAugmenter alloc] init];
    colorAug.maxBrightnessDelta = opts.brightness_max_jitter;
    colorAug.maxContrastProportion = opts.contrast_max_jitter;
    colorAug.maxSaturationProportion = opts.saturation_max_jitter;
    [augmentations addObject:colorAug];
  }

  if (opts.hue_max_jitter > 0.f) {

    // Enable color rotation.
    TCMPSHueAdjustAugmenter *hueAug = [[TCMPSHueAdjustAugmenter alloc] init];
    hueAug.maxHueAdjust = opts.hue_max_jitter;
    [augmentations addObject:hueAug];
  }

  // Always resize to the fixed image size expected by the neural network.
  CGSize outputSize;
  outputSize.width = static_cast<CGFloat>(opts_.output_width);
  outputSize.height = static_cast<CGFloat>(opts_.output_height);
  [augmentations addObject:[[TCMPSResizeAugmenter alloc] initWithSize:outputSize]];

  augmentations_ = augmentations;

  }
}

mps_image_augmenter::result mps_image_augmenter::prepare_images(
    std::vector<labeled_image> source_batch) {

  @autoreleasepool {

  const size_t n = opts_.batch_size;
  const size_t h = opts_.output_height;
  const size_t w = opts_.output_width;
  constexpr size_t c = 3;

  // Discard any source data in excess of the batch size.
  if (source_batch.size() > n) {
    source_batch.resize(n);
  }

  result res;
  res.annotations_batch.resize(source_batch.size());

  // Allocate a float vector large enough to contain the entire image batch.
  const size_t result_array_stride = h * w * c;
  std::vector<float> result_array(n * result_array_stride);
  Span<float> result_span = MakeSpan(result_array);

  // Define the work to perform for each image.
  auto apply_augmentations_for_image = [&](size_t i) {
    @autoreleasepool {
    const labeled_image& source = source_batch[i];

    // Convert from turi::image_type to CIImage.
    TCMPSLabeledImage *labeledImage = convert_to_core_image(source);

    // Apply all augmentations.
    // For image i, use the random number generator at `rng_batch_[i]`, to
    // ensure that no two threads executing this lambda are using the same
    // random engine.
    for (id <TCMPSImageAugmenting> aug : augmentations_) {
      labeledImage = [aug imageAugmentedFromImage:labeledImage
                                        generator:rng_batch_[i]];
    }

    // Convert from CIImage to float array.
    convert_from_core_image(labeledImage, context_, w, h, result_span.SliceByDimension(n, i),
                            &res.annotations_batch[i]);
    }
  };

  // Distribute work across threads, each writing into different regions of
  // result_array and res.annotations_batch.
  TaskQueue::GetGlobalConcurrentQueue()->DispatchApply(source_batch.size(),
                                                       apply_augmentations_for_image);

  // Wrap and return the results.
  res.image_batch = shared_float_array::wrap(std::move(result_array),
                                             { n, h, w, c});
  return res;

  }
}

}  // neural_net
}  // turi

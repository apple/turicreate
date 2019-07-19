/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <ml/neural_net/TCMPSImageAugmenting.h>

#include <math.h>

@implementation TCMPSImageAnnotation

- (instancetype)copyWithZone:(NSZone *)zone {
  TCMPSImageAnnotation *result = [TCMPSImageAnnotation new];
  if (result) {
    result.identifier = self.identifier;
    result.boundingBox = self.boundingBox;
    result.confidence = self.confidence;
  }
  return result;
}

@end

@implementation TCMPSLabeledImage
@end

@implementation TCMPSResizeAugmenter

- (instancetype)initWithSize:(CGSize)size {

  self = [super init];
  if (self) {
    _size = size;
  }
  return self;
}

- (TCMPSLabeledImage *)imageAugmentedFromImage:(TCMPSLabeledImage *)source
                                     generator:(TCMPSUniformRandomNumberGenerator)uniform
{
  TCMPSLabeledImage *result = [TCMPSLabeledImage new];

  // Determine the affine transform to apply.
  CGRect bounds = source.bounds;
  CGFloat sx = self.size.width / bounds.size.width;
  CGFloat sy = self.size.height / bounds.size.height;
  CGAffineTransform transform = CGAffineTransformConcat(
      CGAffineTransformMakeTranslation(-bounds.origin.x, -bounds.origin.y),
      CGAffineTransformMakeScale(sx, sy));


  // Apply to the image itself.
  // TODO: Investigate higher-quality downsampling? The original Python toolkit
  // used MXNet's area-based interpolation.
  result.image = [source.image imageByApplyingTransform:transform];
  result.image = [result.image imageBySamplingLinear];

  // Track the changes to the image bounds.
  result.bounds = CGRectApplyAffineTransform(source.bounds, transform);

  // Apply to all the annotations.
  NSMutableArray<TCMPSImageAnnotation *> *transformedAnnotations =
      [NSMutableArray arrayWithCapacity:source.annotations.count];
  for (TCMPSImageAnnotation *annotation in source.annotations) {

    // Just copy the original annotation, but apply the affine transform to the
    // bounding box.
    TCMPSImageAnnotation *transformed = [annotation copy];
    transformed.boundingBox =
        CGRectApplyAffineTransform(annotation.boundingBox, transform);
    [transformedAnnotations addObject:transformed];
  }
  result.annotations = transformedAnnotations;

  return result;
}

@end

@implementation TCMPSHorizontalFlipAugmenter

- (instancetype)init
{
  self = [super init];
  if (self) {
    _skipProbability = 0.5f;
  }
  return self;
}

- (TCMPSLabeledImage *)imageAugmentedFromImage:(TCMPSLabeledImage *)source
                                     generator:(TCMPSUniformRandomNumberGenerator)uniform
{
  if (uniform(0.f, 1.f) < self.skipProbability) {
    return source;
  }

  // Flip along the x-axis. The end of the pipeline will worry about resetting
  // the origin.
  CGAffineTransform flipTransform = CGAffineTransformMakeScale(-1.f, 1.f);
  TCMPSLabeledImage *result = [TCMPSLabeledImage new];

  // Apply the transform to the image.
  result.image = [source.image imageByApplyingTransform:flipTransform];
  result.bounds = CGRectApplyAffineTransform(source.bounds, flipTransform);

  // Apply the transform to all annotations.
  NSMutableArray<TCMPSImageAnnotation *> *annotations =
      [NSMutableArray arrayWithCapacity: source.annotations.count];
  for (TCMPSImageAnnotation *sourceAnnotation in source.annotations) {
    TCMPSImageAnnotation *annotation = [sourceAnnotation copy];
    annotation.boundingBox =
        CGRectApplyAffineTransform(sourceAnnotation.boundingBox, flipTransform);
    [annotations addObject:annotation];
  }
  result.annotations = annotations;

  return result;
}

@end

@implementation TCMPSRandomCropAugmenter

- (instancetype)init
{
  self = [super init];
  if (self) {
    _skipProbability = 0.1f;
    _minAspectRatio = 0.8f;
    _maxAspectRatio = 1.25f;
    _minAreaFraction = 0.15f;
    _maxAreaFraction = 1.f;
    _minObjectCovered = 0.f;
    _maxAttempts = 50;
    _minEjectCoverage = 0.5f;
  }
  return self;
}

- (TCMPSLabeledImage *)imageAugmentedFromImage:(TCMPSLabeledImage *)source
                                     generator:(TCMPSUniformRandomNumberGenerator)uniform
{
  if (uniform(0.f, 1.f) < self.skipProbability) {
    return source;
  }

  CGFloat imageHeight = CGRectGetHeight(source.bounds);
  CGFloat imageWidth = CGRectGetWidth(source.bounds);

  // Sample crop rects until one satisfies our constraints (by yielding a valid
  // list of cropped annotations), or reaching the limit on attempts.
  for (NSUInteger i = 0; i < self.maxAttempts; ++i) {

    // Randomly sample an aspect ratio.
    CGFloat aspectRatio = uniform(self.minAspectRatio, self.maxAspectRatio);

    // Next we'll sample a height (which combined with the now known aspect
    // ratio, determines the size and area). But first we must compute the range
    // of valid heights. The crop cannot be taller than the original image,
    // cannot be wider than the original image, and must have an area in the
    // specified range.

    // The cropped height must be no larger the original height.
    // h' <= h
    CGFloat maxHeight = imageHeight;

    // The cropped width must be no larger than the original width.
    // w' <= w IMPLIES ah' <= w IMPLIES h' <= w / a
    CGFloat maxHeightFromWidth = imageWidth / aspectRatio;
    if (maxHeight > maxHeightFromWidth) {
      maxHeight = maxHeightFromWidth;
    }

    // The cropped area must not exceed the maximum area fraction.
    // w'h' <= fhw IMPLIES ah'h' <= fhw IMPLIES h' <= sqrt(fhw/a)
    CGFloat maxHeightFromArea =
        sqrtf(imageHeight * imageWidth * self.maxAreaFraction / aspectRatio);
    if (maxHeight > maxHeightFromArea) {
      maxHeight = maxHeightFromArea;
    }

    // The padded area must attain the minimum area fraction.
    CGFloat minHeight =
        sqrtf(imageHeight * imageWidth * self.minAreaFraction / aspectRatio);

    // If the range is empty, then crops with the sampled aspect ratio cannot
    // satisfy the area constraint.
    if (minHeight > maxHeight) {
      continue;
    }

    CGRect cropRect;
    cropRect.size.height = uniform(minHeight, maxHeight);
    cropRect.size.width = cropRect.size.height * aspectRatio;

    // Sample a position for the crop, constrained to lie within the image.
    cropRect.origin.x = uniform(0.f, imageWidth - cropRect.size.width);
    cropRect.origin.y = uniform(0.f, imageHeight - cropRect.size.height);

    // Compensate for the image bound's origin.
    cropRect.origin.x += source.bounds.origin.x;
    cropRect.origin.y += source.bounds.origin.y;

    // Attempt to apply the crop to the annotations. This will return nil if
    // self.minObjectCovered is not satisfied for some annotation.
    NSArray<TCMPSImageAnnotation *> *croppedAnnotations =
        [self applyCrop:cropRect toAnnotations:source.annotations];

    if (croppedAnnotations) {

      // Success! Apply the crop via the bounds property of TCMPSLabeledImage.
      // There's no need to touch the "infinite canvas" stored in the CIImage.
      TCMPSLabeledImage *result = [TCMPSLabeledImage new];
      result.image = source.image;
      result.bounds = cropRect;
      result.annotations = croppedAnnotations;

      return result;
    }
  }

  // If we could not sample a valid crop rect, just return the original data.
  return source;
}

// If the crop does not preserve at least self.minObjectCovered of every
// annotation's area, returns nil. Otherwise, returns the cropped annotations,
// filtering out those that do not overlap the crop with at least
// self.minEjectCoverage of their original area.
- (nullable NSArray<TCMPSImageAnnotation *> *)applyCrop:(CGRect)cropRect
                                          toAnnotations:(NSArray<TCMPSImageAnnotation *> *)annotations {

  NSMutableArray<TCMPSImageAnnotation *> *result =
      [NSMutableArray arrayWithCapacity:annotations.count];

  for (TCMPSImageAnnotation *annotation in annotations) {

    // Compute the cropped bounding box.
    CGRect boundingBox = annotation.boundingBox;
    CGRect croppedRect = CGRectIntersection(cropRect, boundingBox);

    // Compute the fraction of the original bounding box left after cropping.
    CGFloat croppedArea =
        CGRectGetHeight(croppedRect) * CGRectGetWidth(croppedRect);
    CGFloat boundingBoxArea =
        CGRectGetHeight(boundingBox) * CGRectGetWidth(boundingBox);
    CGFloat areaFraction = croppedArea / boundingBoxArea;

    // Invalidate the crop if it did not sufficiently overlap each annotation.
    if (areaFraction < self.minObjectCovered) {
      return nil;
    }

    if (areaFraction >= self.minEjectCoverage) {

      // Copy the annotation, substituting the cropped bounding box.
      TCMPSImageAnnotation *cropped = [annotation copy];
      cropped.boundingBox = croppedRect;
      [result addObject:cropped];
    }
  }
  return result;
}

@end

@implementation TCMPSRandomPadAugmenter

- (instancetype)init
{
  self = [super init];
  if (self) {
    _skipProbability = 0.1f;
    _minAspectRatio = 0.8f;
    _maxAspectRatio = 1.25f;
    _minAreaFraction = 1.f;
    _maxAreaFraction = 2.f;
    _maxAttempts = 50;
  }
  return self;
}

- (TCMPSLabeledImage *)imageAugmentedFromImage:(TCMPSLabeledImage *)source
                                     generator:(TCMPSUniformRandomNumberGenerator)uniform
{
  if (uniform(0.f, 1.f) < self.skipProbability) {
    return source;
  }

  CGFloat imageHeight = CGRectGetHeight(source.bounds);
  CGFloat imageWidth = CGRectGetWidth(source.bounds);

  // Randomly sample aspect ratios until one derives a non-empty range of
  // compatible heights, or until reaching the upper limit on attempts.
  CGFloat aspectRatio = 1.f;
  CGFloat minHeight = imageHeight;
  CGFloat maxHeight = imageHeight;
  NSUInteger numAttempts = 0;
  while (minHeight >= maxHeight && numAttempts++ < self.maxAttempts) {

    // Randomly sample an aspect ratio.
    aspectRatio = uniform(self.minAspectRatio, self.maxAspectRatio);

    // The padded height must be at least as large as the original height.
    // h' >= h
    minHeight = imageHeight;

    // The padded width must be at least as large as the original width.
    // w' >= w IMPLIES ah' >= w IMPLIES h' >= w / a
    CGFloat minHeightFromWidth = imageWidth / aspectRatio;
    if (minHeight < minHeightFromWidth) {
      minHeight = minHeightFromWidth;
    }

    // The padded area must attain the minimum area fraction.
    // w'h' >= fhw IMPLIES ah'h' >= fhw IMPLIES h' >= sqrt(fhw/a)
    CGFloat minHeightFromArea =
        sqrtf(self.minAreaFraction * imageHeight * imageWidth / aspectRatio);
    if (minHeight < minHeightFromArea) {
      minHeight = minHeightFromArea;
    }

    // The padded area must not exceed the maximum area fraction.
    maxHeight =
        sqrtf(imageHeight * imageWidth * self.maxAreaFraction / aspectRatio);
  }

  // We did not find a compatible aspect ratio. Just return the original data.
  if (minHeight > maxHeight) {
    return source;
  }

  // Sample a final size, given the sampled aspect ratio and range of heights.
  CGSize paddedSize;
  paddedSize.height = uniform(minHeight, maxHeight);
  paddedSize.width = paddedSize.height * aspectRatio;

  // Sample the offset of the source image inside the padded image.
  CGFloat xOffset = uniform(0.f, paddedSize.width - imageWidth);
  CGFloat yOffset = uniform(0.f, paddedSize.height - imageHeight);

  // Pad the image by compositing the source image over a gray background and
  // cropping to the desired bounds.
  CIColor *grayColor = [CIColor colorWithRed:0.5f green:0.5f blue:0.5f];
  CIImage *grayImage = [CIImage imageWithColor:grayColor];
  CIImage *croppedImage = [source.image imageByCroppingToRect:source.bounds];
  CIImage *compositedImage =
      [croppedImage imageByCompositingOverImage:grayImage];
  CGRect paddedBounds;
  paddedBounds.origin.x = source.bounds.origin.x - xOffset;
  paddedBounds.origin.y = source.bounds.origin.y - yOffset;
  paddedBounds.size = paddedSize;

  // Package up the results. Note that the annotations do not need to be
  // modified, since the coordinate system did not change and padding does not
  // mutate any of the actual annotation bounding boxes!
  TCMPSLabeledImage *result = [TCMPSLabeledImage new];
  result.image = compositedImage;
  result.bounds = paddedBounds;
  result.annotations = source.annotations;

  return result;
}

@end

@implementation TCMPSColorControlAugmenter

- (instancetype)init
{
  self = [super init];
  if (self) {
    _maxBrightnessDelta = 0.05f;
    _maxContrastProportion = 0.05f;
    _maxSaturationProportion = 0.05f;
  }
  return self;
}

- (TCMPSLabeledImage *)imageAugmentedFromImage:(TCMPSLabeledImage *)source
                                     generator:(TCMPSUniformRandomNumberGenerator)uniform
{
  // Sample a random adjustment to brightness.
  CGFloat brightnessDelta = 0.0f;
  if (self.maxBrightnessDelta > 0.f) {
    brightnessDelta += uniform(-self.maxBrightnessDelta,
                               self.maxBrightnessDelta);
  }

  // Sample a random adjustment to contrast.
  CGFloat contrastProportion = 1.0f;
  if (self.maxContrastProportion > 0.f) {
    contrastProportion += uniform(-self.maxContrastProportion,
                                  self.maxContrastProportion);
  }

  // Sample a random adjustment to saturation.
  CGFloat saturationProportion = 1.0f;
  if (self.maxSaturationProportion > 0.f) {
    saturationProportion += uniform(-self.maxSaturationProportion,
                                    self.maxSaturationProportion);
  }

  TCMPSLabeledImage *result = [TCMPSLabeledImage new];

  // Apply the random adjustments to the image.
  NSDictionary *filterParameters = @{
    kCIInputBrightnessKey : @(brightnessDelta),
    kCIInputContrastKey   : @(contrastProportion),
    kCIInputSaturationKey : @(saturationProportion)
  };
  result.image = [source.image imageByApplyingFilter:@"CIColorControls"
                                 withInputParameters:filterParameters];

  // No geometry changes, so just copy the bounds and annotations.
  result.bounds = source.bounds;
  result.annotations = source.annotations;

  return result;
}

@end

@implementation TCMPSHueAdjustAugmenter

- (instancetype)init
{
  self = [super init];
  if (self) {
    _maxHueAdjust = 0.05f;
  }
  return self;
}

- (TCMPSLabeledImage *)imageAugmentedFromImage:(TCMPSLabeledImage *)source
                                     generator:(TCMPSUniformRandomNumberGenerator)uniform
{
  // Sample a random rotation around the color wheel.
  CGFloat hueAdjust = 0.0f;
  if (self.maxHueAdjust > 0.f) {
    hueAdjust += M_PI * uniform(-self.maxHueAdjust, self.maxHueAdjust);
  }

  TCMPSLabeledImage *result = [TCMPSLabeledImage new];

  // Apply the rotation to the hue.
  result.image =
      [source.image imageByApplyingFilter:@"CIHueAdjust"
                      withInputParameters:@{kCIInputAngleKey : @(hueAdjust)}];

  // No geometry changes, so just copy the bounds and annotations.
  result.bounds = source.bounds;
  result.annotations = source.annotations;

  return result;
}

@end

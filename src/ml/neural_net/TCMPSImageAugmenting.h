/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <Foundation/Foundation.h>
#import <CoreImage/CoreImage.h>

NS_ASSUME_NONNULL_BEGIN

/** Facilitates the injection of Turi (C++) random number generators. */
typedef CGFloat (^TCMPSUniformRandomNumberGenerator)(CGFloat lowerBound,
                                                     CGFloat upperBound);

/** Simple Objective C representation of a labeled object inside an image. */
API_AVAILABLE(macos(10.13))
@interface TCMPSImageAnnotation : NSObject <NSCopying>

@property(nonatomic) NSInteger identifier;
@property(nonatomic) CGRect boundingBox;  // In Core Image coordinates
@property(nonatomic) CGFloat confidence;

@end

/** Simple Objective C representation of an annotated image. */
API_AVAILABLE(macos(10.13))
@interface TCMPSLabeledImage : NSObject

/**
 * A possibly augmented image.
 *
 * This image should have infinite extent, to ensure smooth behavior of filters
 * and sampling at the edges of the intended image. The `bounds` property below
 * will track the actual image geometry.
 */
@property(nonatomic) CIImage *image;

/**
 * The portion of the `image` above corresponding to the desired image data.
 *
 * Note that this can have non-integer values, unlike -[CIImage extent], which
 * appears to be a rounded-to-integer view on a CIImage's underlying
 * float-valued true bounds.
 */
@property(nonatomic) CGRect bounds;

/** Image annotations, in Core Image coordinates. */
@property(nonatomic) NSArray<TCMPSImageAnnotation *> *annotations;

@end

/** Protocol defining the shared interfaces across augmenters. */
API_AVAILABLE(macos(10.13))
@protocol TCMPSImageAugmenting <NSObject>

- (TCMPSLabeledImage *)imageAugmentedFromImage:(TCMPSLabeledImage *)source;

@end

/** Augmenter that resizes inputs to a target image size. */
API_AVAILABLE(macos(10.13))
@interface TCMPSResizeAugmenter : NSObject <TCMPSImageAugmenting>

@property(readonly, nonatomic) CGSize size;

- (instancetype)initWithSize:(CGSize)size;

- (TCMPSLabeledImage *)imageAugmentedFromImage:(TCMPSLabeledImage *)source;

@end

/** Augmenter that possibly flips its input across the y-axis. */
API_AVAILABLE(macos(10.13))
@interface TCMPSHorizontalFlipAugmenter : NSObject <TCMPSImageAugmenting>

@property(nonatomic) CGFloat skipProbability;

- (instancetype)initWithRNG:(TCMPSUniformRandomNumberGenerator)rng;

- (TCMPSLabeledImage *)imageAugmentedFromImage:(TCMPSLabeledImage *)source;

@end

/** Augmenter that possibly applies a random crop. */
API_AVAILABLE(macos(10.13))
@interface TCMPSRandomCropAugmenter : NSObject <TCMPSImageAugmenting>

// See image_augmentation.hpp for parameter semantics.

@property(nonatomic) CGFloat skipProbability;
@property(nonatomic) CGFloat minAspectRatio;
@property(nonatomic) CGFloat maxAspectRatio;
@property(nonatomic) CGFloat minAreaFraction;
@property(nonatomic) CGFloat maxAreaFraction;
@property(nonatomic) CGFloat minObjectCovered;
@property(nonatomic) NSUInteger maxAttempts;
@property(nonatomic) CGFloat minEjectCoverage;

- (instancetype)initWithRNG:(TCMPSUniformRandomNumberGenerator)rng;

- (TCMPSLabeledImage *)imageAugmentedFromImage:(TCMPSLabeledImage *)source;

@end

/** Augmenter that possibly applies random padding. */
API_AVAILABLE(macos(10.13))
@interface TCMPSRandomPadAugmenter : NSObject <TCMPSImageAugmenting>

// See image_augmentation.hpp for parameter semantics.

@property(nonatomic) CGFloat skipProbability;
@property(nonatomic) CGFloat minAspectRatio;
@property(nonatomic) CGFloat maxAspectRatio;
@property(nonatomic) CGFloat minAreaFraction;
@property(nonatomic) CGFloat maxAreaFraction;
@property(nonatomic) NSUInteger maxAttempts;

- (instancetype)initWithRNG:(TCMPSUniformRandomNumberGenerator)rng;

- (TCMPSLabeledImage *)imageAugmentedFromImage:(TCMPSLabeledImage *)source;

@end

/** Augmenter that randomly perturbs brightness, contrast, and saturation. */
API_AVAILABLE(macos(10.13))
@interface TCMPSColorControlAugmenter : NSObject <TCMPSImageAugmenting>

// See image_augmentation.hpp for parameter semantics.

@property(nonatomic) CGFloat maxBrightnessDelta;
@property(nonatomic) CGFloat maxContrastProportion;
@property(nonatomic) CGFloat maxSaturationProportion;

- (instancetype)initWithRNG:(TCMPSUniformRandomNumberGenerator)rng;

- (TCMPSLabeledImage *)imageAugmentedFromImage:(TCMPSLabeledImage *)source;

@end

/** Augmenter that randomly rotates the colors in the input image. */
API_AVAILABLE(macos(10.13))
@interface TCMPSHueAdjustAugmenter : NSObject <TCMPSImageAugmenting>

// Multiplied by pi to obtain maximum angular change in radians.
@property(nonatomic) CGFloat maxHueAdjust;

- (instancetype)initWithRNG:(TCMPSUniformRandomNumberGenerator)rng;

- (TCMPSLabeledImage *)imageAugmentedFromImage:(TCMPSLabeledImage *)source;

@end

NS_ASSUME_NONNULL_END

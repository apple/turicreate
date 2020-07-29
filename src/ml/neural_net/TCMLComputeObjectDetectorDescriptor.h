/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#import <MLCompute/MLCompute.h>

NS_ASSUME_NONNULL_BEGIN

// Defines the parameters for the MLCompute-based implementation of the
// Object Detection model.
API_AVAILABLE(macos(10.16))
@interface TCMLComputeObjectDetectorDescriptor : NSObject

// Defines the shape of the graph's input.
@property(nonatomic) MLCTensor *inputTensor;

// Controls the number of features in the output tensor, which should be anchorBoxesCount * (5 +
// classesCount). For each output grid cell, for each anchor box, the output has x, y, h, w, object
// confidence, then the classesCount class likelihoods (conditional on an object being present).
@property(nonatomic) NSUInteger outputChannels;

// Dictionary mapping layer names to weights.
@property(nonatomic) NSDictionary<NSString *, MLCTensor *> *weights;

@end

API_AVAILABLE(macos(10.16))
@interface MLCGraph (TCMLComputeObjectDetector)

+ (instancetype)tc_graphForObjectDetectorDescriptor:
    (TCMLComputeObjectDetectorDescriptor *)descriptor;

@end

NS_ASSUME_NONNULL_END

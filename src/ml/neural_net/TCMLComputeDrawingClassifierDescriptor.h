/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#import <MLCompute/MLCompute.h>

NS_ASSUME_NONNULL_BEGIN

// Defines the parameters for the MLCompute-based implementation of the
// Drawing Classifier model.
API_AVAILABLE(macos(10.16))
@interface TCMLComputeDrawingClassifierDescriptor : NSObject

// Defines the shape of the graph's input.
@property(nonatomic) MLCTensor *inputTensor;

// Defines the shape of the graph's ouput.
@property(nonatomic) MLCTensor *outputTensor;

// Controls the number of features in the output tensor, which should be equal
// to the number of classes.
@property(nonatomic) NSUInteger outputChannels;

// Dictionary mapping layer names to weights.
@property(nonatomic) NSDictionary<NSString *, MLCTensor *> *weights;

@end

API_AVAILABLE(macos(10.16))
@interface MLCGraph (TCMLComputeDrawingClassifier)

+ (instancetype)tc_graphForDrawingClassifierDescriptor:
                    (TCMLComputeDrawingClassifierDescriptor *)descriptor
                                             batchSize:(NSUInteger)batchSize;

@end

NS_ASSUME_NONNULL_END

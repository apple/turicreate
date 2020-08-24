/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#import <Foundation/Foundation.h>
#import <MLCompute/MLCompute.h>

#include <core/export.hpp>

NS_ASSUME_NONNULL_BEGIN

// Convenient container for a training graph and inference graph (that may share
// some structure).
EXPORT
API_AVAILABLE(macos(10.16))
@interface TCModelTrainerBackendGraphs : NSObject

@property(nonatomic, nullable, strong) MLCTrainingGraph *trainingGraph;

// The input tensors registered with the training graph via
// -addInputs:lossLabels:
@property(nonatomic, nullable, strong) NSDictionary<NSString *, MLCTensor *> *trainingInputs;

// The loss-label tensors registered with the training graph via
// -addInputs:lossLabels:, if any.
@property(nonatomic, nullable, strong) NSDictionary<NSString *, MLCTensor *> *trainingLossLabels;

// The loss-label-weight tensors registered with the training graph via
// -addInputs:lossLabels:lossLabelWeights, if any.
@property(nonatomic, nullable, strong)
    NSDictionary<NSString *, MLCTensor *> *trainingLossLabelWeights;

// The output tensors registered with the training graph via -addOutputs:, if
// any.
@property(nonatomic, nullable, strong) NSDictionary<NSString *, MLCTensor *> *trainingOutputs;

@property(nonatomic, nullable, strong) MLCInferenceGraph *inferenceGraph;

// The input tensors registered with the inference graph via -addInputs:
@property(nonatomic, nullable, strong) NSDictionary<NSString *, MLCTensor *> *inferenceInputs;

// The output tensors registered with the inference graph via -addOutputs:, if
// any.
@property(nonatomic, nullable, strong) NSDictionary<NSString *, MLCTensor *> *inferenceOutputs;

/// The weights of the layers comprising the graph.
@property(nonatomic, nullable, strong) NSDictionary<NSString *, NSData *> *layerWeights;

@end

API_AVAILABLE(macos(10.16))
@interface TCModelTrainerBackendGraphsWithSplitLoss : TCModelTrainerBackendGraphs

@property(nonatomic, nullable, strong) MLCTrainingGraph *lossGraph;

// The input tensors registered with the training graph via
// -addInputs:lossLabels:
@property(nonatomic, nullable, strong) NSDictionary<NSString *, MLCTensor *> *lossGraphInputs;

// The output tensors registered with the training graph via -addOutputs:, if
// any.
@property(nonatomic, nullable, strong) NSDictionary<NSString *, MLCTensor *> *lossGraphOutputs;

@end

NS_ASSUME_NONNULL_END

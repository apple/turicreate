/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <ml/neural_net/style_transfer/mps_style_transfer_pre_processing.h>

#import <ml/neural_net/mps_layer_helper.h>

@interface TCMPSStyleTransferPreProcessing()
@property (nonatomic) MPSNNMultiplicationNode *multiplicationNode;
@property (nonatomic) MPSNNSubtractionNode *subtractionNode;
@end

@implementation TCMPSStyleTransferPreProcessing
- (instancetype) initWithParameters:(NSString *)name
                          inputNode:(MPSNNImageNode *)inputNode
                          scaleNode:(MPSNNImageNode *)scaleNode
                           meanNode:(MPSNNImageNode *)meanNode {
  self = [super init];
  if (self) {
    _multiplicationNode = [MPSNNMultiplicationNode nodeWithLeftSource:inputNode
                                                          rightSource:scaleNode];

    _subtractionNode = [MPSNNSubtractionNode nodeWithLeftSource:[_multiplicationNode resultImage]
                                                    rightSource:meanNode];

    _output = [_subtractionNode resultImage];
  }
  return self;
}

- (MPSNNImageNode *) backwardPass:(MPSNNImageNode *)inputNode {
  MPSNNGradientFilterNode* subtractionGrad = [_subtractionNode gradientFilterWithSource: inputNode];
  MPSNNGradientFilterNode* multiplicationGrad = [_multiplicationNode gradientFilterWithSource: [subtractionGrad resultImage]];

  return [multiplicationGrad resultImage];
}

@end
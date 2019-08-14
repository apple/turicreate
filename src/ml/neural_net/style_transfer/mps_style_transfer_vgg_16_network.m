/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <ml/neural_net/style_transfer/mps_style_transfer_vgg_16_block_1_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_vgg_16_block_2_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_vgg_16_network.h>

@interface TCMPSVgg16Network ()
@property (nonatomic) TCMPSVgg16Block1 *block1;
@property (nonatomic) TCMPSVgg16Block1 *block2;
@property (nonatomic) TCMPSVgg16Block2 *block3;
@property (nonatomic) TCMPSVgg16Block2 *block4;
@end

@implementation TCMPSVgg16Network : NSObject

- (instancetype) initWithParameters:(NSString *)name
                          inputNode:(MPSNNImageNode *)inputNode
                             device:(id<MTLDevice>)dev
                           cmdQueue:(id<MTLCommandQueue>)cmdQ
                         descriptor:(TCMPSVgg16Descriptor *)descriptor
                        initWeights:(NSDictionary<NSString *, NSData *> *) weights {
  self = [super init];
  if (self) {

    _block1 = [[TCMPSVgg16Block1 alloc] initWithParameters:@"vgg_block_1_"
                                                 inputNode:inputNode
                                                    device:dev
                                                  cmdQueue:cmdQ
                                                descriptor:descriptor.block1
                                               initWeights:weights];

    _block2 = [[TCMPSVgg16Block1 alloc] initWithParameters:@"vgg_block_2_"
                                                 inputNode:[_block1 output]
                                                    device:dev
                                                  cmdQueue:cmdQ
                                                descriptor:descriptor.block2
                                               initWeights:weights];

    _block3 = [[TCMPSVgg16Block2 alloc] initWithParameters:@"vgg_block_3_"
                                                 inputNode:[_block2 output]
                                                    device:dev
                                                  cmdQueue:cmdQ
                                                descriptor:descriptor.block3
                                               initWeights:weights];

    _block4 = [[TCMPSVgg16Block2 alloc] initWithParameters:@"vgg_block_4_"
                                                 inputNode:[_block3 output]
                                                    device:dev
                                                  cmdQueue:cmdQ
                                                descriptor:descriptor.block4
                                               initWeights:weights];

    _reluOut1 = [_block1 features];
    _reluOut2 = [_block2 features];
    _reluOut3 = [_block3 features];
    _reluOut4 = [_block4 features];

    _output = [_block4 output];
  }
  return self;
}

- (MPSNNImageNode *) backwardPass:(MPSNNImageNode *) inputNode {
  MPSNNImageNode* block4Grad = [_block4 backwardPass: inputNode];
  MPSNNImageNode* block3Grad = [_block3 backwardPass: block4Grad];
  MPSNNImageNode* block2Grad = [_block2 backwardPass: block3Grad];
  MPSNNImageNode* block1Grad = [_block1 backwardPass: block2Grad];

  return block1Grad;
}

- (void) setLearningRate:(float)lr {
  [_block1 setLearningRate:lr];
  [_block2 setLearningRate:lr];
  [_block3 setLearningRate:lr];
  [_block4 setLearningRate:lr];
}

@end
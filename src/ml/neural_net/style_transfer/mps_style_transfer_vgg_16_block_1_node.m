/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <ml/neural_net/style_transfer/mps_style_transfer_vgg_16_block_1_node.h>
#import <ml/neural_net/mps_layer_helper.h>

@interface TCMPSVgg16Block1 ()
@property (nonatomic) MPSCNNConvolutionNode *conv1;
@property (nonatomic) MPSCNNNeuronReLUNNode *relu1;
@property (nonatomic) MPSCNNConvolutionNode *conv2;
@property (nonatomic) MPSCNNNeuronReLUNNode *relu2;
@property (nonatomic) MPSCNNPoolingAverageNode *pooling;
@end

@implementation TCMPSVgg16Block1 : NSObject

- (instancetype) initWithParameters:(NSString *)name
                          inputNode:(MPSNNImageNode *)inputNode
                             device:(id<MTLDevice>)dev
                           cmdQueue:(id<MTLCommandQueue>)cmdQ
                         descriptor:(TCMPSVgg16Block1Descriptor *)descriptor
                        initWeights:(NSDictionary<NSString *, NSData *> *) weights {
  self = [super init];
  if (self) {
    _conv1 = [MPSCNNConvolutionNode createConvolutional:inputNode
                                            kernelWidth:descriptor.conv1.kernelWidth
                                           kernelHeight:descriptor.conv1.kernelHeight
                                   inputFeatureChannels:descriptor.conv1.inputFeatureChannels
                                  outputFeatureChannels:descriptor.conv1.outputFeatureChannels
                                            strideWidth:descriptor.conv1.strideWidth
                                           strideHeight:descriptor.conv1.strideHeight
                                           paddingWidth:descriptor.conv1.paddingWidth
                                          paddingHeight:descriptor.conv1.paddingHeight
                                                weights:weights[[NSString stringWithFormat:@"%@/%@", name, @"block_1_conv_1_weights"]]
                                                 biases:weights[[NSString stringWithFormat:@"%@/%@", name, @"block_1_conv_1_biases"]]
                                                  label:descriptor.conv1.label
                                          updateWeights:descriptor.conv1.updateWeights
                                                 device:dev
                                               cmdQueue:cmdQ];

    _relu1 = [MPSCNNNeuronReLUNNode nodeWithSource: [_conv1 resultImage]];

    _conv2 = [MPSCNNConvolutionNode createConvolutional:[_relu1 resultImage]
                                            kernelWidth:descriptor.conv2.kernelWidth
                                           kernelHeight:descriptor.conv2.kernelHeight
                                   inputFeatureChannels:descriptor.conv2.inputFeatureChannels
                                  outputFeatureChannels:descriptor.conv2.outputFeatureChannels
                                            strideWidth:descriptor.conv2.strideWidth
                                           strideHeight:descriptor.conv2.strideHeight
                                           paddingWidth:descriptor.conv2.paddingWidth
                                          paddingHeight:descriptor.conv2.paddingHeight
                                                weights:weights[[NSString stringWithFormat:@"%@/%@", name, @"block_1_conv_2_weights"]]
                                                 biases:weights[[NSString stringWithFormat:@"%@/%@", name, @"block_1_conv_2_biases"]]
                                                  label:descriptor.conv2.label
                                          updateWeights:descriptor.conv2.updateWeights
                                                 device:dev
                                               cmdQueue:cmdQ];

    _relu2 = [MPSCNNNeuronReLUNNode nodeWithSource: [_conv2 resultImage]];

    _pooling = [[MPSCNNPoolingAverageNode alloc] initWithSource:[_relu2 resultImage]
                                                    kernelWidth:descriptor.pooling.kernelSize
                                                   kernelHeight:descriptor.pooling.kernelSize 
                                                strideInPixelsX:descriptor.pooling.strideSize
                                                strideInPixelsY:descriptor.pooling.strideSize];

    _output = [_pooling resultImage];
    _features = [_relu2 resultImage];
  }
  return self;
}

- (MPSNNImageNode *) backwardPass:(MPSNNImageNode *) inputNode {
  MPSNNGradientFilterNode* poolingGrad = [_pooling gradientFilterWithSource: inputNode];
  MPSNNGradientFilterNode* relu2Grad = [_relu2 gradientFilterWithSource: [poolingGrad resultImage]];
  MPSNNGradientFilterNode* conv2Grad = [_conv2 gradientFilterWithSource: [relu2Grad resultImage]];
  MPSNNGradientFilterNode* relu1Grad = [_relu1 gradientFilterWithSource: [conv2Grad resultImage]];
  MPSNNGradientFilterNode* conv1Grad = [_conv1 gradientFilterWithSource: [relu1Grad resultImage]];

  return [conv1Grad resultImage];
}

@end
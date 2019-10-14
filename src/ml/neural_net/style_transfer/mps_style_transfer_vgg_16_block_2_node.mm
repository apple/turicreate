/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <ml/neural_net/style_transfer/mps_style_transfer_vgg_16_block_2_node.h>

#import <ml/neural_net/mps_layer_helper.h>
#import <ml/neural_net/mps_weight.h>

@interface TCMPSVgg16Block2 ()
@property (nonatomic) MPSCNNConvolutionNode *conv1;
@property (nonatomic) MPSCNNNeuronReLUNode *relu1;
@property (nonatomic) MPSCNNConvolutionNode *conv2;
@property (nonatomic) MPSCNNNeuronReLUNode *relu2;
@property (nonatomic) MPSCNNConvolutionNode *conv3;
@property (nonatomic) MPSCNNNeuronReLUNode *relu3;
@property (nonatomic) MPSCNNPoolingAverageNode *pooling;
@end

@implementation TCMPSVgg16Block2 : NSObject

- (instancetype) initWithParameters:(NSString *)name
                          inputNode:(MPSNNImageNode *)inputNode
                             device:(id<MTLDevice>)dev
                           cmdQueue:(id<MTLCommandQueue>)cmdQ
                         descriptor:(TCMPSVgg16Block2Descriptor *)descriptor
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
                                                weights:weights[[NSString stringWithFormat:@"%@%@", name, @"conv_1_weight"]]
                                                 biases:weights[[NSString stringWithFormat:@"%@%@", name, @"conv_1_bias"]]
                                                  label:descriptor.conv1.label
                                          updateWeights:descriptor.conv1.updateWeights
                                                 device:dev
                                               cmdQueue:cmdQ];

    _relu1 = [MPSCNNNeuronReLUNode nodeWithSource: [_conv1 resultImage]];

    _conv2 = [MPSCNNConvolutionNode createConvolutional:[_relu1 resultImage]
                                            kernelWidth:descriptor.conv2.kernelWidth
                                           kernelHeight:descriptor.conv2.kernelHeight
                                   inputFeatureChannels:descriptor.conv2.inputFeatureChannels
                                  outputFeatureChannels:descriptor.conv2.outputFeatureChannels
                                            strideWidth:descriptor.conv2.strideWidth
                                           strideHeight:descriptor.conv2.strideHeight
                                           paddingWidth:descriptor.conv2.paddingWidth
                                          paddingHeight:descriptor.conv2.paddingHeight
                                                weights:weights[[NSString stringWithFormat:@"%@%@", name, @"conv_2_weights"]]
                                                 biases:weights[[NSString stringWithFormat:@"%@%@", name, @"conv_2_biases"]]
                                                  label:descriptor.conv2.label
                                          updateWeights:descriptor.conv2.updateWeights
                                                 device:dev
                                               cmdQueue:cmdQ];

    _relu2 = [MPSCNNNeuronReLUNode nodeWithSource: [_conv2 resultImage]];

    _conv3 = [MPSCNNConvolutionNode createConvolutional:[_relu2 resultImage]
                                            kernelWidth:descriptor.conv2.kernelWidth
                                           kernelHeight:descriptor.conv2.kernelHeight
                                   inputFeatureChannels:descriptor.conv2.inputFeatureChannels
                                  outputFeatureChannels:descriptor.conv2.outputFeatureChannels
                                            strideWidth:descriptor.conv2.strideWidth
                                           strideHeight:descriptor.conv2.strideHeight
                                           paddingWidth:descriptor.conv2.paddingWidth
                                          paddingHeight:descriptor.conv2.paddingHeight
                                                weights:weights[[NSString stringWithFormat:@"%@%@", name, @"conv_3_weights"]]
                                                 biases:weights[[NSString stringWithFormat:@"%@%@", name, @"conv_3_biases"]]
                                                  label:descriptor.conv2.label
                                          updateWeights:descriptor.conv2.updateWeights
                                                 device:dev
                                               cmdQueue:cmdQ];

    _relu3 = [MPSCNNNeuronReLUNode nodeWithSource: [_conv3 resultImage]];

    _pooling = [[MPSCNNPoolingAverageNode alloc] initWithSource:[_relu3 resultImage]
                                                    kernelWidth:descriptor.pooling.kernelSize
                                                   kernelHeight:descriptor.pooling.kernelSize 
                                                strideInPixelsX:descriptor.pooling.strideSize
                                                strideInPixelsY:descriptor.pooling.strideSize];

    _output = [_pooling resultImage];
    _features = [_relu3 resultImage];
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

- (void) setLearningRate:(float)lr {
  [_conv1.tc_weightsData setLearningRate:lr];
  [_conv2.tc_weightsData setLearningRate:lr];
  [_conv3.tc_weightsData setLearningRate:lr];
}

@end
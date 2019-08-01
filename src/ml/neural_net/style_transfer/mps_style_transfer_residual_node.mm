/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <ml/neural_net/style_transfer/mps_style_transfer_residual_node.h>

#import <ml/neural_net/mps_layer_helper.h>
#import <ml/neural_net/mps_layer_instance_norm_data_loader.h>
#import <ml/neural_net/mps_weight.h>

@interface TCMPSStyleTransferResidualNode ()
@property (nonatomic) MPSCNNConvolutionNode *conv1;
@property (nonatomic) MPSCNNInstanceNormalizationNode *instNorm1;
@property (nonatomic) MPSCNNNeuronReLUNNode *relu1;

@property (nonatomic) MPSCNNConvolutionNode *conv2;
@property (nonatomic) MPSCNNInstanceNormalizationNode *instNorm2;
@property (nonatomic) MPSNNAdditionNode *add;
@end

@implementation TCMPSStyleTransferResidualNode : NSObject

- (instancetype) initWithParameters:(NSString *)name
                          inputNode:(MPSNNImageNode *)inputNode
                             device:(id<MTLDevice>)dev
                           cmdQueue:(id<MTLCommandQueue>)cmdQ
                         descriptor:(TCMPSResidualDescriptor *)descriptor
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
                                                weights:weights[[NSString stringWithFormat:@"%@%@", name, @"conv_1_weights"]]
                                                 biases:weights[[NSString stringWithFormat:@"%@%@", name, @"conv_1_bias"]]
                                                  label:descriptor.conv1.label
                                          updateWeights:descriptor.conv1.updateWeights
                                                 device:dev
                                               cmdQueue:cmdQ];

    _instNorm1 = [MPSCNNInstanceNormalizationNode createInstanceNormalization:[_conv1 resultImage]
                                                                     channels:descriptor.inst1.channels
                                                                       styles:descriptor.inst1.styles
                                                                        gamma:weights[[NSString stringWithFormat:@"%@%@", name, @"inst_1_gamma"]]
                                                                         beta:weights[[NSString stringWithFormat:@"%@%@", name, @"inst_1_beta"]]
                                                                        label:descriptor.inst1.label
                                                                       device:dev
                                                                     cmdQueue:cmdQ];

    _relu1 = [MPSCNNNeuronReLUNNode nodeWithSource:[_instNorm1 resultImage]];

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
                                                 biases:weights[[NSString stringWithFormat:@"%@%@", name, @"conv_2_bias"]]
                                                  label:descriptor.conv2.label
                                          updateWeights:descriptor.conv2.updateWeights
                                                 device:dev
                                               cmdQueue:cmdQ];

    _instNorm2 = [MPSCNNInstanceNormalizationNode createInstanceNormalization:[_conv2 resultImage]
                                                                     channels:descriptor.inst2.channels
                                                                       styles:descriptor.inst2.styles
                                                                        gamma:weights[[NSString stringWithFormat:@"%@%@", name, @"inst_2_gamma"]]
                                                                         beta:weights[[NSString stringWithFormat:@"%@%@", name, @"inst_2_beta"]]
                                                                        label:descriptor.inst2.label
                                                                       device:dev
                                                                     cmdQueue:cmdQ]; 

    _add = [MPSNNAdditionNode nodeWithSources:@[inputNode, [_instNorm2 resultImage]]];

    _output = [_add resultImage];
  }

  return self;
}

- (MPSNNImageNode *) backwardPass:(MPSNNImageNode *) inputNode {
  NSArray<MPSNNGradientFilterNode *>* addGrad = [_add gradientFiltersWithSources: @[inputNode]];
  MPSNNGradientFilterNode* inst2Grad = [_instNorm2 gradientFilterWithSource: [addGrad[0] resultImage]];
  MPSNNGradientFilterNode* conv2Grad = [_conv2 gradientFilterWithSource: [inst2Grad resultImage]];
  MPSNNGradientFilterNode* relu1Grad = [_relu1 gradientFilterWithSource: [conv2Grad resultImage]];
  MPSNNGradientFilterNode* inst1Grad = [_instNorm1 gradientFilterWithSource: [relu1Grad resultImage]];;
  MPSNNGradientFilterNode* conv1Grad = [_conv1 gradientFilterWithSource: [inst1Grad resultImage]];

  return [conv1Grad resultImage];
}

- (void) setLearningRate:(float)lr {
  [[_conv1 weights] setLearningRate:lr];
  [[_instNorm1 weights] setLearningRate:lr];
  [[_conv2 weights] setLearningRate:lr];
  [[_instNorm2 weights] setLearningRate:lr];
}

- (NSDictionary<NSString *, NSData *> *)exportWeights:(NSString *)prefix {
  NSMutableDictionary<NSString *, NSData *> *weights = [[NSMutableDictionary alloc] init];;

  NSString* conv1Keys = [NSString stringWithFormat:@"%@%@", prefix, @"conv_1_weights"];
  NSUInteger conv1WeightSize = (NSUInteger)([[_conv1 weights] weightSize] * sizeof(float));
  NSMutableData* conv1DataWeight = [NSMutableData dataWithLength:conv1WeightSize];
  memcpy(conv1DataWeight.mutableBytes, [[_conv1 weights] weights], conv1WeightSize);

  weights[conv1Keys] = conv1DataWeight;

  NSString* conv2Keys = [NSString stringWithFormat:@"%@%@", prefix, @"conv_2_weights"];
  NSUInteger conv2WeightSize = (NSUInteger)([[_conv2 weights] weightSize] * sizeof(float));
  NSMutableData* conv2DataWeight = [NSMutableData dataWithLength:conv2WeightSize];
  memcpy(conv2DataWeight.mutableBytes, [[_conv2 weights] weights], conv2WeightSize);

  weights[conv2Keys] = conv2DataWeight;

  NSString* instNorm1GammaKeys = [NSString stringWithFormat:@"%@%@", prefix, @"inst_1_gamma"];
  NSString* instNorm1BetaKeys = [NSString stringWithFormat:@"%@%@", prefix, @"inst_1_beta"];

  NSUInteger instNorm1Size = (NSUInteger)([[_instNorm1 weights] numberOfFeatureChannels] * sizeof(float));
  
  NSMutableData* instNorm1DataGamma = [NSMutableData dataWithLength:instNorm1Size];
  NSMutableData* instNorm1DataBeta = [NSMutableData dataWithLength:instNorm1Size];
  
  memcpy(instNorm1DataGamma.mutableBytes, [[_instNorm1 weights] gamma], instNorm1Size);
  memcpy(instNorm1DataBeta.mutableBytes, [[_instNorm1 weights] beta], instNorm1Size);

  weights[instNorm1GammaKeys] = instNorm1DataGamma;
  weights[instNorm1BetaKeys] = instNorm1DataBeta;

  NSString* instNorm2GammaKeys = [NSString stringWithFormat:@"%@%@", prefix, @"inst_2_gamma"];
  NSString* instNorm2BetaKeys = [NSString stringWithFormat:@"%@%@", prefix, @"inst_2_beta"];

  NSUInteger instNorm2Size = (NSUInteger)([[_instNorm2 weights] numberOfFeatureChannels] * sizeof(float));

  NSMutableData* instNorm2DataGamma = [NSMutableData dataWithLength:instNorm2Size];
  NSMutableData* instNorm2DataBeta = [NSMutableData dataWithLength:instNorm2Size];

  memcpy(instNorm2DataGamma.mutableBytes, [[_instNorm2 weights] gamma], instNorm2Size);
  memcpy(instNorm2DataBeta.mutableBytes, [[_instNorm2 weights] beta], instNorm2Size);

  weights[instNorm2GammaKeys] = instNorm2DataGamma;
  weights[instNorm2BetaKeys] = instNorm2DataBeta;
  
  return [weights copy];
}

@end
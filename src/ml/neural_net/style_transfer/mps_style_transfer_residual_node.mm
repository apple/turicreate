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
@property (nonatomic) MPSCNNNeuronReLUNode *relu1;

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
    // No bias
    NSMutableData* zeroedConv1Biases = [NSMutableData dataWithLength:descriptor.conv1.outputFeatureChannels*sizeof(float)];
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
                                                 biases:zeroedConv1Biases
                                                  label:descriptor.conv1.label
                                          updateWeights:descriptor.conv1.updateWeights
                                                 device:dev
                                               cmdQueue:cmdQ];

    _instNorm1 = [MPSCNNInstanceNormalizationNode createInstanceNormalization:[_conv1 resultImage]
                                                                     channels:descriptor.inst1.channels
                                                                       styles:descriptor.inst1.styles
                                                                        gamma:weights[[NSString stringWithFormat:@"%@%@", name, @"inst_1_gamma_weight"]]
                                                                         beta:weights[[NSString stringWithFormat:@"%@%@", name, @"inst_1_beta_weight"]]
                                                                        label:descriptor.inst1.label
                                                                       device:dev
                                                                     cmdQueue:cmdQ];

    _relu1 = [MPSCNNNeuronReLUNode nodeWithSource:[_instNorm1 resultImage]];

    // No bias
    NSMutableData* zeroedConv2Biases = [NSMutableData dataWithLength:descriptor.conv2.outputFeatureChannels*sizeof(float)];
    _conv2 = [MPSCNNConvolutionNode createConvolutional:[_relu1 resultImage]
                                            kernelWidth:descriptor.conv2.kernelWidth
                                           kernelHeight:descriptor.conv2.kernelHeight
                                   inputFeatureChannels:descriptor.conv2.inputFeatureChannels
                                  outputFeatureChannels:descriptor.conv2.outputFeatureChannels
                                            strideWidth:descriptor.conv2.strideWidth
                                           strideHeight:descriptor.conv2.strideHeight
                                           paddingWidth:descriptor.conv2.paddingWidth
                                          paddingHeight:descriptor.conv2.paddingHeight
                                                weights:weights[[NSString stringWithFormat:@"%@%@", name, @"conv_2_weight"]]
                                                 biases:zeroedConv2Biases
                                                  label:descriptor.conv2.label
                                          updateWeights:descriptor.conv2.updateWeights
                                                 device:dev
                                               cmdQueue:cmdQ];

    _instNorm2 = [MPSCNNInstanceNormalizationNode createInstanceNormalization:[_conv2 resultImage]
                                                                     channels:descriptor.inst2.channels
                                                                       styles:descriptor.inst2.styles
                                                                        gamma:weights[[NSString stringWithFormat:@"%@%@", name, @"inst_2_gamma_weight"]]
                                                                         beta:weights[[NSString stringWithFormat:@"%@%@", name, @"inst_2_beta_weight"]]
                                                                        label:descriptor.inst2.label
                                                                       device:dev
                                                                     cmdQueue:cmdQ]; 

    _add = [MPSNNAdditionNode nodeWithSources:@[inputNode, [_instNorm2 resultImage]]];

    _output = [_add resultImage];
  }

  return self;
}

- (void) setStyleIndex:(NSUInteger)styleIndex {
  _instNorm1.tc_weightsData.styleIndex = styleIndex;
  [_instNorm1.tc_weightsData checkpoint];
  _instNorm2.tc_weightsData.styleIndex = styleIndex;
  [_instNorm2.tc_weightsData checkpoint];
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
  [_conv1.tc_weightsData setLearningRate:lr];
  [_instNorm1.tc_weightsData setLearningRate:lr];
  [_conv2.tc_weightsData setLearningRate:lr];
  [_instNorm2.tc_weightsData setLearningRate:lr];
}

- (NSDictionary<NSString *, NSData *> *)exportWeights:(NSString *)prefix {
  NSMutableDictionary<NSString *, NSData *> *weights = [[NSMutableDictionary alloc] init];;

  NSString* conv1Keys = [NSString stringWithFormat:@"%@%@", prefix, @"conv_1_weight"];
  NSUInteger conv1WeightSize = (NSUInteger)([_conv1.tc_weightsData weightSize] * sizeof(float));
  NSMutableData* conv1DataWeight = [NSMutableData dataWithLength:conv1WeightSize];
  memcpy(conv1DataWeight.mutableBytes, [_conv1.tc_weightsData weights], conv1WeightSize);

  weights[conv1Keys] = conv1DataWeight;

  NSString* conv2Keys = [NSString stringWithFormat:@"%@%@", prefix, @"conv_2_weight"];
  NSUInteger conv2WeightSize = (NSUInteger)([_conv2.tc_weightsData weightSize] * sizeof(float));
  NSMutableData* conv2DataWeight = [NSMutableData dataWithLength:conv2WeightSize];
  memcpy(conv2DataWeight.mutableBytes, [_conv2.tc_weightsData weights], conv2WeightSize);

  weights[conv2Keys] = conv2DataWeight;

  NSString* instNorm1GammaKeys = [NSString stringWithFormat:@"%@%@", prefix, @"inst_1_gamma_weight"];
  NSString* instNorm1BetaKeys = [NSString stringWithFormat:@"%@%@", prefix, @"inst_1_beta_weight"];

  NSUInteger instNorm1Size = (NSUInteger)([_instNorm1.tc_weightsData styles] * [_instNorm1.tc_weightsData numberOfFeatureChannels] * sizeof(float));
  
  NSMutableData* instNorm1DataGamma = [NSMutableData dataWithLength:instNorm1Size];
  NSMutableData* instNorm1DataBeta = [NSMutableData dataWithLength:instNorm1Size];
  
  memcpy(instNorm1DataGamma.mutableBytes, [_instNorm1.tc_weightsData gamma], instNorm1Size);
  memcpy(instNorm1DataBeta.mutableBytes, [_instNorm1.tc_weightsData beta], instNorm1Size);

  weights[instNorm1GammaKeys] = instNorm1DataGamma;
  weights[instNorm1BetaKeys] = instNorm1DataBeta;

  NSString* instNorm2GammaKeys = [NSString stringWithFormat:@"%@%@", prefix, @"inst_2_gamma_weight"];
  NSString* instNorm2BetaKeys = [NSString stringWithFormat:@"%@%@", prefix, @"inst_2_beta_weight"];

  NSUInteger instNorm2Size = (NSUInteger)([_instNorm2.tc_weightsData styles] * [_instNorm2.tc_weightsData numberOfFeatureChannels] * sizeof(float));

  NSMutableData* instNorm2DataGamma = [NSMutableData dataWithLength:instNorm2Size];
  NSMutableData* instNorm2DataBeta = [NSMutableData dataWithLength:instNorm2Size];

  memcpy(instNorm2DataGamma.mutableBytes, [_instNorm2.tc_weightsData gamma], instNorm2Size);
  memcpy(instNorm2DataBeta.mutableBytes, [_instNorm2.tc_weightsData beta], instNorm2Size);

  weights[instNorm2GammaKeys] = instNorm2DataGamma;
  weights[instNorm2BetaKeys] = instNorm2DataBeta;
  
  return [weights copy];
}

@end
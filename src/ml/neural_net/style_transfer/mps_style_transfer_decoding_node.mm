/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <ml/neural_net/style_transfer/mps_style_transfer_decoding_node.h>

#import <ml/neural_net/mps_layer_helper.h>
#import <ml/neural_net/mps_layer_instance_norm_data_loader.h>
#import <ml/neural_net/mps_weight.h>

@interface TCMPSStyleTransferDecodingNode ()
@property (nonatomic) MPSCNNUpsamplingNearestNode *upsample;
@property (nonatomic) MPSCNNConvolutionNode *conv;
@property (nonatomic) MPSCNNInstanceNormalizationNode* instNorm;
@property (nonatomic) MPSCNNNeuronReLUNNode* relu;
@end

@implementation TCMPSStyleTransferDecodingNode

- (instancetype) initWithParameters:(NSString *)name
                          inputNode:(MPSNNImageNode *)inputNode
                             device:(id<MTLDevice>)dev
                           cmdQueue:(id<MTLCommandQueue>)cmdQ
                         descriptor:(TCMPSDecodingDescriptor *)descriptor
                        initWeights:(NSDictionary<NSString *, NSData *> *) weights {
  self = [super init];
  
  if (self) {
  	_upsample = [MPSCNNUpsamplingNearestNode nodeWithSource:inputNode
                                       integerScaleFactorX:descriptor.upsample.scale
                                       integerScaleFactorY:descriptor.upsample.scale];

  	_conv = [MPSCNNConvolutionNode createConvolutional:[_upsample resultImage]
                                           kernelWidth:descriptor.conv.kernelWidth
                                          kernelHeight:descriptor.conv.kernelHeight
                                  inputFeatureChannels:descriptor.conv.inputFeatureChannels
                                 outputFeatureChannels:descriptor.conv.outputFeatureChannels
                                           strideWidth:descriptor.conv.strideWidth
                                          strideHeight:descriptor.conv.strideHeight
                                          paddingWidth:descriptor.conv.paddingWidth
                                         paddingHeight:descriptor.conv.paddingHeight
                                               weights:weights[[NSString stringWithFormat:@"%@%@", name, @"conv_weights"]]
                                                biases:weights[[NSString stringWithFormat:@"%@%@", name, @"conv_biases"]]
                                                 label:descriptor.conv.label
                                         updateWeights:descriptor.conv.updateWeights
                                                device:dev
                                              cmdQueue:cmdQ];

    _instNorm = [MPSCNNInstanceNormalizationNode createInstanceNormalization:[_conv resultImage]
                                                                    channels:descriptor.inst.channels
                                                                      styles:descriptor.inst.styles
                                                                       gamma:weights[[NSString stringWithFormat:@"%@%@", name, @"inst_gamma"]]
                                                                        beta:weights[[NSString stringWithFormat:@"%@%@", name, @"inst_beta"]]
                                                                       label:descriptor.inst.label
                                                                      device:dev
                                                                    cmdQueue:cmdQ];

    _relu = [MPSCNNNeuronReLUNNode nodeWithSource: [_instNorm resultImage]];

    _output = [_relu resultImage];
  }

  return self;
}

- (MPSNNImageNode *) backwardPass:(MPSNNImageNode *) inputNode {
  MPSNNGradientFilterNode* reluGrad = [_relu gradientFilterWithSource: inputNode];
  MPSNNGradientFilterNode* instNormGrad = [_instNorm gradientFilterWithSource: [reluGrad resultImage]];
  MPSNNGradientFilterNode* convGrad = [_conv gradientFilterWithSource: [instNormGrad resultImage]];
  MPSNNGradientFilterNode* upsampleGrad = [_upsample gradientFilterWithSource: [convGrad resultImage]];

  return [upsampleGrad resultImage];
}

- (void) setLearningRate:(float)lr {
  [[_conv weights] setLearningRate:lr];
  [[_instNorm weights] setLearningRate:lr];
}

- (NSDictionary<NSString *, NSData *> *)exportWeights:(NSString *)prefix {
  NSMutableDictionary<NSString *, NSData *> *weights = [[NSMutableDictionary alloc] init];

  NSUInteger convWeightSize = (NSUInteger)([[_conv weights] weightSize] * sizeof(float));
  NSMutableData* convDataWeight = [NSMutableData dataWithLength:convWeightSize];
  memcpy(convDataWeight.mutableBytes, [[_conv weights] weights], convWeightSize);

  NSString* convWeight = [NSString stringWithFormat:@"%@%@", prefix, @"conv_weights"];

  weights[convWeight] = convDataWeight;

  NSUInteger instNormSize = (NSUInteger)([[_instNorm weights] numberOfFeatureChannels] * sizeof(float));
  NSMutableData* instNormDataGamma = [NSMutableData dataWithLength:instNormSize];
  NSMutableData* instNormDataBeta = [NSMutableData dataWithLength:instNormSize];

  memcpy(instNormDataGamma.mutableBytes, [[_instNorm weights] gamma], instNormSize);
  memcpy(instNormDataBeta.mutableBytes, [[_instNorm weights] beta], instNormSize);

  NSString* instNormGamma = [NSString stringWithFormat:@"%@%@", prefix, @"inst_gamma"];
  NSString* instNormBeta = [NSString stringWithFormat:@"%@%@", prefix, @"inst_beta"];

  weights[instNormGamma] = instNormDataGamma;
  weights[instNormBeta] = instNormDataBeta;

  return [weights copy];
}

@end
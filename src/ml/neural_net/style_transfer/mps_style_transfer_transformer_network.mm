/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <ml/neural_net/style_transfer/mps_style_transfer_transformer_network.h>

#import <ml/neural_net/mps_layer_helper.h>
#import <ml/neural_net/mps_layer_instance_norm_data_loader.h>
#import <ml/neural_net/mps_weight.h>

#import <ml/neural_net/style_transfer/mps_style_transfer_decoding_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_encoding_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_residual_node.h>

@interface TCMPSStyleTransferTransformerNetwork ()
@property (nonatomic) TCMPSStyleTransferEncodingNode *encoding1;
@property (nonatomic) TCMPSStyleTransferEncodingNode *encoding2;
@property (nonatomic) TCMPSStyleTransferEncodingNode *encoding3;

@property (nonatomic) TCMPSStyleTransferResidualNode *residual1;
@property (nonatomic) TCMPSStyleTransferResidualNode *residual2;
@property (nonatomic) TCMPSStyleTransferResidualNode *residual3;
@property (nonatomic) TCMPSStyleTransferResidualNode *residual4;
@property (nonatomic) TCMPSStyleTransferResidualNode *residual5;

@property (nonatomic) TCMPSStyleTransferDecodingNode *decoding1;
@property (nonatomic) TCMPSStyleTransferDecodingNode *decoding2;

@property (nonatomic) MPSCNNConvolutionNode *conv;
@property (nonatomic) MPSCNNInstanceNormalizationNode *instNorm;
@property (nonatomic) MPSCNNNeuronSigmoidNode *sigmoid;
@end

@implementation TCMPSStyleTransferTransformerNetwork : NSObject

- (instancetype) initWithParameters:(NSString *)name
                          inputNode:(MPSNNImageNode *)inputNode
                             device:(id<MTLDevice>)dev
                           cmdQueue:(id<MTLCommandQueue>)cmdQ
                         descriptor:(TCMPSTransformerDescriptor *)descriptor
                        initWeights:(NSDictionary<NSString *, NSData *> *) weights {
  self = [super init];
  
  if (self) {
    _encoding1 = [[TCMPSStyleTransferEncodingNode alloc] initWithParameters:@"transformer_encode_1_"
                                                                  inputNode:inputNode
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.encode1
                                                                initWeights:weights];

    _encoding2 = [[TCMPSStyleTransferEncodingNode alloc] initWithParameters:@"transformer_encode_2_"
                                                                  inputNode:[_encoding1 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.encode2
                                                                initWeights:weights];

    _encoding3 = [[TCMPSStyleTransferEncodingNode alloc] initWithParameters:@"transformer_encode_3_"
                                                                  inputNode:[_encoding2 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.encode3
                                                                initWeights:weights];

    _residual1 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_1_"
                                                                  inputNode:[_encoding3 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.residual1
                                                                initWeights:weights];

    _residual2 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_2_"
                                                                  inputNode:[_residual1 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.residual2
                                                                initWeights:weights];

    _residual3 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_3_"
                                                                  inputNode:[_residual2 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.residual3
                                                                initWeights:weights];

    _residual4 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_4_"
                                                                  inputNode:[_residual3 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.residual4
                                                                initWeights:weights];

    _residual5 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_5_"
                                                                  inputNode:[_residual4 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.residual5
                                                                initWeights:weights];

    _decoding1 = [[TCMPSStyleTransferDecodingNode alloc] initWithParameters:@"transformer_decoding_1_"
                                                                 inputNode:[_residual5 output]
                                                                    device:dev
                                                                  cmdQueue:cmdQ
                                                                descriptor:descriptor.decode1
                                                               initWeights:weights];

    _decoding2 = [[TCMPSStyleTransferDecodingNode alloc] initWithParameters:@"transformer_decoding_2_"
                                                                  inputNode:[_decoding1 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.decode2
                                                                initWeights:weights];

    _conv = [MPSCNNConvolutionNode createConvolutional:[_decoding2 output]
                                           kernelWidth:descriptor.conv.kernelWidth
                                          kernelHeight:descriptor.conv.kernelHeight
                                  inputFeatureChannels:descriptor.conv.inputFeatureChannels
                                 outputFeatureChannels:descriptor.conv.outputFeatureChannels
                                           strideWidth:descriptor.conv.strideWidth
                                          strideHeight:descriptor.conv.strideHeight
                                          paddingWidth:descriptor.conv.paddingWidth
                                         paddingHeight:descriptor.conv.paddingHeight
                                               weights:weights[@"transformer_conv5_weight"]
                                                biases:weights[@"transformer_conv5_bias"]
                                                 label:descriptor.conv.label
                                         updateWeights:descriptor.conv.updateWeights
                                                device:dev
                                              cmdQueue:cmdQ];

    _instNorm = [MPSCNNInstanceNormalizationNode createInstanceNormalization:[_conv resultImage]
                                                                    channels:descriptor.inst.channels
                                                                      styles:descriptor.inst.styles
                                                                       gamma:weights[@"transformer_instancenorm5_gamma"]
                                                                        beta:weights[@"transformer_instancenorm5_beta"]
                                                                       label:descriptor.inst.label
                                                                      device:dev
                                                                    cmdQueue:cmdQ];

    _sigmoid = [MPSCNNNeuronSigmoidNode nodeWithSource:[_instNorm resultImage]];
    
    _forwardPass = [_sigmoid resultImage];
  }

  return self;
}

- (MPSNNImageNode *) backwardPass:(MPSNNImageNode *) inputNode {
  MPSNNGradientFilterNode* sigmoidGrad = [_sigmoid gradientFilterWithSource: inputNode];
  MPSNNGradientFilterNode* instanceNormGrad = [_instNorm gradientFilterWithSource: [sigmoidGrad resultImage]];
  MPSNNGradientFilterNode* convGrad = [_conv gradientFilterWithSource: [instanceNormGrad resultImage]];

  MPSNNImageNode* decoding2Img = [_decoding2 backwardPass:[convGrad resultImage]];
  MPSNNImageNode* decoding1Img = [_decoding1 backwardPass:decoding2Img];

  MPSNNImageNode* residual5Img = [_residual5 backwardPass:decoding1Img];
  MPSNNImageNode* residual4Img = [_residual4 backwardPass:residual5Img];
  MPSNNImageNode* residual3Img = [_residual3 backwardPass:residual4Img];
  MPSNNImageNode* residual2Img = [_residual2 backwardPass:residual3Img];
  MPSNNImageNode* residual1Img = [_residual1 backwardPass:residual2Img];

  MPSNNImageNode* encoding3Grad = [_encoding3 backwardPass:residual1Img];
  MPSNNImageNode* encoding2Grad = [_encoding2 backwardPass:encoding3Grad];
  MPSNNImageNode* encoding1Grad = [_encoding1 backwardPass:encoding2Grad];
  
  return encoding1Grad;
}

- (void) setLearningRate:(float)lr {
  [_encoding1 setLearningRate:lr];
  [_encoding2 setLearningRate:lr];
  [_encoding3 setLearningRate:lr];
  [_residual1 setLearningRate:lr];
  [_residual2 setLearningRate:lr];
  [_residual3 setLearningRate:lr];
  [_residual4 setLearningRate:lr];
  [_residual5 setLearningRate:lr];
  [_decoding1 setLearningRate:lr];
  [_decoding2 setLearningRate:lr];
  [_conv.tc_weightsData setLearningRate:lr];
  [_instNorm.tc_weightsData setLearningRate:lr];
}

- (NSDictionary<NSString *, NSData *> *) exportWeights:(NSString *)prefix {
  NSMutableDictionary<NSString *, NSData *> *weights = [[NSMutableDictionary alloc] init];

  NSString* encoding1Prefix = [NSString stringWithFormat:@"%@%@", prefix, @"encoding_1_"];
  NSDictionary<NSString *, NSData *> * encode1Weights = [_encoding1 exportWeights:encoding1Prefix];
  [weights addEntriesFromDictionary: encode1Weights];

  NSString* encoding2Prefix = [NSString stringWithFormat:@"%@%@", prefix, @"encoding_2_"];
  NSDictionary<NSString *, NSData *> * encode2Weights = [_encoding2 exportWeights:encoding2Prefix];
  [weights addEntriesFromDictionary: encode2Weights];

  NSString* encoding3Prefix = [NSString stringWithFormat:@"%@%@", prefix, @"encoding_3_"];
  NSDictionary<NSString *, NSData *> * encode3Weights = [_encoding3 exportWeights:encoding3Prefix];
  [weights addEntriesFromDictionary: encode3Weights];
  
  NSString* residual1Prefix = [NSString stringWithFormat:@"%@%@", prefix, @"residual_1_"];
  NSDictionary<NSString *, NSData *> * residual1Weights = [_residual1 exportWeights:residual1Prefix];
  [weights addEntriesFromDictionary: residual1Weights];

  NSString* residual2Prefix = [NSString stringWithFormat:@"%@%@", prefix, @"residual_2_"];
  NSDictionary<NSString *, NSData *> * residual2Weights = [_residual2 exportWeights:residual2Prefix];
  [weights addEntriesFromDictionary: residual2Weights];

  NSString* residual3Prefix = [NSString stringWithFormat:@"%@%@", prefix, @"residual_3_"];
  NSDictionary<NSString *, NSData *> * residual3Weights = [_residual3 exportWeights:residual3Prefix];
  [weights addEntriesFromDictionary: residual3Weights];
  
  NSString* residual4Prefix = [NSString stringWithFormat:@"%@%@", prefix, @"residual_4_"];
  NSDictionary<NSString *, NSData *> * residual4Weights = [_residual4 exportWeights:residual4Prefix];
  [weights addEntriesFromDictionary: residual4Weights];

  NSString* residual5Prefix = [NSString stringWithFormat:@"%@%@", prefix, @"residual_5_"];
  NSDictionary<NSString *, NSData *> * residual5Weights = [_residual5 exportWeights:residual5Prefix];
  [weights addEntriesFromDictionary: residual5Weights];

  NSString* decode1Prefix = [NSString stringWithFormat:@"%@%@", prefix, @"decode_1_"];
  NSDictionary<NSString *, NSData *> * decode1Weights = [_decoding1 exportWeights:decode1Prefix];
  [weights addEntriesFromDictionary: decode1Weights];

  NSString* decode2Prefix = [NSString stringWithFormat:@"%@%@", prefix, @"decode_2_"];
  NSDictionary<NSString *, NSData *> * decode2Weights = [_decoding2 exportWeights:decode2Prefix];
  [weights addEntriesFromDictionary: decode2Weights];

  NSUInteger convWeightSize = (NSUInteger)([_conv.tc_weightsData weightSize] * sizeof(float));
  NSMutableData* convDataWeight = [NSMutableData dataWithLength:convWeightSize];
  
  memcpy(convDataWeight.mutableBytes, [_conv.tc_weightsData weights], convWeightSize);
  NSString* conv5Weight = [NSString stringWithFormat:@"%@%@", prefix, @"conv5_weight"];

  weights[conv5Weight] = convDataWeight;

  NSString* instNorm5Gamma = [NSString stringWithFormat:@"%@%@", prefix, @"instancenorm5_gamma"];
  NSString* instNorm5Beta = [NSString stringWithFormat:@"%@%@", prefix, @"instancenorm5_beta"];
  NSUInteger instNormSize = (NSUInteger)([_instNorm.tc_weightsData numberOfFeatureChannels] * sizeof(float));
  NSMutableData* instNormDataGamma = [NSMutableData dataWithLength:instNormSize];
  NSMutableData* instNormDataBeta = [NSMutableData dataWithLength:instNormSize];

  memcpy(instNormDataGamma.mutableBytes, [_instNorm.tc_weightsData gamma], instNormSize);
  memcpy(instNormDataBeta.mutableBytes, [_instNorm.tc_weightsData beta], instNormSize);

  weights[instNorm5Gamma] = instNormDataGamma;
  weights[instNorm5Beta] = instNormDataBeta;

  return [weights copy];
}

@end
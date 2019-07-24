/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <ml/neural_net/style_transfer/mps_style_transfer_decoding_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_encoding_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_residual_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_transformer_network.h>
#import <ml/neural_net/mps_layer_helper.h>

#include <ml/neural_net/mps_weight.h>
#import <ml/neural_net/mps_layer_instance_norm_data_loader.h>

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
                        initWeights:(NSDictionary<NSString *, NSDictionary *> *) weights {
  self = [super init];
  
  if (self) {
    _encoding1 = [[TCMPSStyleTransferEncodingNode alloc] initWithParameters:@"transformer_encode_1"
                                                                  inputNode:inputNode
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.encode1
                                                                initWeights:weights[@"transformer_encode_1"]];

    _encoding2 = [[TCMPSStyleTransferEncodingNode alloc] initWithParameters:@"transformer_encode_2"
                                                                  inputNode:[_encoding1 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.encode2
                                                                initWeights:weights[@"transformer_encode_2"]];

    _encoding3 = [[TCMPSStyleTransferEncodingNode alloc] initWithParameters:@"transformer_encode_3"
                                                                  inputNode:[_encoding2 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.encode3
                                                                initWeights:weights[@"transformer_encode_3"]];

    _residual1 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_1"
                                                                  inputNode:[_encoding3 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.residual1
                                                                initWeights:weights[@"transformer_residual_1"]];

    _residual2 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_2"
                                                                  inputNode:[_residual1 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.residual2
                                                                initWeights:weights[@"transformer_residual_2"]];

    _residual3 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_3"
                                                                  inputNode:[_residual2 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.residual3
                                                                initWeights:weights[@"transformer_residual_3"]];

    _residual4 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_4"
                                                                  inputNode:[_residual3 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.residual4
                                                                initWeights:weights[@"transformer_residual_4"]];

    _residual5 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_5"
                                                                  inputNode:[_residual4 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.residual5
                                                                initWeights:weights[@"transformer_residual_5"]];

    _decoding1 = [[TCMPSStyleTransferDecodingNode alloc] initWithParameters:@"transformer_decoding_1"
                                                                 inputNode:[_residual5 output]
                                                                    device:dev
                                                                  cmdQueue:cmdQ
                                                                descriptor:descriptor.decode1
                                                               initWeights:weights[@"transformer_decoding_1"]];

    _decoding2 = [[TCMPSStyleTransferDecodingNode alloc] initWithParameters:@"transformer_decoding_2"
                                                                  inputNode:[_decoding1 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.decode2
                                                                initWeights:weights[@"transformer_decoding_2"]];

    
    _conv = [MPSCNNConvolutionNode createConvolutional:[_decoding2 output]
                                           kernelWidth:descriptor.conv.kernelWidth
                                          kernelHeight:descriptor.conv.kernelHeight
                                  inputFeatureChannels:descriptor.conv.inputFeatureChannels
                                 outputFeatureChannels:descriptor.conv.outputFeatureChannels
                                           strideWidth:descriptor.conv.strideWidth
                                          strideHeight:descriptor.conv.strideHeight
                                          paddingWidth:descriptor.conv.paddingWidth
                                         paddingHeight:descriptor.conv.paddingHeight
                                               weights:((NSData *)weights[@"transformer_conv"][@"weights"])
                                                biases:((NSData *)weights[@"transformer_conv"][@"biases"])
                                                 label:descriptor.conv.label
                                         updateWeights:descriptor.conv.updateWeights
                                                device:dev
                                              cmdQueue:cmdQ];

    _instNorm = [MPSCNNInstanceNormalizationNode createInstanceNormalization:[_conv resultImage]
                                                                    channels:descriptor.inst.channels
                                                                      styles:descriptor.inst.styles
                                                                       gamma:(NSData *)weights[@"transformer_inst"][@"weights"]
                                                                        beta:(NSData *)weights[@"transformer_inst"][@"biases"]
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

- (void)setLearningRate:(float)lr {
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
  [[_conv weights] setLearningRate:lr];
  [[_instNorm weights] setLearningRate:lr];
}

- (NSDictionary<NSString *, NSData *> *)exportWeights {
  NSMutableDictionary<NSString *, NSData *> *weights;

  NSDictionary<NSString *, NSData *> * encode1Weights = [_encoding1 exportWeights];

  weights[@"transformer_conv0_weight"] = encode1Weights[@"conv_weights"];
  weights[@"transformer_instancenorm0_gamma"] = encode1Weights[@"inst_gamma"];
  weights[@"transformer_instancenorm0_beta"] = encode1Weights[@"inst_beta"];

  NSDictionary<NSString *, NSData *> * encode2Weights = [_encoding2 exportWeights];

  weights[@"transformer_conv1_weight"] = encode2Weights[@"conv_weights"];
  weights[@"transformer_instancenorm1_gamma"] = encode2Weights[@"inst_gamma"];
  weights[@"transformer_instancenorm1_beta"] = encode2Weights[@"inst_beta"];

  NSDictionary<NSString *, NSData *> * encode3Weights = [_encoding3 exportWeights];

  weights[@"transformer_conv2_weight"] = encode3Weights[@"conv_weights"];
  weights[@"transformer_instancenorm2_gamma"] = encode3Weights[@"inst_gamma"];
  weights[@"transformer_instancenorm2_beta"] = encode3Weights[@"inst_beta"];

  NSDictionary<NSString *, NSData *> * residual1Weights = [_residual1 exportWeights];

  weights[@"transformer_residualblock0_conv0_weight"] = residual1Weights[@"conv_1_weights"];
  weights[@"transformer_residualblock0_instancenorm0_gamma"] = residual1Weights[@"inst_1_gamma"];
  weights[@"transformer_residualblock0_instancenorm0_beta"] = residual1Weights[@"inst_1_beta"];
  weights[@"transformer_residualblock0_conv1_weight"] = residual1Weights[@"conv_2_weights"];
  weights[@"transformer_residualblock0_instancenorm1_gamma"] = residual1Weights[@"inst_2_gamma"];
  weights[@"transformer_residualblock0_instancenorm1_beta"] = residual1Weights[@"inst_2_beta"];

  NSDictionary<NSString *, NSData *> * residual2Weights = [_residual2 exportWeights];
  
  weights[@"transformer_residualblock1_conv0_weight"] = residual2Weights[@"conv_1_weights"];
  weights[@"transformer_residualblock1_instancenorm0_gamma"] = residual2Weights[@"inst_1_gamma"];
  weights[@"transformer_residualblock1_instancenorm0_beta"] = residual2Weights[@"inst_1_beta"];
  weights[@"transformer_residualblock1_conv1_weight"] = residual2Weights[@"conv_2_weights"];
  weights[@"transformer_residualblock1_instancenorm1_gamma"] = residual2Weights[@"inst_2_gamma"];
  weights[@"transformer_residualblock1_instancenorm1_beta"] = residual2Weights[@"inst_2_beta"];

  NSDictionary<NSString *, NSData *> * residual3Weights = [_residual3 exportWeights];
  
  weights[@"transformer_residualblock2_conv0_weight"] = residual3Weights[@"conv_1_weights"];
  weights[@"transformer_residualblock2_instancenorm0_gamma"] = residual3Weights[@"inst_1_gamma"];
  weights[@"transformer_residualblock2_instancenorm0_beta"] = residual3Weights[@"inst_1_beta"];
  weights[@"transformer_residualblock2_conv1_weight"] = residual3Weights[@"conv_2_weights"];
  weights[@"transformer_residualblock2_instancenorm1_gamma"] = residual3Weights[@"inst_2_gamma"];
  weights[@"transformer_residualblock2_instancenorm1_beta"] = residual3Weights[@"inst_2_beta"];

  NSDictionary<NSString *, NSData *> * residual4Weights = [_residual4 exportWeights];
  
  weights[@"transformer_residualblock3_conv0_weight"] = residual4Weights[@"conv_1_weights"];
  weights[@"transformer_residualblock3_instancenorm0_gamma"] = residual4Weights[@"inst_1_gamma"];
  weights[@"transformer_residualblock3_instancenorm0_beta"] = residual4Weights[@"inst_1_beta"];
  weights[@"transformer_residualblock3_conv1_weight"] = residual4Weights[@"conv_2_weights"];
  weights[@"transformer_residualblock3_instancenorm1_gamma"] = residual4Weights[@"inst_2_gamma"];
  weights[@"transformer_residualblock3_instancenorm1_beta"] = residual4Weights[@"inst_2_beta"];

  NSDictionary<NSString *, NSData *> * residual5Weights = [_residual5 exportWeights];
  
  weights[@"transformer_residualblock4_conv0_weight"] = residual5Weights[@"conv_1_weights"];
  weights[@"transformer_residualblock4_instancenorm0_gamma"] = residual5Weights[@"inst_1_gamma"];
  weights[@"transformer_residualblock4_instancenorm0_beta"] = residual5Weights[@"inst_1_beta"];
  weights[@"transformer_residualblock4_conv1_weight"] = residual5Weights[@"conv_2_weights"];
  weights[@"transformer_residualblock4_instancenorm1_gamma"] = residual5Weights[@"inst_2_gamma"];
  weights[@"transformer_residualblock4_instancenorm1_beta"] = residual5Weights[@"inst_2_beta"];

  NSDictionary<NSString *, NSData *> * decode1Weights = [_decoding1 exportWeights];

  weights[@"transformer_conv3_weight"] = decode1Weights[@"conv_weights"];
  weights[@"transformer_instancenorm3_gamma"] = decode1Weights[@"inst_gamma"];
  weights[@"transformer_instancenorm3_beta"] = decode1Weights[@"inst_beta"];

  NSDictionary<NSString *, NSData *> * decode2Weights = [_decoding2 exportWeights];

  weights[@"transformer_conv4_weight"] = decode2Weights[@"conv_weights"];
  weights[@"transformer_instancenorm4_gamma"] = decode2Weights[@"inst_gamma"];
  weights[@"transformer_instancenorm4_beta"] = decode2Weights[@"inst_beta"];

  NSUInteger convWeightSize = (NSUInteger)([[_conv weights] weight_size] * sizeof(float));

  NSMutableData* convDataWeight = [NSMutableData dataWithCapacity:convWeightSize];

  memcpy(convDataWeight.mutableBytes, [[_conv weights] weights], convWeightSize);

  weights[@"transformer_conv5_weight"] = convDataWeight;

  NSUInteger instNormSize = (NSUInteger)([[_instNorm weights] numberOfFeatureChannels] * sizeof(float));

  NSMutableData* instNormDataGamma = [NSMutableData dataWithCapacity:instNormSize];
  NSMutableData* instNormDataBeta = [NSMutableData dataWithCapacity:instNormSize];

  memcpy(instNormDataGamma.mutableBytes, [[_instNorm weights] gamma], instNormSize);
  memcpy(instNormDataBeta.mutableBytes, [[_instNorm weights] beta], instNormSize);

  weights[@"transformer_instancenorm5_gamma"] = instNormDataGamma;
  weights[@"transformer_instancenorm5_beta"] = instNormDataBeta;

  return [weights copy];
}

@end
/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <ml/neural_net/style_transfer/mps_style_transfer_utils.h>

/**
 * Transformer Descriptor
 *    Encode 1
 *    Encode 2
 *    Encode 3
 *
 *    Residual 1
 *    Residual 2
 *    Residual 3
 *    Residual 4
 *    Residual 5
 *
 *    Decode 1
 *    Decode 2
 */
@implementation TCMPSStyleTransfer (TCMPSStyleTransferUtils)

+ (TCMPSTransformerDescriptor *) defineTransformerDescriptor:(NSUInteger)numStyles
                                              tuneAllWeights:(BOOL)tuneAllWeights {
  TCMPSTransformerDescriptor* transformerDesc = [[TCMPSTransformerDescriptor alloc] init];

  // Encoding 1 Convolution

  transformerDesc.encode1.conv.kernelWidth = 9;
  transformerDesc.encode1.conv.kernelHeight = 9;
  transformerDesc.encode1.conv.inputFeatureChannels = 3;
  transformerDesc.encode1.conv.outputFeatureChannels = 32;
  transformerDesc.encode1.conv.strideWidth = 1;
  transformerDesc.encode1.conv.strideHeight = 1;
  transformerDesc.encode1.conv.paddingWidth = 4;
  transformerDesc.encode1.conv.paddingHeight = 4;
  transformerDesc.encode1.conv.label = @"transformer_encode_1_conv";
  transformerDesc.encode1.conv.updateWeights = tuneAllWeights;

  // Encoding 1 Instance Normalization

  transformerDesc.encode1.inst.channels = 32;
  transformerDesc.encode1.inst.styles = numStyles;
  transformerDesc.encode1.inst.label = @"transformer_encode_1_inst";

  // Encoding 2 Convolution

  transformerDesc.encode2.conv.kernelWidth = 3;
  transformerDesc.encode2.conv.kernelHeight = 3;
  transformerDesc.encode2.conv.inputFeatureChannels = 32;
  transformerDesc.encode2.conv.outputFeatureChannels = 64;
  transformerDesc.encode2.conv.strideWidth = 2;
  transformerDesc.encode2.conv.strideHeight = 2;
  transformerDesc.encode2.conv.paddingWidth = 1;
  transformerDesc.encode2.conv.paddingHeight = 1;
  transformerDesc.encode2.conv.label = @"transformer_encode_2_conv";
  transformerDesc.encode2.conv.updateWeights = tuneAllWeights;

  // Encoding 2 Instance Normalization

  transformerDesc.encode2.inst.channels = 64;
  transformerDesc.encode2.inst.styles = numStyles;
  transformerDesc.encode2.inst.label = @"transformer_encode_2_inst";
  
  // Encoding 3 Convolution

  transformerDesc.encode3.conv.kernelWidth = 3;
  transformerDesc.encode3.conv.kernelHeight = 3;
  transformerDesc.encode3.conv.inputFeatureChannels = 64;
  transformerDesc.encode3.conv.outputFeatureChannels = 128;
  transformerDesc.encode3.conv.strideWidth = 2;
  transformerDesc.encode3.conv.strideHeight = 2;
  transformerDesc.encode3.conv.paddingWidth = 1;
  transformerDesc.encode3.conv.paddingHeight = 1;
  transformerDesc.encode3.conv.label = @"transformer_encode_3_conv";
  transformerDesc.encode3.conv.updateWeights = tuneAllWeights;

  // Encoding 3 Instance Normalization

  transformerDesc.encode2.inst.channels = 128;
  transformerDesc.encode2.inst.styles = numStyles;
  transformerDesc.encode2.inst.label = @"transformer_encode_3_inst";

  // Residual 1 Convolution 1

  transformerDesc.residual1.conv1.kernelWidth = 3;
  transformerDesc.residual1.conv1.kernelHeight = 3;
  transformerDesc.residual1.conv1.inputFeatureChannels = 128;
  transformerDesc.residual1.conv1.outputFeatureChannels = 128;
  transformerDesc.residual1.conv1.strideWidth = 1;
  transformerDesc.residual1.conv1.strideHeight = 1;
  transformerDesc.residual1.conv1.paddingWidth = 1;
  transformerDesc.residual1.conv1.paddingHeight = 1;
  transformerDesc.residual1.conv1.label = @"transformer_residual_1_conv_1";
  transformerDesc.residual1.conv1.updateWeights = tuneAllWeights;

  // Residual 1 Convolution 2

  transformerDesc.residual1.conv2.kernelWidth = 3;
  transformerDesc.residual1.conv2.kernelHeight = 3;
  transformerDesc.residual1.conv2.inputFeatureChannels = 128;
  transformerDesc.residual1.conv2.outputFeatureChannels = 128;
  transformerDesc.residual1.conv2.strideWidth = 1;
  transformerDesc.residual1.conv2.strideHeight = 1;
  transformerDesc.residual1.conv2.paddingWidth = 1;
  transformerDesc.residual1.conv2.paddingHeight = 1;
  transformerDesc.residual1.conv2.label = @"transformer_residual_1_conv_2";
  transformerDesc.residual1.conv2.updateWeights = tuneAllWeights;

  // Residual 1 Instance Normalization 1

  transformerDesc.residual1.inst1.channels = 128;
  transformerDesc.residual1.inst1.styles = numStyles;
  transformerDesc.residual1.inst1.label = @"transformer_residual_1_inst_1";

  // Residual 1 Instance Normalization 2

  transformerDesc.residual1.inst2.channels = 128;
  transformerDesc.residual1.inst2.styles = numStyles;
  transformerDesc.residual1.inst2.label = @"transformer_residual_1_inst_2";

  // Residual 2 Convolution 1

  transformerDesc.residual2.conv1.kernelWidth = 3;
  transformerDesc.residual2.conv1.kernelHeight = 3;
  transformerDesc.residual2.conv1.inputFeatureChannels = 128;
  transformerDesc.residual2.conv1.outputFeatureChannels = 128;
  transformerDesc.residual2.conv1.strideWidth = 1;
  transformerDesc.residual2.conv1.strideHeight = 1;
  transformerDesc.residual2.conv1.paddingWidth = 1;
  transformerDesc.residual2.conv1.paddingHeight = 1;
  transformerDesc.residual2.conv1.label = @"transformer_residual_2_conv_1";
  transformerDesc.residual2.conv1.updateWeights = tuneAllWeights;

  // Residual 2 Convolution 2

  transformerDesc.residual2.conv2.kernelWidth = 3;
  transformerDesc.residual2.conv2.kernelHeight = 3;
  transformerDesc.residual2.conv2.inputFeatureChannels = 128;
  transformerDesc.residual2.conv2.outputFeatureChannels = 128;
  transformerDesc.residual2.conv2.strideWidth = 1;
  transformerDesc.residual2.conv2.strideHeight = 1;
  transformerDesc.residual2.conv2.paddingWidth = 1;
  transformerDesc.residual2.conv2.paddingHeight = 1;
  transformerDesc.residual2.conv2.label = @"transformer_residual_2_conv_2";
  transformerDesc.residual2.conv2.updateWeights = tuneAllWeights;

  // Residual 2 Instance Normalization 1

  transformerDesc.residual2.inst1.channels = 128;
  transformerDesc.residual2.inst1.styles = numStyles;
  transformerDesc.residual2.inst1.label = @"transformer_residual_2_inst_1";

  // Residual 2 Instance Normalization 2

  transformerDesc.residual2.inst2.channels = 128;
  transformerDesc.residual2.inst2.styles = numStyles;
  transformerDesc.residual2.inst2.label = @"transformer_residual_2_inst_2";

  // Residual 3 Convolution 1

  transformerDesc.residual3.conv1.kernelWidth = 3;
  transformerDesc.residual3.conv1.kernelHeight = 3;
  transformerDesc.residual3.conv1.inputFeatureChannels = 128;
  transformerDesc.residual3.conv1.outputFeatureChannels = 128;
  transformerDesc.residual3.conv1.strideWidth = 1;
  transformerDesc.residual3.conv1.strideHeight = 1;
  transformerDesc.residual3.conv1.paddingWidth = 1;
  transformerDesc.residual3.conv1.paddingHeight = 1;
  transformerDesc.residual3.conv1.label = @"transformer_residual_3_conv_1";
  transformerDesc.residual3.conv1.updateWeights = tuneAllWeights;

  // Residual 3 Convolution 2

  transformerDesc.residual3.conv2.kernelWidth = 3;
  transformerDesc.residual3.conv2.kernelHeight = 3;
  transformerDesc.residual3.conv2.inputFeatureChannels = 128;
  transformerDesc.residual3.conv2.outputFeatureChannels = 128;
  transformerDesc.residual3.conv2.strideWidth = 1;
  transformerDesc.residual3.conv2.strideHeight = 1;
  transformerDesc.residual3.conv2.paddingWidth = 1;
  transformerDesc.residual3.conv2.paddingHeight = 1;
  transformerDesc.residual3.conv2.label = @"transformer_residual_3_conv_2";
  transformerDesc.residual3.conv2.updateWeights = tuneAllWeights;

  // Residual 3 Instance Normalization 1

  transformerDesc.residual3.inst1.channels = 128;
  transformerDesc.residual3.inst1.styles = numStyles;
  transformerDesc.residual3.inst1.label = @"transformer_residual_3_inst_1";

  // Residual 3 Instance Normalization 2

  transformerDesc.residual3.inst2.channels = 128;
  transformerDesc.residual3.inst2.styles = numStyles;
  transformerDesc.residual3.inst2.label = @"transformer_residual_3_inst_2";

  // Residual 4 Convolution 1

  transformerDesc.residual4.conv1.kernelWidth = 3;
  transformerDesc.residual4.conv1.kernelHeight = 3;
  transformerDesc.residual4.conv1.inputFeatureChannels = 128;
  transformerDesc.residual4.conv1.outputFeatureChannels = 128;
  transformerDesc.residual4.conv1.strideWidth = 1;
  transformerDesc.residual4.conv1.strideHeight = 1;
  transformerDesc.residual4.conv1.paddingWidth = 1;
  transformerDesc.residual4.conv1.paddingHeight = 1;
  transformerDesc.residual4.conv1.label = @"transformer_residual_4_conv_1";
  transformerDesc.residual4.conv1.updateWeights = tuneAllWeights;

  // Residual 4 Convolution 2

  transformerDesc.residual4.conv2.kernelWidth = 3;
  transformerDesc.residual4.conv2.kernelHeight = 3;
  transformerDesc.residual4.conv2.inputFeatureChannels = 128;
  transformerDesc.residual4.conv2.outputFeatureChannels = 128;
  transformerDesc.residual4.conv2.strideWidth = 1;
  transformerDesc.residual4.conv2.strideHeight = 1;
  transformerDesc.residual4.conv2.paddingWidth = 1;
  transformerDesc.residual4.conv2.paddingHeight = 1;
  transformerDesc.residual4.conv2.label = @"transformer_residual_4_conv_2";
  transformerDesc.residual4.conv2.updateWeights = tuneAllWeights;

  // Residual 4 Instance Normalization 1

  transformerDesc.residual4.inst1.channels = 128;
  transformerDesc.residual4.inst1.styles = numStyles;
  transformerDesc.residual4.inst1.label = @"transformer_residual_4_inst_1";

  // Residual 4 Instance Normalization 2

  transformerDesc.residual4.inst2.channels = 128;
  transformerDesc.residual4.inst2.styles = numStyles;
  transformerDesc.residual4.inst2.label = @"transformer_residual_4_inst_2";

  // Residual 5 Convolution 1

  transformerDesc.residual5.conv1.kernelWidth = 3;
  transformerDesc.residual5.conv1.kernelHeight = 3;
  transformerDesc.residual5.conv1.inputFeatureChannels = 128;
  transformerDesc.residual5.conv1.outputFeatureChannels = 128;
  transformerDesc.residual5.conv1.strideWidth = 1;
  transformerDesc.residual5.conv1.strideHeight = 1;
  transformerDesc.residual5.conv1.paddingWidth = 1;
  transformerDesc.residual5.conv1.paddingHeight = 1;
  transformerDesc.residual5.conv1.label = @"transformer_residual_5_conv_1";
  transformerDesc.residual5.conv1.updateWeights = tuneAllWeights;

  // Residual 5 Convolution 2

  transformerDesc.residual5.conv2.kernelWidth = 3;
  transformerDesc.residual5.conv2.kernelHeight = 3;
  transformerDesc.residual5.conv2.inputFeatureChannels = 128;
  transformerDesc.residual5.conv2.outputFeatureChannels = 128;
  transformerDesc.residual5.conv2.strideWidth = 1;
  transformerDesc.residual5.conv2.strideHeight = 1;
  transformerDesc.residual5.conv2.paddingWidth = 1;
  transformerDesc.residual5.conv2.paddingHeight = 1;
  transformerDesc.residual5.conv2.label = @"transformer_residual_5_conv_2";
  transformerDesc.residual5.conv2.updateWeights = tuneAllWeights;

  // Residual 5 Instance Normalization 1

  transformerDesc.residual5.inst1.channels = 128;
  transformerDesc.residual5.inst1.styles = numStyles;
  transformerDesc.residual5.inst1.label = @"transformer_residual_5_inst_1";

  // Residual 5 Instance Normalization 2

  transformerDesc.residual5.inst2.channels = 128;
  transformerDesc.residual5.inst2.styles = numStyles;
  transformerDesc.residual5.inst2.label = @"transformer_residual_5_inst_2";

  // Decode 1 Convolution

  transformerDesc.decode1.conv.kernelWidth = 3;
  transformerDesc.decode1.conv.kernelHeight = 3;
  transformerDesc.decode1.conv.inputFeatureChannels = 128;
  transformerDesc.decode1.conv.outputFeatureChannels = 64;
  transformerDesc.decode1.conv.strideWidth = 1;
  transformerDesc.decode1.conv.strideHeight = 1;
  transformerDesc.decode1.conv.paddingWidth = 1;
  transformerDesc.decode1.conv.paddingHeight = 1;
  transformerDesc.decode1.conv.label = @"transformer_decode_1_conv";
  transformerDesc.decode1.conv.updateWeights = tuneAllWeights;

  // Decode 1 Instance Normalization

  transformerDesc.decode1.inst.channels = 64;
  transformerDesc.decode1.inst.styles = numStyles;
  transformerDesc.decode1.inst.label = @"transformer_decode_1_inst";

  // Decode 1 Upsampling

  transformerDesc.decode1.upsample.scale = 2;

  // Decode 2 Convolution

  transformerDesc.decode2.conv.kernelWidth = 3;
  transformerDesc.decode2.conv.kernelHeight = 3;
  transformerDesc.decode2.conv.inputFeatureChannels = 64;
  transformerDesc.decode2.conv.outputFeatureChannels = 32;
  transformerDesc.decode2.conv.strideWidth = 1;
  transformerDesc.decode2.conv.strideHeight = 1;
  transformerDesc.decode2.conv.paddingWidth = 1;
  transformerDesc.decode2.conv.paddingHeight = 1;
  transformerDesc.decode2.conv.label = @"transformer_decode_2_conv";
  transformerDesc.decode2.conv.updateWeights = tuneAllWeights;

  // Decode 2 Instance Normalization

  transformerDesc.decode1.inst.channels = 32;
  transformerDesc.decode1.inst.styles = numStyles;
  transformerDesc.decode1.inst.label = @"transformer_decode_2_inst";

  // Decode 2 Upsampling

  transformerDesc.decode1.upsample.scale = 2;

  // Decode 3 Convolution

  transformerDesc.conv.kernelWidth = 9;
  transformerDesc.conv.kernelHeight = 9;
  transformerDesc.conv.inputFeatureChannels = 32;
  transformerDesc.conv.outputFeatureChannels = 3;
  transformerDesc.conv.strideWidth = 1;
  transformerDesc.conv.strideHeight = 1;
  transformerDesc.conv.paddingWidth = 1;
  transformerDesc.conv.paddingHeight = 1;
  transformerDesc.conv.label = @"transformer_decode_3_conv";
  transformerDesc.conv.updateWeights = tuneAllWeights;

  // Decode 3 Instance Normalization

  transformerDesc.decode1.inst.channels = 3;
  transformerDesc.decode1.inst.styles = numStyles;
  transformerDesc.decode1.inst.label = @"transformer_decode_3_inst";

  return transformerDesc;
}

/**
 *
 */
+ (TCMPSVgg16Descriptor *) defineVGG16Descriptor:(NSUInteger)numStyles {
  TCMPSVgg16Descriptor* vgg16Desc  = [[TCMPSVgg16Descriptor alloc] init];

  vgg16Desc.block1.conv1.kernelWidth = 3;
  vgg16Desc.block1.conv1.kernelHeight = 3;
  vgg16Desc.block1.conv1.inputFeatureChannels = 3;
  vgg16Desc.block1.conv1.outputFeatureChannels = 64;
  vgg16Desc.block1.conv1.strideWidth = 1;
  vgg16Desc.block1.conv1.strideHeight = 1;
  vgg16Desc.block1.conv1.paddingWidth = 1;
  vgg16Desc.block1.conv1.paddingHeight = 1;
  vgg16Desc.block1.conv1.label = @"vgg16_block_1_conv_1";
  vgg16Desc.block1.conv1.updateWeights = NO;

  vgg16Desc.block1.conv2.kernelWidth = 3;
  vgg16Desc.block1.conv2.kernelHeight = 3;
  vgg16Desc.block1.conv2.inputFeatureChannels = 64;
  vgg16Desc.block1.conv2.outputFeatureChannels = 64;
  vgg16Desc.block1.conv2.strideWidth = 1;
  vgg16Desc.block1.conv2.strideHeight = 1;
  vgg16Desc.block1.conv2.paddingWidth = 1;
  vgg16Desc.block1.conv2.paddingHeight = 1;
  vgg16Desc.block1.conv2.label = @"vgg16_block_1_conv_2";
  vgg16Desc.block1.conv2.updateWeights = NO;

  vgg16Desc.block1.pooling.kernelSize = 2;
  vgg16Desc.block1.pooling.strideSize = 2;

  vgg16Desc.block2.conv1.kernelWidth = 3;
  vgg16Desc.block2.conv1.kernelHeight = 3;
  vgg16Desc.block2.conv1.inputFeatureChannels = 64;
  vgg16Desc.block2.conv1.outputFeatureChannels = 128;
  vgg16Desc.block2.conv1.strideWidth = 1;
  vgg16Desc.block2.conv1.strideHeight = 1;
  vgg16Desc.block2.conv1.paddingWidth = 1;
  vgg16Desc.block2.conv1.paddingHeight = 1;
  vgg16Desc.block2.conv1.label = @"vgg16_block_2_conv_1";
  vgg16Desc.block2.conv1.updateWeights = NO;

  vgg16Desc.block2.conv2.kernelWidth = 3;
  vgg16Desc.block2.conv2.kernelHeight = 3;
  vgg16Desc.block2.conv2.inputFeatureChannels = 128;
  vgg16Desc.block2.conv2.outputFeatureChannels = 128;
  vgg16Desc.block2.conv2.strideWidth = 1;
  vgg16Desc.block2.conv2.strideHeight = 1;
  vgg16Desc.block2.conv2.paddingWidth = 1;
  vgg16Desc.block2.conv2.paddingHeight = 1;
  vgg16Desc.block2.conv2.label = @"vgg16_block_2_conv_2";
  vgg16Desc.block2.conv2.updateWeights = NO;

  vgg16Desc.block2.pooling.kernelSize = 2;
  vgg16Desc.block2.pooling.strideSize = 2;

  vgg16Desc.block3.conv1.kernelWidth = 3;
  vgg16Desc.block3.conv1.kernelHeight = 3;
  vgg16Desc.block3.conv1.inputFeatureChannels = 128;
  vgg16Desc.block3.conv1.outputFeatureChannels = 256;
  vgg16Desc.block3.conv1.strideWidth = 1;
  vgg16Desc.block3.conv1.strideHeight = 1;
  vgg16Desc.block3.conv1.paddingWidth = 1;
  vgg16Desc.block3.conv1.paddingHeight = 1;
  vgg16Desc.block3.conv1.label = @"vgg16_block_3_conv_1";
  vgg16Desc.block3.conv1.updateWeights = NO;

  vgg16Desc.block3.conv2.kernelWidth = 3;
  vgg16Desc.block3.conv2.kernelHeight = 3;
  vgg16Desc.block3.conv2.inputFeatureChannels = 256;
  vgg16Desc.block3.conv2.outputFeatureChannels = 256;
  vgg16Desc.block3.conv2.strideWidth = 1;
  vgg16Desc.block3.conv2.strideHeight = 1;
  vgg16Desc.block3.conv2.paddingWidth = 1;
  vgg16Desc.block3.conv2.paddingHeight = 1;
  vgg16Desc.block3.conv2.label = @"vgg16_block_3_conv_2";
  vgg16Desc.block3.conv2.updateWeights = NO;

  vgg16Desc.block3.conv3.kernelWidth = 3;
  vgg16Desc.block3.conv3.kernelHeight = 3;
  vgg16Desc.block3.conv3.inputFeatureChannels = 256;
  vgg16Desc.block3.conv3.outputFeatureChannels = 256;
  vgg16Desc.block3.conv3.strideWidth = 1;
  vgg16Desc.block3.conv3.strideHeight = 1;
  vgg16Desc.block3.conv3.paddingWidth = 1;
  vgg16Desc.block3.conv3.paddingHeight = 1;
  vgg16Desc.block3.conv3.label = @"vgg16_block_3_conv_3";
  vgg16Desc.block3.conv3.updateWeights = NO;

  vgg16Desc.block3.pooling.kernelSize = 2;
  vgg16Desc.block3.pooling.strideSize = 2;

  vgg16Desc.block4.conv1.kernelWidth = 3;
  vgg16Desc.block4.conv1.kernelHeight = 3;
  vgg16Desc.block4.conv1.inputFeatureChannels = 256;
  vgg16Desc.block4.conv1.outputFeatureChannels = 512;
  vgg16Desc.block4.conv1.strideWidth = 1;
  vgg16Desc.block4.conv1.strideHeight = 1;
  vgg16Desc.block4.conv1.paddingWidth = 1;
  vgg16Desc.block4.conv1.paddingHeight = 1;
  vgg16Desc.block4.conv1.label = @"vgg16_block_4_conv_1";
  vgg16Desc.block4.conv1.updateWeights = NO;

  vgg16Desc.block4.conv2.kernelWidth = 3;
  vgg16Desc.block4.conv2.kernelHeight = 3;
  vgg16Desc.block4.conv2.inputFeatureChannels = 256;
  vgg16Desc.block4.conv2.outputFeatureChannels = 512;
  vgg16Desc.block4.conv2.strideWidth = 1;
  vgg16Desc.block4.conv2.strideHeight = 1;
  vgg16Desc.block4.conv2.paddingWidth = 1;
  vgg16Desc.block4.conv2.paddingHeight = 1;
  vgg16Desc.block4.conv2.label = @"vgg16_block_4_conv_2";
  vgg16Desc.block4.conv2.updateWeights = NO;

  vgg16Desc.block4.conv3.kernelWidth = 3;
  vgg16Desc.block4.conv3.kernelHeight = 3;
  vgg16Desc.block4.conv3.inputFeatureChannels = 256;
  vgg16Desc.block4.conv3.outputFeatureChannels = 512;
  vgg16Desc.block4.conv3.strideWidth = 1;
  vgg16Desc.block4.conv3.strideHeight = 1;
  vgg16Desc.block4.conv3.paddingWidth = 1;
  vgg16Desc.block4.conv3.paddingHeight = 1;
  vgg16Desc.block4.conv3.label = @"vgg16_block_4_conv_3";
  vgg16Desc.block4.conv3.updateWeights = NO;

  vgg16Desc.block4.pooling.kernelSize = 2;
  vgg16Desc.block4.pooling.strideSize = 2;

  return vgg16Desc;
}

+ (NSDictionary<NSString *, NSDictionary *> *) defineTransformerWeights:(NSDictionary<NSString *, NSData *> *)weights {
  NSMutableDictionary<NSString *, NSDictionary *> *transformerWeight = [[NSMutableDictionary alloc] init];

  NSMutableDictionary<NSString *, NSData *> *transformerWeightEncode1 = [[NSMutableDictionary alloc] init];
  NSMutableDictionary<NSString *, NSData *> *transformerWeightEncode2 = [[NSMutableDictionary alloc] init];
  NSMutableDictionary<NSString *, NSData *> *transformerWeightEncode3 = [[NSMutableDictionary alloc] init];

  NSMutableDictionary<NSString *, NSData *> *transformerWeightResidual1 = [[NSMutableDictionary alloc] init];
  NSMutableDictionary<NSString *, NSData *> *transformerWeightResidual2 = [[NSMutableDictionary alloc] init];
  NSMutableDictionary<NSString *, NSData *> *transformerWeightResidual3 = [[NSMutableDictionary alloc] init];
  NSMutableDictionary<NSString *, NSData *> *transformerWeightResidual4 = [[NSMutableDictionary alloc] init];
  NSMutableDictionary<NSString *, NSData *> *transformerWeightResidual5 = [[NSMutableDictionary alloc] init];

  NSMutableDictionary<NSString *, NSData *> *transformerWeightDecode1 = [[NSMutableDictionary alloc] init];
  NSMutableDictionary<NSString *, NSData *> *transformerWeightDecode2 = [[NSMutableDictionary alloc] init];

  NSMutableDictionary<NSString *, NSData *> *transformerWeightConv = [[NSMutableDictionary alloc] init];
  NSMutableDictionary<NSString *, NSData *> *transformerWeightInst = [[NSMutableDictionary alloc] init];

  transformerWeightEncode1[@"transformer_encode_1encoding_conv_weights"] = weights[@"transformer_conv0_weight"];
  transformerWeightEncode1[@"transformer_encode_1encoding_conv_biases"] = NULL;
  transformerWeightEncode1[@"transformer_encode_1encoding_inst_gamma"] = weights[@"transformer_instancenorm0_gamma"];
  transformerWeightEncode1[@"transformer_encode_1encoding_inst_beta"] = weights[@"transformer_instancenorm0_beta"];

  transformerWeight[@"transformer_encode_1"] = [transformerWeightEncode1 copy];

  transformerWeightEncode2[@"transformer_encode_2encoding_conv_weights"] = weights[@"transformer_conv1_weight"];
  transformerWeightEncode2[@"transformer_encode_2encoding_conv_biases"] = NULL;
  transformerWeightEncode2[@"transformer_encode_2encoding_inst_gamma"] = weights[@"transformer_instancenorm1_gamma"];
  transformerWeightEncode2[@"transformer_encode_2encoding_inst_beta"] = weights[@"transformer_instancenorm1_beta"];

  transformerWeight[@"transformer_encode_2"] = [transformerWeightEncode2 copy];

  transformerWeightEncode3[@"transformer_encode_3encoding_conv_weights"] = weights[@"transformer_conv2_weight"];
  transformerWeightEncode3[@"transformer_encode_3encoding_conv_biases"] = NULL;
  transformerWeightEncode3[@"transformer_encode_3encoding_inst_gamma"] = weights[@"transformer_instancenorm2_gamma"];
  transformerWeightEncode3[@"transformer_encode_3encoding_inst_beta"] = weights[@"transformer_instancenorm2_beta"];

  transformerWeight[@"transformer_encode_3"] = [transformerWeightEncode3 copy];

  transformerWeightResidual1[@"transformer_residual_1residual_conv_1_weights"] = weights[@"transformer_residualblock0_conv0_weight"];
  transformerWeightResidual1[@"transformer_residual_1residual_conv_1_biases"] = NULL;
  transformerWeightResidual1[@"transformer_residual_1residual_inst_1_gamma"] = weights[@"transformer_residualblock0_instancenorm0_gamma"];
  transformerWeightResidual1[@"transformer_residual_1residual_inst_1_beta"] = weights[@"transformer_residualblock0_instancenorm0_beta"];
  transformerWeightResidual1[@"transformer_residual_1residual_conv_2_weights"] = weights[@"transformer_residualblock0_conv1_weight"];
  transformerWeightResidual1[@"transformer_residual_1residual_conv_2_biases"] = NULL;
  transformerWeightResidual1[@"transformer_residual_1residual_inst_2_gamma"] = weights[@"transformer_residualblock0_instancenorm1_gamma"];
  transformerWeightResidual1[@"transformer_residual_1residual_inst_2_beta"] = weights[@"transformer_residualblock0_instancenorm1_beta"];

  transformerWeight[@"transformer_residual_1"] = [transformerWeightResidual1 copy];

  transformerWeightResidual2[@"transformer_residual_2residual_conv_1_weights"] = weights[@"transformer_residualblock1_conv0_weight"];
  transformerWeightResidual2[@"transformer_residual_2residual_conv_1_biases"] = NULL;
  transformerWeightResidual2[@"transformer_residual_2residual_inst_1_gamma"] = weights[@"transformer_residualblock1_instancenorm0_gamma"];
  transformerWeightResidual2[@"transformer_residual_2residual_inst_1_beta"] = weights[@"transformer_residualblock1_instancenorm0_beta"];
  transformerWeightResidual2[@"transformer_residual_2residual_conv_2_weights"] = weights[@"transformer_residualblock1_conv1_weight"];
  transformerWeightResidual2[@"transformer_residual_2residual_conv_2_biases"] = NULL;
  transformerWeightResidual2[@"transformer_residual_2residual_inst_2_gamma"] = weights[@"transformer_residualblock1_instancenorm1_gamma"];
  transformerWeightResidual2[@"transformer_residual_2residual_inst_2_beta"] = weights[@"transformer_residualblock1_instancenorm1_beta"];

  transformerWeight[@"transformer_residual_2"] = [transformerWeightResidual2 copy];

  transformerWeightResidual3[@"transformer_residual_3residual_conv_1_weights"] = weights[@"transformer_residualblock2_conv0_weight"];
  transformerWeightResidual3[@"transformer_residual_3residual_conv_1_biases"] = NULL;
  transformerWeightResidual3[@"transformer_residual_3residual_inst_1_gamma"] = weights[@"transformer_residualblock2_instancenorm0_gamma"];
  transformerWeightResidual3[@"transformer_residual_3residual_inst_1_beta"] = weights[@"transformer_residualblock2_instancenorm0_beta"];
  transformerWeightResidual3[@"transformer_residual_3residual_conv_2_weights"] = weights[@"transformer_residualblock2_conv1_weight"];
  transformerWeightResidual3[@"transformer_residual_3residual_conv_2_biases"] = NULL;
  transformerWeightResidual3[@"transformer_residual_3residual_inst_2_gamma"] = weights[@"transformer_residualblock2_instancenorm1_gamma"];
  transformerWeightResidual3[@"transformer_residual_3residual_inst_2_beta"] = weights[@"transformer_residualblock2_instancenorm1_beta"];

  transformerWeight[@"transformer_residual_3"] = [transformerWeightResidual3 copy];

  transformerWeightResidual4[@"transformer_residual_4residual_conv_1_weights"] = weights[@"transformer_residualblock3_conv0_weight"];
  transformerWeightResidual4[@"transformer_residual_4residual_conv_1_biases"] = NULL;
  transformerWeightResidual4[@"transformer_residual_4residual_inst_1_gamma"] = weights[@"transformer_residualblock3_instancenorm0_gamma"];
  transformerWeightResidual4[@"transformer_residual_4residual_inst_1_beta"] = weights[@"transformer_residualblock3_instancenorm0_beta"];
  transformerWeightResidual4[@"transformer_residual_4residual_conv_2_weights"] = weights[@"transformer_residualblock3_conv1_weight"];
  transformerWeightResidual4[@"transformer_residual_4residual_conv_2_biases"] = NULL;
  transformerWeightResidual4[@"transformer_residual_4residual_inst_2_gamma"] = weights[@"transformer_residualblock3_instancenorm1_gamma"];
  transformerWeightResidual4[@"transformer_residual_4residual_inst_2_beta"] = weights[@"transformer_residualblock3_instancenorm1_beta"];

  transformerWeight[@"transformer_residual_4"] = [transformerWeightResidual4 copy];

  transformerWeightResidual5[@"transformer_residual_5residual_conv_1_weights"] = weights[@"transformer_residualblock4_conv0_weight"];
  transformerWeightResidual5[@"transformer_residual_5residual_conv_1_biases"] = NULL;
  transformerWeightResidual5[@"transformer_residual_5residual_inst_1_gamma"] = weights[@"transformer_residualblock4_instancenorm0_gamma"];
  transformerWeightResidual5[@"transformer_residual_5residual_inst_1_beta"] = weights[@"transformer_residualblock4_instancenorm0_beta"];
  transformerWeightResidual5[@"transformer_residual_5residual_conv_2_weights"] = weights[@"transformer_residualblock4_conv1_weight"];
  transformerWeightResidual5[@"transformer_residual_5residual_conv_2_biases"] = NULL;
  transformerWeightResidual5[@"transformer_residual_5residual_inst_2_gamma"] = weights[@"transformer_residualblock4_instancenorm1_gamma"];
  transformerWeightResidual5[@"transformer_residual_5residual_inst_2_beta"] = weights[@"transformer_residualblock4_instancenorm1_beta"];

  transformerWeight[@"transformer_residual_5"] = [transformerWeightResidual5 copy];

  transformerWeightDecode1[@"transformer_decoding_1decoding_conv_weights"] = weights[@"transformer_conv3_weight"];
  transformerWeightDecode1[@"transformer_decoding_1decoding_conv_biases"] = NULL;
  transformerWeightDecode1[@"transformer_decoding_1decoding_inst_gamma"] = weights[@"transformer_instancenorm3_gamma"];
  transformerWeightDecode1[@"transformer_decoding_1decoding_inst_beta"] = weights[@"transformer_instancenorm3_beta"];

  transformerWeight[@"transformer_decoding_1"] = [transformerWeightDecode1 copy];

  transformerWeightDecode1[@"transformer_decoding_2decoding_conv_weights"] = weights[@"transformer_conv4_weight"];
  transformerWeightDecode1[@"transformer_decoding_2decoding_conv_biases"] = NULL;
  transformerWeightDecode1[@"transformer_decoding_2decoding_inst_gamma"] = weights[@"transformer_instancenorm4_gamma"];
  transformerWeightDecode1[@"transformer_decoding_2decoding_inst_beta"] = weights[@"transformer_instancenorm4_beta"];

  transformerWeight[@"transformer_decoding_2"] = [transformerWeightDecode2 copy];

  transformerWeightConv[@"weights"] = weights[@"transformer_conv5_weight"];
  transformerWeightConv[@"biases"] = NULL;

  transformerWeightInst[@"weights"] = weights[@"transformer_instancenorm5_gamma"];
  transformerWeightInst[@"biases"] = weights[@"transformer_instancenorm5_beta"];

  transformerWeight[@"transformer_conv"] = [transformerWeightConv copy];
  transformerWeight[@"transformer_inst"] = [transformerWeightInst copy];

  return [transformerWeight copy];
}

+ (NSDictionary<NSString *, NSDictionary *> *)defineVGG16:(NSDictionary<NSString *, NSData *> *)weights {
  NSMutableDictionary<NSString *, NSDictionary *> *vgg16Weight = [[NSMutableDictionary alloc] init];

  NSMutableDictionary<NSString *, NSData *> *vgg16WeightBlock1 = [[NSMutableDictionary alloc] init];
  NSMutableDictionary<NSString *, NSData *> *vgg16WeightBlock2 = [[NSMutableDictionary alloc] init];
  NSMutableDictionary<NSString *, NSData *> *vgg16WeightBlock3 = [[NSMutableDictionary alloc] init];
  NSMutableDictionary<NSString *, NSData *> *vgg16WeightBlock4 = [[NSMutableDictionary alloc] init];

  vgg16WeightBlock1[@"vgg_block_1block_1_conv_1_weights"] = weights[@"vgg16_conv0_weight"];
  vgg16WeightBlock1[@"vgg_block_1block_1_conv_1_biases"] = weights[@"vgg16_conv0_bias"];
  vgg16WeightBlock1[@"vgg_block_1block_1_conv_2_weights"] = weights[@"vgg16_conv1_weight"];
  vgg16WeightBlock1[@"vgg_block_1block_1_conv_2_biases"] = weights[@"vgg16_conv1_bias"];

  vgg16Weight[@"vgg_block_1"] = [vgg16WeightBlock1 copy];

  vgg16WeightBlock2[@"vgg_block_2block_1_conv_1_weights"] = weights[@"vgg16_conv2_weight"];
  vgg16WeightBlock2[@"vgg_block_2block_1_conv_1_biases"] = weights[@"vgg16_conv2_bias"];
  vgg16WeightBlock2[@"vgg_block_2block_1_conv_2_weights"] = weights[@"vgg16_conv3_weight"];
  vgg16WeightBlock2[@"vgg_block_2block_1_conv_2_biases"] = weights[@"vgg16_conv3_bias"];

  vgg16Weight[@"vgg_block_2"] = [vgg16WeightBlock2 copy];

  vgg16WeightBlock3[@"vgg_block_3block_2_conv_1_weights"] = weights[@"vgg16_conv4_weight"];
  vgg16WeightBlock3[@"vgg_block_3block_2_conv_1_biases"] = weights[@"vgg16_conv4_bias"];
  vgg16WeightBlock3[@"vgg_block_3block_2_conv_2_weights"] = weights[@"vgg16_conv5_weight"];
  vgg16WeightBlock3[@"vgg_block_3block_2_conv_2_biases"] = weights[@"vgg16_conv5_bias"];
  vgg16WeightBlock3[@"vgg_block_3block_2_conv_2_weights"] = weights[@"vgg16_conv6_weight"];
  vgg16WeightBlock3[@"vgg_block_3block_2_conv_2_biases"] = weights[@"vgg16_conv6_bias"];

  vgg16Weight[@"vgg_block_3"] = [vgg16WeightBlock3 copy];

  vgg16WeightBlock4[@"vgg_block_4block_2_conv_1_weights"] = weights[@"vgg16_conv7_weight"];
  vgg16WeightBlock4[@"vgg_block_4block_2_conv_1_biases"] = weights[@"vgg16_conv7_bias"];
  vgg16WeightBlock4[@"vgg_block_4block_2_conv_2_weights"] = weights[@"vgg16_conv8_weight"];
  vgg16WeightBlock4[@"vgg_block_4block_2_conv_2_biases"] = weights[@"vgg16_conv8_bias"];
  vgg16WeightBlock4[@"vgg_block_4block_2_conv_2_weights"] = weights[@"vgg16_conv9_weight"];
  vgg16WeightBlock4[@"vgg_block_4block_2_conv_2_biases"] = weights[@"vgg16_conv9_bias"];

  vgg16Weight[@"vgg_block_4"] = [vgg16WeightBlock4 copy];

  return [vgg16Weight copy];
}

+ (void) populateMean:(NSMutableData *)data {
  NSUInteger dataSize = (data.length)/sizeof(float);
  NSAssert((dataSize) % 3 == 0, @"Data must follow a 3 channel format");
  float meanWeights[3] = { 123.68, 116.779, 103.939 };
  float* ptr = (float*) data.mutableBytes;
  float* end = ptr + dataSize;
  while (ptr < end) {
    ptr[0] = meanWeights[0];
    ptr[1] = meanWeights[1];
    ptr[2] = meanWeights[2];
    ptr += 3;
  }
}

+ (void) populateMultiplication:(NSMutableData *)data {
  NSUInteger dataSize = (data.length)/sizeof(float);
  NSAssert((dataSize) % 3 == 0, @"Data must follow a 3 channel format");
  for(NSUInteger x = 0; x < dataSize; x++)
    ((float *)data.mutableBytes)[x] = 255.0;
}

@end
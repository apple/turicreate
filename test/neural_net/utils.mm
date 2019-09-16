/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include "utils.hpp"

#include <cmath>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

using boost::lexical_cast;
using boost::property_tree::ptree;

namespace neural_net_test {
namespace style_transfer {

TCMPSEncodingDescriptor* define_encoding_descriptor(ptree config) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      TCMPSEncodingDescriptor *descriptor = [[TCMPSEncodingDescriptor alloc] init];

      descriptor.conv.kernelWidth           = config.get<NSUInteger>("conv.kernel_width");
      descriptor.conv.kernelHeight          = config.get<NSUInteger>("conv.kernel_height");
      descriptor.conv.inputFeatureChannels  = config.get<NSUInteger>("conv.input_feature_channels");
      descriptor.conv.outputFeatureChannels = config.get<NSUInteger>("conv.output_feature_channels");
      descriptor.conv.strideWidth           = config.get<NSUInteger>("conv.stride_width");
      descriptor.conv.strideHeight          = config.get<NSUInteger>("conv.stride_height");
      descriptor.conv.paddingWidth          = config.get<NSUInteger>("conv.padding_width");
      descriptor.conv.paddingHeight         = config.get<NSUInteger>("conv.padding_height");
      descriptor.conv.updateWeights         = config.get<bool>("conv.update_weights");
      descriptor.conv.label = @"encode_conv";

      descriptor.inst.channels = config.get<NSUInteger>("inst.channels");
      descriptor.inst.styles   = config.get<NSUInteger>("inst.styles");
      descriptor.inst.label = @"encode_inst";

      return descriptor;
    } else {
      throw "Need to be on MacOS 10.15 to use this function";
    }
  }
}

NSDictionary<NSString *, NSData *>* define_encoding_weights(ptree weights) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      NSMutableDictionary<NSString *, NSData *>* weights_dict = [[NSMutableDictionary alloc] init];

      NSMutableData * convWeights = [NSMutableData data];
      NSMutableData * instGamma = [NSMutableData data];
      NSMutableData * instBeta = [NSMutableData data];

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("encode_conv_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [convWeights appendBytes:&element length:sizeof(float)];
      }
      
      BOOST_FOREACH(const ptree::value_type &v, weights.get_child("encode_inst_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [instBeta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type &v, weights.get_child("encode_inst_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [instGamma appendBytes:&element length:sizeof(float)];
      }

      weights_dict[@"encode_conv_weights"] = convWeights;
      weights_dict[@"encode_inst_gamma"] = instGamma;
      weights_dict[@"encode_inst_beta"] = instBeta;

      return weights_dict;
    } else {
      throw "Need to be on MacOS 10.15 to use this function";
    }
  }
}

TCMPSResidualDescriptor* define_resiudal_descriptor(boost::property_tree::ptree config) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      TCMPSResidualDescriptor *descriptor = [[TCMPSResidualDescriptor alloc] init];

      descriptor.conv1.kernelWidth           = config.get<NSUInteger>("conv_1.kernel_width");
      descriptor.conv1.kernelHeight          = config.get<NSUInteger>("conv_1.kernel_height");
      descriptor.conv1.inputFeatureChannels  = config.get<NSUInteger>("conv_1.input_feature_channels");
      descriptor.conv1.outputFeatureChannels = config.get<NSUInteger>("conv_1.output_feature_channels");
      descriptor.conv1.strideWidth           = config.get<NSUInteger>("conv_1.stride_width");
      descriptor.conv1.strideHeight          = config.get<NSUInteger>("conv_1.stride_height");
      descriptor.conv1.paddingWidth          = config.get<NSUInteger>("conv_1.padding_width");
      descriptor.conv1.paddingHeight         = config.get<NSUInteger>("conv_1.padding_height");
      descriptor.conv1.updateWeights         = config.get<bool>("conv_1.update_weights");
      descriptor.conv1.label = @"residual_conv_1";

      descriptor.conv2.kernelWidth           = config.get<NSUInteger>("conv_2.kernel_width");
      descriptor.conv2.kernelHeight          = config.get<NSUInteger>("conv_2.kernel_height");
      descriptor.conv2.inputFeatureChannels  = config.get<NSUInteger>("conv_2.input_feature_channels");
      descriptor.conv2.outputFeatureChannels = config.get<NSUInteger>("conv_2.output_feature_channels");
      descriptor.conv2.strideWidth           = config.get<NSUInteger>("conv_2.stride_width");
      descriptor.conv2.strideHeight          = config.get<NSUInteger>("conv_2.stride_height");
      descriptor.conv2.paddingWidth          = config.get<NSUInteger>("conv_2.padding_width");
      descriptor.conv2.paddingHeight         = config.get<NSUInteger>("conv_2.padding_height");
      descriptor.conv2.updateWeights         = config.get<bool>("conv_2.update_weights");
      descriptor.conv2.label = @"residual_conv_2";

      descriptor.inst1.channels = config.get<NSUInteger>("inst_1.channels");
      descriptor.inst1.styles   = config.get<NSUInteger>("inst_1.styles");
      descriptor.inst1.label = @"transformer_residual_1_inst_1";

      descriptor.inst2.channels = config.get<NSUInteger>("inst_2.channels");
      descriptor.inst2.styles   = config.get<NSUInteger>("inst_2.styles");
      descriptor.inst2.label = @"transformer_residual_1_inst_2";

      return descriptor;
    } else {
      throw "Need to be on MacOS 10.15 to use this function";
    }
  }
}

NSDictionary<NSString *, NSData *>* define_residual_weights(boost::property_tree::ptree weights) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      NSMutableDictionary<NSString *, NSData *>* weights_dict = [[NSMutableDictionary alloc] init];
      
      NSMutableData * conv1Weights = [NSMutableData data];
      NSMutableData * conv2Weights = [NSMutableData data];

      NSMutableData * inst1Gamma = [NSMutableData data];
      NSMutableData * inst1Beta = [NSMutableData data];

      NSMutableData * inst2Gamma = [NSMutableData data];
      NSMutableData * inst2Beta = [NSMutableData data];

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("residual_conv_1_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [conv1Weights appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("residual_conv_2_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [conv2Weights appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type &v, weights.get_child("residual_inst_1_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [inst1Beta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type &v, weights.get_child("residual_inst_1_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [inst1Gamma appendBytes:&element length:sizeof(float)];
      }
      
      BOOST_FOREACH(const ptree::value_type &v, weights.get_child("residual_inst_2_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [inst2Beta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type &v, weights.get_child("residual_inst_2_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [inst2Gamma appendBytes:&element length:sizeof(float)];
      }

      weights_dict[@"residual_conv_1_weights"] = conv1Weights;
      weights_dict[@"residual_conv_2_weights"] = conv2Weights;
      weights_dict[@"residual_inst_1_gamma"] = inst1Gamma;
      weights_dict[@"residual_inst_1_beta"] = inst1Beta;
      weights_dict[@"residual_inst_2_gamma"] = inst2Gamma;
      weights_dict[@"residual_inst_2_beta"] = inst2Beta;

      return weights_dict;
    } else {
      throw "Need to be on MacOS 10.15 to use this function";
    }
  }
}

TCMPSDecodingDescriptor* define_decoding_descriptor(ptree config) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      TCMPSDecodingDescriptor *descriptor = [[TCMPSDecodingDescriptor alloc] init];
      
      descriptor.conv.kernelWidth           = config.get<NSUInteger>("conv.kernel_width");
      descriptor.conv.kernelHeight          = config.get<NSUInteger>("conv.kernel_height");
      descriptor.conv.inputFeatureChannels  = config.get<NSUInteger>("conv.input_feature_channels");
      descriptor.conv.outputFeatureChannels = config.get<NSUInteger>("conv.output_feature_channels");
      descriptor.conv.strideWidth           = config.get<NSUInteger>("conv.stride_width");
      descriptor.conv.strideHeight          = config.get<NSUInteger>("conv.stride_height");
      descriptor.conv.paddingWidth          = config.get<NSUInteger>("conv.padding_width");
      descriptor.conv.paddingHeight         = config.get<NSUInteger>("conv.padding_height");
      descriptor.conv.updateWeights         = config.get<bool>("conv.update_weights");
      descriptor.conv.label = @"decode_conv";

      descriptor.inst.channels = config.get<NSUInteger>("inst.channels");
      descriptor.inst.styles   = config.get<NSUInteger>("inst.styles");
      descriptor.inst.label = @"decode_inst";

      descriptor.upsample.scale = config.get<NSUInteger>("upsample.scale");

      return descriptor;
    } else {
      throw "Need to be on MacOS 10.15 to use this function";
    }
  }
}

NSDictionary<NSString *, NSData *>* define_decoding_weights(ptree weights) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      NSMutableDictionary<NSString *, NSData *>* weights_dict = [[NSMutableDictionary alloc] init];
      
      NSMutableData * convWeights = [NSMutableData data];
      NSMutableData * instGamma = [NSMutableData data];
      NSMutableData * instBeta = [NSMutableData data];

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("decode_conv_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [convWeights appendBytes:&element length:sizeof(float)];
      }
      
      BOOST_FOREACH(const ptree::value_type &v, weights.get_child("decode_inst_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [instBeta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type &v, weights.get_child("decode_inst_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [instGamma appendBytes:&element length:sizeof(float)];
      }

      weights_dict[@"decode_conv_weights"] = convWeights;
      weights_dict[@"decode_inst_gamma"] = instGamma;
      weights_dict[@"decode_inst_beta"] = instBeta;

      return weights_dict;
    } else {
      throw "Need to be on MacOS 10.15 to use this function";
    }
  }
}

TCMPSTransformerDescriptor* define_transformer_descriptor(ptree config) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      TCMPSTransformerDescriptor *descriptor = [[TCMPSTransformerDescriptor alloc] init];

      descriptor.encode1.conv.kernelWidth           = config.get<NSUInteger>("encode1.conv.kernel_width");
      descriptor.encode1.conv.kernelHeight          = config.get<NSUInteger>("encode1.conv.kernel_height");
      descriptor.encode1.conv.inputFeatureChannels  = config.get<NSUInteger>("encode1.conv.input_feature_channels");
      descriptor.encode1.conv.outputFeatureChannels = config.get<NSUInteger>("encode1.conv.output_feature_channels");
      descriptor.encode1.conv.strideWidth           = config.get<NSUInteger>("encode1.conv.stride_width");
      descriptor.encode1.conv.strideHeight          = config.get<NSUInteger>("encode1.conv.stride_height");
      descriptor.encode1.conv.paddingWidth          = config.get<NSUInteger>("encode1.conv.padding_width");
      descriptor.encode1.conv.paddingHeight         = config.get<NSUInteger>("encode1.conv.padding_height");
      descriptor.encode1.conv.updateWeights         = config.get<bool>("encode1.conv.update_weights");
      descriptor.encode1.conv.label = @"transformer_encode_1_conv";

      descriptor.encode1.inst.channels = config.get<NSUInteger>("encode1.inst.channels");
      descriptor.encode1.inst.styles   = config.get<NSUInteger>("encode1.inst.styles");
      descriptor.encode1.inst.label = @"transformer_encode_1_inst";

      descriptor.encode2.conv.kernelWidth           = config.get<NSUInteger>("encode2.conv.kernel_width");
      descriptor.encode2.conv.kernelHeight          = config.get<NSUInteger>("encode2.conv.kernel_height");
      descriptor.encode2.conv.inputFeatureChannels  = config.get<NSUInteger>("encode2.conv.input_feature_channels");
      descriptor.encode2.conv.outputFeatureChannels = config.get<NSUInteger>("encode2.conv.output_feature_channels");
      descriptor.encode2.conv.strideWidth           = config.get<NSUInteger>("encode2.conv.stride_width");
      descriptor.encode2.conv.strideHeight          = config.get<NSUInteger>("encode2.conv.stride_height");
      descriptor.encode2.conv.paddingWidth          = config.get<NSUInteger>("encode2.conv.padding_width");
      descriptor.encode2.conv.paddingHeight         = config.get<NSUInteger>("encode2.conv.padding_height");
      descriptor.encode2.conv.updateWeights         = config.get<bool>("encode2.conv.update_weights");
      descriptor.encode2.conv.label = @"transformer_encode_2_conv";

      descriptor.encode2.inst.channels = config.get<NSUInteger>("encode2.inst.channels");
      descriptor.encode2.inst.styles   = config.get<NSUInteger>("encode2.inst.styles");
      descriptor.encode2.inst.label = @"transformer_encode_2_inst";
      
      descriptor.encode3.conv.kernelWidth           = config.get<NSUInteger>("encode3.conv.kernel_width");
      descriptor.encode3.conv.kernelHeight          = config.get<NSUInteger>("encode3.conv.kernel_height");
      descriptor.encode3.conv.inputFeatureChannels  = config.get<NSUInteger>("encode3.conv.input_feature_channels");
      descriptor.encode3.conv.outputFeatureChannels = config.get<NSUInteger>("encode3.conv.output_feature_channels");
      descriptor.encode3.conv.strideWidth           = config.get<NSUInteger>("encode3.conv.stride_width");
      descriptor.encode3.conv.strideHeight          = config.get<NSUInteger>("encode3.conv.stride_height");
      descriptor.encode3.conv.paddingWidth          = config.get<NSUInteger>("encode3.conv.padding_width");
      descriptor.encode3.conv.paddingHeight         = config.get<NSUInteger>("encode3.conv.padding_height");
      descriptor.encode3.conv.updateWeights         = config.get<bool>("encode3.conv.update_weights");
      descriptor.encode3.conv.label = @"transformer_encode_3_conv";
      
      descriptor.encode3.inst.channels = config.get<NSUInteger>("encode3.inst.channels");
      descriptor.encode3.inst.styles   = config.get<NSUInteger>("encode3.inst.styles");
      descriptor.encode3.inst.label = @"transformer_encode_3_inst";

      descriptor.residual1.conv1.kernelWidth           = config.get<NSUInteger>("residual1.conv1.kernel_width");
      descriptor.residual1.conv1.kernelHeight          = config.get<NSUInteger>("residual1.conv1.kernel_height");
      descriptor.residual1.conv1.inputFeatureChannels  = config.get<NSUInteger>("residual1.conv1.input_feature_channels");
      descriptor.residual1.conv1.outputFeatureChannels = config.get<NSUInteger>("residual1.conv1.output_feature_channels");
      descriptor.residual1.conv1.strideWidth           = config.get<NSUInteger>("residual1.conv1.stride_width");
      descriptor.residual1.conv1.strideHeight          = config.get<NSUInteger>("residual1.conv1.stride_height");
      descriptor.residual1.conv1.paddingWidth          = config.get<NSUInteger>("residual1.conv1.padding_width");
      descriptor.residual1.conv1.paddingHeight         = config.get<NSUInteger>("residual1.conv1.padding_height");
      descriptor.residual1.conv1.updateWeights         = config.get<bool>("residual1.conv1.update_weights");
      descriptor.residual1.conv1.label = @"transformer_residual_1_conv_1";

      descriptor.residual1.conv2.kernelWidth           = config.get<NSUInteger>("residual1.conv2.kernel_width");
      descriptor.residual1.conv2.kernelHeight          = config.get<NSUInteger>("residual1.conv2.kernel_height");
      descriptor.residual1.conv2.inputFeatureChannels  = config.get<NSUInteger>("residual1.conv2.input_feature_channels");
      descriptor.residual1.conv2.outputFeatureChannels = config.get<NSUInteger>("residual1.conv2.output_feature_channels");
      descriptor.residual1.conv2.strideWidth           = config.get<NSUInteger>("residual1.conv2.stride_width");
      descriptor.residual1.conv2.strideHeight          = config.get<NSUInteger>("residual1.conv2.stride_height");
      descriptor.residual1.conv2.paddingWidth          = config.get<NSUInteger>("residual1.conv2.padding_width");
      descriptor.residual1.conv2.paddingHeight         = config.get<NSUInteger>("residual1.conv2.padding_height");
      descriptor.residual1.conv2.updateWeights         = config.get<bool>("residual1.conv2.update_weights");
      descriptor.residual1.conv2.label = @"transformer_residual_1_conv_2";

      descriptor.residual1.inst1.channels = config.get<NSUInteger>("residual1.inst1.channels");
      descriptor.residual1.inst1.styles   = config.get<NSUInteger>("residual1.inst1.styles");
      descriptor.residual1.inst1.label = @"transformer_residual_1_inst_1";

      descriptor.residual1.inst2.channels = config.get<NSUInteger>("residual1.inst2.channels");
      descriptor.residual1.inst2.styles   = config.get<NSUInteger>("residual1.inst2.styles");
      descriptor.residual1.inst2.label = @"transformer_residual_1_inst_2";

      descriptor.residual2.conv1.kernelWidth           = config.get<NSUInteger>("residual2.conv1.kernel_width");
      descriptor.residual2.conv1.kernelHeight          = config.get<NSUInteger>("residual2.conv1.kernel_height");
      descriptor.residual2.conv1.inputFeatureChannels  = config.get<NSUInteger>("residual2.conv1.input_feature_channels");
      descriptor.residual2.conv1.outputFeatureChannels = config.get<NSUInteger>("residual2.conv1.output_feature_channels");
      descriptor.residual2.conv1.strideWidth           = config.get<NSUInteger>("residual2.conv1.stride_width");
      descriptor.residual2.conv1.strideHeight          = config.get<NSUInteger>("residual2.conv1.stride_height");
      descriptor.residual2.conv1.paddingWidth          = config.get<NSUInteger>("residual2.conv1.padding_width");
      descriptor.residual2.conv1.paddingHeight         = config.get<NSUInteger>("residual2.conv1.padding_height");
      descriptor.residual2.conv1.updateWeights         = config.get<bool>("residual2.conv1.update_weights");
      descriptor.residual2.conv1.label = @"transformer_residual_2_conv_1";

      descriptor.residual2.conv2.kernelWidth           = config.get<NSUInteger>("residual2.conv2.kernel_width");
      descriptor.residual2.conv2.kernelHeight          = config.get<NSUInteger>("residual2.conv2.kernel_height");
      descriptor.residual2.conv2.inputFeatureChannels  = config.get<NSUInteger>("residual2.conv2.input_feature_channels");
      descriptor.residual2.conv2.outputFeatureChannels = config.get<NSUInteger>("residual2.conv2.output_feature_channels");
      descriptor.residual2.conv2.strideWidth           = config.get<NSUInteger>("residual2.conv2.stride_width");
      descriptor.residual2.conv2.strideHeight          = config.get<NSUInteger>("residual2.conv2.stride_height");
      descriptor.residual2.conv2.paddingWidth          = config.get<NSUInteger>("residual2.conv2.padding_width");
      descriptor.residual2.conv2.paddingHeight         = config.get<NSUInteger>("residual2.conv2.padding_height");
      descriptor.residual2.conv2.updateWeights         = config.get<bool>("residual2.conv2.update_weights");
      descriptor.residual2.conv2.label = @"transformer_residual_2_conv_2";

      descriptor.residual2.inst1.channels = config.get<NSUInteger>("residual2.inst1.channels");
      descriptor.residual2.inst1.styles   = config.get<NSUInteger>("residual2.inst1.styles");
      descriptor.residual2.inst1.label = @"transformer_residual_2_inst_1";

      descriptor.residual2.inst2.channels = config.get<NSUInteger>("residual2.inst2.channels");
      descriptor.residual2.inst2.styles   = config.get<NSUInteger>("residual2.inst2.styles");
      descriptor.residual2.inst2.label = @"transformer_residual_2_inst_2";

      descriptor.residual3.conv1.kernelWidth           = config.get<NSUInteger>("residual3.conv1.kernel_width");
      descriptor.residual3.conv1.kernelHeight          = config.get<NSUInteger>("residual3.conv1.kernel_height");
      descriptor.residual3.conv1.inputFeatureChannels  = config.get<NSUInteger>("residual3.conv1.input_feature_channels");
      descriptor.residual3.conv1.outputFeatureChannels = config.get<NSUInteger>("residual3.conv1.output_feature_channels");
      descriptor.residual3.conv1.strideWidth           = config.get<NSUInteger>("residual3.conv1.stride_width");
      descriptor.residual3.conv1.strideHeight          = config.get<NSUInteger>("residual3.conv1.stride_height");
      descriptor.residual3.conv1.paddingWidth          = config.get<NSUInteger>("residual3.conv1.padding_width");
      descriptor.residual3.conv1.paddingHeight         = config.get<NSUInteger>("residual3.conv1.padding_height");
      descriptor.residual3.conv1.updateWeights         = config.get<bool>("residual3.conv1.update_weights");
      descriptor.residual3.conv1.label = @"transformer_residual_3_conv_1";

      descriptor.residual3.conv2.kernelWidth           = config.get<NSUInteger>("residual3.conv2.kernel_width");
      descriptor.residual3.conv2.kernelHeight          = config.get<NSUInteger>("residual3.conv2.kernel_height");
      descriptor.residual3.conv2.inputFeatureChannels  = config.get<NSUInteger>("residual3.conv2.input_feature_channels");
      descriptor.residual3.conv2.outputFeatureChannels = config.get<NSUInteger>("residual3.conv2.output_feature_channels");
      descriptor.residual3.conv2.strideWidth           = config.get<NSUInteger>("residual3.conv2.stride_width");
      descriptor.residual3.conv2.strideHeight          = config.get<NSUInteger>("residual3.conv2.stride_height");
      descriptor.residual3.conv2.paddingWidth          = config.get<NSUInteger>("residual3.conv2.padding_width");
      descriptor.residual3.conv2.paddingHeight         = config.get<NSUInteger>("residual3.conv2.padding_height");
      descriptor.residual3.conv2.updateWeights         = config.get<bool>("residual3.conv2.update_weights");
      descriptor.residual3.conv2.label = @"transformer_residual_3_conv_2";

      descriptor.residual3.inst1.channels = config.get<NSUInteger>("residual3.inst1.channels");
      descriptor.residual3.inst1.styles   = config.get<NSUInteger>("residual3.inst1.styles");
      descriptor.residual3.inst1.label = @"transformer_residual_3_inst_1";

      descriptor.residual3.inst2.channels = config.get<NSUInteger>("residual3.inst2.channels");
      descriptor.residual3.inst2.styles   = config.get<NSUInteger>("residual3.inst2.styles");
      descriptor.residual3.inst2.label = @"transformer_residual_3_inst_2";

      descriptor.residual4.conv1.kernelWidth           = config.get<NSUInteger>("residual4.conv1.kernel_width");
      descriptor.residual4.conv1.kernelHeight          = config.get<NSUInteger>("residual4.conv1.kernel_height");
      descriptor.residual4.conv1.inputFeatureChannels  = config.get<NSUInteger>("residual4.conv1.input_feature_channels");
      descriptor.residual4.conv1.outputFeatureChannels = config.get<NSUInteger>("residual4.conv1.output_feature_channels");
      descriptor.residual4.conv1.strideWidth           = config.get<NSUInteger>("residual4.conv1.stride_width");
      descriptor.residual4.conv1.strideHeight          = config.get<NSUInteger>("residual4.conv1.stride_height");
      descriptor.residual4.conv1.paddingWidth          = config.get<NSUInteger>("residual4.conv1.padding_width");
      descriptor.residual4.conv1.paddingHeight         = config.get<NSUInteger>("residual4.conv1.padding_height");
      descriptor.residual4.conv1.updateWeights         = config.get<bool>("residual4.conv1.update_weights");
      descriptor.residual4.conv1.label = @"transformer_residual_4_conv_1";

      descriptor.residual4.conv2.kernelWidth           = config.get<NSUInteger>("residual4.conv2.kernel_width");
      descriptor.residual4.conv2.kernelHeight          = config.get<NSUInteger>("residual4.conv2.kernel_height");
      descriptor.residual4.conv2.inputFeatureChannels  = config.get<NSUInteger>("residual4.conv2.input_feature_channels");
      descriptor.residual4.conv2.outputFeatureChannels = config.get<NSUInteger>("residual4.conv2.output_feature_channels");
      descriptor.residual4.conv2.strideWidth           = config.get<NSUInteger>("residual4.conv2.stride_width");
      descriptor.residual4.conv2.strideHeight          = config.get<NSUInteger>("residual4.conv2.stride_height");
      descriptor.residual4.conv2.paddingWidth          = config.get<NSUInteger>("residual4.conv2.padding_width");
      descriptor.residual4.conv2.paddingHeight         = config.get<NSUInteger>("residual4.conv2.padding_height");
      descriptor.residual4.conv2.updateWeights         = config.get<bool>("residual4.conv2.update_weights");
      descriptor.residual4.conv2.label = @"transformer_residual_4_conv_2";

      descriptor.residual4.inst1.channels = config.get<NSUInteger>("residual4.inst1.channels");
      descriptor.residual4.inst1.styles   = config.get<NSUInteger>("residual4.inst1.styles");
      descriptor.residual4.inst1.label = @"transformer_residual_4_inst_1";

      descriptor.residual4.inst2.channels = config.get<NSUInteger>("residual4.inst2.channels");
      descriptor.residual4.inst2.styles   = config.get<NSUInteger>("residual4.inst2.styles");
      descriptor.residual4.inst2.label = @"transformer_residual_4_inst_2";

      descriptor.residual5.conv1.kernelWidth           = config.get<NSUInteger>("residual5.conv1.kernel_width");
      descriptor.residual5.conv1.kernelHeight          = config.get<NSUInteger>("residual5.conv1.kernel_height");
      descriptor.residual5.conv1.inputFeatureChannels  = config.get<NSUInteger>("residual5.conv1.input_feature_channels");
      descriptor.residual5.conv1.outputFeatureChannels = config.get<NSUInteger>("residual5.conv1.output_feature_channels");
      descriptor.residual5.conv1.strideWidth           = config.get<NSUInteger>("residual5.conv1.stride_width");
      descriptor.residual5.conv1.strideHeight          = config.get<NSUInteger>("residual5.conv1.stride_height");
      descriptor.residual5.conv1.paddingWidth          = config.get<NSUInteger>("residual5.conv1.padding_width");
      descriptor.residual5.conv1.paddingHeight         = config.get<NSUInteger>("residual5.conv1.padding_height");
      descriptor.residual5.conv1.updateWeights         = config.get<bool>("residual5.conv1.update_weights");
      descriptor.residual5.conv1.label = @"transformer_residual_5_conv_1";

      descriptor.residual5.conv2.kernelWidth           = config.get<NSUInteger>("residual5.conv2.kernel_width");
      descriptor.residual5.conv2.kernelHeight          = config.get<NSUInteger>("residual5.conv2.kernel_height");
      descriptor.residual5.conv2.inputFeatureChannels  = config.get<NSUInteger>("residual5.conv2.input_feature_channels");
      descriptor.residual5.conv2.outputFeatureChannels = config.get<NSUInteger>("residual5.conv2.output_feature_channels");
      descriptor.residual5.conv2.strideWidth           = config.get<NSUInteger>("residual5.conv2.stride_width");
      descriptor.residual5.conv2.strideHeight          = config.get<NSUInteger>("residual5.conv2.stride_height");
      descriptor.residual5.conv2.paddingWidth          = config.get<NSUInteger>("residual5.conv2.padding_width");
      descriptor.residual5.conv2.paddingHeight         = config.get<NSUInteger>("residual5.conv2.padding_height");
      descriptor.residual5.conv2.updateWeights         = config.get<bool>("residual5.conv2.update_weights");
      descriptor.residual5.conv2.label = @"transformer_residual_5_conv_2";

      descriptor.residual5.inst1.channels = config.get<NSUInteger>("residual5.inst1.channels");
      descriptor.residual5.inst1.styles   = config.get<NSUInteger>("residual5.inst1.styles");
      descriptor.residual5.inst1.label = @"transformer_residual_5_inst_1";

      descriptor.residual5.inst2.channels = config.get<NSUInteger>("residual5.inst2.channels");
      descriptor.residual5.inst2.styles   = config.get<NSUInteger>("residual5.inst2.styles");
      descriptor.residual5.inst2.label = @"transformer_residual_5_inst_2";
      
      descriptor.decode1.conv.kernelWidth           = config.get<NSUInteger>("decode1.conv.kernel_width");
      descriptor.decode1.conv.kernelHeight          = config.get<NSUInteger>("decode1.conv.kernel_height");
      descriptor.decode1.conv.inputFeatureChannels  = config.get<NSUInteger>("decode1.conv.input_feature_channels");
      descriptor.decode1.conv.outputFeatureChannels = config.get<NSUInteger>("decode1.conv.output_feature_channels");
      descriptor.decode1.conv.strideWidth           = config.get<NSUInteger>("decode1.conv.stride_width");
      descriptor.decode1.conv.strideHeight          = config.get<NSUInteger>("decode1.conv.stride_height");
      descriptor.decode1.conv.paddingWidth          = config.get<NSUInteger>("decode1.conv.padding_width");
      descriptor.decode1.conv.paddingHeight         = config.get<NSUInteger>("decode1.conv.padding_height");
      descriptor.decode1.conv.updateWeights         = config.get<bool>("decode1.conv.update_weights");
      descriptor.decode1.conv.label = @"transformer_decode_1_conv";

      descriptor.decode1.inst.channels = config.get<NSUInteger>("decode1.inst.channels");
      descriptor.decode1.inst.styles   = config.get<NSUInteger>("decode1.inst.styles");
      descriptor.decode1.inst.label = @"transformer_decode_1_inst";

      descriptor.decode1.upsample.scale = config.get<NSUInteger>("decode1.upsample.scale");

      descriptor.decode2.conv.kernelWidth           = config.get<NSUInteger>("decode2.conv.kernel_width");
      descriptor.decode2.conv.kernelHeight          = config.get<NSUInteger>("decode2.conv.kernel_height");
      descriptor.decode2.conv.inputFeatureChannels  = config.get<NSUInteger>("decode2.conv.input_feature_channels");
      descriptor.decode2.conv.outputFeatureChannels = config.get<NSUInteger>("decode2.conv.output_feature_channels");
      descriptor.decode2.conv.strideWidth           = config.get<NSUInteger>("decode2.conv.stride_width");
      descriptor.decode2.conv.strideHeight          = config.get<NSUInteger>("decode2.conv.stride_height");
      descriptor.decode2.conv.paddingWidth          = config.get<NSUInteger>("decode2.conv.padding_width");
      descriptor.decode2.conv.paddingHeight         = config.get<NSUInteger>("decode2.conv.padding_height");
      descriptor.decode2.conv.updateWeights         = config.get<bool>("decode2.conv.update_weights");
      descriptor.decode2.conv.label = @"transformer_decode_2_conv";

      descriptor.decode2.inst.channels = config.get<NSUInteger>("decode2.inst.channels");
      descriptor.decode2.inst.styles   = config.get<NSUInteger>("decode2.inst.styles");
      descriptor.decode2.inst.label = @"transformer_decode_2_inst";

      descriptor.decode2.upsample.scale = config.get<NSUInteger>("decode2.upsample.scale");

      descriptor.conv.kernelWidth           = config.get<NSUInteger>("conv.kernel_width");
      descriptor.conv.kernelHeight          = config.get<NSUInteger>("conv.kernel_height");
      descriptor.conv.inputFeatureChannels  = config.get<NSUInteger>("conv.input_feature_channels");
      descriptor.conv.outputFeatureChannels = config.get<NSUInteger>("conv.output_feature_channels");
      descriptor.conv.strideWidth           = config.get<NSUInteger>("conv.stride_width");
      descriptor.conv.strideHeight          = config.get<NSUInteger>("conv.stride_height");
      descriptor.conv.paddingWidth          = config.get<NSUInteger>("conv.padding_width");
      descriptor.conv.paddingHeight         = config.get<NSUInteger>("conv.padding_height");
      descriptor.conv.updateWeights         = config.get<bool>("conv.update_weights");
      descriptor.conv.label = @"transformer_decode_3_conv";

      descriptor.inst.channels = config.get<NSUInteger>("inst.channels");
      descriptor.inst.styles   = config.get<NSUInteger>("inst.styles");
      descriptor.inst.label = @"transformer_decode_3_inst";

      return descriptor;
    } else {
      throw "Need to be on MacOS 10.15 to use this function";
    }
  }
}

NSDictionary<NSString *, NSData *>* define_transformer_weights(ptree weights) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      NSMutableDictionary<NSString *, NSData *>* weights_dict = [[NSMutableDictionary alloc] init];
      
      NSMutableData * encode1Conv = [NSMutableData data];
      NSMutableData * encode1InstGamma = [NSMutableData data];
      NSMutableData * encode1InstBeta = [NSMutableData data];

      NSMutableData * encode2Conv = [NSMutableData data];
      NSMutableData * encode2InstGamma = [NSMutableData data];
      NSMutableData * encode2InstBeta = [NSMutableData data];

      NSMutableData * encode3Conv = [NSMutableData data];
      NSMutableData * encode3InstGamma = [NSMutableData data];
      NSMutableData * encode3InstBeta = [NSMutableData data];

      NSMutableData * residual1Conv1 = [NSMutableData data];
      NSMutableData * residual1Conv2 = [NSMutableData data];
      NSMutableData * residual1Inst1Gamma = [NSMutableData data];
      NSMutableData * residual1Inst1Beta = [NSMutableData data];
      NSMutableData * residual1Inst2Gamma = [NSMutableData data];
      NSMutableData * residual1Inst2Beta = [NSMutableData data];

      NSMutableData * residual2Conv1 = [NSMutableData data];
      NSMutableData * residual2Conv2 = [NSMutableData data];
      NSMutableData * residual2Inst1Gamma = [NSMutableData data];
      NSMutableData * residual2Inst1Beta = [NSMutableData data];
      NSMutableData * residual2Inst2Gamma = [NSMutableData data];
      NSMutableData * residual2Inst2Beta = [NSMutableData data];

      NSMutableData * residual3Conv1 = [NSMutableData data];
      NSMutableData * residual3Conv2 = [NSMutableData data];
      NSMutableData * residual3Inst1Gamma = [NSMutableData data];
      NSMutableData * residual3Inst1Beta = [NSMutableData data];
      NSMutableData * residual3Inst2Gamma = [NSMutableData data];
      NSMutableData * residual3Inst2Beta = [NSMutableData data];      

      NSMutableData * residual4Conv1 = [NSMutableData data];
      NSMutableData * residual4Conv2 = [NSMutableData data];
      NSMutableData * residual4Inst1Gamma = [NSMutableData data];
      NSMutableData * residual4Inst1Beta = [NSMutableData data];
      NSMutableData * residual4Inst2Gamma = [NSMutableData data];
      NSMutableData * residual4Inst2Beta = [NSMutableData data];

      NSMutableData * residual5Conv1 = [NSMutableData data];
      NSMutableData * residual5Conv2 = [NSMutableData data];
      NSMutableData * residual5Inst1Gamma = [NSMutableData data];
      NSMutableData * residual5Inst1Beta = [NSMutableData data];
      NSMutableData * residual5Inst2Gamma = [NSMutableData data];
      NSMutableData * residual5Inst2Beta = [NSMutableData data];

      NSMutableData * decode1Conv = [NSMutableData data];
      NSMutableData * decode1InstGamma = [NSMutableData data];
      NSMutableData * decode1InstBeta = [NSMutableData data];

      NSMutableData * decode2Conv = [NSMutableData data];
      NSMutableData * decode2InstGamma = [NSMutableData data];
      NSMutableData * decode2InstBeta = [NSMutableData data];

      NSMutableData * decode3Conv = [NSMutableData data];
      NSMutableData * decode3InstGamma = [NSMutableData data];
      NSMutableData * decode3InstBeta = [NSMutableData data];

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_encode_1_conv_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [encode1Conv appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_encode_1_inst_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [encode1InstGamma appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_encode_1_inst_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [encode1InstBeta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_encode_2_conv_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [encode2Conv appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_encode_2_inst_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [encode2InstGamma appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_encode_2_inst_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [encode2InstBeta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_encode_3_conv_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [encode3Conv appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_encode_3_inst_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [encode3InstGamma appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_encode_3_inst_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [encode3InstBeta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_1_conv_1_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [residual1Conv1 appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_1_conv_2_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [residual1Conv2 appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_1_inst_1_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [residual1Inst1Gamma appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_1_inst_1_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [residual1Inst1Beta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_1_inst_2_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [residual1Inst2Gamma appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_1_inst_2_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [residual1Inst2Beta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_2_conv_1_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [residual2Conv1 appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_2_conv_2_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [residual2Conv2 appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_2_inst_1_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [residual2Inst1Gamma appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_2_inst_1_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [residual2Inst1Beta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_2_inst_2_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [residual2Inst2Gamma appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_2_inst_2_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [residual2Inst2Beta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_3_conv_1_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [residual3Conv1 appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_3_conv_2_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [residual3Conv2 appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_3_inst_1_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [residual3Inst1Gamma appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_3_inst_1_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [residual3Inst1Beta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_3_inst_2_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [residual3Inst2Gamma appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_3_inst_2_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [residual3Inst2Beta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_4_conv_1_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [residual4Conv1 appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_4_conv_2_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [residual4Conv2 appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_4_inst_1_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [residual4Inst1Gamma appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_4_inst_1_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [residual4Inst1Beta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_4_inst_2_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [residual4Inst2Gamma appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_4_inst_2_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [residual4Inst2Beta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_5_conv_1_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [residual5Conv1 appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_5_conv_2_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [residual5Conv2 appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_5_inst_1_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [residual5Inst1Gamma appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_5_inst_1_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [residual5Inst1Beta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_5_inst_2_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [residual5Inst2Gamma appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_residual_5_inst_2_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [residual5Inst2Beta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_decoding_1_conv_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [decode1Conv appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_decoding_1_inst_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [decode1InstGamma appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_decoding_1_inst_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [decode1InstBeta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_decoding_2_conv_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [decode2Conv appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_decoding_2_inst_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [decode2InstGamma appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_decoding_2_inst_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [decode2InstBeta appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_conv5_weight")) {
        float element = lexical_cast<float>(v.second.data());
        [decode3Conv appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_instancenorm5_gamma")) {
        float element = lexical_cast<float>(v.second.data());
        [decode3InstGamma appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("transformer_instancenorm5_beta")) {
        float element = lexical_cast<float>(v.second.data());
        [decode3InstBeta appendBytes:&element length:sizeof(float)];
      }

      weights_dict[@"transformer_encode_1_conv_weights"] = encode1Conv;
      weights_dict[@"transformer_encode_1_inst_gamma"] = encode1InstGamma;
      weights_dict[@"transformer_encode_1_inst_beta"] = encode1InstBeta;
      
      weights_dict[@"transformer_encode_2_conv_weights"] = encode2Conv;
      weights_dict[@"transformer_encode_2_inst_gamma"] = encode2InstGamma;
      weights_dict[@"transformer_encode_2_inst_beta"] = encode2InstBeta;

      weights_dict[@"transformer_encode_3_conv_weights"] = encode3Conv;
      weights_dict[@"transformer_encode_3_inst_gamma"] = encode3InstGamma;
      weights_dict[@"transformer_encode_3_inst_beta"] = encode3InstBeta;
      
      weights_dict[@"transformer_residual_1_conv_1_weights"] = residual1Conv1;
      weights_dict[@"transformer_residual_1_conv_2_weights"] = residual1Conv2;
      weights_dict[@"transformer_residual_1_inst_1_gamma"] = residual1Inst1Gamma;
      weights_dict[@"transformer_residual_1_inst_1_beta"] = residual1Inst1Beta;
      weights_dict[@"transformer_residual_1_inst_2_gamma"] = residual1Inst2Gamma;
      weights_dict[@"transformer_residual_1_inst_2_beta"] = residual1Inst2Beta;
      
      weights_dict[@"transformer_residual_2_conv_1_weights"] = residual2Conv1;
      weights_dict[@"transformer_residual_2_conv_2_weights"] = residual2Conv2;
      weights_dict[@"transformer_residual_2_inst_1_gamma"] = residual2Inst1Gamma;
      weights_dict[@"transformer_residual_2_inst_1_beta"] = residual2Inst1Beta;
      weights_dict[@"transformer_residual_2_inst_2_gamma"] = residual2Inst2Gamma;
      weights_dict[@"transformer_residual_2_inst_2_beta"] = residual2Inst2Beta;

      weights_dict[@"transformer_residual_3_conv_1_weights"] = residual3Conv1;
      weights_dict[@"transformer_residual_3_conv_2_weights"] = residual3Conv2;
      weights_dict[@"transformer_residual_3_inst_1_gamma"] = residual3Inst1Gamma;
      weights_dict[@"transformer_residual_3_inst_1_beta"] = residual3Inst1Beta;
      weights_dict[@"transformer_residual_3_inst_2_gamma"] = residual3Inst2Gamma;
      weights_dict[@"transformer_residual_3_inst_2_beta"] = residual3Inst2Beta;
      
      weights_dict[@"transformer_residual_4_conv_1_weights"] = residual4Conv1;
      weights_dict[@"transformer_residual_4_conv_2_weights"] = residual4Conv2;
      weights_dict[@"transformer_residual_4_inst_1_gamma"] = residual4Inst1Gamma;
      weights_dict[@"transformer_residual_4_inst_1_beta"] = residual4Inst1Beta;
      weights_dict[@"transformer_residual_4_inst_2_gamma"] = residual4Inst2Gamma;
      weights_dict[@"transformer_residual_4_inst_2_beta"] = residual4Inst2Beta;
      
      weights_dict[@"transformer_residual_5_conv_1_weights"] = residual5Conv1;
      weights_dict[@"transformer_residual_5_conv_2_weights"] = residual5Conv2;
      weights_dict[@"transformer_residual_5_inst_1_gamma"] = residual5Inst1Gamma;
      weights_dict[@"transformer_residual_5_inst_1_beta"] = residual5Inst1Beta;
      weights_dict[@"transformer_residual_5_inst_2_gamma"] = residual5Inst2Gamma;
      weights_dict[@"transformer_residual_5_inst_2_beta"] = residual5Inst2Beta;
      
      weights_dict[@"transformer_decoding_1_conv_weights"] = decode1Conv;
      weights_dict[@"transformer_decoding_1_inst_gamma"] = decode1InstGamma;
      weights_dict[@"transformer_decoding_1_inst_beta"] = decode1InstBeta;

      weights_dict[@"transformer_decoding_2_conv_weights"] = decode2Conv;
      weights_dict[@"transformer_decoding_2_inst_gamma"] = decode2InstGamma;
      weights_dict[@"transformer_decoding_2_inst_beta"] = decode2InstBeta;
      
      weights_dict[@"transformer_conv5_weight"] = decode3Conv;
      weights_dict[@"transformer_instancenorm5_gamma"] = decode3InstGamma;
      weights_dict[@"transformer_instancenorm5_beta"] = decode3InstBeta;

      return weights_dict;
    } else {
      throw "Need to be on MacOS 10.15 to use this function";
    }
  }
}


TCMPSVgg16Block1Descriptor* define_block_1_descriptor(boost::property_tree::ptree config) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      TCMPSVgg16Block1Descriptor *descriptor = [[TCMPSVgg16Block1Descriptor alloc] init];

      descriptor.conv1.kernelWidth           = config.get<NSUInteger>("conv_1.kernel_width");
      descriptor.conv1.kernelHeight          = config.get<NSUInteger>("conv_1.kernel_height");
      descriptor.conv1.inputFeatureChannels  = config.get<NSUInteger>("conv_1.input_feature_channels");
      descriptor.conv1.outputFeatureChannels = config.get<NSUInteger>("conv_1.output_feature_channels");
      descriptor.conv1.strideWidth           = config.get<NSUInteger>("conv_1.stride_width");
      descriptor.conv1.strideHeight          = config.get<NSUInteger>("conv_1.stride_height");
      descriptor.conv1.paddingWidth          = config.get<NSUInteger>("conv_1.padding_width");
      descriptor.conv1.paddingHeight         = config.get<NSUInteger>("conv_1.padding_height");
      descriptor.conv1.updateWeights         = config.get<bool>("conv_1.update_weights");
      descriptor.conv1.label = @"vgg16_block_1_conv_1";

      descriptor.conv2.kernelWidth           = config.get<NSUInteger>("conv_2.kernel_width");
      descriptor.conv2.kernelHeight          = config.get<NSUInteger>("conv_2.kernel_height");
      descriptor.conv2.inputFeatureChannels  = config.get<NSUInteger>("conv_2.input_feature_channels");
      descriptor.conv2.outputFeatureChannels = config.get<NSUInteger>("conv_2.output_feature_channels");
      descriptor.conv2.strideWidth           = config.get<NSUInteger>("conv_2.stride_width");
      descriptor.conv2.strideHeight          = config.get<NSUInteger>("conv_2.stride_height");
      descriptor.conv2.paddingWidth          = config.get<NSUInteger>("conv_2.padding_width");
      descriptor.conv2.paddingHeight         = config.get<NSUInteger>("conv_2.padding_height");
      descriptor.conv2.updateWeights         = config.get<bool>("conv_2.update_weights");
      descriptor.conv2.label = @"vgg16_block_1_conv_2";
      
      descriptor.pooling.kernelSize = config.get<NSUInteger>("pooling.kernel");
      descriptor.pooling.strideSize = config.get<NSUInteger>("pooling.stride");

      return descriptor;
    } else {
      throw "Need to be on MacOS 10.15 to use this function";
    }
  }
}

NSDictionary<NSString *, NSData *>* define_block_1_weights(boost::property_tree::ptree weights) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      NSMutableDictionary<NSString *, NSData *>* weights_dict = [[NSMutableDictionary alloc] init];\

      NSMutableData * block1Conv1Weights = [NSMutableData data];
      NSMutableData * block1Conv1Bias = [NSMutableData data];

      NSMutableData * block1Conv2Weights = [NSMutableData data];
      NSMutableData * block1Conv2Bias = [NSMutableData data];

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("block1_conv_1_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [block1Conv1Weights appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("block1_conv_1_biases")) {
        float element = lexical_cast<float>(v.second.data());
        [block1Conv1Bias appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("block1_conv_2_weights")) {
        float element = lexical_cast<float>(v.second.data());
        [block1Conv2Weights appendBytes:&element length:sizeof(float)];
      }

      BOOST_FOREACH(const ptree::value_type v, weights.get_child("block1_conv_2_biases")) {
        float element = lexical_cast<float>(v.second.data());
        [block1Conv2Bias appendBytes:&element length:sizeof(float)];
      }

      weights_dict[@"block1_conv_1_weights"] = block1Conv1Weights;
      weights_dict[@"block1_conv_1_biases"] = block1Conv1Bias;

      weights_dict[@"block1_conv_2_weights"] = block1Conv2Weights;
      weights_dict[@"block1_conv_2_biases"] = block1Conv2Bias;

      return weights_dict;
    } else {
      throw "Need to be on MacOS 10.15 to use this function";
    }
  }
}

MPSImageBatch* define_input(ptree input, id <MTLDevice> dev) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      NSMutableData * inputArray = [NSMutableData data];

      NSUInteger imageWidth  = input.get<NSUInteger>("width");
      NSUInteger imageHeight = input.get<NSUInteger>("height");
      NSUInteger imageChannels = input.get<NSUInteger>("channels");

      BOOST_FOREACH(const ptree::value_type v, input.get_child("content")) {
        float element = lexical_cast<float>(v.second.data());
        [inputArray appendBytes:&element length:sizeof(float)];
      }

      MPSImageDescriptor *imgDesc = [MPSImageDescriptor
          imageDescriptorWithChannelFormat:MPSImageFeatureChannelFormatFloat32
                                     width:imageWidth
                                    height:imageHeight
                           featureChannels:imageChannels
                            numberOfImages:1
                                     usage:MTLTextureUsageShaderWrite | MTLTextureUsageShaderRead];

      NSMutableArray<MPSImage *> *imageBatch = [[NSMutableArray alloc] init];

      MPSImage *image = [[MPSImage alloc] initWithDevice:dev
                                         imageDescriptor:imgDesc];

      [image writeBytes:inputArray.bytes
             dataLayout:MPSDataLayoutHeightxWidthxFeatureChannels
             imageIndex:0];
      
      [imageBatch addObject:image];

      return imageBatch;
    } else {
      throw "Need to be on MacOS 10.15 to use this function";
    }
  }
}

NSData* define_output(ptree output) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      NSMutableData * outputArray = [NSMutableData data];

      BOOST_FOREACH(const ptree::value_type v, output.get_child("output")) {
        float element = lexical_cast<float>(v.second.data());
        [outputArray appendBytes:&element length:sizeof(float)];
      }
      
      return outputArray;
    } else {
      throw "Need to be on MacOS 10.15 to use this function";
    }
  }
}

bool check_data(NSData* expected, NSData* actual, float epsilon) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      float* float_expected = (float *) expected.bytes;
      float* float_actual = (float *) actual.bytes;

      for (NSUInteger x = 0; x < expected.length/sizeof(float) ; x++)
        if (std::abs(float_actual[x] - float_expected[x]) > epsilon)
          return false;

      return true;
    } else {
      throw "Need to be on MacOS 10.15 to use this function";
    }
  }
}

} // namespace style_transfer
} // namespace neural_net_test
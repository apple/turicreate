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
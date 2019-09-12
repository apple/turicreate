/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include "utils.hpp"

#include <cmath>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>

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
      descriptor.inst.styles = config.get<NSUInteger>("inst.styles");
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

MPSImageBatch* define_input(ptree input, id <MTLDevice> dev) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      NSMutableData * inputArray = [NSMutableData data];

      NSUInteger imageWidth  = input.get<NSUInteger>("width");
      NSUInteger imageHeight = input.get<NSUInteger>("height");

      BOOST_FOREACH(const ptree::value_type v, input.get_child("content")) {
        float element = lexical_cast<float>(v.second.data());
        [inputArray appendBytes:&element length:sizeof(float)];
      }

      MPSImageDescriptor *imgDesc = [MPSImageDescriptor
          imageDescriptorWithChannelFormat:MPSImageFeatureChannelFormatFloat32
                                     width:imageWidth
                                    height:imageHeight
                           featureChannels:3
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

      for (NSUInteger x = 0; x < expected.length/sizeof(float) ; x++) {
        if (std::abs(float_actual[x] - float_expected[x]) > epsilon) {
          std::cout << x << std::endl;
          std::cout << float_actual[x] << std::endl;
          std::cout << float_expected[x] << std::endl;
          return false;
        }
      }

      return true;
    } else {
      throw "Need to be on MacOS 10.15 to use this function";
    }
  }
}

} // namespace style_transfer
} // namespace neural_net_test
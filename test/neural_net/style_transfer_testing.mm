/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include "style_transfer_testing.hpp"
#import "style_transfer_testing_utils.hpp"

#import <ml/neural_net/mps_device_manager.h>
#import <ml/neural_net/mps_layer_helper.h>
#import <ml/neural_net/mps_node_handle.h>

namespace neural_net_test {
namespace style_transfer {

using boost::property_tree::ptree;

struct BaseNetworkTest::common_impl {
  API_AVAILABLE(macos(10.15)) id<MTLDevice> dev = nil;
  API_AVAILABLE(macos(10.15)) id<MTLCommandQueue> cmdQueue = nil;
  API_AVAILABLE(macos(10.15)) MPSNNImageNode *inputNode = nil;
  API_AVAILABLE(macos(10.15)) MPSNNGraph *model = nil;
};

BaseNetworkTest::BaseNetworkTest(float epsilon)
    : m_internal_impl(new BaseNetworkTest::common_impl()), m_epsilon(epsilon) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      m_internal_impl->dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
      m_internal_impl->cmdQueue = [m_internal_impl->dev newCommandQueue];

      TCMPSGraphNodeHandle *handle = [[TCMPSGraphNodeHandle alloc] initWithLabel:@"inputImage"];
      m_internal_impl->inputNode = [MPSNNImageNode nodeWithHandle:handle];
    }
  }
}

BaseNetworkTest::~BaseNetworkTest() = default;

bool BaseNetworkTest::check_predict(std::string input, std::string output) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      NSString *inputPath
          = [NSString stringWithCString:input.c_str() 
                               encoding:[NSString defaultCStringEncoding]];

      MPSImageBatch *imageBatch = [NeuralNetStyleTransferUtils defineInput:inputPath
                                                                       dev:m_internal_impl->dev];

      id<MTLCommandBuffer> cb = [m_internal_impl->cmdQueue commandBuffer];

      NSMutableArray *intermediateImages = [[NSMutableArray alloc] init];
      NSMutableArray *destinationStates = [[NSMutableArray alloc] init];

      MPSImageBatch *outputBatch =
          [m_internal_impl->model encodeBatchToCommandBuffer:cb
                                              sourceImages:@[ imageBatch ]
                                              sourceStates:nil
                                        intermediateImages:intermediateImages
                                         destinationStates:destinationStates];

      for (MPSImage *image in outputBatch) {
        [image synchronizeOnCommandBuffer:cb];
      }

      [cb commit];
      [cb waitUntilCompleted];

      NSString *outputPath
          = [NSString stringWithCString:output.c_str() 
                               encoding:[NSString defaultCStringEncoding]];

      NSData *correctOutput = [NeuralNetStyleTransferUtils defineOutput:outputPath];

      NSMutableData *dataOutput = [NSMutableData dataWithLength:correctOutput.length];

      [[outputBatch objectAtIndex:0] readBytes:dataOutput.mutableBytes
                                    dataLayout:MPSDataLayoutHeightxWidthxFeatureChannels
                                    imageIndex:0];

      

      return [NeuralNetStyleTransferUtils checkData:correctOutput
                                             actual:dataOutput
                                            epsilon:m_epsilon ];
    } else {
      throw "Need to be on MacOS 10.15 to use this method";
    }
  }
}

}  // namespace style_transfer
}  // namespace neural_net_test
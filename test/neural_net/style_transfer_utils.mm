/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include "style_transfer_utils.hpp"
#include "utils.hpp"

#import <ml/neural_net/mps_device_manager.h>
#import <ml/neural_net/mps_layer_helper.h>
#import <ml/neural_net/mps_node_handle.h>

#import <ml/neural_net/style_transfer/mps_style_transfer_decoding_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_encoding_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_residual_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_transformer_network.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_pre_processing.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_vgg_16_block_1_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_vgg_16_block_2_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_vgg_16_network.h>

#include <data/config.h>

#include <iostream>

using boost::property_tree::ptree;

namespace neural_net_test {
namespace style_transfer {

struct EncodingTest::impl {
  API_AVAILABLE(macos(10.15)) id <MTLDevice> dev = nil; 
  API_AVAILABLE(macos(10.15)) id <MTLCommandQueue> cmdQueue = nil;
  API_AVAILABLE(macos(10.15)) MPSNNImageNode* inputNode = nil;
  API_AVAILABLE(macos(10.15)) TCMPSStyleTransferEncodingNode *definition = nil;
  API_AVAILABLE(macos(10.15)) TCMPSEncodingDescriptor* descriptor = nil;
  API_AVAILABLE(macos(10.15)) MPSNNGraph* model = nil;
  API_AVAILABLE(macos(10.15)) NSDictionary<NSString *, NSData *> *weights = nil;
};

EncodingTest::EncodingTest(ptree config, ptree weights) : m_impl(new EncodingTest::impl()) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      m_impl->dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
      m_impl->cmdQueue = [m_impl->dev newCommandQueue];
      
      m_impl->inputNode = [MPSNNImageNode nodeWithHandle:[[TCMPSGraphNodeHandle alloc] initWithLabel:@"inputImage"]];

      m_impl->weights = define_encoding_weights(weights);
      m_impl->descriptor = define_encoding_descriptor(config);

      m_impl->definition = [[TCMPSStyleTransferEncodingNode alloc] initWithParameters:@"encode_"
                                                                            inputNode:m_impl->inputNode
                                                                               device:m_impl->dev
                                                                             cmdQueue:m_impl->cmdQueue
                                                                           descriptor:m_impl->descriptor
                                                                          initWeights:m_impl->weights];

      m_impl->model = [MPSNNGraph graphWithDevice:m_impl->dev
                                      resultImage:m_impl->definition.output
                              resultImageIsNeeded:YES];

      m_impl->model.format = MPSImageFeatureChannelFormatFloat32;
    }
  }
}

EncodingTest::~EncodingTest() = default;

bool EncodingTest::check_predict(ptree input, ptree output) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      MPSImageBatch *imageBatch = define_input(input, m_impl->dev);

      id<MTLCommandBuffer> cb = [m_impl->cmdQueue commandBuffer];

      NSMutableArray *intermediateImages = [[NSMutableArray alloc] init];
      NSMutableArray *destinationStates = [[NSMutableArray alloc] init];
      
      MPSImageBatch *outputBatch =  [m_impl->model encodeBatchToCommandBuffer:cb
                                                                 sourceImages:@[imageBatch]
                                                                 sourceStates:nil
                                                           intermediateImages:intermediateImages
                                                            destinationStates:destinationStates];

      for (MPSImage *image in outputBatch) {
        [image synchronizeOnCommandBuffer:cb];  
      }

      [cb commit];
      [cb waitUntilCompleted];

      NSData* correctOutput = define_output(output);
      NSMutableData* dataOutput = [NSMutableData dataWithLength:correctOutput.length];

      [[outputBatch objectAtIndex:0] readBytes:dataOutput.mutableBytes
                                    dataLayout:MPSDataLayoutHeightxWidthxFeatureChannels
                                    imageIndex:0];

      return check_data(correctOutput, dataOutput, 5e-3);
    } else {
      return true;
    }
  }
}

struct ResidualTest::impl {
  API_AVAILABLE(macos(10.15)) id <MTLDevice> dev = nil; 
  API_AVAILABLE(macos(10.15)) id <MTLCommandQueue> cmdQueue = nil;
  API_AVAILABLE(macos(10.15)) MPSNNImageNode* inputNode = nil;
  API_AVAILABLE(macos(10.15)) TCMPSStyleTransferResidualNode *definition = nil;
  API_AVAILABLE(macos(10.15)) TCMPSResidualDescriptor* descriptor = nil;
  API_AVAILABLE(macos(10.15)) MPSNNGraph* model = nil;
  API_AVAILABLE(macos(10.15)) NSDictionary<NSString *, NSData *> *weights = nil;
};

ResidualTest::ResidualTest(ptree config, ptree weights) : m_impl(new ResidualTest::impl()) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      m_impl->dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
      m_impl->cmdQueue = [m_impl->dev newCommandQueue];
      
      m_impl->inputNode = [MPSNNImageNode nodeWithHandle:[[TCMPSGraphNodeHandle alloc] initWithLabel:@"inputImage"]];

      m_impl->weights = define_residual_weights(weights);
      m_impl->descriptor = define_resiudal_descriptor(config);

      m_impl->definition = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"residual_"
                                                                            inputNode:m_impl->inputNode
                                                                               device:m_impl->dev
                                                                             cmdQueue:m_impl->cmdQueue
                                                                           descriptor:m_impl->descriptor
                                                                          initWeights:m_impl->weights];

      m_impl->model = [MPSNNGraph graphWithDevice:m_impl->dev
                                      resultImage:m_impl->definition.output
                              resultImageIsNeeded:YES];

      m_impl->model.format = MPSImageFeatureChannelFormatFloat32;
    }
  }
}

ResidualTest::~ResidualTest() = default;

bool ResidualTest::check_predict(ptree input, ptree output) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      MPSImageBatch *imageBatch = define_input(input, m_impl->dev);

      id<MTLCommandBuffer> cb = [m_impl->cmdQueue commandBuffer];

      NSMutableArray *intermediateImages = [[NSMutableArray alloc] init];
      NSMutableArray *destinationStates = [[NSMutableArray alloc] init];
      
      MPSImageBatch *outputBatch =  [m_impl->model encodeBatchToCommandBuffer:cb
                                                                 sourceImages:@[imageBatch]
                                                                 sourceStates:nil
                                                           intermediateImages:intermediateImages
                                                            destinationStates:destinationStates];

      for (MPSImage *image in outputBatch) {
        [image synchronizeOnCommandBuffer:cb];  
      }

      [cb commit];
      [cb waitUntilCompleted];

      NSData* correctOutput = define_output(output);
      NSMutableData* dataOutput = [NSMutableData dataWithLength:correctOutput.length];

      [[outputBatch objectAtIndex:0] readBytes:dataOutput.mutableBytes
                                    dataLayout:MPSDataLayoutHeightxWidthxFeatureChannels
                                    imageIndex:0];

      return check_data(correctOutput, dataOutput, 5e-3);
    } else {
      return true;
    }
  }
}

struct DecodingTest::impl {
  API_AVAILABLE(macos(10.15)) id <MTLDevice> dev = nil;
  API_AVAILABLE(macos(10.15)) id <MTLCommandQueue> cmdQueue = nil;
  API_AVAILABLE(macos(10.15)) MPSNNImageNode* inputNode = nil;
  API_AVAILABLE(macos(10.15)) TCMPSStyleTransferDecodingNode *definition = nil;
  API_AVAILABLE(macos(10.15)) TCMPSDecodingDescriptor* descriptor = nil;
  API_AVAILABLE(macos(10.15)) MPSNNGraph* model = nil;
  API_AVAILABLE(macos(10.15)) NSDictionary<NSString *, NSData *> *weights = nil;
};

DecodingTest::DecodingTest(ptree config, ptree weights) : m_impl(new DecodingTest::impl()) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      m_impl->dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
      m_impl->cmdQueue = [m_impl->dev newCommandQueue];
      
      m_impl->inputNode = [MPSNNImageNode nodeWithHandle:[[TCMPSGraphNodeHandle alloc] initWithLabel:@"inputImage"]];

      m_impl->weights = define_decoding_weights(weights);
      m_impl->descriptor = define_decoding_descriptor(config);

      m_impl->definition = [[TCMPSStyleTransferDecodingNode alloc] initWithParameters:@"decode_"
                                                                            inputNode:m_impl->inputNode
                                                                               device:m_impl->dev
                                                                             cmdQueue:m_impl->cmdQueue
                                                                           descriptor:m_impl->descriptor
                                                                          initWeights:m_impl->weights];

      m_impl->model = [MPSNNGraph graphWithDevice:m_impl->dev
                                      resultImage:m_impl->definition.output
                              resultImageIsNeeded:YES];

      m_impl->model.format = MPSImageFeatureChannelFormatFloat32;
    }
  }
}

DecodingTest::~DecodingTest() = default;


bool DecodingTest::check_predict(ptree input, ptree output) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      MPSImageBatch *imageBatch = define_input(input, m_impl->dev);

      id<MTLCommandBuffer> cb = [m_impl->cmdQueue commandBuffer];

      NSMutableArray *intermediateImages = [[NSMutableArray alloc] init];
      NSMutableArray *destinationStates = [[NSMutableArray alloc] init];
      
      MPSImageBatch *outputBatch =  [m_impl->model encodeBatchToCommandBuffer:cb
                                                                 sourceImages:@[imageBatch]
                                                                 sourceStates:nil
                                                           intermediateImages:intermediateImages
                                                            destinationStates:destinationStates];

      for (MPSImage *image in outputBatch) {
        [image synchronizeOnCommandBuffer:cb];  
      }

      [cb commit];
      [cb waitUntilCompleted];

      NSData* correctOutput = define_output(output);
      NSMutableData* dataOutput = [NSMutableData dataWithLength:correctOutput.length];

      [[outputBatch objectAtIndex:0] readBytes:dataOutput.mutableBytes
                                    dataLayout:MPSDataLayoutHeightxWidthxFeatureChannels
                                    imageIndex:0];

      return check_data(correctOutput, dataOutput, 5e-3);
    } else {
      return true;
    }
  }
}

struct ResnetTest::impl {
  API_AVAILABLE(macos(10.15)) id <MTLDevice> dev = nil;
  API_AVAILABLE(macos(10.15)) id <MTLCommandQueue> cmdQueue = nil;
  API_AVAILABLE(macos(10.15)) MPSNNImageNode* inputNode = nil;
  API_AVAILABLE(macos(10.15)) TCMPSStyleTransferTransformerNetwork *definition = nil;
  API_AVAILABLE(macos(10.15)) TCMPSTransformerDescriptor* descriptor = nil;
  API_AVAILABLE(macos(10.15)) MPSNNGraph* model = nil;
  API_AVAILABLE(macos(10.15)) NSDictionary<NSString *, NSData *> *weights = nil;
};

ResnetTest::ResnetTest(ptree config, ptree weights) : m_impl(new ResnetTest::impl()) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      m_impl->dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
      m_impl->cmdQueue = [m_impl->dev newCommandQueue];
      
      m_impl->inputNode = [MPSNNImageNode nodeWithHandle:[[TCMPSGraphNodeHandle alloc] initWithLabel:@"inputImage"]];

      m_impl->weights = define_transformer_weights(weights);
      m_impl->descriptor = define_transformer_descriptor(config);

      m_impl->definition = [[TCMPSStyleTransferTransformerNetwork alloc] initWithParameters:@"transformer_"
                                                                                  inputNode:m_impl->inputNode
                                                                                     device:m_impl->dev
                                                                                   cmdQueue:m_impl->cmdQueue
                                                                                 descriptor:m_impl->descriptor
                                                                                initWeights:m_impl->weights];

      m_impl->model = [MPSNNGraph graphWithDevice:m_impl->dev
                                      resultImage:m_impl->definition.forwardPass
                              resultImageIsNeeded:YES];

      m_impl->model.format = MPSImageFeatureChannelFormatFloat32;
    }
  }
}

ResnetTest::~ResnetTest() = default;

bool ResnetTest::check_predict(ptree input, ptree output) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      MPSImageBatch *imageBatch = define_input(input, m_impl->dev);

      id<MTLCommandBuffer> cb = [m_impl->cmdQueue commandBuffer];

      NSMutableArray *intermediateImages = [[NSMutableArray alloc] init];
      NSMutableArray *destinationStates = [[NSMutableArray alloc] init];
      
      MPSImageBatch *outputBatch =  [m_impl->model encodeBatchToCommandBuffer:cb
                                                                 sourceImages:@[imageBatch]
                                                                 sourceStates:nil
                                                           intermediateImages:intermediateImages
                                                            destinationStates:destinationStates];

      for (MPSImage *image in outputBatch) {
        [image synchronizeOnCommandBuffer:cb];  
      }

      [cb commit];
      [cb waitUntilCompleted];

      NSData* correctOutput = define_output(output);
      NSMutableData* dataOutput = [NSMutableData dataWithLength:correctOutput.length];

      [[outputBatch objectAtIndex:0] readBytes:dataOutput.mutableBytes
                                    dataLayout:MPSDataLayoutHeightxWidthxFeatureChannels
                                    imageIndex:0];

      return check_data(correctOutput, dataOutput, 5e-1);
    } else {
      return true;
    }
  }
}

struct Block1Test::impl {
  API_AVAILABLE(macos(10.15)) TCMPSVgg16Block1 *definition = nil;
};

Block1Test::Block1Test(ptree config) : m_impl(new Block1Test::impl())  {
  // TODO: load block 1 weights
}

Block1Test::~Block1Test() = default;

struct Block2Test::impl {
  API_AVAILABLE(macos(10.15)) TCMPSVgg16Block2 *definition = nil;
};

Block2Test::Block2Test(ptree config) : m_impl(new Block2Test::impl()) {
  // TODO: load block 2 weights
}

Block2Test::~Block2Test() = default;

struct Vgg16Test::impl {
  API_AVAILABLE(macos(10.15)) TCMPSVgg16Network *definition = nil;
};

Vgg16Test::Vgg16Test(ptree config) : m_impl(new Vgg16Test::impl()) {
  // TODO: load vgg16 weights
}

Vgg16Test::~Vgg16Test() = default;

struct LossTest::impl {
  API_AVAILABLE(macos(10.15)) TCMPSStyleTransferPreProcessing *contentPreProcess = nil;
  API_AVAILABLE(macos(10.15)) TCMPSStyleTransferPreProcessing *stylePreProcess = nil;
  API_AVAILABLE(macos(10.15)) TCMPSStyleTransferPreProcessing *transformerPreProcess = nil;
  API_AVAILABLE(macos(10.15)) TCMPSStyleTransferTransformerNetwork *transformer = nil;
  API_AVAILABLE(macos(10.15)) TCMPSVgg16Network *contentVGG = nil;
  API_AVAILABLE(macos(10.15)) TCMPSVgg16Network *styleVGG = nil;
  API_AVAILABLE(macos(10.15)) TCMPSVgg16Network *transformerVGG = nil;
};

LossTest::LossTest(ptree config) : m_impl(new LossTest::impl()) {
  // TODO: load transformer and vgg16 weights
}

LossTest::~LossTest() = default;

struct WeightUpdateTest::impl {
  API_AVAILABLE(macos(10.15)) TCMPSStyleTransferPreProcessing *contentPreProcess = nil;
  API_AVAILABLE(macos(10.15)) TCMPSStyleTransferPreProcessing *stylePreProcess = nil;
  API_AVAILABLE(macos(10.15)) TCMPSStyleTransferPreProcessing *transformerPreProcess = nil;
  API_AVAILABLE(macos(10.15)) TCMPSStyleTransferTransformerNetwork *transformer = nil;
  API_AVAILABLE(macos(10.15)) TCMPSVgg16Network *contentVGG = nil;
  API_AVAILABLE(macos(10.15)) TCMPSVgg16Network *styleVGG = nil;
  API_AVAILABLE(macos(10.15)) TCMPSVgg16Network *transformerVGG = nil;
};

WeightUpdateTest::WeightUpdateTest(ptree config) : m_impl(new WeightUpdateTest::impl()) {
  // TODO: load transformer and vgg16 weights
}

WeightUpdateTest::~WeightUpdateTest() = default;

} // namespace style_transfer
} // namespace neural_net_test
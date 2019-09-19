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

      return check_data(correctOutput, dataOutput, 5e-2);
    } else {
      return true;
    }
  }
}

struct Block1Test::impl {
  API_AVAILABLE(macos(10.15)) id <MTLDevice> dev = nil;
  API_AVAILABLE(macos(10.15)) id <MTLCommandQueue> cmdQueue = nil;
  API_AVAILABLE(macos(10.15)) MPSNNImageNode* inputNode = nil;
  API_AVAILABLE(macos(10.15)) TCMPSVgg16Block1 *definition = nil;
  API_AVAILABLE(macos(10.15)) TCMPSVgg16Block1Descriptor* descriptor = nil;
  API_AVAILABLE(macos(10.15)) MPSNNGraph* model = nil;
  API_AVAILABLE(macos(10.15)) NSDictionary<NSString *, NSData *> *weights = nil;
};

Block1Test::Block1Test(ptree config, ptree weights) : m_impl(new Block1Test::impl())  {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      m_impl->dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
      m_impl->cmdQueue = [m_impl->dev newCommandQueue];
      
      m_impl->inputNode = [MPSNNImageNode nodeWithHandle:[[TCMPSGraphNodeHandle alloc] initWithLabel:@"inputImage"]];

      m_impl->weights = define_block_1_weights(weights);
      m_impl->descriptor = define_block_1_descriptor(config);

      m_impl->definition = [[TCMPSVgg16Block1 alloc] initWithParameters:@"block1_"
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

Block1Test::~Block1Test() = default;

bool Block1Test::check_predict(ptree input, ptree output) {
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

struct Block2Test::impl {
  API_AVAILABLE(macos(10.15)) id <MTLDevice> dev = nil;
  API_AVAILABLE(macos(10.15)) id <MTLCommandQueue> cmdQueue = nil;
  API_AVAILABLE(macos(10.15)) MPSNNImageNode* inputNode = nil;
  API_AVAILABLE(macos(10.15)) TCMPSVgg16Block2 *definition = nil;
  API_AVAILABLE(macos(10.15)) TCMPSVgg16Block2Descriptor* descriptor = nil;
  API_AVAILABLE(macos(10.15)) MPSNNGraph* model = nil;
  API_AVAILABLE(macos(10.15)) NSDictionary<NSString *, NSData *> *weights = nil;
};

Block2Test::Block2Test(ptree config, ptree weights) : m_impl(new Block2Test::impl()) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      m_impl->dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
      m_impl->cmdQueue = [m_impl->dev newCommandQueue];
      
      m_impl->inputNode = [MPSNNImageNode nodeWithHandle:[[TCMPSGraphNodeHandle alloc] initWithLabel:@"inputImage"]];

      m_impl->weights = define_block_2_weights(weights);
      m_impl->descriptor = define_block_2_descriptor(config);

      m_impl->definition = [[TCMPSVgg16Block2 alloc] initWithParameters:@"block2_"
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

Block2Test::~Block2Test() = default;


bool Block2Test::check_predict(ptree input, ptree output) {
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

struct Vgg16Test::impl {
  API_AVAILABLE(macos(10.15)) id <MTLDevice> dev = nil;
  API_AVAILABLE(macos(10.15)) id <MTLCommandQueue> cmdQueue = nil;
  API_AVAILABLE(macos(10.15)) MPSNNImageNode* inputNode = nil;
  API_AVAILABLE(macos(10.15)) TCMPSVgg16Network *definition = nil;
  API_AVAILABLE(macos(10.15)) TCMPSVgg16Descriptor* descriptor = nil;
  API_AVAILABLE(macos(10.15)) MPSNNGraph* model_1 = nil;
  API_AVAILABLE(macos(10.15)) MPSNNGraph* model_2 = nil;
  API_AVAILABLE(macos(10.15)) MPSNNGraph* model_3 = nil;
  API_AVAILABLE(macos(10.15)) MPSNNGraph* model_4 = nil;
  API_AVAILABLE(macos(10.15)) NSDictionary<NSString *, NSData *> *weights = nil;
};

Vgg16Test::Vgg16Test(ptree config, ptree weights) : m_impl(new Vgg16Test::impl()) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      m_impl->dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
      m_impl->cmdQueue = [m_impl->dev newCommandQueue];
      
      m_impl->inputNode = [MPSNNImageNode nodeWithHandle:[[TCMPSGraphNodeHandle alloc] initWithLabel:@"inputImage"]];

      m_impl->weights = define_vgg_weights(weights);
      m_impl->descriptor = define_vgg_descriptor(config);

      m_impl->definition = [[TCMPSVgg16Network alloc] initWithParameters:@"vgg_"
                                                              inputNode:m_impl->inputNode
                                                                 device:m_impl->dev
                                                               cmdQueue:m_impl->cmdQueue
                                                             descriptor:m_impl->descriptor
                                                            initWeights:m_impl->weights];

      m_impl->model_1 = [MPSNNGraph graphWithDevice:m_impl->dev
                                        resultImage:m_impl->definition.reluOut1
                                resultImageIsNeeded:YES];

      m_impl->model_2 = [MPSNNGraph graphWithDevice:m_impl->dev
                                        resultImage:m_impl->definition.reluOut2
                                resultImageIsNeeded:YES];

      m_impl->model_3 = [MPSNNGraph graphWithDevice:m_impl->dev
                                        resultImage:m_impl->definition.reluOut3
                                resultImageIsNeeded:YES];

      m_impl->model_4 = [MPSNNGraph graphWithDevice:m_impl->dev
                                        resultImage:m_impl->definition.reluOut4
                                resultImageIsNeeded:YES];

      m_impl->model_1.format = MPSImageFeatureChannelFormatFloat32;
      m_impl->model_2.format = MPSImageFeatureChannelFormatFloat32;
      m_impl->model_3.format = MPSImageFeatureChannelFormatFloat32;
      m_impl->model_4.format = MPSImageFeatureChannelFormatFloat32;

    }
  }
}

Vgg16Test::~Vgg16Test() = default;

bool Vgg16Test::check_predict(ptree input, ptree output) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      MPSImageBatch *imageBatch = define_input(input, m_impl->dev);

      id<MTLCommandBuffer> cb1 = [m_impl->cmdQueue commandBuffer];

      NSMutableArray *intermediateImagesModel1 = [[NSMutableArray alloc] init];
      NSMutableArray *destinationStatesModel1 = [[NSMutableArray alloc] init];
      
      MPSImageBatch *outputBatchModel1 =  [m_impl->model_1 encodeBatchToCommandBuffer:cb1
                                                                         sourceImages:@[imageBatch]
                                                                         sourceStates:nil
                                                                   intermediateImages:intermediateImagesModel1
                                                                    destinationStates:destinationStatesModel1];

      for (MPSImage *image in outputBatchModel1) {
        [image synchronizeOnCommandBuffer:cb1];  
      }

      [cb1 commit];
      [cb1 waitUntilCompleted];

      id<MTLCommandBuffer> cb2 = [m_impl->cmdQueue commandBuffer];

      NSMutableArray *intermediateImagesModel2 = [[NSMutableArray alloc] init];
      NSMutableArray *destinationStatesModel2 = [[NSMutableArray alloc] init];
      
      MPSImageBatch *outputBatchModel2 =  [m_impl->model_2 encodeBatchToCommandBuffer:cb2
                                                                         sourceImages:@[imageBatch]
                                                                         sourceStates:nil
                                                                   intermediateImages:intermediateImagesModel2
                                                                    destinationStates:destinationStatesModel2];

      for (MPSImage *image in outputBatchModel2) {
        [image synchronizeOnCommandBuffer:cb2];  
      }

      [cb2 commit];
      [cb2 waitUntilCompleted];

      id<MTLCommandBuffer> cb3 = [m_impl->cmdQueue commandBuffer];

      NSMutableArray *intermediateImagesModel3 = [[NSMutableArray alloc] init];
      NSMutableArray *destinationStatesModel3 = [[NSMutableArray alloc] init];
      
      MPSImageBatch *outputBatchModel3 =  [m_impl->model_3 encodeBatchToCommandBuffer:cb3
                                                                         sourceImages:@[imageBatch]
                                                                         sourceStates:nil
                                                                   intermediateImages:intermediateImagesModel3
                                                                    destinationStates:destinationStatesModel3];

      for (MPSImage *image in outputBatchModel3) {
        [image synchronizeOnCommandBuffer:cb3];  
      }

      [cb3 commit];
      [cb3 waitUntilCompleted];

      id<MTLCommandBuffer> cb4 = [m_impl->cmdQueue commandBuffer];

      NSMutableArray *intermediateImagesModel4 = [[NSMutableArray alloc] init];
      NSMutableArray *destinationStatesModel4 = [[NSMutableArray alloc] init];
      
      MPSImageBatch *outputBatchModel4 =  [m_impl->model_4 encodeBatchToCommandBuffer:cb4
                                                                         sourceImages:@[imageBatch]
                                                                         sourceStates:nil
                                                                   intermediateImages:intermediateImagesModel4
                                                                    destinationStates:destinationStatesModel4];

      for (MPSImage *image in outputBatchModel4) {
        [image synchronizeOnCommandBuffer:cb4];  
      }

      [cb4 commit];
      [cb4 waitUntilCompleted];

      NSDictionary<NSString *, NSData *>* correctOutput = define_vgg_output(output);

      NSMutableData* dataOutput1 = [NSMutableData dataWithLength:correctOutput[@"output_1"].length];
      NSMutableData* dataOutput2 = [NSMutableData dataWithLength:correctOutput[@"output_2"].length];
      NSMutableData* dataOutput3 = [NSMutableData dataWithLength:correctOutput[@"output_3"].length];
      NSMutableData* dataOutput4 = [NSMutableData dataWithLength:correctOutput[@"output_4"].length];

      [[outputBatchModel1 objectAtIndex:0] readBytes:dataOutput1.mutableBytes
                                          dataLayout:MPSDataLayoutHeightxWidthxFeatureChannels
                                          imageIndex:0];

      [[outputBatchModel2 objectAtIndex:0] readBytes:dataOutput2.mutableBytes
                                          dataLayout:MPSDataLayoutHeightxWidthxFeatureChannels
                                          imageIndex:0];

      [[outputBatchModel3 objectAtIndex:0] readBytes:dataOutput3.mutableBytes
                                          dataLayout:MPSDataLayoutHeightxWidthxFeatureChannels
                                          imageIndex:0];

      [[outputBatchModel4 objectAtIndex:0] readBytes:dataOutput4.mutableBytes
                                          dataLayout:MPSDataLayoutHeightxWidthxFeatureChannels
                                          imageIndex:0];
            
      return check_data(correctOutput[@"output_1"], dataOutput1, 5e-3)
          && check_data(correctOutput[@"output_2"], dataOutput2, 5e-3)
          && check_data(correctOutput[@"output_3"], dataOutput3, 5e-3)
          && check_data(correctOutput[@"output_4"], dataOutput4, 5e-3);
    } else {
      return true;
    }
  }
}

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
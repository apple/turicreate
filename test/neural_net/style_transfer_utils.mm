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
  API_AVAILABLE(macos(10.15)) id <MTLDevice> dev = nil;
  API_AVAILABLE(macos(10.15)) id <MTLCommandQueue> cmdQueue = nil;

  API_AVAILABLE(macos(10.15)) MPSNNImageNode* contentNode = nil;
  API_AVAILABLE(macos(10.15)) MPSNNImageNode* contentScaleNode = nil;
  API_AVAILABLE(macos(10.15)) MPSNNImageNode* contentMeanNode = nil;
  
  API_AVAILABLE(macos(10.15)) MPSNNImageNode* styleNode = nil;
  API_AVAILABLE(macos(10.15)) MPSNNImageNode* styleScaleNode = nil;
  API_AVAILABLE(macos(10.15)) MPSNNImageNode* styleMeanNode = nil;

  API_AVAILABLE(macos(10.15)) TCMPSVgg16Descriptor* vggDescriptor = nil;
  API_AVAILABLE(macos(10.15)) TCMPSTransformerDescriptor* resnetDescriptor = nil;

  API_AVAILABLE(macos(10.15)) NSDictionary<NSString *, NSData *> *vggWeights = nil;
  API_AVAILABLE(macos(10.15)) NSDictionary<NSString *, NSData *> *resnetWeights = nil;

  API_AVAILABLE(macos(10.15)) TCMPSStyleTransferPreProcessing *contentPreProcess = nil;
  API_AVAILABLE(macos(10.15)) TCMPSStyleTransferPreProcessing *stylePreProcess = nil;
  API_AVAILABLE(macos(10.15)) TCMPSStyleTransferPreProcessing *transformerPreProcess = nil;

  API_AVAILABLE(macos(10.15)) TCMPSStyleTransferTransformerNetwork *transformer = nil;
  
  API_AVAILABLE(macos(10.15)) TCMPSVgg16Network *contentVGG = nil;
  API_AVAILABLE(macos(10.15)) TCMPSVgg16Network *styleVGG = nil;
  API_AVAILABLE(macos(10.15)) TCMPSVgg16Network *transformerVGG = nil;

  API_AVAILABLE(macos(10.15)) MPSNNGraph* model = nil;
};

LossTest::LossTest(ptree resnet_config, ptree vgg_config, ptree weights) : m_impl(new LossTest::impl()) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      float contentLossMultiplier = 1.0;
      float styleLossMultiplier = 1e-4;
      float totalLossMultiplier = 1e-4;

      m_impl->dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
      m_impl->cmdQueue = [m_impl->dev newCommandQueue];

      m_impl->contentNode = [MPSNNImageNode nodeWithHandle:[[TCMPSGraphNodeHandle alloc] initWithLabel:@"contentImage"]];
      m_impl->contentScaleNode = [MPSNNImageNode nodeWithHandle:[[TCMPSGraphNodeHandle alloc] initWithLabel:@"contentImage"]];
      m_impl->contentMeanNode = [MPSNNImageNode nodeWithHandle:[[TCMPSGraphNodeHandle alloc] initWithLabel:@"contentImage"]];

      m_impl->styleNode = [MPSNNImageNode nodeWithHandle:[[TCMPSGraphNodeHandle alloc] initWithLabel:@"styleImage"]];
      m_impl->styleScaleNode = [MPSNNImageNode nodeWithHandle: [[TCMPSGraphNodeHandle alloc] initWithLabel:@"styleScaleImage"]];
      m_impl->styleMeanNode = [MPSNNImageNode nodeWithHandle: [[TCMPSGraphNodeHandle alloc] initWithLabel:@"styleMeanImage"]];

      m_impl->vggWeights = define_vgg_weights(weights);
      m_impl->resnetWeights = define_transformer_weights(weights);

      m_impl->vggDescriptor = define_vgg_descriptor(vgg_config);
      m_impl->resnetDescriptor = define_transformer_descriptor(resnet_config);



      m_impl->transformer = [[TCMPSStyleTransferTransformerNetwork alloc] initWithParameters:@"Transformer"
                                                                                   inputNode:m_impl->contentNode
                                                                                      device:m_impl->dev
                                                                                    cmdQueue:m_impl->cmdQueue
                                                                                  descriptor:m_impl->resnetDescriptor
                                                                                 initWeights:m_impl->resnetWeights];

      m_impl->transformerPreProcess = [[TCMPSStyleTransferPreProcessing alloc] initWithParameters:@"Content_Pre_Processing"
                                                                                        inputNode:m_impl->transformer.forwardPass
                                                                                        scaleNode:m_impl->contentScaleNode
                                                                                         meanNode:m_impl->contentMeanNode];

      m_impl->transformerVGG = [[TCMPSVgg16Network alloc] initWithParameters:@"Content_VGG_16"
                                                                   inputNode:m_impl->transformerPreProcess.output
                                                                      device:m_impl->dev
                                                                    cmdQueue:m_impl->cmdQueue
                                                                  descriptor:m_impl->vggDescriptor
                                                                 initWeights:m_impl->vggWeights];

      m_impl->stylePreProcess = [[TCMPSStyleTransferPreProcessing alloc] initWithParameters:@"Style_Pre_Processing"
                                                                                  inputNode:m_impl->styleNode
                                                                                  scaleNode:m_impl->styleScaleNode
                                                                                   meanNode:m_impl->styleMeanNode];

      m_impl->styleVGG = [[TCMPSVgg16Network alloc] initWithParameters:@"Style_VGG_16"
                                                             inputNode:m_impl->stylePreProcess.output
                                                                device:m_impl->dev
                                                              cmdQueue:m_impl->cmdQueue
                                                            descriptor:m_impl->vggDescriptor
                                                           initWeights:m_impl->vggWeights];

      m_impl->contentPreProcess = [[TCMPSStyleTransferPreProcessing alloc] initWithParameters:@"Content_Loss_Pre_Processing"
                                                                                 inputNode:m_impl->contentNode
                                                                                 scaleNode:m_impl->contentScaleNode
                                                                                  meanNode:m_impl->contentMeanNode];

      m_impl->contentVGG = [[TCMPSVgg16Network alloc] initWithParameters:@"Content_VGG_16"
                                                               inputNode:m_impl->contentPreProcess.output
                                                                  device:m_impl->dev
                                                                cmdQueue:m_impl->cmdQueue
                                                              descriptor:m_impl->vggDescriptor
                                                             initWeights:m_impl->vggWeights];

      NSUInteger DEFAULT_IMAGE_SIZE = 256;

      NSUInteger gramScaling1 = (DEFAULT_IMAGE_SIZE * DEFAULT_IMAGE_SIZE);
      NSUInteger gramScaling2 = ((DEFAULT_IMAGE_SIZE/2) * (DEFAULT_IMAGE_SIZE/2));
      NSUInteger gramScaling3 = ((DEFAULT_IMAGE_SIZE/4) * (DEFAULT_IMAGE_SIZE/4));
      NSUInteger gramScaling4 = ((DEFAULT_IMAGE_SIZE/8) * (DEFAULT_IMAGE_SIZE/8));

      MPSCNNLossDescriptor *styleDesc = [MPSCNNLossDescriptor cnnLossDescriptorWithType:MPSCNNLossTypeMeanSquaredError
                                                                          reductionType:MPSCNNReductionTypeMean];

      styleDesc.weight = 0.5 * styleLossMultiplier * totalLossMultiplier;

      MPSCNNLossDescriptor *contentDesc = [MPSCNNLossDescriptor cnnLossDescriptorWithType:MPSCNNLossTypeMeanSquaredError
                                                                           reductionType:MPSCNNReductionTypeMean];

      contentDesc.weight = 0.5 * contentLossMultiplier * totalLossMultiplier;

      MPSNNGramMatrixCalculationNode *gramMatrixStyleLossFirstReLU
          = [MPSNNGramMatrixCalculationNode nodeWithSource:m_impl->styleVGG.reluOut1
                                                     alpha:(1.0/gramScaling1)];

      MPSNNGramMatrixCalculationNode *gramMatrixContentVggFirstReLU
          = [MPSNNGramMatrixCalculationNode nodeWithSource:m_impl->transformerVGG.reluOut1
                                                     alpha:(1.0/gramScaling1)];

      MPSNNForwardLossNode *styleLossNode1 
          = [MPSNNForwardLossNode nodeWithSource:gramMatrixContentVggFirstReLU.resultImage
                                          labels:gramMatrixStyleLossFirstReLU.resultImage
                                  lossDescriptor:styleDesc];

      MPSNNGramMatrixCalculationNode *gramMatrixStyleLossSecondReLU
          = [MPSNNGramMatrixCalculationNode nodeWithSource:m_impl->styleVGG.reluOut2
                                                     alpha:(1.0/gramScaling2)];

      MPSNNGramMatrixCalculationNode *gramMatrixContentVggSecondReLU
          = [MPSNNGramMatrixCalculationNode nodeWithSource:m_impl->transformerVGG.reluOut2
                                                     alpha:(1.0/gramScaling2)];

      MPSNNForwardLossNode *styleLossNode2
          = [MPSNNForwardLossNode nodeWithSource:gramMatrixContentVggSecondReLU.resultImage
                                          labels:gramMatrixStyleLossSecondReLU.resultImage
                                  lossDescriptor:styleDesc];

      MPSNNGramMatrixCalculationNode *gramMatrixStyleLossThirdReLU
          = [MPSNNGramMatrixCalculationNode nodeWithSource:m_impl->styleVGG.reluOut3
                                                     alpha:(1.0/gramScaling3)];

      MPSNNGramMatrixCalculationNode *gramMatrixContentVggThirdReLU
          = [MPSNNGramMatrixCalculationNode nodeWithSource:m_impl->transformerVGG.reluOut3
                                                     alpha:(1.0/gramScaling3)];

      MPSNNForwardLossNode *styleLossNode3
          = [MPSNNForwardLossNode nodeWithSource:gramMatrixContentVggThirdReLU.resultImage
                                          labels:gramMatrixStyleLossThirdReLU.resultImage
                                  lossDescriptor:styleDesc];

      MPSNNGramMatrixCalculationNode *gramMatrixStyleLossFourthReLU
          = [MPSNNGramMatrixCalculationNode nodeWithSource:m_impl->styleVGG.reluOut4
                                                     alpha:(1.0/gramScaling4)];

      MPSNNGramMatrixCalculationNode *gramMatrixContentVggFourthReLU
          = [MPSNNGramMatrixCalculationNode nodeWithSource:m_impl->transformerVGG.reluOut4
                                                     alpha:(1.0/gramScaling4)];

      MPSNNForwardLossNode *styleLossNode4 
          = [MPSNNForwardLossNode nodeWithSource:gramMatrixContentVggFourthReLU.resultImage
                                          labels:gramMatrixStyleLossFourthReLU.resultImage
                                  lossDescriptor:styleDesc];

      MPSNNForwardLossNode *contentLossNode
          = [MPSNNForwardLossNode nodeWithSource:m_impl->transformerVGG.reluOut3
                                          labels:m_impl->contentVGG.reluOut3
                                  lossDescriptor:contentDesc];

      MPSNNAdditionNode* addLossStyle1Style2
          = [MPSNNAdditionNode nodeWithSources:@[styleLossNode1.resultImage,
                                                 styleLossNode2.resultImage]];

      MPSNNAdditionNode* addLossStyle3Style4
          = [MPSNNAdditionNode nodeWithSources:@[styleLossNode3.resultImage,
                                                 styleLossNode4.resultImage]];

      MPSNNAdditionNode* addTotalStyleLoss 
          = [MPSNNAdditionNode nodeWithSources:@[addLossStyle1Style2.resultImage,
                                                 addLossStyle3Style4.resultImage]];

      MPSNNAdditionNode* totalLoss
          = [MPSNNAdditionNode nodeWithSources:@[contentLossNode.resultImage,
                                                 addTotalStyleLoss.resultImage]];

      m_impl->model = [MPSNNGraph graphWithDevice:m_impl->dev
                                      resultImage:totalLoss.resultImage
                              resultImageIsNeeded:YES];

      m_impl->model.format = MPSImageFeatureChannelFormatFloat32;
    }
  }
}

LossTest::~LossTest() = default;

bool LossTest::check_predict(ptree input, ptree output) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      
    }
  }
}

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
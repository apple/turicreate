/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#import <ml/neural_net/TCMLComputeDrawingClassifierDescriptor.h>

#import <ml/neural_net/TCMLComputeUtil.h>

@implementation TCMLComputeDrawingClassifierDescriptor

- (BOOL)isComplete
{
  if (self.inputTensor == nil) return NO;
  if (self.outputChannels == 0) return NO;

  return YES;
}

- (MLCTensor *)addConvLayer:(NSUInteger)index
             outputChannels:(NSUInteger)outputChannels
                     source:(MLCTensor *)source
                      graph:(MLCGraph *)graph
{
  // Find the weights for this conv layer in our dictionary of parameters.
  NSString *biasKey = [NSString stringWithFormat:@"drawing_conv%lu_bias", (unsigned long)index];
  NSString *weightKey = [NSString stringWithFormat:@"drawing_conv%lu_weight", (unsigned long)index];
  MLCTensor *bias = self.weights[biasKey];
  MLCTensor *weights = self.weights[weightKey];

  // Configure the convolution descriptor.
  NSUInteger inputChannels =
      weights.descriptor.shape[TCMLComputeTensorSizeChannels].unsignedIntegerValue / outputChannels;
  NSUInteger kernelHeight =
      weights.descriptor.shape[TCMLComputeTensorSizeHeight].unsignedIntegerValue;
  NSUInteger kernelWidth =
      weights.descriptor.shape[TCMLComputeTensorSizeWidth].unsignedIntegerValue;
  // Configure the convolution descriptor.
  MLCConvolutionDescriptor *conv_desc =
      [MLCConvolutionDescriptor descriptorWithKernelSizes:@[ @(kernelHeight), @(kernelWidth) ]
                                 inputFeatureChannelCount:inputChannels
                                outputFeatureChannelCount:outputChannels
                                                  strides:@[ @1, @1 ]
                                            paddingPolicy:MLCPaddingPolicySame
                                             paddingSizes:nil];

  MLCConvolutionLayer *conv = [MLCConvolutionLayer layerWithWeights:weights
                                                             biases:bias
                                                         descriptor:conv_desc];
  MLCTensor *convTensor = [graph nodeWithLayer:conv source:source];

  MLCLayer *relu1 = [MLCActivationLayer
      layerWithDescriptor:[MLCActivationDescriptor descriptorWithType:MLCActivationTypeReLU
                                                                    a:0.0f]];
  MLCTensor *reluTensor = [graph nodeWithLayer:relu1 source:convTensor];

  MLCPoolingDescriptor *poolDesc =
      [MLCPoolingDescriptor maxPoolingDescriptorWithKernelSizes:@[ @2, @2 ]
                                                        strides:@[ @2, @2 ]
                                                  paddingPolicy:MLCPaddingPolicyValid
                                                   paddingSizes:nil];
  MLCLayer *pool = [MLCPoolingLayer layerWithDescriptor:poolDesc];

  MLCTensor *poolTensor = [graph nodeWithLayer:pool source:reluTensor];

  return poolTensor;
}

- (MLCTensor *)addDenseLayer:(NSUInteger)index
              outputChannels:(NSUInteger)outputChannels
                      source:(MLCTensor *)source
                       graph:(MLCGraph *)graph
{
  // Find the weights for this conv layer in our dictionary of parameters.
  NSString *biasKey = [NSString stringWithFormat:@"drawing_dense%lu_bias", (unsigned long)index];
  NSString *weightKey =
      [NSString stringWithFormat:@"drawing_dense%lu_weight", (unsigned long)index];
  MLCTensor *bias = self.weights[biasKey];
  MLCTensor *weights = self.weights[weightKey];

  // Configure the convolution descriptor.
  NSUInteger inputChannels =
      weights.descriptor.shape[TCMLComputeTensorSizeChannels].unsignedIntegerValue / outputChannels;
  MLCConvolutionDescriptor *dense_desc =
      [MLCConvolutionDescriptor descriptorWithKernelSizes:@[ @1, @1 ]
                                 inputFeatureChannelCount:inputChannels
                                outputFeatureChannelCount:outputChannels
                                                  strides:@[ @1, @1 ]
                                            paddingPolicy:MLCPaddingPolicySame
                                             paddingSizes:nil];

  MLCFullyConnectedLayer *dense = [MLCFullyConnectedLayer layerWithWeights:weights
                                                                    biases:bias
                                                                descriptor:dense_desc];
  MLCTensor *denseTensor = [graph nodeWithLayer:dense source:source];
  return denseTensor;
}
@end

@implementation MLCGraph (TCMLComputeDrawingClassifier)

+ (instancetype)tc_graphForDrawingClassifierDescriptor:
                    (TCMLComputeDrawingClassifierDescriptor *)descriptor
                                             batchSize:(NSUInteger)batchSize
{
  if (![descriptor isComplete]) return nil;

  MLCGraph *graph = [[self alloc] init];

  NSUInteger channelCounts[] = {16, 32, 64};
  MLCTensor *tensor = descriptor.inputTensor;
  for (NSUInteger i = 0; i < 3; ++i) {
    tensor = [descriptor addConvLayer:i outputChannels:channelCounts[i] source:tensor graph:graph];
  }
  MLCReshapeLayer *flatten_layer =
      [MLCReshapeLayer layerWithShape:@[ @(batchSize), @(576), @(1), @(1) ]];
  tensor = [graph nodeWithLayer:flatten_layer source:tensor];
  tensor = [descriptor addDenseLayer:0 outputChannels:128 source:tensor graph:graph];
  MLCLayer *relu1 = [MLCActivationLayer
      layerWithDescriptor:[MLCActivationDescriptor descriptorWithType:MLCActivationTypeReLU
                                                                    a:0.0f]];
  MLCTensor *reluTensor = [graph nodeWithLayer:relu1 source:tensor];
  descriptor.outputTensor = [descriptor addDenseLayer:1
                                       outputChannels:descriptor.outputChannels
                                               source:reluTensor
                                                graph:graph];
  return graph;
}

@end

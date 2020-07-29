/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#import <ml/neural_net/TCMLComputeObjectDetectorDescriptor.h>

#import <ml/neural_net/TCMLComputeUtil.h>

@implementation TCMLComputeObjectDetectorDescriptor

- (BOOL)isComplete
{
  if (self.inputTensor == nil) return NO;
  if (self.outputChannels == 0) return NO;
  if (self.weights == nil) return NO;

  return YES;
}

- (MLCConvolutionLayer *)convLayerForIndex:(NSUInteger)index
                            outputChannels:(NSUInteger)outputChannels
{
  // Find the weights for this conv layer in our dictionary of parameters.
  NSString *biasKey = [NSString stringWithFormat:@"conv%lu_bias", (unsigned long)index];
  NSString *weightKey = [NSString stringWithFormat:@"conv%lu_weight", (unsigned long)index];
  MLCTensor *bias = self.weights[biasKey];
  MLCTensor *weights = self.weights[weightKey];

  // Configure the convolution descriptor.
  NSUInteger inputChannels =
      weights.descriptor.shape[TCMLComputeTensorSizeChannels].unsignedIntegerValue / outputChannels;
  NSUInteger kernelHeight =
      weights.descriptor.shape[TCMLComputeTensorSizeHeight].unsignedIntegerValue;
  NSUInteger kernelWidth =
      weights.descriptor.shape[TCMLComputeTensorSizeWidth].unsignedIntegerValue;
  MLCConvolutionDescriptor *descriptor =
      [MLCConvolutionDescriptor descriptorWithKernelSizes:@[ @(kernelHeight), @(kernelWidth) ]
                                 inputFeatureChannelCount:inputChannels
                                outputFeatureChannelCount:outputChannels
                                                  strides:@[ @1, @1 ]
                                            paddingPolicy:MLCPaddingPolicySame
                                             paddingSizes:@[ @1, @1 ]];

  return [MLCConvolutionLayer layerWithWeights:weights biases:bias descriptor:descriptor];
}

- (MLCBatchNormalizationLayer *)batchNormLayerForIndex:(NSUInteger)index
{
  // Find the weights for this batch norm layer in our dictionary of parameters.
  NSString *gammaKey = [NSString stringWithFormat:@"batchnorm%lu_gamma", (unsigned long)index];
  NSString *betaKey = [NSString stringWithFormat:@"batchnorm%lu_beta", (unsigned long)index];
  NSString *varianceKey =
      [NSString stringWithFormat:@"batchnorm%lu_running_var", (unsigned long)index];
  NSString *meanKey =
      [NSString stringWithFormat:@"batchnorm%lu_running_mean", (unsigned long)index];

  MLCTensor *gamma = self.weights[gammaKey];
  MLCTensor *beta = self.weights[betaKey];
  MLCTensor *variance = self.weights[varianceKey];
  MLCTensor *mean = self.weights[meanKey];

  NSUInteger featureChannels =
      mean.descriptor.shape[TCMLComputeTensorSizeChannels].unsignedIntegerValue;

  return [MLCBatchNormalizationLayer layerWithFeatureChannelCount:featureChannels
                                                             mean:mean
                                                         variance:variance
                                                             beta:beta
                                                            gamma:gamma
                                                  varianceEpsilon:1e-5f
                                                         momentum:0.9f];
}

- (MLCTensor *)addCommonLayersWithIndex:(NSUInteger)index
                         outputChannels:(NSUInteger)outputChannels
                                 source:(MLCTensor *)source
                                  graph:(MLCTrainingGraph *)graph
{
  // conv
  MLCConvolutionLayer *convLayer = [self convLayerForIndex:index outputChannels:outputChannels];
  MLCTensor *convTensor = [graph nodeWithLayer:convLayer source:source];

  // batchnorm
  MLCBatchNormalizationLayer *batchNormLayer = [self batchNormLayerForIndex:index];
  MLCTensor *batchNormTensor = [graph nodeWithLayer:batchNormLayer source:convTensor];

  // leakyrelu
  MLCActivationDescriptor *leakyReLUDesc =
      [MLCActivationDescriptor descriptorWithType:MLCActivationTypeReLU a:0.1f];
  MLCActivationLayer *leakyReLULayer = [MLCActivationLayer layerWithDescriptor:leakyReLUDesc];
  MLCTensor *leakyReLUTensor = [graph nodeWithLayer:leakyReLULayer source:batchNormTensor];

  // pool
  // On the first five blocks (0-4), pool with stride 2, reducing image dimensions by a factor of 2.
  // On the sixth (5), pool with stride 1, preserving image dimensions.
  if (index <= 5) {
    NSUInteger poolingStride = index < 5 ? 2 : 1;
    MLCPoolingDescriptor *poolingDesc =
        [MLCPoolingDescriptor poolingDescriptorWithType:MLCPoolingTypeMax
                                             kernelSize:2
                                                 stride:poolingStride];
    MLCLayer *poolLayer = [MLCPoolingLayer layerWithDescriptor:poolingDesc];
    MLCTensor *poolTensor = [graph nodeWithLayer:poolLayer source:leakyReLUTensor];
    return poolTensor;
  } else {
    return leakyReLUTensor;
  }
}

@end

@implementation MLCGraph (TCMLComputeObjectDetector)

+ (instancetype)tc_graphForObjectDetectorDescriptor:
    (TCMLComputeObjectDetectorDescriptor *)descriptor
{
  if (![descriptor isComplete]) return nil;

  MLCTrainingGraph *graph = [[self alloc] init];

  // Start with the 8 rounds of convolution, batch norm, and leaky ReLU layers. After the first 5
  // rounds, max pooling reduces the image size by half each time.
  NSUInteger channelCounts[] = {16, 32, 64, 128, 256, 512, 1024, 1024};
  MLCTensor *tensor = descriptor.inputTensor;
  for (NSUInteger i = 0; i < 8; ++i) {
    tensor = [descriptor addCommonLayersWithIndex:i
                                   outputChannels:channelCounts[i]
                                           source:tensor
                                            graph:graph];
  }

  // Add the final convolution layer, which maps the tensor into the YOLO representation.
  MLCConvolutionLayer *conv8Layer = [descriptor convLayerForIndex:8
                                                   outputChannels:descriptor.outputChannels];
  [graph nodeWithLayer:conv8Layer source:tensor];

  return graph;
}

@end

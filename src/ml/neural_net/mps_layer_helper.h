/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#import <Metal/Metal.h>

NS_ASSUME_NONNULL_BEGIN

@interface MPSCNNFullyConnectedNode (TCMPSLayerHelper)
+ (MPSCNNFullyConnectedNode *) createFullyConnected:(MPSNNImageNode *)inputNode
                               inputFeatureChannels:(NSUInteger)inputFeatureChannels
                              outputFeatureChannels:(NSUInteger)outputFeatureChannels
                                        inputHeight:(NSUInteger)inputHeight
                                         inputWidth:(NSUInteger)inputWidth
                                            weights:(NSData *)weights
                                             biases:(NSData *)biases
                                              label:(NSString *)label
                                      updateWeights:(BOOL)updateWeights
                                             device:(id<MTLDevice>)dev
                                           cmdQueue:(id<MTLCommandQueue>)cmdQ
                                          API_AVAILABLE(macosx(10.14));

@end

@interface MPSCNNConvolutionNode (TCMPSLayerHelper)                                   
+ (MPSCNNConvolutionNode *) createConvolutional:(MPSNNImageNode *)inputNode
                                    kernelWidth:(NSUInteger)kernelWidth
                                   kernelHeight:(NSUInteger)kernelHeight
                           inputFeatureChannels:(NSUInteger)inputFeatureChannels
                          outputFeatureChannels:(NSUInteger)outputFeatureChannels
                                    strideWidth:(NSUInteger)strideWidth
                                   strideHeight:(NSUInteger)strideHeight
                                   paddingWidth:(NSUInteger)paddingWidth
                                  paddingHeight:(NSUInteger)paddingHeight
                                        weights:(NSData *)weights
                                         biases:(NSData *)biases
                                          label:(NSString *)label
                                  updateWeights:(BOOL)updateWeights
                                         device:(id<MTLDevice>)dev
                                       cmdQueue:(id<MTLCommandQueue>)cmdQ
                                      API_AVAILABLE(macosx(10.14));

+ (MPSCNNInstanceNormalizationNode *) createInstanceNormalization:(MPSNNImageNode *)inputNode
                                                         channels:(NSUInteger)channels
                                                           styles:(NSUInteger)styles
                                                            gamma:(NSData *)gamma
                                                             beta:(NSData *)beta
                                                            label:(NSString *)label
                                                           device:(id<MTLDevice>)dev
                                                         cmdQueue:(id<MTLCommandQueue>)cmdQ
                                                        API_AVAILABLE(macosx(10.14));
@end

NS_ASSUME_NONNULL_END

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#import <Metal/Metal.h>

NS_ASSUME_NONNULL_BEGIN

API_AVAILABLE(macos(10.14))
@interface TCMPSLayerHelper: NSObject

+ (MPSCNNFullyConnectedNode *) createFullyConnected:(MPSNNImageNode *)inputNode
                               inputFeatureChannels:(NSUInteger)inputFeatureChannels
                              outputFeatureChannels:(NSUInteger)outputFeatureChannels
                                        inputHeight:(NSUInteger)inputHeight
                                         inputWidth:(NSUInteger)inputWidth
                                            weights:(float *)weights
                                             biases:(float *)biases
                                              label:(NSString *)label
                                      updateWeights:(bool)updateWeights
                                             device:(id<MTLDevice>)dev
                                          cmd_queue:(id<MTLCommandQueue>) cmd_q;

+ (MPSCNNConvolutionNode *) createConvolutional:(MPSNNImageNode *)inputNode
                                    kernelWidth:(NSUInteger)kernelWidth
                                   kernelHeight:(NSUInteger)kernelHeight
                           inputFeatureChannels:(NSUInteger)inputFeatureChannels
                          outputFeatureChannels:(NSUInteger)outputFeatureChannels
                                    strideWidth:(NSUInteger)strideWidth
                                   strideHeight:(NSUInteger)strideHeight
                                   paddingWidth:(NSUInteger)paddingWidth
                                  paddingHeight:(NSUInteger)paddingHeight
                                        weights:(float *)weights
                                         biases:(float *)biases
                                          label:(NSString *)label
                                  updateWeights:(bool)updateWeights
                                         device:(id<MTLDevice>)dev
                                      cmd_queue:(id<MTLCommandQueue>) cmd_q;

+ (MPSCNNInstanceNormalizationNode *) createInstanceNormalization:(MPSNNImageNode *)inputNode
                                                         channels:(NSUInteger)channels
                                                           styles:(NSUInteger)styles
                                                            gamma:(float * _Nonnull * _Nonnull)gamma
                                                             beta:(float * _Nonnull * _Nonnull)beta
                                                            label:(NSString *)label
                                                           device:(id<MTLDevice>)dev
                                                        cmd_queue:(id<MTLCommandQueue>) cmd_q;

@end

NS_ASSUME_NONNULL_END

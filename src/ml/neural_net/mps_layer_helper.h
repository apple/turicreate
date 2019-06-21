#ifndef transformer_h
#define transformer_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#import <Metal/Metal.h>

API_AVAILABLE(macos(10.13))
@interface TCMPSLayerHelper: NSObject

// TODO: Make NonNull
+ (MPSCNNFullyConnectedNode *) createFullyConnected:(MPSNNImageNode * _Nonnull)inputNode
                               inputFeatureChannels:(int)inputFeatureChannels
                              outputFeatureChannels:(int)outputFeatureChannels
                                        inputHeight:(int)inputHeight
                                         inputWidth:(int)inputWidth
                                            weights:(float * _Nonnull)weights
                                             biases:(float * _Nonnull)biases
                                             device:(id<MTLDevice> _Nonnull)dev
                                          cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q;

// TODO: Make NonNull
+ (MPSCNNConvolutionNode *) createConvolutional:(MPSNNImageNode * _Nonnull)inputNode
                                    kernelWidth:(int)kernelWidth
                                   kernelHeight:(int)kernelHeight
                           inputFeatureChannels:(int)inputFeatureChannels
                          outputFeatureChannels:(int)outputFeatureChannels
                                    strideWidth:(int)strideWidth
                                   strideHeight:(int)strideHeight
                                   paddingWidth:(int)paddingWidth
                                  paddingHeight:(int)paddingHeight
                                        weights:(float * _Nonnull)weights
                                         biases:(float * _Nonnull)biases
                                         device:(id<MTLDevice> _Nonnull)dev
                                      cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q;

@end

#endif
#ifndef mps_layer_helper_h
#define mps_layer_helper_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#import <Metal/Metal.h>

API_AVAILABLE(macos(10.14))
@interface TCMPSLayerHelper: NSObject

+ (MPSCNNFullyConnectedNode * _Nonnull) createFullyConnected:(MPSNNImageNode * _Nonnull)inputNode
                                        inputFeatureChannels:(int)inputFeatureChannels
                                       outputFeatureChannels:(int)outputFeatureChannels
                                                 inputHeight:(int)inputHeight
                                                  inputWidth:(int)inputWidth
                                                     weights:(float * _Nonnull)weights
                                                      biases:(float * _Nonnull)biases
                                                       label:(NSString * _Nonnull)label
                                               updateWeights:(bool)updateWeights
                                                      device:(id<MTLDevice> _Nonnull)dev
                                                   cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q;

+ (MPSCNNConvolutionNode *_Nonnull) createConvolutional:(MPSNNImageNode * _Nonnull)inputNode
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
                                                  label:(NSString * _Nonnull)label
                                          updateWeights:(bool)updateWeights
                                                 device:(id<MTLDevice> _Nonnull)dev
                                              cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q;

@end

#endif
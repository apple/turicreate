#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#import <Metal/Metal.h>

NS_ASSUME_NONNULL_BEGIN

API_AVAILABLE(macos(10.14))
@interface TCMPSLayerHelper: NSObject

+ (MPSCNNFullyConnectedNode * _Nullable) createFullyConnected:(MPSNNImageNode *)inputNode
                                        inputFeatureChannels:(NSNumber *)inputFeatureChannels
                                       outputFeatureChannels:(NSNumber *)outputFeatureChannels
                                                 inputHeight:(NSNumber *)inputHeight
                                                  inputWidth:(NSNumber *)inputWidth
                                                     weights:(NSArray *)weights
                                                      biases:(NSArray *)biases
                                                       label:(NSString *)label
                                               updateWeights:(bool)updateWeights
                                                      device:(id<MTLDevice>)dev
                                                   cmd_queue:(id<MTLCommandQueue>) cmd_q;

+ (MPSCNNConvolutionNode *_Nullable) createConvolutional:(MPSNNImageNode *)inputNode
                                            kernelWidth:(NSNumber *)kernelWidth
                                           kernelHeight:(NSNumber *)kernelHeight
                                   inputFeatureChannels:(NSNumber *)inputFeatureChannels
                                  outputFeatureChannels:(NSNumber *)outputFeatureChannels
                                            strideWidth:(NSNumber *)strideWidth
                                           strideHeight:(NSNumber *)strideHeight
                                           paddingWidth:(NSNumber *)paddingWidth
                                          paddingHeight:(NSNumber *)paddingHeight
                                                weights:(NSArray *)weights
                                                 biases:(NSArray *)biases
                                                  label:(NSString *)label
                                          updateWeights:(bool)updateWeights
                                                 device:(id<MTLDevice>)dev
                                              cmd_queue:(id<MTLCommandQueue>) cmd_q;

@end

NS_ASSUME_NONNULL_END

#include <ml/neural_net/mps_layer_helper.h>

@implementation TCMPSLayerHelper

// TODO: Make NonNull
+ (MPSCNNFullyConnectedNode *) createFullyConnected:(MPSNNImageNode * _Nonnull)inputNode
                               inputFeatureChannels:(int)inputFeatureChannels
                              outputFeatureChannels:(int)outputFeatureChannels
                                        inputHeight:(int)inputHeight
                                         inputWidth:(int)inputWidth
                                            weights:(float * _Nonnull)weights
                                             biases:(float * _Nonnull)biases
                                             device:(id<MTLDevice> _Nonnull)dev
                                          cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q {
  // TODO: Fully Connected Data Loaders
  
  // TODO: Return MPSCNNFullyConnectedNode
  return Nil;
}

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
                                      cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q {
  // TODO: Convolutional Data Loaders

  // TODO: Return MPSCNNConvolutionNode
	return Nil;
}

@end
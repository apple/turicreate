#include <ml/neural_net/mps_layer_helper.h>

#include <ml/neural_net/mps_layer_fully_connected_data_loader.h>

#include <ml/neural_net/mps_layer_conv_data_loader.h>
#include <ml/neural_net/mps_layer_conv_padding.h>

@implementation TCMPSLayerHelper

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
                                                   cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q {

  FullyConnectedDataLoader* FullyConnectedDataLoad = [[FullyConnectedDataLoader alloc] initWithParams:label
                                                                                 inputFeatureChannels:inputFeatureChannels
                                                                                outputFeatureChannels:outputFeatureChannels
                                                                                          inputHeight:inputHeight
                                                                                           inputWidth:inputWidth
                                                                                              weights:weights
                                                                                               biases:biases
                                                                                        updateWeights:updateWeights
                                                                                               device:dev
                                                                                            cmd_queue:cmd_q];
        
  MPSCNNFullyConnectedNode* FullyConnectedNode = [MPSCNNFullyConnectedNode nodeWithSource:inputNode weights:FullyConnectedDataLoad];
  
  return FullyConnectedNode;
}

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
                                              cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q {

  ConvolutionDataLoader* ConvDataLoad  =  [[ConvolutionDataLoader alloc] initWithParams:label
                                                                            kernelWidth:kernelWidth
                                                                           kernelHeight:kernelHeight
                                                                   inputFeatureChannels:inputFeatureChannels
                                                                  outputFeatureChannels:outputFeatureChannels
                                                                            strideWidth:strideWidth
                                                                           strideHeight:strideHeight
                                                                                weights:weights
                                                                                 biases:biases
                                                                          updateWeights:updateWeights
                                                                                 device:dev
                                                                              cmd_queue:cmd_q];
    
  MPSCNNConvolutionNode*  ConvNode = [MPSCNNConvolutionNode nodeWithSource:inputNode weights:ConvDataLoad];

  ConvolutionPadding* Padding = [[ConvolutionPadding alloc] initWithParams:paddingWidth
                                                             paddingHeight:paddingHeight
                                                               strideWidth:strideWidth
                                                              strideHeight:strideHeight];

  ConvNode.paddingPolicy = Padding;
  
	return ConvNode;
}

@end
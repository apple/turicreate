#include <ml/neural_net/mps_layer_helper.h>

#include <ml/neural_net/mps_layer_fully_connected_data_loader.h>

#include <ml/neural_net/mps_layer_conv_data_loader.h>
#include <ml/neural_net/mps_layer_conv_padding.h>

@implementation TCMPSLayerHelper

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
                                                   cmd_queue:(id<MTLCommandQueue>) cmd_q {
/*
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
  */
  return Nil;
}

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
                                              cmd_queue:(id<MTLCommandQueue>) cmd_q {
/*
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
*/
  return Nil;
}

@end

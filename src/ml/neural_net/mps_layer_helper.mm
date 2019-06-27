#include <ml/neural_net/mps_layer_helper.h>
#include <ml/neural_net/mps_weight.h>

#include <ml/neural_net/mps_layer_fully_connected_data_loader.h>
#include <ml/neural_net/mps_layer_conv_padding.h>

@implementation TCMPSLayerHelper

+ (MPSCNNFullyConnectedNode * _Nullable) createFullyConnected:(MPSNNImageNode *)inputNode
                                        inputFeatureChannels:(NSUInteger)inputFeatureChannels
                                       outputFeatureChannels:(NSUInteger)outputFeatureChannels
                                                 inputHeight:(NSUInteger)inputHeight
                                                  inputWidth:(NSUInteger)inputWidth
                                                     weights:(float *)weights
                                                      biases:(float *)biases
                                                       label:(NSString *)label
                                               updateWeights:(bool)updateWeights
                                                      device:(id<MTLDevice>)dev
                                                   cmd_queue:(id<MTLCommandQueue>) cmd_q {
  
  turi::neural_net::OptimizerOptions optimizerOptions;

  TCMPSConvolutionWeights *FullyConnectedDataLoad = [[TCMPSConvolutionWeights alloc] initWithKernelWidth:inputWidth
                                                                                  kernelHeight:inputHeight
                                                                          inputFeatureChannels:inputFeatureChannels
                                                                         outputFeatureChannels:outputFeatureChannels
                                                                                    neuronType:MPSCNNNeuronTypeNone
                                                                                       strideX:1
                                                                                       strideY:1
                                                                                       neuronA:0.0f
                                                                                       neuronB:0.0f
                                                                        kernelParamsBinaryName:[label UTF8String]
                                                                                        device:dev
                                                                                     cmd_queue:cmd_q
                                                                               init_weight_ptr:weights
                                                                                 init_bias_ptr:biases
                                                                              optimizerOptions:optimizerOptions];
        
  MPSCNNFullyConnectedNode* FullyConnectedNode = [MPSCNNFullyConnectedNode nodeWithSource:inputNode
                                                                                  weights:FullyConnectedDataLoad];
  
  return FullyConnectedNode;
}

+ (MPSCNNConvolutionNode *_Nullable) createConvolutional:(MPSNNImageNode *)inputNode
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
                                              cmd_queue:(id<MTLCommandQueue>) cmd_q {

  turi::neural_net::OptimizerOptions optimizerOptions;
  
  TCMPSConvolutionWeights *ConvDataLoad = [[TCMPSConvolutionWeights alloc] initWithKernelWidth:kernelWidth
                                                                                 kernelHeight:kernelHeight
                                                                          inputFeatureChannels:inputFeatureChannels
                                                                         outputFeatureChannels:outputFeatureChannels
                                                                                    neuronType:MPSCNNNeuronTypeNone
                                                                                       strideX:strideWidth
                                                                                       strideY:strideHeight
                                                                                       neuronA:0.0f
                                                                                       neuronB:0.0f
                                                                        kernelParamsBinaryName:[label UTF8String]
                                                                                        device:dev
                                                                                     cmd_queue:cmd_q
                                                                               init_weight_ptr:weights
                                                                                 init_bias_ptr:biases
                                                                              optimizerOptions:optimizerOptions];

  MPSCNNConvolutionNode*  ConvNode = [MPSCNNConvolutionNode nodeWithSource:inputNode
                                                                   weights:ConvDataLoad];

  ConvolutionPadding* Padding = [[ConvolutionPadding alloc] initWithParams:paddingWidth
                                                             paddingHeight:paddingHeight
                                                               strideWidth:strideWidth
                                                              strideHeight:strideHeight];

  ConvNode.paddingPolicy = Padding;
  
	return ConvNode;
}

@end

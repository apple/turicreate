#include <ml/neural_net/mps_layer_helper.h>
#include <ml/neural_net/mps_weight.h>
#import <ml/neural_net/mps_layer_conv_padding.h>

@implementation MPSCNNFullyConnectedNode (TCMPSLayerHelper)
+ (MPSCNNFullyConnectedNode *) createFullyConnected:(MPSNNImageNode *)inputNode
                               inputFeatureChannels:(NSUInteger)inputFeatureChannels
                              outputFeatureChannels:(NSUInteger)outputFeatureChannels
                                        inputHeight:(NSUInteger)inputHeight
                                         inputWidth:(NSUInteger)inputWidth
                                            weights:(float *)weights
                                             biases:(float *)biases
                                              label:(NSString *)label
                                      updateWeights:(BOOL)updateWeights
                                             device:(id<MTLDevice>)dev
                                          cmd_queue:(id<MTLCommandQueue>) cmd_q {
  
  turi::neural_net::OptimizerOptions optimizerOptions;

  TCMPSConvolutionWeights *fullyConnectedDataLoad =
    [[TCMPSConvolutionWeights alloc] initWithKernelWidth:inputWidth
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
        
  MPSCNNFullyConnectedNode* fullyConnectedNode = 
    [MPSCNNFullyConnectedNode nodeWithSource:inputNode
                                     weights:fullyConnectedDataLoad];
  
  return fullyConnectedNode;
}
@end

@implementation MPSCNNConvolutionNode (TCMPSLayerHelper)
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
                                  updateWeights:(BOOL)updateWeights
                                         device:(id<MTLDevice>)dev
                                      cmd_queue:(id<MTLCommandQueue>) cmd_q {
  
  turi::neural_net::OptimizerOptions optimizerOptions;

  TCMPSConvolutionWeights *convDataLoad =
    [[TCMPSConvolutionWeights alloc] initWithKernelWidth:kernelWidth
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

  MPSCNNConvolutionNode*  convNode =
    [MPSCNNConvolutionNode nodeWithSource:inputNode
                                  weights:convDataLoad];

  TCMPSConvolutionPadding* padding = 
    [[TCMPSConvolutionPadding alloc] initWithParams:paddingWidth
                                      paddingHeight:paddingHeight
                                        strideWidth:strideWidth
                                       strideHeight:strideHeight];

  convNode.paddingPolicy = padding;
  
	return convNode;
}
@end

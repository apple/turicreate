#include <ml/neural_net/mps_layer_helper.h>
#include <ml/neural_net/mps_weight.h>

#import <ml/neural_net/mps_layer_conv_padding.h>
#import <ml/neural_net/mps_layer_instance_norm_data_loader.h>

@implementation MPSCNNFullyConnectedNode (TCMPSLayerHelper)
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
                                           cmdQueue:(id<MTLCommandQueue>)cmdQ {
  
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
                                               cmd_queue:cmdQ
                                         init_weight_ptr:(float *)weights.bytes
                                           init_bias_ptr:(float *)biases.bytes
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
                                        weights:(NSData *)weights
                                         biases:(NSData *)biases
                                          label:(NSString *)label
                                  updateWeights:(BOOL)updateWeights
                                         device:(id<MTLDevice>)dev
                                       cmdQueue:(id<MTLCommandQueue>)cmdQ {
  
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
                                               cmd_queue:cmdQ
                                         init_weight_ptr:(float *)weights.bytes
                                           init_bias_ptr:(float *)biases.bytes
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

+ (MPSCNNInstanceNormalizationNode *) createInstanceNormalization:(MPSNNImageNode *)inputNode
                                                         channels:(NSUInteger)channels
                                                           styles:(NSUInteger)styles
                                                            gamma:(NSData *)gamma
                                                             beta:(NSData *)beta
                                                            label:(NSString *)label
                                                           device:(id<MTLDevice>)dev
                                                         cmdQueue:(id<MTLCommandQueue>) cmdQ {

  TCMPSInstanceNormDataLoader *InstNormDataLoad = [[TCMPSInstanceNormDataLoader alloc] initWithParams:label
                                                                                         gammaWeights:(float *)gamma.bytes
                                                                                          betaWeights:(float *)beta.bytes
                                                                                numberFeatureChannels:channels
                                                                                               styles:styles
                                                                                               device:dev
                                                                                            cmd_queue:cmdQ];
                                      
  MPSCNNInstanceNormalizationNode *instNormNode =  [MPSCNNInstanceNormalizationNode nodeWithSource:inputNode
                                                                                        dataSource:InstNormDataLoad];
  return instNormNode;
}

@end

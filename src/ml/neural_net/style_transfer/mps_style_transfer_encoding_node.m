#import <ml/neural_net/style_transfer/mps_style_transfer_encoding_node.h>
#import <ml/neural_net/mps_layer_helper.h>

@interface TCMPSStyleTransferEncodingNode ()
@property (nonatomic) MPSCNNConvolutionNode *conv;
@property (nonatomic) MPSCNNInstanceNormalizationNode* instNorm;
@property (nonatomic) MPSCNNNeuronReLUNNode* relu;
@end

@implementation TCMPSStyleTransferEncodingNode

- (instancetype) initWithParameters:(NSString *)name
                          inputNode:(MPSNNImageNode *)inputNode
                             device:(id<MTLDevice>)dev
                           cmdQueue:(id<MTLCommandQueue>)cmdQ
                         descriptor:(TCMPSEncodingDescriptor *)descriptor
                        initWeights:(NSDictionary<NSString *, NSData *> *) weights {
  self = [super init];
  
  if (self) {
    _conv = [MPSCNNConvolutionNode createConvolutional:inputNode
                                           kernelWidth:descriptor.conv.kernelWidth
                                          kernelHeight:descriptor.conv.kernelHeight
                                  inputFeatureChannels:descriptor.conv.inputFeatureChannels
                                 outputFeatureChannels:descriptor.conv.outputFeatureChannels
                                           strideWidth:descriptor.conv.strideWidth
                                          strideHeight:descriptor.conv.strideHeight
                                          paddingWidth:descriptor.conv.paddingWidth
                                         paddingHeight:descriptor.conv.paddingHeight
                                               weights:weights[[NSString stringWithFormat:@"%@/%@", name, @"encoding_conv_weights"]]
                                                biases:weights[[NSString stringWithFormat:@"%@/%@", name, @"encoding_conv_biases"]]
                                                 label:descriptor.conv.label
                                         updateWeights:descriptor.conv.updateWeights
                                                device:dev
                                              cmdQueue:cmdQ];

    _instNorm = [MPSCNNConvolutionNode createInstanceNormalization:[_conv resultImage]
                                                          channels:descriptor.inst.channels
                                                            styles:descriptor.inst.styles
                                                             gamma:weights[[NSString stringWithFormat:@"%@/%@", name, @"encoding_inst_gamma"]]
                                                              beta:weights[[NSString stringWithFormat:@"%@/%@", name, @"encoding_inst_beta"]]
                                                             label:descriptor.inst.label
                                                            device:dev
                                                          cmdQueue:cmdQ];

    _relu = [MPSCNNNeuronReLUNNode nodeWithSource: [_instNorm resultImage]];

    _output = [_relu resultImage];
  }

  return self;
}

- (MPSNNImageNode *) backwardPass:(MPSNNImageNode *) inputNode {
  MPSNNGradientFilterNode* reluGrad = [_relu gradientFilterWithSource: inputNode];
  MPSNNGradientFilterNode* instNormGrad = [_instNorm gradientFilterWithSource: [reluGrad resultImage]];
  MPSNNGradientFilterNode* convGrad = [_conv gradientFilterWithSource: [instNormGrad resultImage]];

  return [convGrad resultImage];
}

@end
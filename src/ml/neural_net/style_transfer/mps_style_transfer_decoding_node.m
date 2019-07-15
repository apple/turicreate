#import <ml/neural_net/style_transfer/mps_style_transfer_decoding_node.h>
#import <ml/neural_net/mps_layer_helper.h>

@interface TCMPSStyleTransferDecodingNode () {
  MPSCNNUpsamplingNearestNode *upsample;
  MPSCNNConvolutionNode *conv;
  MPSCNNInstanceNormalizationNode* instNorm;
  MPSCNNNeuronReLUNNode* relu;
}
@end

@implementation TCMPSStyleTransferDecodingNode

- (instancetype) initWithParameters:(NSString *)name
                          inputNode:(MPSNNImageNode *)inputNode
                             device:(id<MTLDevice>)dev
                          cmd_queue:(id<MTLCommandQueue>)cmd_q
                         descriptor:(TCMPSDecodingDescriptor *)descriptor
                        initWeights:(NSDictionary<NSString *, NSData *> *) weights {
  self = [super init];
  
  if (self) {
  	upsample = [MPSCNNUpsamplingNearestNode nodeWithSource:inputNode
                                       integerScaleFactorX:descriptor.upsample.scale
                                       integerScaleFactorY:descriptor.upsample.scale];

  	conv = [MPSCNNConvolutionNode createConvolutional:[upsample resultImage]
                                          kernelWidth:descriptor.conv.kernelWidth
                                         kernelHeight:descriptor.conv.kernelHeight
                                 inputFeatureChannels:descriptor.conv.inputFeatureChannels
                                outputFeatureChannels:descriptor.conv.outputFeatureChannels
                                          strideWidth:descriptor.conv.strideWidth
                                         strideHeight:descriptor.conv.strideHeight
                                         paddingWidth:descriptor.conv.paddingWidth
                                        paddingHeight:descriptor.conv.paddingHeight
                                              weights:(float *)weights[[NSString stringWithFormat:@"%@/%@", name, @"decoding_conv_weights"]].bytes
                                               biases:(float *)weights[[NSString stringWithFormat:@"%@/%@", name, @"decoding_conv_biases"]].bytes
                                                label:descriptor.conv.label
                                        updateWeights:descriptor.conv.updateWeights
                                               device:dev
                                            cmd_queue:cmd_q];

    instNorm = [MPSCNNConvolutionNode createInstanceNormalization:[conv resultImage]
                                                         channels:descriptor.inst.channels
                                                           styles:descriptor.inst.styles
                                                            gamma:(float *)weights[[NSString stringWithFormat:@"%@/%@", name, @"decoding_inst_gamma"]].bytes
                                                             beta:(float *)weights[[NSString stringWithFormat:@"%@/%@", name, @"decoding_inst_beta"]].bytes
                                                            label:descriptor.inst.label
                                                           device:dev
                                                        cmd_queue:cmd_q];

    relu = [MPSCNNNeuronReLUNNode nodeWithSource: [instNorm resultImage]];

    _output = [relu resultImage];
  }

  return self;
}

- (MPSNNImageNode *) backwardPass:(MPSNNImageNode *) inputNode {
	MPSNNGradientFilterNode* reluGrad = [relu gradientFilterWithSource: inputNode];
  MPSNNGradientFilterNode* instNormGrad = [instNorm gradientFilterWithSource: [reluGrad resultImage]];
  MPSNNGradientFilterNode* convGrad = [conv gradientFilterWithSource: [instNormGrad resultImage]];
  MPSNNGradientFilterNode* upsampleGrad = [upsample gradientFilterWithSource: [convGrad resultImage]];

  return [upsampleGrad resultImage];
}

@end
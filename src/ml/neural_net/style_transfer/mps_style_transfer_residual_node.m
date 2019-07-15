#import <ml/neural_net/style_transfer/mps_style_transfer_residual_node.h>
#import <ml/neural_net/mps_layer_helper.h>

@interface TCMPSStyleTransferResidualNode () {
  MPSCNNConvolutionNode *conv1;
  MPSCNNInstanceNormalizationNode *instNorm1;
  MPSCNNNeuronReLUNNode *relu1;

  MPSCNNConvolutionNode *conv2;
  MPSCNNInstanceNormalizationNode *instNorm2;
  MPSNNAdditionNode *add;
}
@end

@implementation TCMPSStyleTransferResidualNode : NSObject

- (instancetype) initWithParameters:(NSString *)name
                          inputNode:(MPSNNImageNode *)inputNode
                             device:(id<MTLDevice>)dev
                          cmd_queue:(id<MTLCommandQueue>)cmdQ
                         descriptor:(TCMPSResidualDescriptor *)descriptor
                        initWeights:(NSDictionary<NSString *, NSData *> *) weights {
  self = [super init];
  
  if (self) {
    conv1 = [MPSCNNConvolutionNode createConvolutional:inputNode
                                           kernelWidth:descriptor.conv_1.kernelWidth
                                          kernelHeight:descriptor.conv_1.kernelHeight
                                  inputFeatureChannels:descriptor.conv_1.inputFeatureChannels
                                 outputFeatureChannels:descriptor.conv_1.outputFeatureChannels
                                           strideWidth:descriptor.conv_1.strideWidth
                                          strideHeight:descriptor.conv_1.strideHeight
                                          paddingWidth:descriptor.conv_1.paddingWidth
                                         paddingHeight:descriptor.conv_1.paddingHeight
                                               weights:(float *)weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_conv_1_weights"]].bytes
                                                biases:(float *)weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_conv_1_biases"]].bytes
                                                 label:descriptor.conv_1.label
                                         updateWeights:descriptor.conv_1.updateWeights
                                                device:dev
                                             cmd_queue:cmdQ];

    instNorm1 = [MPSCNNConvolutionNode createInstanceNormalization:[conv1 resultImage]
                                                          channels:descriptor.inst_1.channels
                                                            styles:descriptor.inst_1.styles
                                                             gamma:(float *)weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_inst_1_gamma"]].bytes
                                                              beta:(float *)weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_inst_1_beta"]].bytes
                                                             label:descriptor.inst_1.label
                                                            device:dev
                                                         cmd_queue:cmdQ];

    relu1 = [MPSCNNNeuronReLUNNode nodeWithSource:[instNorm1 resultImage]];

    conv2 = [MPSCNNConvolutionNode createConvolutional:[relu1 resultImage]
                                           kernelWidth:descriptor.conv_2.kernelWidth
                                          kernelHeight:descriptor.conv_2.kernelHeight
                                  inputFeatureChannels:descriptor.conv_2.inputFeatureChannels
                                 outputFeatureChannels:descriptor.conv_2.outputFeatureChannels
                                           strideWidth:descriptor.conv_2.strideWidth
                                          strideHeight:descriptor.conv_2.strideHeight
                                          paddingWidth:descriptor.conv_2.paddingWidth
                                         paddingHeight:descriptor.conv_2.paddingHeight
                                               weights:(float *)weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_conv_2_weights"]].bytes
                                                biases:(float *)weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_conv_2_biases"]].bytes
                                                 label:descriptor.conv_2.label
                                         updateWeights:descriptor.conv_2.updateWeights
                                                device:dev
                                             cmd_queue:cmdQ];

    instNorm2 = [MPSCNNConvolutionNode createInstanceNormalization:[conv2 resultImage]
                                                          channels:descriptor.inst_2.channels
                                                            styles:descriptor.inst_2.styles
                                                             gamma:(float *)weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_inst_2_gamma"]].bytes
                                                              beta:(float *)weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_inst_2_beta"]].bytes
                                                             label:descriptor.inst_2.label
                                                            device:dev
                                                         cmd_queue:cmdQ];

    add = [MPSNNAdditionNode nodeWithSources:@[inputNode, [instNorm2 resultImage]]];

    _output = [add resultImage];
  }

  return self;
}

- (MPSNNImageNode *) backwardPass:(MPSNNImageNode *) inputNode {
  NSArray<MPSNNGradientFilterNode *>* addGrad = [add gradientFiltersWithSources: @[inputNode]];
  MPSNNGradientFilterNode* inst2Grad = [instNorm2 gradientFilterWithSource: [addGrad[0] resultImage]];
  MPSNNGradientFilterNode* conv2Grad = [conv2 gradientFilterWithSource: [inst2Grad resultImage]];
  MPSNNGradientFilterNode* relu1Grad = [relu1 gradientFilterWithSource: [conv2Grad resultImage]];
  MPSNNGradientFilterNode* inst1Grad = [instNorm1 gradientFilterWithSource: [relu1Grad resultImage]];;
  MPSNNGradientFilterNode* conv1Grad = [conv1 gradientFilterWithSource: [inst1Grad resultImage]];

  return [conv1Grad resultImage];
}

@end
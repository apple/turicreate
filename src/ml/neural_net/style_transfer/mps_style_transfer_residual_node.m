#include <ml/neural_net/style_transfer/mps_style_transfer_residual_node.h>
#include <ml/neural_net/mps_layer_helper.h>

@interface TCMPSStyleTransferResidualNode () {
  MPSCNNConvolutionNode *conv_1;
  MPSCNNInstanceNormalizationNode *inst_norm_1;
  MPSCNNNeuronReLUNNode *relu_1;

  MPSCNNConvolutionNode *conv_2;
  MPSCNNInstanceNormalizationNode *inst_norm_2;
  MPSNNAdditionNode *add;
}
@end

@implementation TCMPSStyleTransferResidualNode : NSObject

- (instancetype) initWithParameters:(NSString *)name
                          inputNode:(MPSNNImageNode *)inputNode
                             device:(id<MTLDevice>)dev
                          cmd_queue:(id<MTLCommandQueue>)cmd_q
                         descriptor:(TCMPSResidualDescriptor *)descriptor
                        initWeights:(NSDictionary<NSString *, NSData *> *) weights {
  self = [super init];
  
  if (self) {
    conv_1 = [MPSCNNConvolutionNode createConvolutional:inputNode
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
                                              cmd_queue:cmd_q];

    inst_norm_1 = [MPSCNNConvolutionNode createInstanceNormalization:[conv_1 resultImage]
                                                            channels:descriptor.inst_1.channels
                                                              styles:descriptor.inst_1.styles
                                                               gamma:(float *)weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_inst_1_gamma"]].bytes
                                                                beta:(float *)weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_inst_1_beta"]].bytes
                                                               label:descriptor.inst_1.label
                                                              device:dev
                                                           cmd_queue:cmd_q];

    relu_1 = [MPSCNNNeuronReLUNNode nodeWithSource:[inst_norm_1 resultImage]];

    conv_2 = [MPSCNNConvolutionNode createConvolutional:[relu_1 resultImage]
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
                                              cmd_queue:cmd_q];

    inst_norm_2 = [MPSCNNConvolutionNode createInstanceNormalization:[conv_2 resultImage]
                                                            channels:descriptor.inst_2.channels
                                                              styles:descriptor.inst_2.styles
                                                               gamma:(float *)weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_inst_2_gamma"]].bytes
                                                                beta:(float *)weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_inst_2_beta"]].bytes
                                                               label:descriptor.inst_2.label
                                                              device:dev
                                                           cmd_queue:cmd_q];

    add = [MPSNNAdditionNode nodeWithSources:@[inputNode, [inst_norm_2 resultImage]]];

    _m_output = [add resultImage];
  }

  return self;
}

- (MPSNNImageNode *) backwardPass:(MPSNNImageNode *) inputNode {
  NSArray<MPSNNGradientFilterNode *>* add_grad = [add gradientFiltersWithSources: @[inputNode]];
  MPSNNGradientFilterNode* inst_2_grad = [inst_norm_2 gradientFilterWithSource: [add_grad[0] resultImage]];
  MPSNNGradientFilterNode* conv_2_grad = [conv_2 gradientFilterWithSource: [inst_2_grad resultImage]];
  MPSNNGradientFilterNode* relu_1_grad = [relu_1 gradientFilterWithSource: [conv_2_grad resultImage]];
  MPSNNGradientFilterNode* inst_1_grad = [inst_norm_1 gradientFilterWithSource: [relu_1_grad resultImage]];;
  MPSNNGradientFilterNode* conv_1_grad = [conv_1 gradientFilterWithSource: [inst_1_grad resultImage]];

  return [conv_1_grad resultImage];
}

@end
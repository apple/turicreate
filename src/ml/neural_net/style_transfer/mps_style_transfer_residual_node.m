#import <ml/neural_net/style_transfer/mps_style_transfer_residual_node.h>
#import <ml/neural_net/mps_layer_helper.h>

@interface TCMPSStyleTransferResidualNode ()
@property (nonatomic) MPSCNNConvolutionNode *conv1;
@property (nonatomic) MPSCNNInstanceNormalizationNode *instNorm1;
@property (nonatomic) MPSCNNNeuronReLUNNode *relu1;

@property (nonatomic) MPSCNNConvolutionNode *conv2;
@property (nonatomic) MPSCNNInstanceNormalizationNode *instNorm2;
@property (nonatomic) MPSNNAdditionNode *add;
@end

@implementation TCMPSStyleTransferResidualNode : NSObject

- (instancetype) initWithParameters:(NSString *)name
                          inputNode:(MPSNNImageNode *)inputNode
                             device:(id<MTLDevice>)dev
                           cmdQueue:(id<MTLCommandQueue>)cmdQ
                         descriptor:(TCMPSResidualDescriptor *)descriptor
                        initWeights:(NSDictionary<NSString *, NSData *> *) weights {
  self = [super init];
  
  if (self) {
    _conv1 = [MPSCNNConvolutionNode createConvolutional:inputNode
                                            kernelWidth:descriptor.conv1.kernelWidth
                                           kernelHeight:descriptor.conv1.kernelHeight
                                   inputFeatureChannels:descriptor.conv1.inputFeatureChannels
                                  outputFeatureChannels:descriptor.conv1.outputFeatureChannels
                                            strideWidth:descriptor.conv1.strideWidth
                                           strideHeight:descriptor.conv1.strideHeight
                                           paddingWidth:descriptor.conv1.paddingWidth
                                          paddingHeight:descriptor.conv1.paddingHeight
                                                weights:weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_conv_1_weights"]]
                                                 biases:weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_conv_1_biases"]]
                                                  label:descriptor.conv1.label
                                          updateWeights:descriptor.conv1.updateWeights
                                                 device:dev
                                               cmdQueue:cmdQ];

    _instNorm1 = [MPSCNNConvolutionNode createInstanceNormalization:[_conv1 resultImage]
                                                           channels:descriptor.inst1.channels
                                                             styles:descriptor.inst1.styles
                                                              gamma:weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_inst_1_gamma"]]
                                                               beta:weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_inst_1_beta"]]
                                                              label:descriptor.inst1.label
                                                             device:dev
                                                           cmdQueue:cmdQ];

    _relu1 = [MPSCNNNeuronReLUNNode nodeWithSource:[_instNorm1 resultImage]];

    _conv2 = [MPSCNNConvolutionNode createConvolutional:[_relu1 resultImage]
                                            kernelWidth:descriptor.conv2.kernelWidth
                                           kernelHeight:descriptor.conv2.kernelHeight
                                   inputFeatureChannels:descriptor.conv2.inputFeatureChannels
                                  outputFeatureChannels:descriptor.conv2.outputFeatureChannels
                                            strideWidth:descriptor.conv2.strideWidth
                                           strideHeight:descriptor.conv2.strideHeight
                                           paddingWidth:descriptor.conv2.paddingWidth
                                          paddingHeight:descriptor.conv2.paddingHeight
                                                weights:weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_conv_2_weights"]]
                                                 biases:weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_conv_2_biases"]]
                                                  label:descriptor.conv2.label
                                          updateWeights:descriptor.conv2.updateWeights
                                                 device:dev
                                               cmdQueue:cmdQ];

    _instNorm2 = [MPSCNNConvolutionNode createInstanceNormalization:[_conv2 resultImage]
                                                           channels:descriptor.inst2.channels
                                                             styles:descriptor.inst2.styles
                                                              gamma:weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_inst_2_gamma"]]
                                                               beta:weights[[NSString stringWithFormat:@"%@/%@", name, @"residual_inst_2_beta"]]
                                                              label:descriptor.inst2.label
                                                             device:dev
                                                           cmdQueue:cmdQ]; 

    _add = [MPSNNAdditionNode nodeWithSources:@[inputNode, [_instNorm2 resultImage]]];

    _output = [_add resultImage];
  }

  return self;
}

- (MPSNNImageNode *) backwardPass:(MPSNNImageNode *) inputNode {
  NSArray<MPSNNGradientFilterNode *>* addGrad = [_add gradientFiltersWithSources: @[inputNode]];
  MPSNNGradientFilterNode* inst2Grad = [_instNorm2 gradientFilterWithSource: [addGrad[0] resultImage]];
  MPSNNGradientFilterNode* conv2Grad = [_conv2 gradientFilterWithSource: [inst2Grad resultImage]];
  MPSNNGradientFilterNode* relu1Grad = [_relu1 gradientFilterWithSource: [conv2Grad resultImage]];
  MPSNNGradientFilterNode* inst1Grad = [_instNorm1 gradientFilterWithSource: [relu1Grad resultImage]];;
  MPSNNGradientFilterNode* conv1Grad = [_conv1 gradientFilterWithSource: [inst1Grad resultImage]];

  return [conv1Grad resultImage];
}

@end
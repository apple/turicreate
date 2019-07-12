#include <ml/neural_net/style_transfer/mps_style_transfer_encoding_node.h>
#include <ml/neural_net/mps_layer_helper.h>

@implementation TCMPSStyleTransferEncodingNode

- (instancetype) initWithParameters:(NSString *)name
                          inputNode:(MPSNNImageNode *)inputNode
                             device:(id<MTLDevice>)dev
                          cmd_queue:(id<MTLCommandQueue>)cmd_q
                         descriptor:(TCMPSEncodingDescriptor *)descriptor
                        initWeights:(NSDictionary<NSString *, NSData *> *) weights {
  @autoreleasepool {
    
    self = [super init];

    MPSCNNConvolutionNode *conv = [MPSCNNConvolutionNode createConvolutional:inputNode
                                                            kernelWidth:descriptor.conv.kernelWidth
                                                           kernelHeight:descriptor.conv.kernelHeight
                                                   inputFeatureChannels:descriptor.conv.inputFeatureChannels
                                                  outputFeatureChannels:descriptor.conv.outputFeatureChannels
                                                            strideWidth:descriptor.conv.strideWidth
                                                           strideHeight:descriptor.conv.strideHeight
                                                           paddingWidth:descriptor.conv.paddingWidth
                                                          paddingHeight:descriptor.conv.paddingHeight
                                                                weights:(float *)weights[@"encoding_conv_weights"].bytes
                                                                 biases:(float *)weights[@"encoding_conv_biases"].bytes
                                                                  label:descriptor.conv.label
                                                          updateWeights:descriptor.conv.updateWeights
                                                                 device:dev
                                                              cmd_queue:cmd_q];

    MPSCNNInstanceNormalizationNode* inst_norm = [MPSCNNConvolutionNode createInstanceNormalization:[conv resultImage]
                                                                                      channels:descriptor.inst.channels
                                                                                        styles:descriptor.inst.styles
                                                                                         gamma:(float *)weights[@"encoding_inst_gamma"].bytes
                                                                                          beta:(float *)weights[@"encoding_inst_beta"].bytes
                                                                                         label:descriptor.inst.label
                                                                                        device:dev
                                                                                     cmd_queue:cmd_q];

    MPSCNNNeuronReLUNNode* relu = [MPSCNNNeuronReLUNNode nodeWithSource: [inst_norm resultImage]];

    _m_output = [relu resultImage];

    return self;
  }
}

@end
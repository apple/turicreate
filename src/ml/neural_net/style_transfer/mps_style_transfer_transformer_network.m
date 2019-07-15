#import <ml/neural_net/style_transfer/mps_style_transfer_decoding_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_encoding_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_residual_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_transformer_network.h>
#import <ml/neural_net/mps_layer_helper.h>

@interface TCMPSStyleTransferTransformerNetwork () {
  TCMPSStyleTransferEncodingNode *encoding1;
  TCMPSStyleTransferEncodingNode *encoding2;
  TCMPSStyleTransferEncodingNode *encoding3;

  TCMPSStyleTransferResidualNode *residual1;
  TCMPSStyleTransferResidualNode *residual2;
  TCMPSStyleTransferResidualNode *residual3;
  TCMPSStyleTransferResidualNode *residual4;
  TCMPSStyleTransferResidualNode *residual5;

  TCMPSStyleTransferDecodingNode *decoding1;
  TCMPSStyleTransferDecodingNode *decoding2;

  MPSCNNConvolutionNode *conv;
  MPSCNNInstanceNormalizationNode *instNorm;
  MPSCNNNeuronSigmoidNode *sigmoid;
}
@end

@implementation TCMPSStyleTransferTransformerNetwork : NSObject

- (instancetype) initWithParameters:(NSString *)name
                          inputNode:(MPSNNImageNode *)inputNode
                             device:(id<MTLDevice>)dev
                          cmd_queue:(id<MTLCommandQueue>)cmdQ
                         descriptor:(TCMPSTransformerDescriptor *)descriptor
                        initWeights:(NSDictionary<NSString *, NSDictionary *> *) weights {
  self = [super init];
  
  if (self) {
    encoding1 = [[TCMPSStyleTransferEncodingNode alloc] initWithParameters:@"transformer_encode_1"
                                                                 inputNode:inputNode
                                                                    device:dev
                                                                 cmd_queue:cmdQ
                                                                descriptor:descriptor.encode_1
                                                               initWeights:weights[@"transformer_encode_1"]];

    encoding2 = [[TCMPSStyleTransferEncodingNode alloc] initWithParameters:@"transformer_encode_2"
                                                                 inputNode:[encoding1 output]
                                                                    device:dev
                                                                 cmd_queue:cmdQ
                                                                descriptor:descriptor.encode_2
                                                               initWeights:weights[@"transformer_encode_2"]];

    encoding3 = [[TCMPSStyleTransferEncodingNode alloc] initWithParameters:@"transformer_encode_3"
                                                                 inputNode:[encoding2 output]
                                                                    device:dev
                                                                 cmd_queue:cmdQ
                                                                descriptor:descriptor.encode_3
                                                               initWeights:weights[@"transformer_encode_3"]];

    residual1 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_1"
                                                                 inputNode:[encoding3 output]
                                                                    device:dev
                                                                 cmd_queue:cmdQ
                                                                descriptor:descriptor.residual_1
                                                               initWeights:weights[@"transformer_residual_1"]];

    residual2 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_2"
                                                                 inputNode:[residual1 output]
                                                                    device:dev
                                                                 cmd_queue:cmdQ
                                                                descriptor:descriptor.residual_2
                                                               initWeights:weights[@"transformer_residual_2"]];

    residual3 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_3"
                                                                 inputNode:[residual2 output]
                                                                    device:dev
                                                                 cmd_queue:cmdQ
                                                                descriptor:descriptor.residual_3
                                                               initWeights:weights[@"transformer_residual_3"]];

    residual4 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_4"
                                                                 inputNode:[residual3 output]
                                                                    device:dev
                                                                 cmd_queue:cmdQ
                                                                descriptor:descriptor.residual_4
                                                               initWeights:weights[@"transformer_residual_4"]];

    residual5 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_5"
                                                                 inputNode:[residual4 output]
                                                                    device:dev
                                                                 cmd_queue:cmdQ
                                                                descriptor:descriptor.residual_5
                                                               initWeights:weights[@"transformer_residual_5"]];

    decoding1 = [[TCMPSStyleTransferDecodingNode alloc] initWithParameters:@"transformer_decoding_1"
                                                                 inputNode:[residual5 output]
                                                                    device:dev
                                                                 cmd_queue:cmdQ
                                                                descriptor:descriptor.decode_1
                                                               initWeights:weights[@"transformer_decoding_1"]];

    decoding2 = [[TCMPSStyleTransferDecodingNode alloc] initWithParameters:@"transformer_decoding_2"
                                                                 inputNode:[decoding1 output]
                                                                    device:dev
                                                                 cmd_queue:cmdQ
                                                                descriptor:descriptor.decode_2
                                                               initWeights:weights[@"transformer_decoding_2"]];

    
    conv = [MPSCNNConvolutionNode createConvolutional:[decoding2 output]
                                          kernelWidth:descriptor.conv.kernelWidth
                                         kernelHeight:descriptor.conv.kernelHeight
                                 inputFeatureChannels:descriptor.conv.inputFeatureChannels
                                outputFeatureChannels:descriptor.conv.outputFeatureChannels
                                          strideWidth:descriptor.conv.strideWidth
                                         strideHeight:descriptor.conv.strideHeight
                                         paddingWidth:descriptor.conv.paddingWidth
                                        paddingHeight:descriptor.conv.paddingHeight
                                              weights:(float *)((NSData *)weights[@"transformer_conv"][@"weights"]).bytes
                                               biases:(float *)((NSData *)weights[@"transformer_conv"][@"biases"]).bytes
                                                label:descriptor.conv.label
                                        updateWeights:descriptor.conv.updateWeights
                                               device:dev
                                            cmd_queue:cmdQ];

    instNorm = [MPSCNNConvolutionNode createInstanceNormalization:[conv resultImage]
                                                         channels:descriptor.inst.channels
                                                           styles:descriptor.inst.styles
                                                            gamma:(float *)((NSData *)weights[@"transformer_inst"][@"weights"]).bytes
                                                             beta:(float *)((NSData *)weights[@"transformer_inst"][@"biases"]).bytes
                                                            label:descriptor.inst.label
                                                           device:dev
                                                        cmd_queue:cmdQ];

    sigmoid = [MPSCNNNeuronSigmoidNode nodeWithSource:[instNorm resultImage]];
    
    _forwardPass = [sigmoid resultImage];
  }

  return self;
}

- (MPSNNImageNode *) backwardPass:(MPSNNImageNode *) inputNode {
  MPSNNGradientFilterNode* sigmoidGrad = [sigmoid gradientFilterWithSource: inputNode];
  MPSNNGradientFilterNode* instanceNormGrad = [instNorm gradientFilterWithSource: [sigmoidGrad resultImage]];
  MPSNNGradientFilterNode* convGrad = [conv gradientFilterWithSource: [instanceNormGrad resultImage]];

  MPSNNImageNode* decoding2Img = [decoding2 backwardPass:[convGrad resultImage]];
  MPSNNImageNode* decoding1Img = [decoding1 backwardPass:decoding2Img];

  MPSNNImageNode* residual5Img = [residual5 backwardPass:decoding1Img];
  MPSNNImageNode* residual4Img = [residual4 backwardPass:residual5Img];
  MPSNNImageNode* residual3Img = [residual3 backwardPass:residual4Img];
  MPSNNImageNode* residual2Img = [residual2 backwardPass:residual3Img];
  MPSNNImageNode* residual1Img = [residual1 backwardPass:residual2Img];

  MPSNNImageNode* encoding3Grad = [encoding3 backwardPass:residual1Img];
  MPSNNImageNode* encoding2Grad = [encoding2 backwardPass:encoding3Grad];
  MPSNNImageNode* encoding1Grad = [encoding1 backwardPass:encoding2Grad];
  
  return encoding1Grad;
}

@end
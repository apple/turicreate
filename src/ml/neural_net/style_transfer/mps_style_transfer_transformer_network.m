#import <ml/neural_net/style_transfer/mps_style_transfer_decoding_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_encoding_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_residual_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_transformer_network.h>
#import <ml/neural_net/mps_layer_helper.h>

@interface TCMPSStyleTransferTransformerNetwork ()
@property (nonatomic) TCMPSStyleTransferEncodingNode *encoding1;
@property (nonatomic) TCMPSStyleTransferEncodingNode *encoding2;
@property (nonatomic) TCMPSStyleTransferEncodingNode *encoding3;

@property (nonatomic) TCMPSStyleTransferResidualNode *residual1;
@property (nonatomic) TCMPSStyleTransferResidualNode *residual2;
@property (nonatomic) TCMPSStyleTransferResidualNode *residual3;
@property (nonatomic) TCMPSStyleTransferResidualNode *residual4;
@property (nonatomic) TCMPSStyleTransferResidualNode *residual5;

@property (nonatomic) TCMPSStyleTransferDecodingNode *decoding1;
@property (nonatomic) TCMPSStyleTransferDecodingNode *decoding2;

@property (nonatomic) MPSCNNConvolutionNode *conv;
@property (nonatomic) MPSCNNInstanceNormalizationNode *instNorm;
@property (nonatomic) MPSCNNNeuronSigmoidNode *sigmoid;
@end

@implementation TCMPSStyleTransferTransformerNetwork : NSObject

- (instancetype) initWithParameters:(NSString *)name
                          inputNode:(MPSNNImageNode *)inputNode
                             device:(id<MTLDevice>)dev
                           cmdQueue:(id<MTLCommandQueue>)cmdQ
                         descriptor:(TCMPSTransformerDescriptor *)descriptor
                        initWeights:(NSDictionary<NSString *, NSDictionary *> *) weights {
  self = [super init];
  
  if (self) {
    _encoding1 = [[TCMPSStyleTransferEncodingNode alloc] initWithParameters:@"transformer_encode_1"
                                                                  inputNode:inputNode
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.encode1
                                                                initWeights:weights[@"transformer_encode_1"]];

    _encoding2 = [[TCMPSStyleTransferEncodingNode alloc] initWithParameters:@"transformer_encode_2"
                                                                  inputNode:[_encoding1 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.encode2
                                                                initWeights:weights[@"transformer_encode_2"]];

    _encoding3 = [[TCMPSStyleTransferEncodingNode alloc] initWithParameters:@"transformer_encode_3"
                                                                  inputNode:[_encoding2 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.encode3
                                                                initWeights:weights[@"transformer_encode_3"]];

    _residual1 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_1"
                                                                  inputNode:[_encoding3 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.residual1
                                                                initWeights:weights[@"transformer_residual_1"]];

    _residual2 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_2"
                                                                  inputNode:[_residual1 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.residual2
                                                                initWeights:weights[@"transformer_residual_2"]];

    _residual3 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_3"
                                                                  inputNode:[_residual2 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.residual3
                                                                initWeights:weights[@"transformer_residual_3"]];

    _residual4 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_4"
                                                                  inputNode:[_residual3 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.residual4
                                                                initWeights:weights[@"transformer_residual_4"]];

    _residual5 = [[TCMPSStyleTransferResidualNode alloc] initWithParameters:@"transformer_residual_5"
                                                                  inputNode:[_residual4 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.residual5
                                                                initWeights:weights[@"transformer_residual_5"]];

    _decoding1 = [[TCMPSStyleTransferDecodingNode alloc] initWithParameters:@"transformer_decoding_1"
                                                                 inputNode:[_residual5 output]
                                                                    device:dev
                                                                  cmdQueue:cmdQ
                                                                descriptor:descriptor.decode1
                                                               initWeights:weights[@"transformer_decoding_1"]];

    _decoding2 = [[TCMPSStyleTransferDecodingNode alloc] initWithParameters:@"transformer_decoding_2"
                                                                  inputNode:[_decoding1 output]
                                                                     device:dev
                                                                   cmdQueue:cmdQ
                                                                 descriptor:descriptor.decode2
                                                                initWeights:weights[@"transformer_decoding_2"]];

    
    _conv = [MPSCNNConvolutionNode createConvolutional:[_decoding2 output]
                                           kernelWidth:descriptor.conv.kernelWidth
                                          kernelHeight:descriptor.conv.kernelHeight
                                  inputFeatureChannels:descriptor.conv.inputFeatureChannels
                                 outputFeatureChannels:descriptor.conv.outputFeatureChannels
                                           strideWidth:descriptor.conv.strideWidth
                                          strideHeight:descriptor.conv.strideHeight
                                          paddingWidth:descriptor.conv.paddingWidth
                                         paddingHeight:descriptor.conv.paddingHeight
                                               weights:((NSData *)weights[@"transformer_conv"][@"weights"])
                                                biases:((NSData *)weights[@"transformer_conv"][@"biases"])
                                                 label:descriptor.conv.label
                                         updateWeights:descriptor.conv.updateWeights
                                                device:dev
                                              cmdQueue:cmdQ];

    _instNorm = [MPSCNNConvolutionNode createInstanceNormalization:[_conv resultImage]
                                                          channels:descriptor.inst.channels
                                                            styles:descriptor.inst.styles
                                                             gamma:(NSData *)weights[@"transformer_inst"][@"weights"]
                                                              beta:(NSData *)weights[@"transformer_inst"][@"biases"]
                                                             label:descriptor.inst.label
                                                            device:dev
                                                          cmdQueue:cmdQ];

    _sigmoid = [MPSCNNNeuronSigmoidNode nodeWithSource:[_instNorm resultImage]];
    
    _forwardPass = [_sigmoid resultImage];
  }

  return self;
}

- (MPSNNImageNode *) backwardPass:(MPSNNImageNode *) inputNode {
  MPSNNGradientFilterNode* sigmoidGrad = [_sigmoid gradientFilterWithSource: inputNode];
  MPSNNGradientFilterNode* instanceNormGrad = [_instNorm gradientFilterWithSource: [sigmoidGrad resultImage]];
  MPSNNGradientFilterNode* convGrad = [_conv gradientFilterWithSource: [instanceNormGrad resultImage]];

  MPSNNImageNode* decoding2Img = [_decoding2 backwardPass:[convGrad resultImage]];
  MPSNNImageNode* decoding1Img = [_decoding1 backwardPass:decoding2Img];

  MPSNNImageNode* residual5Img = [_residual5 backwardPass:decoding1Img];
  MPSNNImageNode* residual4Img = [_residual4 backwardPass:residual5Img];
  MPSNNImageNode* residual3Img = [_residual3 backwardPass:residual4Img];
  MPSNNImageNode* residual2Img = [_residual2 backwardPass:residual3Img];
  MPSNNImageNode* residual1Img = [_residual1 backwardPass:residual2Img];

  MPSNNImageNode* encoding3Grad = [_encoding3 backwardPass:residual1Img];
  MPSNNImageNode* encoding2Grad = [_encoding2 backwardPass:encoding3Grad];
  MPSNNImageNode* encoding1Grad = [_encoding1 backwardPass:encoding2Grad];
  
  return encoding1Grad;
}

@end
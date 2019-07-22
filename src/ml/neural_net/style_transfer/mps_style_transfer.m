/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <ml/neural_net/mps_device_manager.h>
#import <ml/neural_net/mps_layer_helper.h>
#import <ml/neural_net/mps_node_handle.h>

#import <ml/neural_net/style_transfer/mps_style_transfer.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_utils.h>

#import <ml/neural_net/style_transfer/mps_style_transfer_transformer_network.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_vgg_16_network.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_pre_processing.h>

@interface TCMPSStyleTransfer()
@property (nonatomic) TCMPSStyleTransferTransformerNetwork *model;

@property (nonatomic) TCMPSStyleTransferPreProcessing *contentPreProcess;
@property (nonatomic) TCMPSStyleTransferPreProcessing *stylePreProcessLoss;
@property (nonatomic) TCMPSStyleTransferPreProcessing *contentPreProcessLoss;

@property (nonatomic) TCMPSVgg16Network *contentVgg;
@property (nonatomic) TCMPSVgg16Network *styleVggLoss;
@property (nonatomic) TCMPSVgg16Network *contentVggLoss;

@property (nonatomic) MPSNNImageNode *contentNode;
@property (nonatomic) MPSNNImageNode *contentScaleNode;
@property (nonatomic) MPSNNImageNode *contenMeanNode;

@property (nonatomic) MPSNNImageNode *styleNode;
@property (nonatomic) MPSNNImageNode *styleScaleNode;
@property (nonatomic) MPSNNImageNode *styleMeanNode;

@property (nonatomic) MPSNNGraph *trainingGraph;
@property (nonatomic) MPSNNGraph *inferenceGraph;

@property (nonatomic) id<MTLDevice> dev;
@property (nonatomic) id<MTLCommandQueue> commandQueue;
@end

@implementation TCMPSStyleTransfer
- (instancetype) initWithParameters:(NSDictionary<NSString *, NSData *> *)weights
                          numStyles:(NSUInteger)numStyles {
  self = [super init];
  if (self) {
    // Set Default parameters values

    _batchSize = 6;
    _contentLossMultiplier = [NSNumber numberWithFloat:1.0];
    _styleLossMultiplier = [NSNumber numberWithFloat:1e-4];
    _finetuneAllParams = YES;

    // Create the loss descriptors for the style and content images
    MPSCNNLossDescriptor *styleDesc = [MPSCNNLossDescriptor cnnLossDescriptorWithType:MPSCNNLossTypeMeanSquaredError
                                                                         reductionType:MPSCNNReductionTypeMean];

    styleDesc.weight = 0.5 * [_styleLossMultiplier floatValue];

    MPSCNNLossDescriptor *contentDesc = [MPSCNNLossDescriptor cnnLossDescriptorWithType:MPSCNNLossTypeMeanSquaredError
                                                                           reductionType:MPSCNNReductionTypeMean];

    contentDesc.weight = 0.5 * [_contentLossMultiplier floatValue];

    // Create Proper Input Nodes
    _contentNode = [MPSNNImageNode nodeWithHandle: [[TCMPSGraphNodeHandle alloc] initWithLabel:@"contentImage"]];
    _contentScaleNode = [MPSNNImageNode nodeWithHandle: [[TCMPSGraphNodeHandle alloc] initWithLabel:@"contentScaleImage"]];
    _contenMeanNode = [MPSNNImageNode nodeWithHandle: [[TCMPSGraphNodeHandle alloc] initWithLabel:@"contentMeanImage"]];

    _styleNode = [MPSNNImageNode nodeWithHandle: [[TCMPSGraphNodeHandle alloc] initWithLabel:@"styleImage"]];
    _styleScaleNode = [MPSNNImageNode nodeWithHandle: [[TCMPSGraphNodeHandle alloc] initWithLabel:@"styleScaleImage"]];
    _styleMeanNode = [MPSNNImageNode nodeWithHandle: [[TCMPSGraphNodeHandle alloc] initWithLabel:@"styleMeanImage"]];
    

    // Create the Descriptors for the Transformer and VGG16 Networks
    TCMPSTransformerDescriptor *transformerDesc = [TCMPSStyleTransfer defineTransformerDescriptor:numStyles
                                                                                   tuneAllWeights:_finetuneAllParams];
    TCMPSVgg16Descriptor *vgg16Desc = [TCMPSStyleTransfer defineVGG16Descriptor:numStyles];

    // Allocate a _dev and _commandQueue
    _dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
    _commandQueue = [_dev newCommandQueue];

    NSDictionary<NSString *, NSDictionary *> *transformerWeights = [TCMPSStyleTransfer defineTransformerWeights:weights];
    NSDictionary<NSString *, NSDictionary *> *vgg16Weights = [TCMPSStyleTransfer defineVGG16:weights];

    _model = [[TCMPSStyleTransferTransformerNetwork alloc] initWithParameters:@"Transformer"
                                                                    inputNode:_contentNode
                                                                       device:_dev
                                                                     cmdQueue:_commandQueue
                                                                   descriptor:transformerDesc
                                                                  initWeights:transformerWeights];

    _contentPreProcess = [[TCMPSStyleTransferPreProcessing alloc] initWithParameters:@"Content_Pre_Processing"
                                                                           inputNode:_model.forwardPass
                                                                           scaleNode:_contentScaleNode
                                                                            meanNode:_contenMeanNode];
    
    _contentVgg = [[TCMPSVgg16Network alloc] initWithParameters:@"Content_VGG_16"
                                                      inputNode:_contentPreProcess.output
                                                         device:_dev
                                                       cmdQueue:_commandQueue
                                                     descriptor:vgg16Desc
                                                    initWeights:vgg16Weights];

    _stylePreProcessLoss = [[TCMPSStyleTransferPreProcessing alloc] initWithParameters:@"Style_Pre_Processing"
                                                                             inputNode:_styleNode
                                                                             scaleNode:_styleScaleNode
                                                                              meanNode:_styleMeanNode];

    _styleVggLoss = [[TCMPSVgg16Network alloc] initWithParameters:@"Style_VGG_16"
                                                        inputNode:_stylePreProcessLoss.output
                                                           device:_dev
                                                         cmdQueue:_commandQueue
                                                       descriptor:vgg16Desc
                                                      initWeights:vgg16Weights];

    _contentPreProcessLoss = [[TCMPSStyleTransferPreProcessing alloc] initWithParameters:@"Content_Loss_Pre_Processing"
                                                                               inputNode:_contentNode
                                                                               scaleNode:_contentScaleNode
                                                                                meanNode:_contenMeanNode];

    _contentVggLoss = [[TCMPSVgg16Network alloc] initWithParameters:@"Content_Loss_VGG_16"
                                                          inputNode:_contentPreProcessLoss.output
                                                             device:_dev
                                                           cmdQueue:_commandQueue
                                                         descriptor:vgg16Desc
                                                        initWeights:vgg16Weights];

    NSUInteger DEFAULT_IMAGE_SIZE = 256;

    NSUInteger gramScaling1 = (DEFAULT_IMAGE_SIZE * DEFAULT_IMAGE_SIZE);
    NSUInteger gramScaling2 = ((DEFAULT_IMAGE_SIZE/2) * (DEFAULT_IMAGE_SIZE/2));
    NSUInteger gramScaling3 = ((DEFAULT_IMAGE_SIZE/4) * (DEFAULT_IMAGE_SIZE/4));
    NSUInteger gramScaling4 = ((DEFAULT_IMAGE_SIZE/8) * (DEFAULT_IMAGE_SIZE/8));

    MPSNNGramMatrixCalculationNode *gramMatrixStyleLossFirstReLU
      = [MPSNNGramMatrixCalculationNode nodeWithSource:_styleVggLoss.reluOut1
                                                 alpha:(1.0/gramScaling1)];

    MPSNNGramMatrixCalculationNode *gramMatrixContentVggFirstReLU
      = [MPSNNGramMatrixCalculationNode nodeWithSource:_contentVgg.reluOut1
                                                 alpha:(1.0/gramScaling1)];

    MPSNNForwardLossNode *styleLossNode1 
      = [MPSNNForwardLossNode nodeWithSource:gramMatrixContentVggFirstReLU.resultImage
                                      labels:gramMatrixStyleLossFirstReLU.resultImage
                              lossDescriptor:styleDesc];

    MPSNNGramMatrixCalculationNode *gramMatrixStyleLossSecondReLU
      = [MPSNNGramMatrixCalculationNode nodeWithSource:_styleVggLoss.reluOut2
                                                 alpha:(1.0/gramScaling2)];

    MPSNNGramMatrixCalculationNode *gramMatrixContentVggSecondReLU
      = [MPSNNGramMatrixCalculationNode nodeWithSource:_contentVgg.reluOut2
                                                 alpha:(1.0/gramScaling2)];

    MPSNNForwardLossNode *styleLossNode2
      = [MPSNNForwardLossNode nodeWithSource:gramMatrixContentVggSecondReLU.resultImage
                                      labels:gramMatrixStyleLossSecondReLU.resultImage
                              lossDescriptor:styleDesc];

    MPSNNGramMatrixCalculationNode *gramMatrixStyleLossThirdReLU
      = [MPSNNGramMatrixCalculationNode nodeWithSource:_styleVggLoss.reluOut3
                                                 alpha:(1.0/gramScaling3)];

    MPSNNGramMatrixCalculationNode *gramMatrixContentVggThirdReLU
      = [MPSNNGramMatrixCalculationNode nodeWithSource:_contentVgg.reluOut3
                                                 alpha:(1.0/gramScaling3)];

    MPSNNForwardLossNode *styleLossNode3
      = [MPSNNForwardLossNode nodeWithSource:gramMatrixContentVggThirdReLU.resultImage
                                      labels:gramMatrixStyleLossThirdReLU.resultImage
                              lossDescriptor:styleDesc];

    MPSNNGramMatrixCalculationNode *gramMatrixStyleLossFourthReLU
      = [MPSNNGramMatrixCalculationNode nodeWithSource:_styleVggLoss.reluOut4
                                                 alpha:(1.0/gramScaling4)];

    MPSNNGramMatrixCalculationNode *gramMatrixContentVggFourthReLU
      = [MPSNNGramMatrixCalculationNode nodeWithSource:_contentVgg.reluOut4
                                                 alpha:(1.0/gramScaling4)];

    MPSNNForwardLossNode *styleLossNode4 
      = [MPSNNForwardLossNode nodeWithSource:gramMatrixContentVggFourthReLU.resultImage
                                      labels:gramMatrixStyleLossFourthReLU.resultImage
                              lossDescriptor:styleDesc];

    MPSNNForwardLossNode *contentLossNode
      = [MPSNNForwardLossNode nodeWithSource:_contentVgg.reluOut3
                                      labels:_contentVggLoss.reluOut3
                              lossDescriptor:contentDesc];

     MPSNNAdditionNode* addLossStyle1Style2
      = [MPSNNAdditionNode nodeWithSources:@[styleLossNode1.resultImage,
                                             styleLossNode2.resultImage]];

    MPSNNAdditionNode* addLossStyle3Style4
      = [MPSNNAdditionNode nodeWithSources:@[styleLossNode3.resultImage,
                                             styleLossNode4.resultImage]];

    MPSNNAdditionNode* addTotalStyleLoss 
      = [MPSNNAdditionNode nodeWithSources:@[addLossStyle1Style2.resultImage,
                                             addLossStyle3Style4.resultImage]];

    MPSNNAdditionNode* totalLoss
      = [MPSNNAdditionNode nodeWithSources:@[addTotalStyleLoss.resultImage,
                                             contentLossNode.resultImage]];

    MPSNNInitialGradientNode *initialGradient
      = [MPSNNInitialGradientNode nodeWithSource:totalLoss.resultImage];

    BOOL resultsAreNeeded[] = { YES, YES };

    NSArray<MPSNNFilterNode*>* lastNodes 
      = [initialGradient trainingGraphWithSourceGradient:initialGradient.resultImage
                                             nodeHandler: nil];

    _trainingGraph = [MPSNNGraph graphWithDevice:_dev
                                    resultImages:@[lastNodes[0].resultImage, lastNodes[1].resultImage]
                                resultsAreNeeded:&resultsAreNeeded[0]];

    _inferenceGraph = [MPSNNGraph graphWithDevice:_dev
                                      resultImage:_model.forwardPass
                              resultImageIsNeeded:YES];

    _trainingGraph.format = MPSImageFeatureChannelFormatFloat32;
    _inferenceGraph.format = MPSImageFeatureChannelFormatFloat32;
  }
  return self;
}
 
- (NSDictionary<NSString *, NSData *> *)exportWeights {
  // TODO: export weights
  NSDictionary<NSString *, NSData *> *weights;
  return weights;
}

- (NSDictionary<NSString *, NSData *> *)predict:(NSDictionary<NSString *, NSData *> *)inputs {
  // TODO: export weights
  NSDictionary<NSString *, NSData *> *weights;
  return weights;
}

- (void) setLearningRate:(NSNumber *)lr {

}

- (NSDictionary<NSString *, NSData *> *) train:(NSDictionary<NSString *, NSData *> *)inputs {
  // TODO: export weights
  NSDictionary<NSString *, NSData *> *weights;
  return weights;
}

@end
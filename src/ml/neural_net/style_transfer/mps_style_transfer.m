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

    totalLoss.resultImage.handle = [[TCMPSGraphNodeHandle alloc] initWithLabel:@"totalLossValue"];
    totalLoss.resultImage.exportFromGraph = YES;
    totalLoss.resultImage.synchronizeResource = YES;
    totalLoss.resultImage.imageAllocator = [MPSImage defaultAllocator];

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
  NSUInteger contentWidth;
  NSUInteger contentHeight;
  NSUInteger contentChannels;

  [inputs[@"imageWidth"] getBytes:&contentWidth length:sizeof(contentWidth)];
  [inputs[@"imageHeight"] getBytes:&contentHeight length:sizeof(contentHeight)];
  [inputs[@"imageChannels"] getBytes:&contentChannels length:sizeof(contentChannels)];

  MPSImageDescriptor *imgDesc = [MPSImageDescriptor
    imageDescriptorWithChannelFormat:MPSImageFeatureChannelFormatFloat32
                               width:contentWidth
                              height:contentHeight
                     featureChannels:contentChannels
                      numberOfImages:1
                               usage:MTLTextureUsageShaderWrite | MTLTextureUsageShaderRead];

  NSMutableArray<MPSImage *> *contentImageArray;

  for (NSUInteger index = 0; index < _batchSize; index++) {
    NSString* key = [NSString stringWithFormat:@"%@%lu", @"contentImage", index];
    MPSImage *contentImage = [[MPSImage alloc] initWithDevice:_dev imageDescriptor:imgDesc];
    [contentImage writeBytes:inputs[key].bytes dataLayout:(MPSDataLayoutHeightxWidthxFeatureChannels)imageIndex:0];
    [contentImageArray addObject:contentImage];
  }

  MPSImageBatch *contentImageBatch = [contentImageArray copy];  

  id<MTLCommandBuffer> cb = [_commandQueue commandBuffer];

  NSMutableArray *intermediateImages = [[NSMutableArray alloc] initWithArray:@[]];
  NSMutableArray *destinationStates = [[NSMutableArray alloc] initWithArray:@[]];

  MPSImageBatch *stylizedImages =  [_inferenceGraph encodeBatchToCommandBuffer:cb
                                                       sourceImages:@[contentImageBatch]
                                                       sourceStates:nil
                                                 intermediateImages:intermediateImages
                                                  destinationStates:destinationStates];

  for (MPSImage *image in stylizedImages) {
    [image synchronizeOnCommandBuffer:cb];  
  }

  [cb commit];
  [cb waitUntilCompleted];

  NSMutableDictionary<NSString *, NSData *> *imagesOut;

  for (MPSImage *image in stylizedImages) {
    NSMutableData* styleData = [NSMutableData dataWithLength:(NSUInteger)sizeof(float) * contentWidth * contentHeight * contentChannels];
    [image readBytes:styleData.mutableBytes dataLayout:(MPSDataLayoutHeightxWidthxFeatureChannels)imageIndex:0];
    NSString* key = [NSString stringWithFormat:@"%@%lu", @"stylizedImage", [stylizedImages indexOfObject:image]];
    imagesOut[key] = styleData;
  }

  return [imagesOut copy];
}

- (void) setLearningRate:(NSNumber *)lr {
  [_model setLearningRate:[lr floatValue]];
  [_contentVgg setLearningRate:[lr floatValue]];
  [_styleVggLoss setLearningRate:[lr floatValue]];
  [_contentVggLoss setLearningRate:[lr floatValue]];
}

- (NSDictionary<NSString *, NSData *> *) train:(NSDictionary<NSString *, NSData *> *)inputs {
  NSUInteger inputWidth;
  NSUInteger inputHeight;
  NSUInteger inputChannels;

  [inputs[@"imageWidth"] getBytes:&inputWidth length:sizeof(inputWidth)];
  [inputs[@"imageHeight"] getBytes:&inputHeight length:sizeof(inputHeight)];
  [inputs[@"imageChannels"] getBytes:&inputChannels length:sizeof(inputChannels)];

  NSUInteger imageSize = inputWidth * inputHeight * inputChannels;

  NSMutableData* mean = [NSMutableData dataWithCapacity:(NSUInteger)sizeof(float) * imageSize];
  NSMutableData* multiplication = [NSMutableData dataWithCapacity:(NSUInteger)sizeof(float) * imageSize];
  
  MPSImageDescriptor *imgDesc = [MPSImageDescriptor
    imageDescriptorWithChannelFormat:MPSImageFeatureChannelFormatFloat32
                               width:inputWidth
                              height:inputHeight
                     featureChannels:inputChannels
                      numberOfImages:1
                               usage:MTLTextureUsageShaderWrite | MTLTextureUsageShaderRead];

  [TCMPSStyleTransfer populateMean:mean];
  [TCMPSStyleTransfer populateMultiplication:multiplication];

  NSMutableArray<MPSImage *> *contentImageArray;
  NSMutableArray<MPSImage *> *contentMeanArray;
  NSMutableArray<MPSImage *> *contentMultiplicationArray;

  NSMutableArray<MPSImage *> *styleImageArray;
  NSMutableArray<MPSImage *> *styleMeanArray;
  NSMutableArray<MPSImage *> *styleMultiplicationArray;

  for (NSUInteger index = 0; index < _batchSize; index++) {
    // TODO: populate contentImage
    NSString* contentKey = [NSString stringWithFormat:@"%@%lu", @"contentImage", index];
    NSString* styleKey = [NSString stringWithFormat:@"%@%lu", @"styleImage", index];
    
    MPSImage *contentImage = [[MPSImage alloc] initWithDevice:_dev imageDescriptor:imgDesc];
    [contentImage writeBytes:inputs[contentKey].bytes dataLayout:(MPSDataLayoutHeightxWidthxFeatureChannels)imageIndex:0];
    [contentImageArray addObject:contentImage];

    MPSImage *contentMean = [[MPSImage alloc] initWithDevice:_dev imageDescriptor:imgDesc];
    [contentMean writeBytes:mean.bytes dataLayout:(MPSDataLayoutHeightxWidthxFeatureChannels)imageIndex:0];
    [contentMeanArray addObject:contentMean];

    MPSImage *contentMultiplication = [[MPSImage alloc] initWithDevice:_dev imageDescriptor:imgDesc];
    [contentMultiplication writeBytes:multiplication.bytes dataLayout:(MPSDataLayoutHeightxWidthxFeatureChannels)imageIndex:0]; 
    [contentMultiplicationArray addObject:contentMultiplication];

    MPSImage *syleImage = [[MPSImage alloc] initWithDevice:_dev imageDescriptor:imgDesc];
    [syleImage writeBytes:inputs[styleKey].bytes dataLayout:(MPSDataLayoutHeightxWidthxFeatureChannels)imageIndex:0];
    [styleImageArray addObject:syleImage];

    MPSImage *styleMean = [[MPSImage alloc] initWithDevice:_dev imageDescriptor:imgDesc];
    [styleMean writeBytes:mean.bytes dataLayout:(MPSDataLayoutHeightxWidthxFeatureChannels)imageIndex:0];
    [styleMeanArray addObject:styleMean];

    MPSImage *styleMultiplication = [[MPSImage alloc] initWithDevice:_dev imageDescriptor:imgDesc];
    [styleMultiplication writeBytes:multiplication.bytes dataLayout:(MPSDataLayoutHeightxWidthxFeatureChannels)imageIndex:0];
    [styleMultiplicationArray addObject:styleMultiplication];
  }
  
  MPSImageBatch *contentImageBatch = [contentImageArray copy];  
  MPSImageBatch *contentMeanBatch = [contentMeanArray copy];  
  MPSImageBatch *contentMultiplicationBatch = [contentMultiplicationArray copy];  

  MPSImageBatch *styleImageBatch = [styleImageArray copy];  
  MPSImageBatch *styleMeanBatch = [styleMeanArray copy];  
  MPSImageBatch *styleMultiplicationBatch = [styleMultiplicationArray copy];  
  

  id<MTLCommandBuffer> cb = [_commandQueue commandBuffer];

  NSMutableArray *intermediateImages = [[NSMutableArray alloc] initWithArray:@[]];
  NSMutableArray *destinationStates = [[NSMutableArray alloc] initWithArray:@[]];

  MPSImageBatch *ret =  [_trainingGraph encodeBatchToCommandBuffer:cb
                                                      sourceImages:@[contentImageBatch, contentMultiplicationBatch, contentMeanBatch, styleImageBatch, styleMultiplicationBatch, styleMeanBatch]
                                                      sourceStates:nil
                                                intermediateImages:intermediateImages
                                                 destinationStates:destinationStates];

  for (MPSImage *image in ret) {
    [image synchronizeOnCommandBuffer:cb];  
  }

  [cb commit];
  [cb waitUntilCompleted];

  // AAAK: What about overflow?
  float lossValue = 0.0f;
  MPSImageBatch *outBatch = [intermediateImages objectAtIndex:0];
  for (NSUInteger index = 0; index < _batchSize; index++) {
    float loss;
    MPSImage* lossImage = outBatch[index];
    [lossImage readBytes:&loss dataLayout:(MPSDataLayoutHeightxWidthxFeatureChannels)imageIndex:0];
    lossValue += loss;
  }
  lossValue /= _batchSize;

  NSMutableData * lossData = [NSMutableData dataWithCapacity:0];
  [lossData appendBytes:&lossValue length:sizeof(float)];

  NSMutableDictionary<NSString *, NSData *> *lossDict;
  lossDict[@"loss"] = [NSData dataWithData:lossData];

  return [lossDict copy];
}

@end
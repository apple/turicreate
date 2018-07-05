#include "mps_layers.h"
#include "mps_lstm_helper.h"
#include "mps_utils.h"

// TODO: remove the define below once the dropout bug is fixed
#define ALWAYS_ALLOCATE_DO_OUTPUT 1

// --------------------------------------------------------------------------------------------
//                  Common utilities for all Layers
// --------------------------------------------------------------------------------------------
@implementation TCMPSImageAllocator {
  MPSImageFeatureChannelFormat _format;
}

- (MPSImage *__nonnull)
imageForCommandBuffer:(__nonnull id<MTLCommandBuffer>)cmdBuf
      imageDescriptor:(MPSImageDescriptor *__nonnull)descriptor
               kernel:(MPSKernel *__nonnull)kernel {

  if (MPSImageFeatureChannelFormatNone != _format)
    descriptor.channelFormat = _format;
  return [[MPSImage alloc] initWithDevice:cmdBuf.device
                          imageDescriptor:descriptor];
}

- (nullable instancetype)initWithFormat:(MPSImageFeatureChannelFormat)format {
  self = [super init];
  if (nil == self)
    return self;

  _format = format;
  return self;
}

- (nullable instancetype)initWithCoder:(NSCoder *)aDecoder {
  self = [super init];
  if (nil == self)
    return self;

  _format = (MPSImageFeatureChannelFormat)
      [aDecoder decodeIntegerForKey:@"pixelFormat"];
  return self;
}

- (void)encodeWithCoder:(NSCoder *)aCoder {
  [aCoder encodeInteger:(NSInteger)_format forKey:@"pixelFormat"];
}

+ (BOOL)supportsSecureCoding {
  return YES;
}

@end

@implementation MPSCNNWeight

- (nonnull instancetype)initWithKernelWidth:(NSUInteger)kernelWidth
                               kernelHeight:(NSUInteger)kernelHeight
                       inputFeatureChannels:(NSUInteger)inputFeatureChannels
                      outputFeatureChannels:(NSUInteger)outputFeatureChannels
                            strideInPixelsX:(NSUInteger)strideInPixelsX
                            strideInPixelsY:(NSUInteger)strideInPixelsY
                                 neuronType:(MPSCNNNeuronType)neuronType
                                    neuronA:(float)neuronA
                                    neuronB:(float)neuronB {

  self = [super init];
  if (nil == self)
    return nil;

  _desc = [MPSCNNConvolutionDescriptor
      cnnConvolutionDescriptorWithKernelWidth:kernelWidth
                                 kernelHeight:kernelHeight
                         inputFeatureChannels:inputFeatureChannels
                        outputFeatureChannels:outputFeatureChannels];

  [_desc setNeuronType:neuronType parameterA:neuronA parameterB:neuronB];
  [_desc setStrideInPixelsX:strideInPixelsX];
  [_desc setStrideInPixelsY:strideInPixelsY];
  int w_size =
      inputFeatureChannels * kernelHeight * kernelWidth * outputFeatureChannels;
  _bias.resize(outputFeatureChannels);
  _weight.resize(w_size);

  for (int i = 0; i < w_size; ++i) {
    _weight[i] = 0.f;
  }

  for (int i = 0; i < _bias.size(); ++i) {
    _bias[i] = 0.f;
  }
  return self;
}

- (MPSDataType)dataType {
  return MPSDataTypeFloat32;
}

- (MPSCNNConvolutionDescriptor *__nonnull)descriptor {
  return _desc;
}

- (void *__nonnull)weights {
  return &_weight[0];
}

- (float *__nullable)biasTerms {
  if (_bias.size() == 0)
    return nil;
  return &_bias[0];
};

- (size_t)weight_size {
  return _weight.size();
}

- (size_t)bias_size {
  return _bias.size();
}

- (BOOL)load {
  return YES;
};

- (void)purge{};

- (void)checkpoint {
}

- (NSString *_Nullable)label {
  return @"MPSCNNWeight";
}

- (void)dealloc {
}

@end /* MPSCNNWeight */

namespace turi {
namespace mps {

// --------------------------------------------------------------------------------------------
//                                 Layer Implementations
// --------------------------------------------------------------------------------------------

// ReLU Layer
// ------------------------------------------------------------------------------------
void ReLULayer::Forward(MPSImageBatch *_Nonnull src,
                        id<MTLCommandBuffer> _Nonnull cb,
                        bool is_train) {
  state = nil;
  MPSNNGradientStateBatch *tmp_state = nil;
  input = src;

  fwd_output = [op_forward encodeBatchToCommandBuffer:cb
                                         sourceImages:src
                                    destinationStates:&tmp_state
                          destinationStateIsTemporary:false];
   
  state = tmp_state;
}

void ReLULayer::Backward(MPSImageBatch *_Nonnull src,
                         id<MTLCommandBuffer> _Nonnull cb) {

  bwd_output = [op_backward encodeBatchToCommandBuffer:cb
                                       sourceGradients:src
                                          sourceImages:input
                                        gradientStates:state];
   
}

void ReLULayer::Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_q,
                     const FloatArrayMap &config, bool is_train, LowLevelMode net_mode, bool is_output_layer) {
  assert(fparams.size() > 0);
  float a = fparams[0];

    
  MPSNNNeuronDescriptor *desc =
      [MPSNNNeuronDescriptor cnnNeuronDescriptorWithType:MPSCNNNeuronTypeReLU
                                                       a:a];
  op_forward = [[MPSCNNNeuronReLU alloc] initWithDevice:device a:a];
    
  if (is_output_layer || kLowLevelModeTest == net_mode){
      op_forward.destinationImageAllocator = [[TCMPSImageAllocator alloc] initWithFormat:MPSImageFeatureChannelFormatFloat32];
  }
  if (is_train){
      op_backward = [[MPSCNNNeuronGradient alloc] initWithDevice:device
                                                neuronDescriptor:desc];
      
      if (kLowLevelModeTest == net_mode){
          op_backward.destinationImageAllocator = [[TCMPSImageAllocator alloc] initWithFormat:MPSImageFeatureChannelFormatFloat32];
      }
  }
}

// Convolutions
// ------------------------------------------------------------------------------------
void ConvLayer::Forward(MPSImageBatch *_Nonnull src,
                        id<MTLCommandBuffer> _Nonnull cb,
                        bool is_train) {
  state = nil;
  MPSNNGradientStateBatch *tmp_state = nil;
  input = src;

  fwd_output = [op_forward encodeBatchToCommandBuffer:cb
                                         sourceImages:src
                                    destinationStates:&tmp_state
                          destinationStateIsTemporary:false];
    
  state = tmp_state;
}

void ConvLayer::Backward(MPSImageBatch *_Nonnull src,
                         id<MTLCommandBuffer> _Nonnull cb) {

  bwd_output = [op_backward encodeBatchToCommandBuffer:cb
                                       sourceGradients:src
                                          sourceImages:input
                                        gradientStates:state];
}

void ConvLayer::Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_q,
                     const FloatArrayMap &config, bool is_train, LowLevelMode net_mode, bool is_output_layer) {
  assert(iparams.size() >= 8);
  int k_h = iparams[0];
  int k_w = iparams[1];
  int c_in = iparams[2];
  int c_out = iparams[3];
  int s_x = iparams[4];
  int s_y = iparams[5];
  PaddingType pad_type = (PaddingType)iparams[6];
  use_bias = iparams[7] > 0;

  weight = [TCMPSConvolutionWeights alloc];
  weight = [weight initWithKernelWidth:k_w
                          kernelHeight:k_h
                  inputFeatureChannels:c_in
                 outputFeatureChannels:c_out
                            neuronType:MPSCNNNeuronTypeNone
                               strideX:s_x
                               strideY:s_y
                               neuronA:0.0f
                               neuronB:0.0f
                kernelParamsBinaryName:name.c_str()
                                device:device
                             cmd_queue:cmd_q
                       init_weight_ptr:nil
                         init_bias_ptr:nil
                      optimizerOptions:get_array_map_optimizer_options(config)];
    
  op_forward = [[MPSCNNConvolution alloc] initWithDevice:device weights:weight];
  op_forward.padding = SetPaddingType(pad_type);
    
  if (is_output_layer || kLowLevelModeTest == net_mode){
    op_forward.destinationImageAllocator = [[TCMPSImageAllocator alloc] initWithFormat:MPSImageFeatureChannelFormatFloat32];
  }
  if (is_train){
      op_backward =
        [[MPSCNNConvolutionGradient alloc] initWithDevice:device weights:weight];
      
      if (kLowLevelModeTest == net_mode){
          op_backward.destinationImageAllocator = [[TCMPSImageAllocator alloc] initWithFormat:MPSImageFeatureChannelFormatFloat32];
      }
      op_backward.padding = SetPaddingType(pad_type);
  }
}

void ConvLayer::Load(const FloatArrayMap &weights) {
  std::string weight_key = name + "_weight";
  std::string bias_key = name + "_bias";

    if (weights.count(weight_key) > 0){
        LogStdString("Loading weight: " + weight_key);
        [weight loadWeight:(float*) weights.at(weight_key).data];

    }
    if (weights.count(bias_key) > 0){
        LogStdString("Loading weight: " + bias_key);
        [weight loadBias:(float*) weights.at(bias_key).data];
        
    }
    
  [op_forward reloadWeightsAndBiasesFromDataSource];
  if (op_backward){
      [op_backward reloadWeightsAndBiasesFromDataSource];
  }
}

void ConvLayer::Export(

    std::unordered_map<std::string,
                       std::tuple<std::string, float *, int, std::vector<int>>>
        &table) {
  int k_h = iparams[0];
  int k_w = iparams[1];
  int c_in = iparams[2];
  int c_out = iparams[3];
    
  if (weight.load)
  {
      std::string weight_key = name + "_weight";
      std::string bias_key = name + "_bias";
      table[weight_key] = {
          weight_key, (float *)[weight weights], 4, {c_out, k_h, k_w, c_in}};
      table[bias_key] = {bias_key,
                         (float *)[weight biasTerms],
                         1,
                         {static_cast<int>([weight bias_size])}};
      
  }
}

void ConvLayer::GpuUpdate(id<MTLCommandBuffer> _Nonnull cb){

    MPSCNNConvolutionGradientState *cnn_state = (MPSCNNConvolutionGradientState *)state[0];
    [weight updateWithCommandBuffer:cb gradientState:cnn_state];
    
    [op_forward reloadWeightsAndBiasesWithCommandBuffer:cb
                                                  state:weight->convWtsAndBias];
    if (op_backward){
        [op_backward reloadWeightsAndBiasesWithCommandBuffer:cb
                                                      state:weight->convWtsAndBias];
    }
}
void ConvLayer::Update(MPSUpdater *_Nonnull updater, int lid) {
  MPSCNNConvolutionGradientState *cnn_state =
      (MPSCNNConvolutionGradientState *)state[0];
  float *g_w = (float *)ADVANCE_PTR(cnn_state.gradientForWeights.contents, 0);
  float *g_b = (float *)ADVANCE_PTR(cnn_state.gradientForBiases.contents, 0);
  updater->Update((float *)[weight weights], g_w, [weight weight_size], lid, 0);
  if (use_bias) {
    updater->Update((float *)[weight biasTerms], g_b, [weight bias_size], lid,
                    1);
  }
  [op_forward reloadWeightsAndBiasesFromDataSource];
  if (op_backward){
      [op_backward reloadWeightsAndBiasesFromDataSource];
  }
}
// Batch-Norm layer
// ------------------------------------------------------------------------------------
void BNLayer::Forward(MPSImageBatch *_Nonnull src,
                      id<MTLCommandBuffer> _Nonnull cb,
                      bool is_train) {
    
  MPSCNNBatchNormalizationState * state_to_encode;
    
  if (use_temp_images_){
      fwd_output = AllocTempImageBatch(cb, false);
  }
    
  if (is_train_mode_ && is_train) {
    if (!is_state_init) {
      is_state_init = true;
      bn_state = [op_forward resultStateForSourceImage:src[0]
                                          sourceStates:nil
                                      destinationImage:fwd_output[0]];
    }
    state_to_encode = bn_state;
    input = src;
    [stat encodeBatchToCommandBuffer:cb
                        sourceImages:src
             batchNormalizationState:bn_state];
  } else {
    state_to_encode = nil;
  }

  [op_forward encodeBatchToCommandBuffer:cb
                            sourceImages:src
                 batchNormalizationState:state_to_encode
                       destinationImages:fwd_output];

}

void BNLayer::Backward(MPSImageBatch *_Nonnull src,
                       id<MTLCommandBuffer> _Nonnull cb) {
  
  assert(bn_state != nil && "BN Backward can not be called after calling Forward(is_train=true)");
    
  if (use_temp_images_){
      bwd_output = AllocTempImageBatch(cb, true);
  }
    
  [g_stat encodeBatchToCommandBuffer:cb
                     sourceGradients:src
                        sourceImages:input
             batchNormalizationState:bn_state];

  [op_backward encodeBatchToCommandBuffer:cb
                          sourceGradients:src
                             sourceImages:input
                  batchNormalizationState:bn_state
                     destinationGradients:bwd_output];
  [bn_state synchronizeOnCommandBuffer:cb];
}

void BNLayer::Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_q,
                   const FloatArrayMap &config, bool is_train, LowLevelMode net_mode, bool is_output_layer) {
    assert(ishape.size() == 4);
    int ch = ishape[3];

    is_train_mode_ = is_train;
  


    float batchNormEpsilon = get_array_map_scalar(config, "batch_norm_epsilon", 0.001f);

    data = [TCMPSBatchNormData alloc];
    data = [data initWithChannels:ch
           kernelParamsBinaryName:name.c_str()
                           device:device
                        cmd_queue:cmd_q
                            gamma:nil
                             beta:nil
                       moving_avg:nil
                       moving_var:nil
                 optimizerOptions:get_array_map_optimizer_options(config)
                 batchNormEpsilon:batchNormEpsilon];

  op_forward =
      [[MPSCNNBatchNormalization alloc] initWithDevice:device dataSource:data];
  op_forward.epsilon = batchNormEpsilon;
  if (is_output_layer || kLowLevelModeTest == net_mode){
    op_forward.destinationImageAllocator = [[TCMPSImageAllocator alloc] initWithFormat:MPSImageFeatureChannelFormatFloat32];
  }

  if (is_train){
      stat = [[MPSCNNBatchNormalizationStatistics alloc] initWithDevice:device];
      g_stat = [[MPSCNNBatchNormalizationStatisticsGradient alloc]
      initWithDevice:device];
      op_backward =
      [[MPSCNNBatchNormalizationGradient alloc] initWithDevice:device];
      
      if (kLowLevelModeTest == net_mode){
          op_backward.destinationImageAllocator = [[TCMPSImageAllocator alloc] initWithFormat:MPSImageFeatureChannelFormatFloat32];
          AllocImage(device, is_train);
          use_temp_images_ = false;
      }
  }
}

void BNLayer::Load(const FloatArrayMap &weights) {
  std::string gamma_key = name + "_gamma";
  std::string beta_key = name + "_beta";
  std::string var_key = name + "_running_var";
  std::string mean_key = name + "_running_mean";

  if (weights.count(gamma_key) > 0){
    const FloatArray &arr = weights.at(gamma_key);
    assert(arr.size == [data numberOfFeatureChannels]);
    [data loadGamma:arr.data];
  }

  if (weights.count(beta_key) > 0){
    const FloatArray &arr = weights.at(beta_key);
    assert(arr.size == [data numberOfFeatureChannels]);
    [data loadBeta:arr.data];
  }

  if (weights.count(mean_key) > 0){
    const FloatArray &arr = weights.at(mean_key);
    assert(arr.size == [data numberOfFeatureChannels]);
    [data loadMovingAvg:arr.data];
  }

  if (weights.count(var_key) > 0){
    const FloatArray &arr = weights.at(var_key);
    assert(arr.size == [data numberOfFeatureChannels]);
    [data loadMovingVar:arr.data];
  }

  [op_forward reloadGammaAndBetaFromDataSource];
  [op_forward reloadMeanAndVarianceFromDataSource];
}

void BNLayer::Export(
    std::unordered_map<std::string,
                       std::tuple<std::string, float *, int, std::vector<int>>>
        &table) {
  std::string gamma_key = name + "_gamma";
  std::string beta_key = name + "_beta";
  std::string var_key = name + "_running_var";
  std::string mean_key = name + "_running_mean";
  int num_channel = [data numberOfFeatureChannels];

    if ([data load]){
        table[gamma_key] = {gamma_key, (float *)[data gamma], 1, {num_channel}};
        table[beta_key] = {beta_key, (float *)[data beta], 1, {num_channel}};
        table[var_key] = {var_key, (float *)[data variance], 1, {num_channel}};
        table[mean_key] = {mean_key, (float *)[data mean], 1, {num_channel}};
    }
}

void BNLayer::Update(MPSUpdater *_Nonnull updater, int lid) {
  int num_channel = [data numberOfFeatureChannels];
  float *g_w = (float *)ADVANCE_PTR(bn_state.gradientForGamma.contents, 0);
  float *g_b = (float *)ADVANCE_PTR(bn_state.gradientForBeta.contents, 0);
  float *g_m = (float *)ADVANCE_PTR(bn_state.mean.contents, 0);
  float *g_v = (float *)ADVANCE_PTR(bn_state.variance.contents, 0);
    
  updater->Update((float *)[data gamma], g_w, num_channel, lid, 0);
  updater->Update((float *)[data beta], g_b, num_channel, lid, 1);
  updater->MovingAvg([data mean], g_m, num_channel, 0.9);
  updater->MovingAvg([data variance], g_v, num_channel, 0.9);

  [op_forward reloadGammaAndBetaFromDataSource];
  [op_forward reloadMeanAndVarianceFromDataSource];
}

void BNLayer::GpuUpdate(id<MTLCommandBuffer> _Nonnull cb){
    [data updateGammaAndBetaWithCommandBuffer:cb batchNormalizationState:bn_state];

    [op_forward reloadGammaAndBetaWithCommandBuffer:cb
                                  gammaAndBetaState:data->gammaBetaState];
    
    [op_forward reloadMeanAndVarianceWithCommandBuffer:cb
                                  meanAndVarianceState:data->meanVarianceState];
}


void MaxPoolLayer::Forward(MPSImageBatch *_Nonnull src,
                           id<MTLCommandBuffer> _Nonnull cb,
                           bool is_train) {
  state = nil;
  MPSNNGradientStateBatch *tmp_state = nil;
  input = src;

  fwd_output = [op_forward encodeBatchToCommandBuffer:cb
                                         sourceImages:src
                                    destinationStates:&tmp_state
                          destinationStateIsTemporary:false];

  state = tmp_state;
}
void MaxPoolLayer::Backward(MPSImageBatch *_Nonnull src,
                            id<MTLCommandBuffer> _Nonnull cb) {

  bwd_output = [op_backward encodeBatchToCommandBuffer:cb
                                       sourceGradients:src
                                          sourceImages:input
                                        gradientStates:state];
}
void MaxPoolLayer::Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_q,
                        const FloatArrayMap &config, bool is_train, LowLevelMode net_mode, bool is_output_layer) {
  assert(iparams.size() >= 4);
  int kH = iparams[0];
  int kW = iparams[1];
  int sH = iparams[2];
  int sW = iparams[3];


  op_forward = [[MPSCNNPoolingMax alloc] initWithDevice:device
                                            kernelWidth:kW
                                           kernelHeight:kH
                                        strideInPixelsX:sW
                                        strideInPixelsY:sH];
  op_forward.padding = SAME;
    
  if (is_output_layer || kLowLevelModeTest == net_mode){
    op_forward.destinationImageAllocator = [[TCMPSImageAllocator alloc] initWithFormat:MPSImageFeatureChannelFormatFloat32];
  }
  if (is_train){
      op_backward = [[MPSCNNPoolingMaxGradient alloc] initWithDevice:device
                                                         kernelWidth:kW
                                                        kernelHeight:kH
                                                     strideInPixelsX:sW
                                                     strideInPixelsY:sH];
      
      if (kLowLevelModeTest == net_mode){
          op_backward.destinationImageAllocator = [[TCMPSImageAllocator alloc] initWithFormat:MPSImageFeatureChannelFormatFloat32];
      }
      op_backward.padding = SAME;
  }
}

// Dropout
// ------------------------------------------------------------------------------------
void DropOutLayer::Forward(MPSImageBatch *_Nonnull src,
                           id<MTLCommandBuffer> _Nonnull cb,
                           bool is_train) {
  if (false == is_train)
  {
      fwd_output = src;
      return;
  }
    
  state = nil;
  MPSNNGradientStateBatch *tmp_state = nil;
  input = src;
  fwd_output = [op_forward encodeBatchToCommandBuffer:cb
                                         sourceImages:src
                                    destinationStates:&tmp_state
                          destinationStateIsTemporary:false];
  state = tmp_state;
}

void DropOutLayer::Backward(MPSImageBatch *_Nonnull src,
                            id<MTLCommandBuffer> _Nonnull cb) {
  bwd_output = [op_backward encodeBatchToCommandBuffer:cb
                                       sourceGradients:src
                                          sourceImages:input
                                        gradientStates:state];
}

void DropOutLayer::Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_q,
                        const FloatArrayMap &config, bool is_train, LowLevelMode net_mode, bool is_output_layer) {
  assert(iparams.size() >= 2);
  float fKeepProb = (float)iparams[0] / 100.0;
  int nSeed = iparams[1];


  if (nSeed == -1){
      srand((unsigned)time(0));
      nSeed = std::rand();
  }

  assert(is_train && "Dropout is currently not supported for inference! Please remove the layer from the network");

  op_forward = [[MPSCNNDropout alloc]
          initWithDevice:device
         keepProbability:fKeepProb
                    seed:nSeed
      maskStrideInPixels:MTLSize{.width = 1, .height = 1, .depth = 1}];

  if (ALWAYS_ALLOCATE_DO_OUTPUT || is_output_layer || kLowLevelModeTest == net_mode){
      op_forward.destinationImageAllocator = [[TCMPSImageAllocator alloc] initWithFormat:MPSImageFeatureChannelFormatFloat32];
  }
  op_backward = [[MPSCNNDropoutGradient alloc]
          initWithDevice:device
         keepProbability:fKeepProb
                    seed:nSeed
      maskStrideInPixels:MTLSize{.width = 1, .height = 1, .depth = 1}];
    
    if (ALWAYS_ALLOCATE_DO_OUTPUT || kLowLevelModeTest == net_mode){
          op_backward.destinationImageAllocator = [[TCMPSImageAllocator alloc] initWithFormat:MPSImageFeatureChannelFormatFloat32];
    }
}

// SoftMax
// ------------------------------------------------------------------------------------
void SoftMaxLayer::Forward(MPSImageBatch *_Nonnull src,
                           id<MTLCommandBuffer> _Nonnull cb,
                           bool is_train) {
  state = nil;
  MPSNNGradientStateBatch *tmp_state = nil;
  input = src;
  fwd_output = [op_forward encodeBatchToCommandBuffer:cb
                                         sourceImages:src
                                    destinationStates:&tmp_state
                          destinationStateIsTemporary:false];
  state = tmp_state;
}

void SoftMaxLayer::Backward(MPSImageBatch *_Nonnull src,
                            id<MTLCommandBuffer> _Nonnull cb) {
  bwd_output = [op_backward encodeBatchToCommandBuffer:cb
                                       sourceGradients:src
                                          sourceImages:fwd_output
                                        gradientStates:state];
}

void SoftMaxLayer::Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_q,
                        const FloatArrayMap &config, bool is_train, LowLevelMode net_mode, bool is_output_layer) {


  op_forward = [[MPSCNNSoftMax alloc] initWithDevice:device];

  if (is_output_layer || kLowLevelModeTest == net_mode){
      op_forward.destinationImageAllocator = [[TCMPSImageAllocator alloc] initWithFormat:MPSImageFeatureChannelFormatFloat32];
  }
    
  if (is_train){
      op_backward = [[MPSCNNSoftMaxGradient alloc] initWithDevice:device];

      op_backward.destinationImageAllocator = [[TCMPSImageAllocator alloc] initWithFormat:MPSImageFeatureChannelFormatFloat32];
  }
}

// SoftMax Cross Entropy Loss layer
// ------------------------------------------------------------------------------------
void SmceLossLayer::Loss(MPSImageBatch *_Nonnull src,
                         MPSCNNLossLabelsBatch *_Nonnull labels,
                         id<MTLCommandBuffer> _Nonnull cb) {

  bwd_output =
      [op_loss encodeBatchToCommandBuffer:cb sourceImages:src labels:labels];
}

void SmceLossLayer::Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_q,
                         const FloatArrayMap &config, bool is_train, LowLevelMode net_mode, bool is_output_layer) {

  assert(iparams.size() >= 1);

  MPSCNNLossDescriptor *lossDesc = [MPSCNNLossDescriptor
      cnnLossDescriptorWithType:MPSCNNLossTypeSoftMaxCrossEntropy
                  reductionType:MPSCNNReductionTypeMean];

  lossDesc.weight = 1.f;

  op_loss = [[MPSCNNLoss alloc] initWithDevice:device lossDescriptor:lossDesc];
  
  if (kLowLevelModeTest == net_mode){
    op_loss.destinationImageAllocator = [[TCMPSImageAllocator alloc] initWithFormat:MPSImageFeatureChannelFormatFloat32];
  }
}

// LSTM
// ------------------------------------------------------------------------------------
void LstmLayer::Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_q,
                     const FloatArrayMap &config, bool is_train, LowLevelMode net_mode, bool is_output_layer) {
    batch_size_ = (NSUInteger)ishape[0];
    sequence_length_ = (NSUInteger)ishape[2];
    num_input_features_ = (NSUInteger)ishape[3];
    num_output_features_ = (NSUInteger)oshape[3];
    device_ = device;
    cmd_q_ = cmd_q;

    MPSCNNWeight *_Nonnull wi = [[MPSCNNWeight alloc]
                                 initWithKernelWidth:1
                                 kernelHeight:1
                                 inputFeatureChannels:num_input_features_
                                 outputFeatureChannels:num_output_features_
                                 strideInPixelsX:1
                                 strideInPixelsY:1
                                 neuronType:(MPSCNNNeuronTypeSigmoid)
                                 neuronA:0.0f
                                 neuronB:0.0f];
    
    MPSCNNWeight *_Nonnull ri = [[MPSCNNWeight alloc]
                                 initWithKernelWidth:1
                                 kernelHeight:1
                                 inputFeatureChannels:num_output_features_
                                 outputFeatureChannels:num_output_features_
                                 strideInPixelsX:1
                                 strideInPixelsY:1
                                 neuronType:(MPSCNNNeuronTypeTanH)
                                 neuronA:1.0f
                                 neuronB:1.0f];
    
    MPSCNNWeight *_Nonnull wf = [[MPSCNNWeight alloc]
                                 initWithKernelWidth:1
                                 kernelHeight:1
                                 inputFeatureChannels:num_input_features_
                                 outputFeatureChannels:num_output_features_
                                 strideInPixelsX:1
                                 strideInPixelsY:1
                                 neuronType:(MPSCNNNeuronTypeSigmoid)
                                 neuronA:0.0f
                                 neuronB:0.0f];
    
    MPSCNNWeight *_Nonnull rf = [[MPSCNNWeight alloc]
                                 initWithKernelWidth:1
                                 kernelHeight:1
                                 inputFeatureChannels:num_output_features_
                                 outputFeatureChannels:num_output_features_
                                 strideInPixelsX:1
                                 strideInPixelsY:1
                                 neuronType:(MPSCNNNeuronTypeTanH)
                                 neuronA:1.0f
                                 neuronB:1.0f];
    
    MPSCNNWeight *_Nonnull wc = [[MPSCNNWeight alloc]
                                 initWithKernelWidth:1
                                 kernelHeight:1
                                 inputFeatureChannels:num_input_features_
                                 outputFeatureChannels:num_output_features_
                                 strideInPixelsX:1
                                 strideInPixelsY:1
                                 neuronType:(MPSCNNNeuronTypeTanH)
                                 neuronA:1.0f
                                 neuronB:1.0f];
    
    MPSCNNWeight *_Nonnull rc = [[MPSCNNWeight alloc]
                                 initWithKernelWidth:1
                                 kernelHeight:1
                                 inputFeatureChannels:num_output_features_
                                 outputFeatureChannels:num_output_features_
                                 strideInPixelsX:1
                                 strideInPixelsY:1
                                 neuronType:(MPSCNNNeuronTypeTanH)
                                 neuronA:1.0f
                                 neuronB:1.0f];
    
    MPSCNNWeight *_Nonnull wo = [[MPSCNNWeight alloc]
                                 initWithKernelWidth:1
                                 kernelHeight:1
                                 inputFeatureChannels:num_input_features_
                                 outputFeatureChannels:num_output_features_
                                 strideInPixelsX:1
                                 strideInPixelsY:1
                                 neuronType:(MPSCNNNeuronTypeSigmoid)
                                 neuronA:0.0f
                                 neuronB:0.0f];
    
    MPSCNNWeight *_Nonnull ro = [[MPSCNNWeight alloc]
                                 initWithKernelWidth:1
                                 kernelHeight:1
                                 inputFeatureChannels:num_output_features_
                                 outputFeatureChannels:num_output_features_
                                 strideInPixelsX:1
                                 strideInPixelsY:1
                                 neuronType:(MPSCNNNeuronTypeTanH)
                                 neuronA:1.0f
                                 neuronB:1.0f];
    
    MPSLSTMDescriptor* lstmDesc =
        [MPSLSTMDescriptor createLSTMDescriptorWithInputFeatureChannels:num_input_features_
                                                  outputFeatureChannels:num_output_features_];
    
    lstmDesc.inputGateInputWeights = wi;
    lstmDesc.inputGateRecurrentWeights = ri;
    
    lstmDesc.forgetGateInputWeights = wf;
    lstmDesc.forgetGateRecurrentWeights = rf;
    
    lstmDesc.outputGateInputWeights = wo;
    lstmDesc.outputGateRecurrentWeights = ro;
    
    lstmDesc.cellGateInputWeights = wc;
    lstmDesc.cellGateRecurrentWeights = rc;
    
    lstmDesc.cellToOutputNeuronType = MPSCNNNeuronTypeTanH;
    lstmDesc.useFloat32Weights = YES;
    
    filter = [[MPSRNNMatrixTrainingLayer alloc] initWithDevice: device
                                                rnnDescriptor: (MPSRNNDescriptor*)lstmDesc
                                             trainableWeights: weights];

    // Create auxiliary weights for ADAM optimizer:
    [filter createWeightMatrices: weightsFirstMoment];
    [filter createWeightMatrices: weightsSecondMoment];

    [filter createWeightGradientMatrices:weightGradients dataType:MPSDataTypeFloat32];

    optimizers = [[NSMutableArray alloc] initWithCapacity: weightsFirstMoment.count];
    // Init momentums to zero
    for (NSUInteger i = 0; i < weightsFirstMoment.count; i++)
    {
        MPSMatrix * wMat1 = weightsFirstMoment[i];
        MPSMatrix * wMat2 = weightsSecondMoment[i];
        memset(wMat1.data.contents, 0, wMat1.data.length);
        memset(wMat2.data.contents, 0, wMat2.data.length);
#if MPS_TARGET_MAC
        [wMat1.data didModifyRange: NSMakeRange(0, wMat1.data.length)];
        [wMat2.data didModifyRange: NSMakeRange(0, wMat2.data.length)];
#endif
        MPSNNOptimizerAdam * optimizer = [[MPSNNOptimizerAdam alloc] initWithDevice: device learningRate: 1e-3f];
        [optimizers addObject: optimizer];
    }

    image_to_matrix_kernel_ =
        [[MPSImageCopyToMatrix alloc] initWithDevice:device
                                          dataLayout:MPSDataLayoutHeightxWidthxFeatureChannels];
    matrix_to_image_kernel_ =
        [[MPSMatrixCopyToImage alloc] initWithDevice:device
                                          dataLayout:MPSDataLayoutHeightxWidthxFeatureChannels];

    NSUInteger inputFeatureSize = num_input_features_ * sizeof(float);
    fwd_src_buffer_ = [device newBufferWithLength:inputFeatureSize * sequence_length_ * batch_size_
                                          options:MTLResourceStorageModeManaged];
    bwd_dst_buffer_ = [device newBufferWithLength:inputFeatureSize * sequence_length_ * batch_size_
                                          options:MTLResourceStorageModeManaged];

    NSUInteger outputFeatureSize = num_output_features_ * sizeof(float);
    fwd_dst_buffer_ = [device newBufferWithLength:outputFeatureSize * sequence_length_ * batch_size_
                                          options:MTLResourceStorageModeManaged];
    bwd_src_buffer_ = [device newBufferWithLength:outputFeatureSize * sequence_length_ * batch_size_
                                          options:MTLResourceStorageModeManaged];

    use_temp_image_ = !(is_output_layer || kLowLevelModeTest == net_mode);
    
    // Create matrices for copying weights during Export() and Load()
    InitWeightCopyMatrices();
}

NSArray<MPSMatrix *> *LstmLayer::CreateMatrixViews(id <MTLBuffer> buffer, NSUInteger num_features) {
    // Create a NxC matrix that corresponds to one value for T, from the NTC-layout buffer.
    NSUInteger stride = sequence_length_ * num_features * sizeof(float);
    MPSMatrixDescriptor *matrixDesc =
        [MPSMatrixDescriptor matrixDescriptorWithRows:batch_size_
                                              columns:num_features
                                             rowBytes:stride
                                             dataType:MPSDataTypeFloat32];
    MPSMatrix *matrix = [[MPSMatrix alloc] initWithBuffer:buffer descriptor:matrixDesc];

    // Return an array with T copies of the matrix, intended to be used alongside the return value of CreateMatixOffsets.
    NSMutableArray<MPSMatrix *> *views = [[NSMutableArray alloc] initWithCapacity:sequence_length_];
    for (NSUInteger i = 0; i < sequence_length_; ++i) {
        [views addObject:matrix];
    }
    return views;
}

std::vector<NSUInteger> LstmLayer::CreateMatrixOffsets(NSUInteger num_features) {
    // Given an NTC buffer, compute the stride needed to step through each value of T.
    NSUInteger step_size = num_features * static_cast<NSUInteger>(sizeof(float));

    std::vector<NSUInteger> offsets;
    offsets.reserve(sequence_length_);
    for (NSUInteger i = 0; i < sequence_length_; ++i) {
        offsets.push_back(i * step_size);
    }
    return offsets;
}

void LstmLayer::CopyImageBatchToBuffer(MPSImageBatch *imgBatch, id <MTLBuffer> buffer,
                                       NSUInteger num_features, id <MTLCommandBuffer> cb) {
    // Define a (N,TC)-matrix view of the buffer.
    NSUInteger numColumns = sequence_length_ * num_features;
    NSUInteger rowSize = numColumns * sizeof(float);
    MPSMatrixDescriptor *desc = [MPSMatrixDescriptor matrixDescriptorWithRows:batch_size_
                                                                      columns:numColumns
                                                                     rowBytes:rowSize
                                                                     dataType:MPSDataTypeFloat32];
    MPSMatrix *matrix = [[MPSMatrix alloc] initWithBuffer:buffer descriptor:desc];

    // Copy each image from the batch into its own row of the matrix.
    MPSImage *img = nil;
    for (NSUInteger i = 0; i < imgBatch.count; ++i) {
        img = imgBatch[i];
        image_to_matrix_kernel_.destinationMatrixOrigin = MTLOriginMake(i, 0, 0);
        [image_to_matrix_kernel_ encodeToCommandBuffer:cb sourceImage:img destinationMatrix:matrix];
    }

    // Release the image memory back to MPS.
    MPSImageBatchIncrementReadCount(imgBatch, -1);
}

MPSImageBatch *LstmLayer::CopyImageBatchFromBuffer(id <MTLBuffer> buffer,
                                                   NSUInteger num_features,
                                                   id <MTLCommandBuffer> cb) {
    
    // Define a (N,TC)-matrix view of the buffer.
    NSUInteger numColumns = sequence_length_ * num_features;
    NSUInteger rowSize = numColumns * sizeof(float);
    MPSMatrixDescriptor *matrixDesc =
        [MPSMatrixDescriptor matrixDescriptorWithRows:batch_size_
                                              columns:numColumns
                                             rowBytes:rowSize
                                             dataType:MPSDataTypeFloat32];
    MPSMatrix *matrix = [[MPSMatrix alloc] initWithBuffer:buffer descriptor:matrixDesc];

    // Copy the buffer contents into discrete MPSImage instances.
    MTLTextureUsage usage = MTLTextureUsageShaderWrite | MTLTextureUsageShaderRead;
    MPSImageDescriptor *imageDesc =
        [MPSImageDescriptor imageDescriptorWithChannelFormat:MPSImageFeatureChannelFormatFloat32
                                                       width:sequence_length_
                                                      height:1
                                             featureChannels:num_features
                                              numberOfImages:1
                                                       usage:usage];
    NSMutableArray *batch = [NSMutableArray arrayWithCapacity:batch_size_];
    for (NSUInteger i = 0; i < batch_size_; ++i) {
        // Create an MPSImage.
        MPSImage *img;
        if (use_temp_image_){
            MPSTemporaryImage * temp_img = [MPSTemporaryImage temporaryImageWithCommandBuffer:cb imageDescriptor:imageDesc];
            img = (MPSImage *) temp_img;
        } else {
            img = [[MPSImage alloc] initWithDevice:cb.device imageDescriptor:imageDesc];
        }
        // Copy row i of the matrix into this image.
        matrix_to_image_kernel_.sourceMatrixOrigin = MTLOriginMake(i, 0, 0);
        [matrix_to_image_kernel_ encodeToCommandBuffer:cb sourceMatrix:matrix destinationImage:img];
        
        [batch addObject:img];
    }

    return [MPSImageBatch arrayWithArray:batch];
}

void LstmLayer::Forward(MPSImageBatch * _Nonnull src,
                        id<MTLCommandBuffer>  _Nonnull cb,
                        bool is_train) {
    // Convert input image batch to forward source buffer, transposing NTC -> TNC
    input = src;
    CopyImageBatchToBuffer(src, fwd_src_buffer_, num_input_features_, cb);

    // Create [N,C]-matrix views of each buffer, for one value of T determined by offset.
    NSArray<MPSMatrix *> *src_matrices = CreateMatrixViews(fwd_src_buffer_, num_input_features_);
    NSArray<MPSMatrix *> *dst_matrices = CreateMatrixViews(fwd_dst_buffer_, num_output_features_);
    std::vector<NSUInteger> src_offsets = CreateMatrixOffsets(num_input_features_);
    std::vector<NSUInteger> dst_offsets = CreateMatrixOffsets(num_output_features_);

    // Run the LSTM kernel
    [trainingStates removeAllObjects];
    [filter encodeForwardSequenceToCommandBuffer:cb
                                  sourceMatrices:src_matrices
                                   sourceOffsets:src_offsets.data()
                             destinationMatrices:dst_matrices
                              destinationOffsets:dst_offsets.data()
                                  trainingStates:trainingStates
                             recurrentInputState:nil
                           recurrentOutputStates:nil
                                         weights:weights];

    // Convert forward destination buffer back to MPSImage, transposing TNC -> NTC
    fwd_output = CopyImageBatchFromBuffer(fwd_dst_buffer_, num_output_features_, cb);
}

void LstmLayer::Backward(MPSImageBatch *_Nonnull src, id<MTLCommandBuffer> _Nonnull cb) {
    // Convert image batch of gradients to backward source buffer, transposing NTC -> TNC
    CopyImageBatchToBuffer(src, bwd_src_buffer_, num_output_features_, cb);

    // Create [N,C]-matrix views of each buffer, for one value of t determined by offset.
    NSArray<MPSMatrix *> *fwd_src_matrices = CreateMatrixViews(fwd_src_buffer_,
                                                               num_input_features_);
    NSArray<MPSMatrix *> *bwd_src_matrices = CreateMatrixViews(bwd_src_buffer_,
                                                               num_output_features_);
    NSArray<MPSMatrix *> *bwd_dst_matrices = CreateMatrixViews(bwd_dst_buffer_,
                                                               num_input_features_);
    std::vector<NSUInteger> input_offsets = CreateMatrixOffsets(num_input_features_);
    std::vector<NSUInteger> output_offsets = CreateMatrixOffsets(num_output_features_);

    [filter encodeGradientSequenceToCommandBuffer:cb
                                   forwardSources:fwd_src_matrices
                             forwardSourceOffsets:input_offsets.data()
                                  sourceGradients:bwd_src_matrices
                            sourceGradientOffsets:output_offsets.data()
                             destinationGradients:bwd_dst_matrices
                               destinationOffsets:input_offsets.data()
                                  weightGradients:weightGradients
                                   trainingStates:trainingStates
                              recurrentInputState:nil
                            recurrentOutputStates:nil
                                          weights:weights];

    // Convert backward destination buffer back to MPSImage, transposing TNC -> NTC
    bwd_output = CopyImageBatchFromBuffer(bwd_dst_buffer_, num_input_features_, cb);
}

void LstmLayer::Load(const FloatArrayMap &init_weights) {

    id<MTLCommandBuffer> commandBuffer = [cmd_q_ commandBuffer];

    for (const auto& lstm_weight_name : lstm_weight_names_mxnet_format){
        std::string full_key = name + "_" + lstm_weight_name;

        if (init_weights.count(full_key) > 0){
            const FloatArray &arr = init_weights.at(full_key);
            MPSRNNMatrixId wMatId = MxnetNameToMatrixId(lstm_weight_name);
            MPSMatrix * weightMat = copy_weight_matrices_.at(lstm_weight_name);
            LogStdString("Loading weight: " + full_key);
            assert (arr.size*sizeof(float) == weightMat.data.length);
            memcpy(weightMat.data.contents, arr.data, weightMat.data.length);
            [weightMat.data didModifyRange:NSMakeRange(0, weightMat.data.length)];
            
            [filter encodeCopyWeightsToCommandBuffer: commandBuffer
                                             weights: weights
                                            matrixId: wMatId
                                              matrix: weightMat
                             copyFromWeightsToMatrix: NO
                                        matrixOffset: MTLOriginMake(0, 0, 0)];

        }
    }

    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];

}

void LstmLayer::Export(std::unordered_map<std::string,
                                          std::tuple<std::string, float *, int, std::vector<int>>> &table) {
    

    
    id<MTLCommandBuffer> commandBuffer = [cmd_q_ commandBuffer];

    for (const auto& lstm_weight_name : lstm_weight_names_mxnet_format){
        std::string full_key = name + "_" + lstm_weight_name;
        MPSRNNMatrixId wMatId = MxnetNameToMatrixId(lstm_weight_name);
        MPSMatrix * weightMat = copy_weight_matrices_.at(lstm_weight_name);
        memset(weightMat.data.contents , 0 , weightMat.data.length);
        [weightMat.data didModifyRange:NSMakeRange(0, weightMat.data.length)];

        if (lstm_weight_name.find("bias") != std::string::npos){
            table[full_key] = {
                full_key, (float *)weightMat.data.contents, 1 , {(int)weightMat.columns}};
        } else {
            table[full_key] = {
                full_key, (float *)weightMat.data.contents, 2 , {(int)weightMat.rows , (int)weightMat.columns}};
        }

        [filter encodeCopyWeightsToCommandBuffer: commandBuffer
                                         weights: weights
                                        matrixId: wMatId
                                          matrix: weightMat
                         copyFromWeightsToMatrix: YES
                                    matrixOffset: MTLOriginMake(0, 0, 0)];

        [weightMat synchronizeOnCommandBuffer:commandBuffer];

    }

    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];

}

void LstmLayer::GpuUpdate(id<MTLCommandBuffer> _Nonnull cb){
    for (NSUInteger i = 0; i < weights.count; i++)
    {
        [optimizers[i] encodeToCommandBuffer: cb
                         inputGradientVector: MPSMatrixToVector(weightGradients[i])
                           inputValuesVector: MPSMatrixToVector(weights[i])
                         inputMomentumVector: MPSMatrixToVector(weightsFirstMoment[i])
                         inputVelocityVector: MPSMatrixToVector(weightsSecondMoment[i])
                          resultValuesVector: MPSMatrixToVector(weights[i])];
    }
}

void LstmLayer::InitWeightCopyMatrices() {
    for (const auto& lstm_weight_name : lstm_weight_names_mxnet_format){
        MPSRNNMatrixId wMatId = MxnetNameToMatrixId(lstm_weight_name);
        copy_weight_matrices_[lstm_weight_name] = createWeightMatrix(device_, wMatId, num_input_features_, num_output_features_);
    }
}

}  // namespace mps
}  // namespace turi

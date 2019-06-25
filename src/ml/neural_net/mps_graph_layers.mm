#include "mps_graph_layers.h"

#include <memory>

#include "mps_utils.h"


namespace turi {
namespace neural_net {

// --------------------------------------------------------------------------------------------
//                                 Layer Implementations
// --------------------------------------------------------------------------------------------

// ReLU Layer
// ------------------------------------------------------------------------------------

void ReLUGraphLayer::InitFwd(MPSNNImageNode *src) {
  float leakySlope = 0.f;
  if (fparams.size() >= 1) {
    leakySlope = fparams[0];
  }

  node_fwd = [MPSCNNNeuronReLUNode nodeWithSource:src a:leakySlope];
  fwd_img_node = node_fwd.resultImage;
}

void ReLUGraphLayer::InitBwd(MPSNNImageNode *src) {
  node_bwd =
      (MPSCNNNeuronGradientNode *)[node_fwd gradientFilterWithSources:@[ src ]];
  bwd_img_node = node_bwd.resultImage;
}

// Convolution
// ------------------------------------------------------------------------------------
void ConvGraphLayer::Init(id<MTLDevice> _Nonnull device,
                          id<MTLCommandQueue> cmd_queue,
                          const float_array_map& config,
                          const float_array_map& weights) {
  assert(iparams.size() >= 8);
  int k_h = iparams[0];
  int k_w = iparams[1];
  int c_in = iparams[2];
  int c_out = iparams[3];
  int s_x = iparams[4];
  // int s_y = iparams[5];
  PaddingType pad_type = (PaddingType)iparams[6];
  use_bias = iparams[7] > 0;

  std::string weight_key = name + "_weight";
  std::string bias_key = name + "_bias";
  std::unique_ptr<float[]> init_w;
  float *init_b = NULL;
  // TODO: force has key
  if (weights.count(weight_key) > 0) {
    LogStdString("Loading " + weight_key);
    const shared_float_array& w = weights.at(weight_key);
    init_w.reset(new float[w.size()]);
    convert_chw_to_hwc(w, init_w.get(), init_w.get() + w.size());
  }
  if (weights.count(bias_key) > 0) {
    LogStdString("Loading " + bias_key);
    init_b = const_cast<float*>(weights.at(bias_key).data());
  }

  weight = [TCMPSConvolutionWeights alloc];
  weight = [weight initWithKernelWidth:k_w
                          kernelHeight:k_h
                  inputFeatureChannels:c_in
                 outputFeatureChannels:c_out
                            neuronType:MPSCNNNeuronTypeNone
                               neuronA:0.0f
                               neuronB:0.0f
                                stride:s_x
                kernelParamsBinaryName:name.c_str()
                                device:device
                             cmd_queue:cmd_queue
                       init_weight_ptr:init_w.get()
                         init_bias_ptr:init_b
                      optimizerOptions:get_array_map_optimizer_options(config)];


  if (pad_type == kSame) {
    pad_policy = SAME;
  } else {
    pad_policy = VALID;
  }
}

void ConvGraphLayer::InitFwd(MPSNNImageNode *src) {
  node_fwd = [MPSCNNConvolutionNode nodeWithSource:src weights:weight];
  node_fwd.paddingPolicy = pad_policy;
  fwd_img_node = node_fwd.resultImage;
}

void ConvGraphLayer::InitBwd(MPSNNImageNode *src) {
  node_bwd = (MPSCNNConvolutionGradientNode *)[node_fwd
      gradientFilterWithSources:@[ src ]];
  bwd_img_node = node_bwd.resultImage;
}

void ConvGraphLayer::SetLearningRate(float lr) {
  [weight setLearningRate:lr];
}

float_array_map ConvGraphLayer::Export() const {
  float_array_map table;
  size_t k_h = iparams[0];
  size_t k_w = iparams[1];
  size_t c_in = iparams[2];
  size_t c_out = iparams[3];
  size_t size = k_h * k_w * c_in * c_out;
  [weight load];

  // Transpose the weights from NHWC to NCHW.
  std::string weight_key = name + "_weight";
  std::vector<float> weights(size);
  size_t mps_shape[] = { c_out, k_h, k_w, c_in };
  const float* mps_weights = reinterpret_cast<float*>([weight weights]);
  convert_hwc_to_chw(external_float_array(mps_weights, size, mps_shape, 4),
                     weights.data(), weights.data() + size);
  table[weight_key] = shared_float_array::wrap(std::move(weights),
                                               {c_out, c_in, k_h, k_w});

  if (use_bias) {
    std::string bias_key = name + "_bias";
    table[bias_key] = shared_float_array::copy(
        reinterpret_cast<float*>([weight biasTerms]), {c_out});
  }

  return table;
}

// MaxPoolGraphLayer Layer
// ------------------------------------------------------------------------------------

void MaxPoolGraphLayer::InitFwd(MPSNNImageNode *src) {
  int kH = iparams[0];
  // int kW = iparams[1];
  int sH = iparams[2];
  // int sW = iparams[3];
  node_fwd = [MPSCNNPoolingMaxNode nodeWithSource:src filterSize:kH stride:sH];
  if (sH == 1) {
    node_fwd.paddingPolicy = [MPSNNDefaultPadding paddingForTensorflowAveragePooling];
  } else {
    node_fwd.paddingPolicy =
        [MPSNNDefaultPadding paddingForTensorflowAveragePoolingValidOnly];
  }
  fwd_img_node = node_fwd.resultImage;
}

void MaxPoolGraphLayer::InitBwd(MPSNNImageNode *src) {
  node_bwd = (MPSCNNPoolingMaxGradientNode *)[node_fwd
      gradientFilterWithSources:@[ src ]];
  bwd_img_node = node_bwd.resultImage;
}

// BN Layer
void BNGraphLayer::Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_queue,
                        const float_array_map& config,
                        const float_array_map& weights) {
  assert(ishape.size() == 4);
  int ch = ishape[3];

  std::string gamma_key = name + "_gamma";
  std::string beta_key = name + "_beta";
  std::string mv_key = name + "_running_var";
  std::string ma_key = name + "_running_mean";
  float *init_gamma = NULL;
  float *init_beta = NULL;
  float *init_ma = NULL;
  float *init_mv = NULL;
  // TODO: force has key
  if (weights.count(gamma_key) > 0) {
    LogStdString("Loading " + gamma_key);
    init_gamma = const_cast<float*>(weights.at(gamma_key).data());
  }
  if (weights.count(beta_key) > 0) {
    LogStdString("Loading " + beta_key);
    init_beta = const_cast<float*>(weights.at(beta_key).data());
  }
  if (weights.count(mv_key) > 0) {
    LogStdString("Loading " + mv_key);
    init_mv = const_cast<float*>(weights.at(mv_key).data());
  }
  if (weights.count(ma_key) > 0) {
    LogStdString("Loading " + ma_key);
    init_ma = const_cast<float*>(weights.at(ma_key).data());
  }

  float batchNormEpsilon = get_array_map_scalar(config, "batch_norm_epsilon", 1e-5f);

  data = [TCMPSBatchNormWeights alloc];
  data = [data initWithChannels:ch
         kernelParamsBinaryName:name.c_str()
                         device:device
                      cmd_queue:cmd_queue
                          gamma:init_gamma
                           beta:init_beta
                     moving_avg:init_ma
                     moving_var:init_mv
               optimizerOptions:get_array_map_optimizer_options(config)
               batchNormEpsilon:batchNormEpsilon];
}

float_array_map BNGraphLayer::Export() const {
  float_array_map table;

  std::string gamma_key = name + "_gamma";
  std::string beta_key = name + "_beta";
  std::string var_key = name + "_running_var";
  std::string mean_key = name + "_running_mean";
  size_t num_channel = ishape[3];

  [data load];

  table[gamma_key] = shared_float_array::copy([data gamma], {num_channel});
  table[beta_key] = shared_float_array::copy([data beta], {num_channel});
  table[var_key] = shared_float_array::copy([data variance], {num_channel});
  table[mean_key] = shared_float_array::copy([data mean], {num_channel});

  return table;
}

void BNGraphLayer::InitFwd(MPSNNImageNode *src) {
  node_fwd = [MPSCNNBatchNormalizationNode nodeWithSource:src dataSource:data];
  // TODO: set node_fwd.flag to control training vs. inference mode once we have graphs for both
  // node_fwd.flags = MPSCNNBatchNormalizationFlagsCalculateStatisticsAlways;
  fwd_img_node = node_fwd.resultImage;
}

void BNGraphLayer::InitBwd(MPSNNImageNode *src) {
  node_bwd = (MPSCNNBatchNormalizationGradientNode *)[node_fwd
      gradientFilterWithSources:@[ src ]];
  bwd_img_node = node_bwd.resultImage;
}

void BNGraphLayer::SetLearningRate(float lr) {
    [data setLearningRate:lr];
}

// static
std::vector<simd::float2> YoloLossGraphLayer::GetDefaultAnchorBoxes() {
  static constexpr simd::float2 default_anchor_boxes[] = {
    {1.f, 2.f}, {1.f, 1.f}, {2.f, 1.f},
    {2.f, 4.f}, {2.f, 2.f}, {4.f, 2.f},
    {4.f, 8.f}, {4.f, 4.f}, {8.f, 4.f},
    {8.f, 16.f}, {8.f, 8.f}, {16.f, 8.f},
    {16.f, 32.f}, {16.f, 16.f}, {32.f, 16.f},
  };
  constexpr size_t num_anchor_boxes = sizeof(default_anchor_boxes) / sizeof(simd::float2);
  return std::vector<simd::float2>(default_anchor_boxes, default_anchor_boxes + num_anchor_boxes);
}

YoloLossGraphLayer::YoloLossGraphLayer(std::string layer_name, std::vector<int> i_shape,
                             std::vector<int> o_shape, Options options) {
  type = kYoloLoss;
  name = std::move(layer_name);
  ishape = std::move(i_shape);
  oshape = std::move(o_shape);
  options_ = std::move(options);
}

void YoloLossGraphLayer::Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_queue,
                              const float_array_map& config,
                              const float_array_map& weights) {

  if (config.count("od_anchors")) {
    const shared_float_array& arr = config.at("od_anchors");
    const size_t size = arr.size();
    const float* const data = arr.data();
    if (size % 2 == 0 && size > 0) {
      options_.anchor_boxes.clear();
      for (size_t b = 0; b < size / 2; ++b) {
        options_.anchor_boxes.push_back(simd::make_float2(data[b * 2],
                                                          data[b * 2 + 1]));
      }
    } else {
      // TODO: Raise exception
    }
  }

  options_.scale_xy = get_array_map_scalar(config, "od_scale_xy", options_.scale_xy);
  options_.scale_wh = get_array_map_scalar(config, "od_scale_wh", options_.scale_wh);
  options_.scale_no_object = get_array_map_scalar(config, "od_scale_no_object", options_.scale_no_object);
  options_.scale_object = get_array_map_scalar(config, "od_scale_object", options_.scale_object);
  options_.scale_class = get_array_map_scalar(config, "od_scale_class", options_.scale_class);
  options_.max_iou_for_no_object = get_array_map_scalar(config, "od_max_iou_for_no_object", options_.max_iou_for_no_object);
  options_.min_iou_for_object = get_array_map_scalar(config, "od_min_iou_for_object", options_.min_iou_for_object);
  options_.rescore = get_array_map_bool(config, "od_rescore", options_.rescore);
}

void YoloLossGraphLayer::InitFwd(MPSNNImageNode *src) {
  NSUInteger num_anchor_boxes = static_cast<NSUInteger>(options_.anchor_boxes.size());
  NSUInteger anchor_box_size = static_cast<NSUInteger>(sizeof(simd::float2));
  NSData *anchor_boxes_data = [NSData dataWithBytes:options_.anchor_boxes.data()
                                             length:num_anchor_boxes * anchor_box_size];
  MPSCNNYOLOLossDescriptor *desc =
      [MPSCNNYOLOLossDescriptor cnnLossDescriptorWithXYLossType:MPSCNNLossTypeMeanSquaredError
                                                     WHLossType:MPSCNNLossTypeHuber
                                             confidenceLossType:MPSCNNLossTypeSigmoidCrossEntropy
                                                classesLossType:MPSCNNLossTypeSoftMaxCrossEntropy
                                                  reductionType:MPSCNNReductionTypeSum
                                                    anchorBoxes:anchor_boxes_data
                                            numberOfAnchorBoxes:num_anchor_boxes];
  int batch_size = oshape[0];
  desc.rescore = options_.rescore;
  desc.scaleXY = options_.scale_xy;
  desc.scaleWH = options_.scale_wh;
  desc.scaleNoObject = options_.scale_no_object / batch_size;
  desc.scaleObject = options_.scale_object / batch_size;
  desc.scaleClass = options_.scale_class;
  desc.minIOUForObjectPresence = options_.min_iou_for_object;
  desc.maxIOUForObjectAbsence = options_.max_iou_for_no_object;

  yoloNode_ = [MPSCNNYOLOLossNode nodeWithSource:src lossDescriptor:desc];
  labels_node = yoloNode_.inputLabels;
  fwd_img_node = yoloNode_.resultImage;
}

void YoloLossGraphLayer::InitBwd(MPSNNImageNode *src) {
  bwd_img_node = fwd_img_node;
}

MPSCNNLossLabelsBatch *YoloLossGraphLayer::CreateLossState(
    id<MTLDevice> _Nonnull device, const float_array &labels_array) const {
  const float* data = labels_array.data();
  MPSCNNLossLabelsBatch *loss_state = @[];
  int batch_size = oshape[0];
  MTLSize loss_image_size = {1, 1, 1};
  MTLSize input_size;
  input_size.height = static_cast<NSUInteger>(oshape[1]);
  input_size.width = static_cast<NSUInteger>(oshape[2]);
  input_size.depth = static_cast<NSUInteger>(oshape[3]);
  NSUInteger stride = input_size.height * input_size.width * input_size.depth;
  for (int i = 0; i < batch_size; ++i) {
    NSData *label_data = [NSData dataWithBytes:data + i * stride length:stride * sizeof(float)];
    MPSCNNLossDataDescriptor *desc =
        [MPSCNNLossDataDescriptor cnnLossDataDescriptorWithData:label_data
                                                         layout:MPSDataLayoutHeightxWidthxFeatureChannels
                                                           size:input_size];
    MPSCNNLossLabels *loss_labels = [[MPSCNNLossLabels alloc] initWithDevice:device
                                                               lossImageSize:loss_image_size
                                                            labelsDescriptor:desc
                                                           weightsDescriptor:nil];
    loss_state = [loss_state arrayByAddingObject:loss_labels];
  }
  return loss_state;
}

}  // namespace neural_net
}  // namespace turi

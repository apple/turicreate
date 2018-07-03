#ifndef MPS_GRAPH_LAYERS_H_
#define MPS_GRAPH_LAYERS_H_

#import "mps_layers.h"
#import "mps_weight.h"
#import "mps_utils.h"
#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#import <string>
#import <unordered_map>
#import <vector>

#pragma clang diagnostic ignored "-Wunguarded-availability-new"

namespace turi {
namespace mps {

struct GraphLayer {
  virtual void Init(id<MTLDevice> _Nonnull device,
                    id<MTLCommandQueue> _Nonnull cmd_queue,
                    const FloatArrayMap &config,
                    const FloatArrayMap &weights) {}
  virtual void InitFwd(MPSNNImageNode *_Nonnull src) = 0;
  virtual void InitBwd(MPSNNImageNode *_Nonnull src) = 0;
  virtual void Load(const FloatArrayMap &weights) {}
  virtual void SetLearningRate(float lr) {}
  virtual void
  Export(std::unordered_map<std::string, std::tuple<std::string, float *, int,
                                                    std::vector<int>>> &table) {
  }

  virtual ~GraphLayer() {}

  void _Load(const std::string &key,
             const FloatArrayMap &weights,
             int dst_size,
             float *_Nonnull dst) {
    if (weights.count(key) > 0) {
      LogStdString("Loading weight: " + key);
      assert(weights.at(key).size == dst_size);
      size_t size = dst_size * sizeof(float);
      void *dest = (void *)dst;
      void *src = (void *)weights.at(key).data;
      std::memcpy(dest, src, size);
    }
  }

  MPSNNDefaultPadding *_Nonnull SetPaddingType(PaddingType pad_type) {
    switch (pad_type) {
    case kSame:
      return SAME;
    case kValid:
      return VALID;
    default:
      throw std::invalid_argument("Undefined padding type");
    }
  }

  // data
  MPSNNGradientStateBatch *_Nullable state;
  // output
  MPSNNImageNode * _Nonnull fwd_img_node;
  MPSNNImageNode * _Nonnull bwd_img_node;

  // type
  LayerType type;
  std::string name;

  // params
  std::vector<int> iparams;
  std::vector<float> fparams;
  std::vector<int> ishape;
  std::vector<int> oshape;
};

struct LossGraphLayer: public GraphLayer {
  MPSNNLabelsNode *labels_node = nil;

  virtual MPSCNNLossLabelsBatch *CreateLossState(id<MTLDevice> _Nonnull device,
                                                 float *data) const = 0;
};

// Individual Layers
// --------------------------------------------------------------------------------------------

struct ReLUGraphLayer : public GraphLayer {
  explicit ReLUGraphLayer(const std::string &layer_name,
                     const std::vector<float> &fp,
                     const std::vector<int> &i_shape,
                     const std::vector<int> &o_shape) {
    type = kReLU;
    name = layer_name;
    fparams = fp;
    state = nil;
    ishape = i_shape;
    oshape = o_shape;
  }
  ~ReLUGraphLayer() {}
  void InitFwd(MPSNNImageNode * _Nonnull src) override;
  void InitBwd(MPSNNImageNode * _Nonnull src) override;

  // node
  MPSCNNNeuronReLUNode * _Nonnull node_fwd;
  MPSCNNNeuronGradientNode * _Nonnull node_bwd;
};

struct ConvGraphLayer : public GraphLayer {
  explicit ConvGraphLayer(const std::string &layer_name, const std::vector<int> &ip,
                     const std::vector<int> &i_shape,
                     const std::vector<int> &o_shape) {
    type = kConv;
    name = layer_name;
    iparams = ip;
    state = nil;
    ishape = i_shape;
    oshape = o_shape;
  }
  ~ConvGraphLayer() {}

  void Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> _Nonnull cmd_queue,
            const FloatArrayMap &config,
            const FloatArrayMap &weights) override;
  void InitFwd(MPSNNImageNode * _Nonnull src) override;
  void InitBwd(MPSNNImageNode * _Nonnull src) override;
  void SetLearningRate(float lr) override;

  void
  Export(std::unordered_map<
         std::string, std::tuple<std::string, float *, int, std::vector<int>>>
             &table) override;
  // content
  bool use_bias{false};
  MPSCNNConvolutionNode *_Nonnull node_fwd;
  MPSCNNConvolutionGradientNode *_Nonnull node_bwd;
  MPSNNDefaultPadding *_Nonnull pad_policy;
  TCMPSConvolutionWeights *_Nonnull weight;
};

// BN Layer

struct BNGraphLayer : public GraphLayer {
  explicit BNGraphLayer(const std::string &layer_name, const std::vector<int> &ip,
                   const std::vector<int> &i_shape,
                   const std::vector<int> &o_shape) {
    type = kBN;
    name = layer_name;
    iparams = ip;
    ishape = i_shape;
    oshape = o_shape;
  }
  ~BNGraphLayer() {}

  void Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> _Nonnull cmd_queue,
            const FloatArrayMap &config,
            const FloatArrayMap &weights) override;
  void InitFwd(MPSNNImageNode * _Nonnull src) override;
  void InitBwd(MPSNNImageNode * _Nonnull src) override;
  void SetLearningRate(float lr) override;

  void
  Export(std::unordered_map<
         std::string, std::tuple<std::string, float *, int, std::vector<int>>>
             &table) override;

  TCMPSBatchNormData *_Nonnull data;
  MPSCNNBatchNormalizationNode *_Nonnull node_fwd;
  MPSCNNBatchNormalizationGradientNode * _Nonnull node_bwd;
};

struct MaxPoolGraphLayer : public GraphLayer {
  explicit MaxPoolGraphLayer(const std::string &layer_name,
                        const std::vector<int> &ip,
                        const std::vector<int> &i_shape,
                        const std::vector<int> &o_shape) {
    type = kMaxPool;
    name = layer_name;
    iparams = ip;
    state = nil;
    ishape = i_shape;
    oshape = o_shape;
  }
  ~MaxPoolGraphLayer() {}
  void InitFwd(MPSNNImageNode *_Nonnull src) override;
  void InitBwd(MPSNNImageNode *_Nonnull src) override;

  MPSCNNPoolingMaxNode *_Nonnull node_fwd;
  MPSCNNPoolingMaxGradientNode *_Nonnull node_bwd;
};


class YoloLossGraphLayer: public LossGraphLayer {
public:
  static std::vector<simd::float2> GetDefaultAnchorBoxes();

  struct Options {
    std::vector<simd::float2> anchor_boxes = GetDefaultAnchorBoxes();
    float scale_xy = 10.f;
    float scale_wh = 10.f;
    float scale_no_object = 5.f;
    float scale_object = 100.f;
    float scale_class = 2.f;
    float min_iou_for_object = 0.7f;
    float max_iou_for_no_object = 0.3f;
    bool rescore = true;
  };

  YoloLossGraphLayer(std::string layer_name, std::vector<int> i_shape, std::vector<int> o_shape,
                     Options options);

  void Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> _Nonnull cmd_queue,
            const FloatArrayMap &config,
            const FloatArrayMap &weights) override;
  void InitFwd(MPSNNImageNode *src) override;
  void InitBwd(MPSNNImageNode *src) override;

  MPSCNNLossLabelsBatch *CreateLossState(id<MTLDevice> _Nonnull device, float *data) const override;

private:
  Options options_;
  MPSCNNYOLOLossNode *yoloNode_;
};

}  // namespace mps
}  // namespace turi

#endif

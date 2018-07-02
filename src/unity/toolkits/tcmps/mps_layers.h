#ifndef MPS_LAYERS_H_
#define MPS_LAYERS_H_

#import "mps_updater.h"
#import "mps_weight.h"

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#import <string>
#import <unordered_map>
#import <vector>

#pragma clang diagnostic ignored "-Wunguarded-availability-new"

#define ADVANCE_PTR(_a, _size)                                                 \
  (__typeof__(_a))((uintptr_t)(_a) + (size_t)(_size))
//
// Common utilities for all Layers
// -----------------------------------------------------------------------------------------

@interface TCMPSImageAllocator : NSObject <MPSImageAllocator>

/*! @abstract   Create a new MPSImage
 *  @discussion See class description for sample implementation
 *  @param          cmdBuf      The MTLCommandBuffer on which the image will be
 * initialized. cmdBuf.device encodes the MTLDevice.
 *  @param          descriptor  A MPSImageDescriptor containing the image format
 * to use. This format is the result of your MPSPadding policy.
 *  @param          kernel      The kernel that will overwrite the image
 * returned by the filter.
 *
 *  @return         A valid MPSImage or MPSTemporaryImage. It will be
 * automatically released when the command buffer completes.
 */
- (MPSImage *__nonnull)
imageForCommandBuffer:(__nonnull id<MTLCommandBuffer>)cmdBuf
      imageDescriptor:(MPSImageDescriptor *__nonnull)descriptor
               kernel:(MPSKernel *__nonnull)kernel;

- (nullable instancetype)initWithFormat:(MPSImageFeatureChannelFormat)format;
- (nullable instancetype)initWithCoder:(NSCoder *__nonnull)aDecoder;
- (void)encodeWithCoder:(NSCoder *__nonnull)aCoder;
+ (BOOL)supportsSecureCoding;
@end

@interface MPSCNNWeight : NSObject <MPSCNNConvolutionDataSource>

{
@private
  MPSCNNConvolutionDescriptor *_desc;
  std::vector<float> _weight;
  std::vector<float> _bias;
}

- (nonnull instancetype)initWithKernelWidth:(NSUInteger)kernelWidth
                               kernelHeight:(NSUInteger)kernelHeight
                       inputFeatureChannels:(NSUInteger)inputFeatureChannels
                      outputFeatureChannels:(NSUInteger)outputFeatureChannels
                            strideInPixelsX:(NSUInteger)strideInPixelsX
                            strideInPixelsY:(NSUInteger)strideInPixelsY
                                 neuronType:(MPSCNNNeuronType)neuronType
                                    neuronA:(float)neuronA
                                    neuronB:(float)neuronB;

- (MPSDataType)dataType;
- (MPSCNNConvolutionDescriptor *__nonnull)descriptor;
- (void *__nonnull)weights;
- (float *__nullable)biasTerms;
- (BOOL)load;
- (void)purge;
- (void)checkpoint;
- (size_t)weight_size;
- (size_t)bias_size;

@end // MPSCNNWeight

namespace turi {
namespace mps {

struct MPSUpdater;

enum LayerType {
  kReLU,
  kLReLU,
  kConv,
  kBN,
  kMaxPool,
  kDropOut,
  kSoftMax,
  kSmceLoss,
  kLSTM,
  kYoloLoss,
};

static MPSNNDefaultPadding *_Nonnull SAME = [MPSNNDefaultPadding
    paddingWithMethod:MPSNNPaddingMethodAddRemainderToTopLeft |
                      MPSNNPaddingMethodAlignCentered |
                      MPSNNPaddingMethodSizeSame];

static MPSNNDefaultPadding *_Nonnull VALID = [MPSNNDefaultPadding
    paddingWithMethod:MPSNNPaddingMethodAlignCentered |
                      MPSNNPaddingMethodAddRemainderToTopLeft |
                      MPSNNPaddingMethodSizeValidOnly];

enum PaddingType {
  kSame = 0,
  kValid,
};

inline void LogStdString(const std::string &str) {
#if VERBOSE
  NSString *msg =
      [NSString stringWithCString:str.c_str()
                         encoding:[NSString defaultCStringEncoding]];
  NSLog(@"%@", msg);
#endif
}

struct Layer {

  virtual void Forward(MPSImageBatch *_Nonnull src,
                       id<MTLCommandBuffer> _Nonnull cb,
                       bool is_train = true) = 0;
  virtual void Backward(MPSImageBatch *_Nonnull src,
                        id<MTLCommandBuffer> _Nonnull cb) = 0;
  virtual void Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_q,
                    const FloatArrayMap &config, bool is_train,
                    LowLevelMode net_mode, bool is_output_layer) = 0;
  virtual void Load(const FloatArrayMap &weights) {}
  virtual void
  Export(std::unordered_map<std::string, std::tuple<std::string, float *, int,
                                                    std::vector<int>>> &table) {
  }
  virtual void Update(MPSUpdater *_Nonnull updater, int lid) {}
  virtual void GpuUpdate(id<MTLCommandBuffer> _Nonnull cb) {}

virtual void AllocImage(id<MTLDevice> _Nonnull device , bool is_train = true) {
    int n = ishape[0];
    int h = ishape[1];
    int w = ishape[2];
    int ci = ishape[3];
    int co = oshape[3];

    MPSImageDescriptor *input_desc = [MPSImageDescriptor
        imageDescriptorWithChannelFormat:MPSImageFeatureChannelFormatFloat32
                                   width:w
                                  height:h
                         featureChannels:ci
                          numberOfImages:1
                                   usage:MTLTextureUsageShaderWrite |
                                         MTLTextureUsageShaderRead];

    MPSImageDescriptor *output_desc = [MPSImageDescriptor
        imageDescriptorWithChannelFormat:MPSImageFeatureChannelFormatFloat32
                                   width:w
                                  height:h
                         featureChannels:co
                          numberOfImages:1
                                   usage:MTLTextureUsageShaderWrite |
                                         MTLTextureUsageShaderRead];
    fwd_output = @[];
    bwd_output = @[];
    for (int i = 0; i < n; ++i) {
      MPSImage *img =
      [[MPSImage alloc] initWithDevice:device imageDescriptor:input_desc];
      fwd_output = [fwd_output arrayByAddingObject:img];
    }

    if (is_train)
    {
      for (int i = 0; i < n; ++i) {
        MPSImage *img =
        [[MPSImage alloc] initWithDevice:device imageDescriptor:output_desc];
        bwd_output = [bwd_output arrayByAddingObject:img];
      }
    }
  }
  
  virtual MPSImageBatch * AllocTempImageBatch(id<MTLCommandBuffer>_Nonnull cb, bool is_output){
    int n = ishape[0];
    int h = ishape[1];
    int w = ishape[2];
    int c = (is_output) ? oshape[3] : ishape[3];
    
    MPSImageDescriptor *desc = [MPSImageDescriptor
                                imageDescriptorWithChannelFormat:MPSImageFeatureChannelFormatFloat32
                                width:w
                                height:h
                                featureChannels:c
                                numberOfImages:1
                                usage:MTLTextureUsageShaderWrite |
                                MTLTextureUsageShaderRead];
    
    MPSImageBatch * batch =@[];
    for (int i = 0; i < n; ++i) {
      MPSTemporaryImage * temp_img = [MPSTemporaryImage
                                      temporaryImageWithCommandBuffer:cb
                                      imageDescriptor:desc];
      batch = [batch arrayByAddingObject:temp_img];
    }
    
    return batch;
  }

  virtual ~Layer() {}

  void _Load(const std::string &key,
             const FloatArrayMap &weights, int dst_size,
             float *_Nonnull dst) {
    if (weights.count(key) > 0) {
      const FloatArray &arr = weights.at(key);
      LogStdString("Loading weight: " + key);
      assert(arr.size == dst_size);
      size_t size = dst_size * sizeof(float);
      void *dest = (void *)dst;
      void *src = (void *)arr.data;
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
  MPSImageBatch *_Nonnull input{nil};
  MPSImageBatch *_Nonnull fwd_output{nil};
  MPSImageBatch *_Nullable bwd_output{nil};
  MPSNNGradientStateBatch *_Nullable state;

  // type
  LayerType type;
  std::string name;

  // params
  std::vector<int> iparams;
  std::vector<float> fparams;
  std::vector<int> ishape;
  std::vector<int> oshape;
};

struct LossLayer : public Layer {
  void Forward(MPSImageBatch *_Nonnull src,
               id<MTLCommandBuffer> _Nonnull cb,
               bool is_train = true) override {}
  void Backward(MPSImageBatch *_Nonnull src,
                id<MTLCommandBuffer> _Nonnull cb) override {}
  virtual void Loss(MPSImageBatch *_Nonnull src,
                    MPSCNNLossLabelsBatch *_Nonnull labels,
                    id<MTLCommandBuffer> _Nonnull cb) = 0;

};

// Individual Layers
// --------------------------------------------------------------------------------------------

struct ReLULayer : public Layer {
  explicit ReLULayer(const std::string &layer_name,
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
  ~ReLULayer() {}
  void Forward(MPSImageBatch *_Nonnull src,
               id<MTLCommandBuffer> _Nonnull cb,
               bool is_train = true) override;
  void Backward(MPSImageBatch *_Nonnull src,
                id<MTLCommandBuffer> _Nonnull cb) override;
  void Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_q,
            const FloatArrayMap &config, bool is_train, LowLevelMode net_mode, bool is_output_layer) override;

  // content
  MPSCNNNeuronReLU *_Nonnull op_forward;
  MPSCNNNeuronGradient *_Nullable op_backward{nil};
};

struct ConvLayer : public Layer {
  explicit ConvLayer(const std::string &layer_name, const std::vector<int> &ip,
                     const std::vector<int> &i_shape,
                     const std::vector<int> &o_shape) {
    type = kConv;
    name = layer_name;
    iparams = ip;
    state = nil;
    ishape = i_shape;
    oshape = o_shape;
  }
  ~ConvLayer() {}

  void Forward(MPSImageBatch *_Nonnull src,
               id<MTLCommandBuffer> _Nonnull cb,
               bool is_train = true) override;
  void Backward(MPSImageBatch *_Nonnull src,
                id<MTLCommandBuffer> _Nonnull cb) override;
  void Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_q,
            const FloatArrayMap &config, bool is_train, LowLevelMode net_mode, bool is_output_layer) override;
  void Load(const FloatArrayMap &weights) override;

  void
  Export(std::unordered_map<
         std::string, std::tuple<std::string, float *, int, std::vector<int>>>
             &table) override;
  void Update(MPSUpdater *_Nonnull updater, int lid) override;
  void GpuUpdate(id<MTLCommandBuffer> _Nonnull cb) override;

  // content
  bool use_bias{false};
  MPSCNNConvolution *_Nonnull op_forward;
  MPSCNNConvolutionGradient *_Nullable op_backward{nil};
  MPSCNNConvolutionDescriptor *_Nonnull desc;
//  MPSCNNWeight *_Nonnull weight;
  TCMPSConvolutionWeights *_Nonnull weight;
};

struct BNLayer : public Layer {
  explicit BNLayer(const std::string &layer_name, const std::vector<int> &ip,
                   const std::vector<int> &i_shape,
                   const std::vector<int> &o_shape) {
    type = kBN;
    name = layer_name;
    iparams = ip;
    state = nil;
    bn_state = nil;
    ishape = i_shape;
    oshape = o_shape;
  }
  ~BNLayer() {}

  void Forward(MPSImageBatch *_Nonnull src,
               id<MTLCommandBuffer> _Nonnull cb,
               bool is_train = true) override;
  void Backward(MPSImageBatch *_Nonnull src,
                id<MTLCommandBuffer> _Nonnull cb) override;
  void Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_q,
            const FloatArrayMap &config, bool is_train, LowLevelMode net_mode, bool is_output_layer) override;
  void Load(const FloatArrayMap &weights) override;
  void
  Export(std::unordered_map<
         std::string, std::tuple<std::string, float *, int, std::vector<int>>>
             &table) override;

  void Update(MPSUpdater *_Nonnull updater, int lid) override;
  void GpuUpdate(id<MTLCommandBuffer> _Nonnull cb) override;

  bool is_state_init{false};
  bool is_train_mode_{true};
  bool use_temp_images_{true};
  TCMPSBatchNormData *_Nonnull data;
  MPSCNNBatchNormalizationStatistics *_Nullable         stat{nil};
  MPSCNNBatchNormalization *_Nonnull                    op_forward;
  MPSCNNBatchNormalizationGradient *_Nullable           op_backward{nil};
  MPSCNNBatchNormalizationStatisticsGradient *_Nullable g_stat{nil};
  MPSCNNBatchNormalizationState *_Nullable              bn_state{nil};
};

struct MaxPoolLayer : public Layer {
  explicit MaxPoolLayer(const std::string &layer_name,
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
  ~MaxPoolLayer() {}
  void Forward(MPSImageBatch *_Nonnull src,
               id<MTLCommandBuffer> _Nonnull cb,
               bool is_train = true) override;
  void Backward(MPSImageBatch *_Nonnull src,
                id<MTLCommandBuffer> _Nonnull cb) override;
  void Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_q,
            const FloatArrayMap &config, bool is_train, LowLevelMode net_mode, bool is_output_layer) override;

  MPSCNNPoolingMax *_Nonnull op_forward;
  MPSCNNPoolingMaxGradient *_Nullable op_backward{nil};
};

struct DropOutLayer : public Layer {
  explicit DropOutLayer(const std::string &layer_name,
                        const std::vector<int> &ip,
                        const std::vector<int> &i_shape,
                        const std::vector<int> &o_shape) {
    type = kDropOut;
    name = layer_name;
    iparams = ip;
    state = nil;
    ishape = i_shape;
    oshape = o_shape;
  }
  DropOutLayer() {
    // [op_forward dealloc];
    // [op_backward dealloc];
  }
  void Forward(MPSImageBatch *_Nonnull src,
               id<MTLCommandBuffer> _Nonnull cb,
               bool is_train = true) override;
  void Backward(MPSImageBatch *_Nonnull src,
                id<MTLCommandBuffer> _Nonnull cb) override;
  void Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_q,
            const FloatArrayMap &config, bool is_train, LowLevelMode net_mode, bool is_output_layer) override;

  // content
  MPSCNNDropout *_Nonnull op_forward;
  MPSCNNDropoutGradient *_Nullable op_backward{nil};
};

struct SoftMaxLayer : public Layer {
  explicit SoftMaxLayer(const std::string &layer_name,
                        const std::vector<int> &ip,
                        const std::vector<int> &i_shape,
                        const std::vector<int> &o_shape) {
    type = kSoftMax;
    name = layer_name;
    iparams = ip;
    state = nil;
    ishape = i_shape;
    oshape = o_shape;
  }
  SoftMaxLayer() {
    // [op_forward dealloc];
    // [op_backward dealloc];
  }
  void Forward(MPSImageBatch *_Nonnull src,
               id<MTLCommandBuffer> _Nonnull cb,
               bool is_train = true) override;
  void Backward(MPSImageBatch *_Nonnull src,
                id<MTLCommandBuffer> _Nonnull cb) override;
  void Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_q,
            const FloatArrayMap &config, bool is_train, LowLevelMode net_mode, bool is_output_layer) override;

  // content
  MPSCNNSoftMax *_Nonnull op_forward;
  MPSCNNSoftMaxGradient *_Nullable op_backward{nil};
};

struct SmceLossLayer : public LossLayer {
  explicit SmceLossLayer(const std::string &layer_name,
                         const std::vector<int> &ip,
                         const std::vector<int> &i_shape,
                        const std::vector<int> &o_shape) {
    type = kSmceLoss;
    name = layer_name;
    iparams = ip;
    state = nil;
    ishape = i_shape;
    oshape = o_shape;
  }
  SmceLossLayer() {
    // [op_forward dealloc];
    // [op_backward dealloc];
    
    // TODO: add free all matrices and array. Mostly copy_weight_matrices_
  }
  virtual void Loss(MPSImageBatch *_Nonnull src,
                    MPSCNNLossLabelsBatch *_Nonnull labels,
                    id<MTLCommandBuffer> _Nonnull cb) override;
  void Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_q,
            const FloatArrayMap &config, bool is_train = true,
            LowLevelMode net_mode = kLowLevelModeTrain, bool is_output_layer = true) override;

  // content
  MPSCNNLoss *_Nonnull op_loss;
};

struct LstmLayer : public Layer {
  explicit LstmLayer(const std::string &layer_name,
                        const std::vector<int> &ip,
                        const std::vector<int> &i_shape,
                        const std::vector<int> &o_shape) {
    type = kLSTM;
    name = layer_name;
    iparams = ip;
    state = nil;
    ishape = i_shape;
    oshape = o_shape;
  }
  LstmLayer() {
    // [op_forward dealloc];
    // [op_backward dealloc];
  }
  void Forward(MPSImageBatch *_Nonnull src,
               id<MTLCommandBuffer> _Nonnull cb,
               bool is_train = true) override;
  void Backward(MPSImageBatch *_Nonnull src,
                id<MTLCommandBuffer> _Nonnull cb) override;
  void Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_q,
            const FloatArrayMap &config, bool is_train, LowLevelMode net_mode, bool is_output_layer) override;
  void Load(const FloatArrayMap &weights) override;
  void
  Export(std::unordered_map<
         std::string, std::tuple<std::string, float *, int, std::vector<int>>>
         &table) override;
  void GpuUpdate(id<MTLCommandBuffer> _Nonnull cb) override;
  
private:
  NSArray<MPSMatrix *> * _Nonnull CreateMatrixViews(id <MTLBuffer> _Nonnull buffer,
                                                    NSUInteger num_features);
  std::vector<NSUInteger> CreateMatrixOffsets(NSUInteger num_features);
  void CopyImageBatchToBuffer(MPSImageBatch * _Nonnull imgBatch, id <MTLBuffer> _Nonnull buffer,
                              NSUInteger num_features, id <MTLCommandBuffer> _Nonnull cb);
  MPSImageBatch * _Nonnull CopyImageBatchFromBuffer(id <MTLBuffer> _Nonnull output_buffer,
                                                    NSUInteger num_features,
                                                    id <MTLCommandBuffer> _Nonnull cb);
  
  void InitWeightCopyMatrices();

  NSUInteger batch_size_ = 0;
  NSUInteger sequence_length_ = 0;
  NSUInteger num_input_features_ = 0;
  NSUInteger num_output_features_ = 0;
  
  bool use_temp_image_;

  // kernels
  MPSImageCopyToMatrix * _Nonnull image_to_matrix_kernel_ = nil;
  MPSMatrixCopyToImage * _Nonnull matrix_to_image_kernel_ = nil;
  
  // OPtimizer
  //MPSNNOptimizerAdam *_Nonnull optimizer = nil;
  NSMutableArray *_Nonnull optimizers = nil;

  // content
  MPSRNNMatrixTrainingLayer *_Nonnull filter;
  NSMutableArray *_Nonnull weights = [NSMutableArray new];
  NSMutableArray *_Nonnull weightGradients = [NSMutableArray new];
  NSMutableArray *_Nonnull trainingStates = [NSMutableArray new];
  NSMutableArray *_Nonnull weightsFirstMoment = [NSMutableArray new];
  NSMutableArray *_Nonnull weightsSecondMoment = [NSMutableArray new];

  // image/matrix buffers
  id <MTLBuffer> _Nonnull fwd_src_buffer_ = nil;
  id <MTLBuffer> _Nonnull fwd_dst_buffer_ = nil;
  id <MTLBuffer> _Nonnull bwd_src_buffer_ = nil;
  id <MTLBuffer> _Nonnull bwd_dst_buffer_ = nil;
  
  id<MTLCommandQueue> _Nonnull cmd_q_;
  id<MTLDevice> _Nonnull device_;
  
  std::unordered_map<std::string, MPSMatrix *> copy_weight_matrices_;
  
};

}  // namespace mps
}  // namespace turi

#endif

#ifndef MPS_NETWORKS_H_
#define MPS_NETWORKS_H_

#import "unordered_map"
#import "vector"
#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#pragma clang diagnostic ignored "-Wunguarded-availability-new"
#import "mps_layers.h"
#import "mps_updater.h"
#import "mps_utils.h"

namespace turi {
namespace neural_net {

struct Layer;
struct Updater;
struct ConvLayer;

enum NetworkType {
  kSingleReLUNet = 0,
  kSingleConvNet,
  kSingleBNNet,
  kSingleMPNet,
  kSingle1DConvNet,
  kODNet,
  kSingleDropOutNet,
  kSingleFcNet,
  kSingleSoftMaxNet,
  kActivityClassifierNet,
  kSingleLstmNet,
  NUM_SUPPORTED_NETWORK_TYPES
};

struct MPSNetwork {

  std::vector<Layer *> layers;
  LossLayer *_Nullable lossLayer = nil;
  int batch_size{0};

  MPSNetwork(){};
  virtual ~MPSNetwork();

  explicit MPSNetwork(const float_array_map& config) {
      std::string mode_key = "mode";
      network_mode_ = (LowLevelMode) get_array_map_scalar(config, mode_key, kLowLevelModeTrain);
      is_train_ = (kLowLevelModeTrain == network_mode_ || kLowLevelModeTest == network_mode_);
  }

  void Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> cmd_q,
            const float_array_map& config);
  MPSImageBatch *_Nonnull Forward(MPSImageBatch *_Nonnull src,
                                  id<MTLCommandBuffer> _Nonnull cb,
                                  bool is_train = true);
  MPSImageBatch *_Nonnull Backward(MPSImageBatch *_Nonnull src,
                                   id<MTLCommandBuffer> _Nonnull cb);
  MPSImageBatch *_Nonnull Loss(MPSImageBatch *_Nonnull src,
                               MPSCNNLossLabelsBatch *_Nonnull labels,
                               id<MTLCommandBuffer> _Nonnull cb);
  void SyncState(id<MTLCommandBuffer> _Nonnull cb);
  void Load(const float_array_map& weights);
  float_array_map Export() const;
  int NumParams();

  void Update(MPSUpdater *_Nonnull updater);
  void GpuUpdate(id<MTLCommandBuffer> _Nonnull cb);

  LowLevelMode network_mode_{kLowLevelModeTrain};
  bool is_train_{true};
};

// Factory function to create a network
MPSNetwork *_Nonnull createNetwork(NetworkType network_id,
                                   const std::vector<int> &params,
                                   const float_array_map& config);

// Various networks
// ---------------------------------------------------------------------------------------------

// Unit testing networks
// ---------------------------------------------------------------------------------------------
struct SingleConvNetwork : public MPSNetwork {
  SingleConvNetwork(const std::vector<int> &iparam,
                    const float_array_map& config) : MPSNetwork(config) {

    int n = iparam[0];
    int hi = iparam[1];
    int wi = iparam[2];
    int ci = iparam[3];
    int ho = iparam[4];
    int wo = iparam[5];
    int co = iparam[6];
    layers.resize(1);
    layers[0] = new ConvLayer("conv0", {3, 3, ci, co, 1, 1, 0, 0},
                              {n, hi, wi, ci}, {n, ho, wo, co});
  }
};

struct Single1DConvNetwork : public MPSNetwork {
  Single1DConvNetwork(const std::vector<int> &iparam,
                      const float_array_map& config) : MPSNetwork(config) {

    int n = iparam[0];
    int hi = iparam[1];
    int wi = iparam[2];
    int ci = iparam[3];
    int ho = iparam[4];
    int wo = iparam[5];
    int co = iparam[6];

    int pred_window = 3;

    layers.resize(1);
    layers[0] = new ConvLayer("conv0", {1, pred_window, ci, co, pred_window, 1, 1, 1}, {n, hi, wi, ci}, {n, ho, wo, co});
  }
};
struct SingleReLUNetwork : public MPSNetwork {
  SingleReLUNetwork(const std::vector<int> &iparam,
                    const float_array_map& config) : MPSNetwork(config) {

    int n = iparam[0];
    int hi = iparam[1];
    int wi = iparam[2];
    int ci = iparam[3];
    int ho = iparam[4];
    int wo = iparam[5];
    int co = iparam[6];
    layers.resize(1);
    layers[0] = new ReLULayer("relu0", {0.f}, {n, hi, wi, ci}, {n, ho, wo, co});
  }
};

struct SingleBNNetwork : public MPSNetwork {
  SingleBNNetwork(const std::vector<int> &iparam,
                  const float_array_map& config) : MPSNetwork(config) {

    int n = iparam[0];
    int hi = iparam[1];
    int wi = iparam[2];
    int ci = iparam[3];
    int ho = iparam[4];
    int wo = iparam[5];
    int co = iparam[6];
    layers.resize(1);
    layers[0] = new BNLayer("bn0", {5}, {n, hi, wi, ci}, {n, ho, wo, co});
  }
};

struct SingleMPNetwork : public MPSNetwork {
  SingleMPNetwork(const std::vector<int> &iparam,
                  const float_array_map& config) : MPSNetwork(config) {

    layers.resize(1);
    int n = iparam[0];
    int hi = iparam[1];
    int wi = iparam[2];
    int ci = iparam[3];
    int ho = iparam[4];
    int wo = iparam[5];
    int co = iparam[6];
    layers[0] =
        new MaxPoolLayer("mp0", {2, 2, 2, 2}, {n, hi, wi, ci}, {n, ho, wo, co});
  }
};

struct ODNetwork : public MPSNetwork {
  ODNetwork(const std::vector<int> &iparam, const float_array_map& config)
      : MPSNetwork(config) {

    int n = iparam[0];
    int hi = iparam[1];
    int wi = iparam[2];
    // int ci = iparam[3];
    // int ho = iparam[4];
    // int wo = iparam[5];
    int co = iparam[6];
    std::vector<int> filter = {3, 16, 32, 64, 128, 256, 512, 1024, 1024};
    for (int i = 0; size_t(i) < filter.size() - 1; ++i) {
      int idx = i + 1;
      std::string num = std::to_string(i);
      layers.push_back(new ConvLayer(
          "conv" + num, {3, 3, filter[i], filter[i + 1], 1, 1, 0, 0}, {}, {}));
      layers.push_back(new BNLayer("batchnorm" + num, {filter[i + 1]},
                                   {n, hi, wi, filter[i + 1]},
                                   {n, hi, wi, filter[i + 1]}));
      layers.push_back(new ReLULayer("leakyrelu" + num, {0.1},
                                     {n, hi, wi, filter[i + 1]},
                                     {n, hi, wi, filter[i + 1]}));
      if (idx < 6) {
        layers.push_back(new MaxPoolLayer("pool" + num, {2, 2, 2, 2}, {}, {}));
        hi /= 2;
        wi /= 2;
      } else if (idx == 6) {
        layers.push_back(new MaxPoolLayer("pool" + num, {2, 2, 1, 1}, {}, {}));
      }
    }
    layers.push_back(
        new ConvLayer("conv8", {1, 1, 1024, co, 1, 1, 0, 1}, {}, {}));
  }
};

struct SingleDropOutNetwork : public MPSNetwork {
  SingleDropOutNetwork(const std::vector<int> &iparam,
                       const float_array_map& config) : MPSNetwork(config) {

    int n = iparam[0];
    int hi = iparam[1];
    int wi = iparam[2];
    int ci = iparam[3];
    int ho = iparam[4];
    int wo = iparam[5];
    int co = iparam[6];

    layers.resize(1);
    layers[0] = new DropOutLayer("do0", {50, -1}, {n, hi, wi, ci}, {n, ho, wo, co});
  }
};

struct ActivityClassifierNetwork : public MPSNetwork {
  ActivityClassifierNetwork(const std::vector<int> &iparam,
                            const float_array_map& config)
      : MPSNetwork(config) {

    assert(iparam.size() >= 7);
    int n = iparam[0];
    int hi = iparam[1];
    int wi = iparam[2];
    int ci = iparam[3];
    int ho = iparam[4];
    int wo = iparam[5];
    int co = iparam[6];
    assert(wi % wo == 0);

    int k_w = (int) get_array_map_scalar(config, "ac_pred_window", wi/wo);
    int seq_len = (int) get_array_map_scalar(config, "ac_seq_len", wo);

    int conv_filters = 64;
    int lstm_h_size = 200;
    int fc_hidden = 128;

    layers.push_back(new ConvLayer("conv", {1, k_w, ci, conv_filters, k_w, 1, 1, 1},
                                   {n, hi, wi, ci}, {n, ho, seq_len, conv_filters}));
    layers.push_back(new ReLULayer("relu1", {0.f}, {n, hi, seq_len, conv_filters}, {n, ho, seq_len, conv_filters}));

    if (kLowLevelModeTrain == network_mode_){
        layers.push_back(new DropOutLayer("do2", {80, -1}, {n, hi, seq_len, conv_filters}, {n, ho, seq_len, conv_filters}));
    }

    layers.push_back(new LstmLayer("lstm", {}, {n, hi, seq_len, conv_filters}, {n, ho, seq_len,lstm_h_size}));
    layers.push_back(new ConvLayer("dense0", {1, 1, lstm_h_size, fc_hidden, 1, 1, 1, 1},
                                   {n, hi, seq_len, lstm_h_size}, {n, ho, seq_len, fc_hidden}));
    layers.push_back(new BNLayer("bn", {}, {n, hi, seq_len, fc_hidden}, {n, ho, seq_len, fc_hidden}));
    layers.push_back(new ReLULayer("relu6", {0.f}, {n, hi, seq_len, fc_hidden}, {n, ho, seq_len, fc_hidden}));

    if (kLowLevelModeTrain == network_mode_){
        layers.push_back(new DropOutLayer("do7", {50, -1}, {n, hi, seq_len, fc_hidden}, {n, ho, seq_len, fc_hidden}));
    }

    layers.push_back(new ConvLayer("dense1", {1, 1, fc_hidden, co, 1, 1, 1, 1}, {n, hi, seq_len, fc_hidden}, {n, ho, seq_len, co}));

    if (kLowLevelModeInference == network_mode_){
        layers.push_back(new SoftMaxLayer("softmax", {}, {n, ho, seq_len, co}, {n, ho, seq_len, co}));
    } else {
        lossLayer = new SmceLossLayer("Smce", {n}, {n, ho, seq_len, co}, {n, ho, seq_len, co});
    }
  }
};

struct SingleFcNetwork : public MPSNetwork {
  SingleFcNetwork(const std::vector<int> &iparam, const float_array_map& config)
      : MPSNetwork(config) {

    int n = iparam[0];
    int hi = iparam[1];
    int wi = iparam[2];
    int ci = iparam[3];
    int ho = iparam[4];
    int wo = iparam[5];
    int co = iparam[6];

    layers.resize(1);
    layers[0] = new ConvLayer("fc0", {1, 1, 3, 1024, 1, 1, 1, 0}, {n, hi, wi, ci}, {n, ho, wo, co});
  }
};

struct SingleSoftMaxNetwork : public MPSNetwork {
  SingleSoftMaxNetwork(const std::vector<int> &iparam,
                       const float_array_map& config) : MPSNetwork(config) {

    int n = iparam[0];
    int hi = iparam[1];
    int wi = iparam[2];
    int ci = iparam[3];
    int ho = iparam[4];
    int wo = iparam[5];
    int co = iparam[6];

    layers.resize(1);
    layers[0] = new SoftMaxLayer("sm0", {}, {n, hi, wi, ci}, {n, ho, wo, co});
  }
};

struct SingleLstmNetwork : public MPSNetwork {
  SingleLstmNetwork(const std::vector<int> &iparam,
                    const float_array_map& config) : MPSNetwork(config) {

        int n = iparam[0];
        int hi = iparam[1];
        int wi = iparam[2];
        int ci = iparam[3];
        int ho = iparam[4];
        int wo = iparam[5];
        int co = iparam[6];
        layers.resize(1);
        layers[0] = new LstmLayer("lstm0", {},
                                  {n, hi, wi, ci}, {n, ho, wo, co});
    }
};

}  // namespace neural_net
}  // namespace turi

#endif

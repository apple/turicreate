#ifndef MPS_GRAPH_NETWORKS_H_
#define MPS_GRAPH_NETWORKS_H_

#import <memory>
#import <unordered_map>
#import <vector>

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#pragma clang diagnostic ignored "-Wunguarded-availability-new"
#import "mps_graph_layers.h"
#import "mps_networks.h"
#import "mps_utils.h"

namespace turi {
namespace neural_net {

struct GraphLayer;

enum GraphNetworkType {
  kSingleReLUGraphNet = 0,
  kSingleConvGraphNet,
  kSingleBNGraphNet,
  kSingleMPGraphNet,
  kODGraphNet,
  kSTGraphNet,
  NUM_SUPPORTED_GRAPH_NETWORK_TYPES
};

struct MPSGraphNetwork {
  std::vector<GraphLayer *> layers;
  std::unique_ptr<LossGraphLayer> loss_layer_;
  int batch_size{0};

  MPSGraphNetwork(){};
  virtual ~MPSGraphNetwork();

  void Init(id<MTLDevice> _Nonnull device, id<MTLCommandQueue> _Nonnull cmd_queue,
            GraphMode mode,
            const float_array_map& config, const float_array_map& weights);
  MPSImageBatch * _Nonnull RunGraph(id<MTLCommandBuffer>_Nonnull  cb, MPSImageBatch *_Nonnull src,
                          MPSCNNLossLabelsBatch *_Nonnull loss_state);
  MPSImageBatch * _Nonnull RunGraph(id<MTLCommandBuffer> _Nonnull  cb, NSDictionary *_Nonnull inputs);
  float_array_map Export() const;

  MPSNNGraph *_Nonnull  graph;
  MPSNNImageNode *_Nonnull  input_node;
  MPSNNImageNode * _Nullable grad_node;
};

// Factory function to create a network
std::unique_ptr<MPSGraphNetwork> createNetworkGraph(
    GraphNetworkType network_id, const std::vector<int> &params,
    const float_array_map& config);

// Various networks
// ---------------------------------------------------------------------------------------------
// -- TODO: Add object detection network
// -- TODO: Add activity classifier network

// Unit testing networks
// ---------------------------------------------------------------------------------------------

struct SingleConvNetworkGraph : public MPSGraphNetwork {
  SingleConvNetworkGraph(const std::vector<int> &iparam,
                         const float_array_map& config) {
    int n = iparam[0];
    int hi = iparam[1];
    int wi = iparam[2];
    int ci = iparam[3];
    int ho = iparam[4];
    int wo = iparam[5];
    int co = iparam[6];
    int kernel_size = (int)get_array_map_scalar(config, "single_conv_kernel_size", 3);
    layers.resize(1);
    layers[0] = new ConvGraphLayer("conv0", {kernel_size, kernel_size, ci, co, 1, 1, 0, 0},
                              {n, hi, wi, ci}, {n, ho, wo, co});
  }
};

struct SingleReLUNetworkGraph : public MPSGraphNetwork {
  SingleReLUNetworkGraph(const std::vector<int> &iparam,
                         const float_array_map& config) {
    int n = iparam[0];
    int hi = iparam[1];
    int wi = iparam[2];
    int ci = iparam[3];
    int ho = iparam[4];
    int wo = iparam[5];
    int co = iparam[6];
    float leak = get_array_map_scalar(config, "single_relu_leak", 0.0f);
    layers.resize(1);
    layers[0] = new ReLUGraphLayer("relu0", {leak}, {n, hi, wi, ci}, {n, ho, wo, co});
  }
};

struct SingleBNNetworkGraph : public MPSGraphNetwork {
  SingleBNNetworkGraph(const std::vector<int> &iparam,
                       const float_array_map& config) {
    int n = iparam[0];
    int hi = iparam[1];
    int wi = iparam[2];
    int ci = iparam[3];
    int ho = iparam[4];
    int wo = iparam[5];
    int co = iparam[6];
    layers.resize(1);
    layers[0] = new BNGraphLayer("bn0", {ci}, {n, hi, wi, ci}, {n, ho, wo, co});
  }
};

struct SingleMPNetworkGraph : public MPSGraphNetwork {
  SingleMPNetworkGraph(const std::vector<int> &iparam,
                       const float_array_map& config) {
    layers.resize(1);
    int n = iparam[0];
    int hi = iparam[1];
    int wi = iparam[2];
    int ci = iparam[3];
    int ho = iparam[4];
    int wo = iparam[5];
    int co = iparam[6];
    int kernel_size = (int)get_array_map_scalar(config, "single_maxpool_kernel_size", 2);
    int stride = (int)get_array_map_scalar(config, "single_maxpool_stride", 2);
    layers[0] =
        new MaxPoolGraphLayer("mp0", {kernel_size, kernel_size, stride, stride}, {n, hi, wi, ci}, {n, ho, wo, co});
  }
};

struct ODNetworkGraph : public MPSGraphNetwork {
  ODNetworkGraph(const std::vector<int> &iparam,
                 const float_array_map& config) {
    int n = iparam[0];
    int hi = iparam[1];
    int wi = iparam[2];
    int ci = iparam[3];
    int ho = iparam[4];
    int wo = iparam[5];
    int co = iparam[6];

    bool include_network = get_array_map_bool(config, "od_include_network", true);
    bool include_loss = get_array_map_bool(config, "od_include_loss", true);

    if (include_network) {
      std::vector<int> filter = {3, 16, 32, 64, 128, 256, 512, 1024, 1024};
      for (int idx = 1; size_t(idx) < filter.size(); ++idx) {
        std::string num = std::to_string(idx - 1);

        layers.push_back(new ConvGraphLayer(
            "conv" + num, {3, 3, filter[idx - 1], filter[idx], 1, 1, kSame, 0}, {}, {}));
        layers.push_back(new BNGraphLayer("batchnorm" + num, {filter[idx]},
                                     {n, hi, wi, filter[idx]},
                                     {n, hi, wi, filter[idx]}));
        layers.push_back(new ReLUGraphLayer("leakyrelu" + num, {0.1},
                                       {n, hi, wi, filter[idx]},
                                       {n, hi, wi, filter[idx]}));
        if (idx < 6) {
          layers.push_back(new MaxPoolGraphLayer("pool" + num, {2, 2, 2, 2}, {}, {}));
          hi /= 2;
          wi /= 2;
        } else if (idx == 6) {
          layers.push_back(new MaxPoolGraphLayer("pool" + num, {2, 2, 1, 1}, {}, {}));
        }
      }
      layers.push_back(
          new ConvGraphLayer("conv8", {1, 1, filter[filter.size() - 1], co, 1, 1, kSame, 1}, {}, {}));
    } else {
      // Add token layer
      layers.push_back(
          new ReLUGraphLayer("nop", {1.0f}, {n, hi, wi, ci}, {n, ho, wo, co}));
    }

    if (include_loss) {
      loss_layer_.reset(new YoloLossGraphLayer("yololoss", {}, {n, ho, wo, co}, YoloLossGraphLayer::Options()));
    }
  }
};

}  // namespace neural_net
}  // namespace turi

#endif

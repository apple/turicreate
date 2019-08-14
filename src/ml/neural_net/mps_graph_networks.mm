#include "mps_graph_networks.h"
#include "mps_graph_layers.h"
#include "mps_node_handle.h"

namespace turi {
namespace neural_net {

std::unique_ptr<MPSGraphNetwork> createNetworkGraph(
    GraphNetworkType network_id, const std::vector<int> &params,
    const float_array_map& config) {
  std::unique_ptr<MPSGraphNetwork> result;
  switch (network_id) {
  case kSingleReLUGraphNet:
    result.reset(new SingleReLUNetworkGraph(params, config));
    break;
  case kSingleConvGraphNet:
    result.reset(new SingleConvNetworkGraph(params, config));
    break;
  case kSingleMPGraphNet:
    result.reset(new SingleMPNetworkGraph(params, config));
    break;
  case kSingleBNGraphNet:
    result.reset(new SingleBNNetworkGraph(params, config));
    break;
  case kODGraphNet:
    result.reset(new ODNetworkGraph(params, config));
    break;
  default:
    throw std::invalid_argument("Undefined network.");
  }
  return result;
}

// MPS Network base class
// ---------------------------------------------------------------------------------------
MPSGraphNetwork::~MPSGraphNetwork() {
  for (size_t i = 0; i < layers.size(); ++i) {
    delete layers[i];
  }
}

void MPSGraphNetwork::Init(id<MTLDevice> _Nonnull device,
                           id<MTLCommandQueue> cmd_queue,
                           GraphMode mode,
                           const float_array_map& config,
                           const float_array_map& weights) {
  for (size_t i = 0; i < layers.size(); ++i) {
    layers[i]->Init(device, cmd_queue, config, weights);
  }
  input_node =
      [MPSNNImageNode nodeWithHandle:[TCMPSGraphNodeHandle handleWithLabel:@"input"]];
  MPSNNImageNode *src = input_node;
  for (size_t i = 0; i < layers.size(); ++i) {
    layers[i]->InitFwd(src);
    src = layers[i]->fwd_img_node;
  }
  if (mode == kGraphModeTrain || mode == kGraphModeTrainReturnGrad) {
    // Construct forward-backward graph
    if (loss_layer_) {
      loss_layer_->Init(device, cmd_queue, config, weights);
      loss_layer_->labels_node.handle = [TCMPSGraphNodeHandle handleWithLabel:@"labels"];
      loss_layer_->InitFwd(src);
      src = loss_layer_->fwd_img_node;
      loss_layer_->InitBwd(src);
      src = loss_layer_->bwd_img_node;
    } else {
      grad_node = [MPSNNImageNode nodeWithHandle:[TCMPSGraphNodeHandle handleWithLabel:@"grad"]];
      src = grad_node;
    }
    if (layers.size() > 0) {
      for (int i = (int)layers.size() - 1; i >= 0; --i) {
#if VERBOSE
        NSLog(@"i = %d, src = %p", i, src);
#endif
        layers[i]->InitBwd(src);
        src = layers[i]->bwd_img_node;
      }
    }
    graph = [[MPSNNGraph alloc] initWithDevice:device
                                   resultImage:layers[0]->bwd_img_node
                           resultImageIsNeeded:(mode == kGraphModeTrainReturnGrad)];
  } else {
    // Construct pure inference graph
    graph = [[MPSNNGraph alloc] initWithDevice:device
                                   resultImage:layers[layers.size() - 1]->fwd_img_node
                           resultImageIsNeeded:YES];
  }

#if VERBOSE
  NSLog(@"%@", [graph debugDescription]);
#endif
}

MPSImageBatch *MPSGraphNetwork::RunGraph(id<MTLCommandBuffer> cb,
                                    NSDictionary *inputs) {
  NSArray *input_to_graph = @[];
  for (size_t i = 0; i < graph.sourceImageHandles.count; ++i) {
    // check keys
    input_to_graph = [input_to_graph
        arrayByAddingObject:[inputs objectForKey:[graph.sourceImageHandles[i]
                                                     label]]];
  }
  MPSImageBatch *ret = [graph encodeBatchToCommandBuffer:cb
                                            sourceImages:input_to_graph
                                            sourceStates:nil
                                      intermediateImages:nil
                                       destinationStates:nil];

  return ret;
}

MPSImageBatch *MPSGraphNetwork::RunGraph(id<MTLCommandBuffer> cb, MPSImageBatch *src,
                                         MPSCNNLossLabelsBatch *loss_state) {
  MPSImageBatch *ret =
      [graph encodeBatchToCommandBuffer:cb
                           sourceImages:@[ src ]
                           sourceStates:@[ loss_state ]
                     intermediateImages:nil
                      destinationStates:nil]; // need convGradientStates maybe
  return ret;
}

float_array_map MPSGraphNetwork::Export() const {
  float_array_map table;
  for (size_t i = 0; i < layers.size(); ++i) {
    float_array_map layer_table = layers[i]->Export();
    table.insert(layer_table.begin(), layer_table.end());
    // TODO: In C++17, we can use std::map::merge to move the table entries
    // instead of copying them: table.merge(layers[i]->Export());
  }
  return table;
}

}  // namespace neural_net
}  // namespace turi

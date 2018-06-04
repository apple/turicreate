#ifndef MPS_GRAPH_MODULE_H_
#define MPS_GRAPH_MODULE_H_

#include <assert.h>
#import <vector>

#import <dispatch/dispatch.h>
#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#import "mps_dev.h"
#import "mps_utils.h"
#import "mps_graph_networks.h"


class MPSGraphModule {
public:
  MPSGraphModule();
  ~MPSGraphModule();
  void Init(int network_id, int n, int c_in, int h_in, int w_in, int c_out,
            int h_out, int w_out,
            const FloatArrayMap &config,
            const FloatArrayMap &weights);
  void SetLearningRate(float lr);
  void SetInput(void * _Nonnull ptr, int64_t sz, int64_t * _Nonnull shape, int dim, int flag);
  void SetLossState(float *ptr);
  void RunGraph(float *out = nullptr, float *loss = nullptr);
  void WaitUntilCompleted();
  void Export();
  int NumParams();

  std::unordered_map<std::string,
                     std::tuple<std::string, float *, int, std::vector<int>>>
      table_;

private:
  void SetupNetwork(int network_id, const std::vector<int> &params);

private:
  id<MTLDevice> _Nonnull dev_;
  id<MTLCommandQueue> _Nonnull cmd_queue_;
  id<MTLCommandBuffer> _Nullable last_cmd_buf_ = nil;
  dispatch_semaphore_t _Nonnull double_buffering_semaphore_;
  MPSImageBatch *_Nonnull input_;
  MPSImageBatch *_Nonnull output_;
  MPSImageBatch *_Nonnull grad_;
  MPSCNNLossLabelsBatch * _Nonnull loss_state_ = nil;
  MPSGraphNetwork *_Nonnull network_;
  GraphMode mode_;

private:
  void Blob2MPSImage(float *_Nonnull ptr, MPSImageBatch *_Nonnull batch);
  void MPSImage2Blob(float *_Nonnull ptr, MPSImageBatch *_Nonnull batch);
};

#endif

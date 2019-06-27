#ifndef MPS_UPDATER_H_
#define MPS_UPDATER_H_

// #import "mps_layers.h"
#import "mps_layers.h"
#import "vector"
#import <Foundation/Foundation.h>

namespace turi {
namespace neural_net {

struct Layer;

struct MPSUpdater {
  virtual void Init(const std::vector<Layer *> &net,
                    const std::vector<float> &fparam) = 0;
  virtual void Update(float *weight, float *grad, int size, int lid,
                      int oid) = 0;
  virtual void NewIteration(){}
  virtual void MovingAvg(float *src, float *val, int size, float mom) {
    for (int i = 0; i < size; ++i) {
      src[i] = mom * src[i] + (1.0f - mom) * val[i];
    }
  }
  virtual void SetLearningRate(float new_lr){};
  virtual ~MPSUpdater() = default;
};

struct SGDUpdater : public MPSUpdater {
  float lr{1e-3};
  void Init(const std::vector<Layer *> &net,
            const std::vector<float> &fparam) override;
  void Update(float *weight, float *grad, int size, int lid, int oid) override;
  void SetLearningRate(float new_lr) override {
    lr = new_lr;
  }
  std::vector<std::vector<std::vector<float>>> state;
};

struct MomUpdater : public SGDUpdater {
  float lr{1e-3};
  float clip{0.025};
  float momentum{0.9};
  float wd{5e-4};
  void Init(const std::vector<Layer *> &net,
            const std::vector<float> &fparam) override;
  void Update(float *weight, float *grad, int size, int lid, int oid) override;
};

struct AdamUpdater : public SGDUpdater {
    float lr{1e-3};
    float beta1{0.9f};
    float beta2{0.999f};
    float epsilon{1e-8f};
    float t{0.0f};

    void Init(const std::vector<Layer *> &net,
              const std::vector<float> &fparam) override;
    void Update(float *weight, float *grad, int size, int lid, int oid) override;
    void NewIteration() override;
};
MPSUpdater *createUpdater(int updater_id);

}  // namespace neural_net
}  // namespace turi

#endif

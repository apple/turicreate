#import "mps_updater.h"

namespace turi {
namespace neural_net {

void SGDUpdater::Init(const std::vector<Layer *> &net,
                      const std::vector<float> &fparam) {
  assert(fparam.size() == 1);
  lr = fparam[0];
}

void SGDUpdater::Update(float *weight, float *grad, int size, int lid,
                        int oid) {
  for (int i = 0; i < size; ++i) {
    weight[i] -= lr * grad[i];
  }
}

void MomUpdater::Init(const std::vector<Layer *> &net,
                      const std::vector<float> &fparam) {
  // set params
  state.resize(net.size());
  for (size_t i = 0; i < net.size(); ++i) {
    if (net[i]->type == kConv || net[i]->type == kBN) {
      state[i].resize(2);
    }
  }
}

void MomUpdater::Update(float *weight, float *grad, int size, int lid,
                        int oid) {
  if (state[lid][oid].size() == 0) {
    state[lid][oid].resize(size);
    for (int i = 0; i < size; ++i) {
      state[lid][oid][i] = 0.f;
    }
  }
  auto &v = state[lid][oid];
  for (int i = 0; i < size; ++i) {
    float g = grad[i] > clip ? clip : grad[i];
    g = g < -clip ? -clip : g;
    v[i] = momentum * v[i] - lr * g;
    weight[i] += v[i] - wd * weight[i];
  }
}

MPSUpdater *createUpdater(int updater_id) {
  switch (updater_id) {
  case 0:
    return new SGDUpdater();
  case 1:
    return new MomUpdater();
  case 2:
    return new AdamUpdater();
  default:
    throw std::invalid_argument("Undefined updater id.");
  }
}

void AdamUpdater::Init(const std::vector<Layer *> &net, const std::vector<float> &fparam) {
    state.resize(net.size());
    for (size_t i = 0; i < net.size(); ++i) {
        if (net[i]->type == kConv || net[i]->type == kBN) {
            state[i].resize(4);
        }
    }
}

void AdamUpdater::Update(float *weight, float *grad, int size, int lid, int oid) {
    for (int state_type = 0; state_type < 2; state_type++){
        int state_id = oid + 2 * state_type;
        if (state[lid][state_id].size() == 0) {
            state[lid][state_id].resize(size);
            for (int i = 0; i < size; ++i) {
                state[lid][state_id][i] = 0.f;
            }
        };
    }

    float lr_t = lr * sqrtf(1.0f - powf(beta2, t)) / (1.f - powf(beta1, t));

    auto &m = state[lid][oid];
    auto &v = state[lid][oid + 2];
    for (int i = 0; i < size; ++i) {
        m[i] = (beta1 * m[i]) + ((1 - beta1) * grad[i]);
        v[i] = (beta2 * v[i]) + ((1 - beta2) * grad[i] * grad[i]);
        weight[i] -= lr_t * (m[i] / (sqrt(v[i]) + epsilon) );
    }
}

void AdamUpdater::NewIteration(){
    t++;
}

}  // namespace neural_net
}  // namespace turi

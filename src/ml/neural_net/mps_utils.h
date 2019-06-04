/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef mps_utils_h
#define mps_utils_h

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#include <string>
#include <vector>

#include <unity/toolkits/neural_net/float_array.hpp>

namespace turi {
namespace neural_net {

//
// Optimizer options
//
struct OptimizerOptions {
  OptimizerOptions() : // Default values:
      useSGD(false),
      learningRate(1e-3f),
      gradientClipping(0.0f),
      weightDecay(0.0f),
      sgdMomentum(0.9f),
      adamBeta1(0.9f),
      adamBeta2(0.999f),
      adamEpsilon(1e-8f) {}

  bool useSGD;
  float learningRate;
  float gradientClipping;
  float weightDecay;
  
  // sgd
  float sgdMomentum;
  
  // adam
  float adamBeta1;
  float adamBeta2;
  float adamEpsilon;

  API_AVAILABLE(macos(10.14))
  MPSNNOptimizerDescriptor * _Nonnull mpsDescriptor() {
    
    MPSNNRegularizationType regType;
    if (weightDecay == 0.f) {
      regType = MPSNNRegularizationTypeNone;
    } else {
      regType = MPSNNRegularizationTypeL2;
    }
    
    MPSNNOptimizerDescriptor *desc = [MPSNNOptimizerDescriptor
                                      optimizerDescriptorWithLearningRate:learningRate
                                      gradientRescale:1.0f
                                      applyGradientClipping:(gradientClipping > 0.f)
                                      gradientClipMax:gradientClipping
                                      gradientClipMin:-gradientClipping
                                      regularizationType:regType
                                      regularizationScale:weightDecay];
    return desc;
  }
};

API_AVAILABLE(macos(10.14))
shared_float_array copy_image_batch_float16(std::vector<size_t> shape,
                                        MPSImageBatch * _Nonnull batch);

API_AVAILABLE(macos(10.14))
shared_float_array copy_image_batch(std::vector<size_t> shape,
                                    MPSImageBatch * _Nonnull batch);

API_AVAILABLE(macos(10.14))
void fill_image_batch(const float_array& data, MPSImageBatch * _Nonnull batch);

void convert_chw_to_hwc(const float_array& image, float* out_first,
                        float* out_last);
void convert_hwc_to_chw(const float_array& image, float* out_first,
                        float* out_last);

// This version assumes that each void* is actually a float_array*. This casting
// from plain C should go away once we remove the original Python frontend.
float_array_map make_array_map(char **names, void **arrays, int len);

float get_array_map_scalar(const float_array_map &config,
                           const std::string &key, float default_value);
bool get_array_map_bool(const float_array_map &config, const std::string &key,
                        bool default_value);
OptimizerOptions get_array_map_optimizer_options(const float_array_map &config);

// Intended to be wrapped as a Python-style iterator.
class float_array_map_iterator {
public:
  using value_type = float_array_map::value_type;

  float_array_map_iterator(float_array_map array_map)
    : array_map_(std::move(array_map)), iter_(array_map_.begin())
  {}

  bool has_next() const { return iter_ != array_map_.end(); }

  const value_type& next() { return *iter_++; }

private:
  float_array_map array_map_;
  float_array_map::const_iterator iter_;
};

// Graph mode

enum GraphMode {
  kGraphModeTrain = 0,
  kGraphModeTrainReturnGrad = 1,
  kGraphModeInference = 2,
};

/* Low Level Training Mode
 * Sets the network mode for the low-level API networks:
 * Training   - Layers are configured to training mode, calculates loss and gradients
 * Inference  - Layers are in inference mode (e.g. BN - use running mean). Output is Softmax probabilities
 * Validation - Layers are in inference mode, but loss is still calculated.
 * Test       - Similar to training, but without DropOut layers - to allow comparison testing against other implementations.
 *
 */
enum LowLevelMode {
    kLowLevelModeTrain = 0,
    kLowLevelModeInference,
    kLowLevelModeTest
};

//
// Functions on MPS data structures
//

// Sum image along all dimensions
API_AVAILABLE(macos(10.14))
float sumImage(MPSImage * _Nonnull image);

// The API_AVAILABLE macro does not play well with C++ function templates.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability-new"

template<typename T>
float sumSingleImage(MPSImage * _Nonnull image){
  NSUInteger numActualValues = image.height * image.width * image.featureChannels;
  
  std::vector<T> valsVec(numActualValues);
  T *vals = valsVec.data();
  
  float sum = 0.f;
  [image readBytes:vals dataLayout:(MPSDataLayoutFeatureChannelsxHeightxWidth) imageIndex:0];
  for(NSUInteger i = 0; i < numActualValues; i++){
    sum += vals[i];
  }
  return sum;
}

#pragma clang diagnostic pop

}  // namespace neural_net
}  // namespace turi

#endif /* mps_utils_h */

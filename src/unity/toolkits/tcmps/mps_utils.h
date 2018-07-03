//
//  mps_utils.h
//  tcmps
//
//  Created by Gustav Larsson on 5/9/18.
//  Copyright © 2018 Turi. All rights reserved.
//

#ifndef mps_utils_h
#define mps_utils_h

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#include <vector>
#include <string>
#include <unordered_map>

namespace turi {
namespace mps {

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

//
// FloatArrayMap is a (str -> float array pointer) dictionary is used to
// pass weights and config into the library
//
struct FloatArray {
  size_t size;
  float * _Nullable data;
};

typedef std::unordered_map<std::string, FloatArray> FloatArrayMap;

FloatArrayMap make_array_map(char **names, void **arrays,
                             int64_t *sizes, int len);

float get_array_map_scalar(const FloatArrayMap &config, const std::string &key, float default_value);
BOOL get_array_map_bool(const FloatArrayMap &config, const std::string &key, BOOL default_value);
OptimizerOptions get_array_map_optimizer_options(const FloatArrayMap &config);

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

}  // namespace mps
}  // namespace turi

#endif /* mps_utils_h */

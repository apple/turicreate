/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifdef HAS_MACOS_10_15

#import <ml/neural_net/style_transfer/mps_style_transfer_backend.hpp>
#import <ml/neural_net/style_transfer/mps_style_transfer.h>

#import <ml/neural_net/model_spec.hpp>
#import <ml/neural_net/mps_device_manager.h>
#import <ml/neural_net/mps_utils.h>

#include <numeric>

using namespace turi::neural_net;

@interface TCMPSStyleTransferHelpers:NSObject
+(float_array_map) fromNSDictionary: (NSDictionary<NSString *, NSData *> *) dictionary;
+(NSDictionary<NSString *, NSData *> *) toNSDictionary: (float_array_map) map;
@end

@implementation TCMPSStyleTransferHelpers
+(float_array_map) fromNSDictionary: (NSDictionary<NSString *, NSData *> *) dictionary {
  float_array_map map;

  NSMutableDictionary<NSString *, NSData *> *dictionaryMutable = [dictionary mutableCopy];

  NSData* widthData = dictionaryMutable[@"width"];
  NSData* heightData = dictionaryMutable[@"height"];
  NSData* channelsData = dictionaryMutable[@"channels"];
  NSData* batchSizeData = dictionaryMutable[@"batch_size"];

  std::vector<size_t> imageShape;

  if (widthData != nil && heightData != nil && channelsData != nil) {
    size_t width = static_cast<size_t>(((float *) (widthData.bytes)) [0]);
    size_t height = static_cast<size_t>(((float *) (heightData.bytes)) [0]);
    size_t channels = static_cast<size_t>(((float *) (channelsData.bytes)) [0]);
    size_t batchSize = static_cast<size_t>(((float *) (batchSizeData.bytes)) [0]);

    imageShape = { batchSize, height, width, channels };
  }

  [dictionaryMutable removeObjectForKey:@"width"];
  [dictionaryMutable removeObjectForKey:@"height"];
  [dictionaryMutable removeObjectForKey:@"channels"];
  [dictionaryMutable removeObjectForKey:@"batch_size"];

  for (NSString* key in dictionaryMutable) {
    NSData* value = [dictionaryMutable objectForKey:key];

    size_t dataLength = (size_t) (value.length / sizeof(float));
    std::vector<size_t> dataShape = { dataLength }; 

    if (imageShape.size() > 0) {
      size_t imageSize = std::accumulate(imageShape.begin(),
                                         imageShape.end(), 1,
                                         std::multiplies<size_t>());
      ASSERT_EQ(dataLength, imageSize);
      dataShape = imageShape;
    }
    
    float *dataBytes = (float *) (value.bytes);
    
    // TODO: This copy is inefficient. This should be a wrapper around NSData: 
    //       a custom subclass of float_array that preserves a strong reference
    //       to the NSData instance.

    shared_float_array array = shared_float_array::copy(dataBytes, dataShape);
    
    map.emplace(key.UTF8String, array);
  }
  return map;
}

+(NSDictionary<NSString *, NSData *> *) toNSDictionary: (float_array_map) map {
  NSMutableDictionary<NSString *, NSData *> * dictionary 
      = [[NSMutableDictionary alloc] init];

  for (const auto &element : map) {
    const float* elementData = element.second.data();
    const size_t elementSize = element.second.size() * sizeof(float);

    // TODO: This copy is inefficient. We can construct an NSData that wraps a 
    //       shared_float_array. 
    // 
    // API: https://developer.apple.com/documentation/foundation/nsdata/1417337-initwithbytesnocopy?language=objc

    NSData *data = [NSData dataWithBytes:elementData length:elementSize];

    NSString* elementKey = [NSString stringWithUTF8String:element.first.c_str()];
    dictionary[elementKey] = data;
  }

  return [dictionary copy];
}

@end

namespace {

float_array_map convert_weights_coreml_mps(const float_array_map &coreml_weights) {
  float_array_map mps_weights;
  for (auto const& w : coreml_weights) {
    if (w.first.find("conv") != std::string::npos) {
      std::vector<float> init_w;
      init_w.resize(w.second.size());
      convert_chw_to_hwc(w.second, init_w.data(), init_w.data() + w.second.size());
      ASSERT_EQ(w.second.dim(), 4);

      std::vector<size_t> w_shape;
      w_shape.resize(w.second.dim());
      const size_t* w_ptr = w.second.shape();

      // nchw to nhwc
      w_shape[0] = w_ptr[0];
      w_shape[1] = w_ptr[2];
      w_shape[2] = w_ptr[3];
      w_shape[3] = w_ptr[1];
      
      mps_weights[w.first] = shared_float_array::wrap(std::move(init_w),
                                                      std::move(w_shape));
    } else {
      mps_weights.insert(w);
    }
  }
  return mps_weights;
}

float_array_map convert_weights_mps_coreml(const float_array_map &mps_weights) {
  float_array_map coreml_weights;
  for (auto const& w : mps_weights) {
    if (w.first.find("conv") != std::string::npos) {
      std::vector<float> init_w;
      init_w.resize(w.second.size());
      convert_hwc_to_chw(w.second, init_w.data(), init_w.data() + w.second.size());
      ASSERT_EQ(w.second.dim(), 4);

      std::vector<size_t> w_shape;
      w_shape.resize(w.second.dim());
      const size_t* w_ptr = w.second.shape();

      // nhwc to nchw
      w_shape[0] = w_ptr[0];
      w_shape[1] = w_ptr[3];
      w_shape[2] = w_ptr[1];
      w_shape[3] = w_ptr[2];
      
      coreml_weights[w.first] = shared_float_array::wrap(std::move(init_w),
                                                      std::move(w_shape));
    } else {
      coreml_weights.insert(w);
    }
  }
  return coreml_weights;
}


} // namespace

namespace turi {
namespace style_transfer {

struct mps_style_transfer::impl {
  API_AVAILABLE(macos(10.15)) TCMPSStyleTransfer *model = nil;
};

mps_style_transfer::mps_style_transfer(
    const float_array_map &config,
    const float_array_map &weights) 
  : m_impl(new mps_style_transfer::impl())  {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      mps_command_queue queue;

      id <MTLDevice> dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
      queue.impl = [dev newCommandQueue];
      
      init(config, weights, queue);
    } else {
      log_and_throw("Can't construct GPU Style Transfer Network for MacOS \
                     platform lower than 10.15");
    }
  }
}

mps_style_transfer::mps_style_transfer(
    const float_array_map &config,
    const float_array_map &weights,
    const mps_command_queue& command_queue) 
  : m_impl(new mps_style_transfer::impl()) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      init(config, weights, command_queue);
    } else {
      log_and_throw("Can't construct GPU Style Transfer Network for MacOS \
                     platform lower than 10.15");
    }
  }
}

void mps_style_transfer::init(
    const float_array_map &config,
    const float_array_map &weights,
    const mps_command_queue& command_queue) {
  @autoreleasepool {
    if (@available(macOS 10.15, *)) {
      NSUInteger numStyles
          = (NSUInteger) get_array_map_scalar(config, "st_num_styles", 1);

      float_array_map transformed_weights = convert_weights_coreml_mps(weights);

      NSDictionary<NSString *, NSData *> *styleTransferWeights
          = [TCMPSStyleTransferHelpers toNSDictionary: transformed_weights];

      m_impl->model = [[TCMPSStyleTransfer alloc] initWithDev:command_queue.impl.device
                                                 commandQueue:command_queue.impl
                                                      weights:styleTransferWeights
                                                    numStyles:numStyles];
    } else {
      log_and_throw("Can't construct GPU Style Transfer Network for MacOS \
                     platform lower than 10.15");
    }
  }
}

mps_style_transfer::~mps_style_transfer() = default;

float_array_map mps_style_transfer::export_weights() const {
  if (@available(macOS 10.15, *)) {
    NSDictionary<NSString *, NSData *> *dictWeights
        = [m_impl->model exportWeights];
    
    float_array_map weights
        = [TCMPSStyleTransferHelpers fromNSDictionary:dictWeights];

    return convert_weights_mps_coreml(weights);
  } else {
    log_and_throw("Can't export weights on the GPU Style Transfer Network for \
                   MacOS platform lower than 10.15");
  }
}

float_array_map mps_style_transfer::predict(const float_array_map& inputs) const {
  if (@available(macOS 10.15, *)) {
    NSDictionary<NSString *, NSData *> *dictInputs
        = [TCMPSStyleTransferHelpers toNSDictionary: inputs];

    NSDictionary<NSString *, NSData *> *dictOutput
        = [m_impl->model predict:dictInputs];

    float_array_map output
        = [TCMPSStyleTransferHelpers fromNSDictionary:dictOutput];

    return output;
  } else {
    log_and_throw("Can't call predict on GPU Style Transfer Network for MacOS \
                     platform lower than 10.15");
  }
}

void mps_style_transfer::set_learning_rate(float lr) {
  if (@available(macOS 10.15, *)) {
    [m_impl->model setLearningRate:lr];
  } else {
    log_and_throw("Can't call set_learning_rate on GPU Style Transfer Network \
                   for MacOS platform lower than 10.15");
  }
}

float_array_map mps_style_transfer::train(const float_array_map& inputs) {
  if (@available(macOS 10.15, *)) {
    NSDictionary<NSString *, NSData *> *dictInputs
        = [TCMPSStyleTransferHelpers toNSDictionary: inputs];

    NSDictionary<NSString *, NSData *> *dictLoss
        = [m_impl->model train: dictInputs];

    float_array_map loss
        = [TCMPSStyleTransferHelpers fromNSDictionary:dictLoss];

    return loss;
  } else {
    log_and_throw("Can't call train on GPU Style Transfer Network for MacOS \
                     platform lower than 10.15");
  }
}

} // namespace style_transfer
} // namespace turi

#endif // #ifdef HAS_MACOS_10_15
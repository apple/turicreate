//
//  mps_utils.mm
//  tcmps
//
//  Created by Gustav Larsson on 5/9/18.
//  Copyright © 2018 Turi. All rights reserved.
//

#import "mps_utils.h"

#include <sys/stat.h>
#include <memory>

namespace turi {
namespace neural_net {

shared_float_array copy_image_batch_float16(std::vector<size_t> shape,
                                            MPSImageBatch *batch) {
  assert(shape.size() == 4);  // NHWC

  const size_t n = shape[0];  // N
  const size_t stride = shape[1] * shape[2] * shape[3];  // H * W * C
  const size_t size = n * stride;

  // Copy from MPS to a local __fp16 buffer.
  assert(batch.count >= n);
  std::unique_ptr<__fp16[]> fp16_buffer(new __fp16[size]);
  for (size_t i = 0; i < n; ++i) {
    MPSImage *img = batch[i];

    assert(img.height == shape[1]);
    assert(img.width == shape[2]);
    assert(img.featureChannels == shape[3]);
    assert(img.pixelFormat == MTLPixelFormatRGBA16Float);

    [img readBytes:fp16_buffer.get() + stride * i
        dataLayout:MPSDataLayoutHeightxWidthxFeatureChannels
        imageIndex:0];
  }

  // Convert from __fp16 to float.
  std::vector<float> float_buffer(fp16_buffer.get(), fp16_buffer.get() + size);

  return shared_float_array::wrap(std::move(float_buffer), std::move(shape)); 
}

void fill_image_batch(const float_array& blob, MPSImageBatch *batch) {
  assert(blob.dim() == 4);  // NHWC

  const float* data = blob.data();
  const size_t* shape = blob.shape();
  const size_t stride = shape[1] * shape[2] * shape[3];  // H * W * C

  assert(batch.count <= shape[0]);
  for (MPSImage *img in batch) {
    assert(img.height == shape[1]);
    assert(img.width == shape[2]);
    assert(img.featureChannels == shape[3]);
    assert(img.pixelFormat == MTLPixelFormatRGBA32Float);

    [img writeBytes:data
         dataLayout:MPSDataLayoutHeightxWidthxFeatureChannels
         imageIndex:0];

    data += stride;
  }
}

float_array_map make_array_map(char **names, void **arrays,
                               int64_t *sizes, int len) {
  float_array_map ret;
  for (int i = 0; i < len; ++i) {
    std::string name = names[i];
    const float* array = reinterpret_cast<const float*>(arrays[i]);
    const size_t size = static_cast<size_t>(sizes[i]);
    ret[name] = shared_float_array::copy(array, {size});
  }
  return ret;
}

float get_array_map_scalar(const float_array_map &config,
                           const std::string &key, float default_value) {
  float value = default_value;

  if (config.count(key) > 0) {
 
    const shared_float_array& arr = config.at(key);
    if (arr.size() == 1) {
      value = arr.data()[0];
    } else {
      // TODO: raise exception
    }
  }

  return value;
}

bool get_array_map_bool(const float_array_map &config, const std::string &key,
                        bool default_value) {
  // 0 == False, the rest will be True (or the default if key is not present)
  bool value = default_value;
  if (config.count(key) > 0) {
    const shared_float_array& arr = config.at(key);
    if (arr.size() == 1) {
      value = arr.data()[0] != 0;
    } else {
      // TODO: raise exception
    }
  }
  return value;
}

OptimizerOptions
get_array_map_optimizer_options(const float_array_map &config) {
  OptimizerOptions opt;
  opt.useSGD = get_array_map_bool(config,  "use_sgd", opt.useSGD);
  opt.learningRate = get_array_map_scalar(config, "learning_rate", opt.learningRate);
  opt.gradientClipping = get_array_map_scalar(config, "gradient_clipping", opt.gradientClipping);
  opt.weightDecay = get_array_map_scalar(config, "weight_decay", opt.weightDecay);
  opt.sgdMomentum = get_array_map_scalar(config, "sgd_momentum", opt.sgdMomentum);
  opt.adamBeta1 = get_array_map_scalar(config, "adam_beta1", opt.adamBeta1);
  opt.adamBeta2 = get_array_map_scalar(config, "adam_beta2", opt.adamBeta2);
  opt.adamEpsilon = get_array_map_scalar(config, "adam_epsilon", opt.adamEpsilon);
  return opt;
}

float sumImage(MPSImage * _Nonnull image) {
  
  if(image.pixelFormat == MTLPixelFormatR16Float || image.pixelFormat == MTLPixelFormatRG16Float || image.pixelFormat == MTLPixelFormatRGBA16Float){
    return sumSingleImage<__fp16>(image);
  } else if(image.pixelFormat == MTLPixelFormatR32Float || image.pixelFormat == MTLPixelFormatRG32Float || image.pixelFormat == MTLPixelFormatRGBA32Float){
    return sumSingleImage<float>(image);
  } else if(image.pixelFormat == MTLPixelFormatR8Unorm|| image.pixelFormat == MTLPixelFormatRG8Unorm || image.pixelFormat == MTLPixelFormatRGBA8Unorm || image.pixelFormat == MTLPixelFormatBGRA8Unorm){
    return sumSingleImage<uint8_t>(image);
  } else {
    printf("Unrecognized pixel format\n");
    __builtin_trap();
  }
}

}  // namespace neural_net
}  // namespace turi

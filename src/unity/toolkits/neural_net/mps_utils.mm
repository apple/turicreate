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

#import <Accelerate/Accelerate.h>

namespace turi {
namespace neural_net {

// Converts from CHW to HWC
void convert_chw_to_hwc(const float_array& image, float* out_first,
                        float* out_last) {
  assert(image.dim() >= 3);
  assert(out_last - out_first == image.size());

  if (image.dim() == 3) {
    // Use Accelerate framework, interpreting each channel as an image plane.

    const unsigned c = static_cast<unsigned>(image.shape()[0]);
    const vImagePixelCount h = static_cast<vImagePixelCount>(image.shape()[1]);
    const vImagePixelCount w = static_cast<vImagePixelCount>(image.shape()[2]);

    std::vector<vImage_Buffer> image_planes(c);
    std::vector<const vImage_Buffer*> image_plane_ptrs(c);
    std::vector<void*> output_channels(c);
    for (size_t i = 0; i < c; ++i) {
      // Wrap the relevant portion of the float_array data as vImage_Buffer.
      image_planes[i].data = const_cast<float*>(image.data() + i * h * w);
      image_planes[i].height = h;
      image_planes[i].width = w;
      image_planes[i].rowBytes = w * sizeof(float);

      // The Accelerate API wants an array of pointers to vImage_Buffer...
      image_plane_ptrs[i] = &image_planes[i];

      // Tell the Accelerate API to write the data into [out_first, out_last).
      // By setting destStrideBytes and destRowBytes below, the planar data will
      // be interleaved into HWC format.
      output_channels[i] = out_first + i;
    }
    vImage_Error status;
    status = vImageConvert_PlanarToChunkyF(
        /* srcPlanarBuffers */ image_plane_ptrs.data(),
        /* destChannels */     output_channels.data(),
        /* channelCount */     c,
        /* destStrideBytes */  c * sizeof(float),
        /* destWidth */        w,
        /* destHeight */       h,
        /* destRowBytes */     c * w * sizeof(float),
        /* flags */            kvImageNoFlags);
    assert(status == kvImageNoError);
  } else {
    // Recurse on dimensions
    const size_t* shape = image.shape();
    size_t n = shape[0];
    size_t stride = image.size() / n;
    float* out = out_first;
    for (size_t i = 0; i < n; ++i) {
      external_float_array sub_image(image.data() + i * stride,
                                     stride, shape + 1, image.dim() - 1);
      convert_chw_to_hwc(sub_image, out, out + stride);
      out += stride;
    }
    assert(out == out_last);
  }
}

// Converts from HWC to CHW
void convert_hwc_to_chw(const float_array& image, float* out_first,
                        float* out_last) {
  assert(image.dim() >= 3);
  assert(out_last - out_first == image.size());

  if (image.dim() == 3) {
    // Use Accelerate framework, writing each channel to its own image plane.

    const vImagePixelCount h = static_cast<vImagePixelCount>(image.shape()[0]);
    const vImagePixelCount w = static_cast<vImagePixelCount>(image.shape()[1]);
    const unsigned c = static_cast<unsigned>(image.shape()[2]);

    std::vector<const void*> input_channels(c);
    std::vector<vImage_Buffer> output_planes(c);
    std::vector<const vImage_Buffer*> output_plane_ptrs(c);
    for (size_t i = 0; i < c; ++i) {
      // Tell the Accelerate API to read each channel from the input image by
      // striding from the first value for each channel.
      input_channels[i] = image.data() + i;

      // Wrap the relevant portion of the output array as vImage_Buffer.
      output_planes[i].data = out_first + i * h * w;
      output_planes[i].height = h;
      output_planes[i].width = w;
      output_planes[i].rowBytes = w * sizeof(float);

      // The Accelerate API wants an array of pointers to vImage_Buffer...
      output_plane_ptrs[i] = &output_planes[i];
    }
    vImage_Error status;
    status = vImageConvert_ChunkyToPlanarF(
        /* srcChannels */       input_channels.data(),
        /* destPlanarBuffers */ output_plane_ptrs.data(),
        /* channelCount */      c,
        /* srcStrideBytes */    c * sizeof(float),
        /* srcWidth */          w,
        /* srcHeight */         h,
        /* srcRowBytes */       c * w * sizeof(float),
        /* flags */             kvImageNoFlags);
    assert(status == kvImageNoError);
  } else {
    // Recurse on dimensions
    const size_t* shape = image.shape();
    size_t n = shape[0];
    size_t stride = image.size() / n;
    float* out = out_first;
    for (size_t i = 0; i < n; ++i) {
      external_float_array sub_image(image.data() + i * stride,
                                     stride, shape + 1, image.dim() - 1);
      convert_hwc_to_chw(sub_image, out, out + stride);
      out += stride;
    }
    assert(out == out_last);
  }
}

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

// This version assumes that each void* is actually a float_array*. This casting
// from plain C should go away once we remove the original Python frontend.
float_array_map make_array_map(char **names, void **arrays, int len) {
  float_array_map ret;
  for (int i = 0; i < len; ++i) {
    std::string name = names[i];
    const float_array* array = reinterpret_cast<const float_array*>(arrays[i]);

    // Note that we assume that the provided float arrays will outlive the map.
    ret[name] = shared_float_array(
        std::make_shared<external_float_array>(*array));
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

/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#include <ml/neural_net/mlc_layer_weights.hpp>

#include <core/util/Verify.hpp>

NS_ASSUME_NONNULL_BEGIN

namespace turi {
namespace neural_net {

namespace {

NSData *WrapFloatArray(const shared_float_array &arr)
{
  // Create a strong reference to the shared memory, using __block to allow the deallocator below to
  // mutate (release) it.
  __block shared_float_array shared_arr = arr;
  auto deallocator = ^(void *bytes, NSUInteger length) {
    if (bytes == shared_arr.data()) {
      shared_arr = shared_float_array();  // Reset shared_arr to release the strong reference.
    }
  };

  NSUInteger length = shared_arr.size() * sizeof(float);
  return [[NSData alloc] initWithBytesNoCopy:(float *)shared_arr.data()
                                      length:length
                                 deallocator:deallocator];
}

}  // namespace

mlc_layer_weights::mlc_layer_weights()
{
  @autoreleasepool {
    tensors_ = [[NSMutableDictionary alloc] init];
  }
}

mlc_layer_weights::mlc_layer_weights(mlc_layer_weights &&) = default;
mlc_layer_weights &mlc_layer_weights::operator=(mlc_layer_weights &&) = default;
mlc_layer_weights::~mlc_layer_weights() = default;

float_array_map mlc_layer_weights::export_weights() const
{
  float_array_map exported_weights;
  for (const auto &name_and_value : weights_) {
    // Copy the weights instead of just providing a shared reference to them, so that the returned
    // weights remain constant even as MLCompute continues to update the weights we store (after
    // this function returns).
    exported_weights[name_and_value.first] = shared_float_array::copy(name_and_value.second);
  }
  return exported_weights;
}

float_array_map mlc_layer_weights::export_weights_and_optimizer_data() const
{
  @autoreleasepool {
    __block float_array_map exported_weights = export_weights();
    [tensors_ enumerateKeysAndObjectsUsingBlock:^(NSString *_Nonnull key,
                                                  MLCTensor *_Nonnull tensor, BOOL *_Nonnull stop) {
      NSString *optimizerDataKey = [NSString stringWithFormat:@"%@%@", key, @"_optimizer_data"];
      [tensor.optimizerData enumerateObjectsUsingBlock:^(MLCTensorData *_Nonnull tensorData,
                                                         NSUInteger idx, BOOL *_Nonnull stop) {
        std::string optimizer_key =
            std::string(optimizerDataKey.UTF8String) + "_" + std::to_string(idx + 1);
        const float *data_ptr = reinterpret_cast<const float *>(tensorData.bytes);
        size_t data_size = tensorData.length / sizeof(float);
        exported_weights[optimizer_key] = shared_float_array::copy(data_ptr, {data_size});
      }];
    }];

    return exported_weights;
  }
}

NSDictionary<NSString *, NSData *> *mlc_layer_weights::tensor_weights() const
{
  @autoreleasepool {
    NSMutableDictionary<NSString *, NSData *> *result =
        [NSMutableDictionary dictionaryWithCapacity:(NSUInteger)weights_.size()];
    for (const auto &name_and_value : weights_) {
      NSString *name = [NSString stringWithUTF8String:name_and_value.first.c_str()];
      result[name] = WrapFloatArray(name_and_value.second);
    }
    return result;
  }
}

MLCTensorData *mlc_layer_weights::copy_float_array(const std::string &name, const float_array &arr)
{
  shared_float_array &arr_copy = weights_[name];
  arr_copy = shared_float_array::copy(arr);
  return [MLCTensorData dataWithImmutableBytesNoCopy:arr_copy.data()
                                              length:arr_copy.size() * sizeof(float)];
}

void mlc_layer_weights::add_conv_weight(const std::string &name, const float_array &arr)
{
  @autoreleasepool {
    // Create an appropriate descriptor for this layer.
    VerifyIsTrue(arr.dim() == 4, TuriErrorCode::InvalidDimensionality);
    NSUInteger outputs = arr.shape()[0];
    NSUInteger inputs = arr.shape()[1];
    NSUInteger height = arr.shape()[2];
    NSUInteger width = arr.shape()[3];
    MLCTensorDescriptor *descriptor =
        [MLCTensorDescriptor convolutionWeightsDescriptorWithWidth:width
                                                            height:height
                                          inputFeatureChannelCount:inputs
                                         outputFeatureChannelCount:outputs
                                                          dataType:MLCDataTypeFloat32];

    // Copy and store the initial weights, as well as a tensor that combines the descriptor above
    // with a weak reference to the weights.
    NSString *layerName = [NSString stringWithUTF8String:name.c_str()];
    MLCTensorData *data = copy_float_array(name, arr);
    tensors_[layerName] = [MLCTensor tensorWithDescriptor:descriptor data:data];
  }
}

void mlc_layer_weights::add_conv_bias(const std::string &name, const float_array &arr)
{
  @autoreleasepool {
    // Create an appropriate descriptor for this layer.
    VerifyIsTrue(arr.dim() == 1, TuriErrorCode::InvalidDimensionality);
    NSUInteger outputs = arr.shape()[0];
    MLCTensorDescriptor *descriptor =
        [MLCTensorDescriptor convolutionBiasesDescriptorWithFeatureChannelCount:outputs
                                                                       dataType:MLCDataTypeFloat32];

    // Copy and store the initial weights, as well as a tensor that combines the descriptor above
    // with a weak reference to the weights.
    NSString *layerName = [NSString stringWithUTF8String:name.c_str()];
    MLCTensorData *data = copy_float_array(name, arr);
    tensors_[layerName] = [MLCTensor tensorWithDescriptor:descriptor data:data];
  }
}

void mlc_layer_weights::add_flat_array(const std::string &name, const float_array &arr)
{
  @autoreleasepool {
    // Copy and store the initial weights, as well as a tensor that combines an appropriate
    // descriptor with a weak reference to the weights.
    assert(arr.dim() == 1);
    NSUInteger channels = arr.shape()[0];
    NSString *layerName = [NSString stringWithUTF8String:name.c_str()];
    MLCTensorData *data = copy_float_array(name, arr);
    tensors_[layerName] = [MLCTensor tensorWithWidth:1
                                              height:1
                                 featureChannelCount:channels
                                           batchSize:1
                                                data:data];
  }
}

void mlc_layer_weights::add_lstm_weight(const std::string &name, const float_array &arr)
{
  @autoreleasepool {
    // Copy and store the initial weights, as well as a tensor that combines an appropriate
    // descriptor with a weak reference to the weights.
    assert(arr.dim() == 2);
    NSUInteger output_size = arr.shape()[0] * arr.shape()[1];
    NSString *layerName = [NSString stringWithUTF8String:name.c_str()];
    MLCTensorData *data = copy_float_array(name, arr);
    tensors_[layerName] = [MLCTensor tensorWithWidth:1
                                              height:1
                                 featureChannelCount:output_size
                                           batchSize:1
                                                data:data];
  }
}

// Suppress clang warnings for deprecated API with non-trivial replacement, tracked in
// <rdar://problem/63782884> [MLCompute] Switch to '-[MLCTensor bindOptimizerData:deviceData:]' from
// '-[MLCTensor optimizerData:]'
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

void mlc_layer_weights::add_optimizer_data(const std::string &layer_name,
                                           const std::string &optimizer_data_1_key,
                                           const float_array &optimizer_data_1,
                                           const std::string &optimizer_data_2_key,
                                           const float_array &optimizer_data_2)
{
  @autoreleasepool {
    assert(optimizer_data_1.dim() == 1);
    assert(optimizer_data_2.dim() == 1);
    NSString *layerName = [NSString stringWithUTF8String:layer_name.c_str()];

    MLCTensorData *optimizerData1 = copy_float_array(optimizer_data_1_key, optimizer_data_1);
    MLCTensorData *optimizerData2 = copy_float_array(optimizer_data_2_key, optimizer_data_2);
    if (tensors_[layerName] != nil) {
      BOOL success = [tensors_[layerName] optimizerData:@[ optimizerData1, optimizerData2 ]];
      if (!success) {
        throw("Failed to load optimizer data into tensor");
      }
    }
  }
}

#pragma clang diagnostic pop

}  // namespace neural_net
}  // namespace turi

NS_ASSUME_NONNULL_END

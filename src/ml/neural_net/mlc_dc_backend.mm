/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/mlc_dc_backend.hpp>

#include <algorithm>

#import <ml/neural_net/TCMLComputeUtil.h>
#include <core/logging/logger.hpp>
#include <ml/neural_net/mlc_utils.hpp>

#import <ml/neural_net/TCMLComputeDrawingClassifierDescriptor.h>

namespace turi {
namespace neural_net {

using turi::neural_net::convert_chw_data_to_hwc_array;
using turi::neural_net::deferred_float_array;
using turi::neural_net::external_float_array;
using turi::neural_net::float_array;
using turi::neural_net::float_array_map;
using turi::neural_net::mlc_layer_weights;
using turi::neural_net::shared_float_array;

namespace {

NSData *encodeLabels(const float *labels, NSUInteger sequenceLength, NSUInteger numClasses)
{
  NSUInteger dataLength = sequenceLength * numClasses * sizeof(float);
  NSMutableData *result = [NSMutableData dataWithLength:dataLength];  // Zeroed

  // Create an OHE representation of the label, per sequence
  float *output = (float *)result.mutableBytes;
  for (NSUInteger i = 0; i < sequenceLength; ++i) {
    // Interpret the label as an index to a class
    NSUInteger classIndex = (NSUInteger)labels[i];

    // Set the corresponding float value to 1
    output[classIndex] = 1.f;

    // Advance output pointer to encoding of the next sequence element.
    output += numClasses;
  }

  return result;
}

NSData *encodeWeights(const float *weights, NSUInteger sequenceLength, NSUInteger numClasses)
{
  NSUInteger dataLength = sequenceLength * numClasses * sizeof(float);
  NSMutableData *result = [NSMutableData dataWithLength:dataLength];  // Zeroed
  // Shape of weights [batch_size, 1, prediction_window, 1]
  // Create weights for OHE labels
  float *output = (float *)result.mutableBytes;
  for (NSUInteger i = 0; i < sequenceLength; ++i) {
    // Repeat the weights to be of the same size as OHE labels
    for (NSUInteger j = 0; j < numClasses; j++)
    // Set the corresponding float value to weights
    {
      output[j] = weights[i];
    }

    // Advance output pointer to encoding of the next sequence element.
    output += numClasses;
  }

  return result;
}

API_AVAILABLE(macos(10.16))
mlc_layer_weights import_weights(const float_array_map &weights)
{
  mlc_layer_weights layer_weights;
  for (const auto &kv : weights) {
    NSString *key = [NSString stringWithUTF8String:kv.first.c_str()];

    // If we recognize the layer name, convert it.
    // TODO: Someday we will remove this brittle dependency on names by
    // converting models directly from their Core ML proto to graphs.
    if ([key hasPrefix:@"drawing_conv"]) {
      if ([key hasSuffix:@"_weight"]) {
        layer_weights.add_conv_weight(kv.first, kv.second);
      } else if ([key hasSuffix:@"_bias"]) {
        layer_weights.add_conv_bias(kv.first, kv.second);
      }
    } else if ([key hasPrefix:@"drawing_dense"]) {
      if ([key hasSuffix:@"_weight"]) {
        layer_weights.add_conv_weight(kv.first, kv.second);
      } else if ([key hasSuffix:@"_bias"]) {
        layer_weights.add_conv_bias(kv.first, kv.second);
      }
    }
  }
  return layer_weights;
}

}  // namespace

mlc_drawing_classifier_backend::mlc_drawing_classifier_backend(MLCDevice *device,
                                                               const float_array_map &weights,
                                                               size_t batch_size,
                                                               size_t num_classes)
{
  @autoreleasepool {
    TCMLComputeDrawingClassifierDescriptor *descriptor =
        [[TCMLComputeDrawingClassifierDescriptor alloc] init];

    // Define input and output tensor shape. Note that MLCompute requires the weight tensor to have
    // some (placeholder) data when the loss layer is compiled.
    input_ = [MLCTensor tensorWithWidth:28 height:28 featureChannelCount:1 batchSize:batch_size];
    labels_ = [MLCTensor tensorWithWidth:1
                                  height:1
                     featureChannelCount:num_classes
                               batchSize:batch_size];
    weights_ = [MLCTensor tensorWithWidth:1
                                   height:1
                      featureChannelCount:num_classes
                                batchSize:batch_size
                             fillWithData:0.f
                                 dataType:MLCDataTypeFloat32];
    descriptor.inputTensor = input_;
    descriptor.outputChannels = num_classes;
    num_classes_ = num_classes;

    // Wrap weights.
    // Note: this also records references to the MLCTensor values,
    // created to simplify later export of the updated weights.
    layer_weights_ = import_weights(weights);
    descriptor.weights = layer_weights_.tensor_views();

    // Configure loss.
    MLCLossDescriptor *lossDesc;
    lossDesc = [MLCLossDescriptor descriptorWithType:MLCLossTypeSoftmaxCrossEntropy
                                       reductionType:MLCReductionTypeNone];
    MLCLayer *loss = [MLCLossLayer layerWithDescriptor:lossDesc weights:weights_];

    // Configure optimizer.
    MLCOptimizerDescriptor *optimizerDesc =
        [MLCOptimizerDescriptor descriptorWithLearningRate:1e-3f
                                           gradientRescale:1.f
                                        regularizationType:MLCRegularizationTypeNone
                                       regularizationScale:1.0f];

    MLCOptimizer *optimizer = [MLCAdamOptimizer optimizerWithDescriptor:optimizerDesc
                                                                  beta1:0.9f
                                                                  beta2:0.999f
                                                                epsilon:1e-08f
                                                               timeStep:1];

    // Instantiate training graph.
    MLCGraph *graph = [MLCGraph tc_graphForDrawingClassifierDescriptor:descriptor
                                                             batchSize:batch_size];
    training_graph_ = [MLCTrainingGraph trainingGraphWithGraphObjects:@[ graph ]
                                                            lossLayer:loss
                                                            optimizer:optimizer];

    BOOL success = [training_graph_ addInputs:@{input_.label : input_}
                                   lossLabels:@{labels_.label : labels_}
                             lossLabelWeights:@{weights_.label : weights_}];
    if (!success) {
      log_and_throw("Error adding inputs and labels to training graph");
    }

    success = [training_graph_ compileWithOptions:MLCGraphCompilationOptionsNone device:device];
    if (!success) {
      log_and_throw("Error compiling Drawing Classifier MLCTrainingGraph");
    }

    inference_graph_ = [MLCInferenceGraph graphWithGraphObjects:@[ graph ]];
    success = [inference_graph_ addInputs:@{input_.label : input_}];
    if (!success) {
      log_and_throw("Error adding inputs to inference graph");
    }

    MLCSoftmaxLayer *softmax = [MLCSoftmaxLayer layerWithOperation:MLCSoftmaxOperationSoftmax];
    [inference_graph_ nodeWithLayer:softmax source:descriptor.outputTensor];

    success = [inference_graph_ compileWithOptions:MLCGraphCompilationOptionsNone device:device];
    if (!success) {
      log_and_throw("Error compiling Drawing Classifier MLCInferenceGraph");
    }
  }
}

float_array_map mlc_drawing_classifier_backend::export_weights() const
{
  @autoreleasepool {
    // Copies weights from GPU to the tensors referenced in export_infos_.
    [training_graph_ synchronizeUpdates];

    return layer_weights_.export_weights();
  }
}

void mlc_drawing_classifier_backend::set_learning_rate(float lr)
{
  @autoreleasepool {
    training_graph_.optimizer.learningRate = lr;
  }
}

float_array_map mlc_drawing_classifier_backend::train(const float_array_map &inputs)
{
  @autoreleasepool {
    // Extract and convert the input image and annotations.
    auto input_iter = inputs.find("input");
    auto labels_iter = inputs.find("labels");
    auto weights_iter = inputs.find("weights");

    if (input_iter == inputs.end()) {
      log_and_throw("Cannot train without argument named \"input\".");
    }
    if (labels_iter == inputs.end()) {
      log_and_throw("Cannot train without argument named \"labels\".");
    }
    if (weights_iter == inputs.end()) {
      log_and_throw("Cannot train without argument names \"weights\".");
    }
    const shared_float_array &input_batch = input_iter->second;
    const shared_float_array &label_batch = labels_iter->second;
    const shared_float_array &weight_batch = weights_iter->second;
    NSData *inputData = convert_hwc_array_to_chw_data(input_batch);

    size_t batch_size = label_batch.shape()[0];
    // prepare labels
    NSData *labelsData =
        encodeLabels(label_batch.data(), (NSUInteger)batch_size, (NSUInteger)num_classes_);
    NSData *weightsData =
        encodeWeights(weight_batch.data(), (NSUInteger)batch_size, (NSUInteger)num_classes_);

    // Dispatch the batch to MLCompute, with a completion handler wrapping a
    // promise for the resulting loss value.
    __block std::promise<shared_float_array> loss_promise;
    __block std::promise<shared_float_array> output_promise;
    [training_graph_
        executeWithInputsData:@{input_.label : TCMLComputeWrapData(inputData)}
               lossLabelsData:@{labels_.label : TCMLComputeWrapData(labelsData)}
         lossLabelWeightsData:@{weights_.label : TCMLComputeWrapData(weightsData)}
                    batchSize:(NSUInteger)batch_size
                      options:MLCExecutionOptionsNone
            completionHandler:^(MLCTensor __autoreleasing *_Nullable resultTensor,
                                NSError *_Nullable error, CFTimeInterval executionTime) {
              // Propagate MLCompute errors as C++ exceptions.
              if (error != nil) {
                loss_promise.set_exception(std::make_exception_ptr(
                    std::runtime_error(error.localizedDescription.UTF8String)));
                return;
              }

              // Fulfill the promise.
              const float *loss_ptr = reinterpret_cast<const float *>(resultTensor.data.bytes);
              loss_promise.set_value(shared_float_array::copy(loss_ptr, {batch_size}));
            }];
    [inference_graph_
        executeWithInputsData:@{input_.label : TCMLComputeWrapData(inputData)}
                    batchSize:(NSUInteger)batch_size
                      options:MLCExecutionOptionsNone
            completionHandler:^(MLCTensor __autoreleasing *_Nullable resultTensor,
                                NSError *_Nullable error, CFTimeInterval executionTime) {
              // Propagate MLCompute errors as C++ exceptions.
              if (error != nil) {
                output_promise.set_exception(std::make_exception_ptr(
                    std::runtime_error(error.localizedDescription.UTF8String)));
                return;
              }

              // Fulfill the promise.
              const float *output_ptr = reinterpret_cast<const float *>(resultTensor.data.bytes);
              output_promise.set_value(
                  shared_float_array::copy(output_ptr, {batch_size, num_classes_}));
            }];

    // // Return the wrapped future from the promise.
    shared_float_array loss(std::make_shared<deferred_float_array>(
        loss_promise.get_future(), std::vector<size_t>({batch_size})));
    shared_float_array output(std::make_shared<deferred_float_array>(
        output_promise.get_future(), std::vector<size_t>({batch_size, num_classes_})));
    return {{"loss", loss}, {"output", output}};
  }
}

float_array_map mlc_drawing_classifier_backend::predict(const float_array_map &inputs) const
{
  @autoreleasepool {
    auto input_iter = inputs.find("input");
    if (input_iter == inputs.end()) {
      log_and_throw("Cannot train without argument named \"input\".");
    }
    //    auto weights_iter = inputs.find("weights");
    const shared_float_array &input_batch = input_iter->second;
    NSData *inputData = convert_hwc_array_to_chw_data(input_batch);

    size_t batch_size = input_batch.shape()[0];

    // Dispatch the batch to MLCompute, with a completion handler wrapping a
    // promise for the resulting output value.
    __block std::promise<shared_float_array> output_promise;
    [inference_graph_
        executeWithInputsData:@{input_.label : TCMLComputeWrapData(inputData)}
                    batchSize:batch_size
                      options:MLCExecutionOptionsNone
            completionHandler:^(MLCTensor __autoreleasing *_Nullable resultTensor,
                                NSError *_Nullable error, CFTimeInterval executionTime) {
              // Propagate MLCompute errors as C++ exceptions.
              if (error != nil) {
                output_promise.set_exception(std::make_exception_ptr(
                    std::runtime_error(error.localizedDescription.UTF8String)));
                return;
              }

              // Fulfill the promise.
              const float *output_ptr = reinterpret_cast<const float *>(resultTensor.data.bytes);
              output_promise.set_value(
                  shared_float_array::copy(output_ptr, {batch_size, num_classes_}));
            }];

    // Return the wrapped future from the promise.
    shared_float_array output(std::make_shared<deferred_float_array>(
        output_promise.get_future(), std::vector<size_t>({batch_size, num_classes_})));

    auto labels_iter = inputs.find("labels");
    if (labels_iter != inputs.end()) {
      shared_float_array loss = shared_float_array::wrap(std::vector<float>(1, 0.f), {1});
      return {{"loss", loss}, {"output", output}};
    } else {
      return {{"output", output}};
    }
  }
}

}  // namespace neural_net
}  // namespace turi

/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
// Include this header first since it pulls in SFrame headers that rely on MAX
// not being a macro defined by the Foundation Obj-C headers.
#include <toolkits/object_detection/od_darknet_yolo_model_trainer.hpp>

// Header declaring the symbols defined in this file.
#include <ml/neural_net/mlc_od_backend.hpp>

// Standard headers.
#include <algorithm>

#import <ml/neural_net/TCMLComputeObjectDetectorDescriptor.h>
#import <ml/neural_net/TCMLComputeUtil.h>
#include <ml/neural_net/mlc_utils.hpp>

// Import Obj-C headers last to minimize issues.
#import <ml/neural_net/TCModelTrainerBackendGraphs.h>

namespace turi {
namespace neural_net {
namespace {

// TODO: Expose these from DarknetYOLOCheckpoint.
constexpr size_t kNumAnchorBoxes = 15;
constexpr size_t kSpatialReduction = 32;

API_AVAILABLE(macos(10.16))
MLCYOLOLossDescriptor *create_YOLO_loss_descriptor(const float_array_map &config)
{
  simd_float2 anchor_boxes[] = {{1.0f, 2.0f},   {1.0f, 1.0f},   {2.0f, 1.0f},  {2.0f, 4.0f},
                                {2.0f, 2.0f},   {4.0f, 2.0f},   {4.0f, 8.0f},  {4.0f, 4.0f},
                                {8.0f, 4.0f},   {8.0f, 16.0f},  {8.0f, 8.0f},  {16.0f, 8.0f},
                                {16.0f, 32.0f}, {16.0f, 16.0f}, {32.0f, 16.0f}};

  NSUInteger numberOfAnchorBoxes = sizeof(anchor_boxes) / sizeof(simd_float2);
  NSData *anchorBoxes = [NSData dataWithBytes:anchor_boxes
                                       length:numberOfAnchorBoxes * sizeof(simd_float2)];
  MLCYOLOLossDescriptor *desc =
      [MLCYOLOLossDescriptor descriptorWithAnchorBoxes:anchorBoxes
                                        anchorBoxCount:numberOfAnchorBoxes];

  // Set values from config.
  desc.scaleSpatialPositionLoss =
      get_array_map_scalar(config, "od_scale_xy", desc.scaleSpatialPositionLoss);
  desc.scaleSpatialSizeLoss =
      get_array_map_scalar(config, "od_scale_wh", desc.scaleSpatialSizeLoss);
  desc.scaleNoObjectConfidenceLoss =
      get_array_map_scalar(config, "od_scale_no_object", desc.scaleNoObjectConfidenceLoss);
  desc.scaleObjectConfidenceLoss =
      get_array_map_scalar(config, "od_scale_object", desc.scaleObjectConfidenceLoss);
  desc.scaleClassLoss = get_array_map_scalar(config, "od_scale_class", desc.scaleClassLoss);
  desc.maximumIOUForObjectAbsence =
      get_array_map_scalar(config, "od_max_iou_for_no_object", desc.maximumIOUForObjectAbsence);
  desc.minimumIOUForObjectPresence =
      get_array_map_scalar(config, "od_min_iou_for_object", desc.minimumIOUForObjectPresence);
  desc.shouldRescore = get_array_map_bool(config, "od_rescore", desc.shouldRescore);

  return desc;
}

API_AVAILABLE(macos(10.16))
MLCOptimizerDescriptor *create_optimizer_descriptor(const float_array_map &config)
{
  float learningRate = get_array_map_scalar(config, "learning_rate", 1e-3f);
  float gradientRescale = 1.f;
  float gradientClipMax = get_array_map_scalar(config, "gradient_clipping", 0.f);
  float regularizationScale = get_array_map_scalar(config, "weight_decay", 0.f);
  MLCRegularizationType regularizationType =
      regularizationScale > 0.f ? MLCRegularizationTypeL2 : MLCRegularizationTypeNone;
  return [MLCOptimizerDescriptor descriptorWithLearningRate:learningRate
                                            gradientRescale:gradientRescale
                                    appliesGradientClipping:YES
                                            gradientClipMax:gradientClipMax
                                            gradientClipMin:-gradientClipMax
                                         regularizationType:regularizationType
                                        regularizationScale:regularizationScale];
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
    if ([key hasPrefix:@"conv"]) {
      if ([key hasSuffix:@"_weight"]) {
        layer_weights.add_conv_weight(kv.first, kv.second);
      } else if ([key hasSuffix:@"_bias"]) {
        layer_weights.add_conv_bias(kv.first, kv.second);
      }
    } else if ([key hasPrefix:@"batchnorm"]) {
      layer_weights.add_flat_array(kv.first, kv.second);
    }
  }

  return layer_weights;
}

}  // namespace

// static
TCModelTrainerBackendGraphs *mlc_object_detector_backend::create_graphs(
    const turi::object_detection::DarknetYOLOCheckpoint &checkpoint)
{
  size_t n = checkpoint.config().batch_size;
  size_t c_out = kNumAnchorBoxes * (5 + checkpoint.config().num_classes);
  size_t h_out = checkpoint.config().output_height;
  size_t w_out = checkpoint.config().output_width;
  size_t c_in = 3;  // RGB
  size_t h_in = h_out * kSpatialReduction;
  size_t w_in = w_out * kSpatialReduction;

  return create_graphs(n, c_in, h_in, w_in, c_out, h_out, w_out, checkpoint.internal_config(),
                       checkpoint.internal_weights(), /* layer_weights */ nullptr);
}

// static
TCModelTrainerBackendGraphs *mlc_object_detector_backend::create_graphs(
    size_t n, size_t c_in, size_t h_in, size_t w_in, size_t c_out, size_t h_out, size_t w_out,
    const float_array_map &config, const float_array_map &weights,
    mlc_layer_weights *layer_weights_out)
{
  @autoreleasepool {
    TCModelTrainerBackendGraphs *result = [[TCModelTrainerBackendGraphs alloc] init];
    TCMLComputeObjectDetectorDescriptor *descriptor =
        [[TCMLComputeObjectDetectorDescriptor alloc] init];

    // Define input and output tensor shape.
    MLCTensor *input = [MLCTensor tensorWithWidth:w_in
                                           height:h_in
                              featureChannelCount:c_in
                                        batchSize:n];
    input.label = @"input";
    MLCTensor *labels = [MLCTensor tensorWithWidth:w_out
                                            height:h_out
                               featureChannelCount:c_out
                                         batchSize:n];
    labels.label = @"labels";

    descriptor.inputTensor = input;
    descriptor.outputChannels = c_out;

    // Wrap weights.
    // Note: this also records references to the MLCTensor values,
    // created to simplify later export of the updated weights.
    mlc_layer_weights layer_weights = import_weights(weights);
    descriptor.weights = layer_weights.tensor_views();
    result.layerWeights = layer_weights.tensor_weights();

    MLCGraph *graph = [MLCGraph tc_graphForObjectDetectorDescriptor:descriptor];

    // Configure YOLO loss.
    MLCYOLOLossLayer *lossLayer =
        [MLCYOLOLossLayer layerWithDescriptor:create_YOLO_loss_descriptor(config)];

    // Configure optimizer.
    MLCSGDOptimizer *optimizer =
        [MLCSGDOptimizer optimizerWithDescriptor:create_optimizer_descriptor(config)
                                   momentumScale:0.9f
                             usesNestrovMomentum:NO];

    // Instantiate training graph.
    result.trainingGraph = [MLCTrainingGraph trainingGraphWithGraphObjects:@[ graph ]
                                                                 lossLayer:lossLayer
                                                                 optimizer:optimizer];

    result.trainingInputs = @{input.label : input};
    result.trainingLossLabels = @{labels.label : labels};
    BOOL success = [result.trainingGraph addInputs:result.trainingInputs
                                        lossLabels:result.trainingLossLabels];
    if (!success) {
      log_and_throw("Error adding inputs and labels to training graph");
    }

    // Instantiate the inference graph.
    result.inferenceGraph = [MLCInferenceGraph graphWithGraphObjects:@[ graph ]];
    result.inferenceInputs = @{input.label : input};

    success = [result.inferenceGraph addInputs:result.inferenceInputs];
    if (!success) {
      log_and_throw("Error adding inputs to inference graph");
    }

    if (layer_weights_out) {
      *layer_weights_out = std::move(layer_weights);
    }
    return result;
  }  // autoreleasepool
}

mlc_object_detector_backend::mlc_object_detector_backend(MLCDevice *device, size_t n, size_t c_in,
                                                         size_t h_in, size_t w_in, size_t c_out,
                                                         size_t h_out, size_t w_out,
                                                         const float_array_map &config,
                                                         const float_array_map &weights)
{
  @autoreleasepool {
    TCModelTrainerBackendGraphs *graphs =
        create_graphs(n, c_in, h_in, w_in, c_out, h_out, w_out, config, weights, &layer_weights_);

    input_ = graphs.trainingInputs[@"input"];
    labels_ = graphs.trainingLossLabels[@"labels"];
    output_shape_ = {n, h_out, w_out, c_out};

    // Compile the training graph.
    training_graph_ = graphs.trainingGraph;
    BOOL success = [training_graph_ compileWithOptions:MLCGraphCompilationOptionsNone
                                                device:device];
    if (!success) {
      log_and_throw("Error compiling object detector MLCTrainingGraph");
    }

    // Compile the inference graph.
    inference_graph_ = graphs.inferenceGraph;
    success = [inference_graph_ compileWithOptions:MLCGraphCompilationOptionsNone device:device];
    if (!success) {
      log_and_throw("Error compiling object detector MLCInferenceGraph");
    }

  }  // autoreleasepool
}

float_array_map mlc_object_detector_backend::export_weights() const
{
  @autoreleasepool {
    // Copies weights from GPU to the tensors referenced in export_infos_.
    [training_graph_ synchronizeUpdates];

    return layer_weights_.export_weights();
  }
}

void mlc_object_detector_backend::set_learning_rate(float lr)
{
  @autoreleasepool {
    training_graph_.optimizer.learningRate = lr;
  }
}

float_array_map mlc_object_detector_backend::train(const float_array_map &inputs)
{
  @autoreleasepool {
    // Extract and convert the input image and annotations.
    auto input_iter = inputs.find("input");
    auto labels_iter = inputs.find("labels");
    if (input_iter == inputs.end()) {
      log_and_throw("Cannot train without argument named \"input\".");
    }
    if (labels_iter == inputs.end()) {
      log_and_throw("Cannot train without argument named \"labels\".");
    }
    const shared_float_array &input_batch = input_iter->second;
    const shared_float_array &label_batch = labels_iter->second;
    NSData *inputData = convert_hwc_array_to_chw_data(input_batch);
    NSData *labelsData = convert_hwc_array_to_chw_data(label_batch);

    // Dispatch the batch to MLCompute, with a completion handler wrapping a
    // promise for the resulting loss value.
    size_t batch_size = input_batch.shape()[0];
    __block std::promise<shared_float_array> loss_promise;
    [training_graph_
        executeWithInputsData:@{input_.label : TCMLComputeWrapData(inputData)}
               lossLabelsData:@{labels_.label : TCMLComputeWrapData(labelsData)}
         lossLabelWeightsData:nil
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

    // Return the wrapped future from the promise.
    shared_float_array loss(std::make_shared<deferred_float_array>(
        loss_promise.get_future(), std::vector<size_t>({batch_size})));
    return {{"loss", loss}};
  }
}

float_array_map mlc_object_detector_backend::predict(const float_array_map &inputs) const
{
  @autoreleasepool {
    // Extract and convert the input image.
    // TODO: Eventually this should also support computing loss from labels, when
    // computing validation metrics.
    auto input_iter = inputs.find("input");
    if (input_iter == inputs.end()) {
      log_and_throw("Cannot train without argument named \"input\".");
    }
    const shared_float_array &input_batch = input_iter->second;
    NSData *inputData = convert_hwc_array_to_chw_data(input_batch);

    // Dispatch the batch to MLCompute, with a completion handler wrapping a
    // promise for the resulting output value.
    size_t batch_size = input_batch.shape()[0];
    __block std::promise<shared_float_array> output_promise;
    __block std::vector<size_t> output_shape = output_shape_;
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
              shared_float_array output =
                  convert_chw_data_to_hwc_array(resultTensor.data, output_shape);
              output_promise.set_value(std::move(output));
            }];

    // Return the wrapped future from the promise.
    shared_float_array output(
        std::make_shared<deferred_float_array>(output_promise.get_future(), output_shape));
    return {{"output", output}};
  }
}

}  // neural_net
}  // turi

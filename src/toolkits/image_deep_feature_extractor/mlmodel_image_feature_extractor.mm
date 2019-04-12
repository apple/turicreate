/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/image_deep_feature_extractor/mlmodel_image_feature_extractor.hpp>

#include <boost/filesystem.hpp>
#include <fileio/fileio_constants.hpp>
#include <fileio/curl_downloader.hpp>
#include <fileio/fs_utils.hpp>
#include <logger/logger.hpp>
#include <parallel/lambda_omp.hpp>
#include <table_printer/table_printer.hpp>
#include <unity/lib/image_util.hpp>
#include <unity/toolkits/coreml_export/mlmodel_include.hpp>

#import <CoreML/CoreML.h>
#include <memory>

#include <mlmodel/src/Format.hpp>

namespace turi {
namespace image_deep_feature_extractor {

namespace {

struct neural_network_model_details {
  const int input_width;
  const int input_height;
  const int feature_layer_size;
  const std::string feature_layer_output_name;
  const std::string input_name;
  const std::string base_model_url;

  void modify_neural_network(const std::string& base_model_path, const std::string& modified_model_path) const {
    // Load base model and get the spec
    CoreML::Model model = CoreML::Model();
    CoreML::Result r = CoreML::Model::load(base_model_path, model);
    if(!r.good()) {
      log_and_throw("Could not load model: " + r.message());
    }
    CoreML::Specification::Model& base_model = model.getProto();

    // Create a modified model which is just a neuralNetwork, not nerualNetworkClassifier
    CoreML::Specification::Model modified_model = CoreML::Specification::Model();
    modified_model.set_specificationversion(base_model.specificationversion());
    CoreML::Specification::NeuralNetwork* modified_neural_network = modified_model.mutable_neuralnetwork();

    // Copy the input
    const CoreML::Specification::ModelDescription base_desc = base_model.description();
    CoreML::Specification::ModelDescription* modified_desc = modified_model.mutable_description();
    const ::CoreML::Specification::FeatureDescription& input = base_desc.input(0);
    modified_desc->add_input()->CopyFrom(input);

    // Sanity check input
    ASSERT_TRUE(input.type().has_imagetype());
    const ::CoreML::Specification::ImageFeatureType& image_input = input.type().imagetype();
    ASSERT_EQ(image_input.width(), this->input_width);
    ASSERT_EQ(image_input.height(), this->input_height);

    // For backwards compatibility, do not copy preprocessing

    // Copy the needed layers
    const CoreML::Specification::NeuralNetworkClassifier base_neural_network = base_model.neuralnetworkclassifier();
    for(int i = 0; i < base_neural_network.layers_size(); i++) {
      CoreML::Specification::NeuralNetworkLayer* dest_layer = modified_neural_network->add_layers();
      const CoreML::Specification::NeuralNetworkLayer source_layer = base_neural_network.layers(i);
      dest_layer->CopyFrom(source_layer);

      // If this layer outputs what we need, then we're done copying.
      if(has_feature_layer_output_name(source_layer))
          break;
    }

    // Set the output
    CoreML::Specification::FeatureDescription* new_output = modified_desc->add_output();
    new_output->set_name(this->feature_layer_output_name);
    CoreML::Specification::ArrayFeatureType* output_type = new_output->mutable_type()->mutable_multiarraytype();
    output_type->set_datatype(CoreML::Specification::ArrayFeatureType_ArrayDataType::ArrayFeatureType_ArrayDataType_DOUBLE);
    output_type->add_shape(1);
    output_type->add_shape(1);
    output_type->add_shape(this->feature_layer_size);
    output_type->add_shape(1);
    output_type->add_shape(1);

    // Save modified model
    r = CoreML::Model(modified_model).save(modified_model_path);
    if(!r.good()) {
      log_and_throw("Could not export model: " + r.message());
    }

  }

private:
  bool has_feature_layer_output_name(const CoreML::Specification::NeuralNetworkLayer& layer) const {
    const auto& outputs = layer.output();
    return std::find(outputs.begin(), outputs.end(), feature_layer_output_name) != outputs.end();
  }
};


const std::map<const std::string, const neural_network_model_details> model_name_to_info =
  {{"resnet-50", {224, 224, 2048, "flatten0", "data",
                  "Resnet50.mlmodel"}},
   {"VisionFeaturePrint_Scene", {299, 299, 2048, "output", "image_input", ""}},
   {"squeezenet_v1.1", {227, 227, 1000, "pool10", "image",
                        "https://docs-assets.developer.apple.com/coreml/models/SqueezeNet.mlmodel"}}};


static void checkNSError(NSError *error) {
  if (error != nil) {
    std::stringstream msg;
    msg<<"CoreML Error: "<<std::string(error.localizedDescription.UTF8String);
    log_and_throw(msg.str());
  }
}


const neural_network_model_details& get_model_info(const std::string& model_name) {
  auto model_info_entry = model_name_to_info.find(model_name);
  ASSERT_TRUE(model_info_entry != model_name_to_info.end());
  return model_info_entry->second;
}

void build_vision_feature_print_scene_spec(const std::string& model_path) {
  const neural_network_model_details& model_info = get_model_info("VisionFeaturePrint_Scene");

  CoreML::Specification::Model spec = CoreML::Specification::Model();
  spec.set_specificationversion(CoreML::MLMODEL_SPECIFICATION_VERSION);

  auto* description = spec.mutable_description();

  auto* input = description->add_input();
  input->set_name("image_input");
  auto* input_type = input->mutable_type()->mutable_imagetype();

  input_type->set_width(model_info.input_width);
  input_type->set_height(model_info.input_height);

  input_type->set_colorspace(CoreML::Specification::ImageFeatureType::BGR);

  auto* output = description->add_output();
  output->set_name("output");
  auto* output_type = output->mutable_type()->mutable_multiarraytype();
  output_type->set_datatype(CoreML::Specification::ArrayFeatureType::DOUBLE);
  output_type->add_shape(model_info.feature_layer_size);

  auto vision_feature_print = spec.mutable_visionfeatureprint();
  auto scene = vision_feature_print->mutable_scene();
  scene->set_version(CoreML::Specification::CoreMLModels::VisionFeaturePrint_Scene_SceneVersion_SCENE_VERSION_1);

  // Save the model
  CoreML::Result r = CoreML::Model(spec).save(model_path);
  if(!r.good()) {
    log_and_throw("Could not save model: " + r.message());
  }

}

API_AVAILABLE(macos(10.13),ios(11.0))
static MLModel *create_model(const std::string& download_path,
			     const std::string& model_name) {

  @autoreleasepool {

  MLModel* result = nil;

  const std::string compiled_modified_model_path = download_path + "/" + model_name + "_modified.mlmodelc";
  NSURL* compiledModelURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:compiled_modified_model_path.c_str()]];

  // If the compiled modified model already exists on disk, attempt to load it.
  if (boost::filesystem::exists(compiled_modified_model_path)) {

    NSError* error = nil;
    result = [MLModel modelWithContentsOfURL:compiledModelURL error:&error];

    if (error || !result) {

      // The compiled model appears to be corrupted. Attempt to delete it.
      if (!fileio::delete_path_recursive(compiled_modified_model_path)) {
        log_and_throw("Model at " + compiled_modified_model_path + " could not be loaded or deleted.");
      }

      // Ensure that we attempt to regenerate the modified model below.
      result = nil;
    }
  }

  // Create the compiled modified model, if we don't already have it
  if (!result) {

    // Create the modified model
    const std::string modified_model_path = download_path + "/" + model_name + "_modified.mlmodel";
    if(model_name == "VisionFeaturePrint_Scene") {
      build_vision_feature_print_scene_spec(modified_model_path);
    } else {
      std::string base_model_path;
      const neural_network_model_details& model_info = get_model_info(model_name);

      if(turi::fileio::get_protocol(model_info.base_model_url) != "") {
        base_model_path = download_path + "/" + model_name + ".mlmodel";
        logstream(LOG_PROGRESS) << "Downloading base mlmodel" << std::endl;
        const int download_ret = turi::download_url(model_info.base_model_url, base_model_path);
        if(download_ret != 0) {
          log_and_throw("Unable to download model.");
        }
      } else {
        base_model_path = download_path + "/" + model_info.base_model_url;
      }

      model_info.modify_neural_network(base_model_path, modified_model_path);
    }

    @autoreleasepool {
      NSError* error = nil;

      // Swallow output for the very verbose coremlcompiler
      int stdoutBack = dup(STDOUT_FILENO);
      int devnull = open("/dev/null", O_WRONLY);
      dup2(devnull, STDOUT_FILENO);

      // Compile the modified model
      NSString* temp = [NSString stringWithUTF8String:modified_model_path.c_str()];
      NSURL* specPath = [NSURL fileURLWithPath:temp];
      NSURL* modelPath = [MLModel compileModelAtURL:specPath error:&error];
      checkNSError(error);

      // Close all the file descriptors and revert back to normal
      dup2(stdoutBack, STDOUT_FILENO);
      close(devnull);
      close(stdoutBack);

      // Copy the compiled modified model
      temp = [NSString stringWithUTF8String:compiled_modified_model_path.c_str()];
      NSURL* compiledModelPath = [NSURL fileURLWithPath:temp];
      [[NSFileManager defaultManager] copyItemAtURL:modelPath toURL:compiledModelPath error:&error];
      checkNSError(error);
    }

    // Load the compiled modified model
    NSError* error = nil;
    result = [MLModel modelWithContentsOfURL:compiledModelURL error:&error];
    checkNSError(error);
  }

  return [result retain];  // Safe to retain now that no exceptions possible

  }  // @autoreleasepool
}


static void handleCVReturn(CVReturn status) {
  if (status != kCVReturnSuccess) {
    std::stringstream msg;
    msg << "Got unexpected return code " << status << " from CoreVideo.";
    log_and_throw(msg.str());
  }
}

CVPixelBufferRef create_pixel_buffer_from_flex_image(const flex_image image) {
  // The code in this function is largely adapted from convertValueToImage here:
  // https://github.com/apple/coremltools/blob/master/coremlpython/CoreMLPythonUtils.mm

  ASSERT_TRUE(image.is_decoded());
  ASSERT_EQ(image.m_channels, 3);   // RGB == 3

  CVPixelBufferRef result = nil;
  size_t width = image.m_width;
  size_t height = image.m_height;
  OSType format = kCVPixelFormatType_32BGRA;

  // Allocate pixel buffer
  CVReturn status = CVPixelBufferCreate(kCFAllocatorDefault, width, height, format, NULL, &result);
  handleCVReturn(status);

  // copy data into the pixel buffer
  status = CVPixelBufferLockBaseAddress(result, 0);
  handleCVReturn(status);
  void *baseAddress = CVPixelBufferGetBaseAddress(result);
  ASSERT_TRUE(baseAddress != nullptr);
  ASSERT_TRUE(!CVPixelBufferIsPlanar(result));
  size_t bytesPerRow = CVPixelBufferGetBytesPerRow(result);
  const unsigned char* srcPointer = image.get_image_data();

  // convert RGB to BGRA
  ASSERT_TRUE(image.m_image_data_size == width * height * 3);
  ASSERT_TRUE(bytesPerRow >= width * 4);
  for (size_t row = 0; row < height; row++) {
    char *dstPointer = static_cast<char *>(baseAddress) + (row * bytesPerRow);

    for (size_t col = 0; col < width; col++) {

      const char R = *srcPointer;
      srcPointer++;
      const char G = *srcPointer;
      srcPointer++;
      const char B = *srcPointer;
      srcPointer++;

      *dstPointer = B;
      dstPointer++;
      *dstPointer = G;
      dstPointer++;
      *dstPointer = R;
      dstPointer++;
      *dstPointer = 0; // A
      dstPointer++;

    }
    ASSERT_TRUE(bytesPerRow >= width * 4);
  }
  ASSERT_TRUE(srcPointer == image.get_image_data() + image.m_image_data_size);

  return result;
}

}  // namespace

struct mlmodel_image_feature_extractor::impl {
  API_AVAILABLE(macos(10.13),ios(11.0))
  ~impl() {
    [model release];
  }

  std::string name;
  API_AVAILABLE(macos(10.13),ios(11.0)) MLModel *model = nil;
  CoreML::Specification::Model spec;
};

API_AVAILABLE(macos(10.13),ios(11.0))
mlmodel_image_feature_extractor::mlmodel_image_feature_extractor(
    const std::string& model_name, const std::string& download_path)
  : m_impl(new impl) {
  // Validate model name
  if (model_name_to_info.find(model_name) == model_name_to_info.end()) {
    std::stringstream msg;
    msg << "Unrecognized model name: \"" << model_name <<"\"";
    log_and_throw(msg.str());
  }

  m_impl->name = model_name;
  m_impl->model = create_model(download_path, model_name);

  // Read the spec from the file written to produce the MLModel.
  // TODO: Just save this value before writing it to disk.
  const std::string file_path = download_path + "/" + model_name + "_modified.mlmodel";
  CoreML::Model deep_feature_nn = CoreML::Model();
  CoreML::Result r = CoreML::Model::load(file_path, deep_feature_nn);
  if(!r.good()) {
    log_and_throw("Could not load model: " + r.message());
  }
  m_impl->spec.CopyFrom(deep_feature_nn.getProto());
}

const CoreML::Specification::Model&
mlmodel_image_feature_extractor::coreml_spec() const {
  return m_impl->spec;
}

API_AVAILABLE(macos(10.13),ios(11.0))
gl_sarray
mlmodel_image_feature_extractor::extract_features(gl_sarray data, bool verbose, size_t kBatchSize) const {
  ASSERT_EQ((int)data.dtype(), (int)flex_type_enum::IMAGE);
  ASSERT_TRUE(kBatchSize >= 1);

  const neural_network_model_details& model_info = get_model_info(m_impl->name);

  BOOL use_only_cpu = (turi::fileio::NUM_GPUS == 0);

  std::vector<flexible_type> result(data.size());

  mutex mut;

  table_printer table(
        { {"Images Processed", 0}, {"Elapsed Time", 0}, {"Percent Complete", 0} }, 0);
  if (verbose) {
    logprogress_stream << "Analyzing and extracting image features." << std::endl;
    table.print_header();
  }

  // Lambda converting one flex_image from `data` into a MLFeatureProvider to
  // feed into the CoreML model. Must be called inside an autorelease pool.
  auto convert_image_to_feature_provider = [&](size_t i) {
    flexible_type decoded_image = image_util::resize_image(data[i], model_info.input_width, model_info.input_height, 3, true);
    const flex_image& image = decoded_image.get<flex_image>();
    CVPixelBufferRef buffer = create_pixel_buffer_from_flex_image(image);
    MLFeatureValue* image_feature = [MLFeatureValue featureValueWithPixelBuffer:buffer];
    CFRelease(buffer);

    NSString* input_name = [NSString stringWithUTF8String: model_info.input_name.c_str()];
    NSError *error = nil;
    MLDictionaryFeatureProvider *input = [[[MLDictionaryFeatureProvider alloc] initWithDictionary:@{input_name: image_feature} error:&error] autorelease];
    checkNSError(error);  // Can throw, must autorelease before here.
    return input;
  };

  // Lambda converting one MLFeatureProvider output from the CoreML model into
  // a flex_vec value, written to `result[i]`. Must be called inside an
  // autorelease pool.
  auto set_output_vector =
      [&](size_t i, id<MLFeatureProvider> model_prediction) {
    MLFeatureValue* deep_features = [model_prediction featureValueForName: [NSString stringWithUTF8String: model_info.feature_layer_output_name.c_str()]];
    MLMultiArray* deep_features_values = [deep_features multiArrayValue];

    // Santiy check prediction shape
    NSArray<NSNumber *> * shape = [deep_features_values shape];
    size_t feature_dim = -1;
    if(m_impl->name != "VisionFeaturePrint_Scene") {
      ASSERT_EQ(shape.count, (unsigned long)5);
      ASSERT_EQ(shape[0].intValue, 1);
      ASSERT_EQ(shape[1].intValue, 1);
      ASSERT_EQ(shape[2].intValue, model_info.feature_layer_size);
      ASSERT_EQ(shape[3].intValue, 1);
      ASSERT_EQ(shape[4].intValue, 1);
      feature_dim = 2;
    } else {
      ASSERT_EQ(shape.count, (unsigned long)1);
      ASSERT_EQ(shape[0].intValue, model_info.feature_layer_size);
      feature_dim = 0;
    }

    // Copy deep features to a flexible type vector
    size_t deep_feature_length = shape[feature_dim].intValue;
    size_t stride = deep_features_values.strides[feature_dim].intValue;
    flex_vec dest(deep_feature_length);
    double *srcPtr = (double *) deep_features_values.dataPointer;
    for(size_t j = 0; j < deep_feature_length; j++) {
      size_t offset = j * stride;
      dest[j] = srcPtr[offset];
    }
    result[i] = std::move(dest);
  };

  // Lambda performing feature extraction on one batch of images, writing the
  // output into `results`. Must be called inside an autorelease pool.
  auto perform_batch = [&](size_t batch_index) {
    const size_t batch_offset = batch_index * kBatchSize;
    const size_t batch_end = std::min(data.size(), batch_offset + kBatchSize);
    const size_t batch_size = batch_end - batch_offset;

    // Create the batch input for the CoreML model.
    NSMutableArray<id<MLFeatureProvider>> *inputs =
        [NSMutableArray arrayWithCapacity:batch_size];
    for (size_t i = 0; i < batch_size; ++i) {
      [inputs addObject: convert_image_to_feature_provider(batch_offset + i)];
    }
    NSMutableArray<id<MLFeatureProvider>> *outputs =
        [NSMutableArray arrayWithCapacity:batch_size];

    // The CoreML batch API only exists if the base SDK is new enough.
#ifdef HAS_CORE_ML_BATCH_INFERENCE
    // Even when compiled with a new enough SDK, guard against older deployment
    // targets at runtime.
    if (@available(macOS 10.14, iOS 12.0,  *)) {
      // Invoke CoreML using the batch inference API for better performance.
      MLArrayBatchProvider *image_batch = [[MLArrayBatchProvider alloc] initWithFeatureProviderArray: inputs];
      MLPredictionOptions* options = [[MLPredictionOptions alloc] init];
      [options setUsesCPUOnly:use_only_cpu];
      NSError *error = nil;
      id<MLBatchProvider> features_batch = [m_impl->model predictionsFromBatch:image_batch options:options error:&error];
      [options release];
      [image_batch release];
      checkNSError(error);

      for (NSInteger i = 0; i < features_batch.count; ++i) {
        [outputs addObject:[features_batch featuresAtIndex:i]];
      }
    } else {
#else
    {
#endif
      // Once it's our turn to use CoreML, don't let any other threads in until
      // we're done and ready to move on to the CPU-bound phase of processing.
      std::lock_guard<mutex> lock(mut);
      for (size_t i = 0; i < batch_size; ++i) {
        // Invoke the CoreML model.
        NSError *error = nil;
        MLPredictionOptions* options = [[MLPredictionOptions alloc] init];
        [options setUsesCPUOnly:use_only_cpu];
        id<MLFeatureProvider> features = [m_impl->model predictionFromFeatures:inputs[i]  options:options error:&error];
        [options release];
        checkNSError(error);

        // Just collect the outputs for now. Delay any copying until after we
        // release the mutex.
        [outputs addObject:features];
      }
    }

    // Convert/copy the output of the CoreML model.
    for (size_t i = 0; i < batch_size; ++i) {
      set_output_vector(batch_offset + i, outputs[i]);
    }
  };

  // Submit batches in parallel, one for each CPU core, so that:
  // - CoreML is busy all the time, assuming each core can prepare a batch
  //   faster than CoreML can evaluate the other n - 1 batches.
  // - Every core is busy, except when there is a backlog of batches.
  // - The number of batches in flight is bounded (by the number of cores).
  // - The worker threads do not contend or synchronize with one another, except
  //   within CoreML and when joining at the very end.
  std::atomic<size_t> batches_completed(0);
  const size_t batch_count = (data.size() + kBatchSize - 1) / kBatchSize;
  parallel_for(0, batch_count, [&](size_t batch_index) {
    @autoreleasepool {

      perform_batch(batch_index);

      // Only touch the atomic variable once per iteration
      const size_t local_batches_completed = ++batches_completed;

      if (verbose && local_batches_completed < batch_count) {
        std::ostringstream d;
        // For pretty printing, floor percent done
        // resolution to the nearest .25% interval.  Do this by multiplying by
        // 400, then do integer division by the total size, then float divide
        // by 4.0
        d << local_batches_completed * 400 / batch_count / 4.0 << '%';
        table.print_progress_row(local_batches_completed,
                                 local_batches_completed * kBatchSize,
                                 progress_time(), d.str());
      }

    } // end autoreleasepool
  });

  if (verbose) {
    table.print_row(data.size(), progress_time(), "100%");
    table.print_footer();
  }
  return gl_sarray(result, flex_type_enum::VECTOR);
}

} // namespace image_deep_feature_extractor
} // namespace turi

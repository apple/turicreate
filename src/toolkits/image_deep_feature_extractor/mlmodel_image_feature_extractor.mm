/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/image_deep_feature_extractor/mlmodel_image_feature_extractor.hpp>

#include <boost/filesystem.hpp>
#include <fileio/curl_downloader.hpp>
#include <fileio/fs_utils.hpp>
#include <logger/logger.hpp>
#include <unity/lib/image_util.hpp>
#include <unity/toolkits/coreml_export/mlmodel_include.hpp>

#import <CoreML/CoreML.h>

#include <unity/toolkits/coreml_export/MLModel/src/Format.hpp>

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


static MLModel *create_model(const std::string& download_path,
			     const std::string& model_name) {
  const std::string modified_model_path = download_path + "/" + model_name + "_modified.mlmodel";
  if(! boost::filesystem::exists(modified_model_path)) {
    std::string base_model_path;
    const neural_network_model_details& model_info = get_model_info(model_name);

    if(turi::fileio::get_protocol(model_info.base_model_url) != "") {
      base_model_path = download_path + "/" + model_name + ".mlmodel";
      logstream(LOG_PROGRESS) << "Downloading base mlmodel" << std::endl;
      turi::download_url(model_info.base_model_url, base_model_path);
    } else {
      base_model_path = download_path + "/" + model_info.base_model_url;
    }

    model_info.modify_neural_network(base_model_path, modified_model_path);
  }

  // Load the model.
  MLModel* result = nil;
  @autoreleasepool {
    NSError* error = nil;
    NSString* temp = [NSString stringWithUTF8String:modified_model_path.c_str()];
    NSURL* specPath = [NSURL fileURLWithPath:temp];
    NSURL* modelPath = [MLModel compileModelAtURL:specPath error:&error];
    checkNSError(error);
    result = [MLModel modelWithContentsOfURL:modelPath error:&error];
    checkNSError(error);
    result = [result retain];  // Safe to retain now that no exceptions possible
  }

  return result;
}


static void handleCVReturn(CVReturn status) {
  if (status != kCVReturnSuccess) {
    std::stringstream msg;
    msg << "Got unexpected return code " << status << " from CoreVideo.";
    log_and_throw(msg.str());
  }
}


static CVPixelBufferRef flex_image_to_CVPixelBuffer(const flex_image image) {
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
  ~impl() {
    [model release];
  }

  std::string name;
  MLModel *model = nil;
  CoreML::Specification::Model spec;
};

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
  m_impl->model = create_model(download_path, model_name);  // retained

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

gl_sarray
mlmodel_image_feature_extractor::extract_features(gl_sarray data) const {
  const neural_network_model_details& model_info = get_model_info(m_impl->name);
  ASSERT_EQ((int)data.dtype(), (int)flex_type_enum::IMAGE);

  std::vector<flexible_type> result(data.size());
  @autoreleasepool {
  NSError* error = nil;
  for(size_t i = 0; i < data.size(); i++) {
    flexible_type decoded_image = image_util::resize_image(data[i], model_info.input_width, model_info.input_height, 3, true);
    const flex_image& image = decoded_image.get<flex_image>();
    CVPixelBufferRef buffer = flex_image_to_CVPixelBuffer(image);

    MLFeatureValue* image_feature = [MLFeatureValue featureValueWithPixelBuffer:buffer];
    NSString* input_name = [NSString stringWithUTF8String: model_info.input_name.c_str()];
    MLDictionaryFeatureProvider *input = [[MLDictionaryFeatureProvider alloc] initWithDictionary:@{input_name: image_feature} error:&error];
    checkNSError(error);
    id<MLFeatureProvider> model_prediction = [m_impl->model predictionFromFeatures:input error:&error];
    checkNSError(error);

    MLFeatureValue* deep_features = [model_prediction featureValueForName: [NSString stringWithUTF8String: model_info.feature_layer_output_name.c_str()]];
    MLMultiArray* deep_features_values = [deep_features multiArrayValue];

    // Santiy check prediction shape
    NSArray<NSNumber *> * shape = [deep_features_values shape];
    ASSERT_EQ(shape.count, (unsigned long)5);
    ASSERT_EQ(shape[0].intValue, 1);
    ASSERT_EQ(shape[1].intValue, 1);
    ASSERT_EQ(shape[2].intValue, model_info.feature_layer_size);
    ASSERT_EQ(shape[3].intValue, 1);
    ASSERT_EQ(shape[4].intValue, 1);

    // Copy deep features to a flexible type vector
    size_t deep_feature_length = shape[2].intValue;
    size_t stride = deep_features_values.strides[2].intValue;
    flex_vec dest(deep_feature_length);
    double *srcPtr = (double *) deep_features_values.dataPointer;
    for(size_t j = 0; j < deep_feature_length; j++) {
      size_t offset = j * stride;
      dest[j] = srcPtr[offset];
    }
    result[i] = dest;

  }

  } // end autoreleasepool

  return gl_sarray(result, flex_type_enum::VECTOR);
}

} // image_deep_feature_extractor
} // namespace turi

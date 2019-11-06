/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/style_transfer/style_transfer.hpp>

#include <map>
#include <random>
#include <string>

#include <core/data/image/image_type.hpp>
#include <model_server/lib/image_util.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/style_transfer/style_transfer_model_definition.hpp>
#include <toolkits/util/training_utils.hpp>

namespace turi {
namespace style_transfer {

using turi::coreml::MLModelWrapper;
using turi::neural_net::compute_context;
using turi::neural_net::float_array_map;
using turi::neural_net::float_scalar;
using turi::neural_net::model_backend;
using turi::neural_net::shared_float_array;

namespace {

constexpr size_t STYLE_TRANSFER_VERSION = 1;

constexpr size_t DEFAULT_HEIGHT = 256;

constexpr size_t DEFAULT_WIDTH = 256;

constexpr size_t DEFAULT_BATCH_SIZE = 1;

const std::map<std::string, std::string>& get_custom_model_naming_map() {
  static const auto* const CUSTOM_MODEL_NAMING_MAP =
      new std::map<std::string, std::string>(
          {{"transformer_conv4_weight", "transformer_decoding_2_conv_weight"},
           {"transformer_instancenorm2_gamma",
            "transformer_encode_3_inst_gamma_weight"},
           {"transformer_residualblock1_instancenorm1_gamma",
            "transformer_residual_2_inst_2_gamma_weight"},
           {"transformer_residualblock4_conv0_weight",
            "transformer_residual_5_conv_1_weight"},
           {"transformer_residualblock2_instancenorm1_gamma",
            "transformer_residual_3_inst_2_gamma_weight"},
           {"transformer_residualblock0_instancenorm0_beta",
            "transformer_residual_1_inst_1_beta_weight"},
           {"transformer_instancenorm0_gamma",
            "transformer_encode_1_inst_gamma_weight"},
           {"transformer_residualblock2_instancenorm0_gamma",
            "transformer_residual_3_inst_1_gamma_weight"},
           {"transformer_residualblock0_conv0_weight",
            "transformer_residual_1_conv_1_weight"},
           {"transformer_residualblock0_conv1_weight",
            "transformer_residual_1_conv_2_weight"},
           {"transformer_residualblock4_instancenorm1_beta",
            "transformer_residual_5_inst_2_beta_weight"},
           {"transformer_conv1_weight", "transformer_encode_2_conv_weight"},
           {"transformer_residualblock3_instancenorm0_gamma",
            "transformer_residual_4_inst_1_gamma_weight"},
           {"transformer_residualblock2_conv1_weight",
            "transformer_residual_3_conv_2_weight"},
           {"transformer_residualblock3_instancenorm0_beta",
            "transformer_residual_4_inst_1_beta_weight"},
           {"transformer_residualblock3_instancenorm1_gamma",
            "transformer_residual_4_inst_2_gamma_weight"},
           {"transformer_residualblock0_instancenorm0_gamma",
            "transformer_residual_1_inst_1_gamma_weight"},
           {"transformer_residualblock1_instancenorm0_beta",
            "transformer_residual_2_inst_1_beta_weight"},
           {"transformer_residualblock1_conv1_weight",
            "transformer_residual_2_conv_2_weight"},
           {"transformer_instancenorm0_beta",
            "transformer_encode_1_inst_beta_weight"},
           {"transformer_instancenorm4_beta",
            "transformer_decoding_1_inst_beta_weight"},
           {"transformer_conv0_weight", "transformer_encode_1_conv_weight"},
           {"transformer_instancenorm1_gamma",
            "transformer_encode_2_inst_gamma_weight"},
           {"transformer_instancenorm3_beta",
            "transformer_decoding_1_inst_beta_weight"},
           {"transformer_conv5_weight", "transformer_conv5_weight"},
           {"transformer_conv2_weight", "transformer_encode_3_conv_weight"},
           {"transformer_instancenorm2_beta",
            "transformer_encode_3_inst_beta_weight"},
           {"transformer_instancenorm3_gamma",
            "transformer_decoding_1_inst_gamma_weight"},
           {"transformer_residualblock3_instancenorm1_beta",
            "transformer_residual_4_inst_2_beta_weight"},
           {"transformer_residualblock0_instancenorm1_gamma",
            "transformer_residual_1_inst_2_gamma_weight"},
           {"transformer_residualblock4_instancenorm0_gamma",
            "transformer_residual_5_inst_1_gamma_weight"},
           {"transformer_residualblock2_instancenorm1_beta",
            "transformer_residual_3_inst_2_beta_weight"},
           {"transformer_residualblock1_conv0_weight",
            "transformer_residual_2_conv_1_weight"},
           {"transformer_instancenorm5_gamma",
            "transformer_decoding_2_inst_gamma_weight"},
           {"transformer_instancenorm1_beta",
            "transformer_encode_2_inst_beta_weight"},
           {"transformer_residualblock3_conv0_weight",
            "transformer_residual_4_conv_1_weight"},
           {"transformer_residualblock4_instancenorm0_beta",
            "transformer_residual_5_inst_1_beta_weight"},
           {"transformer_residualblock1_instancenorm1_beta",
            "transformer_residual_2_inst_2_beta_weight"},
           {"transformer_residualblock0_instancenorm1_beta",
            "transformer_residual_1_inst_2_beta_weight"},
           {"transformer_conv3_weight", "transformer_decoding_1_conv_weight"},
           {"transformer_instancenorm5_beta",
            "transformer_decoding_2_inst_beta_weight"},
           {"transformer_residualblock2_conv0_weight",
            "transformer_residual_3_conv_1_weight"},
           {"transformer_residualblock4_conv1_weight",
            "transformer_residual_5_conv_2_weight"},
           {"transformer_residualblock4_instancenorm1_gamma",
            "transformer_residual_5_inst_2_gamma_weight"},
           {"transformer_residualblock1_instancenorm0_gamma",
            "transformer_residual_2_inst_1_gamma_weight"},
           {"transformer_instancenorm4_gamma",
            "transformer_decoding_1_inst_gamma_weight"},
           {"transformer_residualblock2_instancenorm0_beta",
            "transformer_residual_3_inst_1_beta_weight"},
           {"transformer_residualblock3_conv1_weight",
            "transformer_residual_4_conv_2_weight"}});
  return *CUSTOM_MODEL_NAMING_MAP;
}

float clamp(float v, float low, float high) {
  return (v < low) ? low : (high < v) ? high : v;
}

void prepare_images(const image_type& image,
                    std::vector<float>::iterator start_iter, size_t width,
                    size_t height, size_t channels, size_t index) {
  size_t image_size = height * width * channels;

  image_type resized_image =
      image_util::resize_image(image, width, height, channels, true, 1);

  const unsigned char* resized_image_ptr = resized_image.get_image_data();

  std::transform(resized_image_ptr, resized_image_ptr + image_size, start_iter,
                 [](unsigned char val) { return val / 255.f; });
}

std::vector<std::pair<flex_int, flex_image>> process_output(
    const shared_float_array& contents, size_t index, size_t batch_size,
    size_t width, size_t height) {
  constexpr size_t channels = 3;

  std::vector<std::pair<flex_int, flex_image>> result;
  result.reserve(batch_size);

  size_t image_size = height * width * channels;

  ASSERT_EQ(contents.size(), image_size * batch_size);

  const float* start_ptr = contents.data();

  for (size_t index = 0; index < batch_size; index++) {
    std::vector<uint8_t> image_data;
    image_data.reserve(image_size);

    size_t start_offset = image_size * index;
    size_t end_offset = start_offset + image_size;

    std::transform(start_ptr + start_offset, start_ptr + end_offset,
                   std::back_inserter(image_data), [](float val) {
                     return static_cast<uint8_t>(
                         clamp(std::round(val * 255.f), 0.f, 255.f));
                   });

    image_type img(reinterpret_cast<char*>(image_data.data()), height, width,
                   channels, image_data.size(), IMAGE_TYPE_CURRENT_VERSION,
                   static_cast<int>(Format::RAW_ARRAY));

    result.emplace_back(index, img);
  }

  return result;
}

float_array_map prepare_batch(std::vector<st_example>& batch, size_t width,
                              size_t height, bool train = true) {
  constexpr size_t channels = 3;
  const size_t batch_size = batch.size();

  std::vector<float> content_array(height * width * channels * batch.size());
  std::vector<float> style_array(height * width * channels * batch.size());
  std::vector<float> index_array(batch_size);

  for (size_t index = 0; index < batch_size; index++) {
    size_t offset = index * height * width * channels;

    std::vector<float>::iterator content_iter = content_array.begin() + offset;
    prepare_images(batch[index].content_image, content_iter, width, height,
                   channels, index);

    size_t style_index = batch[index].style_index;
    index_array[index] = style_index;

    if (train) {
      std::vector<float>::iterator style_iter = style_array.begin() + offset;
      prepare_images(batch[index].style_image, style_iter, width, height,
                     channels, index);
    }
  }

  float_array_map map;

  shared_float_array content_wrap = shared_float_array::wrap(
      std::move(content_array), {batch_size, height, width, channels});
  map.emplace("input", content_wrap);

  shared_float_array index_wrap =
      shared_float_array::wrap(std::move(index_array), {batch_size});
  map.emplace("index", index_wrap);

  if (train) {
    shared_float_array style_wrap = shared_float_array::wrap(
        std::move(style_array), {batch_size, height, width, channels});
    map.emplace("labels", style_wrap);
  }

  return map;
}

flex_int estimate_max_iterations(flex_int num_styles, flex_int batch_size) {
  return static_cast<flex_int>(num_styles * 10000.0f / batch_size);
}

}  // namespace

void style_transfer::init_options(
    const std::map<std::string, flexible_type>& opts) {
  options.create_integer_option(
      "batch_size",
      "The number of images to process for each training iteration",
      FLEX_UNDEFINED, 1, std::numeric_limits<int>::max());

  options.create_integer_option(
      "max_iterations",
      "Maximum number of iterations to perform during training", FLEX_UNDEFINED,
      1, std::numeric_limits<int>::max());

  options.create_integer_option(
      "image_width", "The width of the images passed into the network",
      FLEX_UNDEFINED, 1, std::numeric_limits<int>::max());

  options.create_integer_option(
      "image_height", "The height of the images passed into the network",
      FLEX_UNDEFINED, 1, std::numeric_limits<int>::max());

  options.create_integer_option(
      "random_seed",
      "Seed for random weight initialization and sampling during training",
      FLEX_UNDEFINED, std::numeric_limits<int>::min(),
      std::numeric_limits<int>::max());

  options.create_integer_option(
      "num_styles", "The number of styles present in the model", FLEX_UNDEFINED,
      1, std::numeric_limits<int>::max());
  options.create_boolean_option(
      "verbose", "When set to true, verbose is printed", true, true);
  options.create_string_option("content_feature", "Name of the content column", "image", true);
  options.create_string_option("style_feature", "Name of the style column", "image", true);
  options.set_options(opts);

  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

size_t style_transfer::get_version() const { return STYLE_TRANSFER_VERSION; }

void style_transfer::save_impl(oarchive& oarc) const {
  variant_deep_save(state, oarc);
  oarc << m_resnet_spec->export_params_view();
}

void style_transfer::load_version(iarchive& iarc, size_t version) {
  variant_deep_load(state, iarc);
  float_array_map nn_params;
  iarc >> nn_params;

  m_resnet_spec =
      init_resnet(variant_get_value<size_t>(state.at("num_styles")));
  m_resnet_spec->update_params(nn_params);
}

std::unique_ptr<compute_context> style_transfer::create_compute_context()
    const {
  return compute_context::create();
}

std::unique_ptr<data_iterator> style_transfer::create_iterator(
    gl_sarray content, gl_sarray style, bool repeat, bool training,
    int random_seed) const {
  data_iterator::parameters iterator_params;

  iterator_params.style = std::move(style);
  iterator_params.content = std::move(content);
  iterator_params.repeat = repeat;
  iterator_params.mode = (training) ? st_mode::TRAIN : st_mode::PREDICT;
  iterator_params.random_seed = random_seed;

  return create_iterator(iterator_params);
}

std::unique_ptr<data_iterator> style_transfer::create_iterator(
    data_iterator::parameters iterator_params) const {
  return std::unique_ptr<data_iterator>(
      new style_transfer_data_iterator(iterator_params));
}

void style_transfer::infer_derived_options() {

  // Report to the user what GPU(s) is being used.
  std::vector<std::string> gpu_names = m_training_compute_context->gpu_names();
  print_training_device(gpu_names);

  if (read_state<flexible_type>("batch_size") == FLEX_UNDEFINED) {
    add_or_update_state({{"batch_size", DEFAULT_BATCH_SIZE}});
  }

  if (read_state<flexible_type>("max_iterations") == FLEX_UNDEFINED) {
    flex_int max_iterations = estimate_max_iterations(
        read_state<flex_int>("num_styles"), read_state<flex_int>("batch_size"));

    add_or_update_state({{"max_iterations", max_iterations}});
  }

  if (read_state<flexible_type>("image_width") == FLEX_UNDEFINED) {
    add_or_update_state({{"image_width", DEFAULT_WIDTH}});
  }

  if (read_state<flexible_type>("image_height") == FLEX_UNDEFINED) {
    add_or_update_state({{"image_height", DEFAULT_HEIGHT}});
  }

  add_or_update_state({{"training_iterations", 0}});
}

gl_sframe style_transfer::predict(variant_type data,
                                  std::map<std::string, flexible_type> opts) {
  gl_sframe_writer result({"style_idx", "stylized_image"},
                          {flex_type_enum::INTEGER, flex_type_enum::IMAGE}, 1);

  gl_sarray content_images = convert_types_to_sarray(data);

  std::vector<double> style_idx;
  auto style_idx_iter = opts.find("style_idx");
  if (style_idx_iter == opts.end()) {
    flex_int num_styles = read_state<flex_int>("num_styles");

    style_idx.resize(num_styles);
    std::iota(style_idx.begin(), style_idx.end(), 0);
  } else {
    flexible_type flex_style_idx = style_idx_iter->second;
    switch (flex_style_idx.get_type()) {
      case flex_type_enum::INTEGER:
        style_idx.push_back(flex_style_idx.get<flex_int>());
        break;
      case flex_type_enum::VECTOR:
        style_idx = std::move(flex_style_idx.get<flex_vec>());
        break;
      case flex_type_enum::LIST:
      {
        const auto& list = flex_style_idx.get<flex_list>();
        style_idx.resize(list.size());
        std::transform(list.begin(), list.end(), style_idx.begin(),
                       [](flexible_type val) {
                         return static_cast<double>(val.get<flex_float>());
                       });
        break;

      }
      case flex_type_enum::UNDEFINED: {
        flex_int num_styles = read_state<flex_int>("num_styles");
        style_idx.resize(num_styles);
        std::iota(style_idx.begin(), style_idx.end(), 0);
        break;
      }
      default:
        log_and_throw(
            "Option \"style_idx\" has to be of type `Integer` or `List`.");
    }
  }

  perform_predict(content_images, result, style_idx);

  return result.close();
}

void style_transfer::perform_predict(gl_sarray data, gl_sframe_writer& result,
                                     const std::vector<double>& style_idx) {
  if (data.size() == 0) return;

  flex_int batch_size = read_state<flex_int>("batch_size");
  flex_int num_styles = read_state<flex_int>("num_styles");
  flex_int image_width = read_state<flex_int>("image_width");
  flex_int image_height = read_state<flex_int>("image_height");

  // Since we aren't training the style_images are irrelevant
  std::unique_ptr<data_iterator> data_iter =
      create_iterator(data, /* style_sframe */ {}, /* repeat */ false,
                      /* training */ false, num_styles);

  std::unique_ptr<compute_context> ctx = create_compute_context();
  if (ctx == nullptr) {
    log_and_throw("No neural network compute context provided");
  }

  float_array_map weight_params = m_resnet_spec->export_params_view();

  // A value of `0` to indicate prediction
  shared_float_array st_train = shared_float_array::wrap(0.f);
  shared_float_array st_num_styles(std::make_shared<float_scalar>(num_styles));

  std::unique_ptr<model_backend> model = ctx->create_style_transfer(
      {{"st_num_styles", st_num_styles}, {"st_training", st_train}},
      weight_params);

  // looping through all of the style indicies
  for (size_t i : style_idx) {
    std::vector<st_example> batch = data_iter->next_batch(batch_size);
    while (!batch.empty()) {
      // getting actual batch size
      size_t actual_batch_size = batch.size();

      // setting the style index for each batch
      std::for_each(batch.begin(), batch.end(),
                    [i](st_example& example) { example.style_index = i; });

      // prepare batch for prediction
      float_array_map prepared_batch =
          prepare_batch(batch, image_width, image_height, /* train */ false);

      // perform prediction
      float_array_map result_batch = model->predict(prepared_batch);

      // get shared float array from prediction result
      shared_float_array out_shared_float_array = result_batch.at("output");

      // populate gl_sframe_writer
      std::vector<std::pair<flex_int, flex_image>> processed_batch =
          process_output(out_shared_float_array, i, actual_batch_size,
                         image_width, image_height);

      // Write result to gl_sframe_writer
      for (const auto& row : processed_batch) {
        result.write({row.first, row.second}, 0);
      }

      // get next batch
      batch = data_iter->next_batch(batch_size);
    }

    data_iter->reset();
  }
}

gl_sarray style_transfer::convert_types_to_sarray(const variant_type& data) {
  gl_sarray sarray_data;
  if (variant_is<gl_sarray>(data)) {
    sarray_data = variant_get_value<gl_sarray>(data);
    ASSERT_TRUE(sarray_data.dtype() == flex_type_enum::IMAGE);
  } else if (variant_is<flexible_type>(data)) {
    flexible_type image_data = variant_get_value<flexible_type>(data);
    ASSERT_TRUE(image_data.get_type() == flex_type_enum::IMAGE);
    std::vector<flexible_type> image_vector{image_data};
    sarray_data.construct_from_vector(image_vector);
  } else {
    log_and_throw(
        "Invalid data type for predict()! Expect SArray, or flexible_type!");
  }
  return sarray_data;
}

void style_transfer::init_train(gl_sarray style, gl_sarray content,
                                std::map<std::string, flexible_type> opts) {
  auto resnet_mlmodel_path_iter = opts.find("resnet_mlmodel_path");
  if (resnet_mlmodel_path_iter == opts.end()) {
    log_and_throw("Expected option \"resnet_mlmodel_path\" not found.");
  }
  const std::string resnet_mlmodel_path = resnet_mlmodel_path_iter->second;
  opts.erase(resnet_mlmodel_path_iter);

  auto vgg_mlmodel_path_iter = opts.find("vgg_mlmodel_path");
  if (vgg_mlmodel_path_iter == opts.end()) {
    log_and_throw("Expected option \"vgg_mlmodel_path\" not found.");
  }
  const std::string vgg_mlmodel_path = vgg_mlmodel_path_iter->second;
  opts.erase(vgg_mlmodel_path_iter);

  auto num_styles_iter = opts.find("num_styles");
  if (num_styles_iter == opts.end()) {
    log_and_throw("Expected option \"num_styles\" not found.");
  }
  size_t num_styles = num_styles_iter->second;

  init_options(opts);

  if (read_state<flexible_type>("random_seed") == FLEX_UNDEFINED) {
    std::random_device random_device;
    int random_seed = static_cast<int>(random_device());
    add_or_update_state({{"random_seed", random_seed}});
  }

  m_training_data_iterator = create_iterator(content, style, /* repeat */ true,
                                             /* training */ true, num_styles);

  m_training_compute_context = create_compute_context();
  if (m_training_compute_context == nullptr) {
    log_and_throw("No neural network compute context provided");
  }

  infer_derived_options();

  add_or_update_state({{"model", "resnet-16"}});

  m_resnet_spec = init_resnet(resnet_mlmodel_path, num_styles);
  m_vgg_spec = init_vgg_16(vgg_mlmodel_path);

  float_array_map weight_params = m_resnet_spec->export_params_view();
  float_array_map vgg_params = m_vgg_spec->export_params_view();

  weight_params.insert(vgg_params.begin(), vgg_params.end());

  // A value of `1` to indicate training
  shared_float_array st_train = shared_float_array::wrap(1.f);
  shared_float_array st_num_styles(std::make_shared<float_scalar>(num_styles));

  m_training_model = m_training_compute_context->create_style_transfer(
      {{"st_num_styles", st_num_styles}, {"st_training", st_train}},
      weight_params);

  // TODO: print table printer
}

flex_int style_transfer::get_max_iterations() const {
  return read_state<flex_int>("max_iterations");
}

flex_int style_transfer::get_training_iterations() const {
  return read_state<flex_int>("training_iterations");
}

flex_int style_transfer::get_num_classes() const {
  return read_state<flex_int>("num_classes");
}

void style_transfer::iterate_training() {
  ASSERT_TRUE(m_training_data_iterator != nullptr);
  ASSERT_TRUE(m_training_model != nullptr);

  flex_int iteration_idx = get_training_iterations();

  flex_int batch_size = read_state<flex_int>("batch_size");
  flex_int image_width = read_state<flex_int>("image_width");
  flex_int image_height = read_state<flex_int>("image_height");

  std::vector<st_example> batch =
      m_training_data_iterator->next_batch(batch_size);

  float_array_map prepared_batch =
      prepare_batch(batch, image_width, image_height);

  std::map<std::string, shared_float_array> results =
      m_training_model->train(prepared_batch);

  add_or_update_state({
      {"training_iterations", iteration_idx + 1},
  });

  shared_float_array loss_batch = results.at("loss");

  size_t loss_batch_size = loss_batch.size();
  float batch_loss = std::accumulate(
      loss_batch.data(), loss_batch.data() + loss_batch_size, 0.f,
      [loss_batch_size](float a, float b) { return a + b / loss_batch_size; });

  if (training_table_printer_) {
    training_table_printer_->print_progress_row(
        iteration_idx, iteration_idx + 1, batch_loss, progress_time());
  }
}

void style_transfer::finalize_training() {
  float_array_map trained_weights = m_training_model->export_weights();
  m_resnet_spec->update_params(trained_weights);
}

void style_transfer::train(gl_sarray style, gl_sarray content,
                           std::map<std::string, flexible_type> opts) {
  training_table_printer_.reset(new table_printer(
      {{"Iteration", 12}, {"Loss", 12}, {"Elapsed Time", 12}}));

  init_train(style, content, opts);

  training_table_printer_->print_header();

  while (get_training_iterations() < get_max_iterations()) iterate_training();

  finalize_training();

  training_table_printer_->print_footer();
  training_table_printer_.reset();
}

std::shared_ptr<MLModelWrapper> style_transfer::export_to_coreml(
    std::string filename, std::map<std::string, flexible_type> opts) {
  flex_int image_width = read_state<flex_int>("image_width");
  flex_int image_height = read_state<flex_int>("image_height");

  flex_dict user_defined_metadata = {
      {"model", read_state<flex_string>("model")},
      {"max_iterations", read_state<flex_int>("max_iterations")},
      {"training_iterations", read_state<flex_int>("training_iterations")},
      {"type", "StyleTransfer"},
      {"content_feature", read_state<flex_string>("content_feature")},
      {"style_feature", read_state<flex_string>("style_feature")},
      {"num_styles", read_state<flex_string>("num_styles")},
      {"version", get_version()},
  };

  std::shared_ptr<MLModelWrapper> model_wrapper =
      export_style_transfer_model(*m_resnet_spec, image_width, image_height,
                                  std::move(user_defined_metadata));

  if (!filename.empty()) model_wrapper->save(filename);

  return model_wrapper;
}

void style_transfer::import_from_custom_model(variant_map_type model_data,
                                              size_t version) {
  // Get relevant values from variant_map_type
  auto model_iter = model_data.find("_model");
  if (model_iter == model_data.end()) {
    log_and_throw("The loaded turicreate model must contain '_model'!\n");
  }
  const flex_dict& model = variant_get_value<flex_dict>(model_iter->second);
  model_data.erase(model_iter);

  auto num_styles_iter = model_data.find("num_styles");
  if (num_styles_iter == model_data.end()) {
    log_and_throw("The loaded turicreate model must contain 'num_styles'!\n");
  }
  const size_t num_styles =
      variant_get_value<flex_int>(num_styles_iter->second);
  model_data.erase(num_styles_iter);

  auto max_iterations_iter = model_data.find("max_iterations");
  if (max_iterations_iter == model_data.end()) {
    log_and_throw(
        "The loaded turicreate model must contain 'max_iterations'!\n");
  }
  const size_t max_iterations =
      variant_get_value<flex_int>(max_iterations_iter->second);
  model_data.erase(max_iterations_iter);

  auto model_type_iter = model_data.find("model");
  if (model_type_iter == model_data.end()) {
    log_and_throw("The loaded turicreate model must contain 'model_iter'!\n");
  }
  const std::string model_type =
      variant_get_value<flex_string>(model_type_iter->second);
  model_data.erase(model_type_iter);

  add_or_update_state({{"model", model_type},
                       {"num_styles", num_styles},
                       {"max_iterations", max_iterations}});

  // Extract the weights and shapes
  flex_dict mxnet_data_dict;
  flex_dict mxnet_shape_dict;

  for (const auto& data : model) {
    if (data.first == "data") {
      mxnet_data_dict = data.second;
    }
    if (data.first == "shapes") {
      mxnet_shape_dict = data.second;
    }
  }

  auto cmp = [](const flex_dict::value_type& a,
                const flex_dict::value_type& b) { return (a.first < b.first); };

  std::sort(mxnet_data_dict.begin(), mxnet_data_dict.end(), cmp);
  std::sort(mxnet_shape_dict.begin(), mxnet_shape_dict.end(), cmp);

  // Create Map, converting the weight names from MxNet to CoreML
  float_array_map nn_params;
  for (size_t i = 0; i < mxnet_data_dict.size(); i++) {
    const std::string layer_name =
        get_custom_model_naming_map().at(mxnet_data_dict[i].first);
    flex_nd_vec mxnet_data_nd = mxnet_data_dict[i].second.to<flex_nd_vec>();
    flex_nd_vec mxnet_shape_nd = mxnet_shape_dict[i].second.to<flex_nd_vec>();
    const std::vector<double>& model_weight = mxnet_data_nd.elements();
    const std::vector<double>& model_shape = mxnet_shape_nd.elements();
    std::vector<float> layer_weight(model_weight.begin(), model_weight.end());
    std::vector<size_t> layer_shape(model_shape.begin(), model_shape.end());
    nn_params[layer_name] = shared_float_array::wrap(std::move(layer_weight),
                                                     std::move(layer_shape));
  }

  // Update resnet spec with imported weight map
  m_resnet_spec = init_resnet(variant_get_value<size_t>(num_styles));
  m_resnet_spec->update_params(nn_params);
}

}  // namespace style_transfer
}  // namespace turi
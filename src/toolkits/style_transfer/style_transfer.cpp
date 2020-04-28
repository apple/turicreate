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
#include <sstream>
#include <timer/timer.hpp>

#include <core/data/image/image_type.hpp>
#include <model_server/lib/image_util.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/style_transfer/st_resnet16_model_trainer.hpp>
#include <toolkits/style_transfer/style_transfer_model_definition.hpp>
#include <toolkits/util/float_array_serialization.hpp>
#include <toolkits/util/training_utils.hpp>

#ifdef HAS_MPS
#import <ml/neural_net/mps_compute_context.hpp>
#endif  // HAS_MPS

namespace turi {
namespace style_transfer {

using turi::coreml::MLModelWrapper;
using turi::neural_net::compute_context;
using turi::neural_net::float_array_map;
using turi::neural_net::float_scalar;
using turi::neural_net::FuturesStream;
using turi::neural_net::model_backend;
using turi::neural_net::model_spec;
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
            "transformer_decoding_2_inst_beta_weight"},
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
            "transformer_instancenorm5_gamma_weight"},
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
            "transformer_instancenorm5_beta_weight"},
           {"transformer_residualblock2_conv0_weight",
            "transformer_residual_3_conv_1_weight"},
           {"transformer_residualblock4_conv1_weight",
            "transformer_residual_5_conv_2_weight"},
           {"transformer_residualblock4_instancenorm1_gamma",
            "transformer_residual_5_inst_2_gamma_weight"},
           {"transformer_residualblock1_instancenorm0_gamma",
            "transformer_residual_2_inst_1_gamma_weight"},
           {"transformer_instancenorm4_gamma",
            "transformer_decoding_2_inst_gamma_weight"},
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

}  // namespace

std::vector<std::pair<flex_int, flex_image>> process_output(
    const shared_float_array& contents, size_t index) {
  size_t image_dim = contents.dim();

  ASSERT_EQ(image_dim, 4);

  const size_t* content_ptr = contents.shape();

  // Note: the float array from each context's predict is expected to be in the
  // format {batch_size, height, width, channels}.
  size_t batch_size = content_ptr[0];
  size_t height = content_ptr[1];
  size_t width = content_ptr[2];
  size_t channels = content_ptr[3];

  size_t image_size = contents.size() / batch_size;

  std::vector<std::pair<flex_int, flex_image>> result;
  result.reserve(batch_size);

  ASSERT_EQ(contents.size(), image_size * batch_size);

  const float* start_ptr = contents.data();

  for (size_t i = 0; i < batch_size; i++) {
    std::vector<uint8_t> image_data;
    image_data.reserve(image_size);

    size_t start_offset = image_size * i;
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

float_array_map prepare_batch(const std::vector<st_example>& batch,
                              size_t width, size_t height, bool train) {
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

  map["input"] = shared_float_array::wrap(
      std::move(content_array), {batch_size, height, width, channels});
  map["index"] = shared_float_array::wrap(std::move(index_array), {batch_size});

  map["width"] = shared_float_array::wrap(width);
  map["height"] = shared_float_array::wrap(height);

  if (train) {
    map["labels"] = shared_float_array::wrap(
        std::move(style_array), {batch_size, height, width, channels});
  }

  return map;
}

// Preparing Batch for prediction. Since the tensorflow implementation of style
// transfer doesn't have multiple batches supported in predict this function
// takes exactly one st_example as an argument.
float_array_map prepare_predict(const st_example& example) {
  ASSERT_EQ(3, example.content_image.m_channels);

  size_t image_width = example.content_image.m_width;
  size_t image_height = example.content_image.m_height;
  std::vector<st_example> batch = {example};

  return prepare_batch(batch, image_width, image_height,
                       /* train */ false);
}

namespace {

flex_int estimate_max_iterations(flex_int num_styles, flex_int batch_size) {
  return static_cast<flex_int>(num_styles * 10000.0f / batch_size);
}

void check_style_index(int64_t idx, int64_t num_styles) {
  if ((idx < 0) || (idx >= num_styles))
    log_and_throw("Please choose a valid style index.");
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
  options.create_string_option("content_feature", "Name of the content column",
                               "image", true);
  options.create_string_option("style_feature", "Name of the style column",
                               "image", true);
  options.create_string_option(
      /* name              */ "model",
      /* description       */ "Defines the model type",
      /* default_value     */ "resnet-16",
      /* allowed_overwrite */ true);
  options.set_options(opts);

  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

size_t style_transfer::get_version() const { return STYLE_TRANSFER_VERSION; }

void style_transfer::save_impl(oarchive& oarc) const {
  variant_deep_save(state, oarc);
  save_float_array_map(read_checkpoint().weights(), oarc);
}

void style_transfer::load_version(iarchive& iarc, size_t version) {
  variant_deep_load(state, iarc);

  float_array_map nn_params = load_float_array_map(iarc);
  checkpoint_ = load_checkpoint(nn_params);
}

std::unique_ptr<Checkpoint> style_transfer::load_checkpoint(
    float_array_map weights) const {
  if (read_state<flex_string>("model") != "resnet-16") {
    log_and_throw("Cannot load unknown model: " +
                  read_state<flex_string>("model"));
  }

  std::unique_ptr<Checkpoint> checkpoint;
  checkpoint.reset(new ResNet16Checkpoint(get_config(), std::move(weights)));
  return checkpoint;
}

std::unique_ptr<Checkpoint> style_transfer::create_checkpoint(
    Config config, const std::string& resnet_model_path) const {
  if (read_state<flex_string>("model") != "resnet-16") {
    log_and_throw("Cannot initialize unknown model: " +
                  read_state<flex_string>("model"));
  }

  std::unique_ptr<Checkpoint> checkpoint;
  checkpoint.reset(
      new ResNet16Checkpoint(std::move(config), resnet_model_path));
  return checkpoint;
}

Config style_transfer::get_config() const {
  Config config;
  config.num_styles = read_state<int>("num_styles");
  config.max_iterations = read_state<int>("max_iterations");
  config.batch_size = read_state<int>("batch_size");

  auto state_iter = state.find("image_height");
  if (state_iter != state.end()) {
    config.training_image_height = variant_get_value<int>(state_iter->second);
  }
  state_iter = state.find("image_width");
  if (state_iter != state.end()) {
    config.training_image_width = variant_get_value<int>(state_iter->second);
  }

  config.random_seed = read_state<int>("random_seed");

  return config;
}

const Checkpoint& style_transfer::read_checkpoint() const {
  if (checkpoint_ == nullptr) {
    checkpoint_ = std::move(*checkpoint_futures_->Next().get());
  }
  return *checkpoint_.get();
}

std::unique_ptr<compute_context> style_transfer::create_compute_context()
    const {
// Since the tcmps library isn't compiled if the system doesn't have MPS. We
// have an if_def to check for an mps enabled system system. If it is an MPS
// enabled system, then a check for MacOS greater than 10.15 is performed.
// If it is then the Style Transfer MPS implementation is used. On all other
// systems currently the TensorFlow implementation is used.
#ifdef HAS_MPS
  if (neural_net::mps_compute_context::has_style_transfer()) {
    return compute_context::create();
  }
#endif  // HAS_MPS

  return compute_context::create_tf();
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

void style_transfer::infer_derived_options(compute_context* context) {
  context->print_training_device_info();

  if (read_state<flexible_type>("batch_size") == FLEX_UNDEFINED) {
    add_or_update_state({{"batch_size", DEFAULT_BATCH_SIZE}});
  }

  if (read_state<flexible_type>("max_iterations") == FLEX_UNDEFINED) {
    flex_int max_iterations = estimate_max_iterations(
        read_state<flex_int>("num_styles"), read_state<flex_int>("batch_size"));

    logprogress_stream << "Setting max_iterations to be " << max_iterations << std::endl;

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

gl_sframe style_transfer::get_styles(variant_type style_index) {
  gl_sframe style_sf = read_state<gl_sframe>("styles");
  gl_sarray style_filter = convert_style_indices_to_filter(style_index);

  return style_sf[style_filter];
}

gl_sframe style_transfer::style_sframe_with_index(gl_sarray styles) {
  std::vector<flexible_type> index_list;
  flex_int num_styles = read_state<flex_int>("num_styles");
  index_list.resize(num_styles);
  std::iota(index_list.begin(), index_list.end(), 0);

  return gl_sframe({
      {"index", index_list},
      {"style", styles},
  });
}

/**
 * convert_style_indices_to_filter
 *
 * This function takes a variant type and converts it into a boolean filter. The
 * elements at the indices we want to keep are set to a value of `1`, the
 * elements we don't want to keep are set to a value of `0`.
 */
gl_sarray style_transfer::convert_style_indices_to_filter(
    const variant_type& data) {
  // read the `num_styles`
  flex_int num_styles = read_state<flex_int>("num_styles");
  if (variant_is<flex_list>(data) || variant_is<flex_vec>(data)) {
    // if the type is `flex_list` or `flex_vec` create a vector where every
    // element is set to zero
    std::vector<flexible_type> index_filter(num_styles, 0);
    flex_list index_list = variant_get_value<flex_list>(data);
    // Assert that the list is not zero-length
    ASSERT_NE(index_list.size(), 0);

    // populate the indices that are selected by the flex_list
    std::for_each(
        index_list.begin(), index_list.end(),
        [&index_filter, num_styles](flexible_type& ft) {
          // assert if the type is an integer or a float
          ASSERT_TRUE(ft.get_type() == flex_type_enum::INTEGER ||
                      ft.get_type() == flex_type_enum::FLOAT);

          // parse the float or integer value based on the type and,
          // set the value at the indices to 1 indicating the filter
          // to be true.
          switch (ft.get_type()) {
            case flex_type_enum::INTEGER: {
              flex_int idx = ft.get<flex_int>();
              check_style_index(idx, num_styles);
              index_filter[idx] = 1;
            }
            case flex_type_enum::FLOAT: {
              int idx = static_cast<int>(ft.get<flex_float>());
              check_style_index(idx, num_styles);
              index_filter[idx] = 1;
              break;
            }
            default:
              log_and_throw(
                  "Invalid data type! The `style` list should contain either "
                  "integer or float values!");
          }
        });
    return index_filter;
  } else if (variant_is<flex_int>(data)) {
    // Set the index filter to zeros, set to the length of the style sframe
    std::vector<flexible_type> index_filter(num_styles, 0);
    flex_int idx = variant_get_value<flex_int>(data);
    // Check if the index is out of range or not
    check_style_index(idx, num_styles);
    // Set the value at the index to `1`
    index_filter[idx] = 1;
    return index_filter;
  } else if (variant_is<flex_undefined>(data)) {
    // If undefined set the value to all of the styles in the sframe to 1. Or
    // run stylize on all elements of the `SFrame`
    return std::vector<flexible_type>(num_styles, 1);
  } else {
    log_and_throw(
        "Invalid data type! Expect `list`, `integer`, or "
        "`None` types!");
  }
}

gl_sframe style_transfer::predict(variant_type data,
                                  std::map<std::string, flexible_type> opts) {
  gl_sframe_writer result(
      {"row_id", "style", "stylized_image"},
      {flex_type_enum::INTEGER, flex_type_enum::INTEGER, flex_type_enum::IMAGE},
      1);

  gl_sarray content_images = convert_types_to_sarray(data);

  bool verbose;
  auto verbose_iter = opts.find("verbose");
  if (verbose_iter == opts.end()) {
    verbose = false;
  } else {
    verbose = verbose_iter->second;
  }

  std::vector<int> style_idx;

  auto style_idx_iter = opts.find("style_idx");

  if (style_idx_iter == opts.end()) {
    flex_int num_styles = read_state<flex_int>("num_styles");
    style_idx.resize(num_styles);
    std::iota(style_idx.begin(), style_idx.end(), 0);

  } else {
    flexible_type flex_style_idx = style_idx_iter->second;
    switch (flex_style_idx.get_type()) {
      case flex_type_enum::INTEGER:
        style_idx = {flex_style_idx.to<int>()};
        break;
      case flex_type_enum::VECTOR: {
        const auto& v_ref = flex_style_idx.get<flex_vec>();
        style_idx.assign(v_ref.begin(), v_ref.end());
        if (style_idx.empty()) {
          log_and_throw("The `style` parameter can't be an empty list");
        }
        break;
      }
      case flex_type_enum::LIST: {
        const auto& list = flex_style_idx.get<flex_list>();
        if (list.empty()) {
          log_and_throw("The `style` parameter can't be an empty list");
        }

        style_idx.assign(list.begin(), list.end());
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

  perform_predict(content_images, result, style_idx, verbose);

  return result.close();
}

void style_transfer::perform_predict(gl_sarray data, gl_sframe_writer& result,
                                     const std::vector<int>& style_idx,
                                     bool verbose) {
  if (data.size() == 0) return;

  flex_int num_styles = read_state<flex_int>("num_styles");
  for (size_t i : style_idx) {
    // check whether the style indices are valid
    check_style_index(i, num_styles);
  }

  // Since we aren't training the style_images are irrelevant
  std::unique_ptr<data_iterator> data_iter =
      create_iterator(data, /* style_sframe */ {}, /* repeat */ false,
                      /* training */ false, static_cast<int>(num_styles));

  std::unique_ptr<compute_context> ctx = create_compute_context();
  if (ctx == nullptr) {
    log_and_throw("No neural network compute context provided");
  }

  std::shared_ptr<ModelTrainer> trainer = model_trainer_;
  if (!trainer) {
    trainer = read_checkpoint().CreateModelTrainer();
  }
  std::shared_ptr<FuturesStream<DataBatch>> inference_futures =
      trainer
          ->AsInferenceBatchPublisher(std::move(data_iter), style_idx,
                                      ctx.get())
          ->AsFutures();

  // Style Printer
  // record time of process
  size_t idx = 0;
  table_printer table(
      {{"Images Processed", 0}, {"Elapsed Time", 0}, {"Percent Complete", 0}},
      0);
  if (verbose) {
    table.print_header();
  }

  // looping through all of the data
  std::unique_ptr<DataBatch> batch = inference_futures->Next().get();
  while (batch) {
    // Write result to gl_sframe_writer
    for (const auto& row : batch->examples) {
      result.write({batch->iteration_id - 1, row.style_index, row.style_image},
                   0);
    }

    // progress printing for stylization
    idx++;
    if (verbose) {
      std::ostringstream formatted_percentage;
      formatted_percentage.precision(2);
      formatted_percentage << std::fixed
                           << (idx * 100.0 / (data.size() * style_idx.size()));
      formatted_percentage << "%";
      table.print_progress_row(idx, idx, progress_time(),
                               formatted_percentage.str());
    }

    // get next batch and increase the row_idx
    batch = inference_futures->Next().get();
  }

  if (verbose) {
    table.print_footer();
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

void style_transfer::init_training(gl_sarray style, gl_sarray content,
                                   std::map<std::string, flexible_type> opts) {
  auto pretrained_weights_iter = opts.find("pretrained_weights");
  bool pretrained_weights = false;
  if (pretrained_weights_iter != opts.end()) {
    pretrained_weights = pretrained_weights_iter->second;
  }
  opts.erase(pretrained_weights_iter);

  std::string resnet_mlmodel_path;
  auto resnet_mlmodel_path_iter = opts.find("resnet_mlmodel_path");
  if (pretrained_weights) {
    if (resnet_mlmodel_path_iter == opts.end()) {
      log_and_throw("Expected option \"resnet_mlmodel_path\" not found.");
    }
    resnet_mlmodel_path = resnet_mlmodel_path_iter->second.to<flex_string>();
  }
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

  init_options(opts);

  if (read_state<flexible_type>("random_seed") == FLEX_UNDEFINED) {
    std::random_device random_device;
    int random_seed = static_cast<int>(random_device());
    add_or_update_state({{"random_seed", random_seed}});
  }

  std::unique_ptr<compute_context> training_compute_context =
      create_compute_context();
  if (training_compute_context == nullptr) {
    log_and_throw("No neural network compute context provided");
  }

  infer_derived_options(training_compute_context.get());

  add_or_update_state({{"styles", style_sframe_with_index(style)},
                       {"num_content_images", content.size()}});

  checkpoint_ = create_checkpoint(get_config(), resnet_mlmodel_path);

  connect_trainer(std::move(style), std::move(content), vgg_mlmodel_path,
                  std::move(training_compute_context));
}

void style_transfer::resume_training(
    gl_sarray style, gl_sarray content,
    std::map<std::string, flexible_type> opts) {
  auto vgg_mlmodel_path_iter = opts.find("vgg_mlmodel_path");
  if (vgg_mlmodel_path_iter == opts.end()) {
    log_and_throw("Expected option \"vgg_mlmodel_path\" not found.");
  }
  const std::string vgg_mlmodel_path = vgg_mlmodel_path_iter->second;

  std::unique_ptr<compute_context> training_compute_context =
      create_compute_context();
  if (training_compute_context == nullptr) {
    log_and_throw("No neural network compute context provided");
  }

  connect_trainer(std::move(style), std::move(content), vgg_mlmodel_path,
                  std::move(training_compute_context));
}

void style_transfer::connect_trainer(gl_sarray style, gl_sarray content,
                                     const std::string& vgg_mlmodel_path,
                                     std::unique_ptr<compute_context> context) {
  model_trainer_ = read_checkpoint().CreateModelTrainer();

  // TODO: Iterator needs to support resuming from an offset.
  std::unique_ptr<data_iterator> training_data =
      create_iterator(content, style, /* repeat */ true,
                      /* training */ true, read_state<int>("num_styles"));

  int iteration_offset = read_state<int>("training_iterations");
  std::unique_ptr<float> initial_training_loss;
  auto loss_it = state.find("training_loss");
  if (loss_it != state.end()) {
    initial_training_loss.reset(
        new float(variant_get_value<float>(loss_it->second)));
  }

  training_futures_ =
      model_trainer_
          ->AsTrainingBatchPublisher(
              std::move(training_data), vgg_mlmodel_path, iteration_offset,
              std::move(initial_training_loss), context.get())
          ->AsFutures();

  checkpoint_futures_ = model_trainer_->AsCheckpointPublisher()->AsFutures();
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
  ASSERT_TRUE(model_trainer_ != nullptr);
  ASSERT_TRUE(training_futures_ != nullptr);

  // If we have a local checkpoint, it will no longer be valid.
  checkpoint_.reset();

  std::future<std::unique_ptr<TrainingProgress>> training_batch =
      training_futures_->Next();
  std::unique_ptr<TrainingProgress> progress = training_batch.get();
  ASSERT_TRUE(progress != nullptr);

  add_or_update_state({
      {"training_iterations", progress->iteration_id},
      {"training_loss", progress->smoothed_loss}
  });

  if (model_trainer_->SupportsLossComponents()) {
    add_or_update_state({
      {"training_style_loss", progress->style_loss},
      {"training_content_loss", progress->content_loss}
    });
  }

  if (training_table_printer_) {
    if (model_trainer_->SupportsLossComponents()) {
      training_table_printer_->print_progress_row(
          progress->iteration_id, progress->iteration_id,
          progress->smoothed_loss, progress->style_loss, progress->content_loss,
          progress_time());
    } else {
      training_table_printer_->print_progress_row(
          progress->iteration_id, progress->iteration_id,
          progress->smoothed_loss, progress_time());
    }
  }
}

// This may become necessary once we begin pipelining batches to GPU.
void style_transfer::synchronize_training() {}

void style_transfer::finalize_training() {
  ASSERT_TRUE(model_trainer_ != nullptr);
  ASSERT_TRUE(training_futures_ != nullptr);

  // Copy out the trained model while we still have access to a backend.
  read_checkpoint();

  model_trainer_.reset();
  training_futures_.reset();
  checkpoint_futures_.reset();
}

void style_transfer::train(gl_sarray style, gl_sarray content,
                           std::map<std::string, flexible_type> opts) {

  turi::timer time_object;
  time_object.start();

  init_training(style, content, opts);

  training_table_printer_.reset();
  if (read_state<bool>("verbose")) {
    std::vector<std::pair<std::string, size_t>> table_header_format;
    if (model_trainer_->SupportsLossComponents()) {
      table_header_format = {{"Iteration", 12},
                             {"Total Loss", 12},
                             {"Style Loss", 12},
                             {"Content Loss", 12},
                             {"Elapsed Time", 12}};
    } else {
      table_header_format = {
          {"Iteration", 12}, {"Loss", 12}, {"Elapsed Time", 12}};
    }
    training_table_printer_.reset(new table_printer(table_header_format));
    training_table_printer_->print_header();
  }

  while (get_training_iterations() < get_max_iterations()) iterate_training();

  finalize_training();

  if (training_table_printer_) {
    training_table_printer_->print_footer();
    training_table_printer_.reset();
  }

  // Using training_epochs * data_size = training_iterations * batch_size
  size_t training_epochs = ((read_state<flex_int>("batch_size") * read_state<flex_int>("training_iterations")) / read_state<flex_int>("num_content_images"));
  double current_time = time_object.current_time();

  std::stringstream ss;
  table_internal::_format_time(ss, current_time);

  add_or_update_state({
    {"training_epochs", training_epochs},
    {"training_time", current_time},
    {"_training_time_as_string", ss.str()}
  });
}

std::shared_ptr<MLModelWrapper> style_transfer::export_to_coreml(
    std::string filename, std::string short_desc,
    std::map<std::string, flexible_type> additional_user_defined,
    std::map<std::string, flexible_type> opts) {

  const flex_int image_width = read_opts<flex_int>(opts, "image_width");
  const flex_int image_height = read_opts<flex_int>(opts, "image_height");
  const flex_int include_flexible_shape =
      read_opts<flex_int>(opts, "include_flexible_shape");
  const flex_string content_feature =
      read_state<flex_string>("content_feature");
  const flex_string style_feature = read_state<flex_string>("style_feature");
  const flex_int num_styles = read_state<flex_int>("num_styles");

  flex_dict user_defined_metadata = {
      {"model", read_state<flex_string>("model")},
      {"max_iterations", read_state<flex_int>("max_iterations")},
      {"training_iterations", read_state<flex_int>("training_iterations")},
      {"type", "style_transfer"},
      {"content_feature", content_feature},  // TODO: refactor to take content name and style name
      {"style_feature", style_feature},
      {"num_styles", num_styles},
      {"version", get_version()},
  };
  for(const auto& kvp : additional_user_defined) {
       user_defined_metadata.emplace_back(kvp.first, kvp.second);
  }

  model_spec nn_spec = read_checkpoint().ExportToCoreML();
  std::shared_ptr<MLModelWrapper> model_wrapper = export_style_transfer_model(
      nn_spec, image_width, image_height, include_flexible_shape,
      content_feature, style_feature, num_styles);

  model_wrapper->add_metadata({
      {"user_defined", std::move(user_defined_metadata)},
      {"short_description", short_desc}
  });

  if (!filename.empty()) {
    model_wrapper->save(filename);
  }
  return model_wrapper;
}

void style_transfer::import_from_custom_model(variant_map_type model_data,
                                              size_t version) {
  // Get relevant values from variant_map_type
  const flex_dict& model = read_opts<flex_dict>(model_data, "_model");
  const flex_int num_styles = read_opts<flex_int>(model_data, "num_styles");
  const flex_int max_iterations =
      read_opts<flex_int>(model_data, "max_iterations");
  const flex_string model_type = read_opts<flex_string>(model_data, "model");
  const flex_string _training_time_as_string = read_opts<flex_string>(model_data, "_training_time_as_string");
  const flex_int training_epochs = read_opts<flex_int>(model_data, "training_epochs");
  const flex_int training_iterations = read_opts<flex_int>(model_data, "training_iterations");
  const flex_int num_content_images = read_opts<flex_int>(model_data, "num_content_images");
  const flex_float training_loss = read_opts<flex_float>(model_data, "training_loss");

  add_or_update_state({{"model", model_type},
                       {"num_styles", num_styles},
                       {"max_iterations", max_iterations},
                       {"batch_size", DEFAULT_BATCH_SIZE},
                       {"_training_time_as_string", _training_time_as_string},
                       {"training_epochs", training_epochs},
                       {"training_iterations", training_iterations},
                       {"num_content_images", num_content_images},
                       {"training_loss", training_loss}});

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
  checkpoint_ = load_checkpoint(std::move(nn_params));
}

}  // namespace style_transfer
}  // namespace turi

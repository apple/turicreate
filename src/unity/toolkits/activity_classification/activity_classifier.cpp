/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/activity_classification/activity_classifier.hpp>

#include <functional>
#include <numeric>

#include <unity/toolkits/coreml_export/neural_net_models_exporter.hpp>
#include <util/string_util.hpp>

namespace turi {
namespace activity_classification {

namespace {

using coreml::MLModelWrapper;
using neural_net::compute_context;
using neural_net::float_array_map;
using neural_net::model_spec;
using neural_net::lstm_weight_initializers;
using neural_net::shared_float_array;
using neural_net::xavier_weight_initializer;
using neural_net::zero_weight_initializer;

using padding_type = model_spec::padding_type;

constexpr size_t NUM_PREDICTIONS_PER_CHUNK = 20;

// TODO: Encapsulate these network architecture parameters better, to support
// alternative architectures.
constexpr size_t NUM_CONV_FILTERS = 64;
constexpr size_t LSTM_HIDDEN_SIZE = 200;
constexpr size_t FULLY_CONNECTED_HIDDEN_SIZE = 128;
constexpr float LSTM_CELL_CLIP_THRESHOLD = 50000.f;

// These are the fixed values that the Python implementation currently passes
// into TCMPS.
// TODO: These should be exposed in a way that facilitates experimentation.
// TODO: A struct instead of a map would be nice, too.
float_array_map get_training_config(size_t prediction_window) {
  return {
    { "ac_pred_window", shared_float_array::wrap(prediction_window) },
    { "ac_seq_len", shared_float_array::wrap(NUM_PREDICTIONS_PER_CHUNK) },
    { "mode", shared_float_array::wrap(0.f) },  // kLowLevelModeTrain
  };
}

}  // namespace

void activity_classifier::init_options(
    const std::map<std::string, flexible_type>& opts)
{
  // Define options.
  options.create_integer_option(
      "prediction_window",
      "Number of time units between predictions. For example, if your input"
      " data is sampled at 100Hz, and the `prediction_window` is set to 100,"
      " then this model will make a prediction every 1 second.",
      100,
      1,
      std::numeric_limits<int>::max());
  options.create_integer_option(
      "batch_size",
      "Number of sequence chunks used per training setp",
      32,
      1,
      std::numeric_limits<int>::max());
  options.create_integer_option(
      "max_iterations",
      "Maximum number of iterations/epochs made over the data during the"
      " training phase",
      10,
      1,
      std::numeric_limits<int>::max());

  // Validate user-provided options.
  options.set_options(opts);

  // Write model fields.
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

void activity_classifier::train(gl_sframe data, std::string target_column_name,
                                std::string session_id_column_name,
                                variant_type validation_data,
                                std::map<std::string, flexible_type> opts)
{
  // Begin printing progress.
  // TODO: Make progress printing optional.
  // TODO: Report accuracy.
  // TODO: Support validation set.
  training_table_printer_.reset(new table_printer(
      { {"Iteration", 12}, {"Train Loss", 12}, {"Elapsed Time", 12} }));

  // Instantiate the training dependencies: data iterator, compute context,
  // backend NN model.
  init_train(std::move(data), std::move(target_column_name),
             std::move(session_id_column_name), std::move(validation_data),
             std::move(opts));

  // Perform all the iterations at once.
  flex_int max_iterations = read_state<flex_int>("max_iterations");
  while (read_state<flex_int>("training_iterations") < max_iterations) {
    perform_training_iteration();
  }

  // Finish printing progress.
  training_table_printer_->print_footer();
  training_table_printer_.reset();

  // Sync trained weights to our local storage of the NN weights.
  float_array_map trained_weights = training_model_->export_weights();
  nn_spec_->update_params(trained_weights);
}

std::shared_ptr<MLModelWrapper> activity_classifier::export_to_coreml(
    std::string filename)
{
  std::shared_ptr<MLModelWrapper> model_wrapper =
      export_activity_classifier_model(
          *nn_spec_,
          read_state<flex_int>("prediction_window"),
          read_state<flex_list>("features"),
          LSTM_HIDDEN_SIZE,
          read_state<flex_list>("classes"),
          read_state<flex_string>("target"));

  // Add "user-defined" metadata.
  // TODO: Should we also be adding the non-user-defined keys, such as
  // "turicreate_version" and "shortDescription", or is that up to the frontend?
  const flex_list& features_list = read_state<flex_list>("features");
  const flex_string features_string =
      join(std::vector<std::string>(features_list.begin(),
                                    features_list.end()    ), ",");
  flex_dict user_defined_metadata = {
      {"features", features_string},
      {"max_iterations", read_state<flex_int>("max_iterations")},
      {"prediction_window", read_state<flex_int>("prediction_window")},
      {"session_id", read_state<flex_string>("session_id")},
      {"target", read_state<flex_string>("target")},
      {"type", "activity_classifier"},
      {"version", 2},
  };
  model_wrapper->add_metadata({
      {"user_defined", std::move(user_defined_metadata)}
  });

  if (!filename.empty()) {
    model_wrapper->save(filename);
  }

  return model_wrapper;
}

std::unique_ptr<data_iterator> activity_classifier::create_iterator(
    const data_iterator::parameters& params) const
{
  return std::unique_ptr<data_iterator>(new simple_data_iterator(params));
}

std::unique_ptr<compute_context>
activity_classifier::create_compute_context() const
{
  return compute_context::create();
}

std::unique_ptr<model_spec> activity_classifier::init_model() const
{
  std::unique_ptr<model_spec> result(new model_spec);

  size_t num_classes = read_state<flex_int>("num_classes");
  size_t num_features = read_state<flex_int>("num_features");
  size_t prediction_window = read_state<flex_int>("prediction_window");

  result->add_permute("permute", "features", {{0, 3, 1, 2}});
  result->add_convolution(
      /* name */                "conv",
      /* input */               "permute",
      /* num_output_channels */ NUM_CONV_FILTERS,
      /* num_kernel_channels */ num_features,
      /* kernel_height */       1,
      /* kernel_width */        prediction_window,
      /* stride_height */       1,
      /* stride_width */        prediction_window,
      /* padding */             padding_type::VALID,
      /* weight_init_fn */      xavier_weight_initializer(
          num_features * prediction_window,
          NUM_CONV_FILTERS * prediction_window),
      /* bias_init_fn */        zero_weight_initializer());
  result->add_relu("relu1", "conv");
  result->add_lstm(
      /* name */                "lstm",
      /* input */               "relu1",
      /* hidden_input */        "hiddenIn",
      /* cell_input */          "cellIn",
      /* hidden_output */       "hiddenOut",
      /* cell_output */         "cellOut",
      /* input_vector_size */   NUM_CONV_FILTERS,
      /* output_vector_size */  LSTM_HIDDEN_SIZE,
      /* cell_clip_threshold */ LSTM_CELL_CLIP_THRESHOLD,
      /* initializers */  lstm_weight_initializers::create_with_xavier_method(
          NUM_CONV_FILTERS, LSTM_HIDDEN_SIZE));
  result->add_inner_product(
      /* name */                "dense0",
      /* input */               "lstm",
      /* num_output_channels */ FULLY_CONNECTED_HIDDEN_SIZE,
      /* num_input_channels */  LSTM_HIDDEN_SIZE,
      /* weight_init_fn */      xavier_weight_initializer(
          LSTM_HIDDEN_SIZE, FULLY_CONNECTED_HIDDEN_SIZE),
      /* bias_init_fn */        zero_weight_initializer());
  result->add_batchnorm("bn", "dense0", FULLY_CONNECTED_HIDDEN_SIZE, 0.001f);
  result->add_relu("relu6", "bn");
  result->add_inner_product(
      /* name */                "dense1",
      /* input */               "relu6",
      /* num_output_channels */ num_classes,
      /* num_input_channels */  FULLY_CONNECTED_HIDDEN_SIZE,
      /* weight_init_fn */      xavier_weight_initializer(
          FULLY_CONNECTED_HIDDEN_SIZE, num_classes),
      /* bias_init_fn */        zero_weight_initializer());
  result->add_softmax("activityProbability", "dense1");

  return result;
}

void activity_classifier::init_train(
    gl_sframe data, std::string target_column_name,
    std::string session_id_column_name, variant_type validation_data,
    std::map<std::string, flexible_type> opts)
{
  // Extract feature names from options.
  std::vector<std::string> feature_column_names;
  auto features_it = opts.find("features");
  if (features_it != opts.end()) {
    for (const flexible_type& feature : features_it->second.to<flex_list>()) {
      feature_column_names.push_back(feature.to<std::string>());
    }

    // Don't pass "features" to init_options, which doesn't recognize it.
    opts.erase(features_it);
  }
  init_options(opts);

  // Bind the data to a data iterator.
  data_iterator::parameters data_params;
  data_params.data = data;
  data_params.target_column_name = target_column_name;
  data_params.session_id_column_name = session_id_column_name;
  data_params.feature_column_names = feature_column_names;
  data_params.prediction_window = read_state<flex_int>("prediction_window");
  data_params.predictions_in_chunk = NUM_PREDICTIONS_PER_CHUNK;
  training_data_iterator_ = create_iterator(data_params);

  // Instantiate the compute context.
  training_compute_context_ = create_compute_context();
  if (training_compute_context_ == nullptr) {
    log_and_throw("No neural network compute context provided");
  }

  // Report to the user what GPU(s) is being used.
  std::vector<std::string> gpu_names = training_compute_context_->gpu_names();
  if (gpu_names.empty()) {
    logprogress_stream << "Using CPU to create model";
  } else {
    std::string gpu_names_string = gpu_names[0];
    for (size_t i = 1; i < gpu_names.size(); ++i) {
      gpu_names_string += ", " + gpu_names[i];
    }
    logprogress_stream << "Using "
                       << (gpu_names.size() > 1 ? "GPUs" : "GPU")
                       << " to create model ("
                       << gpu_names_string << ")";
  }

  // Set additional model fields.
  add_or_update_state({
      { "classes", training_data_iterator_->class_labels() },
      { "features", training_data_iterator_->feature_names() },
      { "num_classes", training_data_iterator_->class_labels().size() },
      { "num_features", training_data_iterator_->feature_names().size() },
      { "session_id", session_id_column_name },
      { "target", target_column_name },
      { "training_iterations", 0 },
  });

  // Initialize the neural net. Note that this depends on statistics computed by
  // the data iterator.
  nn_spec_ = init_model();

  // Instantiate the NN backend.
  size_t samples_per_chunk =
      read_state<flex_int>("prediction_window") * NUM_PREDICTIONS_PER_CHUNK;
  training_model_ = training_compute_context_->create_activity_classifier(
      /* n */     read_state<flex_int>("batch_size"),
      /* c_in */  read_state<flex_int>("num_features"),
      /* h_in */  1,
      /* w_in */  samples_per_chunk,
      /* c_out */ read_state<flex_int>("num_classes"),
      /* h_out */ 1,
      /* w_out */ NUM_PREDICTIONS_PER_CHUNK,
      get_training_config(read_state<flex_int>("prediction_window")),
      nn_spec_->export_params_view());

  // Print the header last, after any logging triggered by initialization above.
  if (training_table_printer_) {
    training_table_printer_->print_header();
  }
}

void activity_classifier::perform_training_iteration() {

  // Training must have been initialized.
  ASSERT_TRUE(training_data_iterator_ != nullptr);
  ASSERT_TRUE(training_model_ != nullptr);

  const size_t batch_size = read_state<flex_int>("batch_size");
  const size_t iteration_idx = read_state<flex_int>("training_iterations");

  float cumulative_batch_loss = 0.f;
  size_t num_batches = 0;
  data_iterator::batch batch;
  do {

    batch = training_data_iterator_->next_batch(batch_size);

    // Submit the batch to the neural net model.
    std::map<std::string, shared_float_array> results = training_model_->train(
        { { "input",   batch.features },
          { "labels",  batch.labels   },
          { "weights", batch.weights  }, });
    shared_float_array loss_batch = results.at("loss");

    float batch_loss = std::accumulate(loss_batch.data(),
                                       loss_batch.data() + loss_batch.size(),
                                       0.f, std::plus<float>());
    cumulative_batch_loss += batch_loss / batch.size;

    ++num_batches;

  } while (batch.size == batch_size);

  float average_batch_loss = cumulative_batch_loss / num_batches;

  // Report progress if we have an active table printer.
  if (training_table_printer_) {
    training_table_printer_->print_progress_row(
        iteration_idx, iteration_idx + 1, average_batch_loss, progress_time());
  }

  add_or_update_state({
      { "training_iterations", iteration_idx + 1 },
      { "training_log_loss", average_batch_loss },
  });

  training_data_iterator_->reset();
}

}  // namespace activity_classification
}  // namespace turi

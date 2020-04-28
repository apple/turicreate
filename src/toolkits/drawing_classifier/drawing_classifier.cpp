/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <iostream>
#include <memory>
#include <random>
#include <sstream>

#include <core/logging/assertions.hpp>
#include <core/logging/logger.hpp>
#include <core/util/string_util.hpp>
#include <ml/neural_net/compute_context.hpp>
#include <ml/neural_net/model_backend.hpp>
#include <ml/neural_net/model_spec.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/coreml_export/neural_net_models_exporter.hpp>
#include <toolkits/evaluation/metrics.hpp>
#include <toolkits/supervised_learning/automatic_model_creation.hpp>
#include <toolkits/util/float_array_serialization.hpp>
#include <toolkits/util/training_utils.hpp>

#include <toolkits/drawing_classifier/data_preparation.hpp>
#include <toolkits/drawing_classifier/drawing_classifier.hpp>


namespace turi {
namespace drawing_classifier {

namespace {

using coreml::MLModelWrapper;
using neural_net::compute_context;
using neural_net::float_array_map;
using neural_net::model_backend;
using neural_net::model_spec;
using neural_net::shared_float_array;
using neural_net::weight_initializer;
using neural_net::xavier_weight_initializer;
using neural_net::zero_weight_initializer;

using padding_type = model_spec::padding_type;
// anonymous helper sections

struct result {
  shared_float_array loss_info;
  shared_float_array accuracy_info;
  shared_float_array output_info;
  data_iterator::batch data_info;
};

size_t count_correct_predictions(const result& batch, size_t num_classes) {
  float num_correct = 0.f;

  const float* output_ptr = batch.output_info.data();
  const float* truth_ptr = batch.data_info.targets.data();

  for (size_t i = 0; i < batch.data_info.num_samples; ++i) {
    size_t prediction =
        std::max_element(output_ptr, output_ptr + num_classes) - output_ptr;
    if (prediction == *truth_ptr) {
      num_correct += 1;
    }

    truth_ptr++;
    output_ptr += num_classes;
  }
  return num_correct;
}

}  // namespace

const size_t drawing_classifier::DRAWING_CLASSIFIER_VERSION = 1;

size_t drawing_classifier::get_version() const {
  return DRAWING_CLASSIFIER_VERSION;
}

void drawing_classifier::save_impl(oarchive& oarc) const {
  if (!nn_spec_)
    log_and_throw(
        "model spec is not initalized, please call `init_train` before saving model");

  // Save model attributes.
  variant_deep_save(state, oarc);

  // Save neural net weights.
  save_float_array_map(nn_spec_->export_params_view(), oarc);
}

void drawing_classifier::load_version(iarchive& iarc, size_t version) {

  // Load model attributes.
  variant_deep_load(state, iarc);

  // Load neural net weights.
  float_array_map nn_params = load_float_array_map(iarc);

  nn_spec_ = init_model(false);
  nn_spec_->update_params(nn_params);
}

std::unique_ptr<model_spec> drawing_classifier::init_model(bool use_random_init) const {
  std::unique_ptr<model_spec> result(new model_spec);

  // state is updated through init_train
  flex_string target = read_state<flex_string>("target");
  size_t num_classes = read_state<flex_int>("num_classes");

  std::mt19937 random_engine;

  if (use_random_init) {
    std::seed_seq seed_seq{read_state<int>("random_seed")};
    random_engine = std::mt19937(seed_seq);
  }

  weight_initializer initializer = zero_weight_initializer();

  // feature columns names
  const flex_string& feature_column_name = read_state<flex_string>("feature");

  const std::string prefix{"drawing"};
  // add suffix when needed.
  const std::string _suffix{""};
  std::string input_name{feature_column_name};
  std::string output_name;

  result->add_preprocessing(feature_column_name, 1 / 255.f);

  {
    size_t channels_filter = 16;
    size_t channels_kernel = 1;
    std::stringstream ss;

    for (size_t ii = 0; ii < 3; ii++) {
      if (ii) {
        input_name = std::move(output_name);
      }

      ss.str("");
      ss << prefix << "_conv" << ii << _suffix;
      output_name = ss.str();

      if (use_random_init) {
        initializer = xavier_weight_initializer(
            /* #input_neurons   */ channels_kernel * 3 * 3,
            /* #output_neurons  */ channels_filter * 3 * 3, &random_engine);
      }

      result->add_convolution(
          /* name                */ output_name,
          /* input               */ input_name,
          /* num_output_channels */ channels_filter,
          /* num_kernel_channels */ channels_kernel,
          /* kernel_height       */ 3,
          /* kernel_width        */ 3,
          /* stride_height       */ 1,
          /* stride_width        */ 1,
          /* padding             */ padding_type::SAME,
          /* weight_init_fn      */ initializer,
          /* bias_init_fn        */ zero_weight_initializer());

      channels_kernel = channels_filter;
      channels_filter *= 2;

      input_name = std::move(output_name);
      ss.str("");
      ss << prefix << "_relu" << ii << _suffix;
      output_name = ss.str();

      result->add_relu(output_name, input_name);

      input_name = std::move(output_name);
      ss.str("");
      ss << prefix << "_pool" << ii << _suffix;
      output_name = ss.str();

      result->add_pooling(
          /* name                 */ output_name,
          /* input                */ input_name,
          /* kernel_height        */ 2,
          /* kernel_width         */ 2,
          /* stride_height        */ 2,
          /* stride_width         */ 2,
          /* padding              */ padding_type::VALID,
          /* avg excluded padding */ false);
    }
  }

  input_name = std::move(output_name);
  output_name = prefix + "_flatten0" + _suffix;

  result->add_flatten(output_name, input_name);

  input_name = std::move(output_name);
  output_name = prefix + "_dense0" + _suffix;

  if (use_random_init) {
    initializer = xavier_weight_initializer(
        /* fan_in    */ 64 * 3 * 3,
        /* fan_out   */ 128, &random_engine);
  }

  result->add_inner_product(
      /* name                */ output_name,
      /* input               */ input_name,
      /* num_output_channels */ 128,
      /* num_input_channels  */ 64 * 3 * 3,
      /* weight_init_fn      */ initializer,
      /* bias_init_fn        */ zero_weight_initializer());

  input_name = std::move(output_name);
  output_name = prefix + "_dense0_relu" + _suffix;

  result->add_relu(output_name, input_name);

  input_name = std::move(output_name);
  output_name = prefix + "_dense1" + _suffix;

  if (use_random_init) {
    initializer = xavier_weight_initializer(
        /* fan_in    */ 128,
        /* fan_out   */ num_classes, &random_engine);
  }

  result->add_inner_product(
      /* name                */ output_name,
      /* input               */ input_name,
      /* num_output_channels */ num_classes,
      /* num_input_channels  */ 128,
      /* weight_init_fn      */ initializer,
      /* bias_init_fn        */ zero_weight_initializer());

  input_name = std::move(output_name);

  result->add_softmax(target + "Probability", input_name);

  return result;
}

void drawing_classifier::init_options(
    const std::map<std::string, flexible_type> &opts) {

  // Define options.

  options.create_integer_option(
      "batch_size", "Number of training examples used per training step", 256,
      1, std::numeric_limits<int>::max());

  options.create_integer_option(
      "max_iterations",
      "Maximum number of iterations/epochs made over the data during the"
      " training phase",
      500,
      1,
      std::numeric_limits<int>::max());

  options.create_integer_option(
      "random_seed",
      "Seed for random weight initialization and sampling during training",
      FLEX_UNDEFINED,
      std::numeric_limits<int>::min(), std::numeric_limits<int>::max());

  options.create_string_option(
      "warm_start",
      "Record warm start model version used. If no warmstart used, "
      "'None' is assigned by default.",
      FLEX_UNDEFINED,
      true);

  options.create_boolean_option(
      "verbose",
      "If True, print progress updates and model details.",
      true,
      true);

  // Validate user-provided options.
  options.set_options(opts);

  // Write model fields.
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

std::tuple<gl_sframe, gl_sframe> drawing_classifier::init_data(
    gl_sframe data, variant_type validation_data) const {
  return turi::supervised::create_validation_data(
      data, validation_data, read_state<int>("random_seed"));
}

std::unique_ptr<data_iterator> drawing_classifier::create_iterator(
    data_iterator::parameters iterator_params) const {
  return std::unique_ptr<data_iterator>(
      new simple_data_iterator(iterator_params));
}

std::unique_ptr<data_iterator> drawing_classifier::create_iterator(
    gl_sframe data, bool is_train,
    flex_list class_labels) const {
  data_iterator::parameters data_params;

  std::string feature_column_name = read_state<flex_string>("feature");
  if (data[feature_column_name].dtype() != flex_type_enum::IMAGE) {
    data = _drawing_classifier_prepare_data(data, feature_column_name);
  }

  data_params.data = std::move(data);

  if (!is_train) {
    data_params.class_labels = std::move(class_labels);
  }

  data_params.is_train = is_train;
  if (data.contains_column(read_state<flex_string>("target"))) {
    data_params.target_column_name = read_state<flex_string>("target");
  }

  data_params.feature_column_name = read_state<flex_string>("feature");

  return create_iterator(data_params);
}

void drawing_classifier::init_training(
    gl_sframe data, std::string target_column_name,
    std::string feature_column_name, variant_type validation_data,
    std::map<std::string, flexible_type> opts) {

  if (!data.contains_column(feature_column_name)) {
    log_and_throw(feature_column_name + " column not found. Data does not " +
      "contain the feature column.");
  }

  if (!data.contains_column(target_column_name)) {
    log_and_throw(target_column_name + " column not found. Data does not " +
      "contain the target column.");
  }

  add_or_update_state({
    {"training_iterations", 0},
    {"target", target_column_name},
    {"feature", feature_column_name}
  });

  // Capture Core ML model path from options,
  // if provided by Python.
  std::string mlmodel_path;
  {
    auto mlmodel_path_iter = opts.find("mlmodel_path");
    if ( mlmodel_path_iter != opts.end()) {
      mlmodel_path = mlmodel_path_iter->second.to<std::string>();
      // Remove `mlmodel_path` from options as
      // it is not a user-defined option.
      opts.erase(mlmodel_path_iter);
    }
  }

  bool show_loss = true;
  auto show_loss_it = opts.find("_show_loss");
  if (show_loss_it != opts.end()) {
    show_loss = show_loss_it->second;
    opts.erase(show_loss_it);
  }

  // Read user-specified options.
  init_options(opts);

  // Choose a random seed if not set.
  if (read_state<flexible_type>("random_seed") == FLEX_UNDEFINED) {
    std::random_device rd;
    int random_seed = static_cast<int>(rd());
    add_or_update_state({{"random_seed", random_seed}});
  }

  // Perform validation split if necessary.
  std::tie(training_data_, validation_data_) = init_data(data, validation_data);

  // there should be an early termination version of finding na
  // like `any`; zero is a broader concept than na;
  // however, most time, if no na, traverse the whole data set is still needed.
  auto throw_if_contains_na =
      [](const gl_sframe& data, const std::string& col_name) {
        if (data[col_name].num_missing()) {
          std::stringstream ss;
          ss << "column '" << col_name << "' contains undefined data."
             << " Please call 'dropna()' before training";
          log_and_throw(ss.str());
        }
      };

  if (!training_data_.empty()) {
    throw_if_contains_na(training_data_, feature_column_name);
    throw_if_contains_na(training_data_, target_column_name);
  }

  if (!validation_data_.empty()) {
    throw_if_contains_na(validation_data_, feature_column_name);
    throw_if_contains_na(validation_data_, target_column_name);
  }

  // Bind the data to a data iterator.
  training_data_iterator_ =
      create_iterator(training_data_,
                      /* is_train */ true, /* class labels */ {});

  const flex_list& classes = training_data_iterator_->class_labels();

  add_or_update_state({
      {"classes", classes},
      {"num_classes", classes.size()},
  });

  // Bind the validation data to a data iterator.
  if (!validation_data_.empty()) {
    validation_data_iterator_ =
        create_iterator(validation_data_,
                        /* is_train */ false, /* class labels */ classes);
  } else {
    validation_data_iterator_ = nullptr;
  }

  // Instantiate the compute context.
  training_compute_context_ = create_compute_context();
  if (training_compute_context_ == nullptr) {
    log_and_throw("No neural network compute context provided");
  }

  // Initialize the neural net. Note that this depends on statistics computed
  // by the data iterator.
  nn_spec_ = init_model(true);

  if (mlmodel_path.size()) {
    // Initialize the neural net with warm start model weights.
    model_spec warmstart_model(mlmodel_path);
    float_array_map trained_weights = warmstart_model.export_params_view();
    nn_spec_->update_params(trained_weights);
  }

  training_model_ = training_compute_context_->create_drawing_classifier(
      nn_spec_->export_params_view(), read_state<size_t>("batch_size"),
      read_state<size_t>("num_classes"));

  if (read_state<bool>("verbose")) {
    training_compute_context_->print_training_device_info();
  }

  // Begin printing progress.
  if (read_state<bool>("verbose")) {
    init_table_printer(!validation_data_.empty(), show_loss);
  }

  // Print the header last, after any logging by initialization above
  if (training_table_printer_) {
    training_table_printer_->print_header();
  }
}

// Returns the validation accuracy and validation loss respectively as a tuple
std::tuple<float, float> drawing_classifier::compute_validation_metrics(
    size_t num_classes, size_t batch_size) {
  float cumulative_val_loss = 0.f;
  size_t val_size = 0;
  size_t val_num_correct = 0;

  validation_data_iterator_->reset();

  // To support double buffering, use a queue of pending inference results.
  std::queue<result> pending_batches;

  auto pop_until_size = [&](size_t remaining) {
    while (pending_batches.size() > remaining) {
      // Pop one batch from the queue.
      result batch = pending_batches.front();
      pending_batches.pop();

      val_num_correct += count_correct_predictions(batch, num_classes);
      float val_loss =
          std::accumulate(batch.loss_info.data(),
                          batch.loss_info.data() + batch.loss_info.size(), 0.f,
                          std::plus<float>());
      cumulative_val_loss += val_loss;
    }
  };

  while (validation_data_iterator_->has_next_batch()) {
    // Wait until we have just one asynchronous batch outstanding. The work
    // below should be concurrent with the neural net inference for that batch.
    pop_until_size(1);

    result result_batch;
    result_batch.data_info = validation_data_iterator_->next_batch(batch_size);

    // Submit the batch to the neural net model.
    std::map<std::string, shared_float_array> results =
        training_model_->predict(
            {{"input", result_batch.data_info.drawings},
             {"labels", result_batch.data_info.targets},
             {"weights", result_batch.data_info.weights},
             {"num_samples",
              shared_float_array::wrap(result_batch.data_info.num_samples)}});

    result_batch.loss_info = results.at("loss");
    result_batch.output_info = results.at("output");
    val_size += result_batch.data_info.num_samples;

    // Add the pending result to our queue and move on to the next input batch.
    pending_batches.push(std::move(result_batch));
  }
  // Process all remaining batches.
  pop_until_size(0);
  float average_val_accuracy = static_cast<float>(val_num_correct) / val_size;
  float average_val_loss = cumulative_val_loss / val_size;

  return std::make_tuple(average_val_accuracy, average_val_loss);
}

void drawing_classifier::iterate_training(bool show_loss) {
  // Training must have been initialized.
  ASSERT_TRUE(training_data_iterator_ != nullptr);
  ASSERT_TRUE(training_model_ != nullptr);

  const size_t batch_size = read_state<flex_int>("batch_size");
  const size_t iteration_idx = read_state<flex_int>("training_iterations");

  float cumulative_batch_loss = 0.f;
  size_t num_batches = 0;
  size_t train_num_correct = 0;
  size_t train_num_samples = 0;
  size_t num_classes = read_state<size_t>("num_classes");

  // To support double buffering, use a queue of pending inference results.
  std::queue<result> pending_batches;

  auto pop_until_size = [&](size_t remaining) {
    while (pending_batches.size() > remaining) {
      // Pop one batch from the queue.
      result batch = pending_batches.front();
      pending_batches.pop();

      train_num_correct += count_correct_predictions(batch, num_classes);
      train_num_samples += batch.data_info.num_samples;
      float batch_loss =
          std::accumulate(batch.loss_info.data(),
                          batch.loss_info.data() + batch.loss_info.size(), 0.f,
                          std::plus<float>());

      cumulative_batch_loss += batch_loss;
    }
  };

  while (training_data_iterator_->has_next_batch()) {
    // Wait until we have just one asynchronous batch outstanding. The work
    // below should be concurrent with the neural net inference for that batch.
    pop_until_size(1);

    result result_batch;
    result_batch.data_info = training_data_iterator_->next_batch(batch_size);

    // Submit the batch to the neural net model.
    std::map<std::string, shared_float_array> results =
        training_model_->train({{"input", result_batch.data_info.drawings},
                                {"labels", result_batch.data_info.targets},
                                {"weights", result_batch.data_info.weights},
                                {"num_samples", shared_float_array::wrap(result_batch.data_info.num_samples)}
                              });
    result_batch.output_info = results.at("output");
    result_batch.loss_info = results.at("loss");
    ++num_batches;

    // Add the pending result to our queue and move on to the next input batch.
    pending_batches.push(std::move(result_batch));
  }
  // Process all remaining batches.
  pop_until_size(0);
  float average_batch_loss = cumulative_batch_loss / train_num_samples;
  float average_batch_accuracy =
      static_cast<float>(train_num_correct) / train_num_samples;
  float average_val_accuracy;
  float average_val_loss;

  if (validation_data_iterator_) {
    std::tie(average_val_accuracy, average_val_loss) =
        compute_validation_metrics(num_classes, batch_size);
  }
  add_or_update_state({{"training_iterations", iteration_idx + 1},
                       {"training_accuracy", average_batch_accuracy},
                       {"training_log_loss", average_batch_loss}});

  if (validation_data_iterator_) {
    add_or_update_state({{"validation_accuracy", average_val_accuracy},
                         {"validation_log_loss", average_val_loss}});
  } else {
    add_or_update_state({
        {"validation_accuracy", FLEX_UNDEFINED}
    });
  }

  if (training_table_printer_) {
    if (validation_data_iterator_) {
      if (show_loss) {
        training_table_printer_->print_progress_row(
            iteration_idx, iteration_idx + 1, average_batch_accuracy,
            average_batch_loss, average_val_accuracy, average_val_loss,
            progress_time());
      } else {
        training_table_printer_->print_progress_row(
            iteration_idx, iteration_idx + 1, average_batch_accuracy,
            average_val_accuracy, progress_time());
      }

    } else {
      if (show_loss) {
        training_table_printer_->print_progress_row(
            iteration_idx, iteration_idx + 1, average_batch_accuracy,
            average_batch_loss, progress_time());
      } else {
        training_table_printer_->print_progress_row(
            iteration_idx, iteration_idx + 1, average_batch_accuracy,
            progress_time());
      }
    }
  }

  training_data_iterator_->reset();
}  // namespace

std::unique_ptr<compute_context> drawing_classifier::create_compute_context()
    const {
  return compute_context::create_tf();
}

void drawing_classifier::init_table_printer(bool has_validation,
                                            bool show_loss) {
  if (has_validation) {
    if (show_loss) {
      training_table_printer_.reset(
          new table_printer({{"Iteration", 12},
                             {"Train Accuracy", 12},
                             {"Train Loss", 12},
                             {"Validation Accuracy", 12},
                             {"Validation Loss", 12},
                             {"Elapsed Time", 12}}));
    } else {
      training_table_printer_.reset(
          new table_printer({{"Iteration", 12},
                             {"Train Accuracy", 12},
                             {"Validation Accuracy", 12},
                             {"Elapsed Time", 12}}));
    }

  } else {
    if (show_loss) {
      training_table_printer_.reset(new table_printer({{"Iteration", 12},
                                                       {"Train Accuracy", 12},
                                                       {"Train Loss", 12},
                                                       {"Elapsed Time", 12}}));
    } else {
      training_table_printer_.reset(new table_printer(
          {{"Iteration", 12}, {"Train Accuracy", 12}, {"Elapsed Time", 12}}));
    }
  }
}

void drawing_classifier::train(gl_sframe data, std::string target_column_name,
                               std::string feature_column_name,
                               variant_type validation_data,
                               std::map<std::string, flexible_type> opts) {

  turi::timer time_object;
  time_object.start();

  // Instantiate the training dependencies: data iterator, compute context,
  // backend NN model.

  bool show_loss = true;
  auto show_loss_it = opts.find("_show_loss");
  if (show_loss_it != opts.end()) {
    show_loss = show_loss_it->second;
  }

  init_training(data, target_column_name, feature_column_name, validation_data,
                opts);

  // Perform all the iterations at once.
  flex_int max_iterations = read_state<flex_int>("max_iterations");
  while (read_state<flex_int>("training_iterations") < max_iterations) {
    iterate_training(show_loss);
  }

  // Finish printing progress.
  if (training_table_printer_) {
    training_table_printer_->print_footer();
    training_table_printer_.reset();
  }

  // Sync trained weights to our local storage of the NN weights.
  float_array_map trained_weights = training_model_->export_weights();
  nn_spec_->update_params(trained_weights);

  variant_map_type state_update;

  // Update the state with recall, precision and confusion matrix for training
  // data
  gl_sarray train_predictions = predict(training_data_, "probability_vector");
  variant_map_type train_metric = evaluation::compute_classifier_metrics(
      training_data_, target_column_name, "report", train_predictions,
      {{"classes", read_state<flex_list>("classes")}});

  for (auto& p : train_metric) {
    state_update["training_" + p.first] = p.second;
  }

  // Update the state with recall, precision and confusion matrix for validation
  // data
  if (!validation_data_.empty()) {
    gl_sarray val_predictions = predict(validation_data_, "probability_vector");
    variant_map_type val_metric = evaluation::compute_classifier_metrics(
        validation_data_, target_column_name, "report", val_predictions,
        {{"classes", read_state<flex_list>("classes")}});

    for (auto& p : val_metric) {
      state_update["validation_" + p.first] = p.second;
    }
  }

  state_update["num_examples"] = data.size();
  state_update["training_time"] = time_object.current_time();

  add_or_update_state(state_update);
}

gl_sframe drawing_classifier::perform_inference(data_iterator* data) const {
  // Open a new SFrame for writing.
  gl_sframe_writer writer({"preds"}, {flex_type_enum::VECTOR},
                          /* num_segments */ 1);

  const size_t num_classes = read_state<size_t>("num_classes");
  const size_t batch_size = read_state<size_t>("batch_size");

  // Initialize the NN backend.
  std::unique_ptr<compute_context> ctx = create_compute_context();
  std::unique_ptr<model_backend> backend = ctx->create_drawing_classifier(
      nn_spec_->export_params_view(), batch_size, num_classes);

  // To support double buffering, use a queue of pending inference results.
  std::queue<result> pending_batches;

  // Allocate a buffer into which to write the class probabilities.
  flex_vec preds(num_classes);

  auto pop_until_size = [&](size_t remaining) {
    while (pending_batches.size() > remaining) {
      // Pop one batch from the queue.
      result batch = pending_batches.front();
      pending_batches.pop();

      size_t num_images = batch.data_info.num_samples;
      auto output_itr = batch.data_info.predictions.data();
      
      for (size_t ii = 0; ii < num_images; ++ii) {
        std::copy(output_itr, output_itr + num_classes, preds.begin());
        output_itr += num_classes;
        writer.write({preds}, 0);
      }
    }
  };

  while (data->has_next_batch()) {
    // Wait until we have just one asynchronous batch outstanding. The work
    // below should be concurrent with the neural net inference for that batch.
    pop_until_size(1);

    result result_batch;
    result_batch.data_info = data->next_batch(batch_size);

    /** TODO: Figure out a better solution to having `num_samples` be a
     *  top-level input to the network. May be captured in the first
     *  dimension of the input, or perhaps via a weight tensor.
     */
    std::map<std::string, shared_float_array> results =
        backend->predict({{"input", result_batch.data_info.drawings},
                          {"num_samples", shared_float_array::wrap(result_batch.data_info.num_samples)}
                         });

    // Copy the (float) outputs to our (double) buffer and add to the SArray.
    result_batch.data_info.predictions = results.at("output");

    pending_batches.push(std::move(result_batch));
  }

  pop_until_size(0);

  return writer.close();
}

gl_sarray drawing_classifier::get_predictions_class(
    const gl_sarray& predictions_prob, const flex_list& class_labels) {
  auto max_prob_label = [=](const flexible_type& ft) {
    const flex_vec& prob_vec = ft.get<flex_vec>();
    auto max_it = std::max_element(prob_vec.begin(), prob_vec.end());
    return class_labels[max_it - prob_vec.begin()];
  };

  return predictions_prob.apply(max_prob_label,
                                class_labels.front().get_type());
}

gl_sarray drawing_classifier::predict(gl_sframe data, std::string output_type) {
  // by default, it should be "probability" if the value is
  // passed in through python client
  if (output_type != "probability"
    && output_type != "probability_vector"
    && output_type != "class") {
    log_and_throw(output_type + " is not a valid option for output_type.  " +
                  "Expected one of: probability, probability_vector, rank");
  }

  std::string feature_column_name = read_state<flex_string>("feature");
  if (!data.contains_column(feature_column_name)) {
    log_and_throw(feature_column_name + " column not found. " +
      "Data passed in to predict does not contain the feature column.");
  }

  auto data_itr =
      create_iterator(data, /* is_train */ false, /* class labels */ {});

  gl_sframe predictions = perform_inference(data_itr.get());

  gl_sarray result = predictions["preds"];
  if (output_type == "class") {
    flex_list class_labels = read_state<flex_list>("classes");
    result = get_predictions_class(result, class_labels);

  } else if (output_type == "probability") {
    /** The output_type="probability" is to provide the probability of the True
     *  class in binary classifiers.
     *  For example, if one is building a binary classifier for
     *  "cat" vs "not cat", output_type="probability" should output the
     *  probability of the data point being "cat", not the probability of the
     *  predicted class.
     */

    size_t num_classes = read_state<flex_int>("num_classes");
    DASSERT_GT(num_classes, 0);

    if (num_classes > 2) {
      log_and_throw(
          "Use probability_vector in case of multi-class classification.");
    }

    auto true_class_probability = [=](const flexible_type& ft) {
      const flex_vec& prob_vec = ft.get<flex_vec>();
      return prob_vec.back();
    };

    result = result.apply(true_class_probability, flex_type_enum::FLOAT);
  }

  return result;
}

gl_sframe drawing_classifier::predict_topk(gl_sframe data,
                                           std::string output_type, size_t k) {
  // check valid input arguments
  if (output_type != "probability" && output_type != "rank") {
    log_and_throw(output_type +
                  " is not a valid option for output_type.  "
                  "Expected one of: probability, rank");
  }

  std::string feature_column_name = read_state<flex_string>("feature");
  if (!data.contains_column(feature_column_name)) {
    log_and_throw(feature_column_name + " column not found. " +
      "Data passed in to predict_topk does not contain the feature column.");
  }

  // data inference
  std::unique_ptr<data_iterator> data_it =
      create_iterator(data, /* is_train */ false, /* class labels */ {});

  gl_sframe dc_predictions = perform_inference(data_it.get());

  // argsort probability to get the index for top k class
  // if k is greater than the class number, set it to be class number
  flex_list class_labels = read_state<flex_list>("classes");
  k = std::min(k, class_labels.size());

  auto argsort_top_k_indices = [=](const flexible_type& ft) {
    const flex_vec& prob_vec = ft.get<flex_vec>();

    std::vector<size_t> index_vec(prob_vec.size());
    std::iota(index_vec.begin(), index_vec.end(), 0);

    auto compare = [&](size_t lhs, size_t rhs) {
      return prob_vec[lhs] > prob_vec[rhs];
    };

    std::nth_element(index_vec.begin(), index_vec.begin() + k, index_vec.end(),
                     compare);

    std::sort(index_vec.begin(), index_vec.begin() + k, compare);

    return flex_list(index_vec.begin(), index_vec.begin() + k);
  };

  auto compute_result_column = [=](const flexible_type& ft) {
    const flex_vec& prob_vec = ft.get<flex_vec>();

    const flex_list& top_k_indices = argsort_top_k_indices(ft);

    flex_dict result;

    for (size_t rank = 0; rank < k; rank++) {
      flex_int class_index = top_k_indices[rank];
      flexible_type value;

      if (output_type == "probability") {
        value = prob_vec[class_index];
      } else {
        value = rank;
      }

      result.emplace_back(class_labels[class_index], value);
    }
    return result;
  };

  gl_sframe result = gl_sframe();

  // Get a result column that we can stack, from the probability vector
  result.add_column(dc_predictions["preds"].apply(compute_result_column,
                                                  flex_type_enum::DICT),
                    "result");
  result = result.add_row_number();
  std::vector<std::string> new_column_names;
  new_column_names.push_back("class");
  new_column_names.push_back("rank");
  result = result.stack("result", new_column_names);

  // change the column name rank to probability according to the output_type
  if (output_type == "probability") {
    result.rename({{"rank", "probability"}});
  }

  return result;
}

variant_map_type drawing_classifier::evaluate(gl_sframe data,
                                              std::string metric) {

  gl_sarray predictions_prob = predict(data, "probability_vector");

  flex_list class_labels = read_state<flex_list>("classes");
  gl_sarray predictions_class =
      get_predictions_class(predictions_prob, class_labels);

  variant_map_type result = evaluation::compute_classifier_metrics(
      data, read_state<flex_string>("target"), metric, predictions_prob,
      {{"classes", read_state<flex_list>("classes")}});

  result.emplace("prediction_prob", predictions_prob);
  result.emplace("prediction_class", predictions_class);

  return result;

}

std::shared_ptr<coreml::MLModelWrapper> drawing_classifier::export_to_coreml(
    std::string filename, std::string short_description,
    const std::map<std::string, flexible_type>& additional_user_defined,
    bool use_default_spec) {

  if (!nn_spec_) {
    // use empty nn spec if not initalized;
    // avoid test bad memory access
    if (use_default_spec) {
      nn_spec_ = std::unique_ptr<model_spec>(new model_spec);
    } else {
      log_and_throw(
          "model is not initialized; please call train before export_coreml");
    }
  }

  std::string feature_column_name = read_state<flex_string>("feature");
  flex_list features_list;
  features_list.push_back(feature_column_name);

  std::shared_ptr<MLModelWrapper> model_wrapper =
      export_drawing_classifier_model(
          *nn_spec_, features_list,
          read_state<flex_list>("classes"), read_state<flex_string>("target"));

  flex_dict user_defined_metadata = {
      {"target", read_state<flex_string>("target")},
      {"feature", feature_column_name},
      {"type", "drawing_classifier"},
      {"version", 2},
  };
  // for model imported from version 5.8 or prior
  if (state.count("warm_start")){
    user_defined_metadata.emplace_back("warm_start",
                                       read_state<flex_string>("warm_start"));
  }
  if (state.count("max_iterations")) {
    user_defined_metadata.emplace_back("max_iterations",
                                       read_state<flex_int>("max_iterations"));
  }

  user_defined_metadata.insert(user_defined_metadata.end(),
                               additional_user_defined.begin(),
                               additional_user_defined.end());

  model_wrapper->add_metadata({
      {"short_description", short_description},
      {"user_defined", std::move(user_defined_metadata)},
  });

  if (!filename.empty()) {
    model_wrapper->save(filename);
  }

  return model_wrapper;
}

void drawing_classifier::import_from_custom_model(variant_map_type model_data,
                                                  size_t version) {
  auto model_iter = model_data.find("_model");
  if (model_iter == model_data.end()) {
    log_and_throw("The loaded turicreate model must contain '_model'!\n");
  }
  const flex_dict& model = variant_get_value<flex_dict>(model_iter->second);

  // For a model trained on integer classes, when saved and loaded back,
  // the classes are loaded as floats. The following code block casts
  // the loaded "float" classes back to int.
  if (model_data.count("classes")) {
    flex_list classes_list =
        variant_get_value<flex_list>(model_data.find("classes")->second);

    if (classes_list.size() &&
        classes_list.begin()->get_type() == flex_type_enum::FLOAT) {
        for (flexible_type& ft : classes_list) {
          ft = ft.to<flex_int>();
        }
        model_data["classes"] = std::move(classes_list);
    } else if (classes_list.empty()) {
      log_and_throw(
          "Error during loading model. 'classes' must contain at least one "
          "class label.");
    }
  } else {
    log_and_throw(
        "Error during loading model. 'classes' not found in the saved model.");
  }

  flex_dict mxnet_data_dict;
  flex_dict mxnet_shape_dict;

  for (const auto& data : model) {
    if (data.first == "data") {
      mxnet_data_dict = data.second;
    } else if (data.first == "shapes") {
      mxnet_shape_dict = data.second;
    }
  }

  auto cmp = [](const flex_dict::value_type& a,
                const flex_dict::value_type& b) { return (a.first < b.first); };

  std::sort(mxnet_data_dict.begin(), mxnet_data_dict.end(), cmp);
  std::sort(mxnet_shape_dict.begin(), mxnet_shape_dict.end(), cmp);

  float_array_map nn_params;

  std::vector<float> layer_weight;
  std::vector<size_t> layer_shape;

  for (size_t ii = 0; ii < mxnet_data_dict.size(); ii++) {
    std::string layer_name = mxnet_data_dict[ii].first;
    flex_nd_vec mxnet_data_nd = mxnet_data_dict[ii].second.to<flex_nd_vec>();
    flex_nd_vec mxnet_shape_nd = mxnet_shape_dict[ii].second.to<flex_nd_vec>();

    const std::vector<double>& model_weight = mxnet_data_nd.elements();
    const std::vector<double>& model_shape = mxnet_shape_nd.elements();
    // load the weights
    layer_weight.clear();
    layer_shape.clear();
    layer_weight.insert(layer_weight.end(), model_weight.begin(), model_weight.end());
    layer_shape.insert(layer_shape.end(), model_shape.begin(), model_shape.end());

    nn_params[layer_name] = shared_float_array::wrap(std::move(layer_weight),
                                                     std::move(layer_shape));
  }

  // prune redudant data
  model_data.erase(model_iter);
  if (model_data.count("_class_to_index")) {
    model_data.erase("_class_to_index");
  }
  if (model_data.count("input_image_shape")) {
    model_data.erase("input_image_shape");
  }

  // must set state before init_model(); also update
  state = std::move(model_data);

  // needed by evaluate
  if (!state.count("batch_size")) state.emplace("batch_size", 256);

  nn_spec_ = init_model(false);
  nn_spec_->update_params(nn_params);

  return;
}

}  // namespace drawing_classifier
}  // namespace turi

/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/drawing_classifier/drawing_classifier.hpp>

#include <iostream>

#include <core/logging/assertions.hpp>
#include <core/logging/logger.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/coreml_export/neural_net_models_exporter.hpp>
#include <toolkits/evaluation/metrics.hpp>


namespace turi {
namespace drawing_classifier {

namespace {

using neural_net::compute_context;
using neural_net::float_array_map;
using neural_net::model_backend;
using neural_net::model_spec;
using neural_net::shared_float_array;

struct result {
  shared_float_array loss_info;
  shared_float_array output_info;
  data_iterator::batch data_info;
};

size_t count_correct_predictions(size_t num_classes,
                                 const shared_float_array& outputs,
                                 const data_iterator::batch& batch) {
  
  size_t num_correct = 0;
  const float* prediction_ptr = outputs.data();
  const float* truth_ptr = batch.targets.data();

  for (size_t i = 0; i < batch.num_samples; ++i) {
    if (*prediction_ptr == *truth_ptr) {
      num_correct++;
    }
    prediction_ptr++;
    truth_ptr++;
  }
  return num_correct;
}

}  // namespace

std::unique_ptr<model_spec> drawing_classifier::init_model(
    bool use_random_init) const {
  std::unique_ptr<model_spec> result(new model_spec);
  return result;
}


void drawing_classifier::init_options(
    const std::map<std::string, flexible_type>& opts) {
  // Define options.
  options.create_integer_option(
      "batch_size",
      "Number of training examples used per training step",
      256,
      1,
      std::numeric_limits<int>::max());
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
      std::numeric_limits<int>::min(),
      std::numeric_limits<int>::max());

  // Validate user-provided options.
  options.set_options(opts);

  // Choose a random seed if not set.
  if (options.value("random_seed") == FLEX_UNDEFINED) {
    std::random_device random_device;
    int random_seed = static_cast<int>(random_device());
    options.set_option("random_seed", random_seed);
  }

  // Write model fields.
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

std::tuple<gl_sframe, gl_sframe> drawing_classifier::init_data(
      gl_sframe data, variant_type validation_data) const {
  gl_sframe train_data;
  gl_sframe val_data;
  if (variant_is<gl_sframe>(validation_data)) {
    train_data = data;
    val_data = variant_get_value<gl_sframe>(validation_data);
    if (!val_data.empty()) {
    } else {
      log_and_throw("Input SFrame either has no rows or no columns. A "
                    "non-empty SFrame is required");
    }
  }
  else if ((variant_is<flex_string>(validation_data)) 
      && (variant_get_value<flex_string>(validation_data)=="auto")) {
    size_t num_rows = data.size();
    // size_t seed = read_state<size_t>("random_seed");
    if (num_rows >= 100) {
      std::tie(train_data, val_data) = data.random_split(.95);
    } else {
      train_data = data;
      std::cout << "The dataset has less than the minimum of 100 rows required for train-validation split. "
                       "Continuing without validation set.\n";
    }
  } else {
    train_data = data;
  }
  return std::make_tuple(train_data, val_data);
}

std::unique_ptr<data_iterator> drawing_classifier::create_iterator(
      gl_sframe data, std::vector<std::string> class_labels, bool is_train) const {
  data_iterator::parameters data_params;
  data_params.data = std::move(data);

  if (!is_train) {
    data_params.class_labels = std::move(class_labels);
  }

  data_params.is_train = is_train;
  data_params.target_column_name = read_state<flex_string>("target");
  data_params.feature_column_name = read_state<flex_string>("feature");
  return std::unique_ptr<data_iterator>(new simple_data_iterator(data_params));
}

void drawing_classifier::init_train(gl_sframe data,
                                    std::string target_column_name,
                                    std::string feature_column_name,
                                    variant_type validation_data,
                                    std::map<std::string, flexible_type> opts) {
  /* TODO: Rewrite! */

  // Read user-specified options.
  init_options(opts);

  // Choose a random seed if not set.
  if (read_state<flexible_type>("random_seed") == FLEX_UNDEFINED) {
    std::random_device random_device;
    int random_seed = static_cast<int>(random_device());
    add_or_update_state({{"random_seed", random_seed}});
  }

  // Perform validation split if necessary.
  std::tie(training_data_, validation_data_) = init_data(data, validation_data);

  // Begin printing progress.
  // TODO: Make progress printing optional.
  init_table_printer(!validation_data_.empty());

  add_or_update_state({{"target", target_column_name},
                       {"feature", feature_column_name}});

  const std::vector<std::string>& classes =
      training_data_iterator_->class_labels();
  add_or_update_state({{"classes", flex_list(classes.begin(), classes.end())}});

  // Bind the data to a data iterator.
  training_data_iterator_ =
      create_iterator(training_data_, /* class labels */ classes,
        /* is_train */ true);

  // Bind the validation data to a data iterator.
  if (!validation_data_.empty()) {
    validation_data_iterator_ = create_iterator(validation_data_, 
      /* class labels */ classes, /* is_train */ false);
  } else {
    validation_data_iterator_ = nullptr;
  }

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
      {"num_classes", training_data_iterator_->class_labels().size()},
      {"training_iterations", 0},
  });

  // Initialize the neural net. Note that this depends on statistics computed by
  // the data iterator.
  bool use_random_init = true;
  nn_spec_ = init_model(use_random_init);

  // TODO: Do not hardcode values
  training_model_ = training_compute_context_->create_drawing_classifier(
    validation_data,
    /* TODO: nn_spec_->export_params_view().
     * Until the nn_spec in C++ isn't ready, do not pass in any weights. 
     */
    256,
    2,
    true
  );

  // Print the header last, after any logging triggered by initialization above.
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
  size_t val_num_samples = 0;
  validation_data_iterator_->reset();

  // To support double buffering, use a queue of pending inference results.
  std::queue<result> pending_batches;

  auto pop_until_size = [&](size_t remaining) {

    while (pending_batches.size() > remaining) {

      // Pop one batch from the queue.
      result batch = pending_batches.front();
      pending_batches.pop();

      size_t batch_num_correct = 0;
      batch_num_correct =
          count_correct_predictions(num_classes, 
                                    batch.output_info, batch.data_info);
      val_num_correct += batch_num_correct;
      val_num_samples += batch.data_info.num_samples;
      float val_loss = std::accumulate(batch.loss_info.data(), batch.loss_info.data() + batch.loss_info.size(),
                                     0.f, std::plus<float>());
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
        training_model_->predict({
            {"input", result_batch.data_info.drawings},
            {"labels", result_batch.data_info.targets}
        });

    result_batch.output_info = results.at("output");
    result_batch.loss_info = results.at("loss");
    val_size += result_batch.data_info.num_samples;

    // Add the pending result to our queue and move on to the next input batch.
    pending_batches.push(std::move(result_batch));

  }
  // Process all remaining batches.
  pop_until_size(0);
  float average_val_accuracy =
      static_cast<float>(val_num_correct) / val_num_samples;
  float average_val_loss = cumulative_val_loss / val_size;

  return std::make_tuple(average_val_accuracy, average_val_loss);
}


void drawing_classifier::perform_training_iteration() {

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

      size_t batch_num_correct = 0;
      batch_num_correct = count_correct_predictions(num_classes,
                                                    batch.output_info,
                                                    batch.data_info);
      train_num_correct += batch_num_correct;
      train_num_samples += batch.data_info.num_samples;

      float batch_loss = std::accumulate(batch.loss_info.data(),
                                         batch.loss_info.data() + batch.loss_info.size(),
                                         0.f, std::plus<float>());

      cumulative_batch_loss += batch_loss / batch.data_info.num_samples;

    }
  };


  while (training_data_iterator_->has_next_batch()) {

    // Wait until we have just one asynchronous batch outstanding. The work
    // below should be concurrent with the neural net inference for that batch.
    pop_until_size(1);

    result result_batch;
    result_batch.data_info = training_data_iterator_->next_batch(batch_size);

    // Submit the batch to the neural net model.
    std::map<std::string, shared_float_array> results = training_model_->train(
        { { "input",   result_batch.data_info.drawings },
          { "labels",  result_batch.data_info.targets   }
        });
    result_batch.loss_info = results.at("loss");
    result_batch.output_info = results.at("output");

    ++num_batches;

    // Add the pending result to our queue and move on to the next input batch.
    pending_batches.push(std::move(result_batch));

  }
  // Process all remaining batches.
  pop_until_size(0);
  float average_batch_loss = cumulative_batch_loss / num_batches;
  float average_batch_accuracy =
      static_cast<float>(train_num_correct) / train_num_samples;
  float average_val_accuracy;
  float average_val_loss;

  if (validation_data_iterator_) {

    std::tie(average_val_accuracy, average_val_loss) =
        compute_validation_metrics(num_classes, batch_size);
  }
  add_or_update_state({
      {"training_iterations", iteration_idx + 1},
      {"training_accuracy", average_batch_accuracy},
      {"training_log_loss", average_batch_loss},
  });

  if (validation_data_iterator_) {
    add_or_update_state({
        {"validation_accuracy", average_val_accuracy},
        {"validation_log_loss", average_val_loss},
    });
  }

  if (training_table_printer_) {
    if (validation_data_iterator_) {
      training_table_printer_->print_progress_row(
          iteration_idx, iteration_idx + 1, average_batch_accuracy,
          average_batch_loss, average_val_accuracy, average_val_loss,
          progress_time());
    } else {
      training_table_printer_->print_progress_row(
          iteration_idx, iteration_idx + 1, average_batch_accuracy,
          average_batch_loss, progress_time());
    }
  }

  training_data_iterator_->reset();
}  // namespace

std::unique_ptr<compute_context> drawing_classifier::create_compute_context()
    const {
  return compute_context::create_tf();
}


void drawing_classifier::init_table_printer(bool has_validation) {
  if (has_validation) {
    training_table_printer_.reset(
        new table_printer({{"Iteration", 12},
                           {"Train Accuracy", 12},
                           {"Train Loss", 12},
                           {"Validation Accuracy", 12},
                           {"Validation Loss", 12},
                           {"Elapsed Time", 12}}));
  } else {
    training_table_printer_.reset(new table_printer({{"Iteration", 12},
                                                     {"Train Accuracy", 12},
                                                     {"Train Loss", 12},
                                                     {"Elapsed Time", 12}}));
  }
}


void drawing_classifier::train(gl_sframe data,
                               std::string target_column_name,
                               std::string feature_column_name,
                               variant_type validation_data,
                               std::map<std::string, flexible_type> opts) {
  // Instantiate the training dependencies: data iterator, compute context,
  // backend NN model.
  init_train(data, target_column_name, feature_column_name, validation_data,
             opts);

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

  variant_map_type state_update;

  // Update the state with recall, precision and confusion matrix for training
  // data
  gl_sarray train_predictions = predict(training_data_, "probability_vector");
  variant_map_type train_metric = evaluation::compute_classifier_metrics(
      training_data_, target_column_name, "report", train_predictions,
      {{"classes", read_state<flex_list>("classes")}});

  for (auto &p : train_metric) {
    state_update["training_" + p.first] = p.second;
  }

  // Update the state with recall, precision and confusion matrix for validation
  // data
  if (!validation_data_.empty()) {
    gl_sarray val_predictions = predict(validation_data_, "probability_vector");
    variant_map_type val_metric = evaluation::compute_classifier_metrics(
        validation_data_, target_column_name, "report", val_predictions,
        {{"classes", read_state<flex_list>("classes")}});

    for (auto &p : val_metric) {
      state_update["validation_" + p.first] = p.second;
    }
  }

  add_or_update_state(state_update);
}

gl_sarray drawing_classifier::predict(gl_sframe data, std::string output_type) {
  /* TODO: Add code to predict! */
  return gl_sarray();
}

gl_sframe drawing_classifier::predict_topk(gl_sframe data, 
  std::string output_type, size_t k) {
  /* TODO: Add code to predict_topk! */
  return gl_sframe();
}

variant_map_type drawing_classifier::evaluate(gl_sframe data, std::string metric) {
  // Perform prediction.
  gl_sarray predictions = predict(data, "probability_vector");

  /* TODO: This is just for the skeleton. Rewrite. */
  return evaluation::compute_classifier_metrics(
      data, "label", metric, predictions,
      {{"classes", 2}});
}

std::shared_ptr<coreml::MLModelWrapper> drawing_classifier::export_to_coreml(
    std::string filename) {
  /* Add code for export_to_coreml */
  return nullptr;
}

}  // namespace drawing_classifier
}  // namespace turi

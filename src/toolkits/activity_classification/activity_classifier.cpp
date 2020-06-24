/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/activity_classification/activity_classifier.hpp>

#include <algorithm>
#include <functional>
#include <numeric>
#include <random>

#include <core/logging/assertions.hpp>
#include <core/logging/logger.hpp>
#include <core/util/string_util.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <timer/timer.hpp>
#include <toolkits/coreml_export/neural_net_models_exporter.hpp>
#include <toolkits/evaluation/metrics.hpp>
#include <toolkits/util/float_array_serialization.hpp>
#include <toolkits/util/training_utils.hpp>

namespace turi {
namespace activity_classification {

namespace {

using coreml::MLModelWrapper;
using neural_net::ac_parameters;
using neural_net::compute_context;
using neural_net::float_array_map;
using neural_net::lstm_weight_initializers;
using neural_net::model_backend;
using neural_net::model_spec;
using neural_net::shared_float_array;
using neural_net::weight_initializer;
using neural_net::xavier_weight_initializer;
using neural_net::zero_weight_initializer;

using padding_type = model_spec::padding_type;

// The last Python version was 2.
constexpr size_t ACTIVITY_CLASSIFIER_VERSION = 3;

constexpr size_t NUM_PREDICTIONS_PER_CHUNK = 20;

// TODO: Encapsulate these network architecture parameters better, to support
// alternative architectures.
constexpr size_t NUM_CONV_FILTERS = 64;
constexpr size_t LSTM_HIDDEN_SIZE = 200;
constexpr size_t FULLY_CONNECTED_HIDDEN_SIZE = 128;
constexpr float LSTM_CELL_CLIP_THRESHOLD = 50000.f;

size_t count_correct_predictions(size_t num_classes, const shared_float_array& output_chunk,
    const shared_float_array& label_chunk, size_t num_samples, size_t prediction_window) {

  const float* output_ptr = output_chunk.data();
  const float* truth_ptr = label_chunk.data();

  size_t num_correct_predictions = 0;

  for (size_t i = 0; i < num_samples; i+=prediction_window) {
    size_t prediction = std::max_element(output_ptr, output_ptr + num_classes) - output_ptr;
    for (size_t j = 0; j < std::min(prediction_window, num_samples-i); j++) {
      if (prediction == *truth_ptr) {
        num_correct_predictions += 1;
      }
      truth_ptr++;
    }
    output_ptr += num_classes;
  }
  return num_correct_predictions;
}

std::tuple<size_t, size_t> cumulative_chunk_accuracy(
    size_t prediction_window, size_t num_classes,
    const shared_float_array& output, const data_iterator::batch& batch) {
  size_t num_correct = 0;
  size_t num_samples = 0;

  for (size_t i = 0; i < batch.batch_info.size(); ++i){

    const shared_float_array& output_chunk = output[i];
    const shared_float_array& label_chunk = batch.labels_per_row[i];
    data_iterator::batch::chunk_info info = batch.batch_info[i];
    size_t num_correct_predictions = count_correct_predictions(num_classes, output_chunk, label_chunk, info.num_samples, prediction_window);
    num_correct += num_correct_predictions;
    num_samples += info.num_samples;
  }

  return std::make_tuple(num_correct, num_samples);
}

struct result {
    shared_float_array loss_info;
    shared_float_array output_info;
    data_iterator::batch data_info;
};

}  // namespace

size_t activity_classifier::get_version() const {
  return ACTIVITY_CLASSIFIER_VERSION;
}

void activity_classifier::save_impl(oarchive& oarc) const {
  // Save model attributes.
  variant_deep_save(state, oarc);

  // Save neural net weights.
  save_float_array_map(read_model_spec()->export_params_view(), oarc);
}

void activity_classifier::load_version(iarchive& iarc, size_t version) {
  // Load model attributes.
  variant_deep_load(state, iarc);

  // Load neural net weights.
  float_array_map nn_params = load_float_array_map(iarc);

  bool use_random_init = false;
  nn_spec_ = init_model(use_random_init);
  nn_spec_->update_params(nn_params);
  nn_spec_synchronized_ = true;
}

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
      "Number of sequence chunks used per training step",
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
  options.create_boolean_option(
      "use_data_augmentation",
      "Data augmentation helps use prediction window started with random "
      "offset."
      " If set to True, the trained model uses augmented data.",
      false);
  options.create_integer_option(
      "random_seed",
      "Seed for random weight initialization and sampling during training",
      FLEX_UNDEFINED,
      std::numeric_limits<int>::min(),
      std::numeric_limits<int>::max());
  options.create_boolean_option(
      "verbose",
      "If set to False, the progress table is hidden.",
      true,
      true);
  options.create_integer_option(
      "num_sessions",
      "Number of sessions.",
      FLEX_UNDEFINED,
      0,
      std::numeric_limits<int>::max(),
      true);

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

// Randomly split an SFrame into two SFrames based on the `session_id` such
// that one split contains data for a `fraction` of the sessions while the
// second split contains all data for the rest of the sessions.
std::tuple<gl_sframe, gl_sframe> activity_classifier::random_split_by_session(
    gl_sframe data, std::string session_id_column_name, float fraction,
    size_t seed) {
  if (std::find(data.column_names().begin(), data.column_names().end(),
                session_id_column_name) == data.column_names().end()) {
    log_and_throw("Input dataset must contain a column called " +
                  session_id_column_name);
  }

  if (fraction < 0.f || fraction > 1.f) {
    log_and_throw("Fraction specified must be between 0 and 1");
  }
  // Create a random binary filter (boolean SArray), using the same probability
  // across all lines that belong to the same session. In expectancy - the
  // desired fraction of the sessions will go to the training set. Since boolean
  // filters preserve order - there is no need to re-sort the lines within each
  // session. The boolean filter is a pseudorandom function of the session_id
  // and the global seed above, allowing the train-test split to vary across
  // runs using the same dataset.
  auto random_session_pick = [fraction](uint32_t session_id_hash) {
    std::default_random_engine generator(session_id_hash);
    std::uniform_real_distribution<float> dis(0.f, 1.f);
    return dis(generator) < fraction;
  };

  gl_sarray chosen_filter = data[session_id_column_name].hash(seed).apply(
      random_session_pick, flex_type_enum::INTEGER);
  gl_sframe train = data[chosen_filter];
  gl_sframe val = data[1 - chosen_filter];

  return std::make_tuple(train, val);
}

// Returns the validation accuracy and validation loss respectively as a tuple
std::tuple<float, float> activity_classifier::compute_validation_metrics(
    size_t prediction_window, size_t num_classes, size_t batch_size) {


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
      size_t batch_num_samples = 0;
      std::tie(batch_num_correct, batch_num_samples) =
          cumulative_chunk_accuracy(prediction_window, num_classes,
                                    batch.output_info, batch.data_info);
      val_num_correct += batch_num_correct;
      val_num_samples += batch_num_samples;
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
            {"input", result_batch.data_info.features},
            {"labels", result_batch.data_info.labels},
            {"weights", result_batch.data_info.weights},
        });

    result_batch.output_info = results.at("output");
    result_batch.loss_info = results.at("loss");
    val_size += result_batch.data_info.batch_info.size();

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

void activity_classifier::init_table_printer(bool has_validation) {
  if (read_state<bool>("verbose")) {
    if (has_validation) {
      if (show_loss_) {
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
      if (show_loss_) {
        training_table_printer_.reset(
            new table_printer({{"Iteration", 12},
                               {"Train Accuracy", 12},
                               {"Train Loss", 12},
                               {"Elapsed Time", 12}}));
      } else {
        training_table_printer_.reset(new table_printer(
            {{"Iteration", 12}, {"Train Accuracy", 12}, {"Elapsed Time", 12}}));
      }
    }
  }
}

void activity_classifier::train(
    gl_sframe data, std::string target_column_name,
    std::string session_id_column_name, variant_type validation_data,
    std::map<std::string, flexible_type> opts)
{

  turi::timer time_object;
  time_object.start();

  // Instantiate the training dependencies: data iterator, compute context,
  // backend NN model.
  init_training(data, target_column_name, session_id_column_name,
                validation_data, opts);

  // Perform all the iterations at once.
  flex_int max_iterations = read_state<flex_int>("max_iterations");
  while (read_state<flex_int>("training_iterations") < max_iterations) {
    iterate_training();
  }

  finalize_training();

  variant_map_type state_update;
  state_update["training_time"] = time_object.current_time();
  add_or_update_state(state_update);

  logprogress_stream << "Training complete" << std::endl;
  logprogress_stream << "Total Time Spent: "
                     << read_state<flex_float>("training_time") << std::endl;
}

// iterate_training() performs a complete epoch, synchronizing with the GPU. As
// a result, no explicit synchronization is needed. We expose this method just
// for consistency with other models, like object_detector.
void activity_classifier::synchronize_training() {}

const model_spec* activity_classifier::read_model_spec() const {
  if (training_model_ && !nn_spec_synchronized_) {
    float_array_map trained_weights = training_model_->export_weights();
    nn_spec_->update_params(trained_weights);
    nn_spec_synchronized_ = true;
  }
  return nn_spec_.get();
}

void activity_classifier::finalize_training() {
  // Finish printing progress.
  if (training_table_printer_) {
    training_table_printer_->print_footer();
    training_table_printer_.reset();
  }

  variant_map_type state_update;

  // Update the state with recall, precision and confusion matrix for training
  // data
  gl_sarray train_predictions = predict(training_data_, "probability_vector");
  flex_string target_column_name = read_state<flex_string>("target");
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

  state_update["verbose"] = read_state<bool>("verbose");
  add_or_update_state(state_update);
}

gl_sarray activity_classifier::predict(gl_sframe data,
                                       std::string output_type) {
  if (output_type.empty()) {
    output_type = "class";
  } else if (output_type != "class" && output_type != "probability_vector") {
    log_and_throw(output_type + " is not a valid option for output_type.  Expected one of: probability_vector, class");
  }

  // Bind the data to a data iterator.
  std::unique_ptr<data_iterator> data_it =
      create_iterator(data, /* requires_labels */ false,
                      /* infer_class_labels */ false, /* is_train */ false,
                      /* use_data_augmentation */ false);

  // Accumulate the class probabilities for each prediction window.
  gl_sframe raw_preds_per_window = perform_inference(data_it.get());

  // Assume output_frequency is "per_row". Duplicate each probability vector a
  // number of times equal to the number of samples for that prediction.
  size_t preds_column_index = raw_preds_per_window.column_index("preds");

  size_t num_samples_column_index =
      raw_preds_per_window.column_index("num_samples");
  auto copy_per_row = [=](const sframe_rows::row& row) {
    return flex_list(row[num_samples_column_index], row[preds_column_index]);
  };
  gl_sarray duplicated_preds_per_window =
      raw_preds_per_window.apply(copy_per_row, flex_type_enum::LIST);
  gl_sframe preds_per_row =
      gl_sframe({{"temp", duplicated_preds_per_window}}).stack("temp", "preds");

  gl_sarray result = preds_per_row["preds"];
  if (output_type == "class") {
    flex_list class_labels = read_state<flex_list>("classes");
    auto max_prob_label = [=](const flexible_type& ft) {
      const flex_vec& prob_vec = ft.get<flex_vec>();
      auto max_it = std::max_element(prob_vec.begin(), prob_vec.end());
      return class_labels[max_it - prob_vec.begin()];
    };
    result = result.apply(max_prob_label, class_labels.front().get_type());
  }
  return result;
}

gl_sframe activity_classifier::predict_per_window(gl_sframe data,
                                                  std::string output_type) {

  if (output_type.empty()) {
    output_type = "class";
  } else if (output_type != "class" && output_type != "probability_vector") {
    log_and_throw(output_type + " is not a valid option for output_type.  "
                                "Expected one of: probability_vector, class");
  }

  // Bind the data to a data iterator.
  std::unique_ptr<data_iterator> data_it =
      create_iterator(data, /* requires_labels */ false,
                      /* infer_class_labels */ false, /* is_train */ false,
                      /* use_data_augmentation */ false);

  // Accumulate the class probabilities for each prediction window.
  gl_sframe raw_preds_per_window = perform_inference(data_it.get());
  std::string session_id_column_name = read_state<std::string>("session_id");
  gl_sframe result =
      gl_sframe({{session_id_column_name, raw_preds_per_window["session_id"]},
                 {"prediction_id", raw_preds_per_window["prediction_id"]},
                 {"probability_vector", raw_preds_per_window["preds"]}});

  if (output_type == "class") {
    flex_list class_labels = read_state<flex_list>("classes");
    auto max_prob_label = [=](const flexible_type &ft) {
      const flex_vec &prob_vec = ft.get<flex_vec>();
      auto max_it = std::max_element(prob_vec.begin(), prob_vec.end());
      return class_labels[max_it - prob_vec.begin()];
    };

    result["probability_vector"] = result["probability_vector"].apply(
        max_prob_label, class_labels.front().get_type());
    result.rename({{"probability_vector", "class"}});
  }

  return result;
}

// TODO: This implementation would ideally just post-process the return value of
// predict.
gl_sframe activity_classifier::classify(gl_sframe data,
                                        std::string output_frequency) {
  if (output_frequency != "per_row" &&
             output_frequency != "per_window") {
    log_and_throw(output_frequency +
                  " is not a valid option for output_frequency.  "
                  "Expected one of 'per_row' or 'per_window'.");
  }

  // perform inference
  std::unique_ptr<data_iterator> data_it =
      create_iterator(data, /* requires_labels */ false,
                      /* infer_class_labels */ false, /* is_train */ false,
                      /* use_data_augmentation */ false);
  gl_sframe raw_preds_per_window = perform_inference(data_it.get());

  // lambda function for getting the max probability for each prediction
  auto max_prob = [](const flexible_type& ft) {
    const flex_vec& prob_vec = ft.get<flex_vec>();
    auto max_it = std::max_element(prob_vec.begin(), prob_vec.end());
    return prob_vec[max_it - prob_vec.begin()];
  };

  // lambda function for getting the class label with maximum probability for
  // each prediction
  flex_list class_labels = read_state<flex_list>("classes");
  auto max_prob_label = [=](const flexible_type& ft) {
    const flex_vec& prob_vec = ft.get<flex_vec>();
    auto max_it = std::max_element(prob_vec.begin(), prob_vec.end());
    return class_labels[max_it - prob_vec.begin()];
  };

  // get sarray with predicted class label and probability
  gl_sarray class_sarray = raw_preds_per_window["preds"].apply(
      max_prob_label, class_labels.front().get_type());
  gl_sarray prob_sarray =
      raw_preds_per_window["preds"].apply(max_prob, flex_type_enum::FLOAT);
  raw_preds_per_window.add_column(class_sarray, "class");
  raw_preds_per_window.add_column(prob_sarray, "probability");

  // create the result
  gl_sframe result = gl_sframe();
  if (output_frequency == "per_window") {
    std::string session_id_column_name = read_state<std::string>("session_id");
    result =
        gl_sframe({{session_id_column_name, raw_preds_per_window["session_id"]},
                   {"prediction_id", raw_preds_per_window["prediction_id"]},
                   {"class", raw_preds_per_window["class"]},
                   {"probability", raw_preds_per_window["probability"]}});
  } else {
    size_t class_column_index = raw_preds_per_window.column_index("class");
    size_t prob_column_index = raw_preds_per_window.column_index("probability");
    size_t num_samples_column_index =
        raw_preds_per_window.column_index("num_samples");

    auto copy_class_per_row = [=](const sframe_rows::row& row) {
      return flex_list(row[num_samples_column_index], row[class_column_index]);
    };
    auto copy_prob_per_row = [=](const sframe_rows::row& row) {
      return flex_list(row[num_samples_column_index], row[prob_column_index]);
    };

    gl_sarray duplicated_class_sarray =
        raw_preds_per_window.apply(copy_class_per_row, flex_type_enum::LIST);
    gl_sarray duplicated_prob_sarray =
        raw_preds_per_window.apply(copy_prob_per_row, flex_type_enum::LIST);
    gl_sframe class_per_row =
        gl_sframe({{"temp", duplicated_class_sarray}}).stack("temp", "class");
    gl_sframe prob_per_row = gl_sframe({{"temp", duplicated_prob_sarray}})
                                 .stack("temp", "probability");
    result.add_column(class_per_row["class"], "class");
    result.add_column(prob_per_row["probability"], "probability");
  }
  return result;
}

gl_sframe activity_classifier::predict_topk(gl_sframe data,
                                            std::string output_type, size_t k,
                                            std::string output_frequency) {
  // check valid input arguments
  if (output_type != "probability" && output_type != "rank") {
    log_and_throw(output_type +
                  " is not a valid option for output_type.  "
                  "Expected one of: probability, rank");
  }
  if (output_frequency != "per_row" && output_frequency != "per_window") {
    log_and_throw(output_frequency +
                  " is not a valid option for output_frequency.  "
                  "Expected one of: per_row, per_window");
  }

  // data inference
  std::unique_ptr<data_iterator> data_it =
      create_iterator(data, /* requires_labels */ false,
                      /* infer_class_labels */ false, /* is_train */ false,
                      /* use_data_augmentation */ false);
  gl_sframe raw_preds_per_window = perform_inference(data_it.get());

  // argsort probability to get the index (rank) for top k class
  // if k is greater than the class number, set it to be class number
  flex_list class_labels = read_state<flex_list>("classes");
  k = std::min(k, class_labels.size());
  auto argsort_prob = [=](const flexible_type& ft) {
    const flex_vec& prob_vec = ft.get<flex_vec>();
    std::vector<size_t> index_vec(prob_vec.size());
    std::iota(index_vec.begin(), index_vec.end(), 0);
    auto compare = [&](size_t i, size_t j) {
      return prob_vec[i] > prob_vec[j];
    };
    std::nth_element(index_vec.begin(), index_vec.begin() + k, index_vec.end(),
                     compare);
    std::sort(index_vec.begin(), index_vec.begin() + k, compare);
    return flex_list(index_vec.begin(), index_vec.begin() + k);
  };

  // store the index in a column "rank"
  raw_preds_per_window.add_column(
      raw_preds_per_window["preds"].apply(argsort_prob, flex_type_enum::LIST),
      "rank");

  // get top k class name and store in a column "class"
  size_t rank_column_index = raw_preds_per_window.column_index("rank");
  auto get_class_name = [=](const sframe_rows::row& row) {
    const flex_list& rank_list = row[rank_column_index];
    flex_list topk_class;
    for (const flexible_type& i : rank_list) {
      topk_class.push_back(class_labels[i]);
    }
    return topk_class;
  };
  raw_preds_per_window.add_column(
      raw_preds_per_window.apply(get_class_name, flex_type_enum::LIST),
      "class");

  // if output_type is "probability" then change rank to probabilty
  if (output_type == "probability") {
    size_t prob_column_index = raw_preds_per_window.column_index("preds");
    auto get_probability = [=](const sframe_rows::row& row) {
      const flex_list& rank_list = row[rank_column_index];
      flex_list topk_prob;
      for (const flexible_type& i : rank_list) {
        topk_prob.push_back(row[prob_column_index][i]);
      }
      return topk_prob;
    };
    raw_preds_per_window["rank"] =
        raw_preds_per_window.apply(get_probability, flex_type_enum::LIST);
  }

  // repeat "class" and "rank" column num_samples times if output_frequency is
  // per_row
  if (output_frequency == "per_row") {
    size_t class_column_index = raw_preds_per_window.column_index("class");
    size_t num_samples_column_index =
        raw_preds_per_window.column_index("num_samples");
    auto copy_per_row_class = [=](const sframe_rows::row& row) {
      return flex_list(row[num_samples_column_index], row[class_column_index]);
    };
    auto copy_per_row_rank = [=](const sframe_rows::row& row) {
      return flex_list(row[num_samples_column_index], row[rank_column_index]);
    };
    raw_preds_per_window["class"] =
        raw_preds_per_window.apply(copy_per_row_class, flex_type_enum::LIST);
    raw_preds_per_window["rank"] =
        raw_preds_per_window.apply(copy_per_row_rank, flex_type_enum::LIST);
  }

  // construct the final result
  gl_sframe result = gl_sframe();
  if (output_frequency == "per_row") {
    // stack data into a column with single element for each row
    gl_sframe stacked_class =
        gl_sframe({{"class", raw_preds_per_window["class"]}})
            .stack("class", "class");
    result.add_column(gl_sarray::from_sequence(0, stacked_class.size()),
                      "row_id");
    result.add_column(stacked_class["class"], "class");
    result = result.stack("class", "class");
    gl_sframe stacked_rank = gl_sframe({{"rank", raw_preds_per_window["rank"]}})
                                 .stack("rank", "rank");
    stacked_rank = stacked_rank.stack("rank", "rank");
    result.add_column(stacked_rank["rank"], "rank");
  } else {
    // add session ID and prediction ID if the output_frequency is per_window
    result.add_column(raw_preds_per_window["session_id"],
                      read_state<std::string>("session_id"));
    result.add_column(raw_preds_per_window["prediction_id"], "prediction_id");
    result.add_column(raw_preds_per_window["class"], "class");
    result = result.stack("class", "class");
    gl_sframe rank_per_row = gl_sframe({{"rank", raw_preds_per_window["rank"]}})
                                 .stack("rank", "rank");
    result.add_column(rank_per_row["rank"], "rank");
  }

  // change the column name rank to probability according to the output_type
  if (output_type == "probability") {
    result.rename({{"rank", "probability"}});
  }
  return result;
}

variant_map_type activity_classifier::evaluate(gl_sframe data,
                                               std::string metric)
{
  // Perform prediction.
  gl_sarray predictions = predict(data, "probability_vector");

  // Compute the requested metrics.
  std::string target_column_name = read_state<flex_string>("target");
  return evaluation::compute_classifier_metrics(
      data, target_column_name, metric, predictions,
      {{"classes", read_state<flex_list>("classes")}});
}


std::shared_ptr<MLModelWrapper> activity_classifier::export_to_coreml(
    std::string filename, std::string short_desc,
    std::map<std::string, flexible_type> additional_user_defined)
{
  std::shared_ptr<MLModelWrapper> model_wrapper =
      export_activity_classifier_model(
          *read_model_spec(), read_state<flex_int>("prediction_window"),
          read_state<flex_list>("features"), LSTM_HIDDEN_SIZE,
          read_state<flex_list>("classes"), read_state<flex_string>("target"));

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
  for(const auto& kvp : additional_user_defined) {
       user_defined_metadata.emplace_back(kvp.first, kvp.second);
  }

  model_wrapper->add_metadata({
      {"short_description", short_desc},
      {"user_defined", std::move(user_defined_metadata)},
  });

  if (!filename.empty()) {
    model_wrapper->save(filename);
  }

  return model_wrapper;
}

// Converts a model saved from the original Python (CustomModel) toolkit.
void activity_classifier::import_from_custom_model(
    variant_map_type model_data, size_t version) {

  // Extract the neural net weights from the model data.
  auto model_iter = model_data.find("_pred_model");
  if (model_iter == model_data.end()) {
    log_and_throw(
        "The loaded turicreate model must contain '_pred_model' field!");
  }
  const flex_dict& pred_model =
      variant_get_value<flex_dict>(model_iter->second);

  // The remaining model data should be interpreted as model attributes (state).
  state.clear();
  state.insert(model_data.begin(), model_data.end());

  // Migrate the MXNet weights from pred_model, which is a nested dictionary
  // with three layers. The weights we want are spread among two top-level keys:
  // "arg_params" and "aux_params". The mid-level keys are "data" and "shapes".
  // The bottom-level keys are the layer names.
  float_array_map nn_params;
  auto import_mxnet_params = [&](const flex_dict& params) {
    // Here, params is the value for either "arg_params" or "aux_params" in the
    // top-level dictionary. Extract the data dictionary and shape dictionary.
    flex_dict mxnet_data_dict;
    flex_dict mxnet_shape_dict;
    for (const auto& params_kv : params) {
      if (params_kv.first == "data") {
        mxnet_data_dict = params_kv.second;
      } else if (params_kv.first == "shapes") {
        mxnet_shape_dict = params_kv.second;
      }
    }

    // Sort the two dictionaries, assuming that they contain the same keys.
    auto cmp = [](const flex_dict::value_type& a, const flex_dict::value_type& b) {
      return a.first < b.first;
    };
    std::sort(mxnet_data_dict.begin(), mxnet_data_dict.end(), cmp);
    std::sort(mxnet_shape_dict.begin(), mxnet_shape_dict.end(), cmp);
    ASSERT_EQ(mxnet_data_dict.size(), mxnet_shape_dict.size());

    // Iterate through the shapes and data simultaneously, one layer at a time.
    for (size_t i = 0; i < mxnet_data_dict.size(); ++i) {
      // Copy/convert to std::vector of float (for data) and size_t (for shape).
      std::string name = mxnet_data_dict[i].first;
      flex_nd_vec mxnet_data_nd = mxnet_data_dict[i].second.to<flex_nd_vec>();
      flex_nd_vec mxnet_shape_nd = mxnet_shape_dict[i].second.to<flex_nd_vec>();
      const std::vector<double>& mxnet_data = mxnet_data_nd.elements();
      const std::vector<double>& mxnet_shape = mxnet_shape_nd.elements();
      std::vector<float> data(mxnet_data.size());
      std::vector<size_t> shape(mxnet_shape.size());
      std::copy(mxnet_data.begin(), mxnet_data.end(), data.begin());
      std::copy(mxnet_shape.begin(), mxnet_shape.end(), shape.begin());

      // Replace "moving" with "running" in layer names.
      auto moving_pos = name.find("moving");
      if (moving_pos != std::string::npos) {
        name.replace(moving_pos, 6, "running");
      }

      if (name.find("lstm") == 0) {
        // MXNet packs the parameters for a LSTM layer into a single block,
        // concatenating the input gate, forget gate, cell, and output gate
        // weights. CoreML expects these to be split into separate names.
        shape[0] /= 4;
        std::string prefix = name.substr(0, 8);  // "lstm_i2h" or "lstm_h2h"
        std::string suffix = name.substr(9);     // "weight" or "bias"
        size_t size = data.size() / 4;
        nn_params[prefix + "_i_" + suffix] = shared_float_array::wrap(
            std::vector<float>(data.begin(), data.begin() + size), shape);
        nn_params[prefix + "_f_" + suffix] = shared_float_array::wrap(
            std::vector<float>(data.begin() + size, data.begin() + size*2),
            shape);
        nn_params[prefix + "_c_" + suffix] = shared_float_array::wrap(
            std::vector<float>(data.begin() + size*2, data.begin() + size*3),
            shape);
        nn_params[prefix + "_o_" + suffix] = shared_float_array::wrap(
            std::vector<float>(data.begin() + size*3, data.end()), shape);
      } else {
        nn_params[name] =
            shared_float_array::wrap(std::move(data), std::move(shape));
      }
    }
  };

  // Migrate all the weights found under the "arg_params" or "aux_params" keys.
  for (const auto& pred_model_kv : pred_model) {
    const std::string& key = pred_model_kv.first;
    if (key == "arg_params" || key == "aux_params") {
      import_mxnet_params(pred_model_kv.second.get<flex_dict>());
    }
  }

  // Load the migrated weights.
  bool use_random_init = false;
  nn_spec_ = init_model(use_random_init);
  nn_spec_->update_params(nn_params);
  nn_spec_synchronized_ = true;
  model_data.erase(model_iter);
}

std::unique_ptr<data_iterator> activity_classifier::create_iterator(
    gl_sframe data, bool requires_labels, bool infer_class_labels,
    bool is_train, bool use_data_augmentation) const {
  data_iterator::parameters data_params;
  data_params.data = std::move(data);

  if (!infer_class_labels) {
    data_params.class_labels = read_state<flex_list>("classes");
  }

  data_params.is_train = is_train;
  if (requires_labels) {
    data_params.target_column_name = read_state<flex_string>("target");
  }
  data_params.session_id_column_name = read_state<flex_string>("session_id");
  flex_list features = read_state<flex_list>("features");
  data_params.feature_column_names =
      std::vector<std::string>(features.begin(), features.end());
  data_params.prediction_window = read_state<flex_int>("prediction_window");
  data_params.predictions_in_chunk = NUM_PREDICTIONS_PER_CHUNK;
  data_params.use_data_augmentation = use_data_augmentation;
  if (use_data_augmentation) {
    data_params.random_seed = read_state<int>("random_seed");
  } else {
    data_params.random_seed = 0;
  }
  return std::unique_ptr<data_iterator>(new simple_data_iterator(data_params));
}

std::unique_ptr<compute_context> activity_classifier::create_compute_context()
    const {
  return compute_context::create();
}

std::unique_ptr<model_spec> activity_classifier::init_model(
    bool use_random_init) const {
  flex_string target = read_state<flex_string>("target");
  size_t num_classes = read_state<flex_int>("num_classes");
  size_t prediction_window = read_state<flex_int>("prediction_window");
  const flex_list &features_list = read_state<flex_list>("features");
  std::vector<std::string> features(features_list.begin(), features_list.end());
  int seed = 0;
  if (use_random_init) {
    seed = read_state<int>("random_seed");
  }
  return init_model(target, features, prediction_window, num_classes,
                    use_random_init, seed);
}

// static
std::unique_ptr<model_spec> activity_classifier::init_model(
    const std::string& target, const std::vector<std::string>& features,
    size_t prediction_window, size_t num_classes, bool use_random_init,
    int random_seed) {
  std::unique_ptr<model_spec> result(new model_spec);

  // Only to create a random engine for weight initialization if use_random_init
  // = true
  std::mt19937 random_engine;
  if (use_random_init) {
    std::seed_seq seed_seq{random_seed};
    random_engine = std::mt19937(seed_seq);
  }

  if (features.size() == 1) {
    // Single feature column
    const std::string& feature_column_name = features.front();
    std::string single_input_feature{feature_column_name};
    result->add_reshape("reshape", single_input_feature,
                        {{1, features.size(), 1, prediction_window}});
  } else {
    // Multiple feature columns. Add concatenate layer to concat all columns.
    result->add_channel_concat("features", features);
    result->add_reshape("reshape", "features",
                        {{1, features.size(), 1, prediction_window}});
  }

  weight_initializer initializer = zero_weight_initializer();
  lstm_weight_initializers lstm_initializer =
      lstm_weight_initializers::create_with_zero();

  if (use_random_init) {
    initializer = xavier_weight_initializer(
        features.size() * prediction_window,
        NUM_CONV_FILTERS * prediction_window, &random_engine);
  }
  result->add_convolution(
      /* name                */ "conv",
      /* input               */ "reshape",
      /* num_output_channels */ NUM_CONV_FILTERS,
      /* num_kernel_channels */ features.size(),
      /* kernel_height       */ 1,
      /* kernel_width        */ prediction_window,
      /* stride_height       */ 1,
      /* stride_width        */ prediction_window,
      /* padding             */ padding_type::VALID,
      /* weight_init_fn      */ initializer,
      /* bias_init_fn        */ zero_weight_initializer());
  result->add_relu("relu1", "conv");

  result->add_channel_slice("hiddenIn","stateIn",0,LSTM_HIDDEN_SIZE,1);
  result->add_channel_slice("cellIn","stateIn",LSTM_HIDDEN_SIZE,LSTM_HIDDEN_SIZE*2,1);

  if (use_random_init) {
    lstm_initializer = lstm_weight_initializers::create_with_xavier_method(
        NUM_CONV_FILTERS, LSTM_HIDDEN_SIZE, &random_engine);
  }
  result->add_lstm(
      /* name                */ "lstm",
      /* input               */ "relu1",
      /* hidden_input        */ "hiddenIn",
      /* cell_input          */ "cellIn",
      /* hidden_output       */ "hiddenOut",
      /* cell_output         */ "cellOut",
      /* input_vector_size   */ NUM_CONV_FILTERS,
      /* output_vector_size  */ LSTM_HIDDEN_SIZE,
      /* cell_clip_threshold */ LSTM_CELL_CLIP_THRESHOLD,
      /* initializers        */ lstm_initializer);
  result->add_channel_concat("stateOut",{"hiddenOut","cellOut"});

  if (use_random_init) {
    initializer = xavier_weight_initializer(
        LSTM_HIDDEN_SIZE, FULLY_CONNECTED_HIDDEN_SIZE, &random_engine);
  }
  result->add_inner_product(
      /* name                */ "dense0",
      /* input               */ "lstm",
      /* num_output_channels */ FULLY_CONNECTED_HIDDEN_SIZE,
      /* num_input_channels  */ LSTM_HIDDEN_SIZE,
      /* weight_init_fn      */ initializer,
      /* bias_init_fn        */ zero_weight_initializer());
  result->add_batchnorm("bn", "dense0", FULLY_CONNECTED_HIDDEN_SIZE, 0.001f);
  result->add_relu("relu6", "bn");

  if (use_random_init) {
    initializer = xavier_weight_initializer(FULLY_CONNECTED_HIDDEN_SIZE,
                                            num_classes, &random_engine);
  }
  result->add_inner_product(
      /* name                */ "dense1",
      /* input               */ "relu6",
      /* num_output_channels */ num_classes,
      /* num_input_channels  */ FULLY_CONNECTED_HIDDEN_SIZE,
      /* weight_init_fn      */ initializer);
  result->add_softmax(target + "Probability", "dense1");

  return result;
}

std::tuple<gl_sframe, gl_sframe>
activity_classifier::init_data(gl_sframe data, variant_type validation_data,
                               std::string session_id_column_name) const {
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
  else if ((variant_is<flex_string>(validation_data)) && (variant_get_value<flex_string>(validation_data)=="auto")) {
    gl_sarray unique_session = data[session_id_column_name].unique();
    size_t seed = read_state<size_t>("random_seed");
    if (unique_session.size() >= 200000) {
      // TODO: Expose seed parameter publicly.
      float p = 10000.0 / unique_session.size();
      std::tie(train_data, val_data) =
          random_split_by_session(data, session_id_column_name, p, seed);
    } else if (unique_session.size() >= 200) {
      std::tie(train_data, val_data) =
          random_split_by_session(data, session_id_column_name, 0.95, seed);
    } else if (unique_session.size() >= 50) {
      std::tie(train_data, val_data) =
          random_split_by_session(data, session_id_column_name, 0.90, seed);
    } else {
      train_data = data;
      std::cout << "The dataset has less than the minimum of 50 sessions required for train-validation split. "
                       "Continuing without validation set.\n";
    }
  } else {
    train_data = data;
  }
  return std::make_tuple(train_data,val_data);
}

void activity_classifier::init_training(
    gl_sframe data, std::string target_column_name,
    std::string session_id_column_name, variant_type validation_data,
    std::map<std::string, flexible_type> opts) {
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

  auto show_loss_it = opts.find("_show_loss");
  if (show_loss_it != opts.end()) {
    show_loss_ = show_loss_it->second;
    opts.erase(show_loss_it);
  }

  // Read user-specified options.
  init_options(opts);

  // Choose a random seed if not set.
  if (read_state<flexible_type>("random_seed") == FLEX_UNDEFINED) {
    std::random_device random_device;
    int random_seed = static_cast<int>(random_device());
    add_or_update_state({{"random_seed", random_seed}});
  }

  // Perform validation split if necessary.
  std::tie(training_data_, validation_data_) =
      init_data(data, validation_data, session_id_column_name);

  // Begin printing progress.
  init_table_printer(!validation_data_.empty());

  add_or_update_state({{"session_id", session_id_column_name},
                       {"target", target_column_name},
                       {"features", flex_list(feature_column_names.begin(),
                                              feature_column_names.end())}});

  // Bind the data to a data iterator.
  bool use_data_augmentation = read_state<bool>("use_data_augmentation");
  training_data_iterator_ =
      create_iterator(training_data_, /* requires_labels */ true,
                      /* infer_class_labels */ true, /* is_train */ true,
                      use_data_augmentation);

  add_or_update_state({{"classes", training_data_iterator_->class_labels()}});

  // Bind the validation data to a data iterator.
  if (!validation_data_.empty()) {
    validation_data_iterator_ =
        create_iterator(validation_data_, /* requires_labels */ true,
                        /* infer_class_labels */ false, /* is_train */ false,
                        /* use_data_augmentation */ false);
  } else {
    validation_data_iterator_ = nullptr;
  }

  // Instantiate the compute context.
  training_compute_context_ = create_compute_context();
  if (training_compute_context_ == nullptr) {
    log_and_throw("No neural network compute context provided");
  }

  training_compute_context_->print_training_device_info();

  // Set additional model fields.
  add_or_update_state({
      {"features", training_data_iterator_->feature_names()},
      {"num_classes", training_data_iterator_->class_labels().size()},
      {"num_examples", training_data_.size()},
      {"num_features", training_data_iterator_->feature_names().size()},
      {"num_sessions", training_data_iterator_->num_sessions()},
      {"training_iterations", 0},
  });

  // Initialize the neural net. Note that this depends on statistics computed by
  // the data iterator.
  bool use_random_init = true;
  nn_spec_ = init_model(use_random_init);
  nn_spec_synchronized_ = true;

  // Defining the struct for ac parameters
  ac_parameters ac_params;
  ac_params.batch_size = read_state<int>("batch_size");
  ac_params.num_features = read_state<int>("num_features");
  ac_params.prediction_window = read_state<int>("prediction_window");
  ac_params.num_classes = read_state<int>("num_classes");
  ac_params.num_predictions_per_chunk = NUM_PREDICTIONS_PER_CHUNK;
  ac_params.random_seed = read_state<int>("random_seed");
  ac_params.is_training = true;
  ac_params.weights = read_model_spec()->export_params_view();

  // Instantiate the NN backend.
  training_model_ =
      training_compute_context_->create_activity_classifier(ac_params);

  // Print the header last, after any logging triggered by initialization above.
  if (training_table_printer_) {
    training_table_printer_->print_header();
  }
}

void activity_classifier::resume_training(gl_sframe data,
                                          variant_type validation_data) {
  // Perform validation split if necessary.
  flex_string session_id_column_name = read_state<flex_string>("session_id");
  std::tie(training_data_, validation_data_) =
      init_data(data, validation_data, session_id_column_name);

  // Begin printing progress.
  init_table_printer(!validation_data_.empty());

  // Bind the data to a data iterator.
  bool use_data_augmentation = read_state<bool>("use_data_augmentation");
  training_data_iterator_ =
      create_iterator(training_data_, /* requires_labels */ true,
                      /* infer_class_labels */ false, /* is_train */ true,
                      use_data_augmentation);

  // Bind the validation data to a data iterator.
  if (!validation_data_.empty()) {
    validation_data_iterator_ =
        create_iterator(validation_data_, /* requires_labels */ true,
                        /* infer_class_labels */ false, /* is_train */ false,
                        /* use_data_augmentation */ false);
  } else {
    validation_data_iterator_ = nullptr;
  }

  // Instantiate the compute context.
  training_compute_context_ = create_compute_context();
  if (training_compute_context_ == nullptr) {
    log_and_throw("No neural network compute context provided");
  }

  training_compute_context_->print_training_device_info();

  // Defining the struct for ac parameters
  ac_parameters ac_params;
  ac_params.batch_size = read_state<int>("batch_size");
  ac_params.num_features = read_state<int>("num_features");
  ac_params.prediction_window = read_state<int>("prediction_window");
  ac_params.num_classes = read_state<int>("num_classes");
  ac_params.num_predictions_per_chunk = NUM_PREDICTIONS_PER_CHUNK;
  ac_params.random_seed = read_state<int>("random_seed");
  ac_params.is_training = true;
  ac_params.weights = read_model_spec()->export_params_view();

  // Instantiate the NN backend.
  training_model_ =
      training_compute_context_->create_activity_classifier(ac_params);

  // Print the header last, after any logging triggered by initialization above.
  if (training_table_printer_) {
    training_table_printer_->print_header();
  }
}

void activity_classifier::iterate_training() {
  // Training must have been initialized.
  ASSERT_TRUE(training_data_iterator_ != nullptr);
  ASSERT_TRUE(training_model_ != nullptr);

  // Invalidate any local copy of the model.
  nn_spec_synchronized_ = false;

  const size_t batch_size = read_state<flex_int>("batch_size");
  const size_t iteration_idx = read_state<flex_int>("training_iterations");

  float cumulative_batch_loss = 0.f;
  size_t num_batches = 0;
  size_t train_num_correct = 0;
  size_t train_num_samples = 0;
  size_t num_classes = read_state<size_t>("num_classes");
  size_t prediction_window = read_state<size_t>("prediction_window");


  // To support double buffering, use a queue of pending inference results.
  std::queue<result> pending_batches;

  auto pop_until_size = [&](size_t remaining) {

    while (pending_batches.size() > remaining) {

      // Pop one batch from the queue.
      result batch = pending_batches.front();
      pending_batches.pop();

      size_t batch_num_correct = 0;
      size_t batch_num_samples = 0;
      std::tie(batch_num_correct, batch_num_samples) =
          cumulative_chunk_accuracy(prediction_window, num_classes,
                                    batch.output_info, batch.data_info);
      train_num_correct += batch_num_correct;
      train_num_samples += batch_num_samples;

      float batch_loss = std::accumulate(batch.loss_info.data(),
                                         batch.loss_info.data() + batch.loss_info.size(),
                                         0.f, std::plus<float>());

      cumulative_batch_loss += batch_loss / batch.data_info.batch_info.size();

    }
  };


  while (training_data_iterator_->has_next_batch()) {

    // Wait until we have just one asynchronous batch outstanding. The work
    // below should be concurrent with the neural net inference for that batch.
    pop_until_size(1);

    result result_batch;
    result_batch.data_info = training_data_iterator_->next_batch(batch_size);;

    // Submit the batch to the neural net model.
    std::map<std::string, shared_float_array> results = training_model_->train(
        { { "input",   result_batch.data_info.features },
          { "labels",  result_batch.data_info.labels   },
          { "weights", result_batch.data_info.weights  }, });
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
        compute_validation_metrics(prediction_window, num_classes, batch_size);
  }
  add_or_update_state({
      {"training_iterations", iteration_idx + 1},
      {"training_accuracy", average_batch_accuracy},
      {"training_log_loss", average_batch_loss},
  });

  if (validation_data_iterator_) {
    add_or_update_state({
        {"validation_accuracy", average_val_accuracy},
        {"validation_log_loss", average_val_loss}
    });
  }

  if (training_table_printer_) {
    if (validation_data_iterator_) {
      if (show_loss_) {
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
      if (show_loss_) {
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
}

gl_sframe activity_classifier::perform_inference(data_iterator *data) const {
  // Open a new SFrame for writing.
  gl_sframe_writer writer(
      {"session_id", "prediction_id", "preds", "num_samples"},
      {data->session_id_type(), flex_type_enum::INTEGER, flex_type_enum::VECTOR,
       flex_type_enum::INTEGER},
      /* num_segments */ 1);

  int prediction_window = read_state<int>("prediction_window");
  int num_classes = read_state<int>("num_classes");

  // Allocate a buffer into which to write the class probabilities.
  flex_vec preds(num_classes);

  // Defining the struct for ac parameters
  ac_parameters ac_params;
  ac_params.batch_size = read_state<int>("batch_size");
  ac_params.num_features = read_state<int>("num_features");
  ac_params.prediction_window = read_state<int>("prediction_window");
  ac_params.num_classes = num_classes;
  ac_params.num_predictions_per_chunk = NUM_PREDICTIONS_PER_CHUNK;
  ac_params.random_seed = read_state<int>("random_seed");
  ac_params.is_training = false;
  ac_params.weights = read_model_spec()->export_params_view();

  // Initialize the NN backend.
  std::unique_ptr<compute_context> ctx = create_compute_context();
  std::unique_ptr<model_backend> backend =
      ctx->create_activity_classifier(ac_params);

  // To support double buffering, use a queue of pending inference results.
  std::queue<result> pending_batches;

  auto pop_until_size = [&](size_t remaining) {

    while (pending_batches.size() > remaining) {

      // Pop one batch from the queue.
      result batch = pending_batches.front();
      pending_batches.pop();

      for (size_t i = 0; i < batch.data_info.batch_info.size(); ++i) {

      shared_float_array output_chunk = batch.output_info[i];
      data_iterator::batch::chunk_info info = batch.data_info.batch_info[i];

      // Interpret the NN output as a sequence of NUM_PREDICTIONS_PER_CHUNK
      // probability vectors.
      ASSERT_EQ(output_chunk.size(), NUM_PREDICTIONS_PER_CHUNK * num_classes);

      const float* output_ptr = output_chunk.data();
      size_t cumulative_samples = 0;

        while (cumulative_samples < info.num_samples) {

          // Copy the probability vector for this prediction.
          std::copy(output_ptr, output_ptr + num_classes, preds.begin());

          output_ptr += num_classes;

          // Compute how many samples this prediction applies to.
          size_t num_samples = std::min<size_t>(prediction_window,
                                                info.num_samples - cumulative_samples);
          cumulative_samples += prediction_window ;
          
          // Add a row to the output SFrame.
          flex_int prediction_id = info.chunk_index;
          writer.write({info.session_id, prediction_id, preds, num_samples},
                       /* segment_id */ 0);
        }
      }
    }
  };

  while (data->has_next_batch()) {

    // Wait until we have just one asynchronous batch outstanding. The work
    // below should be concurrent with the neural net inference for that batch.
    pop_until_size(1);

    result result_batch;
    result_batch.data_info = data->next_batch(read_state<flex_int>("batch_size"));;

    // Send the inputs to the model.
    std::map<std::string, shared_float_array> results =
        backend->predict({ { "input", result_batch.data_info.features } });

    // Copy the (float) outputs to our (double) buffer and add to the SArray.
    result_batch.output_info = results.at("output");

    pending_batches.push(std::move(result_batch));

  }

  pop_until_size(0);

  return writer.close();
}
}  // namespace activity_classification
}  // namespace turi

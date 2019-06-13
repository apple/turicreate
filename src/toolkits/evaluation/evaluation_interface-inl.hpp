/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_EVAL_INTERFACE_H_
#define TURI_EVAL_INTERFACE_H_
// Types
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <model_server/lib/variant.hpp>
#include <unordered_map>

#ifdef __clang__
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Woverloaded-virtual"  // TODO: fix these issues below
#endif

const double EVAL_ZERO = 1.0e-9;

namespace turi {
namespace evaluation {


/**
 * An enumeration over the possible types of multi-class averaging
 * that we support.
 *
 * \see average_type_from_name
 */
enum class average_type_enum: char {
  NONE    = 0,        /**< No averaging, return all. */
  MICRO   = 1,        /**< Use global counts.*/
  MACRO   = 2,        /**< Average per-class stats. */
  DEFAULT = 3,        /**< The default behavior . */
};

/**
 * Given the printable name of a average_type_enum type, it returns the name.
 *
 * \param[in] name Name of the average_type_enum type.
 * \returns average_type_enum
 */
inline average_type_enum average_type_enum_from_name(const flexible_type& name) {
  static std::unordered_map<flexible_type, average_type_enum> type_map {
    {FLEX_UNDEFINED, average_type_enum::NONE},
    {flexible_type("micro"), average_type_enum::MICRO},
    {flexible_type("default"), average_type_enum::DEFAULT},
    {flexible_type("macro"), average_type_enum::MACRO}
  };
  auto it = type_map.find(name);
  if (it == type_map.end()) {
    log_and_throw(
      std::string("Invalid average type name " + name.to<std::string>() + ".")
    );
  }
  return it->second;
}

/**
 * Hash for a pair of flexible types (needed for insertion into an unordered_map)
 */
struct flex_pair_hash {
public:
  std::size_t operator()(const std::pair<flexible_type, flexible_type> &x) const {
    return hash64_combine(x.first.hash(), x.second.hash());
  }
};


/**
 * Get the "highest" label as the reference label.
 */
inline flexible_type get_reference_label(
                const std::unordered_set<flexible_type>& labels) {

  // First find atleast 1 label that isn't a None
  flexible_type ret;
  for (const auto& l : labels) {
    if (l != FLEX_UNDEFINED) {
      ret = l;
      break;
    }
  }
  // Now find the "max" label.
  for (const auto& l : labels) {
    if (l != FLEX_UNDEFINED) {
      if (ret < l) {
       ret = l;
      }
    }
  }
  return ret;
}


/**
 * Perform an None save average.
*/
inline flexible_type average_with_none_skip(
    std::unordered_map<flexible_type, flexible_type> scores) {

  double average = 0.0;
  size_t tot_classes = 0;
  for (const auto& sc: scores) {
    if (sc.second != FLEX_UNDEFINED) {
      average +=  sc.second.get<double>();
      tot_classes += 1;
    }
  }

  // If every value is a None, then return None.
  if (tot_classes == 0) {
    return FLEX_UNDEFINED;
  } else {
    return flex_float(average / tot_classes);
  }
}

/**
 * Check that probabilities are in the range [0, 1].
 *
*/
inline void check_probability_range(const double& pred) {
  if ((pred < 0 - EVAL_ZERO) || (pred > (1 + EVAL_ZERO))) {
    log_and_throw("Prediction scores/probabilities are expected to be "
         "in the range [0, 1]. If they aren't, try normalizing them.");
  }
}

/**
 * Check undefined.
*/
inline void check_undefined(const flexible_type& pred) {
  if (pred.get_type() == flex_type_enum::UNDEFINED) {
    log_and_throw("Prediction scores/probabilities cannot contain missing "
         "values (i.e None values). Try removing them with 'dropna'.");
  }
}

/**
 * Compute precision (returns None when not defined)
 *
 * \param[in] tp True positives.
 * \param[in] fp False positives.
*/
inline flexible_type compute_precision_score(size_t tp, size_t fp) {
  if (tp + fp > 0) {
    return double(tp)/(tp + fp);
  } else {
    return FLEX_UNDEFINED;
  }
}

/**
 * Compute recall (returns None when not defined)
 *
 * \param[in] tp True positives.
 * \param[in] fn False negatives.
*/
inline flexible_type compute_recall_score(size_t tp, size_t fn) {
  if (tp + fn > 0) {
    return double(tp)/(tp + fn);
  } else {
    return FLEX_UNDEFINED;
  }
}

/**
 * Compute fbeta_score (returns None when not defined)
 *
 * \param[in] tp True positives.
 * \param[in] fp False positives.
 * \param[in] fn False negatives.
*/
inline flexible_type compute_fbeta_score(
    size_t tp, size_t fp, size_t fn, double beta) {

  flexible_type pr = compute_precision_score(tp, fp);
  flexible_type rec = compute_recall_score(tp, fn);

  if (pr == FLEX_UNDEFINED) {
    return rec;
  }
  if (rec == FLEX_UNDEFINED) {
    return pr;
  }

  double pr_d = pr.get<double>();
  double rec_d = rec.get<double>();
  double denom = std::max(1e-20, beta * beta * pr_d + rec_d);
  return (1.0 + beta * beta) * (pr_d * rec_d) / denom;
}

/**
 *
 * Interface for performing evaluation in a streaming manner for supervised
 * learning.
 *
 * Background: Evaluation
 * --------------------------------------------------------------------------
 *
 *  An evaluation that can be computed in a streaming manner. All it needs is
 *  an aggregation over a sequence of individual statistics computed
 *  from individual evaluations.
 *
 *
 * What we need for a supervised evaluation scheme.
 * ---------------------------------------------------------------
 *
 * The interface makes sure that you can implement various types of streaming
 * evaluations.
 *
 * Each standardization scheme requires the following methods:
 *
 * *) init: Initialize the state
 *
 * *) register_example: Register a label and a prediction
 *
 * *) get_metric: Final transformation required. eg. square root for rmse.
 *
 *
*/
class supervised_evaluation_interface {

  public:

  /**
   * Default destructor.
   */
  virtual ~supervised_evaluation_interface() = default;

  /**
  * Default constructor.
  */
  supervised_evaluation_interface() = default;

  /**
   * Name of the evaluator.
   */
  virtual std::string name() const = 0;

  /**
   * Init the state with n_threads.
   *
   * \param[in] n_threads Number of threads.
   */
  virtual void init(size_t _n_threads = 1) = 0;

  /**
   * Returns true of this evaluator works on probabilities/scores (vs)
   * classes.
   */
  virtual bool is_prob_evaluator() const {
    return false;
  }

  /**
   * Returns true of this evaluator can be displayed as a single float value.
   */
  virtual bool is_table_printer_compatible() const {
    return true;
  }


  /**
   * Register a (target, prediction) pair
   *
   * \param[in] target     Target of a simple example.
   * \param[in] prediction Prediction of a single example.
   * \param[in] thread_id  Thread id registering this example.
   *
   */
  virtual void register_example(const flexible_type& target,
                                const flexible_type& prediction,
                                size_t thread_id = 0) = 0;

  /**
   * Register an unmapped (target, prediction) pair. Use this for performance
   * only. Here the target and prediction are assumed to be integers to avoid
   * flexible_type comparisons and flexible_type hashing.
   *
   * \param[in] target     Target of a simple example.
   * \param[in] prediction Prediction of a single example.
   * \param[in] thread_id  Thread id registering this example.
   *
   */
  virtual void register_unmapped_example(
                                const size_t& target,
                                const size_t& prediction,
                                size_t thread_id = 0) {
    register_example(target, prediction, thread_id);
  }

  /**
   * Init the state with a variant type.
   *
   * \param[in] _state Starting state.
   *
   */
  virtual variant_type get_metric() = 0;

};

/**
 * Computes the RMSE between two SArrays.
 *
 * \sqrt((1/N) \sum_{i=1}^N (targets[i] - predictions[i])^2)
 *
 */
class rmse: public supervised_evaluation_interface {

  private:

  size_t n_threads;
  std::vector<double> mse;
  std::vector<size_t> num_examples;

  public:

  /**
   * Name of the evaluator.
   */
  std::string name() const {
    return (std::string)("rmse");
  }

  /**
   * Init the state with a variant type.
   */
  void init(size_t _n_threads = 1){
    n_threads = _n_threads;
    mse.resize(n_threads);
    num_examples.resize(n_threads);
    for(size_t i = 0; i < n_threads; i++){
      mse[i] = 0;
      num_examples[i] = 0;
    }
  }

  /**
   * Register a (target, prediction) pair
   *
   * \param[in] target     Target of a simple example.
   * \param[in] prediction Prediction of a single example.
   * \param[in] thread_id  Thread id
   *
   */
  void register_example(const flexible_type& target,
                                const flexible_type& prediction,
                                size_t thread_id = 0){
    DASSERT_TRUE(thread_id < n_threads);

    // See http://www.johndcook.com/standard_deviation.html
    // Mk = Mk-1+ (xk - Mk-1)/k
    double a = (double)prediction - (double)target;
    num_examples[thread_id]++;
    mse[thread_id] += (a * a - mse[thread_id]) / num_examples[thread_id];
  }

  /**
   * Return the final metric.
   */
  variant_type get_metric() {
    double rmse = 0;
    size_t total_examples = 0;
    for(size_t i = 0; i < n_threads; i++){
      rmse += num_examples[i] * mse[i];
      total_examples += num_examples[i];
    }
    DASSERT_TRUE(total_examples > 0);
    DASSERT_TRUE(rmse >= 0);
    return to_variant(sqrt(rmse/total_examples));
  }

};


/**
 * Computes the worst case errors between two SArrays.
 */
class max_error: public supervised_evaluation_interface {

  private:

  size_t n_threads;
  std::vector<double> max_error;

  public:

  /**
   * Name of the evaluator.
   */
  std::string name() const {
    return (std::string)("max_error");
  }

  /**
   * Init the state with a variant type.
   */
  void init(size_t _n_threads = 1){
    n_threads = _n_threads;
    max_error.resize(n_threads);
    for (size_t i = 0; i < n_threads; i++) {
      max_error[i] = 0.0;
    }
  }

  /**
   * Register a (target, prediction) pair
   *
   * \param[in] target     Target of a simple example.
   * \param[in] prediction Prediction of a single example.
   * \param[in] thread_id  Thread id
   *
   */
  void register_example(const flexible_type& target,
                        const flexible_type& prediction,
                        size_t thread_id = 0){
    DASSERT_TRUE(thread_id < n_threads);
    double err = (double)prediction - (double)target;
    max_error[thread_id] = std::max(std::abs(err), max_error[thread_id]);
  }

  /**
   * Return the final metric.
   */
  variant_type get_metric() {
    double max_max_error = 0;
    for(size_t i = 0; i < n_threads; i++){
      max_max_error = std::max(max_max_error, max_error[i]);
    }
    return to_variant(max_max_error);
  }

};


class multiclass_logloss: public supervised_evaluation_interface {

  private:

  size_t n_threads;
  std::vector<double> logloss;
  std::vector<size_t> num_examples;
  std::unordered_map<flexible_type, size_t> m_index_map;
  size_t num_classes = size_t(-1);

  public:

  /**
   * Constructor.
   */
  multiclass_logloss(
      const std::unordered_map<flexible_type, size_t>& index_map,
      size_t num_classes = size_t(-1)) {
    m_index_map = index_map;
    if (num_classes == size_t(-1)) {
      this->num_classes = index_map.size();
    } else {
      this->num_classes = num_classes;
    }
  }

  /**
   * Returns true of this evaluator works on probabilities/scores (vs)
   * classes.
   */
  bool is_prob_evaluator() const {
    return true;
  }

  /**
   * Name of the evaluator.
   */
  std::string name() const {
    return (std::string)("multiclass_logloss");
  }


  /**
   * Init the state with a variant type.
   */
  void init(size_t _n_threads = 1){
    n_threads = _n_threads;
    logloss.resize(n_threads);
    num_examples.resize(n_threads);
    for(size_t i = 0; i < n_threads; i++){
      logloss[i] = 0;
      num_examples[i] = 0;
    }
  }

  /**
   * Register a (target, prediction) pair that are unmapped
   *
   * \param[in] target     target of a simple example.
   * \param[in] prediction prediction of a single example.
   * \param[in] thread_id  thread id
   *
   * \note Use this for performance because it does not perform a
   *       a flexible_type compare.
   */
  void register_unmapped_example(const size_t& target,
                                 const std::vector<double>& prediction,
                                 size_t thread_id = 0){
    DASSERT_TRUE(thread_id < n_threads);

    // If the class provided is a "new" class then treat the probability as 0.0;
    double pred = 0.0;
    if (target < prediction.size()) {
      pred = prediction[target];
    }
    num_examples[thread_id]++;
    check_probability_range(pred);
    logloss[thread_id] += log(
        std::max(std::min(1.0 - EVAL_ZERO, pred), EVAL_ZERO));
  }

  /**
   * Register a (target, prediction) pair.
   *
   * \param[in] target     target of a simple example.
   * \param[in] prediction prediction of a single example.
   * \param[in] thread_id  thread id
   *
   */
  void register_example(const flexible_type& target,
                        const flexible_type& prediction,
                        size_t thread_id = 0){
    DASSERT_TRUE(thread_id < n_threads);
    num_examples[thread_id]++;


    // Error out!
    if(prediction.size() != this->num_classes) {
      std::stringstream ss;
      ss << "Size of prediction probability vector"
         << "(" << prediction.size() << ") != number of classes"
         << "(" << m_index_map.size() << ")." << std::endl;
      log_and_throw(ss.str());
    }

    // If the class provided is a "new" class then treat the probability as 0.0;
    auto it = m_index_map.find(target);
    size_t label = 0;
    double pred = 0.0;
    if (it != m_index_map.end()) {
      label = size_t(it->second);
      const flex_vec& preds = prediction.get<flex_vec>();
      // Check that the new class was a class obtained in training.
      if (label < preds.size()) {
        pred = preds[label];
      }
    }

    check_probability_range(pred);
    logloss[thread_id] += log(
        std::max(std::min(1.0 - 1e-15, pred), 1e-15));
  }

  /**
   * Return the final metric.
   */
  variant_type get_metric() {
    double total_logloss = 0;
    size_t total_examples = 0;
    for(size_t i = 0; i < n_threads; i++){
      total_logloss += logloss[i];
      total_examples += num_examples[i];
    }
    DASSERT_TRUE(total_examples > 0);

    total_examples = std::max<size_t>(1, total_examples);
    return to_variant(-total_logloss / total_examples);
  }

};

class binary_logloss: public supervised_evaluation_interface {

  private:

  size_t n_threads;
  std::vector<double> logloss;
  std::vector<size_t> num_examples;
  std::unordered_map<flexible_type, size_t> index_map;

  public:

  /**
   * Constructor.
   *
   * \param[in] index_map Dictionary from flexible_type -> size_t for classes.
   */
  binary_logloss(
      std::unordered_map<flexible_type, size_t> index_map =
                  std::unordered_map<flexible_type, size_t>()) {
    this->index_map = index_map;
  }

  /**
   * Name of the evaluator.
   */
  std::string name() const override {
    return (std::string)("binary_logloss");
  }


  /**
   * Returns true of this evaluator works on probabilities/scores (vs)
   * classes.
   */
  bool is_prob_evaluator() const override {
    return true;
  }

  /**
   * Init the state with a variant type.
   */
  void init(size_t _n_threads = 1) override {
    n_threads = _n_threads;
    logloss.resize(n_threads);
    num_examples.resize(n_threads);
    for(size_t i = 0; i < n_threads; i++){
      logloss[i] = 0;
      num_examples[i] = 0;
    }
  }

  /**
   * Register a (target, prediction) pair that are unmapped
   *
   * \param[in] target     target of a simple example.
   * \param[in] prediction prediction of a single example.
   * \param[in] thread_id  thread id
   *
   * \note Use this for performance because it does not perform a
   *       a flexible_type compare.
   */
  void register_unmapped_example(const size_t& target,
                                 const double& prediction,
                                 size_t thread_id = 0) {
    DASSERT_TRUE(target == 0 || target == 1);
    DASSERT_TRUE(thread_id < n_threads);
    num_examples[thread_id]++;
    check_probability_range(prediction);
    logloss[thread_id] +=
      log(target !=0 ? std::max(prediction, EVAL_ZERO) :
                       std::max(1.0 - prediction, EVAL_ZERO));
  }

  /**
   * Register a (target, prediction) pair.
   *
   * \param[in] target     target of a simple example.
   * \param[in] prediction prediction of a single example.
   * \param[in] thread_id  thread id
   *
   */
  void register_example(const flexible_type& target,
                        const flexible_type& prediction,
                        size_t thread_id = 0) override {
    DASSERT_TRUE(thread_id < n_threads);
    check_undefined(prediction);
    DASSERT_TRUE((prediction.get_type() == flex_type_enum::FLOAT) ||
                 (prediction.get_type() == flex_type_enum::INTEGER));
    DASSERT_EQ(index_map.size(), 2);
    DASSERT_TRUE(index_map.count(target) > 0);

    num_examples[thread_id]++;
    size_t label = index_map.at(target);
    double pred = prediction.to<double>();
    check_probability_range(pred);
    logloss[thread_id] +=
      log(label != 0 ? std::max(pred, EVAL_ZERO) : std::max(1.0 - pred, EVAL_ZERO));
  }

  /**
   * Return the final metric.
   */
  variant_type get_metric() override {
    double total_logloss = 0;
    size_t total_examples = 0;
    for(size_t i = 0; i < n_threads; i++){
      total_logloss += logloss[i];
      total_examples += num_examples[i];
    }
    DASSERT_TRUE(total_examples > 0);
    total_examples = std::max<size_t>(1, total_examples);
    return to_variant(-total_logloss/total_examples);
  }

};

/**
 * Computes the classifier accuracy for a set of predictions, where all
 * predictions above the provided threshold are considered positive labels.
 *
 * accuaracy = num_right / num_examples
 *
 * where num_right are the things you got right!
 *
 */
class classifier_accuracy: public supervised_evaluation_interface {

  private:

  size_t n_threads;
  std::vector<double> accuracy;
  std::vector<size_t> num_examples;

  public:

  /**
   * Name of the evaluator.
   */
  std::string name() const {
    return (std::string)("classifier_accuracy");
  }


  /**
   * Init the state with a variant type.
   */
  void init(size_t _n_threads = 1){
    n_threads = _n_threads;
    accuracy.resize(n_threads);
    num_examples.resize(n_threads);
    for(size_t i = 0; i < n_threads; i++){
      accuracy[i] = 0;
      num_examples[i] = 0;
    }
  }


  /**
   * Register a (target, prediction) pair that are unmapped
   *
   * \param[in] target     target of a simple example.
   * \param[in] prediction prediction of a single example.
   * \param[in] thread_id  thread id
   *
   * \note Use this for performance because it does not perform a
   *       a flexible_type compare.
   */
  void register_unmapped_example(
                                const size_t& target,
                                const size_t& prediction,
                                size_t thread_id = 0){
    DASSERT_TRUE(thread_id < n_threads);
    num_examples[thread_id]++;
    accuracy[thread_id] += (target == prediction);
  }

  /**
   * Register a (target, prediction) pair.
   *
   * \param[in] target     target of a simple example.
   * \param[in] prediction prediction of a single example.
   * \param[in] thread_id  thread id
   *
   */
  void register_example(const flexible_type& target,
                                const flexible_type& prediction,
                                size_t thread_id = 0){
    DASSERT_TRUE(thread_id < n_threads);
    num_examples[thread_id]++;
    accuracy[thread_id] += (target == prediction);
  }

  /**
   * Return the final metric.
   */
  variant_type get_metric() {
    double total_accuracy = 0;
    size_t total_examples = 0;
    for(size_t i = 0; i < n_threads; i++){
      total_accuracy += accuracy[i];
      total_examples += num_examples[i];
    }
    DASSERT_TRUE(total_examples > 0);
    DASSERT_TRUE(total_accuracy >= 0);
    return to_variant(total_accuracy * 1.0 / total_examples);
  }

};

/**
 * Computes the confusion matrix for a set of predictions, where all
 * predictions above the provided threshold are considered positive
 * labels.
 *
 *  -----------------------------------
 *  true_label   predicted_label count
 *  -----------------------------------
 *
 *  -----------------------------------
 *
 */
class confusion_matrix: public supervised_evaluation_interface {

  private:

  // Accumulators
  std::vector<std::unordered_map<std::pair<flexible_type, flexible_type>, size_t,
                                                        flex_pair_hash>> counts;
  protected:

  // Useful variables
  size_t n_threads = 0;
  std::unordered_set<flexible_type> labels;
  std::map<size_t, flexible_type> index_map;
  std::unordered_map<std::pair<flexible_type, flexible_type>, size_t,
         flex_pair_hash> final_counts_thread, final_counts;


  public:

  /**
   * Constructor.
   */
  confusion_matrix(std::map<size_t, flexible_type> index_map =
                                        std::map<size_t, flexible_type>()) {
    this->index_map = index_map;
  }

  /**
   * Name of the evaluator.
   */
  std::string name() const {
    return (std::string)("confusion_matrix");
  }

  /**
   * Init the state with a variant type.
   */
  void init(size_t _n_threads = 1){
    n_threads = _n_threads;
    counts.resize(n_threads);
  }

  /**
   * Returns true of this evaluator can be displayed as a single float value.
   */
  virtual bool is_table_printer_compatible() const {
    return false;
  }

  /**
   * Register a (target, prediction) pair
   *
   * \param[in] target     Target of a simple example.
   * \param[in] prediction Prediction of a single example.
   * \param[in] thread_id  Thread id
   *
   */
  void register_example(const flexible_type& target,
                        const flexible_type& prediction,
                        size_t thread_id = 0){
    DASSERT_TRUE(thread_id < n_threads);
    std::pair<flexible_type, flexible_type> pair =
       std::make_pair(target, prediction);

    if(counts[thread_id].count(pair) > 0){
      counts[thread_id][pair]++;
    } else {
      counts[thread_id][pair] = 1;
    }
  }

  /**
   * Gather all final counts.
   */
  void gather_counts_and_labels() {

    // Merge by thread.
    std::unordered_map<std::pair<flexible_type, flexible_type>, size_t,
                                        flex_pair_hash> final_counts_thread;
    for(size_t i = 0; i < n_threads; i++){
      for (const auto& kvp: counts[i]){
        if(final_counts_thread.count(kvp.first) > 0){
          final_counts_thread[kvp.first] += kvp.second;
        } else {
          final_counts_thread[kvp.first] = kvp.second;
        }
      }
    }
    final_counts = final_counts_thread;

    // Gather labels.
    DASSERT_TRUE(final_counts_thread.size() >= 0);
    for (const auto& kvp: final_counts) {
      if (labels.count(kvp.first.first) == 0) {
        labels.insert(kvp.first.first);
      }
      if (labels.count(kvp.first.second) == 0) {
        labels.insert(kvp.first.second);
      }
    }
  }

  /**
   * Return the final metric.
   */
  variant_type get_metric() {

    // Accumulate counts & labels for each class.
    this->gather_counts_and_labels();

    // If map provided, then do nothing!
    if (!index_map.empty()) {
      std::unordered_map<std::pair<flexible_type, flexible_type>, size_t,
                     flex_pair_hash> final_counts_copy;
      for (const auto& kvp: final_counts) {
        size_t first_index = kvp.first.first.get<flex_int>();
        size_t second_index = kvp.first.second.get<flex_int>();
        const flexible_type& first_key = index_map.at(first_index);
        const flexible_type& second_key = index_map.at(second_index);
        DASSERT_TRUE(index_map.count(first_index) > 0);
        DASSERT_TRUE(index_map.count(second_index) > 0);
        final_counts_copy[std::make_pair(first_key,second_key)] = kvp.second;
      }
      final_counts = final_counts_copy;
    }

    // Write to an SFrame.
    sframe confusion_matrix_sf;
    std::vector<std::string> names;
    names.push_back("target_label");
    names.push_back("predicted_label");
    names.push_back("count");

    // Inspect types: If things are the same type, then use the type that they
    // share, otherwise use string.
    flex_type_enum target_type = flex_type_enum::UNDEFINED;
    flex_type_enum predicted_type = flex_type_enum::UNDEFINED;
    for (const auto &cf_entry: final_counts){
      auto t_type = cf_entry.first.first.get_type();
      auto p_type = cf_entry.first.second.get_type();

      if(target_type == flex_type_enum::UNDEFINED) {
        target_type = t_type;
      } else {
        if (t_type != flex_type_enum::UNDEFINED && t_type != target_type) {
          target_type = flex_type_enum::STRING;
          break;
        }
      }

     if(predicted_type == flex_type_enum::UNDEFINED) {
        predicted_type = p_type;
      } else {
        if (p_type != flex_type_enum::UNDEFINED && p_type != predicted_type) {
          predicted_type = flex_type_enum::STRING;
          break;
        }
      }
    }

    if (target_type == flex_type_enum::UNDEFINED) {
      target_type = flex_type_enum::FLOAT;
    }

    if (predicted_type == flex_type_enum::UNDEFINED) {
      predicted_type = flex_type_enum::FLOAT;
    }

    std::vector<flex_type_enum> types;
    types.push_back(target_type);
    types.push_back(predicted_type);
    types.push_back(flex_type_enum::INTEGER);
    confusion_matrix_sf.open_for_write(names, types, "", 1);  // write to temp file
    auto it = confusion_matrix_sf.get_output_iterator(0);

    std::vector<flexible_type> x(3);
    for (const auto &cf_entry: final_counts){
      x[0] = cf_entry.first.first;
      x[1] = cf_entry.first.second;
      x[2] = cf_entry.second;
      *it= x;
      ++it;
    }

    confusion_matrix_sf.close();
    std::shared_ptr<unity_sframe> unity_confusion_matrix =
                                            std::make_shared<unity_sframe>();
    unity_confusion_matrix->construct_from_sframe(confusion_matrix_sf);
    return to_variant(unity_confusion_matrix);
  }

};


/**
 * Compute the F-Beta score.
 */
class precision_recall_base : public confusion_matrix {

  protected:

  average_type_enum average;
  std::unordered_map<flexible_type, size_t> tp; // True positives
  std::unordered_map<flexible_type, size_t> tn; // True negatives
  std::unordered_map<flexible_type, size_t> fp; // False positives
  std::unordered_map<flexible_type, size_t> fn; // False negatives

  public:

  /**
   * Get the "highest" label as the reference label.
   */
  flexible_type get_reference_label() {

    // First find atleast 1 label that isn't a None
    flexible_type ret;
    for (const auto& l : labels) {
      if (l != FLEX_UNDEFINED) {
        ret = l;
        break;
      }
    }
    // Now find the "max" label.
    for (const auto& l : labels) {
      if (l != FLEX_UNDEFINED) {
        if (ret < l) {
         ret = l;
        }
      }
    }
    return ret;
  }

  /**
   * Name of the evaluator.
   */
  std::string name() const = 0;

  /**
   * Returns true of this evaluator can be displayed as a single float value.
   */
  bool is_table_printer_compatible() const {
    return average != average_type_enum::NONE;
  }

  /**
   * Gather global metrics for true_positives and false negatives
   */
  void gather_global_metrics() {

    // Accumulate counts & labels for each class.
    this->gather_counts_and_labels();
    for (const auto& l: labels) {
      tp[l] = 0;
      fp[l] = 0;
      tn[l] = 0;
      fn[l] = 0;
    }

    // Compute the global metrics for tp, fp, tn, fn for each label.
    for (const auto& kvp: final_counts) {
      flexible_type t = kvp.first.first;
      flexible_type p = kvp.first.second;
      size_t count = kvp.second;

      // If predicted is the same as the target.
      for (const auto& l: labels) {

        // Correctly predicted "l"
        if ( (p == l) == (t == l)) {
          // True positive with repect to label [p]
          if (l == p) {
            tp[l] += count;
          // True negative with repect to label l != p
          } else {
            tn[l] += count;
          }

        // Correctly predicted not "l"
        } else {
          // False positive with repect to label [p]
          if (l == p) {
            fp[l] += count;
          // False negative with repect to label l != p
          } else {
            fn[l] += count;
          }
        }
      }
    }
  }

  /**
   * Get the metric!
   */
  variant_type get_metric() = 0;

};

/**
 * Compute the F-Beta score.
 */
class fbeta_score: public precision_recall_base {

  private:

  double beta;

  public:

  /**
   * Constructor to set the value of beta.
   */
  fbeta_score(double beta = 1.0, flexible_type average = "macro") {
    if (beta <= 0) {
      log_and_throw("The beta value in the F-beta score must be > 0.0");
    }
    this->beta = beta;
    this->average = average_type_enum_from_name(average);
  }

  /**
   * Name of the evaluator.
   */
  std::string name() const {
    return (std::string)("fbeta_score");
  }


  /**
   * Get the metric!
   */
  variant_type get_metric() {

    // Gather all the global metrics.
    this->gather_global_metrics();
    DASSERT_TRUE(labels.size() >= 0);
    DASSERT_TRUE(beta > 0);
    DASSERT_EQ(fp.size(), labels.size());
    DASSERT_EQ(tp.size(), labels.size());


    // Multi-class vs binary classification.
    std::unordered_map<flexible_type, flexible_type> fbeta_scores;
    for (const auto& l: labels) {
      fbeta_scores[l] = compute_fbeta_score(tp[l], fp[l], fn[l], beta);
    }

    // For binary classification, return the scores for the final label.
    if (labels.size() == 2) {
      return to_variant(fbeta_scores[get_reference_label()]);
    }

    // Multi-class scores: Average based on user request.
    switch (average) {
      // Global scores.
      case average_type_enum::MICRO:
      {
        size_t total_tp = 0;
        size_t total_fp = 0;
        size_t total_fn = 0;
        for (const auto& l: labels) {
          total_tp += tp[l];
          total_fp += fp[l];
          total_fn += fn[l];
        }
        return to_variant(compute_fbeta_score(total_tp, total_fp, total_fn, beta));

      // Average scores.
      }
      case average_type_enum::DEFAULT:
      case average_type_enum::MACRO:
      {
        return to_variant(average_with_none_skip(fbeta_scores));

      // All scores.
      }
      case average_type_enum::NONE:
      {
        return to_variant(fbeta_scores);
      }

      default: {
        log_and_throw(std::string("Unsupported average_type_enum case"));
        ASSERT_UNREACHABLE();
      }
    }
  }

};


/**
 * Compute the precision score.
 */
class precision : public precision_recall_base {

  public:

  /**
   * Constructor.
   */
  precision(flexible_type average = "macro") {
    this->average = average_type_enum_from_name(average);
  }

  /**
   * Name of the evaluator.
   */
  std::string name() const {
    return (std::string)("precision");
  }

  /**
   * Get the metric!
   */
  variant_type get_metric() {

    // Gather all the global metrics.
    this->gather_global_metrics();
    DASSERT_TRUE(labels.size() >= 0);
    DASSERT_EQ(fp.size(), labels.size());
    DASSERT_EQ(tp.size(), labels.size());


    // Multi-class vs binary classification.
    std::unordered_map<flexible_type, flexible_type> precision_scores;
    for (const auto& l: labels) {
        precision_scores[l] =  compute_precision_score(tp[l], fp[l]);
    }

    // For binary classification, return the scores for the final label.
    if (labels.size() == 2) {
      return to_variant(precision_scores[get_reference_label()]);
    }

    // Multi-class scores: Average based on user request.
    switch (average) {
      // Global scores.
      case average_type_enum::MICRO:
      {
        size_t total_tp = 0;
        size_t total_fp = 0;
        for (const auto& l: labels) {
          total_tp += tp[l];
          total_fp += fp[l];
        }
        return to_variant(compute_precision_score(total_tp, total_fp));
      }
      // Average scores.
      case average_type_enum::DEFAULT:
      case average_type_enum::MACRO:
      {
        return to_variant(average_with_none_skip(precision_scores));
      }

      // All scores.
      case average_type_enum::NONE:
      {
        return to_variant(precision_scores);
      }

      default: {
        log_and_throw(std::string("Unsupported average_type_enum case"));
        ASSERT_UNREACHABLE();
      }
    }
  }

};


/**
 * Compute the recall score.
 */
class recall : public precision_recall_base {


  public:

  /**
   * Constructor.
   */
  recall(flexible_type average = "macro") {
    this->average = average_type_enum_from_name(average);
  }

  /**
   * Name of the evaluator.
   */
  std::string name() const {
    return (std::string)("recall");
  }


  /**
   * Get the metric!
   */
  variant_type get_metric() {

    // Gather all the global metrics.
    this->gather_global_metrics();
    DASSERT_TRUE(labels.size() >= 0);
    DASSERT_EQ(fp.size(), labels.size());
    DASSERT_EQ(tp.size(), labels.size());

    // Multi-class vs binary classification.
    std::unordered_map<flexible_type, flexible_type> recall_scores;
    for (const auto& l: labels) {
        recall_scores[l] =  compute_recall_score(tp[l], fn[l]);
    }

    // For binary classification, return the scores for the final label.
    if (labels.size() == 2) {
      return to_variant(recall_scores[get_reference_label()]);
    }

    // Multi-class scores: Average based on user request.
    switch (average) {
      // Global scores.
      case average_type_enum::MICRO:
      {
        size_t total_tp = 0;
        size_t total_fn = 0;
        for (const auto& l: labels) {
          total_tp += tp[l];
          total_fn += fn[l];
        }
        return to_variant(compute_recall_score(total_tp, total_fn));
      }

      // Average scores.
      case average_type_enum::DEFAULT:
      case average_type_enum::MACRO:
      {
        return to_variant(average_with_none_skip(recall_scores));
      }

      // All scores.
      case average_type_enum::NONE:
      {
        return to_variant(recall_scores);
      }

      default: {
        log_and_throw(std::string("Unsupported average_type_enum case"));
        ASSERT_UNREACHABLE();
      }
    }
  }

};

/**
 * Compute the accuracy score. This is a slower, but more flexible version
 * of the accuracy.
 *
 */
class flexible_accuracy : public precision_recall_base {


  public:

  /**
   * Constructor.
   */
  flexible_accuracy(flexible_type average = "micro") {
    this->average = average_type_enum_from_name(average);
  }

  /**
   * Name of the evaluator.
   */
  std::string name() const {
    return (std::string)("accuracy");
  }


  /**
   * Get the metric!
   */
  variant_type get_metric() {

    // Gather all the global metrics.
    this->gather_global_metrics();
    DASSERT_TRUE(labels.size() >= 0);
    DASSERT_EQ(fp.size(), labels.size());
    DASSERT_EQ(tp.size(), labels.size());

    // Multi-class vs binary classification.
    std::unordered_map<flexible_type, double> accuracy_scores;
    std::unordered_map<flexible_type, flexible_type> precision_scores;
    for (const auto& l: labels) {
      accuracy_scores[l] = double(tp[l] + tn[l])/(tp[l] + fp[l] + tn[l] + fn[l]);
      precision_scores[l] = compute_precision_score(tp[l], fp[l]);
    }

    // For binary classification, return the scores for the final label.
    if (labels.size() == 2) {
      return to_variant(accuracy_scores[get_reference_label()]);
    }

    // Multi-class scores: Average based on user request.
    switch (average) {
      // Global scores.
      case average_type_enum::MICRO:
      case average_type_enum::DEFAULT:
      {
        size_t tot_tp = 0;
        size_t tot_tn = 0;
        size_t tot_fp = 0;
        size_t tot_fn = 0;
        for (const auto& l: labels) {
          tot_tp += tp[l];
          tot_tn += tp[l];
          tot_fp += fp[l];
          tot_fn += fn[l];
        }
        double accuracy = double(tot_tp + tot_tn)/(tot_tp + tot_fp + tot_tn + tot_fn);
        return to_variant(accuracy);

      }
      // Average scores.
      case average_type_enum::MACRO:
      {
        double average_accuracy = 0.0;
        for (const auto& rec: accuracy_scores) {
          average_accuracy +=  rec.second;
        }
        average_accuracy /= labels.size();
        return to_variant(average_accuracy);
      }
      // All scores.
      case average_type_enum::NONE:
      {
        return to_variant(precision_scores);
      }

      default: {
        log_and_throw(std::string("Unsupported average_type_enum case"));
        ASSERT_UNREACHABLE();
      }
    }
  }
};

/**
 * Computes the ROC curve. An aggregated version is computed, where we
 * compute the true positive rate and false positive rate for a set of
 * 1000 predefined thresholds equally spaced from 0 to 1.
 * For each prediction, we find which bin it belongs to and we increment
 * the count of true positives (where y=1 and yhat is greater than the lower
 * bound for that bin) and the number of false positives.
 * When complete, these counts are used to compute false positive rate and
 * true positive rate for each bin.
 *
 * In order to use this class, there are two modes:
 * - binary mode: In this mode, the inputs are (target_class, prediction_prob)
 *   where prediction_prob is the probability of the "positive" class. Here
 *   the "positive" class is defined as the largest class as sorted by flexible_type
 *   semantics.
 *
 * - multiclass mode: In this mode, the inputs are (target_class, prob_vec)
 *   where prob_vec are the vector of probabilities. In this case, the
 *   target_class must be integer
 */
class roc_curve: public supervised_evaluation_interface {


  private:

  // Accumulators
  std::unordered_map<std::pair<flexible_type, flexible_type>, size_t,
         flex_pair_hash> final_counts_thread, final_counts;
  std::vector<std::vector<std::vector<size_t>>> tpr;
  std::vector<std::vector<std::vector<size_t>>> fpr;
  std::vector<std::vector<size_t>> num_examples;

  protected:

  // Options
  average_type_enum average = average_type_enum::NONE;
  bool binary = false;
  const size_t NUM_BINS=100000;
  size_t n_threads = 0;
  size_t num_classes = 0;

  // Input map.
  std::unordered_map<flexible_type, size_t> index_map;

  // Total counts
  std::vector<std::vector<size_t>> total_fp;
  std::vector<std::vector<size_t>> total_tp;
  std::vector<size_t> total_examples;

  public:

  /**
   * Constructor.
   *
   * \param[in] index_map Dictionary from flexible_type -> size_t for classes.
   * \param[in] average   Averaging mode
   * \param[in] binary    Is the input mode expected to be binary?
   */
  roc_curve(
      std::unordered_map<flexible_type, size_t> index_map =
                  std::unordered_map<flexible_type, size_t>(),
      flexible_type average = FLEX_UNDEFINED,
      bool binary = true,
      size_t num_classes = size_t(-1)) {
    this->average = average_type_enum_from_name(average);
    this->binary = binary;
    this->index_map = index_map;
    if (num_classes == size_t(-1)) {
      this->num_classes = index_map.size();
    } else {
      this->num_classes = num_classes;
    }
  }

  /**
   * Name of the evaluator.
   */
  std::string name() const {
    return (std::string)("roc_curve");
  }

  /**
   * Returns true of this evaluator works on probabilities/scores (vs)
   * classes.
   */
  bool is_prob_evaluator() const {
    return true;
  }

  /**
   * Returns true of this evaluator can be displayed as a single float value.
   */
  virtual bool is_table_printer_compatible() const {
    return false;
  }

  /**
   * Init the state with a variant type.
   */
  void init(size_t _n_threads = 1) {
    DASSERT_TRUE(num_classes > 0);
    DASSERT_LE(binary, num_classes == 2);

    // Init the options.
    n_threads = _n_threads;

    // Initialize the accumulators
    tpr.resize(n_threads);
    fpr.resize(n_threads);
    num_examples.resize(n_threads);
    for (size_t i = 0; i < n_threads; i++) {

      tpr[i].resize(num_classes);
      fpr[i].resize(num_classes);
      num_examples[i].resize(num_classes);
      for (size_t c = 0; c < num_classes; c++) {

        tpr[i][c].resize(NUM_BINS);
        fpr[i][c].resize(NUM_BINS);
        num_examples[i][c] = 0;
        for (size_t j = 0; j < NUM_BINS; j++) {
          tpr[i][c][j] = 0;
          fpr[i][c][j] = 0;
        }
      }
    }

    // Initialize the aggregators.
    total_fp.resize(num_classes);
    total_tp.resize(num_classes);
    total_examples.resize(num_classes);
    for (size_t c = 0; c < num_classes; c++) {
      total_examples[c] = 0;
      total_fp[c].resize(NUM_BINS);
      total_tp[c].resize(NUM_BINS);
      for (size_t j = 0; j < NUM_BINS; j++) {
       total_fp[c][j] = 0;
       total_tp[c][j] = 0;
      }
    }

  };

  const float get_bin(double prediction) const {
    // Assign this prediction to an integer that indicates a "bin" id.
    size_t bin = std::floor((double) std::max(0.0, prediction * NUM_BINS));

    // This effectively makes the upper bin [0.999, 1] instead of [0.999, 1).
    // If a prediction is exactly 1.0, then it would get assigned to
    // a bin with lower bound 1.0, but since we want 1000 bins, we move
    // these into the bin with lower bound 0.999.
    if (bin >= NUM_BINS) bin = NUM_BINS - 1;
    return bin;
  }

  const float get_lower_bound(size_t bin) const {
    // Get the lower threshold of predictions that fall into this bin.
    return bin/((double)NUM_BINS);
  }

  /**
   * Register a (target, prediction) pair
   *
   * \param[in] target     Target of a simple example.
   * \param[in] prediction Prediction of a single example.
   * \param[in] thread_id  Thread id
   *
   */
  void register_example(const flexible_type& target,
                        const flexible_type& prediction,
                        size_t thread_id = 0){
    DASSERT_LT(thread_id, n_threads);
    DASSERT_LT(thread_id, fpr.size());
    DASSERT_LT(thread_id, tpr.size());
    check_undefined(prediction);
    DASSERT_EQ(binary, (prediction.get_type() == flex_type_enum::FLOAT) ||
                       (prediction.get_type() == flex_type_enum::INTEGER));
    DASSERT_EQ(!binary, prediction.get_type() == flex_type_enum::VECTOR);

    // The index for this target. Skip the example if it doesn't exist!
    size_t idx = 0;
    auto it = index_map.find(target);
    if (it == index_map.end()) {
      return;
    } else {
      idx = size_t(it->second);
    }
    DASSERT_LT(idx, index_map.size());

    // Binary mode.
    if (binary) {
      DASSERT_EQ(num_classes, 2);

      // Add this example to the prediction counter.
      double pred = prediction.to<double>();
      check_probability_range(pred);
      size_t bin = get_bin(pred);

      if (idx == 1) {
        DASSERT_LT(bin, tpr[thread_id][1].size());
        DASSERT_LT(bin, fpr[thread_id][0].size());
        tpr[thread_id][1][bin]++;
        fpr[thread_id][0][bin]++;
        num_examples[thread_id][1]++;
      } else {
        DASSERT_LT(bin, tpr[thread_id][0].size());
        DASSERT_LT(bin, fpr[thread_id][1].size());
        fpr[thread_id][1][bin]++;
        tpr[thread_id][0][bin]++;
        num_examples[thread_id][0]++;
      }

    // Multi-class mode.
    } else {

      // Error out!
      if(prediction.size() != num_classes) {
        std::stringstream ss;
        ss << "Size of prediction probability vector"
           << "(" << prediction.size() << ") != number of classes"
           << "(" << num_classes << ")." << std::endl;
        log_and_throw(ss.str());
      }

      // Data point in the test set but not in the training set. Skip.
      if (idx >= prediction.size()) {
        return;
      }

      // Get the prediction for the true class.
      for (size_t i = 0; i < prediction.size(); i++) {
        check_probability_range(prediction[i]);
        size_t bin = get_bin(prediction[i]);

        // Update the tpr and fpr rates!
        if (i == idx) {
          DASSERT_LT(bin, tpr[thread_id][idx].size());
          tpr[thread_id][i][bin]++;
        } else {
          DASSERT_LT(bin, fpr[thread_id][idx].size());
          fpr[thread_id][i][bin]++;
        }
      }
      num_examples[thread_id][idx]++;
    }

  }

  /**
   * Gather global metrics for true_positives and false negatives
   */
  void gather_global_metrics() {

    // Total fp, tp, examples
    for (size_t i = 0; i < n_threads; ++i) {
      for (size_t c = 0; c < num_classes; c++) {
        total_examples[c]  += num_examples[i][c];
        for (size_t j = 0; j < NUM_BINS; ++j) {
          total_fp[c][j] += fpr[i][c][j];
          total_tp[c][j] += tpr[i][c][j];
        }
      }
    }

    // Get the number of false positives and true positives for all
    // bins above the current bin.
    for (size_t c = 0; c < num_classes; c++) {
      for (int j = NUM_BINS-2; j >= 0; --j) {
        total_fp[c][j] += total_fp[c][j+1];
        total_tp[c][j] += total_tp[c][j+1];
      }
    }

  }

  /**
   * Return the final metric.
   */
  virtual variant_type get_metric() {

    this->gather_global_metrics();

    // Helper function for computing the roc curve from the statistics
    size_t total_bins = NUM_BINS;
    size_t _num_classes = this->num_classes;
    auto compute_roc_curve = [total_bins, _num_classes](
          const std::vector<std::vector<size_t>>& total_fp,
          const std::vector<std::vector<size_t>>& total_tp,
          const std::vector<size_t>& total_examples,
          const size_t& c,
          bool binary = true, // Is this in binary mode?
          const std::map<size_t, flexible_type>& inv_map =
                 std::map<size_t, flexible_type>())  -> variant_type {

      size_t all_examples = 0;
      for (const auto& cex: total_examples) {
        all_examples += cex;
      }

      // Columns in the SFrame.
      sframe ret;
      std::vector<std::string> col_names {"threshold", "fpr", "tpr", "p", "n"};
      std::vector<flex_type_enum> col_types {flex_type_enum::FLOAT,
                                             flex_type_enum::FLOAT,
                                             flex_type_enum::FLOAT,
                                             flex_type_enum::INTEGER,
                                             flex_type_enum::INTEGER};

      // Not binary, add class to it!
      if (not binary) {
        col_names.push_back("class");
        DASSERT_TRUE(inv_map.size() > 0);
        col_types.push_back(inv_map.at(c).get_type());
      }

      ret.open_for_write(col_names, col_types, "", 1);
      std::vector<flexible_type> out_v;
      auto it_out = ret.get_output_iterator(0);

      // Write to the SFrame.
      size_t cl = 0;
      do {
        if (binary) cl = c;

        // Add all rows.
        for (size_t j=0; j < total_bins; ++j) {
          DASSERT_LE(total_tp[cl][j], total_examples[cl]);
          DASSERT_LE(total_fp[cl][j], all_examples - total_examples[cl]);
          out_v = {j / double(total_bins),
                   (1.0 * total_fp[cl][j]) / (all_examples - total_examples[cl]),
                   (1.0 * total_tp[cl][j]) / total_examples[cl],
                   total_examples[cl], (all_examples - total_examples[cl])};
          if (not binary) {
            out_v.push_back(inv_map.at(cl));
          }
          *it_out = out_v;
        }

        // Manually add final row.
        out_v = {1.0, 0.0, 0.0,
                 total_examples[c], (all_examples - total_examples[c])};
        if (not binary) {
            out_v.push_back(inv_map.at(cl));
        }

        // Write the row.
        *it_out = out_v;
        cl++;

        if (binary) break;
        if (cl == _num_classes) break;

      } while (true);

      ret.close();
      DASSERT_EQ(ret.size(),
            (total_bins + 1) * (binary + (1 - binary) * _num_classes));

      // Convert to variant type.
      std::shared_ptr<unity_sframe> tmp = std::make_shared<unity_sframe>();
      tmp->construct_from_sframe(ret);
      variant_type roc_curve = to_variant<std::shared_ptr<unity_sframe>>(tmp);
      return roc_curve;

    }; // end-of-helper function.

    // Compute the integral with respect to ROC-1
    if (num_classes == 2) {
      return to_variant(compute_roc_curve(total_fp, total_tp, total_examples, 1));
    }

    switch(average) {

      // Score for each class.
      case average_type_enum::NONE:
      case average_type_enum::DEFAULT:
      {

        // Create an inverse map.
        std::map<size_t, flexible_type> inv_map;
        for (const auto& kvp: index_map) {
          inv_map[kvp.second] = kvp.first;
        }
        return compute_roc_curve(total_fp, total_tp, total_examples, 0, false, inv_map);
      }

      default: {
        log_and_throw(std::string("Unsupported average_type_enum case"));
        ASSERT_UNREACHABLE();
      }
    }
  }
};


/*
 * Compute the Area Under the Curve (AUC) using the trapezoidal rule
 */
class auc: public roc_curve {

  public:

  /**
   * Constructor.
   *
   * \param[in] index_map Dictionary from flexible_type -> size_t for classes.
   * \param[in] average   Averaging mode
   * \param[in] binary    Is the input mode expected to be binary?
   */
  auc(
      std::unordered_map<flexible_type, size_t> index_map =
                  std::unordered_map<flexible_type, size_t>(),
      flexible_type average = "micro",
      bool binary = true,
      size_t num_classes = size_t(-1)) {
    this->average = average_type_enum_from_name(average);
    this->binary = binary;
    this->index_map = index_map;
    if (num_classes == size_t(-1)) {
      this->num_classes = index_map.size();
    } else {
      this->num_classes = num_classes;
    }
  }

  /*
   * Name of the evaluator.
   */
  std::string name() const {
    return (std::string)("auc");
  }

  /**
   * Returns true of this evaluator can be displayed as a single float value.
   */
  virtual bool is_table_printer_compatible() const {
    return average != average_type_enum::NONE;
  }


  /**
   * Return the final metric.
   */
  variant_type get_metric() {

    this->gather_global_metrics();

    // Compute the auc-score.
    size_t total_bins = NUM_BINS;
    auto compute_auc = [total_bins](
          const std::vector<std::vector<size_t>>& total_fp,
          const std::vector<std::vector<size_t>>& total_tp,
          const std::vector<size_t>& total_examples,
          const size_t& c)  -> double {

      size_t all_examples = 0;
      for (const auto& cex: total_examples) {
        all_examples += cex;
      }

      double auc_score = 0;
      for(size_t i = 0; i < total_bins- 1; i++) {
        double delta = total_fp[c][i] - total_fp[c][i+1];
        delta /= (all_examples - total_examples[c]);
        if (delta > 1e-10) {
          auc_score += 0.5 * (total_tp[c][i] + total_tp[c][i+1]) * delta
                                                          / total_examples[c];
        }
      }
      return auc_score;
    };

    // Compute the integral with respect to ROC-1
    if (num_classes == 2) {
      return to_variant(compute_auc(total_fp, total_tp, total_examples, 1));
    }

    switch(average) {

      // Score for each class.
      case average_type_enum::NONE:
      {

        // Create an inverse map.
        std::map<size_t, flexible_type> inv_map;
        for (const auto& kvp: index_map) {
          inv_map[kvp.second] = kvp.first;
        }

        // Compute AUC score.
        std::unordered_map<flexible_type, double> auc_score;
        for (size_t c = 0; c < num_classes; c++) {
          flexible_type k = inv_map[c];
          auc_score[k] = compute_auc(total_fp, total_tp, total_examples, c);
        }
        return to_variant(auc_score);
      }

      case average_type_enum::DEFAULT:
      case average_type_enum::MACRO:
      {
        double auc_score = 0;
        for (size_t c = 0; c < num_classes; c++) {
          auc_score += compute_auc(total_fp, total_tp, total_examples, c);
        }
        return to_variant(auc_score / num_classes);
      }

      default: {
        log_and_throw(std::string("Unsupported average_type_enum case"));
        ASSERT_UNREACHABLE();
      }
    }
  }
};

/*
 * Factory method to get the set of evaluation metrics.
 * \param[in] metric Name of the metric
 * \param[in] kwargs Arguments for the metric
 *
 *
 * \example
 *
 * For a constructor of the following format:
 *
 *  flexible_accuracy(flexible_type _average = "micro") {
 *    average = average_type_enum_from_name(average);
 *  }
 *
 * this factory function can be called as follows:
 *
 * get_evaluator_metric("flexible_accuracy",
 *            {"average", to_variant(std::string("micro"))})
 *
 * This is intended to work just like the python side.
 *
 */
inline std::shared_ptr<supervised_evaluation_interface> get_evaluator_metric(
                const std::string& metric,
                const std::map<std::string, variant_type>& kwargs = std::map<std::string, variant_type>()) {

  std::shared_ptr<supervised_evaluation_interface> evaluator;
  if(metric == "rmse"){
    evaluator = std::make_shared<rmse>(rmse());
  } else if(metric == "max_error"){
    evaluator = std::make_shared<max_error>(max_error());

  } else if(metric == "confusion_matrix_no_map"){
    evaluator = std::make_shared<confusion_matrix>(confusion_matrix());

  } else if(metric == "confusion_matrix"){
    DASSERT_TRUE(kwargs.count("inv_index_map") > 0);
    std::map<size_t, flexible_type> inv_map = variant_get_value<
                 std::map<size_t, flexible_type>>(kwargs.at("inv_index_map"));
    evaluator = std::make_shared<confusion_matrix>(confusion_matrix(inv_map));

  } else if(metric == "accuracy"){
    evaluator = std::make_shared<classifier_accuracy>(classifier_accuracy());

  } else if(metric == "binary_logloss") {
    DASSERT_TRUE(kwargs.count("index_map") > 0);
    auto index_map = variant_get_value<
       std::unordered_map<flexible_type, size_t>>(kwargs.at("index_map"));
    evaluator = std::make_shared<binary_logloss>(
                              binary_logloss(index_map));

  } else if((metric == "multiclass_logloss") || (metric == "log_loss")){
    DASSERT_TRUE(kwargs.count("index_map") > 0);
    auto index_map = variant_get_value<
       std::unordered_map<flexible_type, size_t>>(kwargs.at("index_map"));
    size_t num_classes = size_t(-1);
    if (kwargs.count("num_classes") > 0) {
      num_classes = variant_get_value<size_t>(kwargs.at("num_classes"));
    }
    evaluator = std::make_shared<multiclass_logloss>(
                              multiclass_logloss(index_map, num_classes));

  } else if(metric == "roc_curve"){
    DASSERT_TRUE(kwargs.count("average") > 0);
    DASSERT_TRUE(kwargs.count("binary") > 0);
    DASSERT_TRUE(kwargs.count("index_map") > 0);
    auto average = variant_get_value<flexible_type>(kwargs.at("average"));
    auto binary = variant_get_value<bool>(kwargs.at("binary"));
    auto index_map = variant_get_value<
       std::unordered_map<flexible_type, size_t>>(kwargs.at("index_map"));
    size_t num_classes = size_t(-1);
    if (kwargs.count("num_classes") > 0) {
      num_classes = variant_get_value<size_t>(kwargs.at("num_classes"));
    }
    evaluator = std::make_shared<roc_curve>(
                    roc_curve(index_map, average, binary, num_classes));

  } else if(metric == "auc"){
    DASSERT_TRUE(kwargs.count("average") > 0);
    DASSERT_TRUE(kwargs.count("binary") > 0);
    DASSERT_TRUE(kwargs.count("index_map") > 0);
    auto average = variant_get_value<flexible_type>(kwargs.at("average"));
    auto binary = variant_get_value<bool>(kwargs.at("binary"));
    auto index_map = variant_get_value<
       std::unordered_map<flexible_type, size_t>>(kwargs.at("index_map"));
    size_t num_classes = size_t(-1);
    if (kwargs.count("num_classes") > 0) {
      num_classes = variant_get_value<size_t>(kwargs.at("num_classes"));
    }
    evaluator = std::make_shared<auc>(auc(index_map, average, binary, num_classes));

  } else if(metric == "flexible_accuracy"){
    DASSERT_TRUE(kwargs.count("average") > 0);
    auto average = variant_get_value<flexible_type>(kwargs.at("average"));
    evaluator = std::make_shared<flexible_accuracy>(
                                      flexible_accuracy(average));

  } else if(metric == "precision"){
    DASSERT_TRUE(kwargs.count("average") > 0);
    auto average = variant_get_value<flexible_type>(kwargs.at("average"));
    evaluator = std::make_shared<precision>(precision(average));

  } else if(metric == "recall"){
    DASSERT_TRUE(kwargs.count("average") > 0);
    auto average = variant_get_value<flexible_type>(kwargs.at("average"));
    evaluator = std::make_shared<recall>(recall(average));

  } else if(metric == "fbeta_score"){
    DASSERT_TRUE(kwargs.count("beta") > 0);
    DASSERT_TRUE(kwargs.count("average") > 0);
    auto beta = variant_get_value<double>(kwargs.at("beta"));
    auto average = variant_get_value<flexible_type>(kwargs.at("average"));
    evaluator = std::make_shared<fbeta_score>(fbeta_score(beta, average));

  } else if(metric == "f1_score"){
    DASSERT_TRUE(kwargs.count("average") > 0);
    auto average = variant_get_value<flexible_type>(kwargs.at("average"));
    evaluator = std::make_shared<fbeta_score>(fbeta_score(1.0, average));

  } else {
    log_and_throw("\'" + metric + "\' is not a supported evaluation metric.");
  }

  // Initialize with number of threads.
  size_t n_threads = turi::thread::cpu_count();
  evaluator->init(n_threads);
  return evaluator;
}

} // evaluation
} // turicreate

#ifdef __clang__
  #pragma clang diagnostic pop
#endif

#endif

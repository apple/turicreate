/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE test_activity_classifier

#include <set>

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <toolkits/activity_classification/activity_classifier.hpp>

namespace turi {
namespace activity_classification {
namespace {

class test_activity_classifier : public activity_classifier {
 public:
  class mock_iterator : public data_iterator {
    flex_list class_labels_;
    flex_list feature_names_;
    size_t num_sessions_;
    void reset() override { return; }

    bool has_next_batch() const override { return true; }

    const flex_list& class_labels() const override { return class_labels_; }

    const flex_list& feature_names() const override { return feature_names_; }

    const size_t num_sessions() const override { return num_sessions_; }

    flex_type_enum session_id_type() const override {
      return flex_type_enum::UNDEFINED;
    }

    batch next_batch(size_t batch_size) override {
      batch result;
      return result;
    }
  };

  test_activity_classifier() {
    add_or_update_state({{"session_id", "exp_id"}});
  }

  gl_sframe predict_data;

  std::unique_ptr<data_iterator> create_iterator(
      gl_sframe data, bool requires_labels, bool infer_class_labels,
      bool is_train, bool use_data_augmentation) const override {
    return std::unique_ptr<mock_iterator>(new mock_iterator());
  }

  gl_sframe perform_inference(data_iterator* data) const override {
    return predict_data;
  }

  void set_mock_predict_data(gl_sarray session, gl_sarray prediction_id,
                             gl_sarray num_samples, gl_sarray prob) {
    predict_data = gl_sframe();
    predict_data.add_column(session, "session_id");
    predict_data.add_column(prediction_id, "prediction_id");
    predict_data.add_column(num_samples, "num_samples");
    predict_data.add_column(prob, "preds");
  }

  void set_model_label(flex_list labels) { state.emplace("classes", labels); }
};


BOOST_AUTO_TEST_CASE(test_activity_classifier_classify_and_predict) {

    const flex_list class_labels = {"a", "b", "c", "d", "e", "f", "g"};

  // session id for each prediction
  // in this case it is [1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4,...]
  constexpr size_t session_num = 5;
  const std::vector<size_t> session_distribution = {1, 3, 5, 7, 9};
  TS_ASSERT_EQUALS(session_num, session_distribution.size());

  constexpr size_t test_num_examples = 25;
  size_t check_sum = 0;
  for (size_t num_per_session : session_distribution)
    check_sum += num_per_session;
  TS_ASSERT_EQUALS(check_sum, test_num_examples);

  /**
   * mock predict data
   * we assume the sliding window has size 3
   * num_samples indicates the number of samples in each sliding window,
   * which is betweem [1,3] in this case
   * for the first session, which has one example, will produce 1 prediction
   * for the second session, which has three examples, will produce 1
   * prediction. the third session, which has five examples, will produce 2
   * predictions. session_id indicates the session id for each prediction
   */
  const std::vector<flexible_type> num_samples = {1, 3, 3, 2, 3, 3, 1, 3, 3, 3};
  const std::vector<flexible_type> session_id = {1, 2, 3, 3, 4, 4, 4, 5, 5, 5};
  const std::vector<flexible_type> prediction_id = {0, 0, 0, 1, 0,
                                                    1, 2, 0, 1, 2};
  TS_ASSERT_EQUALS(session_id.size(), num_samples.size());

  flexible_type check_sum_samples = 0;
  for (flexible_type x : num_samples) check_sum_samples += x;
  TS_ASSERT_EQUALS(check_sum, check_sum_samples);

  // produce probability and make them different by using set
  std::vector<flexible_type> predict_probability;
  for (size_t i = 0; i < num_samples.size(); i++) {
    float sum = 0;
    std::set<float> probability_set;
    flex_vec predict_score;
    for (size_t j = 0; j < class_labels.size(); j++) {
      while (true) {
        float probability = (float)(rand() % 10 + j * i);
        if (probability_set.find(probability) == probability_set.end()) {
          probability_set.insert(probability);
          sum += probability;
          predict_score.push_back(probability);
          break;
        }
      }
    }
    for (size_t j = 0; j < class_labels.size(); j++) {
      predict_score[j] /= sum;
    }
    predict_probability.push_back(predict_score);
  }

  // put all thing in a predict data
  test_activity_classifier classifier;
  gl_sarray session_id_array;
  gl_sarray prediction_id_array;
  gl_sarray num_samples_array;
  gl_sarray predict_probability_array;

  session_id_array.construct_from_vector(session_id, flex_type_enum::INTEGER);
  prediction_id_array.construct_from_vector(prediction_id,
                                            flex_type_enum::INTEGER);
  num_samples_array.construct_from_vector(num_samples, flex_type_enum::INTEGER);
  predict_probability_array.construct_from_vector(predict_probability,
                                                  flex_type_enum::VECTOR);

  classifier.set_mock_predict_data(session_id_array, prediction_id_array,
                                   num_samples_array,
                                   predict_probability_array);
  classifier.set_model_label(class_labels);



  // test for classify() with output_frequency = 'per_window'
  gl_sframe predict_result = classifier.classify(gl_sframe(), "per_window");

  // some simple tests for shape, column names
  TS_ASSERT_EQUALS(predict_result.size(), session_id.size());
  std::vector<std::string> column_names_vector = predict_result.column_names();
  std::set<std::string> column_names(column_names_vector.begin(),
                                     column_names_vector.end());
  TS_ASSERT_EQUALS(column_names.size(), 4);
  TS_ASSERT_EQUALS(column_names.count("exp_id"), 1);
  TS_ASSERT_EQUALS(column_names.count("prediction_id"), 1);
  TS_ASSERT_EQUALS(column_names.count("class"), 1);
  TS_ASSERT_EQUALS(column_names.count("probability"), 1);

  // test exp_id
  gl_sarray exp_id_array = predict_result["exp_id"];
  for (size_t i = 0; i < exp_id_array.size(); i++) {
    TS_ASSERT_EQUALS(exp_id_array[i], session_id[i]);
  }

  // build the ground truth class and prob
  std::vector<float> gt_prob;
  std::vector<flexible_type> gt_class;
  for (size_t i = 0; i < predict_probability.size(); i++) {
    std::vector<flex_float> x = predict_probability[i];
    auto max_it = std::max_element(x.begin(), x.end());
    gt_prob.push_back(x[max_it - x.begin()]);
    gt_class.push_back(class_labels[max_it - x.begin()]);
  }

  // test class
  gl_sarray class_array = predict_result["class"];
  for (size_t i = 0; i < gt_class.size(); i++) {
    TS_ASSERT_EQUALS(gt_class[i], class_array[i]);
  }

  // test prob
  gl_sarray prob_array = predict_result["probability"];
  for (size_t i = 0; i < gt_prob.size(); i++) {
    TS_ASSERT_EQUALS((float)gt_prob[i], (float)prob_array[i]);
  }

  // test classify() with output_frequency = "per_row"
  predict_result = classifier.classify(gl_sframe(), "per_row");

  // simple test for shape, column names
  TS_ASSERT_EQUALS(predict_result.size(), test_num_examples);
  column_names_vector = predict_result.column_names();
  column_names = std::set<std::string>(column_names_vector.begin(),
                                       column_names_vector.end());
  TS_ASSERT_EQUALS(column_names.size(), 2);
  TS_ASSERT_EQUALS(column_names.count("class"), 1);
  TS_ASSERT_EQUALS(column_names.count("probability"), 1);

  // ground truth class and probability for per_row
  std::vector<float> gt_prob_per_row;
  std::vector<std::string> gt_class_per_row;
  for (size_t i = 0; i < num_samples.size(); i++) {
    for (size_t j = 0; j < num_samples[i]; j++) {
      gt_prob_per_row.push_back(gt_prob[i]);
      gt_class_per_row.push_back(gt_class[i]);
    }
  }

  // test class and prob
  gl_sarray prob_array_per_row = predict_result["probability"];
  gl_sarray class_array_per_row = predict_result["class"];
  TS_ASSERT_EQUALS(gt_prob_per_row.size(), predict_result.size());
  for (size_t i = 0; i < predict_result.size(); i++) {
    TS_ASSERT_EQUALS((float)gt_prob_per_row[i], (float)prob_array_per_row[i]);
    TS_ASSERT_EQUALS(gt_class_per_row[i], class_array_per_row[i]);
  }

  // test for predict() with output_type = "class"
  gl_sarray predict_class = classifier.predict(gl_sframe(), "class");
  TS_ASSERT_EQUALS(predict_class.size(), test_num_examples);
  for (size_t i = 0; i < predict_class.size(); i++) {
    TS_ASSERT_EQUALS(predict_class[i], gt_class_per_row[i]);
  }

  // test for predict() with output_type = "probability_vector"
  gl_sarray predict_probability_vector =
      classifier.predict(gl_sframe(), "probability_vector");
  TS_ASSERT_EQUALS(predict_probability_vector.size(), test_num_examples);
  std::vector<flex_vec> gt_probability_vector;
  for (size_t i = 0; i < num_samples.size(); i++) {
    for (size_t j = 0; j < num_samples[i]; j++) {
      gt_probability_vector.push_back(predict_probability[i]);
    }
  }
  for (size_t i = 0; i < test_num_examples; i++) {
    flex_vec prob_vec = predict_probability_vector[i].get<flex_vec>();
    for (size_t j = 0; j < prob_vec.size(); j++) {
      TS_ASSERT_EQUALS(gt_probability_vector[i][j],
                       predict_probability_vector[i][j]);
    }
  }

  // test for predict_per_window() with output_type = "class"
  gl_sframe predict_per_row_class =
      classifier.predict_per_window(gl_sframe(), "class");
  TS_ASSERT_EQUALS(predict_per_row_class.size(), num_samples.size());
  std::vector<std::string> predict_per_row_column_names_vector =
      predict_per_row_class.column_names();
  std::set<std::string> predict_per_row_column_names(
      predict_per_row_column_names_vector.begin(),
      predict_per_row_column_names_vector.end());
  TS_ASSERT_EQUALS(predict_per_row_column_names.size(), 3);
  TS_ASSERT_EQUALS(predict_per_row_column_names.count("exp_id"), 1);
  TS_ASSERT_EQUALS(predict_per_row_column_names.count("prediction_id"), 1);
  TS_ASSERT_EQUALS(predict_per_row_column_names.count("class"), 1);

  // test for session_id
  gl_sarray predict_session_id = predict_per_row_class["exp_id"];
  for (size_t i = 0; i < predict_session_id.size(); i++) {
    TS_ASSERT_EQUALS(predict_session_id[i], session_id[i]);
  }

  // test for class
  gl_sarray predict_class_array = predict_per_row_class["class"];
  TS_ASSERT_EQUALS(predict_class_array.size(), gt_class.size());
  for (size_t i = 0; i < predict_class_array.size(); i++) {
    TS_ASSERT_EQUALS(predict_class_array[i], gt_class[i]);
  }

  // test for predict_per_window() with output_type = "probability_vector"
  gl_sframe predict_per_row_prob =
      classifier.predict_per_window(gl_sframe(), "probability_vector");
  TS_ASSERT_EQUALS(predict_per_row_prob.size(), num_samples.size());
  std::vector<std::string> predict_per_row_column_names_prob_vector =
      predict_per_row_prob.column_names();
  std::set<std::string> predict_per_row_column_names_prob(
      predict_per_row_column_names_prob_vector.begin(),
      predict_per_row_column_names_prob_vector.end());
  TS_ASSERT_EQUALS(predict_per_row_column_names_prob.size(), 3);
  TS_ASSERT_EQUALS(predict_per_row_column_names_prob.count("exp_id"), 1);
  TS_ASSERT_EQUALS(predict_per_row_column_names_prob.count("prediction_id"), 1);
  TS_ASSERT_EQUALS(
      predict_per_row_column_names_prob.count("probability_vector"), 1);

  // test for session_id
  gl_sarray predict_session_id_prob = predict_per_row_prob["exp_id"];
  for (size_t i = 0; i < predict_session_id_prob.size(); i++) {
    TS_ASSERT_EQUALS(predict_session_id_prob[i], session_id[i]);
  }

  // test for probability_vector
  gl_sarray predict_probability_vector_prob =
      predict_per_row_prob["probability_vector"];
  for (size_t i = 0; i < predict_probability_vector_prob.size(); i++) {
    for (size_t j = 0; j < class_labels.size(); j++) {
      TS_ASSERT_EQUALS(predict_probability_vector_prob[i][j],
                       predict_probability[i][j]);
    }
  }

}

BOOST_AUTO_TEST_CASE(test_activity_classifier_predict_topk_per_row_rank) {

  const flex_list class_labels = {"a", "b", "c", "d", "e", "f", "g"};

  // session id for each prediction
  // in this case it is [1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4,...]
  constexpr size_t session_num = 5;
  const std::vector<size_t> session_distribution = {1, 3, 5, 7, 9};
  TS_ASSERT_EQUALS(session_num, session_distribution.size());

  constexpr size_t test_num_examples = 25;
  size_t check_sum = 0;
  for (size_t num_per_session : session_distribution)
    check_sum += num_per_session;
  TS_ASSERT_EQUALS(check_sum, test_num_examples);

  /**
   * mock predict data
   * we assume the sliding window has size 3
   * num_samples indicates the number of samples in each sliding window,
   * which is betweem [1,3] in this case
   * for the first session, which has one example, will produce 1 prediction
   * for the second session, which has three examples, will produce 1
   * prediction. the third session, which has five examples, will produce 2
   * predictions. session_id indicates the session id for each prediction
   */
  const std::vector<flexible_type> num_samples = {1, 3, 3, 2, 3, 3, 1, 3, 3, 3};
  const std::vector<flexible_type> session_id = {1, 2, 3, 3, 4, 4, 4, 5, 5, 5};
  const std::vector<flexible_type> prediction_id = {0, 0, 0, 1, 0,
                                                    1, 2, 0, 1, 2};
  TS_ASSERT_EQUALS(session_id.size(), num_samples.size());

  flexible_type check_sum_samples = 0;
  for (flexible_type x : num_samples) check_sum_samples += x;
  TS_ASSERT_EQUALS(check_sum, check_sum_samples);

  // produce probability and make them different by using set
  std::vector<flexible_type> predict_probability;
  for (size_t i = 0; i < num_samples.size(); i++) {
    float sum = 0;
    std::set<float> probability_set;
    flex_vec predict_score;
    for (size_t j = 0; j < class_labels.size(); j++) {
      while (true) {
        float probability = (float)(rand() % 10 + j * i);
        if (probability_set.find(probability) == probability_set.end()) {
          probability_set.insert(probability);
          sum += probability;
          predict_score.push_back(probability);
          break;
        }
      }
    }
    for (size_t j = 0; j < class_labels.size(); j++) {
      predict_score[j] /= sum;
    }
    predict_probability.push_back(predict_score);
  }

  // put all thing in a predict data
  test_activity_classifier classifier;
  gl_sarray session_id_array;
  gl_sarray prediction_id_array;
  gl_sarray num_samples_array;
  gl_sarray predict_probability_array;

  session_id_array.construct_from_vector(session_id, flex_type_enum::INTEGER);
  prediction_id_array.construct_from_vector(prediction_id,
                                            flex_type_enum::INTEGER);
  num_samples_array.construct_from_vector(num_samples, flex_type_enum::INTEGER);
  predict_probability_array.construct_from_vector(predict_probability,
                                                  flex_type_enum::VECTOR);

  classifier.set_mock_predict_data(session_id_array, prediction_id_array,
                                   num_samples_array,
                                   predict_probability_array);
  classifier.set_model_label(class_labels);

  constexpr size_t topk = 5;
  gl_sframe predict_result =
      classifier.predict_topk(gl_sframe(), "rank", topk, "per_row");
  TS_ASSERT_EQUALS(predict_result.size(), test_num_examples * topk);
  std::vector<std::string> column_names = predict_result.column_names();
  TS_ASSERT_EQUALS(column_names.size(), 3);
  TS_ASSERT_EQUALS(column_names[0], "row_id");
  TS_ASSERT_EQUALS(column_names[1], "class");
  TS_ASSERT_EQUALS(column_names[2], "rank");

  // test row_id
  gl_sarray row_id = predict_result["row_id"];
  std::vector<size_t> gt_row_id;
  for (size_t i = 0; i < test_num_examples; i++) {
    for (size_t j = 0; j < topk; j++) {
      gt_row_id.push_back(i);
    }
  }
  TS_ASSERT_EQUALS(gt_row_id.size(), row_id.size());
  for (size_t i = 0; i < row_id.size(); i++) {
    TS_ASSERT_EQUALS(row_id[i], gt_row_id[i]);
  }

  // test rank
  std::vector<flexible_type> gt_probability_row;
  for (size_t i = 0; i < predict_probability.size(); i++) {
    for (size_t j = 0; j < num_samples[i]; j++) {
      gt_probability_row.push_back(predict_probability[i]);
    }
  }
  TS_ASSERT_EQUALS(gt_probability_row.size(), test_num_examples);
  auto argsort_prob = [=](const flexible_type& ft) {
    const flex_vec& prob_vec = ft.get<flex_vec>();
    std::vector<size_t> index_vec(prob_vec.size());
    std::iota(index_vec.begin(), index_vec.end(), 0);
    auto compare = [&](const size_t& i, const size_t& j) {
      return prob_vec[i] > prob_vec[j];
    };
    std::sort(index_vec.begin(), index_vec.end(), compare);
    return std::vector<size_t>(index_vec.begin(), index_vec.begin() + topk);
  };
  std::vector<size_t> gt_rank;
  for (size_t i = 0; i < gt_probability_row.size(); i++) {
    std::vector<size_t> index_vec = argsort_prob(gt_probability_row[i]);
    for (size_t x : index_vec) {
      gt_rank.push_back(x);
    }
  }
  gl_sarray rank = predict_result["rank"];
  TS_ASSERT_EQUALS(gt_rank.size(), rank.size());
  for (size_t i = 0; i < gt_rank.size(); i++) {
    TS_ASSERT_EQUALS(gt_rank[i], rank[i]);
  }

  // test class
  std::vector<std::string> gt_class;
  for (size_t i = 0; i < gt_rank.size(); i++) {
    gt_class.push_back(class_labels[gt_rank[i]]);
  }
  gl_sarray class_array = predict_result["class"];
  TS_ASSERT_EQUALS(gt_class.size(), class_array.size());
  for (size_t i = 0; i < gt_class.size(); i++) {
    TS_ASSERT_EQUALS(gt_class[i], class_array[i]);
  }
}

BOOST_AUTO_TEST_CASE(
    test_activity_classifier_predict_topk_per_row_probability) {
  const flex_list class_labels = {"a", "b", "c", "d", "e", "f", "g"};

  // session id for each prediction
  // in this case it is [1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4,...]
  constexpr size_t session_num = 5;
  const std::vector<size_t> session_distribution = {1, 3, 5, 7, 9};
  TS_ASSERT_EQUALS(session_num, session_distribution.size());

  constexpr size_t test_num_examples = 25;
  size_t check_sum = 0;
  for (size_t num_per_session : session_distribution)
    check_sum += num_per_session;
  TS_ASSERT_EQUALS(check_sum, test_num_examples);

  /**
   * mock predict data
   * we assume the sliding window has size 3
   * num_samples indicates the number of samples in each sliding window,
   * which is betweem [1,3] in this case
   * for the first session, which has one example, will produce 1 prediction
   * for the second session, which has three examples, will produce 1
   * prediction. the third session, which has five examples, will produce 2
   * predictions. session_id indicates the session id for each prediction
   */
  const std::vector<flexible_type> num_samples = {1, 3, 3, 2, 3, 3, 1, 3, 3, 3};
  const std::vector<flexible_type> session_id = {1, 2, 3, 3, 4, 4, 4, 5, 5, 5};
  const std::vector<flexible_type> prediction_id = {0, 0, 0, 1, 0,
                                                    1, 2, 0, 1, 2};
  TS_ASSERT_EQUALS(session_id.size(), num_samples.size());

  flexible_type check_sum_samples = 0;
  for (flexible_type x : num_samples) check_sum_samples += x;
  TS_ASSERT_EQUALS(check_sum, check_sum_samples);

  // produce probability and make them different by using set
  std::vector<flexible_type> predict_probability;
  for (size_t i = 0; i < num_samples.size(); i++) {
    float sum = 0;
    std::set<float> probability_set;
    flex_vec predict_score;
    for (size_t j = 0; j < class_labels.size(); j++) {
      while (true) {
        float probability = (float)(rand() % 10 + j * i);
        if (probability_set.find(probability) == probability_set.end()) {
          probability_set.insert(probability);
          sum += probability;
          predict_score.push_back(probability);
          break;
        }
      }
    }
    for (size_t j = 0; j < class_labels.size(); j++) {
      predict_score[j] /= sum;
    }
    predict_probability.push_back(predict_score);
  }

  // put all thing in a predict data
  test_activity_classifier classifier;
  gl_sarray session_id_array;
  gl_sarray prediction_id_array;
  gl_sarray num_samples_array;
  gl_sarray predict_probability_array;

  session_id_array.construct_from_vector(session_id, flex_type_enum::INTEGER);
  prediction_id_array.construct_from_vector(prediction_id,
                                            flex_type_enum::INTEGER);
  num_samples_array.construct_from_vector(num_samples, flex_type_enum::INTEGER);
  predict_probability_array.construct_from_vector(predict_probability,
                                                  flex_type_enum::VECTOR);

  classifier.set_mock_predict_data(session_id_array, prediction_id_array,
                                   num_samples_array,
                                   predict_probability_array);
  classifier.set_model_label(class_labels);

  // test for classify()
  // test for per_window
  constexpr size_t topk = 5;
  gl_sframe predict_result =
      classifier.predict_topk(gl_sframe(), "probability", topk, "per_row");
  TS_ASSERT_EQUALS(predict_result.size(), test_num_examples * topk);
  std::vector<std::string> column_names = predict_result.column_names();
  TS_ASSERT_EQUALS(column_names.size(), 3);
  TS_ASSERT_EQUALS(column_names[0], "row_id");
  TS_ASSERT_EQUALS(column_names[1], "class");
  TS_ASSERT_EQUALS(column_names[2], "probability");

  // test row_id
  gl_sarray row_id = predict_result["row_id"];
  std::vector<size_t> gt_row_id;
  for (size_t i = 0; i < test_num_examples; i++) {
    for (size_t j = 0; j < topk; j++) {
      gt_row_id.push_back(i);
    }
  }
  TS_ASSERT_EQUALS(gt_row_id.size(), row_id.size());
  for (size_t i = 0; i < row_id.size(); i++) {
    TS_ASSERT_EQUALS(row_id[i], gt_row_id[i]);
  }

  // test rank
  std::vector<flexible_type> gt_probability_row;
  for (size_t i = 0; i < predict_probability.size(); i++) {
    for (size_t j = 0; j < num_samples[i]; j++) {
      gt_probability_row.push_back(predict_probability[i]);
    }
  }
  TS_ASSERT_EQUALS(gt_probability_row.size(), test_num_examples);

  auto argsort_prob = [=](const flexible_type& ft) {
    const flex_vec& prob_vec = ft.get<flex_vec>();
    std::vector<size_t> index_vec(prob_vec.size());
    std::iota(index_vec.begin(), index_vec.end(), 0);
    auto compare = [&](const size_t& i, const size_t& j) {
      return prob_vec[i] > prob_vec[j];
    };
    std::sort(index_vec.begin(), index_vec.end(), compare);
    return std::vector<size_t>(index_vec.begin(), index_vec.begin() + topk);
  };

  std::vector<size_t> gt_rank;
  std::vector<float> gt_probability;
  for (size_t i = 0; i < gt_probability_row.size(); i++) {
    std::vector<size_t> index_vec = argsort_prob(gt_probability_row[i]);
    for (size_t x : index_vec) {
      gt_rank.push_back(x);
      gt_probability.push_back(gt_probability_row[i][x]);
    }
  }

  // test probability
  gl_sarray probability = predict_result["probability"];
  TS_ASSERT_EQUALS(probability.size(), gt_probability.size());
  for (size_t i = 0; i < probability.size(); i++) {
    TS_ASSERT_EQUALS((float)probability[i], (float)gt_probability[i]);
  }

  // test class
  std::vector<std::string> gt_class;
  for (size_t i = 0; i < gt_rank.size(); i++) {
    gt_class.push_back(class_labels[gt_rank[i]]);
  }

  gl_sarray class_array = predict_result["class"];
  TS_ASSERT_EQUALS(gt_class.size(), class_array.size());
  for (size_t i = 0; i < gt_class.size(); i++) {
    TS_ASSERT_EQUALS(gt_class[i], class_array[i]);
  }
}

BOOST_AUTO_TEST_CASE(test_activity_classifier_predict_topk_per_window_rank) {
  const flex_list class_labels = {"a", "b", "c", "d", "e", "f", "g"};

  // session id for each prediction
  // in this case it is [1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4,...]
  constexpr size_t session_num = 5;
  const std::vector<size_t> session_distribution = {1, 3, 5, 7, 9};
  TS_ASSERT_EQUALS(session_num, session_distribution.size());

  constexpr size_t test_num_examples = 25;
  size_t check_sum = 0;
  for (size_t num_per_session : session_distribution)
    check_sum += num_per_session;
  TS_ASSERT_EQUALS(check_sum, test_num_examples);

  /**
   * mock predict data
   * we assume the sliding window has size 3
   * num_samples indicates the number of samples in each sliding window,
   * which is betweem [1,3] in this case
   * for the first session, which has one example, will produce 1 prediction
   * for the second session, which has three examples, will produce 1
   * prediction. the third session, which has five examples, will produce 2
   * predictions. session_id indicates the session id for each prediction
   */
  const std::vector<flexible_type> num_samples = {1, 3, 3, 2, 3, 3, 1, 3, 3, 3};
  const std::vector<flexible_type> session_id = {1, 2, 3, 3, 4, 4, 4, 5, 5, 5};
  const std::vector<flexible_type> prediction_id = {0, 0, 0, 1, 0,
                                                    1, 2, 0, 1, 2};
  TS_ASSERT_EQUALS(session_id.size(), num_samples.size());

  flexible_type check_sum_samples = 0;
  for (flexible_type x : num_samples) check_sum_samples += x;
  TS_ASSERT_EQUALS(check_sum, check_sum_samples);

  // produce probability and make them different by using set
  std::vector<flexible_type> predict_probability;
  for (size_t i = 0; i < num_samples.size(); i++) {
    float sum = 0;
    std::set<float> probability_set;
    flex_vec predict_score;
    for (size_t j = 0; j < class_labels.size(); j++) {
      while (true) {
        float probability = (float)(rand() % 10 + j * i);
        if (probability_set.find(probability) == probability_set.end()) {
          probability_set.insert(probability);
          sum += probability;
          predict_score.push_back(probability);
          break;
        }
      }
    }
    for (size_t j = 0; j < class_labels.size(); j++) {
      predict_score[j] /= sum;
    }
    predict_probability.push_back(predict_score);
  }

  // put all thing in a predict data
  test_activity_classifier classifier;
  gl_sarray session_id_array;
  gl_sarray prediction_id_array;
  gl_sarray num_samples_array;
  gl_sarray predict_probability_array;

  session_id_array.construct_from_vector(session_id, flex_type_enum::INTEGER);
  prediction_id_array.construct_from_vector(prediction_id,
                                            flex_type_enum::INTEGER);
  num_samples_array.construct_from_vector(num_samples, flex_type_enum::INTEGER);
  predict_probability_array.construct_from_vector(predict_probability,
                                                  flex_type_enum::VECTOR);

  classifier.set_mock_predict_data(session_id_array, prediction_id_array,
                                   num_samples_array,
                                   predict_probability_array);
  classifier.set_model_label(class_labels);

  // test for classify()
  // test for per_window
  constexpr size_t topk = 5;
  gl_sframe predict_result =
      classifier.predict_topk(gl_sframe(), "rank", topk, "per_window");
  TS_ASSERT_EQUALS(predict_result.size(), num_samples.size() * topk);
  std::vector<std::string> column_names = predict_result.column_names();
  TS_ASSERT_EQUALS(column_names.size(), 4);
  TS_ASSERT_EQUALS(column_names[0], "exp_id");
  TS_ASSERT_EQUALS(column_names[1], "prediction_id");
  TS_ASSERT_EQUALS(column_names[2], "class");
  TS_ASSERT_EQUALS(column_names[3], "rank");

  // test exp_id
  gl_sarray exp_id = predict_result["exp_id"];
  std::vector<size_t> gt_exp_id;
  for (size_t i = 0; i < session_id.size(); i++) {
    for (size_t j = 0; j < topk; j++) {
      gt_exp_id.push_back(session_id[i]);
    }
  }
  TS_ASSERT_EQUALS(gt_exp_id.size(), exp_id.size());
  for (size_t i = 0; i < exp_id.size(); i++) {
    TS_ASSERT_EQUALS(exp_id[i], gt_exp_id[i]);
  }

  // test rank
  auto argsort_prob = [=](const flexible_type& ft) {
    const flex_vec& prob_vec = ft.get<flex_vec>();
    std::vector<size_t> index_vec(prob_vec.size());
    std::iota(index_vec.begin(), index_vec.end(), 0);
    auto compare = [&](const size_t& i, const size_t& j) {
      return prob_vec[i] > prob_vec[j];
    };
    std::sort(index_vec.begin(), index_vec.end(), compare);
    return std::vector<size_t>(index_vec.begin(), index_vec.begin() + topk);
  };
  std::vector<size_t> gt_rank;
  for (size_t i = 0; i < predict_probability.size(); i++) {
    std::vector<size_t> index_vec = argsort_prob(predict_probability[i]);
    for (size_t x : index_vec) {
      gt_rank.push_back(x);
    }
  }
  gl_sarray rank = predict_result["rank"];
  TS_ASSERT_EQUALS(gt_rank.size(), rank.size());
  for (size_t i = 0; i < gt_rank.size(); i++) {
    TS_ASSERT_EQUALS(gt_rank[i], rank[i]);
  }

  // test class
  std::vector<std::string> gt_class;
  for (size_t i = 0; i < gt_rank.size(); i++) {
    gt_class.push_back(class_labels[gt_rank[i]]);
  }
  gl_sarray class_array = predict_result["class"];
  TS_ASSERT_EQUALS(class_array.size(), gt_class.size());
  for (size_t i = 0; i < class_array.size(); i++) {
    TS_ASSERT_EQUALS(class_array[i], gt_class[i]);
  }
}

BOOST_AUTO_TEST_CASE(
    test_activity_classifier_predict_topk_per_window_probability) {
  const flex_list class_labels = {"a", "b", "c", "d", "e", "f", "g"};

  // session id for each prediction
  // in this case it is [1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4,...]
  constexpr size_t session_num = 5;
  const std::vector<size_t> session_distribution = {1, 3, 5, 7, 9};
  TS_ASSERT_EQUALS(session_num, session_distribution.size());

  constexpr size_t test_num_examples = 25;
  size_t check_sum = 0;
  for (size_t num_per_session : session_distribution)
    check_sum += num_per_session;
  TS_ASSERT_EQUALS(check_sum, test_num_examples);

  /**
   * mock predict data
   * we assume the sliding window has size 3
   * num_samples indicates the number of samples in each sliding window,
   * which is betweem [1,3] in this case
   * for the first session, which has one example, will produce 1 prediction
   * for the second session, which has three examples, will produce 1
   * prediction. the third session, which has five examples, will produce 2
   * predictions. session_id indicates the session id for each prediction
   */
  const std::vector<flexible_type> num_samples = {1, 3, 3, 2, 3, 3, 1, 3, 3, 3};
  const std::vector<flexible_type> session_id = {1, 2, 3, 3, 4, 4, 4, 5, 5, 5};
  const std::vector<flexible_type> prediction_id = {0, 0, 0, 1, 0,
                                                    1, 2, 0, 1, 2};
  TS_ASSERT_EQUALS(session_id.size(), num_samples.size());

  flexible_type check_sum_samples = 0;
  for (flexible_type x : num_samples) check_sum_samples += x;
  TS_ASSERT_EQUALS(check_sum, check_sum_samples);

  // produce probability and make them different by using set
  std::vector<flexible_type> predict_probability;
  for (size_t i = 0; i < num_samples.size(); i++) {
    float sum = 0;
    std::set<float> probability_set;
    flex_vec predict_score;
    for (size_t j = 0; j < class_labels.size(); j++) {
      while (true) {
        float probability = (float)(rand() % 10 + j * i);
        if (probability_set.find(probability) == probability_set.end()) {
          probability_set.insert(probability);
          sum += probability;
          predict_score.push_back(probability);
          break;
        }
      }
    }
    for (size_t j = 0; j < class_labels.size(); j++) {
      predict_score[j] /= sum;
    }
    predict_probability.push_back(predict_score);
  }

  // put all thing in a predict data
  test_activity_classifier classifier;
  gl_sarray session_id_array;
  gl_sarray prediction_id_array;
  gl_sarray num_samples_array;
  gl_sarray predict_probability_array;

  session_id_array.construct_from_vector(session_id, flex_type_enum::INTEGER);
  prediction_id_array.construct_from_vector(prediction_id,
                                            flex_type_enum::INTEGER);
  num_samples_array.construct_from_vector(num_samples, flex_type_enum::INTEGER);
  predict_probability_array.construct_from_vector(predict_probability,
                                                  flex_type_enum::VECTOR);

  classifier.set_mock_predict_data(session_id_array, prediction_id_array,
                                   num_samples_array,
                                   predict_probability_array);
  classifier.set_model_label(class_labels);

  // test for classify()
  // test for per_window
  constexpr size_t topk = 5;
  gl_sframe predict_result =
      classifier.predict_topk(gl_sframe(), "probability", topk, "per_window");
  TS_ASSERT_EQUALS(predict_result.size(), num_samples.size() * topk);
  std::vector<std::string> column_names = predict_result.column_names();
  TS_ASSERT_EQUALS(column_names.size(), 4);
  TS_ASSERT_EQUALS(column_names[0], "exp_id");
  TS_ASSERT_EQUALS(column_names[1], "prediction_id");
  TS_ASSERT_EQUALS(column_names[2], "class");
  TS_ASSERT_EQUALS(column_names[3], "probability");

  // test exp_id
  gl_sarray exp_id = predict_result["exp_id"];
  std::vector<size_t> gt_exp_id;
  for (size_t i = 0; i < session_id.size(); i++) {
    for (size_t j = 0; j < topk; j++) {
      gt_exp_id.push_back(session_id[i]);
    }
  }
  TS_ASSERT_EQUALS(gt_exp_id.size(), exp_id.size());
  for (size_t i = 0; i < exp_id.size(); i++) {
    TS_ASSERT_EQUALS(exp_id[i], gt_exp_id[i]);
  }

  // test probability
  auto argsort_prob = [=](const flexible_type& ft) {
    const flex_vec& prob_vec = ft.get<flex_vec>();
    std::vector<size_t> index_vec(prob_vec.size());
    std::iota(index_vec.begin(), index_vec.end(), 0);
    auto compare = [&](const size_t& i, const size_t& j) {
      return prob_vec[i] > prob_vec[j];
    };
    std::sort(index_vec.begin(), index_vec.end(), compare);
    return std::vector<size_t>(index_vec.begin(), index_vec.begin() + topk);
  };
  std::vector<size_t> gt_rank;
  std::vector<size_t> gt_probability;
  for (size_t i = 0; i < predict_probability.size(); i++) {
    std::vector<size_t> index_vec = argsort_prob(predict_probability[i]);
    for (size_t x : index_vec) {
      gt_rank.push_back(x);
      gt_probability.push_back(predict_probability[i][x]);
    }
  }
  gl_sarray probability = predict_result["probability"];
  TS_ASSERT_EQUALS(probability.size(), gt_probability.size());

  // test class
  std::vector<std::string> gt_class;
  for (size_t i = 0; i < gt_rank.size(); i++) {
    gt_class.push_back(class_labels[gt_rank[i]]);
  }
  gl_sarray class_array = predict_result["class"];
  TS_ASSERT_EQUALS(class_array.size(), gt_class.size());
  for (size_t i = 0; i < class_array.size(); i++) {
    TS_ASSERT_EQUALS(class_array[i], gt_class[i]);
  }
}

}  // namespace
}  // namespace activity_classification
}  // namespace turi

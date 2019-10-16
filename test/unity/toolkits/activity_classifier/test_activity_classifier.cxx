/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE test_activity_classifier

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <iostream>

#include <toolkits/activity_classification/activity_classifier.hpp>

namespace turi {
namespace activity_classification {
namespace {

class test_activity_classifier : public activity_classifier {
 public:
  class mock_iterator : public data_iterator {
    flex_list class_labels_;
    flex_list feature_names_;
    void reset() override { return; }

    bool has_next_batch() const override { return true; }

    const flex_list& class_labels() const override { return class_labels_; }

    const flex_list& feature_names() const override { return feature_names_; }

    flex_type_enum session_id_type() const override {
      return flex_type_enum::UNDEFINED;
    }

    batch next_batch(size_t batch_size) override {
      batch result;
      return result;
    }
  };

  gl_sframe predict_data;

  std::unique_ptr<data_iterator> create_iterator(
      gl_sframe data, bool requires_labels, bool is_train,
      bool use_data_augmentation) const override {
    return std::unique_ptr<mock_iterator>(new mock_iterator());
  }

  gl_sframe perform_inference(data_iterator* data) const override {
    return predict_data;
  }

  void set_mock_predict_data(gl_sarray session, gl_sarray num_samples,
                             gl_sarray prob) {
    predict_data = gl_sframe();
    predict_data.add_column(session, "session_id");
    predict_data.add_column(num_samples, "num_samples");
    predict_data.add_column(prob, "preds");
  }

  void set_model_label(flex_list labels) { state.emplace("classes", labels); }
};

BOOST_AUTO_TEST_CASE(test_activity_classifier_classify_and_predict) {
  static constexpr size_t test_num_examples = 25;
  static constexpr size_t session_num = 5;
  const flex_list class_labels = {"a", "b", "c", "d", "e", "f", "g"};
  const std::vector<size_t> session_distribution = {1, 3, 5, 7, 9};
  TS_ASSERT_EQUALS(session_num, session_distribution.size());
  size_t check_sum = 0;
  for (auto& x : session_distribution) {
    check_sum += x;
  }
  TS_ASSERT_EQUALS(check_sum, test_num_examples);

  // mock predict data
  const std::vector<flexible_type> session_id = {1, 2, 3, 3, 4, 4, 4, 5, 5, 5};
  const std::vector<flexible_type> num_samples = {1, 3, 3, 2, 3, 3, 1, 3, 3, 3};
  TS_ASSERT_EQUALS(session_id.size(), num_samples.size());
  flexible_type check_sum_samples = 0;
  for (auto& x : num_samples) {
    check_sum_samples += x;
  }
  TS_ASSERT_EQUALS(check_sum, check_sum_samples);
  std::vector<flexible_type> predict_probability;
  for (size_t i = 0; i < num_samples.size(); i++) {
    float sum = 0;
    flex_vec predict_score;
    for (size_t j = 0; j < class_labels.size(); j++) {
      predict_score.push_back((double)(rand() % 10 + j * i));
      sum += predict_score.back();
    }
    for (size_t j = 0; j < class_labels.size(); j++) {
      predict_score[j] /= sum;
    }
    predict_probability.push_back(predict_score);
  }

  // put all thing in a predict data
  test_activity_classifier classifier;
  gl_sarray session_id_array;
  gl_sarray num_samples_array;
  gl_sarray predict_probability_array;

  session_id_array.construct_from_vector(session_id, flex_type_enum::INTEGER);
  num_samples_array.construct_from_vector(num_samples, flex_type_enum::INTEGER);
  predict_probability_array.construct_from_vector(predict_probability,
                                                  flex_type_enum::VECTOR);

  classifier.set_mock_predict_data(session_id_array, num_samples_array,
                                   predict_probability_array);
  classifier.set_model_label(class_labels);

  // test for classify()
  // test for per_window
  gl_sframe predict_result = classifier.classify(gl_sframe(), "per_window");

  // some simple tests for shape, column names
  TS_ASSERT_EQUALS(predict_result.size(), session_id.size());
  std::vector<std::string> column_names = predict_result.column_names();
  TS_ASSERT_EQUALS(column_names.size(), 4);
  TS_ASSERT_EQUALS(column_names[0], "prediction_id");
  TS_ASSERT_EQUALS(column_names[1], "exp_id");
  TS_ASSERT_EQUALS(column_names[2], "class");
  TS_ASSERT_EQUALS(column_names[3], "probability");

  // test prediction_id
  gl_sarray prediction_id_array = predict_result["prediction_id"];
  for (size_t i = 0; i < prediction_id_array.size(); i++) {
    TS_ASSERT_EQUALS(prediction_id_array[i], i);
  }

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

  // test for per_row
  predict_result = classifier.classify(gl_sframe(), "per_row");

  // simple test for shape, column names
  TS_ASSERT_EQUALS(predict_result.size(), test_num_examples);
  column_names = predict_result.column_names();
  TS_ASSERT_EQUALS(column_names.size(), 2);
  TS_ASSERT_EQUALS(column_names[0], "class");
  TS_ASSERT_EQUALS(column_names[1], "probability");

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

  // test for predict()
  // ouput_type = class
  gl_sarray predict_class = classifier.predict(gl_sframe(), "class");
  TS_ASSERT_EQUALS(predict_class.size(), test_num_examples);
  for (size_t i = 0; i < predict_class.size(); i++) {
    TS_ASSERT_EQUALS(predict_class[i], gt_class_per_row[i]);
  }

  // output_type = probability_vector
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

  // test for predict_per_window()
  // output_type = class
  gl_sframe predict_per_row_class =
      classifier.predict_per_window(gl_sframe(), "class");
  TS_ASSERT_EQUALS(predict_per_row_class.size(), num_samples.size());
  std::vector<std::string> predict_per_row_column_names =
      predict_per_row_class.column_names();
  TS_ASSERT_EQUALS(predict_per_row_column_names.size(), 3);
  TS_ASSERT_EQUALS(predict_per_row_column_names[0], "prediction_id");
  TS_ASSERT_EQUALS(predict_per_row_column_names[1], "session_id");
  TS_ASSERT_EQUALS(predict_per_row_column_names[2], "class");

  // test for prediction_id
  gl_sarray predict_prediction_id = predict_per_row_class["prediction_id"];
  for (size_t i = 0; i < predict_prediction_id.size(); i++) {
    TS_ASSERT_EQUALS(predict_prediction_id[i], i);
  }

  // test for session_id
  gl_sarray predict_session_id = predict_per_row_class["session_id"];
  for (size_t i = 0; i < predict_session_id.size(); i++) {
    TS_ASSERT_EQUALS(predict_session_id[i], session_id[i]);
  }

  // test for class
  gl_sarray predict_class_array = predict_per_row_class["class"];
  TS_ASSERT_EQUALS(predict_class_array.size(), gt_class.size());
  for (size_t i = 0; i < predict_class_array.size(); i++) {
    TS_ASSERT_EQUALS(predict_class_array[i], gt_class[i]);
  }

  // output_type = probability_vector
  gl_sframe predict_per_row_prob =
      classifier.predict_per_window(gl_sframe(), "probability_vector");
  TS_ASSERT_EQUALS(predict_per_row_prob.size(), num_samples.size());
  std::vector<std::string> predict_per_row_column_names_prob =
      predict_per_row_prob.column_names();
  TS_ASSERT_EQUALS(predict_per_row_column_names_prob.size(), 3);
  TS_ASSERT_EQUALS(predict_per_row_column_names_prob[0], "prediction_id");
  TS_ASSERT_EQUALS(predict_per_row_column_names_prob[1], "session_id");
  TS_ASSERT_EQUALS(predict_per_row_column_names_prob[2], "probability_vector");

  // test for prediction_id
  gl_sarray predict_prediction_id_prob = predict_per_row_prob["prediction_id"];
  for (size_t i = 0; i < predict_prediction_id_prob.size(); i++) {
    TS_ASSERT_EQUALS(predict_prediction_id_prob[i], i);
  }

  // test for session_id
  gl_sarray predict_session_id_prob = predict_per_row_prob["session_id"];
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

}  // namespace
}  // namespace activity_classification
}  // namespace turi
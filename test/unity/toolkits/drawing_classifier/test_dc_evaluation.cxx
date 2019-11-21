/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_dc_evaluation

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <ml/neural_net/model_spec.hpp>
#include <toolkits/coreml_export/mlmodel_include.hpp>
#include <toolkits/drawing_classifier/drawing_classifier.hpp>
#include <toolkits/evaluation/metrics.hpp>

namespace turi {
namespace drawing_classifer {
namespace {

struct predict_sframe_generator {
  template <class T>
  gl_sarray operator()(size_t num_of_rows, size_t num_of_classes,
                       size_t batch_size, T generator) {
    constexpr char const* dummy_name = "dummy_name";
    gl_sframe_writer writer({dummy_name}, {flex_type_enum::VECTOR},
                            /* num_segments */ 1);

    size_t ii = 0;
    while (ii < num_of_rows) {
      size_t real_batch_size =
          ii + batch_size > num_of_rows ? num_of_rows - ii : batch_size;

      ii += batch_size;

      for (size_t jj = 0; jj < real_batch_size; ++jj) {
        flex_vec vec_copy(generator(ii + jj, num_of_classes));
        TS_ASSERT_EQUALS(vec_copy.size(), num_of_classes);
        auto pbeg = std::begin(vec_copy);
        auto pend = pbeg + num_of_classes;
        float my_sum = std::accumulate(pbeg, pend, 0.0f, std::plus<float>());
        std::transform(pbeg, pend, pbeg,
                       [=](float num) { return num / my_sum; });
        writer.write({std::move(vec_copy)}, 0);
      }
    }
    // since we are using push front;
    return writer.close()[dummy_name];
  }
};

struct random_result_generator {
  flex_vec operator()(size_t row_id, size_t num_of_preds) {
    result_.resize(num_of_preds);
    result_.clear();

    std::generate(result_.begin(), result_.end(),
                  [&]() { return dist_(eng_); });

    return result_;
  };

 private:
  flex_vec result_;
  std::default_random_engine eng_;
  std::uniform_int_distribution<float> dist_{0, 35};
};

// always predict the first label regardless of the input.
struct fixed_result_generator {
  flex_vec operator()(size_t row_id, size_t num_of_preds) {
    if (!num_of_preds) throw std::logic_error("num_of_preds should > 0");
    result_.clear();
    result_.resize(num_of_preds);
    result_[0] = 1.0;
    return result_;
  };

 private:
  flex_vec result_;
};

}  // anonymous namespace

BOOST_AUTO_TEST_CASE(test_dc_evaluation_all_correct) {
  size_t num_of_rows = 10;
  size_t num_of_classes = 4;
  size_t batch_size = 3;

  // okay, generate the labels
  flex_list labels;
  labels.resize(num_of_classes);
  std::iota(labels.begin(), labels.end(), 0);

  // okay, set up the state
  auto predictions = predict_sframe_generator{}(
      num_of_rows, num_of_classes, batch_size, fixed_result_generator());

  TS_ASSERT_EQUALS(predictions.size(), num_of_rows);
  TS_ASSERT_EQUALS(predictions.dtype(), flex_type_enum::VECTOR);

  auto evaluate = [&](gl_sframe data, std::string metric) {
    auto result = evaluation::compute_classifier_metrics(
        data, "target", metric, predictions,
        {{"classes", labels}});
    return result;
  };

  // targets all point to '0'
  std::vector<flexible_type> targets(num_of_rows, labels.front());
  gl_sframe data({{"target", targets}});

  auto metrics = evaluate(data, "accuracy");
  TS_ASSERT_EQUALS(variant_get_value<double>(metrics["accuracy"]), 1.0);

  metrics = evaluate(data, "precision");
  TS_ASSERT_EQUALS(variant_get_value<double>(metrics["precision"]), 1.0);

  metrics = evaluate(data, "recall");
  TS_ASSERT_EQUALS(variant_get_value<double>(metrics["recall"]), 1.0);
}


BOOST_AUTO_TEST_CASE(test_dc_evaluation_all_wrong) {
  size_t num_of_rows = 10;
  size_t num_of_classes = 4;
  size_t batch_size = 3;

  // okay, generate the labels
  flex_list labels;
  labels.resize(num_of_classes);
  std::iota(labels.begin(), labels.end(), 0);

  // okay, set up the state
  auto predictions = predict_sframe_generator{}(
      num_of_rows, num_of_classes, batch_size, fixed_result_generator());

  TS_ASSERT_EQUALS(predictions.size(), num_of_rows);
  TS_ASSERT_EQUALS(predictions.dtype(), flex_type_enum::VECTOR);

  auto evaluate = [&](gl_sframe data, std::string metric) {
    auto result = evaluation::compute_classifier_metrics(
        data, "target", metric, predictions,
        {{"classes", labels}});
    return result;
  };

  // targets all point to '2'
  std::vector<flexible_type> targets(num_of_rows, labels.back());
  gl_sframe data({{"target", targets}});

  auto metrics = evaluate(data, "accuracy");
  TS_ASSERT_EQUALS(variant_get_value<double>(metrics["accuracy"]), 0);

  // undefined this time true positive + true negative = 0;
  metrics = evaluate(data, "precision");
  TS_ASSERT_EQUALS(
      variant_get_value<flexible_type>(metrics["precision"]), FLEX_UNDEFINED);

  metrics = evaluate(data, "recall");
  TS_ASSERT_EQUALS(variant_get_value<double>(metrics["recall"]), 0);
}

}  // namespace drawing_classifer
}  // namespace turi
// Data structures
#define BOOST_TEST_MODULE test_evaluation

#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <unity/lib/unity_sarray.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <sframe/sframe.hpp>

#include <vector>
#include <string>
#include <random/random.hpp>

#include <sframe/testing_utils.hpp>
#include <util/testing_utils.hpp>
#include <unity/toolkits/recsys/models.hpp>
#include <unity/toolkits/evaluation/metrics.hpp>
#include <sframe/testing_utils.hpp>
#include <util/testing_utils.hpp>


namespace turi {
namespace evaluation {

BOOST_AUTO_TEST_CASE(test_rmse_and_max_error) {
    size_t num_observations = 5000;
    random::seed(0);

    auto predictions = std::vector<flexible_type>(num_observations);
    auto targets = std::vector<flexible_type>(num_observations);
    double total = 0.0;
    double true_max_error = 0;
    for (size_t i = 0; i < num_observations; ++i) {
      predictions[i] = random::fast_uniform<double>(0, 1);
      targets[i] = random::fast_uniform<double>(0, 1);
      double err = std::abs((double)predictions[i] - (double)targets[i]);
      total += std::pow(err, 2);
      true_max_error = std::max(true_max_error, err);
    }
    double true_rmse = std::pow(total / (double) num_observations, .5);
 
    std::shared_ptr<sarray<flexible_type> > predictions_sa = make_testing_sarray(
          flex_type_enum::FLOAT, predictions);
    std::shared_ptr<sarray<flexible_type> > targets_sa = make_testing_sarray(
          flex_type_enum::FLOAT, targets);
    std::shared_ptr<unity_sarray> unity_targets_sa = std::make_shared<unity_sarray>();
    unity_targets_sa->construct_from_sarray(targets_sa);
    std::shared_ptr<unity_sarray> unity_predictions_sa= std::make_shared<unity_sarray>();
    unity_predictions_sa->construct_from_sarray(predictions_sa);

    variant_type rmse = evaluation::_supervised_streaming_evaluator(
                        unity_targets_sa, unity_predictions_sa, "rmse");
    variant_type max_error = evaluation::_supervised_streaming_evaluator(
                        unity_targets_sa, unity_predictions_sa, "max_error");
    TS_ASSERT(std::abs(variant_get_value<double>(rmse) - true_rmse) < 1e-15);
    TS_ASSERT(std::abs(variant_get_value<double>(max_error) - true_max_error) < 1e-15);
}

BOOST_AUTO_TEST_CASE(test_roc_curve) {
    // Arrange
    size_t num_observations = 20;
    random::seed(0);
    size_t true_positive = 0;
    size_t true_negative = 0;
    size_t false_positive = 0;
    size_t false_negative = 0;

    auto predictions = std::vector<flexible_type>(num_observations);
    auto targets = std::vector<flexible_type>(num_observations);

    for (size_t i = 0; i < num_observations; ++i) {
      predictions[i] = float(i) / num_observations;
      // random::fast_uniform<double>(0, 1);
      targets[i] = random::fast_uniform<size_t>(0, 1);
      if (targets[i] == 1) {
        if (predictions[i] == 1) {
          true_positive += 1;
        } else {
          false_negative += 1;
        }
      } else {
        if (predictions[i] == 1) {
          false_positive += 1;
        } else {
          true_negative += 1;
        }
      }
      size_t n_bins = 100;
      size_t bin = std::floor((double) predictions[i] * (double) n_bins);
      if (bin == n_bins) bin -= 1;
      //logprogress_stream << predictions[i] 
      //                   << " " 
      //                   << bin
      //                   << " "
      //                   << targets[i]
      //                   << " " 
      //                   << std::endl;
    }
 
    std::shared_ptr<sarray<flexible_type> > predictions_sa = make_testing_sarray(
          flex_type_enum::FLOAT, predictions);
    std::shared_ptr<sarray<flexible_type> > targets_sa = make_testing_sarray(
          flex_type_enum::INTEGER, targets);
    std::shared_ptr<unity_sarray> unity_targets_sa = std::make_shared<unity_sarray>();
    unity_targets_sa->construct_from_sarray(targets_sa);
    std::shared_ptr<unity_sarray> unity_predictions_sa= std::make_shared<unity_sarray>();
    unity_predictions_sa->construct_from_sarray(predictions_sa);

    // Act
    std::map<std::string, flexible_type> kwargs {
               {"average", FLEX_UNDEFINED}, 
               {"binary", true}}; 
    variant_type result = evaluation::_supervised_streaming_evaluator(
             unity_targets_sa, unity_predictions_sa, "roc_curve", kwargs);

    // Assert
    //logprogress_stream << "Now printing." << std::endl;
    //auto sf = *(variant_get_value<std::shared_ptr<unity_sframe>>(result))->get_underlying_sframe();
    //sf.debug_print();
}


BOOST_AUTO_TEST_CASE(test_accuracy) {
    size_t num_observations = 5000;
    random::seed(0);
    size_t true_positive = 0;
    size_t true_negative = 0;
    size_t false_positive = 0;
    size_t false_negative = 0;

    auto predictions = std::vector<flexible_type>(num_observations);
    auto pred_vectors = std::vector<flexible_type>(num_observations);
    auto pred_dicts = std::vector<flexible_type>(num_observations);
    auto targets = std::vector<flexible_type>(num_observations);

    for (size_t i = 0; i < num_observations; ++i) {
      double prob = random::fast_uniform<double>(0.0, 1.0);
      predictions[i] = prob > 0.5 ? 1 : 0;
      pred_vectors[i] = flex_vec({1.0 - prob, prob});
      pred_dicts[i] = flex_dict({{0, 1.0 - prob}, {1, prob}});
      targets[i] = random::fast_uniform<size_t>(0, 1);
      if (targets[i] == 1) {
        if (predictions[i] == 1) {
          true_positive += 1;
        } else {
          false_negative += 1;
        }
      } else {
        if (predictions[i] == 1) {
          false_positive += 1;
        } else {
          true_negative += 1;
        }
      }
    }
 
    std::shared_ptr<sarray<flexible_type> > predictions_sa = make_testing_sarray(
          flex_type_enum::FLOAT, predictions);
    std::shared_ptr<sarray<flexible_type> > targets_sa = make_testing_sarray(
          flex_type_enum::INTEGER, targets);
    std::shared_ptr<unity_sarray> unity_targets_sa = std::make_shared<unity_sarray>();
    unity_targets_sa->construct_from_sarray(targets_sa);
    std::shared_ptr<unity_sarray> unity_predictions_sa= std::make_shared<unity_sarray>();
    unity_predictions_sa->construct_from_sarray(predictions_sa);
     
    std::map<std::string, flexible_type> kwargs {
               {"average", "micro"}, 
               {"binary", true}}; 
    variant_type result = evaluation::_supervised_streaming_evaluator(
             unity_targets_sa, unity_predictions_sa, "roc_curve", kwargs);
    variant_type accuracy = evaluation::_supervised_streaming_evaluator(
                        unity_targets_sa, unity_predictions_sa, "accuracy");
    double true_accuracy =
        static_cast<double>(true_positive + true_negative) / num_observations;
    TS_ASSERT_DELTA(variant_get_value<double>(accuracy), true_accuracy, 1e-15);

    // Test compute_classifier_metric API
    gl_sframe data({{"target", gl_sarray(unity_targets_sa)}});
    std::map<std::string, flexible_type> opts = {{"classes", flex_list({0,1})}};
    variant_map_type vec_result = compute_classifier_metrics(
        data, "target", "accuracy", gl_sarray(pred_vectors), opts);
    TS_ASSERT_DELTA(variant_get_value<double>(vec_result["accuracy"]),
                    true_accuracy, 1e-15);

    variant_map_type dict_result = compute_classifier_metrics(
        data, "target", "accuracy", gl_sarray(pred_dicts), opts);
    TS_ASSERT_DELTA(variant_get_value<double>(dict_result["accuracy"]),
                    true_accuracy, 1e-15);
}

}  // namespace evaluation
}  // namespace turi

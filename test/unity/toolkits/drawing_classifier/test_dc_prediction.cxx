/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_dc_data_iterator

#include <toolkits/drawing_classifier/dc_data_iterator.hpp>
#include <toolkits/drawing_classifier/drawing_classifier.hpp>

#include <algorithm>

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <model_server/lib/image_util.hpp>

#include "../neural_net/neural_net_mocks.hpp"
#include "dc_data_utils.hpp"
#include "dc_mock_interface.hpp"

namespace turi {
namespace drawing_classifier {

namespace {

using turi::neural_net::compute_context;
using turi::neural_net::float_array_map;
using turi::neural_net::shared_float_array;
using turi::neural_net::mock_compute_context;
using turi::neural_net::mock_model_backend;
using turi::neural_net::model_backend;
using turi::neural_net::model_spec;
using turi::neural_net::shared_float_array;


/* ==================== global variables  ====================== */

/**
 * tuple of batch_size, num_of_rows, num_of_classes
 *
 * batch_size, num_of_rows, num_of_classes
 * 1. batch_size > num_of_rows
 * 2. batch_size == num_of_rows
 * 3. num_of_rows can divide batch_size
 * 4. num_of_rows can not divide batch_size
 *
 **/
std::vector<std::tuple<unsigned, unsigned, unsigned>> TEST_CASES = {
    {2, 1, 1}, {2, 1, 2}, {2, 2, 2}, {2, 4, 2},
    {2, 4, 3}, {2, 5, 2}, {2, 5, 3}};

constexpr auto PRED_NAME = "preds";

/* ============== utils borrowed from gl_sframe.cxx ============= */
// TODO: create a common util class
void _assert_flexvec_equals(const std::vector<flexible_type>& sa,
                            const std::vector<flexible_type>& sb) {
  TS_ASSERT_EQUALS(sa.size(), sb.size());
  for (size_t i = 0; i < sa.size(); ++i) {
    if (sa[i] != sb[i]) {
      std::cout << sa[i] << " vs " << sb[i] << std::endl;
    }
    TS_ASSERT_EQUALS(sa[i], sb[i]);
  }
}

void _assert_sframe_equals(gl_sframe sa, gl_sframe sb) {
  TS_ASSERT_EQUALS(sa.size(), sb.size());
  TS_ASSERT_EQUALS(sa.num_columns(), sb.num_columns());
  auto a_cols = sa.column_names();
  auto b_cols = sb.column_names();
  std::sort(a_cols.begin(), a_cols.end());
  std::sort(b_cols.begin(), b_cols.end());
  for (size_t i = 0; i < a_cols.size(); ++i)
    TS_ASSERT_EQUALS(a_cols[i], b_cols[i]);
  sb = sb[sa.column_names()];
  for (size_t i = 0; i < sa.size(); ++i) {
    _assert_flexvec_equals(sa[i], sb[i]);
  }
}

/* ========================= test drivers & utils ======================== */

#ifndef NDEBUG
#define log_for_debug(x) \
  { logprogress_stream << ">>>" << (x) << std::endl; }
#else
#define log_for_debug(x) {}
#endif

struct mock_perform_inference : public test_drawing_classifier {
  inline gl_sframe get_inference_result(data_iterator* data) {
    return perform_inference(data);
  }
};

gl_sframe set_up_perform_inference(
    test_drawing_classifier& mock_model,
    std::unique_ptr<mock_model_backend>& mock_backend,
    std::unique_ptr<mock_compute_context>& mock_context, size_t batch_size,
    const size_t num_of_rows, const size_t num_of_classes) {
  ASSERT_MSG(num_of_rows > 0, "num_of_rows should be bigger than 0");
  ASSERT_MSG(num_of_classes > 0, "num_of_classes should be bigger than 0");
  ASSERT_MSG(batch_size > 0, "batch_size should be bigger than 0");

  gl_sframe expected_sf;
  {
    std::vector<float> buffer(num_of_classes * batch_size);

    std::default_random_engine eng;
    std::uniform_int_distribution<float> distribution(0, 20);

    gl_sframe_writer writer({PRED_NAME}, {flex_type_enum::VECTOR},
                            /* num_segments */ 1);

    size_t ii = 0;
    while (ii < num_of_rows) {
      buffer.clear();

      size_t repeat =
          ii + batch_size > num_of_rows ? num_of_rows - ii : batch_size;

      ii += batch_size;

      for (size_t jj = 0; jj < repeat; ++jj) {
        auto pbeg = buffer.begin() + jj * num_of_classes;
        auto pend = pbeg + num_of_classes;
        std::generate(pbeg, pend, [&]() { return distribution(eng); });
        float my_sum = std::accumulate(pbeg, pend, 0.0f, std::plus<float>());
        std::transform(pbeg, pend, pbeg,
                       [=](float num) { return num / my_sum; });

        writer.write({flex_vec{pbeg, pend}}, 0);
      }

      auto to_return =
          shared_float_array::copy(buffer.data(), {num_of_classes, batch_size});

      mock_backend->predict_calls_.push_back([=](const float_array_map& input) {
        TS_ASSERT_EQUALS(input.at("input").size(), batch_size * 28 * 28);
        return float_array_map({{"output", to_return}});
      });
    }
    // since we are using push front;
    expected_sf = writer.close();
  }

  TS_ASSERT_EQUALS(expected_sf.size(), num_of_rows);

  TS_ASSERT_EQUALS(mock_backend->predict_calls_.size(),
                   num_of_rows % batch_size ? num_of_rows / batch_size + 1
                                            : num_of_rows / batch_size);

  mock_model.add_or_update_state(
      {{"num_classes", num_of_classes},
       {"batch_size", batch_size}});

  auto create_drawing_classifier_impl =
      [&mock_backend](const float_array_map&, size_t batch_size,
                      size_t num_classes) { return std::move(mock_backend); };

  mock_context->create_drawing_classifier_calls_.push_back(
      create_drawing_classifier_impl);

  // be extremely careful about reference a unique_ptr
  // never use mock_context after this line
  auto create_compute_context_impl = [&] { return std::move(mock_context); };
  mock_model.create_compute_context_calls_.push_back(
      create_compute_context_impl);

  return expected_sf;
}

std::unique_ptr<data_iterator> prepare_data_for_prediction(
    bool is_bitmap_based, size_t num_of_rows, size_t num_of_classes) {
  flex_list class_labels;

  class_labels.reserve(num_of_classes);
  for (size_t ii = 0; ii < num_of_classes; ++ii) {
    class_labels.push_back(std::to_string(ii));
  }

  // Create an arbitrary SFrame with test_num_rows rows.
  drawing_data_generator data_generator(/* is_bitmap_based */ is_bitmap_based,
                                        num_of_rows, class_labels);
  gl_sframe my_data = data_generator.get_data();

  TS_ASSERT_EQUALS(my_data.size(), num_of_rows);

  data_iterator::parameters params;
  params.data = my_data;
  // no repeat since it's not iterating training
  params.repeat = false;
  params.target_column_name = data_generator.get_target_column_name();
  params.feature_column_name = data_generator.get_feature_column_name();

  params.is_train = false;

  return std::unique_ptr<data_iterator>(new simple_data_iterator(params));
}

using test_runner_t =
    std::function<void(test_drawing_classifier&, gl_sframe, gl_sframe)>;


void prediction_test_driver(size_t batch_size, size_t num_of_rows,
                            size_t num_of_classes, test_runner_t runner,
                            bool is_bitmap_based) {
#ifndef NDEBUG
  logprogress_stream << "batch_size=" << batch_size
                     << "; num_of_rows=" << num_of_rows
                     << "; num_of_classes=" << num_of_classes << std::endl;
#endif

  const std::string feature_name = "feature";
  const std::string target_name = "target";

  // mode the data iterator first
  test_drawing_classifier mock_model;
  std::unique_ptr<mock_model_backend> mock_backend(new mock_model_backend);
  std::unique_ptr<mock_compute_context> mock_context(new mock_compute_context);

  auto expected_sf =
      set_up_perform_inference(mock_model, mock_backend, mock_context,
                               batch_size, num_of_rows, num_of_classes);

  // make sure the output is what expected
  flex_list class_labels;
  class_labels.reserve(num_of_classes);
  for (size_t ii = 0; ii < num_of_classes; ++ii) {
    class_labels.push_back(std::to_string(ii));
  }

  // name 'target', 'feature' are used by create_iterator
  drawing_data_generator data_generator(/* is_bitmap_based */ is_bitmap_based,
                                        num_of_rows, class_labels, target_name,
                                        feature_name);

  gl_sframe my_data = data_generator.get_data();
  TS_ASSERT_EQUALS(my_data.size(), num_of_rows);

  mock_model.create_iterator_calls_.push_back(
      [=](data_iterator::parameters my_params) {
        return std::unique_ptr<data_iterator>(
            new simple_data_iterator(my_params));
      });

  // specific for predict
  mock_model.add_or_update_state(
      {{"target", target_name},
       {"feature", feature_name},
       {"classes", class_labels}});

  runner(mock_model, my_data, expected_sf);
}
}  // namespace

/* ========================= perform inference ===========================*/

BOOST_AUTO_TEST_CASE(test_drawing_classifier_perform_inference) {
  log_for_debug("test_drawing_classifier_perform_inference");

  for (auto& entry : TEST_CASES) {
    size_t batch_size = std::get<0>(entry);
    size_t num_of_rows = std::get<1>(entry);
    size_t num_of_classes = std::get<2>(entry);

#ifndef NDEBUG
    logprogress_stream << "batch_size=" << batch_size
                       << "; num_of_rows=" << num_of_rows
                       << "; num_of_classes=" << num_of_classes << std::endl;
#endif

    const std::string feature_name = "feature";
    const std::string target_name = "target";

    // mode the data iterator first
    mock_perform_inference mock_model;
    std::unique_ptr<mock_model_backend> mock_backend(new mock_model_backend);
    std::unique_ptr<mock_compute_context> mock_context(
        new mock_compute_context);

    auto expected_sf =
        set_up_perform_inference(mock_model, mock_backend, mock_context,
                                 batch_size, num_of_rows, num_of_classes);

    // For perform inference,
    // we only want to test with is_bitmap_based set to true.
    auto data_itr = prepare_data_for_prediction(/* is_bitmap_based */ true,
      num_of_rows, num_of_classes);
    auto result = mock_model.get_inference_result(data_itr.get());
    _assert_sframe_equals(result, expected_sf);
    // make sure the output is what expected
  }
}

/* ============================= predict ================================*/

BOOST_AUTO_TEST_CASE(test_drawing_classifier_predict_rank) {
  log_for_debug("test_drawing_classifier_predict_rank");

  for (auto& entry : TEST_CASES) {
    size_t batch_size = std::get<0>(entry);
    size_t num_of_rows = std::get<1>(entry);
    size_t num_of_classes = std::get<2>(entry);

    auto runner = [](test_drawing_classifier& mock_model, gl_sframe my_data,
                     gl_sframe expected) {
      auto result_class = mock_model.predict(my_data, "class");

      TS_ASSERT_EQUALS(result_class.size(), expected.size());

      for (size_t ii = 0; ii < result_class.size(); ++ii) {
        flex_vec probs = expected[PRED_NAME][ii];
        TS_ASSERT_EQUALS(result_class[ii].get_type(), flex_type_enum::STRING);

        size_t label_idx = std::stoi(result_class[ii].to<flex_string>());
        auto expected_idx =
            std::max_element(probs.begin(), probs.end()) - probs.begin();
        TS_ASSERT_EQUALS(label_idx, expected_idx);
      }
    };

    for (bool is_bitmap_based : {true, false}) {
      prediction_test_driver(batch_size, num_of_rows, num_of_classes, runner,
                             is_bitmap_based);
    }
  }
}

BOOST_AUTO_TEST_CASE(test_drawing_classifier_predict_prob_vec) {
  log_for_debug("test_drawing_classifier_predict_prob_vec");

  for (auto& entry : TEST_CASES) {
    size_t batch_size = std::get<0>(entry);
    size_t num_of_rows = std::get<1>(entry);
    size_t num_of_classes = std::get<2>(entry);

    auto runner = [](test_drawing_classifier& mock_model, gl_sframe my_data,
                     gl_sframe expected) {
      auto result_prob = mock_model.predict(my_data, "probability_vector");

      TS_ASSERT_EQUALS(result_prob.size(), expected[PRED_NAME].size());
      _assert_sframe_equals(gl_sframe({{PRED_NAME, result_prob}}), expected);
    };

    for (bool is_bitmap_based : {true, false}) {
      prediction_test_driver(batch_size, num_of_rows, num_of_classes, runner,
                             is_bitmap_based);
    }
  }
}

BOOST_AUTO_TEST_CASE(test_drawing_classifier_predict_prob) {
  log_for_debug("test_drawing_classifier_predict_prob");

  for (auto& entry : TEST_CASES) {
    size_t batch_size = std::get<0>(entry);
    size_t num_of_rows = std::get<1>(entry);
    size_t num_of_classes = std::get<2>(entry);

    auto runner = [&num_of_classes](test_drawing_classifier& mock_model,
                                    gl_sframe my_data, gl_sframe expected) {
      if (num_of_classes > 2) {
        TS_ASSERT_THROWS_ANYTHING(mock_model.predict(my_data, "probability"));

      } else {
        auto result_prob = mock_model.predict(my_data, "probability");

        for (size_t ii = 0; ii < result_prob.size(); ++ii) {
          flex_float expected_prob =
              expected[PRED_NAME][ii][num_of_classes - 1];
          TS_ASSERT_EQUALS(result_prob[ii].get_type(), flex_type_enum::FLOAT);
          TS_ASSERT_EQUALS(expected_prob, result_prob[ii]);
        }
        TS_ASSERT_EQUALS(result_prob.size(), expected[PRED_NAME].size());
      }
    };

    for (bool is_bitmap_based : {true, false}) {
      prediction_test_driver(batch_size, num_of_rows, num_of_classes, runner,
                             is_bitmap_based);
    }
  }
}

/* ========================= predict top k ============================*/

namespace {

void verify_topk_result(size_t kk, size_t num_of_classes, size_t num_of_rows,
                        gl_sframe result, gl_sframe expected) {
  bool test_rank = result.contains_column("rank");

  std::vector<unsigned> idx_vec;
  if (test_rank) idx_vec.resize(num_of_classes);
  flex_vec output(kk);

  for (size_t ii = 0; ii < num_of_rows; ++ii) {
    flex_vec prob_vec = expected[PRED_NAME][ii];

    size_t idx_beg = ii * kk;

    if (test_rank) {
      // reset for each iteration
      std::iota(idx_vec.begin(), idx_vec.end(), 0);

      auto compare = [&](size_t i, size_t j) {
        return prob_vec[i] > prob_vec[j];
      };

      std::sort(idx_vec.begin(), idx_vec.end(), compare);

      // using rank
      for (size_t jj = 0; jj < kk; ++jj) {
        output[jj] = prob_vec[result["rank"][idx_beg + jj]];
      }
    } else {
      // test probability
      for (size_t jj = 0; jj < kk; ++jj) {
        output[jj] = result["probability"][idx_beg + jj];
      }
    }

    auto prob_vec_sorted = prob_vec;
    std::sort(prob_vec_sorted.begin(), prob_vec_sorted.end(),
              std::greater<double>());

    TS_ASSERT(std::equal(prob_vec_sorted.begin(), prob_vec_sorted.begin(),
                         output.begin()));

    // label is stringized sequence [0, num_of_classes)
    auto get_index_of_label = [](const std::string& label) {
      return std::stoi(label);
    };

    for (size_t jj = 0; jj < kk; ++jj) {
      output[jj] = prob_vec[get_index_of_label(
          result["class"][idx_beg + jj].get<flex_string>())];
    }

    TS_ASSERT(std::equal(prob_vec_sorted.begin(), prob_vec_sorted.begin(),
                         output.begin()));
  }
};

}  // namespace

BOOST_AUTO_TEST_CASE(test_drawing_classifier_predict_topk_rank_zero_k) {
  log_for_debug("test_drawing_classifier_predict_topk_rank_zero_k");

  size_t batch_size = 2;
  size_t num_of_rows = 1;
  size_t num_of_classes = 2;

  size_t kk = 0;
  auto runner = [=](test_drawing_classifier& mock_model, gl_sframe my_data,
                    gl_sframe expected) {
    auto result_rank = mock_model.predict_topk(my_data, "rank", kk);

    TS_ASSERT_EQUALS(result_rank.size(), num_of_rows);
    TS_ASSERT_EQUALS(result_rank["rank"].size(), expected[PRED_NAME].size());
    // stack empty list will return undefined value
    TS_ASSERT_EQUALS(result_rank["rank"][0].get_type(),
                     flex_type_enum::UNDEFINED);
  };

  for (bool is_bitmap_based : {true, false}) {
    prediction_test_driver(batch_size, num_of_rows, num_of_classes, runner,
                           is_bitmap_based);
  }
}

BOOST_AUTO_TEST_CASE(test_drawing_classifier_predict_topk_rank_normal_k) {
  log_for_debug("test_drawing_classifier_predict_topk_rank_normal_k");

  for (auto& entry : TEST_CASES) {
    size_t batch_size = std::get<0>(entry);
    size_t num_of_rows = std::get<1>(entry);
    size_t num_of_classes = std::get<2>(entry);

    if (num_of_classes == 0) continue;

    size_t kk = num_of_classes > 1 ? num_of_classes - 1 : 1;

    auto runner = [=](test_drawing_classifier& mock_model, gl_sframe my_data,
                      gl_sframe expected) {
      auto result_rank = mock_model.predict_topk(my_data, "rank", kk);

      TS_ASSERT_EQUALS(result_rank.size(), num_of_rows * kk);
      TS_ASSERT_EQUALS(result_rank["rank"].size() / kk,
                       expected[PRED_NAME].size());
      // label should be string type
      TS_ASSERT_EQUALS(result_rank["rank"][0].get_type(),
                       flex_type_enum::INTEGER);
      TS_ASSERT_EQUALS(result_rank["class"][0].get_type(),
                       flex_type_enum::STRING);

      verify_topk_result(kk, num_of_classes, num_of_rows, result_rank,
                         expected);
    };

    for (bool is_bitmap_based : {true, false}) {
      prediction_test_driver(batch_size, num_of_rows, num_of_classes, runner,
                             is_bitmap_based);
    }
  }
}

BOOST_AUTO_TEST_CASE(test_drawing_classifier_predict_topk_rank_big_k) {
  log_for_debug("test_drawing_classifier_predict_topk_rank_big_k");

  for (auto& entry : TEST_CASES) {
    size_t batch_size = std::get<0>(entry);
    size_t num_of_rows = std::get<1>(entry);
    size_t num_of_classes = std::get<2>(entry);

    if (num_of_classes == 0) continue;

    size_t kk = num_of_classes + 1;

    auto runner = [=](test_drawing_classifier& mock_model, gl_sframe my_data,
                      gl_sframe expected) {
      auto result_rank = mock_model.predict_topk(my_data, "rank", kk);

      size_t realk = num_of_classes;
      TS_ASSERT_EQUALS(result_rank.size(), num_of_rows * realk);
      TS_ASSERT_EQUALS(result_rank["rank"].size() / realk,
                       expected[PRED_NAME].size());
      // label should be string type
      TS_ASSERT_EQUALS(result_rank["rank"][0].get_type(),
                       flex_type_enum::INTEGER);
      TS_ASSERT_EQUALS(result_rank["class"][0].get_type(),
                       flex_type_enum::STRING);

      verify_topk_result(realk, num_of_classes, num_of_rows, result_rank,
                         expected);
    };

    for (bool is_bitmap_based : {true, false}) {
      prediction_test_driver(batch_size, num_of_rows, num_of_classes, runner,
                             is_bitmap_based);
    }
  }
}

BOOST_AUTO_TEST_CASE(test_drawing_classifier_predict_topk_prob_zero_k) {
  log_for_debug("test_drawing_classifier_predict_topk_prob_zero_k");

  size_t batch_size = 2;
  size_t num_of_rows = 1;
  size_t num_of_classes = 2;

  size_t kk = 0;
  auto runner = [=](test_drawing_classifier& mock_model, gl_sframe my_data,
                    gl_sframe expected) {
    auto result_rank = mock_model.predict_topk(my_data, "probability", kk);

    TS_ASSERT_EQUALS(result_rank.size(), num_of_rows);
    TS_ASSERT_EQUALS(result_rank["probability"].size(),
                     expected[PRED_NAME].size());
    // stack empty list will return undefined value
    TS_ASSERT_EQUALS(result_rank["probability"][0].get_type(),
                     flex_type_enum::UNDEFINED);
  };

  for (bool is_bitmap_based : {true, false}) {
    prediction_test_driver(batch_size, num_of_rows, num_of_classes, runner,
                           is_bitmap_based);
  }
}

BOOST_AUTO_TEST_CASE(test_drawing_classifier_predict_topk_prob_normal_k) {
  log_for_debug("test_drawing_classifier_predict_topk_prob_normal_k");

  for (auto& entry : TEST_CASES) {
    size_t batch_size = std::get<0>(entry);
    size_t num_of_rows = std::get<1>(entry);
    size_t num_of_classes = std::get<2>(entry);

    if (num_of_classes == 0) continue;

    size_t kk = num_of_classes > 1 ? num_of_classes - 1 : 1;

    auto runner = [=](test_drawing_classifier& mock_model, gl_sframe my_data,
                      gl_sframe expected) {
      auto result_rank = mock_model.predict_topk(my_data, "probability", kk);

      TS_ASSERT_EQUALS(result_rank.size(), num_of_rows * kk);
      TS_ASSERT_EQUALS(result_rank["probability"].size() / kk,
                       expected[PRED_NAME].size());
      // label should be string type
      TS_ASSERT_EQUALS(result_rank["probability"][0].get_type(),
                       flex_type_enum::FLOAT);
      TS_ASSERT_EQUALS(result_rank["class"][0].get_type(),
                       flex_type_enum::STRING);

      verify_topk_result(kk, num_of_classes, num_of_rows, result_rank,
                         expected);
    };

    for (bool is_bitmap_based : {true, false}) {
      prediction_test_driver(batch_size, num_of_rows, num_of_classes, runner,
                             is_bitmap_based);
    }
  }
}

BOOST_AUTO_TEST_CASE(test_drawing_classifier_predict_topk_prob_big_k) {
  log_for_debug("test_drawing_classifier_predict_topk_prob_big_k");

  for (auto& entry : TEST_CASES) {
    size_t batch_size = std::get<0>(entry);
    size_t num_of_rows = std::get<1>(entry);
    size_t num_of_classes = std::get<2>(entry);

    if (num_of_classes == 0) continue;

    size_t kk = num_of_classes + 1;

    auto runner = [=](test_drawing_classifier& mock_model, gl_sframe my_data,
                      gl_sframe expected) {
      auto result_rank = mock_model.predict_topk(my_data, "probability", kk);

      size_t realk = num_of_classes;
      TS_ASSERT_EQUALS(result_rank.size(), num_of_rows * realk);
      TS_ASSERT_EQUALS(result_rank["probability"].size() / realk,
                       expected[PRED_NAME].size());
      // label should be string type
      TS_ASSERT_EQUALS(result_rank["probability"][0].get_type(),
                       flex_type_enum::FLOAT);
      TS_ASSERT_EQUALS(result_rank["class"][0].get_type(),
                       flex_type_enum::STRING);

      verify_topk_result(realk, num_of_classes, num_of_rows, result_rank,
                         expected);
    };

    for (bool is_bitmap_based : {true, false}) {
      prediction_test_driver(batch_size, num_of_rows, num_of_classes, runner,
                             is_bitmap_based);
    }
  }
}

}  // namespace drawing_classifier
}  // namespace turi
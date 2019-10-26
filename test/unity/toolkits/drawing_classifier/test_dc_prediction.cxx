/* Copyright © 2018 Apple Inc. All rights reserved.
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

struct mock_perform_inference : public test_drawing_classifier {
  inline gl_sframe get_inference_result(data_iterator* data) {
    return perform_inference(data);
  }
};

// borrowed from gl_sframe.cxx
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

}  // namespace

BOOST_AUTO_TEST_CASE(test_drawing_classifier_perform_inference) {
  constexpr size_t batch_size = 2;
  constexpr size_t num_of_rows = 6;
  const std::vector<std::string> class_labels = {"0", "1"};
  const size_t num_classes = class_labels.size();

  std::unique_ptr<mock_model_backend> mock_backend(new mock_model_backend);

  gl_sframe expected_sf;
  {
    std::vector<float> buffer(num_classes * batch_size);

    std::default_random_engine eng;
    std::uniform_int_distribution<float> distribution(0, 20);

    gl_sframe_writer writer({"preds"}, {flex_type_enum::VECTOR},
                            /* num_segments */ 1);

    size_t ii = 0;
    while (ii < num_of_rows) {
      buffer.clear();

      size_t repeat =
          ii + batch_size > num_of_rows ? num_of_rows - ii : batch_size;

      ii += batch_size;

      for (size_t jj = 0; jj < repeat; ++jj) {
        auto pbeg = buffer.begin() + jj * num_classes;
        auto pend = pbeg + num_classes;
        std::generate(pbeg, pend, [&]() { return distribution(eng); });
        float my_sum = std::accumulate(pbeg, pend, 0.0f, std::plus<float>());
        std::transform(pbeg, pend, pbeg,
                       [=](float num) { return num / my_sum; });

        writer.write({flex_vec{pbeg, pend}}, 0);
      }

      auto to_return =
          shared_float_array::copy(buffer.data(), {num_classes, repeat});

      mock_backend->predict_calls_.push_back([=](const float_array_map& input) {
        TS_ASSERT_EQUALS(input.at("input").size(), repeat * 28 * 28);
        return float_array_map({{"output", to_return}});
      });
    }

    // since we are using push front;
    expected_sf = writer.close();
  }

  TS_ASSERT_EQUALS(expected_sf.size(), num_of_rows);
  std::cout << mock_backend->predict_calls_.size() << std::endl;
  TS_ASSERT_EQUALS(mock_backend->predict_calls_.size(),
                   num_of_rows % batch_size ? num_of_rows / batch_size + 1
                                            : num_of_rows / batch_size);

  // Create an arbitrary SFrame with test_num_rows rows.
  drawing_data_generator data_generator(num_of_rows, class_labels);
  gl_sframe my_data = data_generator.get_data();

  TS_ASSERT_EQUALS(my_data.size(), num_of_rows);

  data_iterator::parameters params;
  params.data = my_data;
  // this is buggy
  params.repeat = false;
  params.target_column_name = data_generator.get_target_column_name();
  params.feature_column_name = data_generator.get_feature_column_name();

  params.is_train = false;
  auto data_itr =
      std::unique_ptr<data_iterator>(new simple_data_iterator(params));

  // mode the data iterator first
  mock_perform_inference mock_model;

  mock_model.add_or_update_state(
      {{"num_classes", num_classes}, {"batch_size", batch_size}});

  std::unique_ptr<mock_compute_context> mock_context(new mock_compute_context);

  auto create_drawing_classifier_impl =
      [&mock_backend](const float_array_map&, size_t batch_size,
                      size_t num_classes) { return std::move(mock_backend); };

  mock_context->create_drawing_classifier_calls_.push_back(
      create_drawing_classifier_impl);

  // be extremely careful about reference a unique_ptr
  // never use mock_context after this line
  auto create_compute_context_impl = [&] { return std::move(mock_context); };
  mock_model.create_compute_context_calls_.push_back(create_compute_context_impl);

  // make sure the output is what expected
  auto result = mock_model.get_inference_result(data_itr.get());

  _assert_sframe_equals(result, expected_sf);
}

BOOST_AUTO_TEST_CASE(test_drawing_classifier_predict) {}

BOOST_AUTO_TEST_CASE(test_drawing_classifier_predict_topk) {}

}  // namespace drawing_classifier
}  // namespace turi

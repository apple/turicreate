/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_object_detector

#include <unity/toolkits/object_detection/object_detector.hpp>

#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>

namespace turi {
namespace object_detection {

namespace {

using turi::neural_net::cnn_module;
using turi::neural_net::deferred_float_array;
using turi::neural_net::float_array;
using turi::neural_net::float_array_map;
using turi::neural_net::shared_float_array;

// Dependencies we can inject into the object_detector class for testing.
using coreml_importer =
    std::function<turi::neural_net::float_array_map(const std::string&)>;
using module_factory =
    std::function<std::unique_ptr<turi::neural_net::cnn_module>(
        int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
        const turi::neural_net::float_array_map& config,
        const turi::neural_net::float_array_map& weights)>;

constexpr float TEST_PARAM_VALUE = 3.14159f;

// Create a float_array_map for the mock coreml_importer to return.
float_array_map create_test_params() {
  float_array_map result;
  result["test_param"] = shared_float_array::wrap(TEST_PARAM_VALUE);
  return result;
}

// Create a deferred_float_array for the stub cnn_module to return.
deferred_float_array wrap_float_array(shared_float_array params) {
  std::vector<size_t> shape(params.shape(), params.shape() + params.dim());

  std::promise<shared_float_array> promise;
  promise.set_value(std::move(params));
  return deferred_float_array(promise.get_future(), std::move(shape));
}

// A trivial cnn_module implementation for the mock module_factory to return.
// TODO: Move this somewhere shared.
// TODO: Adopt a real mocking library.
class stub_cnn_module: public cnn_module {
public:
  void set_learning_rate(float lr) override {}
  deferred_float_array train(const float_array& input_batch,
                             const float_array& label_batch) override {
    return wrap_float_array(shared_float_array());
  }
  deferred_float_array predict(const float_array& input_batch) const override {
    return wrap_float_array(shared_float_array());
  }
  float_array_map export_weights() const override {
    return create_test_params();
  }
};

// Test implementation of the object_detector that differs from the production
// implementation only in that we can inject the dependencies.
class test_object_detector: public object_detector {
public:
  test_object_detector(coreml_importer coreml_importer_fn,
                       module_factory module_factory_fn)
    : object_detector(std::move(coreml_importer_fn),
                      std::move(module_factory_fn))
  {}
};

}  // namespace

BOOST_AUTO_TEST_CASE(test_object_detector_train) {
  const std::string test_path = "/test/foo.mlmodel";

  // Create a mock coreml_importer that just returns some dummy parameters.
  int num_coreml_importer_calls = 0;
  coreml_importer coreml_importer_fn = [&](const std::string& path) {
    ++num_coreml_importer_calls;

    // Verify that we receive the expected model_params_path.
    TS_ASSERT_EQUALS(test_path, path);

    return create_test_params();
  };

  // Create a mock module_factory that just returns a stub cnn_module.
  int num_module_factory_calls = 0;
  stub_cnn_module* stub_module = new stub_cnn_module;
  std::unique_ptr<cnn_module> unique_stub_module(stub_module);
  module_factory module_factory_fn = [&](
      int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
      const float_array_map& config, const float_array_map& weights) {
    ++num_module_factory_calls;

    // Verify that the weights we received was the output of the mock
    // coreml_importer above.
    TS_ASSERT_EQUALS(weights.size(), 1);
    auto it = weights.find("test_param");
    TS_ASSERT(it != weights.end());
    const auto& test_param_value = it->second;
    TS_ASSERT_EQUALS(test_param_value.size(), 1);
    TS_ASSERT_EQUALS(test_param_value.data()[0], TEST_PARAM_VALUE);

    TS_ASSERT(unique_stub_module != nullptr);
    return std::move(unique_stub_module);
  };

  // Inject the two mocks into a test_object_detector instance.
  test_object_detector od_instance(coreml_importer_fn, module_factory_fn);

  // Invoke training.
  // TODO: Use some non-empty training data.
  od_instance.train(gl_sframe(), std::string(), std::string(),
                    { { "model_params_path", test_path } });

  // Verify that each mock dependency was invoked once.
  TS_ASSERT_EQUALS(num_coreml_importer_calls, 1);
  TS_ASSERT_EQUALS(num_module_factory_calls, 1);
}

}  // namespace object_detection
}  // namespace turi

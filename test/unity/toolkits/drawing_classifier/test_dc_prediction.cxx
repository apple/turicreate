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

namespace turi {
namespace drawing_classifier {

namespace {

using turi::neural_net::compute_context;
using turi::neural_net::float_array_map;
using turi::neural_net::mock_compute_context;
using turi::neural_net::mock_model_backend;
using turi::neural_net::model_backend;
using turi::neural_net::model_spec;
using turi::neural_net::shared_float_array;


BOOST_AUTO_TEST_CASE(test_drawing_classifier_init_training) {}

}  // namespace
}  // namespace drawing_classifier
}  // namespace turi

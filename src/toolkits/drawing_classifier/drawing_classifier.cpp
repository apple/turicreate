/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/drawing_classifier/drawing_classifier.hpp>

#include <algorithm>
#include <functional>
#include <numeric>
#include <random>

#include <core/logging/assertions.hpp>
#include <core/logging/logger.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/coreml_export/neural_net_models_exporter.hpp>
#include <toolkits/evaluation/metrics.hpp>
#include <core/util/string_util.hpp>


namespace turi {
namespace drawing_classifier {

namespace {

using coreml::MLModelWrapper;
using neural_net::compute_context;
using neural_net::float_array_map;
using neural_net::model_backend;
using neural_net::model_spec;
using neural_net::shared_float_array;
using neural_net::xavier_weight_initializer;
using neural_net::zero_weight_initializer;

using padding_type = model_spec::padding_type;

}
}  // namespace drawing_classifier
}  // namespace turi
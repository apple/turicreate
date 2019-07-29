/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_od_serialization

#include <toolkits/object_detection/od_serialization.hpp>
#include <cstdio>

//maybe
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <toolkits/coreml_export/mlmodel_include.hpp>
#include <ml/neural_net/model_spec.hpp>

namespace turi {
namespace object_detection {
namespace {

BOOST_AUTO_TEST_CASE(test_init_darknet_yolo) {

	const std::vector<std::pair<float, float>> anchor_boxes = {
      {1.f, 2.f}, {1.f, 1.f}, {2.f, 1.f},
      {2.f, 4.f}, {2.f, 2.f}, {4.f, 2.f},
      {4.f, 8.f}, {4.f, 4.f}, {8.f, 4.f},
      {8.f, 16.f}, {8.f, 8.f}, {16.f, 8.f},
      {16.f, 32.f}, {16.f, 16.f}, {32.f, 16.f},
	};

	neural_net::model_spec nn_spec;
	const size_t num_classes = 10;
	init_darknet_yolo(nn_spec, num_classes, anchor_boxes);

	const CoreML::Specification::NeuralNetwork& nn = nn_spec.get_coreml_spec();
	TS_ASSERT_EQUALS(nn.layers_size(), 25);

	int layer_num = 0;
	int num_features = 3;
	const std::map<int, int> layer_num_to_channels = {{0, 16}, {1, 32}, {2,64}, {3, 128}, {4, 256}, {5,512}, {6, 1024}, {7, 1024}};
	for (auto const& x : layer_num_to_channels)
	{

		const auto& convlayer_ = nn.layers(layer_num);
		TS_ASSERT(convlayer_.has_convolution());
		TS_ASSERT_EQUALS(convlayer_.name(), "conv"+std::to_string(x.first)+"_fwd");
		TS_ASSERT_EQUALS(convlayer_.convolution().outputchannels(), x.second);
		TS_ASSERT_EQUALS(convlayer_.convolution().kernelchannels(), num_features);
		TS_ASSERT_EQUALS(convlayer_.convolution().stride(0), 1);
		TS_ASSERT_EQUALS(convlayer_.convolution().stride(1), 1);
		TS_ASSERT_EQUALS(convlayer_.convolution().kernelsize(0), 3);
		TS_ASSERT_EQUALS(convlayer_.convolution().kernelsize(1), 3);

		const auto& batchnormlayer_ = nn.layers(layer_num+1);
		TS_ASSERT(batchnormlayer_.has_batchnorm());
		TS_ASSERT_EQUALS(batchnormlayer_.name(), "batchnorm"+std::to_string(x.first)+"_fwd");
		TS_ASSERT_EQUALS(batchnormlayer_.batchnorm().channels(), x.second);
		TS_ASSERT_EQUALS(batchnormlayer_.batchnorm().epsilon(), 0.00001f);

		const auto& relulayer_ = nn.layers(layer_num+2);
		TS_ASSERT(relulayer_.has_activation());
		TS_ASSERT_EQUALS(relulayer_.name(), "leakyrelu"+std::to_string(x.first)+"_fwd");
		TS_ASSERT_EQUALS(relulayer_.activation().leakyrelu().alpha(), 0.1f);

		layer_num = layer_num+3;
		num_features = x.second;

	}

}


}  // namespace
}  // namespace object_detection
}  // namespace turi

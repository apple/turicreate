# Copyright (c) 2018, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import coremltools
import unittest
import tempfile
from coremltools.proto import Model_pb2
from coremltools.models.utils import rename_feature, save_spec, macos_version
from coremltools.models import MLModel
from coremltools.models import NeuralNetworkShaper
from coremltools.models.neural_network.flexible_shape_utils import can_allow_multiple_input_shapes, get_allowed_shape_ranges, update_multiarray_shape_range, _get_input_names, NeuralNetworkMultiArrayShapeRange, _CONSTRAINED_KEYS
import numpy as np

class ShaperTest(unittest.TestCase):

    @classmethod
    def setUpClass(self):

        width = 100
        height = width
        numLayers = 10

        channels = 1
        modelName = 'planSubmitTestColorArray'

        self.spec = coremltools.proto.Model_pb2.Model()
        self.spec.specificationVersion = 1

        input_ = self.spec.description.input.add()
        input_.name = 'input'
        input_.type.multiArrayType.MergeFromString('')
        input_.type.multiArrayType.shape.append(channels)
        input_.type.multiArrayType.shape.append(height)
        input_.type.multiArrayType.shape.append(width)
        input_.type.multiArrayType.dataType = coremltools.proto.Model_pb2.ArrayFeatureType.DOUBLE

        output_ = self.spec.description.output.add()
        output_.name = 'output'
        output_.type.multiArrayType.MergeFromString('')
        output_.type.multiArrayType.dataType = coremltools.proto.Model_pb2.ArrayFeatureType.DOUBLE

        # big stack of identity convolutions
        for i in range(0, numLayers):

            layer = self.spec.neuralNetwork.layers.add()
            if (i == 0):
                layer.input.append('input')
            else:
                layer.input.append('conv' + str(i - 1))

            if (i == numLayers - 1):
                #######
                # Just convolutions

                layer.name = 'last_layer'
                layer.output.append('output')
                layer.name = 'conv' + str(i)
                layer.convolution.outputChannels = channels
                layer.convolution.kernelChannels = channels
                layer.convolution.kernelSize.append(1)
                layer.convolution.kernelSize.append(1)
                layer.convolution.same.MergeFromString('')
                layer.convolution.hasBias = False
                for i in range(0, channels):
                    for j in range(0, channels):
                        if i == j:
                            layer.convolution.weights.floatValue.append(1.0)
                        else:
                            layer.convolution.weights.floatValue.append(0.0)


            else:  # not the last layer
                layer.output.append('conv' + str(i))
                layer.name = 'conv' + str(i)

                layer.convolution.outputChannels = channels
                layer.convolution.kernelChannels = channels

                layer.convolution.kernelSize.append(1)
                layer.convolution.kernelSize.append(1)

                layer.convolution.same.MergeFromString('')

                layer.convolution.hasBias = False

                for i in range(0, channels):
                    for j in range(0, channels):
                        if (i == j):
                            layer.convolution.weights.floatValue.append(1.0)
                        else:
                            layer.convolution.weights.floatValue.append(0.0)

        self.coremlModel = coremltools.models.MLModel(self.spec)

    def test_model_creation_spec(self):

        shaper = NeuralNetworkShaper(self.spec)
        self.assertIsNotNone(shaper)

    def test_model_creation_file(self):

        filename = tempfile.mktemp(suffix = '.mlmodel')
        self.coremlModel.save(filename)

        shaper = NeuralNetworkShaper(filename)
        self.assertIsNotNone(shaper)


    def test_get_shape(self):

        shaper = NeuralNetworkShaper(self.spec)

        input_shape = shaper.shape('input')
        self.assertIsNotNone(input_shape)

        random_shape = shaper.shape('conv5')
        self.assertIsNotNone(random_shape)

        with self.assertRaises(Exception):
            missing_shape = shaper.shape('idontexist')

    def test_is_flexible(self):

        self.assertTrue(can_allow_multiple_input_shapes(self.spec))

    def test_get_ranges(self):

        ranges = get_allowed_shape_ranges(self.spec)
        input_names = _get_input_names(self.spec)
        spec_copy = self.spec
        for input_name in input_names:
            whole_range = ranges[input_name]
            constraint_range = {key: whole_range[key] for key in _CONSTRAINED_KEYS}
            shape_range = NeuralNetworkMultiArrayShapeRange(constraint_range)
            update_multiarray_shape_range(spec_copy, input_name, shape_range)

        self.assertTrue(spec_copy.description.input[0].type.multiArrayType.HasField('shapeRange'))

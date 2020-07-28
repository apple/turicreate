import unittest

import numpy as np
import tempfile
import pytest
import shutil
import os

import coremltools
from coremltools._deps import _HAS_KERAS_TF, MSG_KERAS1_NOT_FOUND
from coremltools._deps import _HAS_TF, MSG_TF1_NOT_FOUND
from coremltools.models.utils import (
    _get_custom_layer_names,
    _replace_custom_layer_name,
    _macos_version,
    _is_macos,
)
from coremltools.proto import Model_pb2

if _HAS_KERAS_TF:
    from keras.models import Sequential
    from keras.layers import Dense, LSTM
    from coremltools.converters import keras as keras_converter

if _HAS_TF:
    import tensorflow as tf
    from tensorflow.python.platform import gfile
    from tensorflow.python.tools import freeze_graph

    tf.compat.v1.disable_eager_execution()


@unittest.skipIf(not _HAS_KERAS_TF, MSG_KERAS1_NOT_FOUND)
@pytest.mark.keras1
class KerasBasicNumericCorrectnessTest(unittest.TestCase):
    def test_classifier(self):
        np.random.seed(1988)

        print("running test classifier")

        input_dim = 5
        num_hidden = 12
        num_classes = 6
        input_length = 3

        model = Sequential()
        model.add(
            LSTM(
                num_hidden,
                input_dim=input_dim,
                input_length=input_length,
                return_sequences=False,
            )
        )
        model.add(Dense(num_classes, activation="softmax"))

        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        input_names = ["input"]
        output_names = ["zzzz"]
        class_labels = ["a", "b", "c", "d", "e", "f"]
        predicted_feature_name = "pf"
        coremlmodel = keras_converter.convert(
            model,
            input_names,
            output_names,
            class_labels=class_labels,
            predicted_feature_name=predicted_feature_name,
            predicted_probabilities_output=output_names[0],
        )

        if _is_macos() and _macos_version() >= (10, 13):
            inputs = np.random.rand(input_dim)
            outputs = coremlmodel.predict({"input": inputs})
            # this checks that the dictionary got the right name and type
            self.assertEquals(type(outputs[output_names[0]]), type({"a": 0.5}))

    def test_classifier_no_name(self):
        np.random.seed(1988)

        input_dim = 5
        num_hidden = 12
        num_classes = 6
        input_length = 3

        model = Sequential()
        model.add(
            LSTM(
                num_hidden,
                input_dim=input_dim,
                input_length=input_length,
                return_sequences=False,
            )
        )
        model.add(Dense(num_classes, activation="softmax"))

        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        input_names = ["input"]
        output_names = ["zzzz"]
        class_labels = ["a", "b", "c", "d", "e", "f"]
        predicted_feature_name = "pf"
        coremlmodel = keras_converter.convert(
            model,
            input_names,
            output_names,
            class_labels=class_labels,
            predicted_feature_name=predicted_feature_name,
        )

        if _is_macos() and _macos_version() >= (10, 13):
            inputs = np.random.rand(input_dim)
            outputs = coremlmodel.predict({"input": inputs})
            # this checks that the dictionary got the right name and type
            self.assertEquals(type(outputs[output_names[0]]), type({"a": 0.5}))

    def test_internal_layer(self):

        np.random.seed(1988)

        input_dim = 5
        num_channels1 = 10
        num_channels2 = 7
        num_channels3 = 5

        w1 = (np.random.rand(input_dim, num_channels1) - 0.5) / 5.0
        w2 = (np.random.rand(num_channels1, num_channels2) - 0.5) / 5.0
        w3 = (np.random.rand(num_channels2, num_channels3) - 0.5) / 5.0

        b1 = (np.random.rand(num_channels1,) - 0.5) / 5.0
        b2 = (np.random.rand(num_channels2,) - 0.5) / 5.0
        b3 = (np.random.rand(num_channels3,) - 0.5) / 5.0

        model = Sequential()
        model.add(Dense(num_channels1, input_dim=input_dim))
        model.add(Dense(num_channels2, name="middle_layer"))
        model.add(Dense(num_channels3))

        model.set_weights([w1, b1, w2, b2, w3, b3])

        input_names = ["input"]
        output_names = ["output"]
        coreml1 = keras_converter.convert(model, input_names, output_names)

        # adjust the output parameters of coreml1 to include the intermediate layer
        spec = coreml1.get_spec()
        coremlNewOutputs = spec.description.output.add()
        coremlNewOutputs.name = "middle_layer_output"
        coremlNewParams = coremlNewOutputs.type.multiArrayType
        coremlNewParams.dataType = coremltools.proto.FeatureTypes_pb2.ArrayFeatureType.ArrayDataType.Value(
            "DOUBLE"
        )
        coremlNewParams.shape.extend([num_channels2])

        coremlfinal = coremltools.models.MLModel(spec)

        # generate a second model which
        model2 = Sequential()
        model2.add(Dense(num_channels1, input_dim=input_dim))
        model2.add(Dense(num_channels2))
        model2.set_weights([w1, b1, w2, b2])

        coreml2 = keras_converter.convert(model2, input_names, ["output2"])

        if _is_macos() and _macos_version() >= (10, 13):
            # generate input data
            inputs = np.random.rand(input_dim)

            fullOutputs = coremlfinal.predict({"input": inputs})

            partialOutput = coreml2.predict({"input": inputs})

            for i in range(0, num_channels2):
                self.assertAlmostEquals(
                    fullOutputs["middle_layer_output"][i],
                    partialOutput["output2"][i],
                    2,
                )


class CustomLayerUtilsTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        spec = Model_pb2.Model()
        spec.specificationVersion = coremltools.SPECIFICATION_VERSION

        features = ["feature_1", "feature_2"]
        output = "output"
        for f in features:
            input_ = spec.description.input.add()
            input_.name = f
            input_.type.doubleType.MergeFromString(b"")

        output_ = spec.description.output.add()
        output_.name = output
        output_.type.doubleType.MergeFromString(b"")

        layer = spec.neuralNetwork.layers.add()
        layer.name = "custom1"
        layer.input.append("input")
        layer.output.append("temp1")
        layer.custom.className = "name1"

        layer2 = spec.neuralNetwork.layers.add()
        layer2.name = "custom2"
        layer2.input.append("temp1")
        layer2.output.append("temp2")
        layer2.custom.className = "name2"

        layer3 = spec.neuralNetwork.layers.add()
        layer3.name = "custom3"
        layer3.input.append("temp2")
        layer3.output.append("output")
        layer3.custom.className = "name1"

        self.spec = spec

    def test_get_custom_names(self):
        names = _get_custom_layer_names(self.spec)
        self.assertEqual(names, {"name1", "name2"})

    def test_change_custom_name(self):
        _replace_custom_layer_name(self.spec, "name1", "notname1")
        names = _get_custom_layer_names(self.spec)
        self.assertEqual(names, {"notname1", "name2"})
        # set it back for future tests
        _replace_custom_layer_name(self.spec, "notname1", "name1")

# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import coremltools
from coremltools._deps import (
    _HAS_KERAS2_TF,
    MSG_KERAS2_NOT_FOUND,
    _HAS_SKLEARN,
    MSG_SKLEARN_NOT_FOUND,
)
from coremltools.models.utils import _macos_version, _is_macos

if _HAS_KERAS2_TF:
    import keras
if _HAS_SKLEARN:
    import sklearn
    from sklearn.tree import DecisionTreeClassifier, DecisionTreeRegressor
    from sklearn.ensemble import RandomForestRegressor, RandomForestClassifier
    from sklearn.linear_model import LinearRegression
    from sklearn.svm import SVC, SVR
    from sklearn.datasets import load_boston
import unittest
import numpy as np
import pytest
import PIL.Image


def create_model(spec):
    """
    Create MLModel with specified types
    Parameters
    ----------
    spec: Pb spec from 3rd party converted model

    Returns
    -------
    MLModel
    """
    return coremltools.models.MLModel(spec)


@unittest.skipUnless(
    _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
)
@unittest.skipIf(not _HAS_SKLEARN, MSG_SKLEARN_NOT_FOUND)
class TestIODataTypes(unittest.TestCase):
    """
    This class tests for different I/O feature data types for an .mlmodel
    It will cover the following areas to test for:
    - All features must have a valid type
    - Multiarrays must have a valid dataType. Inputs must specify shape. Shape must have >= 0 elements
    - Images must have a valid colorspace. width & height have to be >= 0
    - Dictionaries must have a valid key type
    """

    @property
    def scikit_data(self):
        return load_boston()

    def _feature_data_type(self, dtype):
        feature_dict = {np.int32: "INT32", np.float32: "FLOAT32", np.float64: "DOUBLE"}
        return feature_dict[dtype]

    @property
    def number_data_type(self):
        return dict(
            int8=np.int8,
            int16=np.int16,
            int32=np.int32,
            uint8=np.uint8,
            uint16=np.uint16,
            uint32=np.uint32,
            float=np.float32,
            double=np.double,
        )

    def _sklearn_setup(self, model, dtype, data, target):
        model.fit(data, target)
        spec = coremltools.converters.sklearn.convert(
            model, "data", "target"
        ).get_spec()
        return model, spec

    def _check_tree_model(self, spec, inputType, outputType, n_out):
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertIsNotNone(spec.treeEnsembleClassifier)

        # Test the interface class
        self.assertEqual(spec.description.predictedFeatureName, "target")

        # Test the inputs and outputs
        self.assertEqual(len(spec.description.output), n_out)
        self.assertEqual(spec.description.output[0].name, "target")
        self.assertEqual(spec.description.output[0].type.WhichOneof("Type"), outputType)
        self.assertEqual(spec.description.input[0].name, "data")
        self.assertEqual(spec.description.input[0].type.WhichOneof("Type"), inputType)

    def test_tree_regressor(self):
        for dtype in self.number_data_type.keys():
            scikit_model = DecisionTreeRegressor(random_state=1)
            data = self.scikit_data["data"].astype(dtype)
            target = self.scikit_data["target"].astype(dtype)
            scikit_model, spec = self._sklearn_setup(scikit_model, dtype, data, target)
            test_data = data[0].reshape(1, -1)
            self._check_tree_model(spec, "multiArrayType", "doubleType", 1)
            coreml_model = create_model(spec)
            try:
                self.assertEqual(
                    scikit_model.predict(test_data)[0].dtype,
                    type(coreml_model.predict({"data": test_data})["target"]),
                )
                self.assertEqual(
                    scikit_model.predict(test_data)[0],
                    coreml_model.predict({"data": test_data})["target"],
                    msg="{} != {} for Dtype: {}".format(
                        scikit_model.predict(test_data)[0],
                        coreml_model.predict({"data": test_data})["target"],
                        dtype,
                    ),
                )
            except RuntimeError:
                print("{} not supported. ".format(dtype))

    def test_random_forest_classifier(self):
        for dtype in self.number_data_type.keys():
            scikit_model = RandomForestClassifier(random_state=1)
            data = self.scikit_data["data"].astype(dtype)
            target = (
                self.scikit_data["target"].astype(dtype)
                > self.scikit_data["target"].astype(dtype).mean()
            )
            scikit_model, spec = self._sklearn_setup(scikit_model, dtype, data, target)
            test_data = data[0].reshape(1, -1)
            self._check_tree_model(spec, "multiArrayType", "int64Type", 2)
            coreml_model = create_model(spec)
            try:
                self.assertEqual(
                    scikit_model.predict(test_data)[0],
                    bool(int(coreml_model.predict({"data": test_data})["target"])),
                    msg="{} != {} for Dtype: {}".format(
                        scikit_model.predict(test_data)[0],
                        bool(int(coreml_model.predict({"data": test_data})["target"])),
                        dtype,
                    ),
                )
            except RuntimeError:
                print("{} not supported. ".format(dtype))

    def test_random_forest_regressor(self):
        for dtype in self.number_data_type.keys():
            scikit_model = RandomForestRegressor(random_state=1)
            data = self.scikit_data["data"].astype(dtype)
            target = self.scikit_data["target"].astype(dtype)
            scikit_model, spec = self._sklearn_setup(scikit_model, dtype, data, target)
            test_data = data[0].reshape(1, -1)
            self._check_tree_model(spec, "multiArrayType", "doubleType", 1)
            coreml_model = create_model(spec)
            try:
                self.assertEqual(
                    scikit_model.predict(test_data)[0].dtype,
                    type(coreml_model.predict({"data": test_data})["target"]),
                )
                self.assertAlmostEqual(
                    scikit_model.predict(test_data)[0],
                    coreml_model.predict({"data": test_data})["target"],
                    msg="{} != {} for Dtype: {}".format(
                        scikit_model.predict(test_data)[0],
                        coreml_model.predict({"data": test_data})["target"],
                        dtype,
                    ),
                )
            except RuntimeError:
                print("{} not supported. ".format(dtype))

    def test_support_vector_classifier(self):
        for dtype in self.number_data_type.keys():
            scikit_model = SVC(kernel="rbf", gamma=1.2, C=1)
            data = self.scikit_data["data"].astype(dtype)
            target = (
                self.scikit_data["target"].astype(dtype)
                > self.scikit_data["target"].astype(dtype).mean()
            )
            scikit_model, spec = self._sklearn_setup(scikit_model, dtype, data, target)
            coreml_model = create_model(spec)
            for idx in range(0, 10):
                test_data = data[idx].reshape(1, -1)
                try:
                    self.assertEqual(
                        scikit_model.predict(test_data)[0],
                        bool(int(coreml_model.predict({"data": test_data})["target"])),
                        msg="{} != {} for Dtype: {}".format(
                            scikit_model.predict(test_data)[0],
                            bool(
                                int(coreml_model.predict({"data": test_data})["target"])
                            ),
                            dtype,
                        ),
                    )
                except RuntimeError:
                    print("{} not supported. ".format(dtype))

    def test_support_vector_regressor(self):
        for dtype in self.number_data_type.keys():
            scikit_model = SVR(kernel="rbf")
            data = self.scikit_data["data"].astype(dtype)
            target = self.scikit_data["target"].astype(dtype)
            scikit_model, spec = self._sklearn_setup(scikit_model, dtype, data, target)
            test_data = data[0].reshape(1, -1)
            coreml_model = create_model(spec)
            try:
                self.assertEqual(
                    scikit_model.predict(test_data)[0],
                    coreml_model.predict({"data": test_data})["target"],
                    msg="{} != {} for Dtype: {}".format(
                        scikit_model.predict(test_data)[0],
                        coreml_model.predict({"data": test_data})["target"],
                        dtype,
                    ),
                )
            except RuntimeError:
                print("{} not supported. ".format(dtype))

    def test_linear_regressor(self):
        for dtype in self.number_data_type.keys():
            scikit_model = LinearRegression(normalize=True)
            data = self.scikit_data["data"].astype(dtype)
            target = self.scikit_data["target"].astype(dtype)
            scikit_model, spec = self._sklearn_setup(scikit_model, dtype, data, target)
            test_data = data[0].reshape(1, -1)
            coreml_model = create_model(spec)
            try:
                self.assertEqual(
                    scikit_model.predict(test_data)[0].dtype,
                    type(coreml_model.predict({"data": test_data})["target"]),
                )
                self.assertAlmostEqual(
                    scikit_model.predict(test_data)[0],
                    coreml_model.predict({"data": test_data})["target"],
                    msg="{} != {} for Dtype: {}".format(
                        scikit_model.predict(test_data)[0],
                        coreml_model.predict({"data": test_data})["target"],
                        dtype,
                    ),
                )
            except RuntimeError:
                print("{} not supported. ".format(dtype))

    @unittest.skipIf(not _HAS_KERAS2_TF, MSG_KERAS2_NOT_FOUND)
    @pytest.mark.keras2
    def test_keras_dense_model(self):
        model = keras.models.Sequential()
        model.add(
            keras.layers.Dense(
                3,
                activation="sigmoid",
                kernel_initializer="random_uniform",
                bias_initializer="random_uniform",
                input_dim=3,
            )
        )
        for key, dtype in self.number_data_type.items():
            try:
                input_data = np.random.rand(1, 3).astype(key)
                keras_out = model.predict(input_data)
                coreml_model = coremltools.converters.keras.convert(
                    model, input_names=["data"], output_names=["target"]
                )
                spec = coreml_model.get_spec()
                spec.description.output[
                    0
                ].type.multiArrayType.dataType = coremltools.proto.FeatureTypes_pb2.ArrayFeatureType.ArrayDataType.Value(
                    self._feature_data_type(dtype)
                )
                spec.description.input[
                    0
                ].type.multiArrayType.dataType = coremltools.proto.FeatureTypes_pb2.ArrayFeatureType.ArrayDataType.Value(
                    self._feature_data_type(dtype)
                )
                coreml_model = coremltools.models.MLModel(spec)
                coreml_out = coreml_model.predict(
                    {"data": np.expand_dims(input_data, 0)}
                )["target"]
                self.assertEqual(dtype, coreml_out.dtype)
                if dtype != np.int32:
                    for idx in range(0, len(keras_out)):
                        self.assertAlmostEqual(
                            keras_out[0][idx],
                            coreml_out[idx],
                            msg="{}\n{} != {}".format(dtype, keras_out, coreml_out),
                            places=2,
                        )
            except KeyError:
                print("{} not supported. ".format(dtype))

    def test_keras_embedding_model(self):

        model = keras.models.Sequential()
        model.add(keras.layers.Embedding(100, 3, input_length=5, input_dtype="float32"))
        for key, dtype in self.number_data_type.items():
            try:
                input_data = np.random.randint(0, 100, size=(1, 5)).astype(key)
                keras_out = np.squeeze(model.predict(input_data)).flatten()
                coreml_model = coremltools.converters.keras.convert(
                    model, input_names=["data"], output_names=["target"]
                )

                spec = coreml_model.get_spec()
                spec.description.output[
                    0
                ].type.multiArrayType.dataType = coremltools.proto.FeatureTypes_pb2.ArrayFeatureType.ArrayDataType.Value(
                    self._feature_data_type(dtype)
                )
                spec.description.input[
                    0
                ].type.multiArrayType.dataType = coremltools.proto.FeatureTypes_pb2.ArrayFeatureType.ArrayDataType.Value(
                    self._feature_data_type(dtype)
                )
                coreml_model = coremltools.models.MLModel(spec)
                coreml_out = np.squeeze(
                    coreml_model.predict({"data": np.expand_dims(input_data, 0).T})[
                        "target"
                    ]
                ).flatten()
                self.assertEqual(dtype, coreml_out.dtype)
                if dtype != np.int32:
                    for idx in range(0, len(keras_out)):
                        self.assertAlmostEqual(
                            keras_out[idx],
                            coreml_out[idx],
                            msg="{}\n{} != {}".format(dtype, keras_out, coreml_out),
                            places=2,
                        )
            except KeyError:
                print("{} not supported. ".format(dtype))

    def test_image_output_rgb(self):
        input_shape = (3, 10, 20)
        input_features = [("data", coremltools.models.datatypes.Array(*input_shape))]
        output_features = [("target", coremltools.models.datatypes.Array(*input_shape))]
        builder = coremltools.models.neural_network.NeuralNetworkBuilder(
            input_features, output_features
        )
        builder.add_elementwise(
            "Identity",
            input_names=["data"],
            output_name="target",
            mode="ADD",
            alpha=0.0,
        )
        spec = builder.spec
        output = spec.description.output[0]
        output.type.imageType.colorSpace = coremltools.proto.FeatureTypes_pb2.ImageFeatureType.ColorSpace.Value(
            "RGB"
        )
        output.type.imageType.height = input_shape[1]
        output.type.imageType.width = input_shape[2]

        coreml_model = coremltools.models.MLModel(spec)
        input_data = np.floor(np.random.rand(*input_shape) * 255)

        coreml_out = coreml_model.predict({"data": input_data})["target"]
        self.assertEqual(PIL.Image.Image, type(coreml_out))
        self.assertEqual("RGBA", coreml_out.mode)
        np.testing.assert_equal(
            np.uint8(input_data), np.array(coreml_out).transpose(2, 0, 1)[:3, :]
        )

    def test_image_output_bgr(self):
        input_shape = (3, 15, 25)
        input_features = [("data", coremltools.models.datatypes.Array(*input_shape))]
        output_features = [("target", coremltools.models.datatypes.Array(*input_shape))]
        builder = coremltools.models.neural_network.NeuralNetworkBuilder(
            input_features, output_features
        )
        builder.add_elementwise(
            "Identity",
            input_names=["data"],
            output_name="target",
            mode="ADD",
            alpha=0.0,
        )
        spec = builder.spec
        output = spec.description.output[0]
        output.type.imageType.colorSpace = coremltools.proto.FeatureTypes_pb2.ImageFeatureType.ColorSpace.Value(
            "BGR"
        )
        output.type.imageType.height = input_shape[1]
        output.type.imageType.width = input_shape[2]

        coreml_model = coremltools.models.MLModel(spec)
        input_data = np.floor(np.random.rand(*input_shape) * 255)

        coreml_out = coreml_model.predict({"data": input_data})["target"]
        self.assertEqual(PIL.Image.Image, type(coreml_out))
        self.assertEqual("RGBA", coreml_out.mode)
        np.testing.assert_equal(
            np.uint8(input_data),
            np.array(coreml_out)[:, :, ::-1].transpose(2, 0, 1)[1:, :],
        )

    def test_image_output_grayscale(self):
        input_shape = (1, 20, 30)
        input_features = [("data", coremltools.models.datatypes.Array(*input_shape))]
        output_features = [("target", coremltools.models.datatypes.Array(*input_shape))]
        builder = coremltools.models.neural_network.NeuralNetworkBuilder(
            input_features, output_features
        )
        builder.add_elementwise(
            "Identity",
            input_names=["data"],
            output_name="target",
            mode="ADD",
            alpha=0.0,
        )
        spec = builder.spec
        output = spec.description.output[0]
        output.type.imageType.colorSpace = coremltools.proto.FeatureTypes_pb2.ImageFeatureType.ColorSpace.Value(
            "GRAYSCALE"
        )
        output.type.imageType.height = input_shape[1]
        output.type.imageType.width = input_shape[2]

        coreml_model = coremltools.models.MLModel(spec)
        input_data = np.floor(np.random.rand(*input_shape) * 255)

        coreml_out = coreml_model.predict({"data": input_data})["target"]
        self.assertEqual(PIL.Image.Image, type(coreml_out))
        self.assertEqual("L", coreml_out.mode)
        np.testing.assert_equal(np.uint8(input_data)[0], np.array(coreml_out))

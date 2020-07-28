# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import coremltools
import pytest
import unittest
import tempfile
import numpy as np
from coremltools.proto import Model_pb2
import PIL.Image

from coremltools.models.utils import (
    rename_feature,
    save_spec,
    _macos_version,
    _convert_neural_network_spec_weights_to_fp16,
    _is_macos,
    convert_double_to_float_multiarray_type,
)
from coremltools.models import MLModel, datatypes
from coremltools.models.neural_network import NeuralNetworkBuilder
from coremltools.models.neural_network.utils import make_image_input, make_nn_classifier


class MLModelTest(unittest.TestCase):
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

        lr = spec.glmRegressor
        lr.offset.append(0.1)
        weights = lr.weights.add()
        coefs = [1.0, 2.0]
        for i in coefs:
            weights.value.append(i)

        spec.description.predictedFeatureName = "output"
        self.spec = spec

    def test_model_creation(self):
        model = MLModel(self.spec)
        self.assertIsNotNone(model)

        filename = tempfile.mktemp(suffix=".mlmodel")
        save_spec(self.spec, filename)
        model = MLModel(filename)
        self.assertIsNotNone(model)

    def test_model_api(self):
        model = MLModel(self.spec)
        self.assertIsNotNone(model)

        model.author = "Test author"
        self.assertEqual(model.author, "Test author")
        self.assertEqual(model.get_spec().description.metadata.author, "Test author")

        model.license = "Test license"
        self.assertEqual(model.license, "Test license")
        self.assertEqual(model.get_spec().description.metadata.license, "Test license")

        model.short_description = "Test model"
        self.assertEqual(model.short_description, "Test model")
        self.assertEqual(
            model.get_spec().description.metadata.shortDescription, "Test model"
        )

        model.version = "1.3"
        self.assertEqual(model.version, "1.3")
        self.assertEqual(model.get_spec().description.metadata.versionString, "1.3")

        model.input_description["feature_1"] = "This is feature 1"
        self.assertEqual(model.input_description["feature_1"], "This is feature 1")

        model.output_description["output"] = "This is output"
        self.assertEqual(model.output_description["output"], "This is output")

        filename = tempfile.mktemp(suffix=".mlmodel")
        model.save(filename)
        loaded_model = MLModel(filename)

        self.assertEqual(model.author, "Test author")
        self.assertEqual(model.license, "Test license")
        # self.assertEqual(model.short_description, 'Test model')
        self.assertEqual(model.input_description["feature_1"], "This is feature 1")
        self.assertEqual(model.output_description["output"], "This is output")

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_predict_api(self):
        model = MLModel(self.spec)
        preds = model.predict({"feature_1": 1.0, "feature_2": 1.0})
        self.assertIsNotNone(preds)
        self.assertEqual(preds["output"], 3.1)

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_rename_input(self):
        rename_feature(self.spec, "feature_1", "renamed_feature", rename_inputs=True)
        model = MLModel(self.spec)
        preds = model.predict({"renamed_feature": 1.0, "feature_2": 1.0})
        self.assertIsNotNone(preds)
        self.assertEqual(preds["output"], 3.1)
        # reset the spec for next run
        rename_feature(self.spec, "renamed_feature", "feature_1", rename_inputs=True)

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_rename_input_bad(self):
        rename_feature(self.spec, "blah", "bad_name", rename_inputs=True)
        model = MLModel(self.spec)
        preds = model.predict({"feature_1": 1.0, "feature_2": 1.0})
        self.assertIsNotNone(preds)
        self.assertEqual(preds["output"], 3.1)

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_rename_output(self):
        rename_feature(
            self.spec,
            "output",
            "renamed_output",
            rename_inputs=False,
            rename_outputs=True,
        )
        model = MLModel(self.spec)
        preds = model.predict({"feature_1": 1.0, "feature_2": 1.0})
        self.assertIsNotNone(preds)
        self.assertEqual(preds["renamed_output"], 3.1)
        rename_feature(
            self.spec,
            "renamed_output",
            "output",
            rename_inputs=False,
            rename_outputs=True,
        )

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_rename_output_bad(self):
        rename_feature(
            self.spec, "blah", "bad_name", rename_inputs=False, rename_outputs=True
        )
        model = MLModel(self.spec)
        preds = model.predict({"feature_1": 1.0, "feature_2": 1.0})
        self.assertIsNotNone(preds)
        self.assertEqual(preds["output"], 3.1)

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_future_version(self):
        self.spec.specificationVersion = 10000
        filename = tempfile.mktemp(suffix=".mlmodel")
        save_spec(self.spec, filename, auto_set_specification_version=False)
        model = MLModel(filename)
        # this model should exist, but throw an exception when we try to use
        # predict because the engine doesn't support this model version
        self.assertIsNotNone(model)
        with self.assertRaises(Exception):
            try:
                model.predict({})
            except Exception as e:
                assert "Core ML model specification version" in str(e)
                raise
        self.spec.specificationVersion = 1

    @unittest.skipUnless(
        _is_macos() and _macos_version() < (10, 13), "Only supported on macOS 10.13-"
    )
    def test_MLModel_warning(self):
        self.spec.specificationVersion = 3
        import warnings

        with warnings.catch_warnings(record=True) as w:
            # Cause all warnings to always be triggered.
            warnings.simplefilter("always")
            model = MLModel(self.spec)
            assert len(w) == 1
            assert issubclass(w[-1].category, RuntimeWarning)
            assert "not able to run predict()" in str(w[-1].message)
        self.spec.specificationVersion = 1
        model = MLModel(self.spec)

    def test_convert_nn_spec_to_half_precision(self):
        # simple network with quantization layer
        input_features = [("data", datatypes.Array(3))]
        output_features = [("out", datatypes.Array(3))]
        builder = NeuralNetworkBuilder(input_features, output_features)
        weights = np.random.uniform(-0.5, 0.5, (3, 3))
        builder.add_inner_product(
            name="inner_product",
            W=weights,
            b=None,
            input_channels=3,
            output_channels=3,
            has_bias=False,
            input_name="data",
            output_name="out",
        )
        model = MLModel(builder.spec)
        spec = _convert_neural_network_spec_weights_to_fp16(model.get_spec())
        self.assertIsNotNone(spec)

        # simple network without quantization layer
        input_features = [("data", datatypes.Array(3))]
        output_features = [("out", datatypes.Array(3))]
        builder = NeuralNetworkBuilder(input_features, output_features)
        builder.add_lrn(
            name="lrn",
            input_name="data",
            output_name="out",
            alpha=2,
            beta=3,
            local_size=1,
            k=8,
        )
        model = MLModel(builder.spec)
        spec = _convert_neural_network_spec_weights_to_fp16(model.get_spec())
        self.assertIsNotNone(spec)

    @unittest.skip
    def test_downgrade_specification_version(self):
        # manually set a invalid specification version
        self.spec.specificationVersion = -1
        model = MLModel(self.spec)
        assert model.get_spec().specificationVersion == 1

        # manually set a high specification version
        self.spec.specificationVersion = 4
        filename = tempfile.mktemp(suffix=".mlmodel")
        save_spec(self.spec, filename, auto_set_specification_version=True)
        model = MLModel(filename)
        assert model.get_spec().specificationVersion == 1

        # simple neural network with only spec 1 layer
        input_features = [("data", datatypes.Array(3))]
        output_features = [("out", datatypes.Array(3))]
        builder = NeuralNetworkBuilder(input_features, output_features)
        builder.add_activation("relu", "RELU", "data", "out")
        # set a high specification version
        builder.spec.specificationVersion = 3
        model = MLModel(builder.spec)
        filename = tempfile.mktemp(suffix=".mlmodel")
        model.save(filename)
        # load the model back
        model = MLModel(filename)
        assert model.get_spec().specificationVersion == 1

        # test save without automatic set specification version
        self.spec.specificationVersion = 3
        filename = tempfile.mktemp(suffix=".mlmodel")
        save_spec(self.spec, filename, auto_set_specification_version=False)
        model = MLModel(filename)
        # the specification version should be original
        assert model.get_spec().specificationVersion == 3

    def test_multiarray_type_convert_to_float(self):
        input_features = [("data", datatypes.Array(2))]
        output_features = [("out", datatypes.Array(2))]
        builder = NeuralNetworkBuilder(input_features, output_features)
        builder.add_ceil("ceil", "data", "out")
        spec = builder.spec
        self.assertEqual(
            spec.description.input[0].type.multiArrayType.dataType,
            Model_pb2.ArrayFeatureType.DOUBLE,
        )
        self.assertEqual(
            spec.description.output[0].type.multiArrayType.dataType,
            Model_pb2.ArrayFeatureType.DOUBLE,
        )
        convert_double_to_float_multiarray_type(spec)
        self.assertEqual(
            spec.description.input[0].type.multiArrayType.dataType,
            Model_pb2.ArrayFeatureType.FLOAT32,
        )
        self.assertEqual(
            spec.description.output[0].type.multiArrayType.dataType,
            Model_pb2.ArrayFeatureType.FLOAT32,
        )

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_multiarray_to_image_input_util(self):
        H, W, C = 1, 1, 3
        input_features = [("data", datatypes.Array(C, H, W))]
        output_features = [("out", datatypes.Array(C, H, W))]
        builder = NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        builder.add_activation("linear", "LINEAR", "data", "out")
        spec = builder.spec
        mlmodel = MLModel(spec)
        mlmodel = make_image_input(
            mlmodel,
            "data",
            red_bias=-5,
            green_bias=-6,
            blue_bias=-2.5,
            scale=10.0,
            image_format="NCHW",
        )
        x = np.array([4, 2, 5], dtype=np.uint8)
        x = np.reshape(x, (H, W, C))
        pil_img = PIL.Image.fromarray(x)
        y = mlmodel.predict({"data": pil_img}, useCPUOnly=True)["out"]
        self.assertEqual(y.shape, (C, H, W))
        np.testing.assert_almost_equal(y.flatten(), [35.0, 14.0, 47.5])

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_multiarray_to_image_input_util_transpose_elimination(self):
        H, W, C = 1, 1, 3
        input_features = [("data", datatypes.Array(H, W, C))]
        output_features = [("out", datatypes.Array(H, W, C))]
        builder = NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        builder.add_transpose("transpose", [2, 0, 1], "data", "transpose")
        builder.add_activation("linear", "LINEAR", "transpose", "out")
        spec = builder.spec
        mlmodel = MLModel(spec)
        mlmodel = make_image_input(
            mlmodel,
            "data",
            red_bias=-5,
            green_bias=-6,
            blue_bias=-2.5,
            scale=10.0,
            image_format="NHWC",
        )
        x = np.array([4, 2, 5], dtype=np.uint8)
        x = np.reshape(x, (H, W, C))
        pil_img = PIL.Image.fromarray(x)
        y = mlmodel.predict({"data": pil_img}, useCPUOnly=True)["out"]
        self.assertEqual(y.shape, (H, W, C))
        np.testing.assert_almost_equal(y.flatten(), [35.0, 14.0, 47.5])

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_multiarray_to_image_input_util_HWC_format(self):
        H, W, C = 1, 1, 3
        input_features = [("data", datatypes.Array(H, W, C))]
        output_features = [("out", datatypes.Array(H, W, C))]
        builder = NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        builder.add_activation("linear", "LINEAR", "data", "out")
        spec = builder.spec
        mlmodel = MLModel(spec)
        mlmodel = make_image_input(
            mlmodel,
            "data",
            red_bias=-5,
            green_bias=-6,
            blue_bias=-2.5,
            scale=10.0,
            image_format="NHWC",
        )
        x = np.array([4, 2, 5], dtype=np.uint8)
        x = np.reshape(x, (H, W, C))
        pil_img = PIL.Image.fromarray(x)
        y = mlmodel.predict({"data": pil_img}, useCPUOnly=True)["out"]
        self.assertEqual(y.shape, (H, W, C))
        np.testing.assert_almost_equal(y.flatten(), [35.0, 14.0, 47.5])

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_nn_classifier_util(self):
        input_features = [("data", datatypes.Array(3))]
        output_features = [("out", datatypes.Array(3))]
        builder = NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        builder.add_activation("linear", "LINEAR", "data", "out")
        spec = builder.spec
        mlmodel = MLModel(spec)
        mlmodel = make_nn_classifier(
            mlmodel,
            class_labels=["a", "b", "c"],
            predicted_feature_name="out_confidence",
            predicted_probabilities_output="out",
        )
        out_dict = mlmodel.predict({"data": np.array([4.0, 5.5, 6.0])}, useCPUOnly=True)
        self.assertEqual(out_dict["out_confidence"], "c")
        self.assertEqual(
            mlmodel.get_spec().WhichOneof("Type"), "neuralNetworkClassifier"
        )

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_nn_classifier_util_file(self):
        input_features = [("data", datatypes.Array(3))]
        output_features = [("out", datatypes.Array(3))]
        builder = NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        builder.add_activation("linear", "LINEAR", "data", "out")
        spec = builder.spec
        mlmodel = MLModel(spec)

        class_labels = ["a", "b", "c"]
        with tempfile.NamedTemporaryFile(mode="w", suffix=".txt") as f:
            f.write("\n".join(class_labels))
            f.flush()
            mlmodel = make_nn_classifier(
                mlmodel,
                class_labels=f.name,
                predicted_feature_name="out_confidence",
                predicted_probabilities_output="out",
            )
        out_dict = mlmodel.predict({"data": np.array([4.0, 5.5, 6.0])}, useCPUOnly=True)
        self.assertEqual(out_dict["out_confidence"], "c")
        self.assertEqual(
            mlmodel.get_spec().WhichOneof("Type"), "neuralNetworkClassifier"
        )

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_rename_output_nn_classifier(self):
        input_features = [("data", datatypes.Array(3))]
        output_features = [("out", datatypes.Array(3))]
        builder = NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        builder.add_activation("linear", "LINEAR", "data", "out")
        spec = builder.spec
        mlmodel = MLModel(spec)

        class_labels = ["a", "b", "c"]
        mlmodel = make_nn_classifier(mlmodel, class_labels=["a", "b", "c"])

        # rename output
        spec = mlmodel.get_spec()
        rename_feature(spec, "out", "new_out_name")
        mlmodel = MLModel(spec)

        out_dict = mlmodel.predict({"data": np.array([4.0, 5.5, 6.0])}, useCPUOnly=True)
        self.assertEqual(out_dict["classLabel"], "c")
        self.assertTrue("new_out_name" in out_dict)
        self.assertTrue(isinstance(out_dict["new_out_name"], dict))


if __name__ == "__main__":
    unittest.main()
    # suite = unittest.TestSuite()
    # suite.addTest(MLModelTest('test_multiarray_type_convert_to_float'))
    # unittest.TextTestRunner().run(suite)

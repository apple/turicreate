#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import os
import shutil
import tempfile

import pytest
import coremltools.converters as converter
from coremltools.converters.mil.input_types import TensorType
from coremltools.converters.mil.frontend.tensorflow.test.testing_utils import (
    get_tf_keras_io_names,
)
from coremltools.converters.mil.frontend.tensorflow.test import (
    testing_utils as tf_testing_utils,
)
from coremltools.converters.mil.frontend.tensorflow2.test.testing_utils import (
    make_tf2_graph,
    run_compare_tf2,
)

tf = pytest.importorskip("tensorflow", minversion="2.1.0")

# -----------------------------------------------------------------------------
# Overwrite utilities to enable different conversion / compare method
tf_testing_utils.frontend = "tensorflow"
tf_testing_utils.make_tf_graph = make_tf2_graph
tf_testing_utils.run_compare_tf = run_compare_tf2

# -----------------------------------------------------------------------------
# Import TF 2.x-compatible TF 1.x test cases
from coremltools.converters.mil.frontend.tensorflow.test.test_load import (
    frontend,
    TestTf1ModelInputsOutputs as TestTf2ModelInputsOutputs,
)


class TestTf2ModelFormats:
    def setup(self):
        self.saved_model_dir = tempfile.mkdtemp()
        _, self.model_path_h5 = tempfile.mkstemp(
            suffix=".h5", prefix=self.saved_model_dir
        )
        _, self.model_path_pb = tempfile.mkstemp(
            suffix=".pb", prefix=self.saved_model_dir
        )

    def teardown(self):
        if os.path.exists(self.saved_model_dir):
            shutil.rmtree(self.saved_model_dir)

    def test_keras_model(self):
        keras_model = tf.keras.Sequential(
            [tf.keras.layers.ReLU(input_shape=(4, 5), batch_size=3)]
        )
        input_names, output_names = get_tf_keras_io_names(keras_model)
        mlmodel = converter.convert(
            keras_model,
            inputs=[TensorType(input_names[0], (3, 4, 5))],
            outputs=["Identity"],
            source=frontend,
        )
        assert mlmodel is not None

    def test_keras_saved_model_file(self):
        keras_model = tf.keras.Sequential(
            [
                tf.keras.layers.Flatten(input_shape=(28, 28), batch_size=1),
                tf.keras.layers.Dense(10, activation=tf.nn.relu),
            ]
        )
        keras_model.save(self.saved_model_dir, save_format="tf")
        mlmodel = converter.convert(
            self.saved_model_dir, outputs="Identity", source=frontend
        )
        assert mlmodel is not None

    def test_keras_h5_file(self):
        keras_model = tf.keras.Sequential(
            [tf.keras.layers.ReLU(input_shape=(4, 5), batch_size=3)]
        )
        input_names, output_names = get_tf_keras_io_names(keras_model)
        keras_model.save(self.model_path_h5, save_format="h5")
        mlmodel = converter.convert(
            self.model_path_h5,
            inputs=[TensorType(input_names[0], (3, 4, 5))],
            outputs=["Identity"],
            source=frontend,
        )
        assert mlmodel is not None

    def test_concrete_function_list_from_tf_low_level_api(self):
        root = tf.train.Checkpoint()
        root.v1 = tf.Variable(3.0)
        root.v2 = tf.Variable(2.0)
        root.f = tf.function(lambda x: root.v1 * root.v2 * x)

        input_data = tf.constant(1.0, shape=[1, 1])
        to_save = root.f.get_concrete_function(input_data)
        tf.saved_model.save(root, self.saved_model_dir, to_save)

        tf_model = tf.saved_model.load(self.saved_model_dir)
        concrete_func = tf_model.signatures[
            tf.saved_model.DEFAULT_SERVING_SIGNATURE_DEF_KEY
        ]
        mlmodel = converter.convert(
            [concrete_func], outputs="Identity", source=frontend
        )
        assert mlmodel is not None

    def test_saved_model_list_from_tf_function(self):
        class build_model(tf.Module):
            @tf.function(
                input_signature=[tf.TensorSpec(shape=[3, 4, 5], dtype=tf.float32)]
            )
            def __call__(self, x):
                return tf.nn.relu(x)

        model = build_model()
        tf.saved_model.save(model, self.saved_model_dir)
        mlmodel = converter.convert(
            self.saved_model_dir, outputs=["Identity"], source=frontend
        )
        assert mlmodel is not None

    def test_concrete_function_list_from_tf_function(self):
        class build_model(tf.Module):
            @tf.function(
                input_signature=[tf.TensorSpec(shape=[3, 4, 5], dtype=tf.float32)]
            )
            def __call__(self, x):
                return tf.nn.relu(x)

        model = build_model()
        concrete_func = model.__call__.get_concrete_function()
        mlmodel = converter.convert(
            [concrete_func], outputs=["Identity"], source=frontend
        )
        assert mlmodel is not None

    def test_model_metadata(self):
        keras_model = tf.keras.Sequential(
            [tf.keras.layers.ReLU(input_shape=(4, 5), batch_size=3)]
        )
        input_names, output_names = get_tf_keras_io_names(keras_model)
        mlmodel = converter.convert(
            keras_model,
            inputs=[TensorType(input_names[0], (3, 4, 5))],
            outputs=["Identity"],
            source=frontend,
        )
        metadata_keys = mlmodel.get_spec().description.metadata.userDefined
        assert "com.github.apple.coremltools.version" in metadata_keys
        assert "com.github.apple.coremltools.source" in metadata_keys
        assert "tensorflow==2." in metadata_keys["com.github.apple.coremltools.source"]

    def test_invalid_format_none(self):
        with pytest.raises(NotImplementedError) as e:
            converter.convert(None, source=frontend)
        e.match(r"Expected model format: .* .h5")

    def test_invalid_format_invalid_extension(self):
        _, invalid_filename = tempfile.mkstemp(
            suffix=".invalid", prefix=self.saved_model_dir
        )
        with pytest.raises(NotImplementedError) as e:
            converter.convert(invalid_filename, source=frontend)
        e.match(r"Expected model format: .* .h5")

    def test_invalid_format_multiple_concrete_functions(self):
        class build_model(tf.Module):
            @tf.function(
                input_signature=[tf.TensorSpec(shape=[3, 4, 5], dtype=tf.float32)]
            )
            def __call__(self, x):
                return tf.nn.relu(x)

        model = build_model()
        cf = model.__call__.get_concrete_function()
        with pytest.raises(NotImplementedError) as e:
            converter.convert([cf, cf, cf], source=frontend)
        e.match(r"Only a single concrete function is supported")

    def test_invalid_converter_type(self):
        with pytest.raises(ValueError) as e:
            converter.convert(None, source="invalid")

        expected_msg = r'Unrecognized value of argument "source": .*'
        e.match(expected_msg)

        with pytest.raises(NotImplementedError) as e:
            converter.convert(None, convert_to="invalid", source=frontend)
        e.match(r"Backend converter .* not implemented")

    def test_invalid_format_non_exist(self):
        non_exist_filename = self.model_path_h5.replace(".h5", "_non_exist.h5")
        with pytest.raises(ValueError) as e:
            converter.convert(non_exist_filename, source=frontend)
        e.match(r"Input model .* does not exist")

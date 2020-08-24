#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import numpy as np
import os
import six
import pytest
import shutil
import tempfile
import coremltools as ct
import coremltools.converters as converter
from coremltools._deps import _IS_MACOS
import coremltools.proto.FeatureTypes_pb2 as ft
from coremltools import TensorType, ImageType, RangeDim, EnumeratedShapes
from coremltools.converters.mil.testing_utils import random_gen
from coremltools.converters.mil.frontend.tensorflow.converter import TFConverter
from coremltools.converters.mil.frontend.tensorflow.test.testing_utils import (
    frontend,
    make_tf_graph,
    run_compare_tf,
    get_tf_keras_io_names,
)

tf = pytest.importorskip("tensorflow")


class TestTf1ModelInputsOutputs:
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

    def test_infer_inputs(self):
        x_shape = (3, 4, 5)

        @make_tf_graph([x_shape])
        def build_model(x):
            return tf.nn.relu(x)

        model, inputs, outputs = build_model
        if not isinstance(outputs, (tuple, list)):
            outputs = [outputs]

        output_names = [
            j if isinstance(j, six.string_types) else j.op.name for j in outputs
        ]
        mlmodel = converter.convert(model, outputs=output_names)
        assert mlmodel is not None

        input_values = [random_gen(x_shape, -10.0, 10.0)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(model, input_dict, outputs)

    def test_infer_outputs(self):
        x_shape = (3, 4, 5)

        @make_tf_graph([x_shape])
        def build_model(x):
            return tf.nn.relu(x)

        model, inputs, outputs = build_model
        input_name = (
            inputs[0] if isinstance(inputs[0], six.string_types) else inputs[0].op.name
        )
        mlmodel = converter.convert(model, inputs=[TensorType(input_name, (3, 4, 5))])
        assert mlmodel is not None

        input_values = [random_gen(x_shape, -10.0, 10.0)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(model, input_dict, outputs)

    def test_infer_inputs_and_outputs(self):
        x_shape = (3, 4, 5)

        @make_tf_graph([x_shape])
        def build_model(x):
            return tf.nn.relu(x)

        model, inputs, outputs = build_model
        mlmodel = converter.convert(model)
        assert mlmodel is not None

        input_values = [random_gen(x_shape, -10.0, 10.0)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(model, input_dict, outputs)

    def test_extract_sub_model(self):
        x_shape = (3, 4, 5)
        y_shape = (3, 4, 5)

        @make_tf_graph([x_shape, y_shape])
        def build_model(x, y):
            return tf.nn.relu(x), tf.math.add(x, y)

        model, inputs, outputs = build_model
        if isinstance(outputs[0], six.string_types):
            first_output_name = outputs[0]
        else:
            first_output_name = outputs[0].name.split(":")[0]
        mlmodel = converter.convert(model, outputs=[first_output_name])
        assert mlmodel is not None

    def test_auto_image_nhwc_input_names(self):
        x_shape = (4, 5, 3)

        @make_tf_graph([x_shape])
        def build_model(x):
            return tf.nn.relu(x)

        model, inputs, outputs = build_model

        mlmodel = converter.convert(model, inputs=[ImageType()])
        assert mlmodel is not None

    def test_auto_image_nchw_input_names(self):
        x_shape = (3, 4, 5)

        @make_tf_graph([x_shape])
        def build_model(x):
            return tf.nn.relu(x)

        model, inputs, outputs = build_model

        mlmodel = converter.convert(model, inputs=[ImageType(channel_first=True)])
        assert mlmodel is not None

    @pytest.mark.parametrize(
        "target",
        [ct.target.iOS13, ct.target.macOS15, ct.target.watchOS6, ct.target.tvOS13],
    )
    def test_invalid_deployment_target_cumsum(self, target):
        x_shape = (3, 4, 5)

        @make_tf_graph([x_shape])
        def build_model(x):
            return tf.math.cumsum(x, axis=-1, reverse=False, exclusive=False)

        model, inputs, outputs = build_model

        with pytest.raises(ValueError) as e:
            converter.convert(model, minimum_deployment_target=target)
        e.match(
            r"Provided minimum deployment target .* version 4 but converted model "
            r"uses .* available from version 5 onwards.\n    1. Cumsum operation\n"
        )

    @pytest.mark.parametrize(
        "target",
        [ct.target.iOS14, ct.target.macOS16, ct.target.watchOS7, ct.target.tvOS14],
    )
    def test_valid_deployment_target_cumsum(self, target):
        x_shape = (3, 4, 5)

        @make_tf_graph([x_shape])
        def build_model(x):
            return tf.math.cumsum(x, axis=-1, reverse=False, exclusive=False)

        model, inputs, outputs = build_model

        # successful conversion
        converter.convert(model, minimum_deployment_target=target)

    def test_invalid_output_names(self):
        x_shape = (3, 4, 5)

        @make_tf_graph([x_shape])
        def build_model(x):
            return tf.nn.relu(x)

        model, inputs, outputs = build_model
        with pytest.raises(AssertionError) as e:
            converter.convert(model, source=frontend, outputs=["invalid_name"])
        e.match(r".* is not in graph")

    def test_shaping_utils(self):
        @make_tf_graph([(None, 4, 5)])
        def build_flexible_model(x):
            return tf.nn.relu(x)

        model, inputs, outputs = build_flexible_model
        input_name = TFConverter._get_tensor_name(inputs[0])
        output_name = TFConverter._get_tensor_name(outputs[0])

        # static-Flexible shape
        mlmodel = converter.convert(
            model, inputs=[TensorType(name=input_name)], outputs=[output_name]
        )
        assert mlmodel is not None
        input_values = [random_gen((3, 4, 5), -10.0, 10.0)]
        input_dict = {input_name: input_values[0]}
        if _IS_MACOS:
            ret = mlmodel.predict(input_dict)
            np.allclose(ret[output_name], np.maximum(input_values[0], 0.0))

        # Enumerate shape
        inputs_shape = [
            TensorType(input_name, EnumeratedShapes(shapes=[(3, 4, 5), (4, 4, 5)]))
        ]
        mlmodel = converter.convert(model, inputs=inputs_shape, outputs=[output_name])
        assert mlmodel is not None
        input_values = [random_gen((3, 4, 5), -10.0, 10.0)]
        input_dict = {input_name: input_values[0]}
        if _IS_MACOS:
            ret = mlmodel.predict(input_dict)
            np.allclose(ret[output_name], np.maximum(input_values[0], 0.0))

        input_values = [random_gen((4, 4, 5), -10.0, 10.0)]
        input_dict = {input_name: input_values[0]}
        if _IS_MACOS:
            ret = mlmodel.predict(input_dict)
            np.allclose(ret[output_name], np.maximum(input_values[0], 0.0))

        if _IS_MACOS:
            with pytest.raises(RuntimeError) as e:
                input_values = [random_gen((5, 4, 5), -10.0, 10.0)]
                input_dict = {input_name: input_values[0]}
                ret = mlmodel.predict(input_dict)

        # Ranged shape
        inputs_shape = [TensorType(input_name, [RangeDim(3, 5), 4, 5])]
        mlmodel = converter.convert(model, inputs=inputs_shape, outputs=[output_name])
        assert mlmodel is not None
        input_values = [random_gen((3, 4, 5), -10.0, 10.0)]
        input_dict = {input_name: input_values[0]}
        if _IS_MACOS:
            ret = mlmodel.predict(input_dict)
            np.allclose(ret[output_name], np.maximum(input_values[0], 0.0))

        input_values = [random_gen((4, 4, 5), -10.0, 10.0)]
        input_dict = {input_name: input_values[0]}
        if _IS_MACOS:
            ret = mlmodel.predict(input_dict)
            np.allclose(ret[output_name], np.maximum(input_values[0], 0.0))

        if _IS_MACOS:
            with pytest.raises(RuntimeError) as e:
                input_values = [random_gen((2, 4, 5), -10.0, 10.0)]
                input_dict = {input_name: input_values[0]}
                ret = mlmodel.predict(input_dict)

    def test_default_data_types(self):
        @make_tf_graph([(2, 2)])
        def build_model(x):
            return tf.nn.relu(x)

        model, inputs, outputs = build_model
        mlmodel = converter.convert(model)
        assert mlmodel is not None
        spec = mlmodel.get_spec()

        # Defaults should be FLOAT32 instead of DOUBLE
        it = spec.description.input[0].type.multiArrayType.dataType
        assert it == ft.ArrayFeatureType.ArrayDataType.Value("FLOAT32")
        ot = spec.description.output[0].type.multiArrayType.dataType
        assert ot == ft.ArrayFeatureType.ArrayDataType.Value("FLOAT32")


class TestTf1ModelFormats:
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

    def test_graph_def(self):
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=(3, 4, 5))
            out = tf.nn.relu(x)
        mlmodel = converter.convert(
            graph, inputs=[TensorType(x.op.name, (3, 4, 5))], outputs=[out.op.name]
        )
        assert mlmodel is not None

    def test_graph_def_file(self):
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=(3, 4, 5))
            out = tf.nn.relu(x)
        tf.io.write_graph(
            graph, self.saved_model_dir, self.model_path_pb, as_text=False
        )
        mlmodel = converter.convert(
            self.model_path_pb,
            inputs=[TensorType(x.op.name, (3, 4, 5))],
            outputs=[out.op.name],
        )
        assert mlmodel is not None

    def test_saved_model_from_simple_save(self):
        with tf.compat.v1.Session() as sess:
            x = tf.placeholder(shape=(1, 3, 5), dtype=tf.float32)
            y = tf.nn.relu(x)
            inputs = {"x": x}
            outputs = {"y": y}
            tf.compat.v1.saved_model.simple_save(
                sess, self.saved_model_dir, inputs, outputs
            )
        mlmodel = converter.convert(self.saved_model_dir)
        assert mlmodel is not None

    def test_tf_keras(self):
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

    def test_tf_keras_hdf5_file(self):
        keras_model = tf.keras.Sequential(
            [tf.keras.layers.ReLU(input_shape=(4, 5), batch_size=3)]
        )
        keras_model.save(self.model_path_h5)
        input_names, output_names = get_tf_keras_io_names(keras_model)
        mlmodel = converter.convert(
            self.model_path_h5,
            inputs=[TensorType(input_names[0], (3, 4, 5))],
            outputs=["Identity"],
            source=frontend,
        )
        assert mlmodel is not None

    def test_model_metadata(self):
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=(3, 4, 5))
            out = tf.nn.relu(x)
        mlmodel = converter.convert(
            graph, inputs=[TensorType(x.op.name, (3, 4, 5))], outputs=[out.op.name]
        )
        metadata_keys = mlmodel.get_spec().description.metadata.userDefined
        assert "com.github.apple.coremltools.version" in metadata_keys
        assert "com.github.apple.coremltools.source" in metadata_keys
        assert "tensorflow==1." in metadata_keys["com.github.apple.coremltools.source"]

    def test_invalid_format_none(self):
        with pytest.raises(NotImplementedError) as e:
            converter.convert(None, source="tensorflow")
        e.match(r"Expected model format: .* .pb")

    def test_invalid_format_invalid_extension(self):
        _, invalid_filename = tempfile.mkstemp(
            suffix=".invalid", prefix=self.saved_model_dir
        )
        with pytest.raises(NotImplementedError) as e:
            converter.convert(invalid_filename, source="tensorflow")
        e.match(r"Expected model format: .* .pb")

    def test_invalid_converter_source(self):
        with pytest.raises(ValueError) as e:
            converter.convert(None, source="invalid")
        expected_msg = r'Unrecognized value of argument "source": .*'
        e.match(expected_msg)

    def test_invalid_converter_minimum_deployment_flag(self):
        with pytest.raises(TypeError) as e:
            converter.convert(
                None, source="tensorflow", minimum_deployment_target="iOs14"
            )
        expected_msg = (
            "Unrecognized value of argument 'minimum_deployment_target': iOs14. "
            "It needs to be a member of 'coremltools.target' enumeration"
        )

        e.match(expected_msg)

    def test_invalid_converter_target(self):
        with pytest.raises(NotImplementedError) as e:
            converter.convert(None, convert_to="invalid", source="tensorflow")
        e.match(r"Backend converter .* not implemented")

    def test_invalid_format_non_exist(self):
        non_exist_filename = self.model_path_pb.replace(".pb", "_non_exist.pb")
        with pytest.raises(ValueError) as e:
            converter.convert(non_exist_filename, source="tensorflow")
        e.match(r"Input model .* does not exist")

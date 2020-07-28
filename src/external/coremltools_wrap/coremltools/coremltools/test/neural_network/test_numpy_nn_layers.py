from __future__ import print_function as _

import itertools
import math
import os
import random
import shutil
import tempfile
import unittest
import uuid
import pytest
from packaging import version
from six import string_types as _string_types

import numpy as np
from coremltools._deps import _HAS_TF, MSG_TF1_NOT_FOUND

if _HAS_TF:
    import tensorflow as tf
import torch

import coremltools
import coremltools.models.datatypes as datatypes
from coremltools.converters.mil.mil.ops.defs._utils import aggregated_pad
from coremltools.models import _MLMODEL_FULL_PRECISION, _MLMODEL_HALF_PRECISION
from coremltools.models import neural_network as neural_network
from coremltools.models.neural_network import flexible_shape_utils
from coremltools.models.utils import _macos_version, _is_macos

np.random.seed(10)

MIN_MACOS_VERSION_REQUIRED = (10, 13)
LAYERS_10_15_MACOS_VERSION = (10, 15)
LAYERS_11_0_MACOS_VERSION = (11, 0)


def _get_unary_model_spec(x, mode, alpha=1.0):
    input_dim = x.shape
    input_features = [("data", datatypes.Array(*input_dim))]
    output_features = [("output", datatypes.Array(*input_dim))]

    builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

    builder.add_unary(
        name="unary", input_name="data", output_name="output", mode=mode, alpha=alpha
    )
    return builder.spec


class CorrectnessTest(unittest.TestCase):
    def runTest(self):
        pass

    def _compare_shapes(self, np_preds, coreml_preds):
        return np.squeeze(np_preds).shape == np.squeeze(coreml_preds).shape

    def _test_shape_equality(self, np_preds, coreml_preds):
        np.testing.assert_array_equal(
            np.squeeze(coreml_preds).shape, np.squeeze(np_preds).shape
        )

    def _test_nd_shape_equality(self, np_preds, coreml_preds, shape=()):
        if shape:
            np.testing.assert_array_equal(coreml_preds.shape, shape)
        else:
            # check if shape has 0 valued dimension
            if np.prod(np_preds.shape) == 0 and np.prod(coreml_preds.shape) == 0:
                return
            np.testing.assert_array_equal(coreml_preds.shape, np_preds.shape)

    def _compare_predictions(self, np_preds, coreml_preds, delta=0.01):
        np_preds = np_preds.flatten()
        coreml_preds = coreml_preds.flatten()
        max_arr = np.maximum(np.maximum(np_preds, coreml_preds), 1.0)
        all_deltas = np.abs(np_preds / max_arr - coreml_preds / max_arr)
        max_delta = np.amax(all_deltas)
        if max_delta > delta:
            return False
        return True

    def _test_predictions(
        self,
        np_preds,
        coreml_preds,
        delta=0.01,
        test_metric="rel_error",
        SNR=30,
        PSNR=40,
    ):
        np_preds = np_preds.flatten()
        coreml_preds = coreml_preds.flatten()
        if test_metric == "rel_error":
            max_arr = np.maximum(np.abs(np_preds), 1.0)
            all_deltas = np.abs(np_preds / max_arr - coreml_preds / max_arr)
            max_delta = np.amax(all_deltas, initial=0)
            self.assertLessEqual(
                max_delta,
                delta,
                "Expected %s to be within %s of %s" % (coreml_preds, delta, np_preds),
            )
        elif test_metric == "SNR":
            noise = np_preds - coreml_preds
            noise_var = np.sum(noise ** 2) / len(noise) + 1e-7
            signal_energy = np.sum(np_preds ** 2) / len(np_preds)
            max_signal_energy = np.amax(np_preds ** 2)
            snr = 10 * np.log10(signal_energy / noise_var)
            psnr = 10 * np.log10(max_signal_energy / noise_var)
            self.assertGreaterEqual(snr, SNR)
            self.assertGreaterEqual(psnr, PSNR)
        else:
            raise ValueError("Test metric not supported")

    @staticmethod
    def _compare_moments(model, inputs, expected, use_cpu_only=True, num_moments=10):
        """
        This utility function is used for validate random distributions layers.
        It validates the first 10 moments of prediction and expected values.
        """

        def get_moment(data, k):
            return np.mean(np.power(data - np.mean(data), k))

        if isinstance(model, _string_types):
            model = coremltools.models.MLModel(model)

        model = coremltools.models.MLModel(model, useCPUOnly=use_cpu_only)
        prediction = model.predict(inputs, useCPUOnly=use_cpu_only)

        for output_name in expected:
            np_preds = expected[output_name]
            coreml_preds = prediction[output_name]

            np_moments = [get_moment(np_preds.flatten(), k) for k in range(num_moments)]
            coreml_moments = [
                get_moment(coreml_preds.flatten(), k) for k in range(num_moments)
            ]

            np.testing.assert_almost_equal(np_moments, coreml_moments, decimal=2)

        # override expected values to allow element-wise compares
        for output_name in expected:
            expected[output_name] = prediction[output_name]

    def _test_model(
        self,
        model,
        input,
        expected,
        model_precision=_MLMODEL_FULL_PRECISION,
        useCPUOnly=False,
        output_name_shape_dict={},
        validate_shapes_only=False,
        test_metric="rel_error",
        delta=0.01,
        SNR=30,
    ):

        model_dir = None
        # if we're given a path to a model
        if isinstance(model, _string_types):
            model = coremltools.models.MLModel(model)

        # If we're passed in a specification, save out the model
        # and then load it back up
        elif isinstance(model, coremltools.proto.Model_pb2.Model):
            model_dir = tempfile.mkdtemp()
            model_name = str(uuid.uuid4()) + ".mlmodel"
            model_path = os.path.join(model_dir, model_name)
            coremltools.utils.save_spec(model, model_path)
            model = coremltools.models.MLModel(model, useCPUOnly=useCPUOnly)

        # If we want to test the half precision case
        if model_precision == _MLMODEL_HALF_PRECISION:
            model = coremltools.utils._convert_neural_network_weights_to_fp16(model)

        try:
            prediction = model.predict(input, useCPUOnly=useCPUOnly)
            for output_name in expected:
                if self.__class__.__name__ == "SimpleTest":
                    self._test_shape_equality(
                        expected[output_name], prediction[output_name]
                    )
                else:
                    if output_name in output_name_shape_dict:
                        output_shape = output_name_shape_dict[output_name]
                    else:
                        output_shape = []

                    if len(output_shape) == 0 and len(expected[output_name].shape) == 0:
                        output_shape = (1,)

                    self._test_nd_shape_equality(
                        expected[output_name], prediction[output_name], output_shape
                    )

                if not validate_shapes_only:
                    self._test_predictions(
                        expected[output_name],
                        prediction[output_name],
                        delta=delta,
                        test_metric=test_metric,
                        SNR=SNR,
                    )
        finally:
            # Remove the temporary directory if we created one
            if model_dir and os.path.exists(model_dir):
                shutil.rmtree(model_dir)


@unittest.skipIf(
    not _is_macos() or _macos_version() < MIN_MACOS_VERSION_REQUIRED,
    "macOS 10.13+ is required. Skipping tests.",
)
class SimpleTest(CorrectnessTest):
    def test_tiny_upsample_linear_mode(self):
        input_dim = (1, 1, 3)  # (C,H,W)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        builder.add_upsample(
            name="upsample",
            scaling_factor_h=2,
            scaling_factor_w=3,
            input_name="data",
            output_name="output",
            mode="BILINEAR",
        )

        input = {"data": np.reshape(np.array([1.0, 2.0, 3.0]), (1, 1, 3))}
        expected = {
            "output": np.array(
                [
                    [1, 1.333, 1.666, 2, 2.333, 2.666, 3, 3, 3],
                    [1, 1.333, 1.6666, 2, 2.33333, 2.6666, 3, 3, 3],
                ]
            )
        }

        self._test_model(builder.spec, input, expected)
        self.assertEquals(len(input_dim), builder._get_rank("output"))

    def test_LRN(self):
        input_dim = (1, 3, 3)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", datatypes.Array(*input_dim))]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        builder.add_lrn(
            name="lrn",
            input_name="data",
            output_name="output",
            alpha=2,
            beta=3,
            local_size=1,
            k=8,
        )

        input = {"data": np.ones((1, 3, 3))}
        expected = {"output": 1e-3 * np.ones((1, 3, 3))}

        self._test_model(builder.spec, input, expected)
        self.assertEqual(len(input_dim), builder._get_rank("output"))

    def test_MVN(self):
        input_dim = (2, 2, 2)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", datatypes.Array(*input_dim))]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        builder.add_mvn(
            name="mvn",
            input_name="data",
            output_name="output",
            across_channels=False,
            normalize_variance=False,
        )

        input = {"data": np.reshape(np.arange(8, dtype=np.float32), (2, 2, 2))}
        expected = {
            "output": np.reshape(
                np.arange(8) - np.array([1.5, 1.5, 1.5, 1.5, 5.5, 5.5, 5.5, 5.5]),
                (2, 2, 2),
            )
        }

        self._test_model(builder.spec, input, expected)

    def test_L2_normalize(self):
        input_dim = (1, 2, 2)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", datatypes.Array(*input_dim))]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        builder.add_l2_normalize(name="mvn", input_name="data", output_name="output")

        input = {"data": np.reshape(np.arange(4, dtype=np.float32), (1, 2, 2))}
        expected = {
            "output": np.reshape(np.arange(4, dtype=np.float32), (1, 2, 2))
            / np.sqrt(14)
        }

        self._test_model(builder.spec, input, expected)

    def test_unary_sqrt(self):
        x = np.reshape(np.arange(1, 5, dtype=np.float32), (1, 2, 2))
        input = {"data": x}
        expected = {"output": np.sqrt(x)}
        spec = _get_unary_model_spec(x, "sqrt")
        self._test_model(spec, input, expected)

    def test_unary_rsqrt(self):
        x = np.reshape(np.arange(1, 5, dtype=np.float32), (1, 2, 2))
        input = {"data": x}
        expected = {"output": 1 / np.sqrt(x)}
        spec = _get_unary_model_spec(x, "rsqrt")
        self._test_model(spec, input, expected)

    def test_unary_inverse(self):
        x = np.reshape(np.arange(1, 5, dtype=np.float32), (1, 2, 2))
        input = {"data": x}
        expected = {"output": 1 / x}
        spec = _get_unary_model_spec(x, "inverse")
        self._test_model(spec, input, expected)

    def test_unary_power(self):
        x = np.reshape(np.arange(1, 5, dtype=np.float32), (1, 2, 2))
        input = {"data": x}
        expected = {"output": x ** 3}
        spec = _get_unary_model_spec(x, "power", 3)
        self._test_model(spec, input, expected)

    def test_unary_exp(self):
        x = np.reshape(np.arange(1, 5, dtype=np.float32), (1, 2, 2))
        input = {"data": x}
        expected = {"output": np.exp(x)}
        spec = _get_unary_model_spec(x, "exp")
        self._test_model(spec, input, expected)

    def test_unary_log(self):
        x = np.reshape(np.arange(1, 5, dtype=np.float32), (1, 2, 2))
        input = {"data": x}
        expected = {"output": np.log(x)}
        spec = _get_unary_model_spec(x, "log")
        self._test_model(spec, input, expected)

    def test_unary_abs(self):
        x = np.reshape(np.arange(1, 5, dtype=np.float32), (1, 2, 2))
        input = {"data": x}
        expected = {"output": np.abs(x)}
        spec = _get_unary_model_spec(x, "abs")
        self._test_model(spec, input, expected)

    def test_unary_threshold(self):
        x = np.reshape(np.arange(1, 5, dtype=np.float32), (1, 2, 2))
        input = {"data": x}
        expected = {"output": np.maximum(x, 2)}
        spec = _get_unary_model_spec(x, "threshold", 2)
        self._test_model(spec, input, expected)

    def test_split(self):
        input_dim = (9, 2, 2)
        x = np.random.rand(*input_dim)

        input_features = [("data", datatypes.Array(*input_dim))]
        output_names = []
        output_features = []
        for i in range(3):
            out = "out_" + str(i)
            output_names.append(out)
            output_features.append((out, None))

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        builder.add_split(name="split", input_name="data", output_names=output_names)

        input = {"data": x}
        expected = {"out_0": x[0:3, :, :], "out_1": x[3:6, :, :], "out_2": x[6:9, :, :]}

        self._test_model(builder.spec, input, expected)
        for output_ in output_names:
            self.assertEqual(len(input_dim), builder._get_rank(output_))

    def test_scale_constant(self):
        input_dim = (1, 2, 2)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        builder.add_scale(
            name="scale",
            W=5,
            b=45,
            has_bias=True,
            input_name="data",
            output_name="output",
        )

        x = np.reshape(np.arange(4, dtype=np.float32), (1, 2, 2))
        input = {"data": x}
        expected = {"output": 5 * x + 45}

        self._test_model(builder.spec, input, expected)

    def test_scale_matrix(self):
        input_dim = (1, 2, 2)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        W = np.reshape(np.arange(5, 9), (1, 2, 2))

        builder.add_scale(
            name="scale",
            W=W,
            b=None,
            has_bias=False,
            input_name="data",
            output_name="output",
            shape_scale=[1, 2, 2],
        )

        x = np.reshape(np.arange(4, dtype=np.float32), (1, 2, 2))
        input = {"data": x}
        expected = {"output": W * x}

        self._test_model(builder.spec, input, expected)

    def test_bias_constant(self):
        input_dim = (1, 2, 2)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        builder.add_bias(name="bias", b=45, input_name="data", output_name="output")

        x = np.reshape(np.arange(4, dtype=np.float32), (1, 2, 2))
        input = {"data": x}
        expected = {"output": x + 45}

        self._test_model(builder.spec, input, expected)

    def test_bias_matrix(self):
        input_dim = (1, 2, 2)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        b = np.reshape(np.arange(5, 9), (1, 2, 2))

        builder.add_bias(
            name="bias",
            b=b,
            input_name="data",
            output_name="output",
            shape_bias=[1, 2, 2],
        )

        x = np.reshape(np.arange(4, dtype=np.float32), (1, 2, 2))
        input = {"data": x}
        expected = {"output": x + b}

        self._test_model(builder.spec, input, expected)

    def test_load_constant(self, model_precision=_MLMODEL_FULL_PRECISION):
        input_dim = (1, 2, 2)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        b = np.reshape(np.arange(5, 9), (1, 2, 2))

        builder.add_load_constant(
            name="load_constant", output_name="bias", constant_value=b, shape=[1, 2, 2]
        )
        builder.add_elementwise(
            name="add", input_names=["data", "bias"], output_name="output", mode="ADD"
        )

        x = np.reshape(np.arange(4, dtype=np.float32), (1, 2, 2))
        input = {"data": x}
        expected = {"output": x + b}

        self._test_model(builder.spec, input, expected, model_precision)
        self.assertEqual(len(input_dim), builder._get_rank("output"))

    def test_load_constant_half_precision(self):
        self.test_load_constant(model_precision=_MLMODEL_HALF_PRECISION)

    def test_min(self):
        input_dim = (1, 2, 2)
        input_features = [
            ("data_0", datatypes.Array(*input_dim)),
            ("data_1", datatypes.Array(*input_dim)),
        ]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        builder.add_elementwise(
            name="min",
            input_names=["data_0", "data_1"],
            output_name="output",
            mode="MIN",
        )
        x1 = np.reshape(np.arange(4, dtype=np.float32), (1, 2, 2))
        x2 = np.reshape(np.arange(2, 6, dtype=np.float32), (1, 2, 2))

        input = {"data_0": x1, "data_1": x2}
        expected = {"output": np.minimum(x1, x2)}

        self._test_model(builder.spec, input, expected)
        self.assertEqual(len(input_dim), builder._get_rank("output"))

    def test_conv_same_padding(self):
        input_dim = (10, 15, 15)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        W = np.random.rand(3, 3, 10, 20)

        builder.add_convolution(
            name="conv",
            kernel_channels=10,
            output_channels=20,
            height=3,
            width=3,
            stride_height=2,
            stride_width=2,
            border_mode="same",
            groups=1,
            W=W,
            b=None,
            has_bias=False,
            input_name="data",
            output_name="output",
            same_padding_asymmetry_mode="TOP_LEFT_HEAVY",
        )

        x = np.random.rand(*input_dim)
        input = {"data": x}
        expected = {"output": np.random.rand(20, 8, 8)}

        self._test_model(builder.spec, input, expected, validate_shapes_only=True)
        self.assertEqual(len(input_dim), builder._get_rank("output"))

    def test_deconv_valid_padding(self):
        input_dim = (10, 15, 15)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        W = np.random.rand(3, 3, 10, 20)

        builder.add_convolution(
            name="deconv",
            kernel_channels=10,
            output_channels=20,
            height=3,
            width=3,
            stride_height=2,
            stride_width=2,
            border_mode="valid",
            groups=1,
            W=W,
            b=None,
            has_bias=False,
            is_deconv=True,
            input_name="data",
            output_name="output",
            padding_top=2,
            padding_bottom=3,
            padding_left=2,
            padding_right=3,
        )

        x = np.random.rand(*input_dim)
        input = {"data": x}
        expected = {"output": np.random.rand(20, 26, 26)}

        self._test_model(builder.spec, input, expected, validate_shapes_only=True)

    def test_deconv_non_unit_groups(self):
        input_dim = (16, 15, 15)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        W = np.random.rand(3, 3, 16, 5)
        builder.add_convolution(
            name="deconv",
            kernel_channels=16,
            output_channels=20,
            height=3,
            width=3,
            stride_height=2,
            stride_width=2,
            border_mode="valid",
            groups=4,
            W=W,
            b=None,
            has_bias=False,
            is_deconv=True,
            input_name="data",
            output_name="output",
            padding_top=2,
            padding_bottom=3,
            padding_left=2,
            padding_right=3,
        )

        x = np.random.rand(*input_dim)
        input = {"data": x}
        expected = {"output": np.random.rand(20, 26, 26)}

        self._test_model(builder.spec, input, expected, validate_shapes_only=True)

    def test_linear_activation(self):
        input_dim = (10, 15, 15)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        builder.add_activation(
            name="activation",
            non_linearity="LINEAR",
            input_name="data",
            output_name="output",
            params=[34.0, 67.0],
        )

        x = np.random.rand(*input_dim)
        input = {"data": x}
        expected = {"output": 34.0 * x + 67.0}

        self._test_model(builder.spec, input, expected)

    def test_padding_constant(self):
        input_dim = (1, 2, 3)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        builder.add_padding(
            name="pad",
            left=1,
            right=0,
            top=2,
            bottom=0,
            value=-1,
            input_name="data",
            output_name="output",
        )

        x = np.reshape(np.array([[1, 2, 3], [4, 5, 6]]), (1, 2, 3)).astype(np.float32)
        input = {"data": x}
        y = np.reshape(
            np.array(
                [[-1, -1, -1, -1], [-1, -1, -1, -1], [-1, 1, 2, 3], [-1, 4, 5, 6]]
            ),
            (1, 4, 4),
        ).astype(np.float32)
        expected = {"output": y}

        self._test_model(builder.spec, input, expected)

    def test_padding_replication(self):
        input_dim = (1, 2, 3)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        builder.add_padding(
            name="pad",
            left=1,
            top=2,
            input_name="data",
            output_name="output",
            padding_type="replication",
        )

        x = np.reshape(np.array([[1, 2, 3], [4, 5, 6]]), (1, 2, 3)).astype(np.float32)
        input = {"data": x}
        y = np.reshape(
            np.array([[1, 1, 2, 3], [1, 1, 2, 3], [1, 1, 2, 3], [4, 4, 5, 6]]),
            (1, 4, 4),
        ).astype(np.float32)
        expected = {"output": y}

        self._test_model(builder.spec, input, expected)

    def test_reshape_target_shape_3(self):
        input_dim = (1, 2, 5)  # (C,H,W)
        target_dim = (10, 1, 1)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        builder.add_reshape(
            name="reshape",
            input_name="data",
            output_name="output",
            target_shape=target_dim,
            mode=0,
        )

        x = np.random.rand(*input_dim)
        input = {"data": x}
        expected = {"output": np.reshape(x, (10, 1, 1))}

        self._test_model(builder.spec, input, expected)
        self.assertEqual(len(target_dim), builder._get_rank("output"))

    def test_reshape_target_shape_4(self):
        input_dim = (1, 2, 5)  # (C,H,W)
        target_dim = (1, 10, 1, 1)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        builder.add_reshape(
            name="reshape",
            input_name="data",
            output_name="output",
            target_shape=target_dim,
            mode=0,
        )

        x = np.random.rand(*input_dim)
        input = {"data": x}
        expected = {"output": np.reshape(x, (1, 10, 1, 1))}

        self._test_model(builder.spec, input, expected)
        self.assertEqual(len(target_dim), builder._get_rank("output"))

    def test_bias_matrix_cpu(self):
        input_dim = (1, 2, 2)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        b = np.reshape(np.arange(5, 9), (1, 2, 2))

        builder.add_bias(
            name="bias",
            b=b,
            input_name="data",
            output_name="output",
            shape_bias=[1, 2, 2],
        )

        x = np.reshape(np.arange(4, dtype=np.float32), (1, 2, 2))
        input = {"data": x}
        expected = {"output": x + b}

        self._test_model(builder.spec, input, expected, useCPUOnly=True)

    def test_linear_activation_cpu(self):
        input_dim = (10, 15, 15)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        builder.add_activation(
            name="activation",
            non_linearity="LINEAR",
            input_name="data",
            output_name="output",
            params=[34.0, 67.0],
        )

        x = np.random.rand(*input_dim)
        input = {"data": x}
        expected = {"output": 34.0 * x + 67.0}

        self._test_model(builder.spec, input, expected, useCPUOnly=True)


@unittest.skipIf(
    not _is_macos() or _macos_version() < LAYERS_10_15_MACOS_VERSION,
    "macOS 10.15+ required. Skipping tests.",
)
class NewLayersSimpleTest(CorrectnessTest):
    def test_shape_flexibility_range(self):

        input_features = [("data", datatypes.Array(*(3, 4)))]
        builder = neural_network.NeuralNetworkBuilder(
            input_features, [("output", None)], disable_rank5_shape_mapping=True
        )
        builder.add_sin(name="sin", input_name="data", output_name="output")
        spec = builder.spec

        flexible_shape_utils.set_multiarray_ndshape_range(
            spec, feature_name="data", lower_bounds=[1, 1], upper_bounds=[-1, 5]
        )

        shapes = [(3, 4), (1, 5), (60, 5), (22, 4), (5, 3)]
        for s in shapes:
            x = np.random.rand(*s)
            expected = {"output": np.sin(x)}
            self._test_model(spec, {"data": x}, expected, useCPUOnly=True)

    def test_shape_flexibility_enumeration(self, rank=4):
        default_shape = tuple(np.random.randint(1, 15, size=rank))
        input_features = [("data", datatypes.Array(*default_shape))]
        builder = neural_network.NeuralNetworkBuilder(
            input_features=input_features,
            output_features=[("output", None)],
            disable_rank5_shape_mapping=True,
        )
        builder.add_sin(name="sin", input_name="data", output_name="output")
        spec = builder.spec

        shapes = [
            tuple(np.random.randint(1, 15, size=rank)),
            tuple(np.random.randint(1, 15, size=rank)),
        ]
        flexible_shape_utils.add_multiarray_ndshape_enumeration(
            spec, feature_name="data", enumerated_shapes=shapes
        )

        shapes.append(default_shape)
        for s in shapes:
            x = np.random.rand(*s)
            expected = {"output": np.sin(x)}
            self._test_model(spec, {"data": x}, expected, useCPUOnly=True)

    def test_shape_flexibility_enumeration_rank3(self):
        self.test_shape_flexibility_enumeration(rank=3)

    def test_shape_flexibility_enumeration_rank2(self):
        self.test_shape_flexibility_enumeration(rank=2)

    def test_transpose_cpu(self):
        for rank in range(1, 6):
            axes = np.random.permutation(rank)
            axes = [
                axis - rank if np.random.choice([True, False]) else axis
                for axis in axes
            ]
            input_shape = np.random.randint(low=2, high=6, size=rank)
            input_features = [("data", datatypes.Array(*input_shape))]
            output_features = [("output", None)]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )

            builder.add_transpose(
                name="TransposeND", axes=axes, input_name="data", output_name="output"
            )

            x = np.random.rand(*input_shape)
            input = {"data": x}
            expected = {"output": np.transpose(x, axes)}

            self._test_model(builder.spec, input, expected, useCPUOnly=True)

    def test_dynamic_weight_conv(self):

        input_dim = (1, 3, 16, 16)
        # weight layout: (output_channels, kernel_channels, height, width)
        weight_dim = (4, 3, 3, 3)
        output_dim = (1, 4, 14, 14)

        kernel_channels = input_dim[0]
        output_channels, kernel_channels, height, width = weight_dim

        input_features = [
            ("input", datatypes.Array(*input_dim)),
            ("weight", datatypes.Array(*weight_dim)),
        ]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )

        builder.add_convolution(
            name="two_input_conv_layer",
            kernel_channels=kernel_channels,
            output_channels=output_channels,
            height=height,
            width=width,
            stride_height=1,
            stride_width=1,
            border_mode="valid",
            groups=1,
            W=None,
            b=None,
            has_bias=False,
            input_name=["input", "weight"],
            output_name="output",
        )

        # Assigning everything to ones should cover the execution path
        # and engine failures, but is not a complete check on numerics.
        input_val = np.ones(input_dim)
        weight_val = np.ones(weight_dim)
        expected = np.ones(output_dim) * 27

        feed_dict = {"input": input_val, "weight": weight_val}
        expected = {"output": expected}

        self._test_model(builder.spec, feed_dict, expected, useCPUOnly=True)
        self._test_model(builder.spec, feed_dict, expected, useCPUOnly=False)

    @pytest.mark.xfail
    def test_dynamic_weight_deconv(self):
        # Expect to fail in Core ML 3
        input_dim = (1, 1, 16, 16)
        # weight layout: (output_channels, kernel_channels, height, width)
        weight_dim = (1, 1, 3, 3)
        output_dim = (1, 1, 18, 18)
        output_channels, kernel_channels, height, width = weight_dim

        input_features = [
            ("data", datatypes.Array(*input_dim)),
            ("weight", datatypes.Array(*weight_dim)),
        ]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )

        builder.add_convolution(
            name="deconv",
            kernel_channels=kernel_channels,
            output_channels=output_channels,
            height=height,
            width=width,
            stride_height=1,
            stride_width=1,
            border_mode="valid",
            groups=1,
            W=None,
            b=None,
            has_bias=False,
            is_deconv=True,
            input_name=["data", "weight"],
            output_name="output",
        )

        input_val = np.ones(input_dim)
        weight_val = np.ones(weight_dim)
        expected = np.ones(output_dim) * 27

        feed_dict = {"data": input_val, "weight": weight_val}
        expected = {"output": expected}

        self._test_model(builder.spec, feed_dict, expected)

    def test_batched_mat_mul_cpu(self, cpu_only=True):
        a_shapes = [
            (10,),
            (4, 10),
            (10,),
            (10,),
            (2, 3),
            (1, 3, 4),
            (1, 3, 1, 2, 3),
            (2, 3, 1, 3, 4),
        ]
        b_shapes = [
            (10,),
            (10,),
            (10, 3),
            (2, 10, 3),
            (3, 4),
            (3, 2, 4, 5),
            (1, 4, 3, 2),
            (2, 1, 2, 4, 5),
        ]
        out_shapes = [
            (1, 1),
            (4, 1),
            (1, 3),
            (2, 1, 3),
            (2, 4),
            (3, 2, 3, 5),
            (1, 3, 4, 2, 2),
            (2, 3, 2, 3, 5),
        ]

        for a_shape, b_shape, outShape in zip(a_shapes, b_shapes, out_shapes):
            input_shapes = [a_shape, b_shape]
            input_features = [
                ("A", datatypes.Array(*input_shapes[0])),
                ("B", datatypes.Array(*input_shapes[1])),
            ]
            output_features = [("output", None)]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )

            builder.add_batched_mat_mul(
                name="batched_mat_mul",
                input_names=["A", "B"],
                output_name="output",
                transpose_a=False,
                transpose_b=False,
            )

            a = np.random.rand(*input_shapes[0])
            b = np.random.rand(*input_shapes[1])
            input_ = {"A": a, "B": b}
            expected = {"output": np.array(np.matmul(a, b))}
            shape_dict = {"output": outShape}
            self._test_model(
                builder.spec,
                input_,
                expected,
                useCPUOnly=cpu_only,
                output_name_shape_dict=shape_dict,
            )
            self.assertEqual(len(outShape), builder._get_rank("output"))

    def test_batched_mat_mul_gpu(self):
        self.test_batched_mat_mul_cpu(cpu_only=False)

    def test_batched_mat_mul_with_transposes_cpu(self, cpu_only=True):
        for transpose_a, transpose_b in itertools.product([True, False], [True, False]):
            a_shape = (3, 4)
            b_shape = (4, 5)
            a_shape = a_shape[::-1] if transpose_a else a_shape
            b_shape = b_shape[::-1] if transpose_b else b_shape
            input_shapes = [a_shape, b_shape]
            input_features = [
                ("A", datatypes.Array(*input_shapes[0])),
                ("B", datatypes.Array(*input_shapes[1])),
            ]

            output_features = [("output", None)]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )
            builder.add_batched_mat_mul(
                name="BatchedMatMul",
                input_names=["A", "B"],
                output_name="output",
                transpose_a=transpose_a,
                transpose_b=transpose_b,
            )
            a = np.random.rand(*input_shapes[0])
            b = np.random.rand(*input_shapes[1])
            inputs = {"A": a, "B": b}
            a = a.T if transpose_a else a
            b = b.T if transpose_b else b
            expected = {"output": np.matmul(a, b)}

            self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)

    def test_batched_mat_mul_with_transposes_gpu(self):
        self.test_batched_mat_mul_with_transposes_cpu(cpu_only=False)

    def test_batched_mat_mul_single_input_cpu(
        self, model_precision=_MLMODEL_FULL_PRECISION, cpu_only=True
    ):
        X1 = 11
        X2 = 23
        W = np.random.rand(X1, X2)
        bias = np.random.rand(X2)
        input_shapes = [
            (X1,),
            (5, X1),
            (2, 3, X1),
            (4, 1, X1),
            (12, 5, 8, X1),
            (2, 3, 1, 5, X1),
        ]
        for input_shape in input_shapes:
            x = np.random.rand(*input_shape)
            np_out = np.matmul(x, W) + bias
            expected = {"output": np_out}

            input_features = [("data", datatypes.Array(*input_shape))]
            output_features = [("output", None)]
            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )

            builder.add_batched_mat_mul(
                name="batched_mat_mul",
                input_names=["data"],
                output_name="output",
                weight_matrix_rows=X1,
                weight_matrix_columns=X2,
                W=W,
                bias=bias,
            )
            inputs = {"data": x}

            self._test_model(
                builder.spec,
                inputs,
                expected,
                model_precision=model_precision,
                useCPUOnly=cpu_only,
            )

    def test_batched_mat_mul_single_input_half_precision_cpu(self):
        self.test_batched_mat_mul_single_input_cpu(
            model_precision=_MLMODEL_HALF_PRECISION, cpu_only=True
        )

    def test_batched_mat_mul_single_input_gpu(self):
        self.test_batched_mat_mul_single_input_cpu(
            model_precision=_MLMODEL_FULL_PRECISION, cpu_only=False
        )

    def test_embedding_nd_cpu(
        self, model_precision=_MLMODEL_FULL_PRECISION, use_cpu_only=True
    ):
        vocab_size = 10
        embedding_size = 19
        W = np.random.rand(embedding_size, vocab_size)
        input_shapes = [(5, 1), (2, 3, 1), (4, 1, 1), (12, 5, 8, 1), (2, 3, 1, 5, 1)]
        for input_shape in input_shapes:
            x = np.random.randint(vocab_size, size=input_shape)

            np_out = np.take(np.transpose(W), np.squeeze(x, axis=-1), axis=0)
            expected = {"output": np_out}

            input_features = [("data", datatypes.Array(*input_shape))]
            output_features = [("output", None)]
            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )

            builder.add_embedding_nd(
                name="embedding_nd",
                input_name="data",
                output_name="output",
                vocab_size=vocab_size,
                embedding_size=embedding_size,
                W=W,
            )

            input = {"data": x.astype(np.float32)}

            self._test_model(
                builder.spec,
                input,
                expected,
                model_precision=model_precision,
                useCPUOnly=use_cpu_only,
            )

    def test_embedding_nd_half_precision_cpu(self):
        self.test_embedding_nd_cpu(
            model_precision=_MLMODEL_HALF_PRECISION, use_cpu_only=True
        )

    def test_embedding_nd_GPU(self):
        self.test_embedding_nd_cpu(
            model_precision=_MLMODEL_FULL_PRECISION, use_cpu_only=False
        )

    def test_embedding_nd_half_precision_GPU(self):
        self.test_embedding_nd_cpu(
            model_precision=_MLMODEL_HALF_PRECISION, use_cpu_only=False
        )

    def test_softmax_nan_bug_cpu(self):
        input_shape = [2, 2]
        input_features = [("data", datatypes.Array(*input_shape))]
        output_features = [("output", None)]
        for axis in [0, 1]:
            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )

            builder.add_softmax_nd(
                name="softmax_nd", input_name="data", output_name="output", axis=axis
            )

            x = np.array([[0.5, 0.5], [1e8, 1e8]])
            input = {"data": x}
            y = np.exp(x - np.max(x, axis=axis, keepdims=True))
            y = y / np.sum(y, axis=axis, keepdims=True)
            expected = {"output": y}

            self._test_model(builder.spec, input, expected, useCPUOnly=True)

    def test_softmax_nd_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            for axis in range(-rank, rank):
                input_shape = np.random.randint(low=2, high=5, size=rank)
                input_features = [("data", datatypes.Array(*input_shape))]
                output_features = [("output", None)]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_softmax_nd(
                    name="softmax_nd",
                    input_name="data",
                    output_name="output",
                    axis=axis,
                )

                x = np.random.rand(*input_shape)
                input = {"data": x}
                y = np.exp(x - np.max(x, axis=axis, keepdims=True))
                y = y / np.sum(y, axis=axis, keepdims=True)
                expected = {"output": y}

                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_softmax_nd_gpu(self):
        self.test_softmax_nd_cpu(cpu_only=False)

    def test_concat_nd_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            for axis in range(-rank, rank):
                n_inputs = np.random.choice(range(2, 5))
                output_shape = np.random.randint(low=2, high=5, size=rank)
                output_shape[axis] = 0
                input_shapes = []
                input_features = []
                input_names = []
                for _ in range(n_inputs):
                    input_shapes.append(np.copy(output_shape))
                    input_shapes[-1][axis] = np.random.choice(range(2, 8))
                    output_shape[axis] += input_shapes[-1][axis]
                for i, input_dim in enumerate(input_shapes):
                    input_name = "input_%s" % str(i)
                    input_names.append(input_name)
                    input_features.append((input_name, datatypes.Array(*input_dim)))

                output_features = [("output", None)]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_concat_nd(
                    name="concat_nd",
                    input_names=input_names,
                    output_name="output",
                    axis=axis,
                )

                input_tensors = []
                for input_dim in input_shapes:
                    input_tensors.append(np.random.rand(*input_dim))
                input = dict(zip(input_names, input_tensors))
                expected = {"output": np.concatenate(input_tensors, axis)}

                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_concat_nd_gpu(self):
        self.test_concat_nd_cpu(cpu_only=False)

    def test_fill_like_cpu(self, cpu_only=True):

        for rank in range(1, 6):
            target_shape = np.random.randint(low=2, high=6, size=rank)
            value = float(np.random.rand())

            input_features = [("tensor", datatypes.Array(*target_shape))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_fill_like(
                name="fill_like", input_name="tensor", output_name="output", value=value
            )

            tensor = np.random.rand(*target_shape)
            input = {"tensor": tensor}
            expected = {"output": np.zeros(target_shape) + value}

            self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_fill_like_gpu(self):
        self.test_fill_like_cpu(cpu_only=False)

    def test_fill_static_cpu(self, cpu_only=True):

        for rank in range(1, 6):
            shape = np.random.randint(low=2, high=8, size=rank)

            input_features = [("data", datatypes.Array(*shape))]
            value = float(np.random.rand())

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )
            builder.add_fill_static(
                name="fill_static",
                output_name="tmp",
                output_shape=list(shape),
                value=value,
            )

            builder.add_elementwise("add_layer", ["data", "tmp"], "output", mode="ADD")

            data = np.random.rand(*shape)
            input = {"data": data}
            expected = {"output": data + value}

            self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)
            self.assertEqual(len(shape), builder._get_rank("output"))

    def test_fill_static_gpu(self):
        self.test_fill_static_cpu(cpu_only=False)

    def test_fill_dynamic_cpu(self, cpu_only=True):

        for rank in range(1, 6):
            input_shape = np.random.randint(low=2, high=8, size=rank)
            value = float(np.random.rand())

            input_features = [("shape", datatypes.Array(len(input_shape)))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_fill_dynamic(
                name="fill_dynamic",
                input_name="shape",
                output_name="output",
                value=value,
            )

            input = {"shape": np.array(input_shape, dtype="float")}
            expected = {"output": np.zeros(input_shape) + value}

            self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)
            self.assertEqual(builder._get_rank("output"), -1)

    def test_fill_dynamic_gpu(self):
        self.test_fill_dynamic_cpu(cpu_only=False)

    def test_broadcast_to_like_cpu(self, cpu_only=True):

        for rank in range(1, 6):
            input_shape = np.random.randint(low=2, high=8, size=rank)
            mask = [np.random.choice([True, False, False]) for _ in range(rank)]
            input_shape = np.where(mask, 1, input_shape)

            target_rank = np.random.randint(low=rank, high=6)
            target_shape = [
                np.random.randint(low=2, high=8)
                if (-i > rank or input_shape[i] == 1)
                else input_shape[i]
                for i in range(-1, -target_rank - 1, -1)
            ][::-1]

            input_features = [
                ("data", datatypes.Array(*input_shape)),
                ("tensor", datatypes.Array(*target_shape)),
            ]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_broadcast_to_like(
                name="broadcast_to_like",
                input_names=["data", "tensor"],
                output_name="output",
            )

            data = np.random.rand(*input_shape)
            tensor = np.random.rand(*target_shape)
            inputs = {"data": data, "tensor": tensor}
            expected = {"output": np.broadcast_to(data, target_shape)}

            self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)

    def test_broadcast_to_like_gpu(self):
        self.test_broadcast_to_like_cpu(cpu_only=False)

    def test_broadcast_to_static_cpu(self, cpu_only=True):

        for rank in range(1, 6):
            input_shape = np.random.randint(low=2, high=8, size=rank)
            mask = [np.random.choice([True, False, False]) for _ in range(rank)]
            input_shape = np.where(mask, 1, input_shape)

            target_rank = np.random.randint(low=rank, high=6)
            target_shape = [
                np.random.randint(low=2, high=8)
                if (-i > rank or input_shape[i] == 1)
                else input_shape[i]
                for i in range(-1, -target_rank - 1, -1)
            ][::-1]

            input_features = [("data", datatypes.Array(*input_shape))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_broadcast_to_static(
                name="broadcast_to_static",
                input_name="data",
                output_name="output",
                output_shape=list(target_shape),
            )

            data = np.random.rand(*input_shape)
            input = {"data": data}
            expected = {"output": np.broadcast_to(data, target_shape)}

            self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)
            self.assertEqual(target_rank, builder._get_rank("output"))

    def test_broadcast_to_static_gpu(self):
        self.test_broadcast_to_static_cpu(cpu_only=False)

    def test_broadcast_to_dynamic_cpu(self, cpu_only=True):

        for rank in range(1, 6):
            input_shape = np.random.randint(low=2, high=8, size=rank)
            mask = [np.random.choice([True, False, False]) for _ in range(rank)]
            input_shape = np.where(mask, 1, input_shape)

            target_rank = np.random.randint(low=rank, high=6)
            target_shape = [
                np.random.randint(low=2, high=8)
                if (-i > rank or input_shape[i] == 1)
                else input_shape[i]
                for i in range(-1, -target_rank - 1, -1)
            ][::-1]

            input_features = [
                ("data", datatypes.Array(*input_shape)),
                ("shape", datatypes.Array(len(target_shape))),
            ]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_broadcast_to_dynamic(
                name="broadcast_to_dynamic",
                input_names=["data", "shape"],
                output_name="output",
            )

            data = np.random.rand(*input_shape)
            inputs = {"data": data, "shape": np.array(target_shape, dtype="float")}
            expected = {"output": np.broadcast_to(data, target_shape)}

            self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)
            self.assertEqual(builder._get_rank("output"), -1)

    def test_broadcast_to_dynamic_gpu(self):
        self.test_broadcast_to_dynamic_cpu(cpu_only=False)

    # Test Rank being set to unknown when one of the input rank is unknown
    # For max rank case
    def test_unknown_rank(self, cpu_only=True):

        for rank in range(1, 6):
            input_shape = np.random.randint(low=2, high=8, size=rank)
            mask = [np.random.choice([True, False, False]) for _ in range(rank)]
            input_shape = np.where(mask, 1, input_shape)

            target_rank = np.random.randint(low=rank, high=6)
            target_shape = [
                np.random.randint(low=2, high=8)
                if (-i > rank or input_shape[i] == 1)
                else input_shape[i]
                for i in range(-1, -target_rank - 1, -1)
            ][::-1]

            input_features = [
                ("x", datatypes.Array(*input_shape)),
                ("shape", datatypes.Array(len(target_shape))),
            ]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_broadcast_to_dynamic(
                name="broadcast_to_dynamic", input_names=["x", "shape"], output_name="y"
            )

            condition = np.random.randint(0, 2, input_shape).astype(np.float32)
            builder.add_load_constant_nd(
                name="load_constant_condition",
                output_name="condition",
                constant_value=condition,
                shape=input_shape,
            )

            builder.add_where_broadcastable(
                name="where", input_names=["condition", "x", "y"], output_name="output"
            )

            self.assertEqual(builder._get_rank("output"), -1)

    def test_trigonometry_cpu(self, cpu_only=True):

        ops = [
            "sin",
            "cos",
            "tan",
            "asin",
            "acos",
            "atan",
            "sinh",
            "cosh",
            "tanh",
            "asinh",
            "acosh",
            "atanh",
        ]

        for op in ops:
            for rank in range(1, 6):
                shape = np.random.randint(low=2, high=8, size=rank)
                input_features = [("data", datatypes.Array(*shape))]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, [("output", None)], disable_rank5_shape_mapping=True
                )

                x = np.random.rand(*shape)

                if op == "sin":
                    builder.add_sin(name=op, input_name="data", output_name="output")
                    expected = {"output": np.sin(x)}
                elif op == "cos":
                    builder.add_cos(name=op, input_name="data", output_name="output")
                    expected = {"output": np.cos(x)}
                elif op == "tan":
                    builder.add_tan(name=op, input_name="data", output_name="output")
                    expected = {"output": np.tan(x)}
                elif op == "asin":
                    builder.add_asin(name=op, input_name="data", output_name="output")
                    expected = {"output": np.arcsin(x)}
                elif op == "acos":
                    builder.add_acos(name=op, input_name="data", output_name="output")
                    expected = {"output": np.arccos(x)}
                elif op == "atan":
                    builder.add_atan(name=op, input_name="data", output_name="output")
                    expected = {"output": np.arctan(x)}
                elif op == "sinh":
                    builder.add_sinh(name=op, input_name="data", output_name="output")
                    expected = {"output": np.sinh(x)}
                elif op == "cosh":
                    builder.add_cosh(name=op, input_name="data", output_name="output")
                    expected = {"output": np.cosh(x)}
                elif op == "tanh":
                    builder.add_tanh(name=op, input_name="data", output_name="output")
                    expected = {"output": np.tanh(x)}
                elif op == "asinh":
                    builder.add_asinh(name=op, input_name="data", output_name="output")
                    expected = {"output": np.arcsinh(x)}
                elif op == "acosh":
                    x = np.random.choice([10, np.e, 1], tuple(shape)).astype(np.float32)
                    builder.add_acosh(name=op, input_name="data", output_name="output")
                    expected = {"output": np.arccosh(x)}
                elif op == "atanh":
                    builder.add_atanh(name=op, input_name="data", output_name="output")
                    expected = {"output": np.arctanh(x)}

                self._test_model(
                    builder.spec, {"data": x}, expected, useCPUOnly=cpu_only
                )

    def test_trigonometry_gpu(self):
        self.test_trigonometry_cpu(cpu_only=False)

    def test_exp2_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            shape = np.random.randint(low=2, high=8, size=rank)
            input_features = [("data", datatypes.Array(*shape))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )
            builder.add_exp2(name="exp2", input_name="data", output_name="output")

            x = np.random.rand(*shape)
            input = {"data": x}
            expected = {"output": np.exp2(x)}

            self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_exp2_gpu(self):
        self.test_exp2_cpu(cpu_only=False)

    def test_elementwise_binary_cpu(self, cpu_only=True):
        input_names = ["A", "B"]
        test_cases = [
            "greater",
            "less",
            "equal",
            "not_equal",
            "greater_equal",
            "less_equal",
            "logical_and",
            "logical_or",
            "logical_xor",
            "add",
            "subtract",
            "multiply",
            "divide",
            "power",
            "maximum",
            "minimum",
            "floor_divide",
            "mod",
        ]
        for test_case in test_cases:
            for _ in range(10):
                rank_a = np.random.randint(low=1, high=6)
                rank_b = np.random.randint(low=1, high=6)

                rank_out = max(rank_a, rank_b)

                shape_a = np.random.randint(low=2, high=8, size=rank_a)
                shape_b = np.random.randint(low=2, high=8, size=rank_b)

                for i in range(-1, -rank_out - 1, -1):
                    dims = []
                    if -i <= rank_a:
                        dims.append(shape_a[i])
                    if -i <= rank_b:
                        dims.append(shape_b[i])

                    dim = np.random.choice(dims)
                    if -i <= rank_a:
                        shape_a[i] = np.random.choice([1, dim])
                    if -i <= rank_b:
                        shape_b[i] = np.random.choice([1, dim])

                input_shapes = [shape_a, shape_b]
                input_features = [
                    ("A", datatypes.Array(*input_shapes[0])),
                    ("B", datatypes.Array(*input_shapes[1])),
                ]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, [("output", None)], disable_rank5_shape_mapping=True
                )

                func = getattr(np, test_case)
                if test_case == "greater":
                    builder.add_greater_than(
                        test_case, input_names=input_names, output_name="output"
                    )
                elif test_case == "less":
                    builder.add_less_than(
                        test_case, input_names=input_names, output_name="output"
                    )
                elif test_case == "equal":
                    builder.add_equal(
                        test_case, input_names=input_names, output_name="output"
                    )
                elif test_case == "not_equal":
                    builder.add_not_equal(
                        test_case, input_names=input_names, output_name="output"
                    )
                elif test_case == "greater_equal":
                    builder.add_greater_than(
                        test_case,
                        input_names=input_names,
                        output_name="output",
                        use_greater_than_equal=True,
                    )
                elif test_case == "less_equal":
                    builder.add_less_than(
                        test_case,
                        input_names=input_names,
                        output_name="output",
                        use_less_than_equal=True,
                    )
                elif test_case == "logical_and":
                    builder.add_logical(
                        test_case,
                        input_names=input_names,
                        output_name="output",
                        mode="AND",
                    )
                elif test_case == "logical_or":
                    builder.add_logical(
                        test_case,
                        input_names=input_names,
                        output_name="output",
                        mode="OR",
                    )
                elif test_case == "logical_xor":
                    builder.add_logical(
                        test_case,
                        input_names=input_names,
                        output_name="output",
                        mode="XOR",
                    )
                elif test_case == "add":
                    builder.add_add_broadcastable(
                        test_case, input_names=input_names, output_name="output"
                    )
                elif test_case == "subtract":
                    builder.add_subtract_broadcastable(
                        test_case, input_names=input_names, output_name="output"
                    )
                elif test_case == "multiply":
                    builder.add_multiply_broadcastable(
                        test_case, input_names=input_names, output_name="output"
                    )
                elif test_case == "divide":
                    builder.add_divide_broadcastable(
                        test_case, input_names=input_names, output_name="output"
                    )
                elif test_case == "power":
                    builder.add_pow_broadcastable(
                        test_case, input_names=input_names, output_name="output"
                    )
                elif test_case == "maximum":
                    builder.add_max_broadcastable(
                        test_case, input_names=input_names, output_name="output"
                    )
                elif test_case == "minimum":
                    builder.add_min_broadcastable(
                        test_case, input_names=input_names, output_name="output"
                    )
                elif test_case == "floor_divide":
                    builder.add_floor_div_broadcastable(
                        test_case, input_names=input_names, output_name="output"
                    )
                elif test_case == "mod":
                    builder.add_mod_broadcastable(
                        test_case, input_names=input_names, output_name="output"
                    )
                a = np.random.rand(*input_shapes[0])
                b = np.random.rand(*input_shapes[1])
                input = {"A": a, "B": b}
                expected = {"output": func(a, b, dtype=np.float32)}
                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_elementwise_binary_gpu(self):
        self.test_elementwise_binary_cpu(cpu_only=False)

    def test_elementwise_boolean_unary_cpu(self, cpu_only=True):
        input_names = ["input"]
        shapes = [
            (1, 2, 3, 1),
            (3, 1, 2, 1, 2),
            (1, 2, 1, 3),
            (2, 3),
            (2, 1, 1),
            (2, 3, 4),
            (2, 4),
            (1,),
            (1,),
        ]
        test_cases = [
            "greater",
            "less",
            "equal",
            "not_equal",
            "greater_equal",
            "less_equal",
        ]
        for test_case in test_cases:
            for shape in shapes:
                input_features = [("input", datatypes.Array(*shape))]
                b = np.random.rand()
                builder = neural_network.NeuralNetworkBuilder(
                    input_features, [("output", None)], disable_rank5_shape_mapping=True
                )

                func = getattr(np, test_case)
                if test_case == "greater":
                    builder.add_greater_than(
                        test_case,
                        input_names=input_names,
                        output_name="output",
                        alpha=b,
                    )
                elif test_case == "less":
                    builder.add_less_than(
                        test_case,
                        input_names=input_names,
                        output_name="output",
                        alpha=b,
                    )
                elif test_case == "equal":
                    builder.add_equal(
                        test_case,
                        input_names=input_names,
                        output_name="output",
                        alpha=b,
                    )
                elif test_case == "not_equal":
                    builder.add_not_equal(
                        test_case,
                        input_names=input_names,
                        output_name="output",
                        alpha=b,
                    )
                elif test_case == "greater_equal":
                    builder.add_greater_than(
                        test_case,
                        input_names=input_names,
                        output_name="output",
                        use_greater_than_equal=True,
                        alpha=b,
                    )
                elif test_case == "less_equal":
                    builder.add_less_than(
                        test_case,
                        input_names=input_names,
                        output_name="output",
                        use_less_than_equal=True,
                        alpha=b,
                    )

                a = np.random.rand(*shape)
                input = {"input": a}
                expected = {"output": func(a, b, dtype=np.float32)}

                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_elementwise_boolean_unary_gpu(self):
        self.test_elementwise_boolean_unary_cpu(cpu_only=False)

    def test_logical_not_cpu(self, cpu_only=True):
        input_names = ["input"]
        shapes = [
            (1, 2, 3, 1),
            (3, 1, 2, 1, 2),
            (1, 2, 1, 3),
            (2, 3),
            (2, 1, 1),
            (2, 3, 4),
            (2, 4),
            (1,),
            (1,),
        ]
        for shape in shapes:
            input_features = [("input", datatypes.Array(*shape))]
            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )
            builder.add_logical(
                "logical_not", input_names=input_names, output_name="output", mode="NOT"
            )

            a = np.random.rand(*shape)
            input = {"input": a}
            expected = {"output": np.logical_not(a)}

            self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_logical_not_gpu(self):
        self.test_logical_not_cpu(cpu_only=False)

    def test_stack_cpu(self, cpu_only=True):
        for input_rank in range(1, 5):
            for axis in range(-input_rank - 1, input_rank + 1):
                n_inputs = np.random.choice(range(2, 5))
                input_shape = np.random.randint(low=2, high=5, size=input_rank)
                input_features = []
                input_names = []
                for i in range(n_inputs):
                    input_name = "input_%s" % str(i)
                    input_names.append(input_name)
                    input_features.append((input_name, datatypes.Array(*input_shape)))
                output_features = [("output", None)]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_stack(
                    name="stack",
                    input_names=input_names,
                    output_name="output",
                    axis=axis,
                )

                input_tensors = []
                for _ in range(n_inputs):
                    input_tensors.append(np.random.rand(*input_shape))
                input = dict(zip(input_names, input_tensors))
                expected = {"output": np.stack(input_tensors, axis)}

                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)
                self.assertEqual(input_rank + 1, builder._get_rank("output"))

    def test_stack_gpu(self):
        self.test_stack_cpu(cpu_only=False)

    def test_ceil_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            shape = np.random.randint(low=2, high=8, size=rank)
            input_features = [("data", datatypes.Array(*shape))]
            output_features = [("output", datatypes.Array(*shape))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )

            builder.add_ceil(name="ceil", input_name="data", output_name="output")

            x = np.random.rand(*shape)
            inputs = {"data": x}
            expected = {"output": np.ceil(x)}

            self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)
            self.assertEqual(rank, builder._get_rank("output"))

    def test_ceil_gpu(self):
        self.test_ceil_cpu(cpu_only=False)

    def test_floor_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            shape = np.random.randint(low=2, high=8, size=rank)
            input_features = [("data", datatypes.Array(*shape))]
            output_features = [("output", datatypes.Array(*shape))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )

            builder.add_floor(name="floor", input_name="data", output_name="output")

            x = np.random.rand(*shape)
            inputs = {"data": x}
            expected = {"output": np.floor(x)}

            self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)

    @pytest.mark.xfail(reason="[GitLab CI failure: test_floor_gpu](rdar://64311149)")
    def test_floor_gpu(self):
        self.test_floor_cpu(cpu_only=False)

    def test_round_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            shape = np.random.randint(low=2, high=8, size=rank)
            input_features = [("data", datatypes.Array(*shape))]
            output_features = [("output", datatypes.Array(*shape))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )

            builder.add_round(name="round", input_name="data", output_name="output")

            x = np.float32(
                np.random.rand(*shape) * np.random.randint(low=-100, high=101)
            )
            inputs = {"data": x}
            expected = {"output": np.around(x)}

            self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)

    def test_round_gpu(self):
        self.test_round_cpu(cpu_only=False)

    def test_sign_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            shape = np.random.randint(low=2, high=8, size=rank)
            input_features = [("data", datatypes.Array(*shape))]
            output_features = [("output", datatypes.Array(*shape))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )

            builder.add_sign(name="sign", input_name="data", output_name="output")

            x = np.random.choice(
                [-np.random.rand(1), 0.0, np.random.rand(1)], tuple(shape)
            ).astype(np.float32)
            inputs = {"data": x}
            expected = {"output": np.sign(x)}

            self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)

    def test_sign_gpu(self):
        self.test_sign_cpu(cpu_only=False)

    def test_clip_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            shape = np.random.randint(low=2, high=6, size=rank)
            input_features = [("data", datatypes.Array(*shape))]
            output_features = [("output", datatypes.Array(*shape))]

            x = np.random.rand(*shape)
            min_value = np.percentile(x, 25)
            max_value = np.percentile(x, 75)
            input = {"data": x}

            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )
            builder.add_clip(
                name="clip",
                input_name="data",
                output_name="output",
                min_value=min_value,
                max_value=max_value,
            )

            expected = {"output": np.clip(x, min_value, max_value)}
            self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_clip_gpu(self):
        self.test_clip_cpu(cpu_only=False)

    def test_split_nd_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            for axis in range(-rank, rank):
                n_outputs = np.random.choice(range(2, 4))
                input_shape = np.random.randint(low=2, high=5, size=rank)
                input_shape[axis] = 0
                output_shapes = []
                output_features = []
                output_names = []
                almost_equal = random.choice([True, False])
                remainder = np.random.choice(range(1, n_outputs)) if almost_equal else 0
                value = np.random.choice(range(2, 5))
                for k in range(n_outputs):
                    output_shapes.append(np.copy(input_shape))
                    output_shapes[-1][axis] = value + 1 if k < remainder else value
                    input_shape[axis] += output_shapes[-1][axis]

                for i in range(n_outputs):
                    output_name = "output_%s" % str(i)
                    output_names.append(output_name)
                    output_features.append((output_name, None))

                input_features = [("data", datatypes.Array(*input_shape))]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_split_nd(
                    name="split_nd",
                    input_name="data",
                    output_names=output_names,
                    axis=axis,
                    num_splits=n_outputs,
                )

                x = np.random.rand(*input_shape)
                input = {"data": x}
                expected = dict(
                    zip(
                        output_names,
                        np.array_split(x, n_outputs, axis=axis)
                        if almost_equal
                        else np.split(x, n_outputs, axis=axis),
                    )
                )  # Explicitly trying to compare against both versions of numpy split

                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)
                for output_ in output_names:
                    self.assertEqual(rank, builder._get_rank(output_))

    def test_split_nd_gpu(self):
        self.test_split_nd_cpu(cpu_only=False)

    def test_split_nd_with_split_sizes_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            for axis in range(-rank, rank):
                n_outputs = np.random.choice(range(2, 4))
                input_shape = np.random.randint(low=2, high=5, size=rank)
                input_shape[axis] = 0
                output_shapes, output_features, output_names = [], [], []
                sections, split_sizes = [], []
                for _ in range(n_outputs):
                    output_shapes.append(np.copy(input_shape))
                    output_shapes[-1][axis] = np.random.choice(range(2, 5))
                    input_shape[axis] += output_shapes[-1][axis]
                    sections.append(input_shape[axis])
                    split_sizes.append(output_shapes[-1][axis])

                sections.pop()
                for i in range(n_outputs):
                    output_name = "output_%s" % str(i)
                    output_names.append(output_name)
                    output_features.append((output_name, None))

                input_features = [("data", datatypes.Array(*input_shape))]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_split_nd(
                    name="split_nd",
                    input_name="data",
                    output_names=output_names,
                    axis=axis,
                    split_sizes=split_sizes,
                )

                x = np.random.rand(*input_shape)
                input = {"data": x}
                expected = dict(zip(output_names, np.split(x, sections, axis=axis)))

                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)
                for output_ in output_names:
                    self.assertEqual(rank, builder._get_rank(output_))

    def test_split_nd_with_split_sizes_gpu(self):
        self.test_split_nd_with_split_sizes_cpu(cpu_only=False)

    def test_slice_static_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            for _ in range(200):
                input_shape = np.array([5 for _ in range(rank)])
                objs, strides, begin_masks, end_ids, end_masks, begin_ids = (
                    [],
                    [],
                    [],
                    [],
                    [],
                    [],
                )
                for dim in range(rank):
                    stride = random.choice([-3, -1, 1, 2])
                    begin_mask = random.choice([True, False])
                    end_mask = random.choice([True, False])
                    length = 0
                    while length <= 0:
                        begin_id = np.random.randint(
                            low=-input_shape[dim], high=input_shape[dim]
                        )
                        end_id = np.random.randint(
                            low=-input_shape[dim], high=input_shape[dim]
                        )
                        obj = slice(
                            None if begin_mask else begin_id,
                            None if end_mask else end_id,
                            stride,
                        )
                        length = np.arange(input_shape[dim])[(obj,)].shape[0]

                    objs.append(obj), strides.append(stride), begin_masks.append(
                        begin_mask
                    )
                    end_masks.append(end_mask), begin_ids.append(
                        begin_id
                    ), end_ids.append(end_id)

                input_features = [("data", datatypes.Array(*input_shape))]
                output_features = [("output", None)]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_slice_static(
                    "slice_static",
                    "data",
                    "output",
                    begin_ids=begin_ids,
                    end_ids=end_ids,
                    strides=strides,
                    begin_masks=begin_masks,
                    end_masks=end_masks,
                )

                x = np.random.rand(*input_shape)
                inputs = {"data": x}
                expected = {"output": x[tuple(objs)]}

                self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)
                self.assertEqual(rank, builder._get_rank("output"))

    def test_slice_static_gpu(self):
        self.test_slice_static_cpu(cpu_only=False)

    def test_slice_dynamic_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            input_shape = np.array([5 for _ in range(rank)])
            objs, strides, begin_masks, end_ids, end_masks, begin_ids = (
                [],
                [],
                [],
                [],
                [],
                [],
            )
            squeeze_masks = []
            squeeze_axes = []
            for dim in range(rank):
                stride = random.choice([-3, -1, 1, 2])
                begin_mask = random.choice([True, False])
                end_mask = random.choice([True, False])
                if len(squeeze_axes) + 1 < rank:
                    squeeze_mask = random.choice([True, False])
                else:
                    squeeze_mask = False
                if squeeze_mask:
                    squeeze_axes.append(dim)
                length = 0
                while length <= 0:
                    begin_id = np.random.randint(
                        low=-input_shape[dim], high=input_shape[dim]
                    )
                    end_id = np.random.randint(
                        low=-input_shape[dim], high=input_shape[dim]
                    )
                    obj = slice(
                        None if begin_mask else begin_id,
                        None if end_mask else end_id,
                        stride,
                    )
                    length = np.arange(input_shape[dim])[(obj,)].shape[0]

                objs.append(obj), strides.append(stride), begin_masks.append(begin_mask)
                end_masks.append(end_mask), begin_ids.append(begin_id), end_ids.append(
                    end_id
                )
                squeeze_masks.append(squeeze_mask)

            # test different number of inputs, from 2 inputs up to 7 inputs
            # when num_inputs == 2, begin_ids are inputs, rest are read from parameters
            # when num_inputs == 7, all read from inputs, none are read from parameters
            for num_inputs in [2, 3, 4, 5, 6]:
                x = np.random.rand(*input_shape)

                input_features = [("data", datatypes.Array(*input_shape))]
                input_names = ["data"]
                inputs = dict()
                inputs["data"] = x

                if num_inputs == 2:
                    input_features = [
                        ("data", datatypes.Array(*input_shape)),
                        ("begin_ids", datatypes.Array(len(begin_ids))),
                    ]
                    input_names = ["data", "begin_ids"]
                    inputs["begin_ids"] = np.array(begin_ids, dtype=np.int32)
                elif num_inputs == 3:
                    input_features = [
                        ("data", datatypes.Array(*input_shape)),
                        ("begin_ids", datatypes.Array(len(begin_ids))),
                        ("end_ids", datatypes.Array(len(end_ids))),
                    ]
                    input_names = ["data", "begin_ids", "end_ids"]
                    inputs["begin_ids"] = np.array(begin_ids, dtype=np.int32)
                    inputs["end_ids"] = np.array(end_ids, dtype=np.int32)
                elif num_inputs == 4:
                    input_features = [
                        ("data", datatypes.Array(*input_shape)),
                        ("begin_ids", datatypes.Array(len(begin_ids))),
                        ("end_ids", datatypes.Array(len(end_ids))),
                        ("strides", datatypes.Array(len(strides))),
                    ]
                    input_names = ["data", "begin_ids", "end_ids", "strides"]
                    inputs["begin_ids"] = np.array(begin_ids, dtype=np.int32)
                    inputs["end_ids"] = np.array(end_ids, dtype=np.int32)
                    inputs["strides"] = np.array(strides, dtype=np.int32)
                elif num_inputs == 5:
                    input_features = [
                        ("data", datatypes.Array(*input_shape)),
                        ("begin_ids", datatypes.Array(len(begin_ids))),
                        ("end_ids", datatypes.Array(len(end_ids))),
                        ("strides", datatypes.Array(len(strides))),
                        ("begin_masks", datatypes.Array(len(begin_masks))),
                    ]
                    input_names = [
                        "data",
                        "begin_ids",
                        "end_ids",
                        "strides",
                        "begin_masks",
                    ]
                    inputs["begin_ids"] = np.array(begin_ids, dtype=np.int32)
                    inputs["end_ids"] = np.array(end_ids, dtype=np.int32)
                    inputs["strides"] = np.array(strides, dtype=np.int32)
                    inputs["begin_masks"] = np.array(begin_masks, dtype=np.int32)
                elif num_inputs == 6:
                    input_features = [
                        ("data", datatypes.Array(*input_shape)),
                        ("begin_ids", datatypes.Array(len(begin_ids))),
                        ("end_ids", datatypes.Array(len(end_ids))),
                        ("strides", datatypes.Array(len(strides))),
                        ("begin_masks", datatypes.Array(len(begin_masks))),
                        ("end_masks", datatypes.Array(len(end_masks))),
                    ]
                    input_names = [
                        "data",
                        "begin_ids",
                        "end_ids",
                        "strides",
                        "begin_masks",
                        "end_masks",
                    ]
                    inputs["begin_ids"] = np.array(begin_ids, dtype=np.int32)
                    inputs["end_ids"] = np.array(end_ids, dtype=np.int32)
                    inputs["strides"] = np.array(strides, dtype=np.int32)
                    inputs["begin_masks"] = np.array(begin_masks, dtype=np.int32)
                    inputs["end_masks"] = np.array(end_masks, dtype=np.int32)
                elif num_inputs == 7:
                    input_features = [
                        ("data", datatypes.Array(*input_shape)),
                        ("begin_ids", datatypes.Array(len(begin_ids))),
                        ("end_ids", datatypes.Array(len(end_ids))),
                        ("strides", datatypes.Array(len(strides))),
                        ("begin_masks", datatypes.Array(len(begin_masks))),
                        ("end_masks", datatypes.Array(len(end_masks))),
                        ("squeeze_masks", datatypes.Array(len(squeeze_masks))),
                    ]
                    input_names = [
                        "data",
                        "begin_ids",
                        "end_ids",
                        "strides",
                        "begin_masks",
                        "end_masks",
                        "squeeze_masks",
                    ]
                    inputs["begin_ids"] = np.array(begin_ids, dtype=np.int32)
                    inputs["end_ids"] = np.array(end_ids, dtype=np.int32)
                    inputs["strides"] = np.array(strides, dtype=np.int32)
                    inputs["begin_masks"] = np.array(begin_masks, dtype=np.int32)
                    inputs["end_masks"] = np.array(end_masks, dtype=np.int32)
                    inputs["squeeze_masks"] = np.array(squeeze_masks, dtype=np.int32)

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, [("output", None)], disable_rank5_shape_mapping=True
                )

                if num_inputs == 2:
                    builder.add_slice_dynamic(
                        "slice_dynamic",
                        input_names,
                        "output",
                        end_ids=end_ids,
                        strides=strides,
                        begin_masks=begin_masks,
                        end_masks=end_masks,
                        squeeze_masks=squeeze_masks,
                    )
                elif num_inputs == 3:
                    builder.add_slice_dynamic(
                        "slice_dynamic",
                        input_names,
                        "output",
                        strides=strides,
                        begin_masks=begin_masks,
                        end_masks=end_masks,
                        squeeze_masks=squeeze_masks,
                    )
                elif num_inputs == 4:
                    builder.add_slice_dynamic(
                        "slice_dynamic",
                        input_names,
                        "output",
                        begin_masks=begin_masks,
                        end_masks=end_masks,
                        squeeze_masks=squeeze_masks,
                    )
                elif num_inputs == 5:
                    builder.add_slice_dynamic(
                        "slice_dynamic",
                        input_names,
                        "output",
                        end_masks=end_masks,
                        squeeze_masks=squeeze_masks,
                    )
                elif num_inputs == 6:
                    builder.add_slice_dynamic(
                        "slice_dynamic",
                        input_names,
                        "output",
                        squeeze_masks=squeeze_masks,
                    )
                elif num_inputs == 7:
                    builder.add_slice_dynamic("slice_dynamic", input_names, "output")

                expected_x = x[tuple(objs)]
                squeeze_slices = []
                for squeeze in squeeze_masks:
                    if squeeze:
                        squeeze_slices.append(slice(None, 1, None))
                    else:
                        squeeze_slices.append(slice(None, None, None))
                expected_x = np.squeeze(
                    expected_x[tuple(squeeze_slices)], axis=tuple(squeeze_axes)
                )
                expected = {"output": expected_x}

                self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)
                self.assertEqual(rank, builder._get_rank("output"))

    def test_slice_dynamic_gpu(self):
        self.test_slice_dynamic_cpu(cpu_only=False)

    def test_tile_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            input_shape = np.random.randint(low=2, high=5, size=rank)
            for rep_rank in range(1, rank + 1):
                reps = list(np.random.randint(low=1, high=9, size=rep_rank))
                input_features = [("data", datatypes.Array(*input_shape))]
                output_features = [("output", None)]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_tile("Tile", "data", "output", reps)

                x = np.random.rand(*input_shape)
                input = {"data": x}
                expected = {"output": np.tile(x, reps)}

                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_tile_gpu(self):
        self.test_tile_cpu(cpu_only=False)

    @pytest.mark.skip(
        reason="rdar://65198011 (Re-enable Conv3dTranspose and DynamicTile unit tests)"
    )
    def test_dynamic_tile_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            input_shape = np.random.randint(low=2, high=5, size=rank)
            for rep_rank in range(1, rank + 1):
                reps = np.random.randint(low=1, high=9, size=rep_rank)
                input_features = [
                    ("data", datatypes.Array(*input_shape)),
                    ("reps", datatypes.Array(*reps.shape)),
                ]
                output_features = [("output", None)]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_tile("Tile", ["data", "reps"], "output")

                x = np.random.rand(*input_shape)
                input = {"data": x, "reps": reps.astype(np.float32)}
                expected = {"output": np.tile(x, list(reps))}

                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_sliding_windows_cpu(self, cpu_only=True):
        def numpy_sliding_windows(a, np_axis, np_size, np_step):
            n = (a.shape[np_axis] - np_size) // np_step + 1
            shape = list(a.shape)
            shape[np_axis] = n
            if np_axis < 0:
                np_axis += len(shape)
            shape.insert(np_axis + 1, np_size)
            strides = list(a.strides)
            effstride = strides[np_axis] * np_step
            strides.insert(np_axis, effstride)
            return np.lib.stride_tricks.as_strided(a, shape, strides)

        for rank in range(1, 5):
            for axis in range(-rank, rank):
                input_shape = np.random.randint(low=2, high=5, size=rank)
                output_shape = list(input_shape)
                window_size = np.random.randint(low=1, high=input_shape[axis])

                length = 0
                while length <= 0:
                    step = np.random.randint(low=1, high=input_shape[axis])
                    length = (input_shape[axis] - window_size) // step + 1

                output_shape[axis] = length

                pos_axis = axis if axis >= 0 else axis + rank
                output_shape.insert(pos_axis + 1, window_size)

                input_features = [("data", datatypes.Array(*input_shape))]
                output_features = [("output", None)]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_sliding_windows(
                    "sliding_windows",
                    input_name="data",
                    output_name="output",
                    axis=axis,
                    window_size=window_size,
                    step=step,
                )

                x = np.random.rand(*input_shape)
                input = {"data": x}
                expected = {"output": numpy_sliding_windows(x, axis, window_size, step)}

                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)
                self.assertEqual(rank + 1, builder._get_rank("output"))

    def test_sliding_windows_gpu(self):
        self.test_sliding_windows_cpu(cpu_only=False)

    def test_range_static_cpu(self, cpu_only=True):

        params = [
            (-10.4, 23, 12.2),
            (0, 1000, 1),
            (50.5, 90.5, 1.5),
            (5, 8, 2),
            (5, 8, 98),
            (5, 8, 1.5),
            (10, 5, -0.6),
            (24, -65, -2),
        ]

        for param in params:
            start, end, step = param
            input_features = [("multiplicative_input", datatypes.Array(1))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_range_static(
                "range_static", "output_range", end=end, start=start, step=step
            )
            builder.add_multiply_broadcastable(
                name="multiply_broadcastable",
                input_names=["multiplicative_input", "output_range"],
                output_name="output",
            )

            # save the model
            model_dir = tempfile.mkdtemp()
            model_path = os.path.join(model_dir, "test_layer.mlmodel")
            coremltools.utils.save_spec(builder.spec, model_path)

            inputs = dict()
            inputs["multiplicative_input"] = np.ones((1,), dtype=np.float64)
            expected = {"output": np.arange(start, end, step)}

            self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)
            self.assertEqual(1, builder._get_rank("output"))

    def test_range_static_gpu(self):
        self.test_range_static_cpu(cpu_only=False)

    def test_range_dynamic_cpu(self, cpu_only=True):
        params = [
            (-10.4, 23, 12.2),
            (0, 1000, 1),
            (50.5, 90.5, 1.5),
            (5, 8, 2),
            (5, 8, 98),
            (5, 8, 1.5),
            (10, 5, -0.6),
            (24, -65, -2),
        ]

        # input size == 1: end is input, start and step are read from parameters
        # input size == 2: end, start are inputs, step is read from parameters
        # input size == 3: start, end, step are all inputs, none of the parameters are used.
        for num_inputs in [1, 2, 3]:
            for param in params:
                inputs = dict()
                start, end, step = param

                if num_inputs == 1:
                    input_features = [("end", datatypes.Array(1))]
                elif num_inputs == 2:
                    input_features = [
                        ("end", datatypes.Array(1)),
                        ("start", datatypes.Array(1)),
                    ]
                elif num_inputs == 3:
                    input_features = [
                        ("end", datatypes.Array(1)),
                        ("start", datatypes.Array(1)),
                        ("step", datatypes.Array(1)),
                    ]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, [("output", None)], disable_rank5_shape_mapping=True
                )

                if num_inputs == 1:
                    inputs["end"] = end * np.ones((1,), dtype=np.float64)
                    builder.add_range_dynamic(
                        "range_dynamic",
                        output_name="output",
                        input_names=["end"],
                        start=start,
                        step=step,
                    )
                elif num_inputs == 2:
                    inputs["end"] = end * np.ones((1,), dtype=np.float64)
                    inputs["start"] = start * np.ones((1,), dtype=np.float64)
                    builder.add_range_dynamic(
                        "range_dynamic",
                        output_name="output",
                        input_names=["end", "start"],
                        step=step,
                    )
                elif num_inputs == 3:
                    inputs["end"] = end * np.ones((1,), dtype=np.float64)
                    inputs["start"] = start * np.ones((1,), dtype=np.float64)
                    inputs["step"] = step * np.ones((1,), dtype=np.float64)
                    builder.add_range_dynamic(
                        "range_dynamic",
                        output_name="output",
                        input_names=["end", "start", "step"],
                    )

                expected = {"output": np.arange(start, end, step)}

                self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)
                self.assertEqual(1, builder._get_rank("output"))

    def test_range_dynamic_gpu(self):
        self.test_range_dynamic_cpu(cpu_only=False)

    def test_linear_activation_different_ranks_cpu(self, cpu_only=True):
        for input_dim in [(10, 15), (10, 15, 2, 3), (10, 2, 4, 15, 1), (6,)]:
            input_features = [("data", datatypes.Array(*input_dim))]
            output_features = [("output", datatypes.Array(*input_dim))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )

            builder.add_activation(
                name="activation",
                non_linearity="LINEAR",
                input_name="data",
                output_name="output",
                params=[34.0, 67.0],
            )

            x = np.random.rand(*input_dim)
            input = {"data": x}
            expected = {"output": 34.0 * x + 67.0}

            self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_linear_activation_different_ranks_gpu(self):
        self.test_linear_activation_different_ranks_cpu(cpu_only=False)

    def test_topk_cpu(self, cpu_only=True):
        test_input_shapes = [(9,), (8, 6), (9, 8, 10), (5, 9, 7, 9), (12, 8, 6, 6, 7)]
        K = [3, 5]
        axes = [[0], [0, 1], [1, 2], [0, 3, 1], [1, 3, 4]]

        for ii, input_shape in enumerate(test_input_shapes):
            for k in K:
                for n_inputs in [1, 2]:
                    for bottom_k_flag in [False, True]:
                        for axis in axes[ii]:
                            for negative_axis in [False, True]:

                                if negative_axis:
                                    axis = axis - len(input_shape)

                                input_features = [
                                    ("data", datatypes.Array(*input_shape))
                                ]
                                output_features = [("values", None), ("indices", None)]

                                input_names = ["data"]
                                output_names = ["values", "indices"]

                                if n_inputs == 2:
                                    input_names.append("k_in")
                                    input_features.append(("k_in", datatypes.Array(1)))

                                builder = neural_network.NeuralNetworkBuilder(
                                    input_features,
                                    output_features,
                                    disable_rank5_shape_mapping=True,
                                )

                                if n_inputs == 2:
                                    builder.add_topk(
                                        "topk",
                                        input_names,
                                        output_names,
                                        axis=axis,
                                        use_bottom_k=bottom_k_flag,
                                    )
                                else:
                                    builder.add_topk(
                                        "topk",
                                        input_names,
                                        output_names,
                                        k=k,
                                        axis=axis,
                                        use_bottom_k=bottom_k_flag,
                                    )

                                data = np.random.randint(
                                    low=0,
                                    high=int(np.prod(input_shape)),
                                    size=input_shape,
                                )
                                data = data.astype(np.float32)

                                input = {"data": data}
                                if n_inputs == 2:
                                    input["k_in"] = k * np.ones([1], dtype=np.float32)

                                # numpy reference values
                                if bottom_k_flag:
                                    ref_indices = np.argsort(data, axis=axis)
                                else:
                                    ref_indices = np.argsort(-data, axis=axis)

                                slc = [slice(None)] * len(input_shape)
                                slc[axis] = slice(0, k)
                                ref_indices = ref_indices[tuple(slc)]
                                ref_values = np.take_along_axis(
                                    data, ref_indices, axis=axis
                                )
                                expected = {
                                    "values": ref_values,
                                    "indices": ref_indices,
                                }

                                self._test_model(
                                    builder.spec, input, expected, useCPUOnly=cpu_only
                                )

    def test_topk_gpu(self):
        self.test_topk_cpu(cpu_only=False)

    @unittest.skipIf(not _HAS_TF, MSG_TF1_NOT_FOUND)
    def test_const_pad_cpu(self, cpu_only=True):
        def get_reference(data, pads, value):
            with tf.Graph().as_default(), tf.Session() as sess:
                x = tf.placeholder(tf.float32, shape=data.shape)
                p = tf.placeholder(tf.int32, shape=pads.shape)
                y = tf.pad(x, p, mode="CONSTANT", constant_values=value)
                return sess.run(y, feed_dict={x: data, p: pads})

        value = 34.0
        shapes = [(3,), (4, 5), (2, 4, 5), (12, 6, 3, 5, 7), (1, 24, 2, 4, 8)]

        ctr = 0
        for shape in shapes:
            rank = len(shape)
            for force_zeros_in_end in [0, 2, 6]:
                for max_pad_value in range(1, 6):
                    for n_inputs in [1, 2]:
                        pads = np.random.randint(
                            low=0, high=max_pad_value, size=(rank, 2)
                        )

                        if force_zeros_in_end > 2 * rank:
                            continue

                        # pads = np.reshape(np.array([1,1,1,0,0,1]), (rank, 2))
                        if force_zeros_in_end != 0:
                            pads[-force_zeros_in_end:] = 0

                        data = np.random.rand(*shape)
                        reference = get_reference(data, pads, value)

                        ctr += 1

                        input_features = [("data", datatypes.Array(*shape))]
                        output_features = [("output", None)]

                        input_names = ["data"]
                        if n_inputs == 2:
                            input_names.append("pads")
                            input_features.append(("pads", datatypes.Array(2 * rank,)))

                        builder = neural_network.NeuralNetworkBuilder(
                            input_features,
                            output_features,
                            disable_rank5_shape_mapping=True,
                        )
                        if n_inputs == 2:
                            builder.add_constant_pad(
                                "pad", input_names, "output", value=value
                            )
                        else:
                            builder.add_constant_pad(
                                "pad",
                                input_names,
                                "output",
                                value=value,
                                pad_amounts=pads.flatten(),
                            )

                        input = {"data": data}
                        if n_inputs == 2:
                            input["pads"] = pads.flatten().astype(np.float)

                        expected = {"output": reference}
                        self._test_model(
                            builder.spec, input, expected, useCPUOnly=cpu_only
                        )

    def test_const_pad_gpu(self):
        self.test_const_pad_cpu(cpu_only=False)

    @unittest.skipIf(not _HAS_TF, MSG_TF1_NOT_FOUND)
    def test_const_pad_mode2_cpu(self, cpu_only=True):
        def get_reference(data, output_shape, value, left_pad=False):
            with tf.Graph().as_default(), tf.Session() as sess:
                x = tf.placeholder(tf.float32, shape=data.shape)
                p = tf.placeholder(tf.int32, shape=(len(output_shape), 2))
                y = tf.pad(x, p, mode="CONSTANT", constant_values=value)
                pads = np.zeros((len(output_shape), 2))
                if left_pad:
                    pads[:, 0] = np.array(output_shape) - np.array(data.shape)
                else:
                    pads[:, 1] = np.array(output_shape) - np.array(data.shape)

                return sess.run(y, feed_dict={x: data, p: pads})

        value = 34.0
        shapes = [(3,), (4, 5), (2, 4, 5), (12, 6, 3, 5, 7), (1, 24, 2, 4, 8)]
        out_shapes = [(5,), (4, 8), (2, 4, 10), (20, 6, 7, 10, 7), (5, 24, 10, 4, 10)]

        ctr = 0
        for ii, shape in enumerate(shapes):
            rank = len(shape)
            for left_pad in [True, False]:
                for n_inputs in [1, 2]:

                    data = np.random.rand(*shape)
                    reference = get_reference(data, out_shapes[ii], value, left_pad)

                    pads = np.zeros((rank, 2))
                    tmp = np.zeros((rank))

                    for i in range(rank):
                        if out_shapes[ii][i] == shape[i]:
                            tmp[i] = 0
                        else:
                            tmp[i] = out_shapes[ii][i]

                    if left_pad:
                        pads[:, 0] = tmp
                    else:
                        pads[:, 1] = tmp

                    ctr += 1

                    input_features = [("data", datatypes.Array(*shape))]
                    output_features = [("output", None)]

                    input_names = ["data"]
                    if n_inputs == 2:
                        input_names.append("pads")
                        input_features.append(("pads", datatypes.Array(2 * rank,)))

                    builder = neural_network.NeuralNetworkBuilder(
                        input_features,
                        output_features,
                        disable_rank5_shape_mapping=True,
                    )
                    if n_inputs == 2:
                        builder.add_constant_pad(
                            "pad",
                            input_names,
                            "output",
                            value=value,
                            pad_to_given_output_size_mode=True,
                        )
                    else:
                        builder.add_constant_pad(
                            "pad",
                            input_names,
                            "output",
                            value=value,
                            pad_amounts=pads.flatten(),
                            pad_to_given_output_size_mode=True,
                        )

                    input = {"data": data}
                    if n_inputs == 2:
                        input["pads"] = pads.flatten().astype(np.float)

                    expected = {"output": reference}
                    self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_const_pad_mode2_gpu(self):
        self.test_const_pad_mode2_cpu(cpu_only=False)

    @pytest.mark.xfail(reason="rdar://problem/59486372", run=False)
    def test_nms_cpu(self, cpu_only=True):
        def _compute_iou_matrix(boxes):
            # input is (N,4), in order [center_w, center_h, width, height]
            self.assertEqual(len(boxes.shape), 2)
            self.assertEqual(boxes.shape[1], 4)
            boxes = boxes.astype(np.float)
            center_w, center_h, width, height = np.split(
                boxes, 4, axis=1
            )  # outs are all (N,1)
            top = center_h + 0.5 * height
            bottom = center_h - 0.5 * height
            left = center_w - 0.5 * width
            right = center_w + 0.5 * width
            area = width * height

            hB = np.minimum(top, np.transpose(top))
            wB = np.minimum(right, np.transpose(right))
            hA = np.maximum(bottom, np.transpose(bottom))
            wA = np.maximum(left, np.transpose(left))

            intersection_area = np.maximum(0, hB - hA) * np.maximum(0, wB - wA)
            union_area = area + np.transpose(area) - intersection_area
            iou = intersection_area / union_area
            return iou

        @unittest.skipIf(not _HAS_TF, MSG_TF1_NOT_FOUND)
        def _nms_TF(
            boxes, scores, iou_threshold, score_threshold, per_class_suppression, M
        ):
            # boxes is (B,N,4), in order [center_w, center_h, width, height]
            # scores is (B,N,C)
            # output shapes: (B,M,4), (B,M,C), (B,M), (B,)
            """
            this is implementation of CoreML's NMS layer
            """
            B, N, C = scores.shape

            iou_threshold = iou_threshold.astype(np.float32)
            score_threshold = score_threshold.astype(np.float32)

            # convert box ids to TF style
            center_w, center_h, width, height = np.split(
                boxes, 4, axis=-1
            )  # outs are all (B,N,1)
            y1 = center_h - 0.5 * height
            y2 = center_h + 0.5 * height
            x1 = center_w - 0.5 * width
            x2 = center_w + 0.5 * width
            boxes_tf = np.concatenate((y1, x1, y2, x2), axis=-1)  # (B,N,4)

            out1 = np.zeros((B, M, 4))
            out2 = np.zeros((B, M, C))
            out3 = -1 * np.ones((B, M))
            out4 = np.zeros((B,))

            for b in range(B):
                box_coord_matrix = boxes_tf[b, :, :]  # (N,4)
                score_vector = np.max(scores[b, :, :], axis=-1)  # (N,)
                if not per_class_suppression:
                    # this is the simple case as TF directly supports it
                    with tf.Graph().as_default(), tf.Session() as sess:
                        box_coord_matrix_pl = tf.placeholder(
                            tf.float32, shape=box_coord_matrix.shape
                        )
                        score_vector_pl = tf.placeholder(
                            tf.float32, shape=score_vector.shape
                        )
                        ids_g = tf.image.non_max_suppression(
                            box_coord_matrix_pl,
                            score_vector_pl,
                            max_output_size=M,
                            iou_threshold=iou_threshold,
                            score_threshold=score_threshold,
                        )

                        ids = sess.run(
                            ids_g,
                            feed_dict={
                                box_coord_matrix_pl: box_coord_matrix,
                                score_vector_pl: score_vector,
                            },
                        )
                else:
                    # this is slightly complicated as TF does not directly support it
                    class_ids = np.argmax(scores[b, :, :], axis=-1)  # (N,)
                    sorted_score_ids = np.argsort(-score_vector)
                    box_coord_matrix2 = np.take(
                        box_coord_matrix, sorted_score_ids, axis=0
                    )
                    score_vector2 = np.take(score_vector, sorted_score_ids)
                    class_ids = np.take(class_ids, sorted_score_ids)
                    classes_seen = dict()
                    ids_intermediate = np.array([], dtype=np.int)
                    for n in range(N):
                        if class_ids[n] in classes_seen:
                            continue
                        c = class_ids[n]
                        classes_seen[c] = True
                        current_class_ids = np.where(class_ids == c)[0]
                        if len(current_class_ids) > 0:
                            feed_in1 = np.take(
                                box_coord_matrix2, current_class_ids, axis=0
                            )
                            feed_in2 = np.take(score_vector2, current_class_ids)

                            with tf.Graph().as_default(), tf.Session() as sess:
                                box_coord_matrix_pl = tf.placeholder(
                                    tf.float32, shape=feed_in1.shape
                                )
                                score_vector_pl = tf.placeholder(
                                    tf.float32, shape=feed_in2.shape
                                )
                                cur_ids_g = tf.image.non_max_suppression(
                                    box_coord_matrix_pl,
                                    score_vector_pl,
                                    max_output_size=M,
                                    iou_threshold=iou_threshold,
                                    score_threshold=score_threshold,
                                )
                                cur_ids = sess.run(
                                    cur_ids_g,
                                    feed_dict={
                                        box_coord_matrix_pl: feed_in1,
                                        score_vector_pl: feed_in2,
                                    },
                                )

                            from_sort_ids = np.take(current_class_ids, cur_ids)
                            ids_intermediate = np.append(
                                ids_intermediate, from_sort_ids
                            )
                    ids_intermediate.sort()
                    ids = np.take(sorted_score_ids, ids_intermediate)

                xx = len(ids)
                if xx == 0:
                    ids = np.array([np.argmax(score_vector)])
                    xx = 1
                if xx > M:
                    ids = ids[:M]
                    xx = len(ids)
                out1[b, :xx, :] = np.take(boxes[b, :, :], ids, axis=0)
                out2[b, :xx, :] = np.take(scores[b, :, :], ids, axis=0)
                out3[b, :xx] = ids
                out4[b] = xx

            return out1, out2, out3, out4

        iou_threshold_percentile = [0, 30, 80, 100]
        score_threshold_percentile_arr = [0, 40, 100]
        N_M_pairs_to_test = [[100, 48], [100, 112]]  # N : boxes in, M: max boxes out

        number_of_test = 0
        for N_M in N_M_pairs_to_test:
            for B in [1]:  # [1, 5] TODO Re-enable when rdar://60280745 is fixed
                for C in [1, 7]:
                    N, M = N_M

                    boxes = np.random.rand(B, N, 4)
                    scores = np.random.rand(B, N, C)

                    iou_matrix = _compute_iou_matrix(boxes[0, :, :])  # (N,N)
                    iou_matrix = iou_matrix[
                        ~np.eye(iou_matrix.shape[0], dtype=bool)
                    ].reshape(iou_matrix.shape[0], -1)

                    for per_class_suppression in [False, True]:
                        for iou_thresh in iou_threshold_percentile:
                            for score_thresh in score_threshold_percentile_arr:
                                for is_dynamic in [False, True]:

                                    if score_thresh == 0:
                                        score_threshold = np.min(scores) - 1
                                    elif score_thresh == 100:
                                        score_threshold = np.max(scores) + 1
                                    else:
                                        score_threshold = (
                                            np.percentile(scores, score_thresh) + 0.01
                                        )

                                    if iou_thresh == 0:
                                        iou_threshold = np.maximum(
                                            np.min(iou_matrix) - 0.01, 0.0
                                        )
                                    else:
                                        iou_threshold = (
                                            np.percentile(iou_matrix, iou_thresh) + 0.01
                                        )

                                    number_of_test += 1

                                    tf_boxes, tf_scores, tf_ids, tf_num_boxes = _nms_TF(
                                        boxes,
                                        scores,
                                        iou_threshold,
                                        score_threshold,
                                        per_class_suppression,
                                        M,
                                    )
                                    expected = dict()
                                    expected["selected_boxes"] = tf_boxes
                                    expected["selected_scores"] = tf_scores
                                    expected["selected_box_ids"] = tf_ids
                                    expected["number_of_boxes"] = tf_num_boxes

                                    # define CoreML model

                                    input_features = [
                                        ("boxes", datatypes.Array(B, N, 4)),
                                        ("scores", datatypes.Array(B, N, C)),
                                    ]
                                    output_features = [
                                        ("selected_boxes", None),
                                        ("selected_scores", None),
                                        ("selected_box_ids", None),
                                        ("number_of_boxes", None),
                                    ]

                                    input_names = ["boxes", "scores"]
                                    if is_dynamic:
                                        input_names.extend(
                                            [
                                                "iou_threshold",
                                                "score_threshold",
                                                "max_boxes",
                                            ]
                                        )
                                        input_features.append(
                                            ("iou_threshold", datatypes.Array(1,))
                                        )
                                        input_features.append(
                                            ("score_threshold", datatypes.Array(1,))
                                        )
                                        input_features.append(
                                            ("max_boxes", datatypes.Array(1,))
                                        )

                                    builder = neural_network.NeuralNetworkBuilder(
                                        input_features,
                                        output_features,
                                        disable_rank5_shape_mapping=True,
                                    )

                                    input_dict = dict()
                                    input_dict["boxes"] = boxes
                                    input_dict["scores"] = scores

                                    if is_dynamic:
                                        builder.add_nms(
                                            "nms",
                                            input_names,
                                            [
                                                "selected_boxes",
                                                "selected_scores",
                                                "selected_box_ids",
                                                "number_of_boxes",
                                            ],
                                            per_class_suppression=per_class_suppression,
                                        )

                                        input_dict[
                                            "iou_threshold"
                                        ] = iou_threshold * np.ones([1], dtype=np.float)
                                        input_dict["score_threshold"] = (
                                            score_threshold
                                            * np.ones([1], dtype=np.float)
                                        )
                                        input_dict["max_boxes"] = M * np.ones(
                                            [1], dtype=np.float
                                        )
                                    else:
                                        builder.add_nms(
                                            "nms",
                                            input_names,
                                            [
                                                "selected_boxes",
                                                "selected_scores",
                                                "selected_box_ids",
                                                "number_of_boxes",
                                            ],
                                            iou_threshold=iou_threshold,
                                            score_threshold=score_threshold,
                                            max_boxes=M,
                                            per_class_suppression=per_class_suppression,
                                        )

                                    self._test_model(
                                        builder.spec,
                                        input_dict,
                                        expected,
                                        useCPUOnly=cpu_only,
                                    )

    def test_nms_gpu(self):
        self.test_nms_cpu(cpu_only=False)

    def test_rank_preserving_reshape(self):
        input_shapes = [(20, 10), (20, 10, 5), (10, 3, 5)]
        target_shapes = [(5, -1), (0, 2, 25), (25, 0, -1)]
        output_shapes = [(5, 40), (20, 2, 25), (25, 3, 2)]

        for i in range(len(input_shapes)):
            input_features = [("data", datatypes.Array(*input_shapes[i]))]
            output_features = [("output", None)]
            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )

            builder.add_rank_preserving_reshape(
                name="rank_preserving_reshape",
                input_name="data",
                output_name="output",
                output_shape=target_shapes[i],
            )

            x = np.random.rand(*input_shapes[i])
            input = {"data": x}
            expected = {"output": np.reshape(x, output_shapes[i])}

            self._test_model(builder.spec, input, expected, useCPUOnly=True)
            self.assertEqual(len(output_shapes[i]), builder._get_rank("output"))

    def test_expand_dims(self):
        input_shapes = [(10, 5), (10, 5), (10, 5), (10, 5), (10,)]
        axes = [(0, 1), (0, 2), (2, 0), (-2, -1), (1, 0, -2)]
        output_shapes = [
            (1, 1, 10, 5),
            (1, 10, 1, 5),
            (1, 10, 1, 5),
            (10, 5, 1, 1),
            (1, 1, 1, 10),
        ]

        for i in range(len(input_shapes)):
            input_features = [("data", datatypes.Array(*input_shapes[i]))]
            output_features = [("output", None)]
            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )

            builder.add_expand_dims(
                name="expand_dims",
                input_name="data",
                output_name="output",
                axes=axes[i],
            )

            x = np.random.rand(*input_shapes[i])
            input = {"data": x}
            expected = {"output": np.reshape(x, output_shapes[i])}

            self._test_model(builder.spec, input, expected, useCPUOnly=True)
            self.assertEqual(len(output_shapes[i]), builder._get_rank("output"))

    def test_squeeze(self):
        input_shapes = [
            (1, 1, 10, 5),
            (1, 10, 1, 5),
            (10, 5, 1, 1),
            (10, 5, 1, 1),
            (1,),
            (10, 5, 1, 1),
            (3, 1, 7),
        ]
        axes = [(0, 1), (0, 2), (-2, -1), (-1, -2), (0,), (3, -2), (1,)]
        output_shapes = [(10, 5), (10, 5), (10, 5), (10, 5), (1,), (10, 5), (3, 7)]

        for i in range(len(input_shapes)):
            input_features = [("data", datatypes.Array(*input_shapes[i]))]
            output_features = [("output", None)]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )
            builder.add_squeeze(
                name="squeeze_layer",
                input_name="data",
                output_name="output",
                axes=list(axes[i]),
            )

            x = np.random.rand(*input_shapes[i])
            input = {"data": x}
            expected = {"output": np.reshape(x, output_shapes[i])}

            self._test_model(builder.spec, input, expected, useCPUOnly=True)
            self.assertEqual(len(output_shapes[i]), builder._get_rank("output"))

    def test_squeeze_all(self):
        input_shapes = [
            (1, 1, 10, 5),
            (1, 10, 1, 5),
            (10, 5, 1, 1),
            (10, 5, 1, 1),
            (1,),
            (10, 5, 1, 1),
            (3, 1, 7),
            (3,),
            (5, 6),
        ]
        for input_shape in input_shapes:
            input_features = [("data", datatypes.Array(*input_shape))]
            output_features = [("output", None)]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )
            builder.add_squeeze(
                name="squeeze_layer",
                input_name="data",
                output_name="output",
                squeeze_all=True,
            )

            x = np.random.rand(*input_shape)
            input = {"data": x}
            reference = np.squeeze(x)
            if not reference.shape:
                reference = np.reshape(reference, (1,))
            expected = {"output": reference}

            self._test_model(builder.spec, input, expected, useCPUOnly=True)
            self.assertEqual(-1, builder._get_rank("output"))

    def test_argmax_argmin(self):
        test_input_shapes = [(9,), (8, 6), (9, 8, 10), (5, 9, 7, 9), (12, 8, 6, 6, 7)]

        # (1+2+3+4+5) * 2^3 = 120 test cases
        for input_shape in test_input_shapes:
            for negative_axis in [False, True]:
                for mode in ["argmax", "argmin"]:
                    for keep_dims in [True, False]:
                        for axis in np.arange(len(input_shape)):

                            if negative_axis:
                                axis_val = axis - len(input_shape)
                            else:
                                axis_val = axis

                            input_features = [("data", datatypes.Array(*input_shape))]
                            output_features = [("output", None)]

                            builder = neural_network.NeuralNetworkBuilder(
                                input_features,
                                output_features,
                                disable_rank5_shape_mapping=True,
                            )

                            x = np.random.rand(*input_shape)

                            if mode == "argmax":
                                builder.add_argmax(
                                    "argmax",
                                    "data",
                                    "output",
                                    axis=axis_val,
                                    keepdims=keep_dims,
                                )
                                np_out = np.argmax(x, axis=axis_val)
                            else:
                                builder.add_argmin(
                                    "argmin",
                                    "data",
                                    "output",
                                    axis=axis_val,
                                    keepdims=keep_dims,
                                )
                                np_out = np.argmin(x, axis=axis_val)

                            if keep_dims:
                                np_out = np.expand_dims(np_out, axis=axis_val)
                            elif len(input_shape) == 1:
                                np_out = np.expand_dims(np_out, axis=axis_val)

                            input = {"data": x}
                            expected = {"output": np_out}

                            test_case = "test_argmax_argmin_input_shape_{}_axis_{}_keep_dims_{}_numpy_out_shape_{}".format(
                                x.shape, axis_val, keep_dims, np_out.shape
                            )

                            self._test_model(
                                builder.spec, input, expected, useCPUOnly=True
                            )
                            if len(np_out.shape) != 0:
                                self.assertEqual(
                                    len(np_out.shape), builder._get_rank("output")
                                )

    def test_get_shape(self):
        dims = [1, 2, 3, 4, 5]
        for rank in range(1, len(dims) + 1):
            input_shape = dims[:rank]
            input_features = [("data", datatypes.Array(*input_shape))]
            output_features = [("output", None)]
            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )
            builder.add_get_shape(
                name="get_shape_layer", input_name="data", output_name="output"
            )

            feed = {"data": np.random.rand(*input_shape)}
            expected = {"output": np.array(input_shape)}

            self._test_model(builder.spec, feed, expected, useCPUOnly=True)
            self.assertEqual(1, builder._get_rank("output"))

    def test_load_constant_nd(self):
        dims = [2, 3, 4, 5, 6]
        for rank in range(1, len(dims) + 1):
            input_shape = dims[:rank]
            input_features = [("data", datatypes.Array(*input_shape))]
            output_features = [("output", None)]
            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )
            builder.add_load_constant_nd(
                "load_const_nd_layer",
                "tmp",
                constant_value=np.ones(input_shape),
                shape=input_shape,
            )
            builder.add_elementwise("add_layer", ["data", "tmp"], "output", mode="ADD")
            feed = {"data": np.random.rand(*input_shape)}
            expected = {"output": feed["data"] + 1}

            self._test_model(builder.spec, feed, expected, useCPUOnly=True)
            self.assertEqual(rank, builder._get_rank("output"))

    def test_simple_array_alloc_scatter(self):
        alloc_shape = [2, 3, 4]
        value_shape = [1, 3, 4]
        input_features = [
            ("alloc_shape", datatypes.Array(len(alloc_shape))),
            ("value", datatypes.Array(*value_shape)),
            ("index", datatypes.Array(1)),
        ]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        builder.add_fill_dynamic(
            name="fill_dynamic_layer",
            input_name="alloc_shape",
            output_name="array",
            value=np.float(0.0),
        )
        # CoreML input order: container (array), indices, slices (value)
        builder.add_scatter(
            name="scatter_layer",
            input_names=["array", "index", "value"],
            output_name="output",
        )

        value = np.random.rand(*value_shape).astype("float")
        feed = {
            "alloc_shape": np.array(alloc_shape, dtype="float"),
            "value": value,
            "index": np.array([1], dtype="float"),
        }

        ref = np.zeros(alloc_shape)
        ref[1, :, :] = value
        expected = {"output": ref}

        self._test_model(builder.spec, feed, expected, useCPUOnly=True)

    def test_erf_activation_cpu(self, cpu_only=True):
        input_features = [("data", datatypes.Array(10, 45))]
        output_features = [("output", datatypes.Array(10, 45))]

        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        builder.add_erf(name="erf", input_name="data", output_name="output")
        x = np.random.rand(10, 45)
        input = {"data": x}
        expected = {
            "output": np.asarray([math.erf(i) for i in x.flatten().tolist()]).reshape(
                10, 45
            )
        }

        self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_erf_activation_gpu(self):
        self.test_erf_activation_cpu(cpu_only=False)

    def test_gelu_activation(self):

        for mode in ["EXACT", "TANH_APPROXIMATION", "SIGMOID_APPROXIMATION"]:
            for rank in range(1, 6):
                shape = np.random.randint(low=2, high=5, size=rank)
                input_features = [("data", datatypes.Array(*shape))]
                output_features = [("output", None)]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )
                builder.add_gelu(
                    name="gelu", input_name="data", output_name="output", mode=mode
                )

                x = np.random.rand(*shape)
                input = {"data": x}
                exact = np.asarray(
                    [
                        0.5 * i * (1.0 + math.erf(i / math.sqrt(2)))
                        for i in x.flatten().tolist()
                    ]
                ).reshape(*shape)

                expected = {"output": exact}
                self._test_model(builder.spec, input, expected, useCPUOnly=True)

    def test_lower_triangular_cpu(self, cpu_only=True):
        for rank in range(2, 6):
            for k in range(-3, 4):
                shape = np.random.randint(low=2, high=6, size=rank)
                input_features = [("data", datatypes.Array(*shape))]
                output_features = [("output", None)]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_lower_triangular("tril", "data", "output", k=k)

                x = np.random.rand(*shape)
                input = {"data": x}
                expected = {"output": np.tril(x, k=k)}
                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_lower_triangular_gpu(self):
        self.test_lower_triangular_cpu(cpu_only=False)

    def test_upper_triangular_cpu(self, cpu_only=True):
        for rank in range(2, 6):
            for k in range(-3, 4):
                shape = np.random.randint(low=2, high=6, size=rank)
                input_features = [("data", datatypes.Array(*shape))]
                output_features = [("output", None)]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_upper_triangular("triu", "data", "output", k=k)

                x = np.random.rand(*shape)
                input = {"data": x}
                expected = {"output": np.triu(x, k=k)}
                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_upper_triangular_gpu(self):
        self.test_upper_triangular_cpu(cpu_only=False)

    def test_where_broadcastable_cpu(self, cpu_only=True):
        for _ in range(150):
            rank_cond = np.random.randint(low=1, high=6)
            rank_true = np.random.randint(low=1, high=6)
            rank_false = np.random.randint(low=1, high=6)

            rank_out = max(rank_cond, rank_true, rank_false)

            shape_cond = np.random.randint(low=2, high=8, size=rank_cond)
            shape_true = np.random.randint(low=2, high=8, size=rank_true)
            shape_false = np.random.randint(low=2, high=8, size=rank_false)

            for i in range(-1, -rank_out - 1, -1):
                dims = []
                if -i <= rank_cond:
                    dims.append(shape_cond[i])
                if -i <= rank_true:
                    dims.append(shape_true[i])
                if -i <= rank_false:
                    dims.append(shape_false[i])

                dim = np.random.choice(dims)
                if -i <= rank_cond:
                    shape_cond[i] = np.random.choice([1, dim])
                if -i <= rank_true:
                    shape_true[i] = np.random.choice([1, dim])
                if -i <= rank_false:
                    shape_false[i] = np.random.choice([1, dim])

            input_features = [
                ("cond", datatypes.Array(*shape_cond)),
                ("true", datatypes.Array(*shape_true)),
                ("false", datatypes.Array(*shape_false)),
            ]
            output_features = [("output", None)]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )

            builder.add_where_broadcastable(
                "if_broadcastable",
                input_names=["cond", "true", "false"],
                output_name="output",
            )

            cond = np.random.choice([1.0, 0.0], size=shape_cond)
            true = np.random.rand(*shape_true)
            false = np.random.rand(*shape_false)

            input = {"cond": cond, "true": true, "false": false}
            expected = {"output": np.where(cond, true, false)}
            self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)
            self.assertEqual(len(expected["output"].shape), builder._get_rank("output"))

    def test_where_broadcastable_gpu(self):
        self.test_where_broadcastable_cpu(cpu_only=False)

    @pytest.mark.slow
    def test_random_normal_like_cpu(self, cpu_only=True):
        mean, stddev, seed = 0.0, 1.0, 42

        for rank in range(5, -1, -1):
            if rank > 0:
                low_factor = np.random.randint(low=2, high=4)
                low = int(np.power(1000, 1.0 / rank)) * low_factor
                high = int(np.power(2000, 1.0 / rank)) * np.random.randint(
                    low=low_factor, high=4
                )
                shape = np.random.randint(low=low, high=high, size=rank)
            else:  # one extra test to test more moments
                shape = np.array([10, 10, 10, 10, 10000])

            input_features = [("tensor", datatypes.Array(*shape))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_random_normal_like(
                name="random_normal_like",
                input_name="tensor",
                output_name="output",
                mean=mean,
                stddev=stddev,
                seed=seed,
            )

            inputs = {"tensor": np.random.rand(*shape)}
            expected = {"output": np.random.normal(mean, stddev, shape)}

            if rank > 0:
                CorrectnessTest._compare_moments(
                    builder.spec, inputs, expected, num_moments=2
                )
                self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)
            else:  # one extra test to test more moments
                CorrectnessTest._compare_moments(
                    builder.spec, inputs, expected, num_moments=6
                )

    @pytest.mark.slow
    def test_random_normal_like_gpu(self):
        self.test_random_normal_like_cpu(cpu_only=False)

    def test_random_normal_static_cpu(self, cpu_only=True):

        mean, stddev, seed = 0.0, 1.0, 42

        for rank in range(1, 6):
            low_factor = np.random.randint(low=2, high=4)
            low = int(np.power(1000, 1.0 / rank)) * low_factor
            high = int(np.power(2000, 1.0 / rank)) * np.random.randint(
                low=low_factor, high=4
            )

            shape = np.random.randint(low=low, high=high, size=rank)

            input_features = [("data", datatypes.Array(*shape))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_random_normal_static(
                name="random_normal_static",
                output_name="tmp",
                output_shape=list(shape),
                mean=mean,
                stddev=stddev,
                seed=seed,
            )

            builder.add_elementwise("add_layer", ["data", "tmp"], "output", mode="ADD")

            data = np.zeros(shape)
            inputs = {"data": data}
            expected = {"output": data + np.random.normal(mean, stddev, shape)}

            CorrectnessTest._compare_moments(
                builder.spec, inputs, expected, num_moments=2
            )
            self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)
            self.assertEqual(rank, builder._get_rank("output"))

    def test_random_normal_static_gpu(self):
        self.test_random_normal_static_cpu(cpu_only=False)

    def test_random_normal_dynamic_cpu(self, cpu_only=True):
        mean, stddev, seed = 0.0, 1.0, 42

        for rank in range(1, 6):
            low_factor = np.random.randint(low=2, high=4)
            low = int(np.power(1000, 1.0 / rank)) * low_factor
            high = int(np.power(2000, 1.0 / rank)) * np.random.randint(
                low=low_factor, high=4
            )

            shape = np.random.randint(low=low, high=high, size=rank)

            input_features = [("shape", datatypes.Array(len(shape)))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_random_normal_dynamic(
                name="random_normal_dynamic",
                input_names=["shape"],
                output_name="output",
                mean=mean,
                stddev=stddev,
                seed=seed,
            )

            inputs = {"shape": np.array(shape, np.float)}
            expected = {"output": np.random.normal(mean, stddev, shape)}

            CorrectnessTest._compare_moments(
                builder.spec, inputs, expected, num_moments=2
            )
            self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)
            self.assertEqual(-1, builder._get_rank("output"))

    def test_random_normal_dynamic_gpu(self):
        self.test_random_normal_dynamic_cpu(cpu_only=False)

    def test_random_uniform_like_cpu(self, cpu_only=True):
        minval, maxval, seed = 0.0, 1.0, 42

        for rank in range(1, 6):
            low_factor = np.random.randint(low=2, high=4)
            low = int(np.power(1000, 1.0 / rank)) * low_factor
            high = int(np.power(2000, 1.0 / rank)) * np.random.randint(
                low=low_factor, high=4
            )

            shape = np.random.randint(low=low, high=high, size=rank)

            input_features = [("tensor", datatypes.Array(*shape))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_random_uniform_like(
                name="random_uniform_like",
                input_name="tensor",
                output_name="output",
                minval=minval,
                maxval=maxval,
                seed=seed,
            )

            tensor = np.random.rand(*shape)
            inputs = {"tensor": tensor}
            expected = {"output": np.random.uniform(minval, maxval, shape)}

            CorrectnessTest._compare_moments(builder.spec, inputs, expected)
            self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)
            self.assertEqual(rank, builder._get_rank("output"))

    def test_random_uniform_like_gpu(self):
        self.test_random_uniform_like_cpu(cpu_only=False)

    def test_random_uniform_static_cpu(self, cpu_only=True):
        minval, maxval, seed = 0.0, 1.0, 42

        for rank in range(1, 6):
            low_factor = np.random.randint(low=2, high=4)
            low = int(np.power(1000, 1.0 / rank)) * low_factor
            high = int(np.power(2000, 1.0 / rank)) * np.random.randint(
                low=low_factor, high=4
            )

            shape = np.random.randint(low=low, high=high, size=rank)

            input_features = [("data", datatypes.Array(*shape))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_random_uniform_static(
                name="random_uniform_static",
                output_name="tmp",
                output_shape=list(shape),
                minval=minval,
                maxval=maxval,
                seed=seed,
            )

            builder.add_elementwise("add_layer", ["data", "tmp"], "output", mode="ADD")

            data = np.zeros(shape)
            inputs = {"data": data}
            expected = {"output": data + np.random.uniform(minval, maxval, shape)}

            CorrectnessTest._compare_moments(builder.spec, inputs, expected)
            self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)
            self.assertEqual(rank, builder._get_rank("output"))

    def test_random_uniform_static_gpu(self):
        self.test_random_uniform_static_cpu(cpu_only=False)

    def test_random_uniform_dynamic_cpu(self, cpu_only=True):
        minval, maxval, seed = 0.0, 1.0, 42

        for rank in range(1, 6):
            low_factor = np.random.randint(low=2, high=4)
            low = int(np.power(1000, 1.0 / rank)) * low_factor
            high = int(np.power(2000, 1.0 / rank)) * np.random.randint(
                low=low_factor, high=4
            )

            shape = np.random.randint(low=low, high=high, size=rank)

            input_features = [("shape", datatypes.Array(len(shape)))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_random_uniform_dynamic(
                name="random_uniform_dynamic",
                input_names=["shape"],
                output_name="output",
                minval=minval,
                maxval=maxval,
                seed=seed,
            )

            inputs = {"shape": np.array(shape, np.float)}
            expected = {"output": np.random.uniform(minval, maxval, shape)}

            CorrectnessTest._compare_moments(builder.spec, inputs, expected)
            self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)
            self.assertEqual(-1, builder._get_rank("output"))

    def test_random_uniform_dynamic_gpu(self):
        self.test_random_uniform_dynamic_cpu(cpu_only=False)

    def test_random_bernoulli_like_cpu(self, cpu_only=True):

        prob, seed = 0.5, 42

        for rank in range(1, 6):
            low_factor = np.random.randint(low=2, high=4)
            low = int(np.power(1000, 1.0 / rank)) * low_factor
            high = int(np.power(2000, 1.0 / rank)) * np.random.randint(
                low=low_factor, high=4
            )

            shape = np.random.randint(low=low, high=high, size=rank)

            input_features = [("tensor", datatypes.Array(*shape))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_random_bernoulli_like(
                name="random_bernoulli_like",
                input_name="tensor",
                output_name="output",
                prob=prob,
                seed=seed,
            )

            tensor = np.random.rand(*shape)
            inputs = {"tensor": tensor}
            expected = {"output": np.random.binomial(1, prob, shape)}

            CorrectnessTest._compare_moments(builder.spec, inputs, expected)
            self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)

    def test_random_bernoulli_like_gpu(self):
        self.test_random_bernoulli_like_cpu(cpu_only=False)

    def test_random_bernoulli_static_cpu(self, cpu_only=True):
        prob, seed = 0.5, 42

        for rank in range(1, 6):
            low_factor = np.random.randint(low=2, high=4)
            low = int(np.power(1000, 1.0 / rank)) * low_factor
            high = int(np.power(2000, 1.0 / rank)) * np.random.randint(
                low=low_factor, high=4
            )

            shape = np.random.randint(low=low, high=high, size=rank)

            input_features = [("data", datatypes.Array(*shape))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_random_bernoulli_static(
                name="random_bernoulli_static",
                output_name="tmp",
                output_shape=list(shape),
                prob=prob,
                seed=seed,
            )

            builder.add_elementwise("add_layer", ["data", "tmp"], "output", mode="ADD")

            data = np.zeros(shape)
            inputs = {"data": data}
            expected = {"output": data + np.random.binomial(1, prob, shape)}

            CorrectnessTest._compare_moments(builder.spec, inputs, expected)
            self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)

    def test_random_bernoulli_static_gpu(self):
        self.test_random_bernoulli_static_cpu(cpu_only=False)

    def test_random_bernoulli_dynamic_cpu(self, cpu_only=True):
        prob, seed = 0.5, 42

        for rank in range(1, 6):
            low_factor = np.random.randint(low=2, high=4)
            low = int(np.power(1000, 1.0 / rank)) * low_factor
            high = int(np.power(2000, 1.0 / rank)) * np.random.randint(
                low=low_factor, high=4
            )

            shape = np.random.randint(low=low, high=high, size=rank)

            input_features = [("shape", datatypes.Array(len(shape)))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_random_bernoulli_dynamic(
                name="random_bernoulli_dynamic",
                input_names=["shape"],
                output_name="output",
                prob=prob,
                seed=seed,
            )

            inputs = {"shape": np.array(shape, np.float)}
            expected = {"output": np.random.binomial(1, prob, shape)}

            CorrectnessTest._compare_moments(builder.spec, inputs, expected)
            self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)

    def test_random_bernoulli_dynamic_gpu(self):
        self.test_random_bernoulli_dynamic_cpu(cpu_only=False)

    def test_categorical_distribution_cpu_shapes(self):

        for rank in range(1, 6):
            shape = np.random.randint(low=2, high=8, size=rank)
            num_samples = np.random.randint(low=10, high=1000)

            input_features = [("data", datatypes.Array(*shape))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_categorical_distribution(
                name="categorical_distribution",
                input_name="data",
                output_name="output",
                num_samples=num_samples,
            )

            x = np.random.randint(low=0, high=20, size=shape).astype(np.float32)
            inputs = {"data": x}
            shape[-1] = num_samples
            expected = {"output": np.random.rand(*shape)}

            self._test_model(
                builder.spec,
                inputs,
                expected,
                useCPUOnly=True,
                validate_shapes_only=True,
            )

    @pytest.mark.xfail(
        reason="rdar://64153463 ([GitLab CI] test_categorical_distribution_cpu_probs failing)"
    )
    def test_categorical_distribution_cpu_logits(self):
        def softmax(data):
            e_data = np.exp(data - np.max(data))
            return e_data / e_data.sum()

        num_samples, num_class = 50000, 10
        input_name, output_name = "data", "output"

        shapes = [
            (2, num_class),
            (2, 1, num_class),
            (1, 2, num_class),
            (2, 1, 1, num_class),
            (1, 2, 1, num_class),
            (1, 1, 2, num_class),
            (2, 1, 1, 1, num_class),
            (1, 2, 1, 1, num_class),
            (1, 1, 2, 1, num_class),
            (1, 1, 1, 2, num_class),
        ]

        for shape in shapes:
            input_features = [("data", datatypes.Array(*shape))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_categorical_distribution(
                name="categorical_distribution",
                input_name=input_name,
                output_name=output_name,
                num_samples=num_samples,
                is_logits=True,
                seed=42,
            )

            x = np.random.rand(*shape)
            inputs = {input_name: x}

            model = builder.spec
            if isinstance(model, _string_types):
                model = coremltools.models.MLModel(model)

            model = coremltools.models.MLModel(model, useCPUOnly=True)
            prediction = model.predict(inputs, useCPUOnly=True)

            # validate each distribution separately
            logits = x.reshape(2, num_class)
            probs = [softmax(logits[0]), softmax(logits[1])]

            ref0 = np.random.multinomial(num_samples, probs[0])
            ref1 = np.random.multinomial(num_samples, probs[1])

            pre0 = prediction[output_name].reshape(2, num_samples)[0]
            pre1 = prediction[output_name].reshape(2, num_samples)[1]

            expected = {output_name: np.stack((pre0, pre1))}

            # convert to bincount and validate probabilities
            pre0 = np.bincount(np.array(pre0).astype(np.int), minlength=num_class)
            pre1 = np.bincount(np.array(pre1).astype(np.int), minlength=num_class)

            np.testing.assert_allclose(
                np.true_divide(pre0, num_samples), probs[0], atol=1e-2
            )
            np.testing.assert_allclose(
                np.true_divide(pre0, num_samples),
                np.true_divide(ref0, num_samples),
                atol=1e-2,
            )

            np.testing.assert_allclose(
                np.true_divide(pre1, num_samples), probs[1], atol=1e-2
            )
            np.testing.assert_allclose(
                np.true_divide(pre1, num_samples),
                np.true_divide(ref1, num_samples),
                atol=1e-2,
            )

            self._test_model(
                model,
                inputs,
                expected,
                useCPUOnly=True,
                output_name_shape_dict={"output": prediction["output"].shape},
            )

    @pytest.mark.xfail(
        reason="rdar://64153463 ([GitLab CI] test_categorical_distribution_cpu_probs failing)"
    )
    def test_categorical_distribution_cpu_probs(self):
        def softmax(data):
            e_data = np.exp(data - np.max(data))
            return e_data / e_data.sum()

        num_samples, num_class = 50000, 10
        input_name, output_name = "data", "output"

        shapes = [
            (2, num_class),
            (2, 1, num_class),
            (1, 2, num_class),
            (2, 1, 1, num_class),
            (1, 2, 1, num_class),
            (1, 1, 2, num_class),
            (2, 1, 1, 1, num_class),
            (1, 2, 1, 1, num_class),
            (1, 1, 2, 1, num_class),
            (1, 1, 1, 2, num_class),
        ]

        for shape in shapes:
            input_features = [("data", datatypes.Array(*shape))]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, [("output", None)], disable_rank5_shape_mapping=True
            )

            builder.add_categorical_distribution(
                name="categorical_distribution",
                input_name=input_name,
                output_name=output_name,
                num_samples=num_samples,
                is_logits=False,
                seed=42,
            )

            x = np.random.rand(*shape)
            probs = x.reshape(2, num_class)
            probs[0], probs[1] = softmax(probs[0]), softmax(probs[1])
            inputs = {input_name: np.reshape(probs, shape)}

            model = builder.spec
            if isinstance(model, _string_types):
                model = coremltools.models.MLModel(model)

            model = coremltools.models.MLModel(model, useCPUOnly=True)
            prediction = model.predict(inputs, useCPUOnly=True)

            # validate each distribution separately
            probs = probs.reshape(2, num_class)

            ref0 = np.random.multinomial(num_samples, probs[0])
            ref1 = np.random.multinomial(num_samples, probs[1])

            pre0 = prediction[output_name].reshape(2, num_samples)[0]
            pre1 = prediction[output_name].reshape(2, num_samples)[1]

            expected = {output_name: np.stack((pre0, pre1))}

            # convert to bincount and validate probabilities
            pre0 = np.bincount(np.array(pre0).astype(np.int), minlength=num_class)
            pre1 = np.bincount(np.array(pre1).astype(np.int), minlength=num_class)

            np.testing.assert_allclose(
                np.true_divide(pre0, num_samples), probs[0], atol=1e-2
            )
            np.testing.assert_allclose(
                np.true_divide(pre0, num_samples),
                np.true_divide(ref0, num_samples),
                atol=1e-2,
            )

            np.testing.assert_allclose(
                np.true_divide(pre1, num_samples), probs[1], atol=1e-2
            )
            np.testing.assert_allclose(
                np.true_divide(pre1, num_samples),
                np.true_divide(ref1, num_samples),
                atol=1e-2,
            )

            self._test_model(
                model,
                inputs,
                expected,
                useCPUOnly=True,
                output_name_shape_dict={"output": prediction["output"].shape},
            )

    def test_reverse_cpu(self, cpu_only=True):

        for rank in range(1, 6):
            for _ in range(20):
                input_shape = np.random.randint(low=2, high=8, size=rank)
                reverse_dim = [np.random.choice([True, False]) for _ in range(rank)]
                axes = [i for i in range(rank) if reverse_dim[i] == True]

                input_features = [("data", datatypes.Array(*input_shape))]
                output_features = [("output", None)]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_reverse("reverse", "data", "output", reverse_dim)

                x = np.random.rand(*input_shape)
                input = {"data": x}
                expected = {"output": np.flip(x, axis=axes)}

                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_reverse_gpu(self):
        self.test_reverse_cpu(cpu_only=False)

    def test_matrix_band_part_cpu(self, cpu_only=True):

        for rank in range(2, 6):
            for _ in range(20):
                num_lower = np.random.randint(low=-7, high=8)
                num_upper = np.random.randint(low=-7, high=8)
                shape = np.random.randint(low=2, high=6, size=rank)
                input_features = [("data", datatypes.Array(*shape))]
                output_features = [("output", None)]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_matrix_band_part(
                    "matrix_band_part",
                    "data",
                    "output",
                    num_lower=num_lower,
                    num_upper=num_upper,
                )

                x = np.random.rand(*shape)
                input = {"data": x}

                rows, cols = shape[-2:]
                band = np.ones((rows, cols))
                for m in range(rows):
                    for n in range(cols):
                        band[m, n] = (num_lower < 0 or (m - n) <= num_lower) and (
                            num_upper < 0 or (n - m) <= num_upper
                        )

                expected = {"output": np.multiply(band, x)}
                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_matrix_band_part_gpu(self):
        self.test_matrix_band_part_cpu(cpu_only=False)

    def test_flatten_to_2d_cpu(self, cpu_only=True):

        for rank in range(1, 6):
            for axis in range(-rank, rank + 1):
                shape = np.random.randint(low=2, high=6, size=rank)
                input_features = [("data", datatypes.Array(*shape))]
                output_features = [("output", None)]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_flatten_to_2d("flatten_to_2d", "data", "output", axis=axis)

                x = np.random.rand(*shape)
                np_axis = axis + rank if axis < 0 else axis
                pl, pr = 1, 1
                for i in range(0, np_axis):
                    pl *= shape[i]
                for i in range(np_axis, len(shape)):
                    pr *= shape[i]

                new_shape = [pl, pr]
                ref = x.reshape(new_shape)

                input = {"data": x}
                expected = {"output": ref}
                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)
                self.assertEqual(2, builder._get_rank("output"))

    def test_flatten_to_2d_gpu(self):
        self.test_flatten_to_2d_cpu(cpu_only=False)

    def test_reshape_like_cpu(self, cpu_only=True):

        for rank in range(1, 6):
            for _ in range(20):
                input_shape = np.random.randint(low=2, high=8, size=rank)
                n = int(np.prod(input_shape))
                divisors = [d for d in range(1, n) if n % d == 0]
                target_rank = np.random.randint(low=2, high=6)
                target_shape = [1]
                for i in range(target_rank - 1):
                    dim_size = np.random.choice(divisors)
                    while n % (np.prod(target_shape) * dim_size) != 0:
                        dim_size = np.random.choice(divisors)
                    target_shape.append(dim_size)
                target_shape[0] = n // np.prod(target_shape)

                np.random.shuffle(target_shape)
                input_features = [
                    ("data", datatypes.Array(*input_shape)),
                    ("tensor", datatypes.Array(*target_shape)),
                ]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, [("output", None)], disable_rank5_shape_mapping=True
                )

                builder.add_reshape_like(
                    name="reshape_like",
                    input_names=["data", "tensor"],
                    output_name="output",
                )

                data = np.random.rand(*input_shape)
                tensor = np.random.rand(*target_shape)
                inputs = {"data": data, "tensor": tensor}
                expected = {"output": np.reshape(data, target_shape)}

                self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)
                self.assertEqual(target_rank, builder._get_rank("output"))

    @pytest.mark.xfail(reason="Fixed in https://github.com/apple/coremltools/pull/634")
    def test_reshape_like_gpu(self):
        self.test_reshape_like_cpu(cpu_only=False)

    def test_reshape_static_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            for _ in range(20):
                input_shape = np.random.randint(low=2, high=8, size=rank)
                n = int(np.prod(input_shape))
                divisors = [d for d in range(1, n) if n % d == 0]
                target_rank = np.random.randint(low=2, high=6)

                target_shape = [1]
                for i in range(target_rank - 1):
                    dim_size = np.random.choice(divisors)
                    while n % (np.prod(target_shape) * dim_size) != 0:
                        dim_size = np.random.choice(divisors)
                    target_shape.append(dim_size)

                target_shape[0] = -1

                np.random.shuffle(target_shape)
                input_features = [("data", datatypes.Array(*input_shape))]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, [("output", None)], disable_rank5_shape_mapping=True
                )

                builder.add_reshape_static(
                    name="reshape_static",
                    input_name="data",
                    output_name="output",
                    output_shape=target_shape,
                )

                data = np.random.rand(*input_shape)
                inputs = {"data": data}
                expected = {"output": np.reshape(data, target_shape)}

                self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)
                self.assertEqual(len(target_shape), builder._get_rank("output"))

    def test_reshape_static_gpu(self):
        self.test_reshape_static_cpu(cpu_only=False)

    def test_reshape_dynamic_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            for _ in range(20):
                input_shape = np.random.randint(low=2, high=8, size=rank)
                n = int(np.prod(input_shape))
                divisors = [d for d in range(1, n) if n % d == 0]
                target_rank = np.random.randint(low=2, high=6)

                target_shape = [1]
                for i in range(target_rank - 1):
                    dim_size = np.random.choice(divisors)
                    while n % (np.prod(target_shape) * dim_size) != 0:
                        dim_size = np.random.choice(divisors)
                    target_shape.append(dim_size)

                target_shape[0] = -1

                np.random.shuffle(target_shape)
                input_features = [
                    ("data", datatypes.Array(*input_shape)),
                    ("shape", datatypes.Array(len(target_shape))),
                ]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, [("output", None)], disable_rank5_shape_mapping=True
                )

                builder.add_reshape_dynamic(
                    name="reshape_dynamic",
                    input_names=["data", "shape"],
                    output_name="output",
                )

                data = np.random.rand(*input_shape)
                inputs = {"data": data, "shape": np.array(target_shape, dtype="float")}
                expected = {"output": np.reshape(data, target_shape)}

                self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)
                self.assertEqual(-1, builder._get_rank("output"))

    def test_reshape_dynamic_gpu(self):
        self.test_reshape_dynamic_cpu(cpu_only=False)

    def test_reduce_sum_cpu(self, cpu_only=True):

        for rank in range(1, 6):
            axes_list = [
                axes
                for length in range(1, rank + 1)
                for axes in itertools.combinations(range(rank), length)
            ]
            axes_list.append(None)

            for axes in axes_list:
                if axes:
                    axes = tuple(
                        [
                            axis if np.random.choice([True, False]) else axis - rank
                            for axis in axes
                        ]
                    )
                    reduce_all = False
                else:
                    reduce_all = True

                for keep_dims in [True, False]:
                    input_shape = np.random.randint(low=2, high=5, size=rank)

                    input_features = [("data", datatypes.Array(*input_shape))]
                    output_features = [("output", None)]

                    builder = neural_network.NeuralNetworkBuilder(
                        input_features,
                        output_features,
                        disable_rank5_shape_mapping=True,
                    )

                    builder.add_reduce_sum(
                        "reduce",
                        "data",
                        "output",
                        axes,
                        keepdims=keep_dims,
                        reduce_all=reduce_all,
                    )

                    x = np.random.rand(*input_shape)
                    input = {"data": x}
                    expected = {"output": np.add.reduce(x, axes, keepdims=keep_dims)}

                    self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)
                    expected_rank = len(expected["output"].shape)
                    if expected_rank == 0:
                        expected_rank = 1
                    self.assertEqual(expected_rank, builder._get_rank("output"))

    def test_reduce_sum_gpu(self):
        self.test_reduce_sum_cpu(cpu_only=False)

    def test_reduce_prod_cpu(self, cpu_only=True):

        for rank in range(1, 6):
            axes_list = [
                axes
                for length in range(1, rank + 1)
                for axes in itertools.combinations(range(rank), length)
            ]
            axes_list.append(None)

            for axes in axes_list:
                if axes:
                    axes = tuple(
                        [
                            axis if np.random.choice([True, False]) else axis - rank
                            for axis in axes
                        ]
                    )
                    reduce_all = False
                else:
                    reduce_all = True

                for keep_dims in [True, False]:
                    input_shape = np.random.randint(low=2, high=5, size=rank)

                    input_features = [("data", datatypes.Array(*input_shape))]
                    output_features = [("output", None)]

                    builder = neural_network.NeuralNetworkBuilder(
                        input_features,
                        output_features,
                        disable_rank5_shape_mapping=True,
                    )

                    builder.add_reduce_prod(
                        "reduce",
                        "data",
                        "output",
                        axes,
                        keepdims=keep_dims,
                        reduce_all=reduce_all,
                    )

                    x = np.random.rand(*input_shape)
                    input = {"data": x}
                    expected = {
                        "output": np.multiply.reduce(x, axes, keepdims=keep_dims)
                    }

                    self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)
                    expected_rank = len(expected["output"].shape)
                    if expected_rank == 0:
                        expected_rank = 1
                    self.assertEqual(expected_rank, builder._get_rank("output"))

    def test_reduce_prod_gpu(self):
        self.test_reduce_prod_cpu(cpu_only=False)

    def test_reduce_mean_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            axes_list = [
                axes
                for length in range(1, rank + 1)
                for axes in itertools.combinations(range(rank), length)
            ]
            axes_list.append(None)

            for axes in axes_list:
                if axes:
                    axes = tuple(
                        [
                            axis if np.random.choice([True, False]) else axis - rank
                            for axis in axes
                        ]
                    )
                    reduce_all = False
                else:
                    reduce_all = True

                for keep_dims in [True, False]:
                    input_shape = np.random.randint(low=2, high=5, size=rank)

                    input_features = [("data", datatypes.Array(*input_shape))]
                    output_features = [("output", None)]

                    builder = neural_network.NeuralNetworkBuilder(
                        input_features,
                        output_features,
                        disable_rank5_shape_mapping=True,
                    )

                    builder.add_reduce_mean(
                        "reduce",
                        "data",
                        "output",
                        axes,
                        keepdims=keep_dims,
                        reduce_all=reduce_all,
                    )

                    x = np.random.rand(*input_shape)
                    input = {"data": x}
                    expected = {"output": np.mean(x, axes, keepdims=keep_dims)}

                    self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_reduce_mean_gpu(self):
        self.test_reduce_mean_cpu(cpu_only=False)

    def test_reduce_max_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            axes_list = [
                axes
                for length in range(1, rank + 1)
                for axes in itertools.combinations(range(rank), length)
            ]
            axes_list.append(None)

            for axes in axes_list:
                if axes:
                    axes = tuple(
                        [
                            axis if np.random.choice([True, False]) else axis - rank
                            for axis in axes
                        ]
                    )
                    reduce_all = False
                else:
                    reduce_all = True

                for keep_dims in [True, False]:
                    input_shape = np.random.randint(low=2, high=5, size=rank)

                    input_features = [("data", datatypes.Array(*input_shape))]
                    output_features = [("output", None)]

                    builder = neural_network.NeuralNetworkBuilder(
                        input_features,
                        output_features,
                        disable_rank5_shape_mapping=True,
                    )

                    builder.add_reduce_max(
                        "reduce",
                        "data",
                        "output",
                        axes,
                        keepdims=keep_dims,
                        reduce_all=reduce_all,
                    )

                    x = np.random.rand(*input_shape)
                    input = {"data": x}
                    expected = {
                        "output": np.maximum.reduce(x, axes, keepdims=keep_dims)
                    }

                    self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_reduce_max_gpu(self):
        self.test_reduce_max_cpu(cpu_only=False)

    def test_reduce_min_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            axes_list = [
                axes
                for length in range(1, rank + 1)
                for axes in itertools.combinations(range(rank), length)
            ]
            axes_list.append(None)

            for axes in axes_list:
                if axes:
                    axes = tuple(
                        [
                            axis if np.random.choice([True, False]) else axis - rank
                            for axis in axes
                        ]
                    )
                    reduce_all = False
                else:
                    reduce_all = True

                for keep_dims in [True, False]:
                    input_shape = np.random.randint(low=2, high=5, size=rank)

                    input_features = [("data", datatypes.Array(*input_shape))]
                    output_features = [("output", None)]

                    builder = neural_network.NeuralNetworkBuilder(
                        input_features,
                        output_features,
                        disable_rank5_shape_mapping=True,
                    )

                    builder.add_reduce_min(
                        "reduce",
                        "data",
                        "output",
                        axes,
                        keepdims=keep_dims,
                        reduce_all=reduce_all,
                    )

                    x = np.random.rand(*input_shape)
                    input = {"data": x}
                    expected = {
                        "output": np.minimum.reduce(x, axes, keepdims=keep_dims)
                    }

                    self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_reduce_min_gpu(self):
        self.test_reduce_min_cpu(cpu_only=False)

    def test_reduce_l2_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            axes_list = [
                axes
                for length in range(1, rank + 1)
                for axes in itertools.combinations(range(rank), length)
            ]
            axes_list.append(None)

            for axes in axes_list:
                if axes:
                    axes = tuple(
                        [
                            axis if np.random.choice([True, False]) else axis - rank
                            for axis in axes
                        ]
                    )
                    reduce_all = False
                else:
                    reduce_all = True

                for keep_dims in [True, False]:
                    input_shape = np.random.randint(low=2, high=5, size=rank)

                    input_features = [("data", datatypes.Array(*input_shape))]
                    output_features = [("output", None)]

                    builder = neural_network.NeuralNetworkBuilder(
                        input_features,
                        output_features,
                        disable_rank5_shape_mapping=True,
                    )

                    builder.add_reduce_l2(
                        "reduce",
                        "data",
                        "output",
                        axes,
                        keepdims=keep_dims,
                        reduce_all=reduce_all,
                    )

                    x = np.random.rand(*input_shape)
                    input = {"data": x}
                    expected = {
                        "output": np.sqrt(
                            np.sum(np.square(x), axis=axes, keepdims=keep_dims)
                        )
                    }

                    self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_reduce_l2_gpu(self):
        self.test_reduce_l2_cpu(cpu_only=False)

    def test_reduce_l1_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            axes_list = [
                axes
                for length in range(1, rank + 1)
                for axes in itertools.combinations(range(rank), length)
            ]
            axes_list.append(None)

            for axes in axes_list:
                if axes:
                    axes = tuple(
                        [
                            axis if np.random.choice([True, False]) else axis - rank
                            for axis in axes
                        ]
                    )
                    reduce_all = False
                else:
                    reduce_all = True

                for keep_dims in [True, False]:
                    input_shape = np.random.randint(low=2, high=5, size=rank)

                    input_features = [("data", datatypes.Array(*input_shape))]
                    output_features = [("output", None)]

                    builder = neural_network.NeuralNetworkBuilder(
                        input_features,
                        output_features,
                        disable_rank5_shape_mapping=True,
                    )

                    builder.add_reduce_l1(
                        "reduce",
                        "data",
                        "output",
                        axes,
                        keepdims=keep_dims,
                        reduce_all=reduce_all,
                    )

                    x = np.random.rand(*input_shape)
                    input = {"data": x}
                    expected = {
                        "output": np.sum(np.abs(x), axis=axes, keepdims=keep_dims)
                    }

                    self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_reduce_l1_gpu(self):
        self.test_reduce_l1_cpu(cpu_only=False)

    def test_reduce_sumsquare_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            axes_list = [
                axes
                for length in range(1, rank + 1)
                for axes in itertools.combinations(range(rank), length)
            ]
            axes_list.append(None)

            for axes in axes_list:
                if axes:
                    axes = tuple(
                        [
                            axis if np.random.choice([True, False]) else axis - rank
                            for axis in axes
                        ]
                    )
                    reduce_all = False
                else:
                    reduce_all = True

                for keep_dims in [True, False]:
                    input_shape = np.random.randint(low=2, high=5, size=rank)

                    input_features = [("data", datatypes.Array(*input_shape))]
                    output_features = [("output", None)]

                    builder = neural_network.NeuralNetworkBuilder(
                        input_features,
                        output_features,
                        disable_rank5_shape_mapping=True,
                    )

                    builder.add_reduce_sumsquare(
                        "reduce",
                        "data",
                        "output",
                        axes,
                        keepdims=keep_dims,
                        reduce_all=reduce_all,
                    )

                    x = np.random.rand(*input_shape)
                    input = {"data": x}
                    expected = {
                        "output": np.sum(np.square(x), axis=axes, keepdims=keep_dims)
                    }

                    self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_reduce_sumsquare_gpu(self):
        self.test_reduce_sumsquare_cpu(cpu_only=False)

    def test_reduce_logsum_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            axes_list = [
                axes
                for length in range(1, rank + 1)
                for axes in itertools.combinations(range(rank), length)
            ]
            axes_list.append(None)

            for axes in axes_list:
                if axes:
                    axes = tuple(
                        [
                            axis if np.random.choice([True, False]) else axis - rank
                            for axis in axes
                        ]
                    )
                    reduce_all = False
                else:
                    reduce_all = True

                for keep_dims in [True, False]:
                    input_shape = np.random.randint(low=2, high=5, size=rank)

                    input_features = [("data", datatypes.Array(*input_shape))]
                    output_features = [("output", None)]

                    builder = neural_network.NeuralNetworkBuilder(
                        input_features,
                        output_features,
                        disable_rank5_shape_mapping=True,
                    )

                    builder.add_reduce_logsum(
                        "reduce",
                        "data",
                        "output",
                        axes,
                        keepdims=keep_dims,
                        reduce_all=reduce_all,
                    )

                    x = np.random.rand(*input_shape)
                    input = {"data": x}
                    expected = {
                        "output": np.log(np.sum(x, axis=axes, keepdims=keep_dims))
                    }

                    self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_reduce_logsum_gpu(self):
        self.test_reduce_logsum_cpu(cpu_only=False)

    def test_reduce_logsumexp_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            axes_list = [
                axes
                for length in range(1, rank + 1)
                for axes in itertools.combinations(range(rank), length)
            ]
            axes_list.append(None)

            for axes in axes_list:
                if axes:
                    axes = tuple(
                        [
                            axis if np.random.choice([True, False]) else axis - rank
                            for axis in axes
                        ]
                    )
                    reduce_all = False
                else:
                    reduce_all = True

                for keep_dims in [True, False]:
                    input_shape = np.random.randint(low=2, high=5, size=rank)

                    input_features = [("data", datatypes.Array(*input_shape))]
                    output_features = [("output", None)]

                    builder = neural_network.NeuralNetworkBuilder(
                        input_features,
                        output_features,
                        disable_rank5_shape_mapping=True,
                    )

                    builder.add_reduce_logsumexp(
                        "reduce",
                        "data",
                        "output",
                        axes,
                        keepdims=keep_dims,
                        reduce_all=reduce_all,
                    )

                    x = np.random.rand(*input_shape)
                    input = {"data": x}
                    expected = {
                        "output": np.log(
                            np.sum(np.exp(x), axis=axes, keepdims=keep_dims)
                        )
                    }

                    self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_reduce_logsumexp_gpu(self):
        self.test_reduce_logsumexp_cpu(cpu_only=False)

    @unittest.skipIf(not _HAS_TF, MSG_TF1_NOT_FOUND)
    def test_reverse_sequence_cpu(self, cpu_only=True):
        for rank in range(2, 6):
            for i in range(20):
                input_shape = np.random.randint(low=2, high=6, size=rank)

                seq_axis = np.random.randint(low=-rank, high=rank)
                batch_axis = np.random.randint(low=-rank, high=rank)
                pos_batch_axis = batch_axis if batch_axis >= 0 else rank + batch_axis
                pos_seq_axis = seq_axis if seq_axis >= 0 else rank + seq_axis
                while pos_batch_axis >= pos_seq_axis:
                    seq_axis = np.random.randint(low=-rank, high=rank)
                    batch_axis = np.random.randint(low=-rank, high=rank)
                    pos_batch_axis = (
                        batch_axis if batch_axis >= 0 else rank + batch_axis
                    )
                    pos_seq_axis = seq_axis if seq_axis >= 0 else rank + seq_axis

                input_features = [
                    ("data", datatypes.Array(*input_shape)),
                    ("lengths", datatypes.Array(input_shape[batch_axis])),
                ]

                output_features = [("output", None)]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_reverse_sequence(
                    "reverse_sequence",
                    ["data", "lengths"],
                    "output",
                    batch_axis=batch_axis,
                    seq_axis=seq_axis,
                )

                data = np.random.rand(*input_shape)
                lengths = np.random.randint(
                    low=0, high=input_shape[seq_axis], size=input_shape[batch_axis]
                )

                input = {"data": data, "lengths": lengths.astype(np.float32)}

                with tf.Graph().as_default(), tf.Session() as sess:
                    tf_op = tf.reverse_sequence(
                        input=data,
                        seq_lengths=lengths,
                        seq_axis=pos_seq_axis,
                        batch_axis=pos_batch_axis,
                    )
                    expected = {"output": sess.run(tf_op)}

                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_reverse_sequence_gpu(self):
        self.test_reverse_sequence_cpu(cpu_only=False)

    def test_where_nonzero_cpu(self, cpu_only=True):

        for rank in range(1, 6):
            for i in range(10):
                shape = np.random.randint(low=2, high=8, size=rank)

                input_features = [("data", datatypes.Array(*shape))]
                output_features = [("output", None)]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_where_nonzero("multi_indices", "data", "output")

                x = np.random.randint(low=0, high=3, size=shape)

                input = {"data": x.astype(np.float)}
                expected = {"output": np.transpose(np.nonzero(x)).astype(np.float)}
                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_where_nonzero_gpu(self):
        self.test_where_nonzero_cpu(cpu_only=False)

    def test_gather_cpu(self, cpu_only=True):
        for rankParams, rankIndices in [
            (i, j) for i in range(1, 6) for j in range(1, 6)
        ]:
            for axis in range(-rankParams, rankParams):
                shapeParams = np.random.randint(low=2, high=5, size=rankParams)
                shapeIndices = np.random.randint(low=2, high=5, size=rankIndices)
                input_shapes = [shapeParams, shapeIndices]
                posAxis = axis if axis >= 0 else axis + rankParams
                output_shape = (
                    list(shapeParams[:posAxis])
                    + list(shapeIndices)
                    + list(shapeParams[posAxis + 1 :])
                )

                if len(output_shape) > 5:
                    continue

                input_names = ["params", "indices"]
                input_features = [
                    ("params", datatypes.Array(*input_shapes[0])),
                    ("indices", datatypes.Array(*input_shapes[1])),
                ]
                output_features = [("output", None)]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_gather(
                    name="gather",
                    input_names=input_names,
                    output_name="output",
                    axis=axis,
                )

                a = np.random.rand(*input_shapes[0])
                b = np.random.randint(
                    -shapeParams[axis], shapeParams[axis], size=shapeIndices
                )
                input = {"params": a, "indices": b.astype(np.float)}
                expected = {"output": np.take(a, b, axis=axis)}

                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)
                self.assertEqual(
                    len(expected["output"].shape), builder._get_rank("output")
                )

    def test_gather_gpu(self):
        self.test_gather_cpu(cpu_only=False)

    def test_gather_along_axis_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            for axis in range(-rank, rank):
                for _ in range(5):
                    params_shape = np.random.randint(low=2, high=8, size=rank)
                    indices_shape = np.copy(params_shape)
                    indices_shape[axis] = np.random.randint(low=1, high=8)

                    input_features = [
                        ("params", datatypes.Array(*params_shape)),
                        ("indices", datatypes.Array(*indices_shape)),
                    ]
                    output_features = [("output", None)]

                    builder = neural_network.NeuralNetworkBuilder(
                        input_features,
                        output_features,
                        disable_rank5_shape_mapping=True,
                    )
                    builder.add_gather_along_axis(
                        "gather_along_axis", ["params", "indices"], "output", axis=axis
                    )

                    a = np.random.rand(*params_shape)
                    b = np.random.randint(
                        -params_shape[axis], params_shape[axis], size=indices_shape
                    )

                    input = {"params": a, "indices": b.astype(np.float)}
                    expected = {"output": np.take_along_axis(a, b, axis=axis)}
                    self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)
                    self.assertEqual(
                        len(expected["output"].shape), builder._get_rank("output")
                    )

    def test_gather_along_axis_gpu(self):
        self.test_gather_along_axis_cpu(cpu_only=False)

    @unittest.skipIf(not _HAS_TF, MSG_TF1_NOT_FOUND)
    def test_gather_nd_cpu(self, cpu_only=True):
        for params_rank, indices_rank in [
            (i, j) for i in range(1, 6) for j in range(1, 6)
        ]:
            params_shape = np.random.randint(low=2, high=8, size=params_rank)
            indices_shape = np.random.randint(low=2, high=8, size=indices_rank)
            indices_shape[-1] = np.random.randint(low=1, high=params_rank + 1)

            for _ in range(5):
                input_features = [
                    ("params", datatypes.Array(*params_shape)),
                    ("indices", datatypes.Array(*indices_shape)),
                ]
                output_features = [("output", None)]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                output_shape = list(indices_shape[:-1]) + list(
                    params_shape[indices_shape[-1] :]
                )
                if len(output_shape) > 5:
                    continue

                builder.add_gather_nd("gather_nd", ["params", "indices"], "output")

                a = np.random.rand(*params_shape)
                indices_list = []
                for i in range(indices_shape[-1]):
                    indices_list.append(
                        np.random.randint(0, params_shape[i], size=indices_shape[:-1])
                    )

                indices = np.stack(indices_list, axis=-1)
                input = {"params": a, "indices": indices.astype(np.float)}

                with tf.Graph().as_default(), tf.Session() as sess:
                    tf_op = tf.gather_nd(a, indices)
                    expected = {"output": sess.run(tf_op)}

                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)
                self.assertEqual(-1, builder._get_rank("output"))

    def test_gather_nd_gpu(self):
        self.test_gather_nd_cpu(cpu_only=False)

    @unittest.skipIf(not _HAS_TF, MSG_TF1_NOT_FOUND)
    def test_scatter_cpu(self, cpu_only=True):
        for ref_rank, indices_rank in [
            (i, j) for i in range(1, 6) for j in range(1, 6)
        ]:
            for accumulate_mode in ["UPDATE", "ADD", "SUB", "MUL", "DIV", "MAX", "MIN"]:
                for _ in range(5):
                    ref_shape = np.random.randint(low=2, high=8, size=ref_rank)
                    indices_shape = np.random.randint(low=2, high=8, size=indices_rank)
                    updates_shape = list(indices_shape) + list(ref_shape[1:])

                    input_features = [
                        ("ref", datatypes.Array(*ref_shape)),
                        ("indices", datatypes.Array(*indices_shape)),
                        ("updates", datatypes.Array(*updates_shape)),
                    ]
                    output_features = [("output", None)]

                    builder = neural_network.NeuralNetworkBuilder(
                        input_features,
                        output_features,
                        disable_rank5_shape_mapping=True,
                    )

                    if len(updates_shape) > 5:
                        continue

                    builder.add_scatter(
                        "scatter",
                        ["ref", "indices", "updates"],
                        "output",
                        axis=0,
                        mode=accumulate_mode,
                    )

                    ref = np.random.rand(*ref_shape)
                    updates = np.random.rand(*updates_shape)
                    if accumulate_mode == "DIV":
                        updates += 10.0
                    indices = np.random.randint(0, ref_shape[0], size=indices_shape)
                    input = {
                        "ref": ref,
                        "indices": indices.astype(np.float),
                        "updates": updates,
                    }

                    with tf.Graph().as_default(), tf.Session() as sess:
                        tf_output = tf.Variable(ref)
                        sess.run(tf.global_variables_initializer())
                        if accumulate_mode == "UPDATE":
                            sess.run(tf.scatter_update(tf_output, indices, updates))
                        if accumulate_mode == "ADD":
                            sess.run(tf.scatter_add(tf_output, indices, updates))
                        if accumulate_mode == "SUB":
                            sess.run(tf.scatter_sub(tf_output, indices, updates))
                        if accumulate_mode == "MUL":
                            sess.run(tf.scatter_mul(tf_output, indices, updates))
                        if accumulate_mode == "DIV":
                            sess.run(tf.scatter_div(tf_output, indices, updates))
                        if accumulate_mode == "MAX":
                            sess.run(tf.scatter_max(tf_output, indices, updates))
                        if accumulate_mode == "MIN":
                            sess.run(tf.scatter_min(tf_output, indices, updates))
                        expected = {"output": sess.run(tf_output)}

                    self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_scatter_gpu(self):
        self.test_scatter_cpu(cpu_only=False)

    def test_gather_scatter_multiple_axis_cpu(self, cpu_only=True):

        for params_rank, indices_rank in [
            (i, j) for i in range(1, 6) for j in range(1, 6)
        ]:
            for axis in range(-params_rank, params_rank):
                for _ in range(5):
                    params_shape = np.random.randint(low=2, high=8, size=params_rank)
                    indices_shape = np.random.randint(low=2, high=8, size=indices_rank)

                    pos_axis = axis if axis >= 0 else axis + params_rank
                    output_shape = (
                        list(params_shape[:pos_axis])
                        + list(indices_shape)
                        + list(params_shape[pos_axis + 1 :])
                    )

                    if len(output_shape) > 5:
                        continue

                    input_features = [
                        ("params", datatypes.Array(*params_shape)),
                        ("indices", datatypes.Array(*indices_shape)),
                    ]
                    output_features = [("output", None)]

                    builder = neural_network.NeuralNetworkBuilder(
                        input_features,
                        output_features,
                        disable_rank5_shape_mapping=True,
                    )

                    builder.add_gather(
                        "gather", ["params", "indices"], "updates", axis=axis
                    )
                    builder.add_scatter(
                        "scatter",
                        ["params", "indices", "updates"],
                        "output",
                        axis=axis,
                        mode="UPDATE",
                    )

                    a = np.random.rand(*params_shape)
                    b = np.random.randint(
                        -params_shape[axis], params_shape[axis], size=indices_shape
                    )

                    input = {"params": a, "indices": b.astype(np.float)}
                    expected = {"output": a}
                    self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_gather_scatter_multiple_axis_gpu(self):
        self.test_gather_scatter_multiple_axis_cpu(cpu_only=False)

    def test_scatter_along_axis_cpu(self, cpu_only=True):
        for rank in range(1, 6):
            for axis in range(-rank, rank):
                for id in range(5):
                    ref_shape = np.random.randint(low=2, high=8, size=rank)
                    indices_shape = np.copy(ref_shape)
                    indices_shape[axis] = np.random.randint(low=1, high=8)
                    updates_shape = indices_shape

                    input_features = [
                        ("ref", datatypes.Array(*ref_shape)),
                        ("indices", datatypes.Array(*indices_shape)),
                        ("updates", datatypes.Array(*updates_shape)),
                    ]
                    output_features = [("output", None)]

                    builder = neural_network.NeuralNetworkBuilder(
                        input_features,
                        output_features,
                        disable_rank5_shape_mapping=True,
                    )

                    builder.add_scatter_along_axis(
                        "scatter_along_axis",
                        ["ref", "indices", "updates"],
                        "output",
                        axis=axis,
                        mode="UPDATE",
                    )

                    ref = np.random.rand(*ref_shape)
                    updates = np.random.rand(*updates_shape)
                    indices = np.random.randint(
                        -ref_shape[axis], ref_shape[axis], size=indices_shape
                    )
                    input = {
                        "ref": ref,
                        "indices": indices.astype(np.float),
                        "updates": updates,
                    }

                    np_output = np.copy(ref)
                    np.put_along_axis(np_output, indices, updates, axis=axis)
                    expected = {"output": np_output}
                    self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_scatter_along_axis_gpu(self):
        self.test_scatter_along_axis_cpu(cpu_only=False)

    @unittest.skipIf(not _HAS_TF, MSG_TF1_NOT_FOUND)
    def test_scatter_nd_cpu(self, cpu_only=True):
        for ref_rank, indices_rank in [
            (i, j) for i in range(1, 6) for j in range(2, 6)
        ]:
            ref_shape = np.random.randint(low=2, high=8, size=ref_rank)
            indices_shape = np.random.randint(low=2, high=8, size=indices_rank)
            indices_shape[-1] = np.random.randint(low=1, high=ref_rank + 1)
            for accumulate_mode in ["UPDATE", "ADD", "SUB"]:
                for id in range(20):
                    updates_shape = list(indices_shape[:-1]) + list(
                        ref_shape[indices_shape[-1] :]
                    )
                    if len(updates_shape) > 5:
                        continue

                    input_features = [
                        ("ref", datatypes.Array(*ref_shape)),
                        ("indices", datatypes.Array(*indices_shape)),
                        ("updates", datatypes.Array(*updates_shape)),
                    ]
                    output_features = [("output", None)]

                    builder = neural_network.NeuralNetworkBuilder(
                        input_features,
                        output_features,
                        disable_rank5_shape_mapping=True,
                    )

                    builder.add_scatter_nd(
                        "scatter_nd",
                        ["ref", "indices", "updates"],
                        "output",
                        mode=accumulate_mode,
                    )

                    ref = np.random.rand(*ref_shape)
                    updates = np.random.rand(*updates_shape)
                    indices_list = []
                    for i in range(indices_shape[-1]):
                        indices_list.append(
                            np.random.randint(0, ref_shape[i], size=indices_shape[:-1])
                        )

                    indices = np.stack(indices_list, axis=-1)

                    input = {
                        "ref": ref,
                        "indices": indices.astype(np.float),
                        "updates": updates,
                    }

                    with tf.Graph().as_default(), tf.Session() as sess:
                        tf_output = tf.Variable(ref)
                        sess.run(tf.global_variables_initializer())
                        if accumulate_mode == "UPDATE":
                            sess.run(tf.scatter_nd_update(tf_output, indices, updates))
                        if accumulate_mode == "ADD":
                            sess.run(tf.scatter_nd_add(tf_output, indices, updates))
                        if accumulate_mode == "SUB":
                            sess.run(tf.scatter_nd_sub(tf_output, indices, updates))
                        expected = {"output": sess.run(tf_output)}

                    self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_scatter_nd_gpu(self):
        self.test_scatter_nd_cpu(cpu_only=False)

    def test_layer_normalization_cpu(self, cpu_only=True):
        def layer_norm_numpy(x, shapes, gamma_, beta_, eps=1e-5):
            axes = [-i - 1 for i, _ in enumerate(shapes)]
            num = x - np.mean(x, axis=tuple(axes), keepdims=True)
            dem = np.sqrt(
                np.sum(np.square(num), axis=tuple(axes), keepdims=True)
                / np.prod(shapes)
                + eps
            )
            return num / dem * gamma_ + beta_

        for rank in range(1, 6):
            input_shape = np.random.randint(low=2, high=6, size=rank)
            for axis in range(1, len(input_shape) + 1):
                norm_shapes = input_shape[-axis:]

                data = np.random.rand(*input_shape)

                gamma = np.random.rand(*norm_shapes)
                beta = np.random.rand(*norm_shapes)

                input_features = [("data", datatypes.Array(*input_shape))]
                output_features = [("output", None)]

                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_layer_normalization(
                    name="layer_normalization",
                    input_name="data",
                    output_name="output",
                    normalized_shape=norm_shapes,
                    gamma=gamma,
                    beta=beta,
                )

                inputs = {"data": data}
                ref = layer_norm_numpy(data, norm_shapes, gamma, beta)
                expected = {"output": ref}

                self._test_model(builder.spec, inputs, expected, useCPUOnly=cpu_only)

    def test_layer_normalization_gpu(self):
        self.test_layer_normalization_cpu(cpu_only=False)


def get_size_after_stride(X, params):
    start = params["start"]
    end = params["end"]
    stride = params["stride"]
    if params["axis"] == "width":
        axis = 2
    if params["axis"] == "height":
        axis = 1
    if params["axis"] == "channel":
        axis = 0
    N = X.shape[axis]
    if end < 0:
        end = end + N
    end = min(end, N)
    if start > N - 1:
        L = 0
    else:
        L = np.floor((end - 1 - start) / stride) + 1
        if L < 0:
            L = 0
    return L


def get_numpy_predictions_slice(X, params):
    start = params["start"]
    end = params["end"]
    stride = params["stride"]
    if params["axis"] == "width":
        return X[:, :, start:end:stride]
    if params["axis"] == "height":
        return X[:, start:end:stride, :]
    if params["axis"] == "channel":
        return X[start:end:stride, :, :]


def get_coreml_predictions_slice(X, params):
    coreml_preds = []
    eval = True
    try:
        input_dim = X.shape
        output_dim = (
            1,
            1,
            1,
        )  # some random dimensions here: we are going to remove this information later
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", datatypes.Array(*output_dim))]
        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        builder.add_slice(
            "slice",
            "data",
            "output",
            start_index=params["start"],
            end_index=params["end"],
            stride=params["stride"],
            axis=params["axis"],
        )
        # Remove output shape by deleting and adding an output
        del builder.spec.description.output[-1]
        output = builder.spec.description.output.add()
        output.name = "output"
        output.type.multiArrayType.dataType = coremltools.proto.FeatureTypes_pb2.ArrayFeatureType.ArrayDataType.Value(
            "DOUBLE"
        )
        # save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, "test_layer.mlmodel")
        coremltools.utils.save_spec(builder.spec, model_path)
        # prepare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        coreml_input = {"data": X}
        if _is_macos() and _macos_version() >= (10, 13):
            coreml_preds = coreml_model.predict(coreml_input)["output"]
        else:
            coreml_preds = None
        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)
    except RuntimeError as e:
        print(e)
        eval = False

    return coreml_preds, eval


def get_numpy_predictions_reduce(X, params):
    if params["axis"] == "CHW":
        axis = (0, 1, 2)
    if params["axis"] == "HW":
        axis = (1, 2)
    if params["axis"] == "C":
        axis = 0
    if params["axis"] == "H":
        axis = 1
    if params["axis"] == "W":
        axis = 2

    if params["mode"] == "sum":
        return np.sum(X, axis)
    if params["mode"] == "avg":
        return np.mean(X, axis)
    if params["mode"] == "prod":
        return np.prod(X, axis)
    if params["mode"] == "logsum":
        return np.sum(np.log(X + 1e-6), axis)
    if params["mode"] == "sumsquare":
        return np.sum(X ** 2, axis)
    if params["mode"] == "L2":
        return np.sqrt(np.sum(X ** 2, axis))
    if params["mode"] == "L1":
        return np.sum(np.abs(X), axis)
    if params["mode"] == "max":
        return np.amax(X, axis)
    if params["mode"] == "min":
        return np.amin(X, axis)
    if params["mode"] == "argmax":
        return np.argmax(X, axis)


def get_coreml_predictions_reduce(X, params):
    coreml_preds = []
    eval = True
    try:
        input_dim = X.shape
        # some random dimensions here: we are going to remove this information later
        output_dim = (1, 1, 1)
        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", datatypes.Array(*output_dim))]
        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        builder.add_reduce(
            "reduce", "data", "output", axis=params["axis"], mode=params["mode"]
        )
        # Remove output shape by deleting and adding an output
        del builder.spec.description.output[-1]
        output = builder.spec.description.output.add()
        output.name = "output"
        output.type.multiArrayType.dataType = coremltools.proto.FeatureTypes_pb2.ArrayFeatureType.ArrayDataType.Value(
            "DOUBLE"
        )
        # save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, "test_layer.mlmodel")
        coremltools.utils.save_spec(builder.spec, model_path)
        # prepare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        coreml_input = {"data": X}
        if _is_macos() and _macos_version() >= (10, 13):
            coreml_preds = coreml_model.predict(coreml_input)["output"]
        else:
            coreml_preds = None
        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)
    except RuntimeError as e:
        print(e)
        eval = False

    return coreml_preds, eval


@pytest.mark.slow
class StressTest(CorrectnessTest):
    def test_slice_layer(self):
        params_dict = dict(
            input_shape=[[30, 100, 8], [80, 50, 5], [4, 12, 5], [56, 8, 14]],
            axis=["channel", "height", "width"],
            start=[0, 1, 2, 5],
            end=[5, 100, 56, -1, -2, -4],
            stride=[1, 2, 3],
        )
        params = list(itertools.product(*params_dict.values()))
        all_candidates = [dict(zip(params_dict.keys(), x)) for x in params]
        valid_params = []
        for pr in all_candidates:
            X = np.random.rand(*pr["input_shape"])
            if get_size_after_stride(X, pr):
                valid_params.append(pr)
        print(
            "Total params to be tested: ",
            len(valid_params),
            "out of candidates: ",
            len(all_candidates),
        )

        failed_tests_compile = []
        failed_tests_shape = []
        failed_tests_numerical = []
        for i in range(len(valid_params)):
            params = valid_params[i]
            X = np.random.rand(*params["input_shape"])
            np_preds = get_numpy_predictions_slice(X, params)
            coreml_preds, eval = get_coreml_predictions_slice(X, params)
            if eval is False:
                failed_tests_compile.append(params)
            elif coreml_preds is not None:
                if not self._compare_shapes(np_preds, coreml_preds):
                    failed_tests_shape.append(params)
                elif not self._compare_predictions(np_preds, coreml_preds):
                    failed_tests_numerical.append(params)

        self.assertEqual(failed_tests_compile, [])
        self.assertEqual(failed_tests_shape, [])
        self.assertEqual(failed_tests_numerical, [])

    def test_reduce_layer(self):
        params_dict = dict(
            input_shape=[[3, 10, 8], [8, 5, 5], [4, 12, 10], [7, 1, 14]],
            mode=[
                "sum",
                "avg",
                "prod",
                "sumsquare",
                "L1",
                "L2",
                "max",
                "min",
                "argmax",
            ],
            axis=["CHW", "HW", "C", "H", "W"],
        )
        params = list(itertools.product(*params_dict.values()))
        all_candidates = [dict(zip(params_dict.keys(), x)) for x in params]
        valid_params = []
        for pr in all_candidates:
            if pr["mode"] == "argmax":
                if pr["axis"] == "CHW" or pr["axis"] == "HW":
                    continue
            valid_params.append(pr)
        print(
            "Total params to be tested: ",
            len(valid_params),
            "out of candidates: ",
            len(all_candidates),
        )

        failed_tests_compile = []
        failed_tests_shape = []
        failed_tests_numerical = []
        for i in range(len(valid_params)):
            params = valid_params[i]
            X = np.random.rand(*params["input_shape"])
            np_preds = get_numpy_predictions_reduce(X, params)
            coreml_preds, eval = get_coreml_predictions_reduce(X, params)
            if eval is False:
                failed_tests_compile.append(params)
            elif coreml_preds is not None:
                if not self._compare_shapes(np_preds, coreml_preds):
                    failed_tests_shape.append(params)
                elif not self._compare_predictions(np_preds, coreml_preds):
                    failed_tests_numerical.append(params)

        self.assertEqual(failed_tests_compile, [])
        self.assertEqual(failed_tests_shape, [])
        self.assertEqual(failed_tests_numerical, [])


@pytest.mark.slow
@unittest.skipIf(
    not _is_macos() or _macos_version() < LAYERS_10_15_MACOS_VERSION,
    "macOS 10.15+ required. Skipping tests.",
)
class CoreML3NetworkStressTest(CorrectnessTest):
    def test_dyn_weight_conv2d_stress(self):
        options = dict(
            padding=["valid"],
            filters=[1, 2, 4],
            kernel_size=[1, 3, 5],  # square kernels
            strides=[1, 2],
            dilation_rate=[1],
            batch_size=[1, 64, 512],
        )

        input_size = 64
        input_channels = 64
        input_dim = [1, input_channels, input_size, input_size]

        def conv_spatial_size(image_size, kernel_size, stride, dilation, padding):
            if padding == "valid":
                kernel_size_dilated = (kernel_size - 1) * dilation + 1
                return (image_size - kernel_size_dilated) // stride + 1
            elif padding == "same":
                return int(math.ceil(image_size * 1.0 / stride))
            else:
                return 0

        for x in itertools.product(*options.values()):
            kwargs = dict(zip(options.keys(), x))
            if kwargs["strides"] > 1 and kwargs["dilation_rate"] > 1:
                continue
            # weight layout: (output_channels, kernel_channels, height, width)
            weight_dim = (
                kwargs["filters"],
                input_channels,
                kwargs["kernel_size"],
                kwargs["kernel_size"],
            )

            input_dim[0] = kwargs["batch_size"]
            input_features = [
                ("input", datatypes.Array(*input_dim)),
                ("weight", datatypes.Array(*weight_dim)),
            ]
            output_features = [("output", None)]

            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )

            builder.add_convolution(
                name="two_input_conv_layer",
                kernel_channels=input_channels,
                output_channels=kwargs["filters"],
                height=kwargs["kernel_size"],
                width=kwargs["kernel_size"],
                stride_height=kwargs["strides"],
                stride_width=kwargs["strides"],
                border_mode=kwargs["padding"],
                groups=1,
                W=None,
                b=None,
                has_bias=False,
                dilation_rate=kwargs["dilation_rate"],
                input_name=["input", "weight"],
                output_name="output",
            )

            # Assigning everything to ones should cover the execution path
            # and engine failures, but is not a complete check on numerics.
            out_spatial_size = conv_spatial_size(
                input_size,
                kwargs["kernel_size"],
                kwargs["strides"],
                kwargs["dilation_rate"],
                kwargs["padding"],
            )

            input_val = np.ones(input_dim)
            weight_val = np.ones(weight_dim)
            output_dim = (
                kwargs["batch_size"],
                kwargs["filters"],
                out_spatial_size,
                out_spatial_size,
            )
            expected = np.ones(output_dim) * (
                kwargs["kernel_size"] * kwargs["kernel_size"] * input_channels
            )

            feed_dict = {"input": input_val, "weight": weight_val}
            expected = {"output": expected}

            self._test_model(builder.spec, feed_dict, expected)

    def test_static_weight_conv2d_stress(self):
        options = dict(
            padding=["valid"],
            filters=[1, 2, 5],
            kernel_size=[1, 3, 4],  # square kernels
            strides=[1, 2],
            dilation_rate=[1, 2],
            batch_size=[1, 32, 512],
        )

        input_size = 64
        input_channels = 64
        input_dim = [1, input_channels, input_size, input_size]

        def conv_spatial_size(image_size, kernel_size, stride, dilation, padding):
            if padding == "valid":
                kernel_size_dilated = (kernel_size - 1) * dilation + 1
                return (image_size - kernel_size_dilated) // stride + 1
            elif padding == "same":
                return int(math.ceil(image_size * 1.0 / stride))
            else:
                return 0

        for x in itertools.product(*options.values()):
            kwargs = dict(zip(options.keys(), x))
            if kwargs["strides"] > 1 and kwargs["dilation_rate"] > 1:
                continue
            # weight layout: (output_channels, kernel_channels, height, width)
            weight_dim = (
                kwargs["filters"],
                input_channels,
                kwargs["kernel_size"],
                kwargs["kernel_size"],
            )

            input_dim[0] = kwargs["batch_size"]
            input_features = [("input", datatypes.Array(*input_dim))]
            # ('weight', datatypes.Array(*weight_dim))]
            output_features = [("output", None)]

            input_weight = np.ones(weight_dim)
            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )

            builder.add_convolution(
                name="two_input_conv_layer",
                kernel_channels=input_channels,
                output_channels=kwargs["filters"],
                height=kwargs["kernel_size"],
                width=kwargs["kernel_size"],
                stride_height=kwargs["strides"],
                stride_width=kwargs["strides"],
                border_mode=kwargs["padding"],
                groups=1,
                W=input_weight,
                b=None,
                has_bias=False,
                dilation_factors=[kwargs["dilation_rate"]] * 2,
                input_name=["input"],
                output_name="output",
            )

            # Assigning everything to ones should cover the execution path
            # and engine failures, but is not a complete check on numerics.
            out_spatial_size = conv_spatial_size(
                input_size,
                kwargs["kernel_size"],
                kwargs["strides"],
                kwargs["dilation_rate"],
                kwargs["padding"],
            )

            input_val = np.ones(input_dim)
            weight_val = np.ones(weight_dim)
            output_dim = (
                kwargs["batch_size"],
                kwargs["filters"],
                out_spatial_size,
                out_spatial_size,
            )
            expected = np.ones(output_dim) * (
                kwargs["kernel_size"] * kwargs["kernel_size"] * input_channels
            )

            feed_dict = {"input": input_val}  # , 'weight': weight_val}
            expected = {"output": expected}

            self._test_model(builder.spec, feed_dict, expected)

    def test_power_iteration_cpu(self):

        convergence_tolerance = 1e-8
        number_of_iterations = 200

        input_features = [
            ("matrix", datatypes.Array(*(2, 2))),
            ("starting_vector", datatypes.Array(*(2,))),
        ]

        output_features = [
            ("maximum_eigen_value", datatypes.Array(*(1, 1))),
            ("eigen_vector", None),
            ("iteration_count", datatypes.Array(*(1,))),
        ]

        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        builder.add_expand_dims("expand_dims", "starting_vector", "x", axes=[-1])
        builder.add_load_constant_nd(
            "iteration_count",
            "iteration_count",
            constant_value=np.zeros((1,)),
            shape=(1,),
        )

        loop_layer = builder.add_loop("loop", max_iterations=number_of_iterations)
        loop_body_builder = neural_network.NeuralNetworkBuilder(
            nn_spec=loop_layer.loop.bodyNetwork
        )
        # output shape: (n,1)
        loop_body_builder.add_batched_mat_mul(
            "bmm.1", input_names=["matrix", "x"], output_name="y"
        )
        loop_body_builder.add_reduce_l2(
            "reduce", input_name="y", output_name="norm", axes=[0]
        )
        loop_body_builder.add_divide_broadcastable(
            "divide", ["y", "norm"], "y_normalized"
        )
        # find diff: 1- abs(cosine)
        loop_body_builder.add_batched_mat_mul(
            "cosine", ["y_normalized", "x"], "cosine_diff", transpose_a=True
        )
        loop_body_builder.add_squeeze(
            "squeeze_all", "cosine_diff", "cosine_diff_squeeze", squeeze_all=True
        )
        loop_body_builder.add_unary(
            "abs_cosine", "cosine_diff_squeeze", "abs_cosine_diff", mode="abs"
        )
        loop_body_builder.add_activation(
            "diff",
            non_linearity="LINEAR",
            input_name="abs_cosine_diff",
            output_name="diff",
            params=[-1, 1],
        )

        # update iteration count
        loop_body_builder.add_activation(
            "iteration_count_add",
            non_linearity="LINEAR",
            input_name="iteration_count",
            output_name="iteration_count_plus_1",
            params=[1, 1],
        )
        loop_body_builder.add_copy(
            "iteration_count_copy", "iteration_count_plus_1", "iteration_count"
        )

        # update 'x'
        loop_body_builder.add_copy("update_x", "y_normalized", "x")

        # add condition to break from the loop, if convergence criterion is met
        loop_body_builder.add_less_than(
            "cond", ["diff"], "cond", alpha=convergence_tolerance
        )
        branch_layer = loop_body_builder.add_branch("branch_layer", "cond")
        builder_ifbranch = neural_network.NeuralNetworkBuilder(
            nn_spec=branch_layer.branch.ifBranch
        )
        builder_ifbranch.add_loop_break("break")

        # now we are out of the loop, compute the eigen value
        builder.add_batched_mat_mul(
            "bmm.2", input_names=["matrix", "x"], output_name="x_right"
        )
        builder.add_batched_mat_mul(
            "bmm.3",
            input_names=["x", "x_right"],
            output_name="maximum_eigen_value",
            transpose_a=True,
        )
        builder.add_squeeze("squeeze", "x", "eigen_vector", squeeze_all=True)

        # make input sizes flexible
        spec = builder.spec

        flexible_shape_utils.add_multiarray_ndshape_enumeration(
            spec, feature_name="matrix", enumerated_shapes=[(3, 3), (4, 4)]
        )

        flexible_shape_utils.add_multiarray_ndshape_enumeration(
            spec, feature_name="starting_vector", enumerated_shapes=[(3,), (4,)]
        )

        from numpy import linalg as LA

        # try on 3x3 matrix
        A = np.array([[2, -6, 8], [-6, 4, 5], [8, 5, 3]], dtype=np.float)
        starting_vector = np.random.rand(3)
        starting_vector = starting_vector / np.sqrt(np.sum(starting_vector ** 2))

        e, v = LA.eig(A)
        idx = np.argmax(abs(e))
        input = {"starting_vector": starting_vector, "matrix": A.astype(np.float)}
        expected = {"maximum_eigen_value": np.array([[e[idx]]])}
        self._test_model(spec, input, expected, useCPUOnly=True)

        # try on 2x2 matrix
        A = np.array([[4, -5], [-5, 3]], dtype=np.float)
        starting_vector = np.random.rand(2)
        starting_vector = starting_vector / np.sqrt(np.sum(starting_vector ** 2))

        e, v = LA.eig(A)
        idx = np.argmax(abs(e))

        input = {"starting_vector": starting_vector, "matrix": A.astype(np.float)}
        expected = {"maximum_eigen_value": np.array([[e[idx]]])}
        self._test_model(spec, input, expected, useCPUOnly=True)


@unittest.skipIf(
    _macos_version() < LAYERS_11_0_MACOS_VERSION,
    "macOS 11.0+ required. Skipping tests.",
)
class IOS14SingleLayerTests(CorrectnessTest):
    @unittest.skipIf(not _HAS_TF, MSG_TF1_NOT_FOUND)
    def test_onehot_layer_cpu(self, cpu_only=True):
        ctr = 0
        params_dict = dict(
            input_rank=[1, 2, 3, 4],
            negative_axis=[True, False],
            depth=[30],
            on_value=[30.0],
            off_value=[-4.0],
        )
        params = list(itertools.product(*params_dict.values()))
        for param in params:
            param = dict(zip(params_dict.keys(), param))
            input_rank = param["input_rank"]
            vectorSize = param["depth"]
            on_value = param["on_value"]
            off_value = param["off_value"]

            for axis in range(input_rank + 1):
                ctr += 1
                if param["negative_axis"]:
                    axis_param = axis - (input_rank + 1)
                else:
                    axis_param = axis

                input_shape = np.random.randint(1, 10, size=(input_rank,))

                input_features = [("data", datatypes.Array(*input_shape))]
                output_features = [("output", None)]
                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_one_hot(
                    "one_hot",
                    ["data"],
                    "output",
                    one_hot_vector_size=vectorSize,
                    axis=axis_param,
                    on_value=on_value,
                    off_value=off_value,
                )

                x = np.random.randint(0, vectorSize, size=input_shape)
                # x[::4] -= vectorSize  # [To do] Need to Handle this case.

                with tf.Graph().as_default(), tf.Session() as sess:
                    # TF seems to have a bug with axis < -1
                    if axis_param < -1:
                        axis_param += input_rank + 1
                    tf_op = tf.one_hot(
                        x,
                        axis=axis_param,
                        depth=vectorSize,
                        on_value=on_value,
                        off_value=off_value,
                    )
                    expected = {"output": sess.run(tf_op)}

                input = {"data": x.astype(np.float)}
                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_batched_mat_mul_dynamic_quantization_cpu(self, cpu_only=True):
        X1 = 11
        X2 = 23
        W = np.random.rand(X1, X2) * 20 - 10  # uniform between [-10, 10]
        b = np.random.rand(X2) * 20 - 10
        input_shapes = [
            (X1,),
            (5, X1),
            (2, 3, X1),
            (4, 1, X1),
        ]  # , (12, 5, 8, X1), (2, 3, 1, 5, X1)]

        W_max = max(np.abs(np.min(W)), np.abs(np.max(W)))
        W_normalized = W / W_max  # [-1,1]
        W_quantized_int8 = 127.0 * W_normalized  # [-127, 127]
        W_quantized_int8 = W_quantized_int8.astype(np.int8)
        quant_scale = W_max / 127.0

        for input_shape in input_shapes:
            x = np.random.rand(*input_shape) * 10

            input_features = [("data", datatypes.Array(*input_shape))]
            output_features = [("output", None)]

            for has_bias in [True, False]:
                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_batched_mat_mul(
                    name="batched_mat_mul",
                    input_names=["data"],
                    output_name="output",
                    weight_matrix_rows=X1,
                    weight_matrix_columns=X2,
                    int_8_dynamic_quantize=True,
                    is_quantized_weight=True,
                    quantization_type="linear",
                    nbits=8,
                    W=W_quantized_int8.tobytes(),
                    bias=b if has_bias else None,
                    quant_scale=np.array([quant_scale]),
                )
                inputs = {"data": x}
                expected = {
                    "output": np.matmul(
                        x, W_quantized_int8.astype(np.float) * quant_scale
                    )
                    + (b if has_bias else np.zeros(X2))
                }
                self._test_model(
                    builder.spec,
                    inputs,
                    expected,
                    useCPUOnly=cpu_only,
                    test_metric="SNR",
                    SNR=40,
                )

    def test_batched_mat_mul_dynamic_quantization_gpu(self):
        self.test_batched_mat_mul_dynamic_quantization_cpu(cpu_only=False)

    def test_inner_product_dynamic_quantization_cpu(self, cpu_only=True):
        Xin = 24
        Xout = 23
        W = np.random.rand(Xout, Xin)
        b = np.random.rand(Xout)
        # For rank 4 and 5, the product of the last 3 dimensions must equal Xin
        input_shapes = [
            (Xin,),
            (5, Xin),
            (2, 3, Xin),
            (4, 1, Xin),
            (5, 2, 3, 4),
            (5, 6, 2, 3, 4),
        ]

        W_max = max(np.abs(np.min(W)), np.abs(np.max(W)))
        W_normalized = W / W_max  # [-1,1]
        W_quantized_int8 = 127.0 * W_normalized  # [-127, 127]
        W_quantized_int8 = W_quantized_int8.astype(np.int8)
        quant_scale = W_max / 127.0

        for input_shape in input_shapes:
            rank = len(input_shape)
            x = np.random.rand(*input_shape) * 5

            W_for_numpy = W_quantized_int8.astype(np.float) * quant_scale
            for has_bias in [True, False]:
                b = b if has_bias else np.zeros(Xout)
                if rank == 1 or rank == 2 or rank == 3:
                    np_out = np.matmul(x, np.transpose(W_for_numpy)) + b
                    expected = {"output": np_out}
                elif rank == 4:
                    x_shaped = np.reshape(x, (x.shape[0], np.product(x.shape[1:])))
                    np_out = np.matmul(x_shaped, np.transpose(W_for_numpy)) + b
                    expected = {"output": np.reshape(np_out, np_out.shape + (1, 1))}
                elif rank == 5:
                    x_shaped = np.reshape(x, x.shape[0:2] + (np.product(x.shape[2:]),))
                    np_out = np.matmul(x_shaped, np.transpose(W_for_numpy)) + b
                    expected = {
                        "output": np.reshape(
                            np_out, x.shape[0:2] + (np_out.shape[-1],) + (1, 1)
                        )
                    }

                input_features = [("data", datatypes.Array(*input_shape))]
                output_features = [("output", None)]
                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                builder.add_inner_product(
                    name="ip",
                    W=W_quantized_int8.tobytes(),
                    b=b if has_bias else None,
                    input_channels=Xin,
                    output_channels=Xout,
                    has_bias=has_bias,
                    input_name="data",
                    output_name="output",
                    int_8_dynamic_quantize=True,
                    is_quantized_weight=True,
                    quantization_type="linear",
                    nbits=8,
                    quant_scale=np.array([quant_scale]),
                )
                inputs = {"data": x}
                self._test_model(
                    builder.spec,
                    inputs,
                    expected,
                    useCPUOnly=cpu_only,
                    test_metric="SNR",
                    SNR=40,
                )

    def test_inner_product_dynamic_quantization_gpu(self):
        self.test_inner_product_dynamic_quantization_cpu(cpu_only=False)

    def test_onehot_layer_gpu(self):
        self.test_onehot_layer_cpu(cpu_only=False)

    @unittest.skipIf(not _HAS_TF, MSG_TF1_NOT_FOUND)
    def test_cumsum_layer_cpu(self, cpu_only=True):
        ctr = 0
        params_dict = dict(
            rank=[1, 2, 3, 4, 5],
            exclusive=[False, True],
            reverse=[False, True],
            n_inputs=[1, 2],
        )
        params = list(itertools.product(*params_dict.values()))
        for param in params:
            param = dict(zip(params_dict.keys(), param))
            rank = param["rank"]
            exclusive = param["exclusive"]
            reverse = param["reverse"]
            n_inputs = param["n_inputs"]

            for axis in range(rank):
                ctr += 1
                if np.random.rand(1) > 0.5:
                    axis_param = axis
                else:
                    axis_param = axis - rank

                input_shape = np.random.randint(1, 10, size=(rank,))

                input_features = [("data", datatypes.Array(*input_shape))]
                if n_inputs == 2:
                    input_features.append(("axis", datatypes.Array(1,)))

                output_features = [("output", None)]
                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )

                if n_inputs == 1:
                    builder.add_cumsum(
                        "cumsum",
                        ["data"],
                        "output",
                        axis=axis_param,
                        reverse=reverse,
                        exclusive=exclusive,
                    )
                else:
                    builder.add_cumsum(
                        "cumsum",
                        ["data", "axis"],
                        "output",
                        reverse=reverse,
                        exclusive=exclusive,
                    )

                x = np.random.rand(*input_shape)

                with tf.Graph().as_default(), tf.Session() as sess:
                    tf_op = tf.cumsum(
                        x, axis=axis_param, exclusive=exclusive, reverse=reverse
                    )
                    expected = {"output": sess.run(tf_op)}

                input = {"data": x}
                if n_inputs == 2:
                    input["axis"] = axis_param * np.ones((1,), dtype=np.float)

                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_cumsum_layer_gpu(self):
        self.test_cumsum_layer_cpu(cpu_only=False)

    def test_clamped_relu_cpu(self, cpu_only=True):

        params_dict = dict(alpha=[0.0, 2.0, -3.0], beta=[7.0, -8.0])
        params = list(itertools.product(*params_dict.values()))
        for param in params:
            param = dict(zip(params_dict.keys(), param))
            alpha = param["alpha"]
            beta = param["beta"]
            input_shape = [40]
            input_features = [("data", datatypes.Array(*input_shape))]
            output_features = [("output", None)]
            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )
            builder.add_clamped_relu(
                "clamped_relu", "data", "output", alpha=alpha, beta=beta
            )

            x = np.arange(-20, 20, dtype=np.float)
            input = {"data": x}
            expected = {"output": np.minimum(beta, np.where(x >= 0, x, x * alpha))}
            self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_clamped_relu_gpu(self):
        self.test_clamped_relu_cpu(cpu_only=False)

    def _test_pool3d(self, cpu_only):
        pool_types = ("MAX", "AVERAGE")
        # Defining shapes as (batch, channel, depth, height, width)
        shapes = ((1, 1, 1, 2, 2), (1, 1, 3, 3, 3), (3, 4, 10, 17, 90))
        # Defining kernels and strides as (depth, height, width)
        kernels = ((2, 2, 2), (1, 3, 4), (2, 3, 4), (5, 1, 6), (8, 9, 1), (7, 11, 13))
        strides = ((1, 1, 1), (1, 2, 3), (2, 3, 2), (4, 1, 2), (3, 4, 1), (7, 11, 13))
        # Defining paddings as (left, right, top, bottom, front, back)
        # This is backwards from how we define shapes, kernels, and strides,
        # but it better matches pytorch, making the creation of pytorch layers
        # much easier.
        paddings = (
            ("CUSTOM", (0, 0, 0, 0, 0, 0)),
            ("CUSTOM", (2, 2, 2, 2, 2, 2)),
            ("CUSTOM", (5, 6, 3, 4, 2, 2)),
            # VALID and SAME padding must have custom paddings unset or set to zero.
            ("VALID", (0, 0, 0, 0, 0, 0)),
            ("SAME", (0, 0, 0, 0, 0, 0)),
        )

        # Structure to collect failures so
        # we can run all tests, even if one fails.
        # This should be able to go away when we can parameterize
        # our tests: <rdar://problem/59966164> Enable parameterized tests in test_numpy_nn_layers.py
        failures = []
        num_successes = 0
        num_skipped = 0

        for pool_type in pool_types:
            for shape in shapes:
                for kernel in kernels:
                    for stride in strides:
                        for padding in paddings:
                            for average_pooling_count_excludes_padding in (False, True):
                                result = self._test_pool3d_single_case(
                                    cpu_only,
                                    pool_type,
                                    shape,
                                    kernel,
                                    stride,
                                    padding,
                                    average_pooling_count_excludes_padding,
                                )
                                if type(result) is str:
                                    failures.append(result)
                                elif result:
                                    num_successes += 1
                                else:
                                    num_skipped += 1
        self.assertEqual(
            len(failures),
            0,
            "Got %s successes, %s skipped,  %s failures: %s"
            % (num_successes, num_skipped, len(failures), failures),
        )

    def _test_pool3d_single_case(
        self,
        cpu_only,
        pool_type,
        shape,
        kernel,
        stride,
        padding,
        average_pooling_count_excludes_padding,
    ):
        """

        Args:
            cpu_only:
            pool_type:
            shape:
            kernel:
            stride:
            padding:
            average_pooling_count_excludes_padding:

        Returns: True if success, False if skipped, Str if error

        """
        test_case = (
            "Test case:: pool_type: %s, shape: %s, kernel: %s, stride: %s, padding: %s, average_pooling_count_excludes_padding: %s"
            % (
                pool_type,
                shape,
                kernel,
                stride,
                padding,
                average_pooling_count_excludes_padding,
            )
        )
        input_features = [("data", datatypes.Array(*shape))]
        output_features = [("output", None)]
        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        padding_mode = padding[0]
        padding_values = padding[1]
        builder.add_pooling3d(
            name="pooling3d",
            input_name="data",
            output_name="output",
            pooling_type=pool_type,
            kernel_depth=kernel[0],
            kernel_height=kernel[1],
            kernel_width=kernel[2],
            stride_depth=stride[0],
            stride_height=stride[1],
            stride_width=stride[2],
            padding_mode=padding_mode,
            custom_padding_front=padding_values[4],
            custom_padding_back=padding_values[5],
            custom_padding_top=padding_values[2],
            custom_padding_bottom=padding_values[3],
            custom_padding_left=padding_values[0],
            custom_padding_right=padding_values[1],
            average_pooling_count_excludes_padding=average_pooling_count_excludes_padding,
        )

        # Expected output
        input = np.random.rand(*shape)
        torch_input = torch.from_numpy(np.reshape(input, shape))

        # Padding
        if padding_mode == "CUSTOM":
            torch_padding = torch.nn.ConstantPad3d(padding_values, 0)
        elif padding_mode == "VALID":
            torch_padding = torch.nn.ConstantPad3d(0, 0)
        elif padding_mode == "SAME":
            padding_list = []
            # torch.nn.ConstantPad3d wants (left, right, top, bottom, front, back)
            # but our shape, kernel, and stride are (depth, height, width).
            total_paddings = aggregated_pad(
                pad_type=padding_mode.lower(),
                kernel_shape=kernel,
                input_shape=shape[2:],
                strides=stride,
            )
            total_paddings.reverse()
            for p in total_paddings:
                before = int(math.floor(float(p) / 2.0))
                after = int(math.ceil(float(p) / 2.0))
                padding_list.append(before)
                padding_list.append(after)

            torch_padding = torch.nn.ConstantPad3d(tuple(padding_list), 0)
            padding_values = padding_list[:]
        else:
            assert False

        # Validate output shape
        for i in range(3):
            try:
                IOS14SingleLayerTests._validate_pooling_dimension(
                    shape[i + 2],
                    kernel[i],
                    stride[i],
                    padding_values[6 - i - 2],
                    padding_values[6 - i - 1],
                )
            except ValueError:
                return False

        # Pooling type
        # Average pooling
        if pool_type == "AVERAGE":
            # torch.nn.AvgPool3d only accepts a single integer for padding, so we normally
            # create a pooling layer first which allows us to fully specify the
            # before and after padding in all three dimensions.
            #
            # However, when we use a padding layer, torch.nn.AvgPool3d doesn't
            # know what is padding and what isn't, which means that its
            # `count_include_pad` parameter has no effect.
            #
            # Therefore, we can only test average_pooling_count_excludes_padding=True
            # when padding is homogeneous.
            is_padding_homogeneous = all(p == padding_values[0] for p in padding_values)
            if average_pooling_count_excludes_padding:
                if not is_padding_homogeneous:
                    return False
                else:
                    # padding is homogeneous
                    torch_model = torch.nn.AvgPool3d(
                        kernel,
                        stride=stride,
                        padding=padding_values[0],
                        count_include_pad=not average_pooling_count_excludes_padding,
                    )
            else:
                # average_pooling_count_excludes_padding == False
                torch_pool = torch.nn.AvgPool3d(
                    kernel,
                    stride=stride,
                    count_include_pad=not average_pooling_count_excludes_padding,
                )
                torch_model = torch.nn.Sequential(torch_padding, torch_pool)
        # Max pooling
        else:
            torch_pool = torch.nn.MaxPool3d(kernel, stride=stride)
            torch_model = torch.nn.Sequential(torch_padding, torch_pool)

        try:
            expected = torch_model(torch_input).numpy()
            self._test_model(
                builder.spec, {"data": input}, {"output": expected}, useCPUOnly=cpu_only
            )
            return True
        except AssertionError as e:
            print(e)
            return "test_case: %s, error: %s" % (test_case, e)

    @staticmethod
    def _validate_pooling_dimension(
        input_size, kernel_size, stride, start_padding, end_padding
    ):
        # https://adeshpande3.github.io/A-Beginner%27s-Guide-To-Understanding-Convolutional-Neural-Networks-Part-2/
        output_size = (
            input_size + start_padding + end_padding - kernel_size
        ) / stride + 1
        if output_size < 1:
            raise ValueError(
                "Dimension with input_size: %s, kernel_size: %s, stride: %s, start_padding: %s, end_padding: %s "
                "has output size of %s, but must be >= 1"
                % (
                    input_size,
                    kernel_size,
                    stride,
                    start_padding,
                    end_padding,
                    output_size,
                )
            )
        if input_size < kernel_size:
            raise ValueError(
                "Dimension has input_size (%s) less than kernel_size (%s)"
                % (input_size, kernel_size)
            )
        if (start_padding + end_padding) / 2 >= kernel_size / 2:
            raise ValueError(
                "The average of the start (%s) and end (%s) padding must be less than half the kernel size (%s / 2 = %s)"
                % (start_padding, end_padding, kernel_size, kernel_size / 2)
            )

    def test_pool3d_cpu(self):
        self._test_pool3d(cpu_only=True)

    def test_pool3d_gpu(self):
        self._test_pool3d(cpu_only=False)

    def _test_global_pool3d(self, cpu_only):
        shapes = ((1, 1, 1, 2, 2), (1, 1, 3, 3, 3), (3, 4, 10, 17, 90))
        pool_types = ("MAX", "AVERAGE")

        for shape in shapes:
            for pool_type in pool_types:
                test_case = "test_case:: shape: %s, pool_type: %s" % (shape, pool_type)
                print(test_case)
                input_features = [("data", datatypes.Array(*shape))]
                output_features = [("output", None)]
                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )
                builder.add_global_pooling3d(
                    name="pooling3d",
                    input_name="data",
                    output_name="output",
                    pooling_type=pool_type,
                )
                input = np.random.rand(*shape)

                # Expected output from Torch
                torch_input = torch.from_numpy(np.reshape(input, shape))
                if pool_type == "AVERAGE":
                    torch_pool = torch.nn.AvgPool3d(shape[-3:])
                else:
                    torch_pool = torch.nn.MaxPool3d(shape[-3:])
                exptected = torch_pool(torch_input).numpy()

                self._test_model(
                    builder.spec,
                    {"data": input},
                    {"output": exptected},
                    useCPUOnly=cpu_only,
                )

    def test_global_pool3d_cpu(self):
        self._test_global_pool3d(cpu_only=True)

    def test_global_pool3d_gpu(self):
        self._test_global_pool3d(cpu_only=False)

    def test_argsort_cpu(self, cpu_only=True):

        shapes = [(4,), (3, 4), (2, 5, 6), (3, 5, 2, 4), (4, 5, 3, 6, 7)]

        for shape in shapes:
            for descending in [False, True]:
                for axis in range(len(shape)):

                    input_features = [("data", datatypes.Array(*shape))]
                    output_features = [("output", None)]
                    builder = neural_network.NeuralNetworkBuilder(
                        input_features,
                        output_features,
                        disable_rank5_shape_mapping=True,
                    )
                    builder.add_argsort(
                        "argsort", "data", "output", axis=axis, descending=descending
                    )

                    x = np.random.rand(*shape)
                    if descending:
                        expected = {"output": np.argsort(-x, axis)}
                    else:
                        expected = {"output": np.argsort(x, axis)}

                    input = {"data": x}
                    self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_argsort_gpu(self):
        self.test_argsort_cpu(cpu_only=False)

    def test_upsample_pytorch_cpu(self):
        self.upsample_pytorch_test_iter(np.arange(1, 4), True)
        self.upsample_pytorch_test_iter(np.arange(1.0, 3.0, 0.66), True)

    def test_upsample_pytorch_gpu(self):
        self.upsample_pytorch_test_iter(np.arange(1, 4), False)
        self.upsample_pytorch_test_iter(np.arange(1.0, 3.0, 0.66), False)

    def upsample_pytorch_test_iter(self, scale_range, cpu_only):
        for align_corners in [False, True]:
            for scale_h in scale_range:
                for scale_w in scale_range:
                    for input_h in range(2, 6):
                        for input_w in range(2, 6):
                            self.upsample_pytorch_test(
                                input_h,
                                input_w,
                                scale_h,
                                scale_w,
                                align_corners,
                                cpu_only,
                            )

    def upsample_pytorch_test(self, h, w, scale_h, scale_w, align_corners, cpu_only):
        input_dim = (1, 1, h, w)
        if align_corners:
            linear_upsample_mode = "ALIGN_CORNERS_TRUE"
        else:
            linear_upsample_mode = "ALIGN_CORNERS_FALSE"

        input_features = [("data", datatypes.Array(*input_dim))]
        output_features = [("output", None)]

        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        builder.add_upsample(
            name="upsample",
            scaling_factor_h=scale_h,
            scaling_factor_w=scale_w,
            linear_upsample_mode=linear_upsample_mode,
            input_name="data",
            output_name="output",
            mode="BILINEAR",
        )

        input_tensor = np.reshape(np.arange(1.0, 1.0 + (h * w), 1.0), input_dim)
        input = {"data": input_tensor}

        # Get result from PyTorch
        x = torch.from_numpy(np.reshape(input_tensor, (1, 1, h, w)))
        m = torch.nn.Upsample(
            scale_factor=(scale_h, scale_w),
            mode="bilinear",
            align_corners=align_corners,
        )
        pytorch_output = m(x)

        # Expect PyTorch output matches CoreML output
        expected = {"output": pytorch_output.numpy()}

        self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)
        self.assertEquals(len(input_dim), builder._get_rank("output"))

    def test_slice_by_size_cpu(self, cpu_only=True):

        shapes = [(4,), (3, 4), (2, 5, 6), (3, 5, 2, 4), (4, 5, 3, 6, 7)]

        for shape in shapes:
            for axis in range(len(shape)):
                begin = np.random.randint(shape[axis])
                begin_input = np.array([begin]).astype(np.float32)
                size = np.random.randint(shape[axis] - begin) + 1

                x = np.random.rand(*shape)
                slices = []
                for i in range(len(shape)):
                    if i != axis:
                        slices.append(slice(None, None, None))
                    else:
                        slices.append(slice(begin, begin + size, 1))
                expected = {"output": x[slices]}

                input_features = [
                    ("data", datatypes.Array(*shape)),
                    ("begin", datatypes.Array(1)),
                ]
                output_features = [("output", datatypes.Array(*x[slices].shape))]
                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features, disable_rank5_shape_mapping=True
                )
                builder.add_slice_by_size(
                    "slice_by_size", ["data", "begin"], "output", axis=axis, size=size
                )

                input = {"data": x, "begin": begin_input}
                self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def _test_conv3d(self, cpu_only, full_test):
        # Input shape defined by us and PyTorch as [batch, channels, depth, height, width]
        input_shapes = [
            [1, 3, 3, 8, 8],
            [1, 1, 3, 8, 8],
            [1, 7, 8, 15, 63],
            [4, 32, 8, 16, 16],
        ]
        # Large enough kernels and/or input causes int overflow and seg fault: see rdar://60309763
        kernels = [[3, 3, 3], [2, 2, 2]]
        strides = [[1, 1, 1], [2, 2, 2]]
        dilations = [[1, 1, 1], [2, 2, 2]]
        has_biases = [True, False]
        # Note: PyTorch's `torch.nn.Conv3d` doesn't support these padding modes, just a single
        # padding value (for all dimensions) or 3 values (for each dimension)
        padding_modes = ["custom", "valid", "same"]
        # Padding shape is front, back, top, bottom, left, right
        paddings = [[0, 0, 0, 0, 0, 0], [1, 1, 1, 1, 1, 1]]

        # Add some additional test cases if `full_test` is True
        if full_test:
            input_shapes.extend([[1, 4, 3, 128, 128]])
            kernels.extend([[1, 2, 3], [5, 5, 5]])
            strides.extend([[1, 2, 3]])
            dilations.extend([[1, 2, 3]])
            paddings.extend([[2, 0, 2, 0, 2, 0], [0, 1, 2, 3, 4, 5]])

        test_case_format_str = (
            "Conv3d test case | Input shape: {}, Output channels: {}, Groups: {}, Kernel shape: {},"
            " Stride: {}, Padding: {}, Padding mode: {}, Dilation: {}, Has bias: {}"
        )

        for in_shape in input_shapes:
            # Test "normal" and depthwise convolution with corresponding groups and output channels
            groups_outchannels = [(1, 2), (in_shape[1], 2 * in_shape[1])]
            for kernel in kernels:
                for has_bias in has_biases:
                    for stride in strides:
                        for dilation in dilations:
                            for padding_mode in padding_modes:
                                # For all modes besides 'custom', the padding values are ignored
                                if padding_mode == "custom":
                                    loop_paddings = paddings
                                else:
                                    loop_paddings = [[0, 0, 0, 0, 0, 0]]
                                for padding in loop_paddings:
                                    for groups, output_channels in groups_outchannels:
                                        # Dilated kernel shape = (K - 1) * D + 1
                                        dilated_kernel = list(
                                            map(
                                                lambda k, d: (k - 1) * d + 1,
                                                kernel,
                                                dilation,
                                            )
                                        )

                                        # Use paddings if padding_mode is "custom", else compute
                                        # them according to
                                        # https://stanford.edu/~shervine/teaching/cs-230/cheatsheet-convolutional-neural-networks#filter
                                        if padding_mode == "same":
                                            pad_d = max(
                                                0,
                                                (
                                                    stride[0]
                                                    * math.ceil(
                                                        in_shape[2] / float(stride[0])
                                                    )
                                                    - in_shape[2]
                                                    + dilated_kernel[0]
                                                    - stride[0]
                                                )
                                                / 2.0,
                                            )
                                            pad_h = max(
                                                0,
                                                (
                                                    stride[1]
                                                    * math.ceil(
                                                        in_shape[3] / float(stride[1])
                                                    )
                                                    - in_shape[3]
                                                    + dilated_kernel[1]
                                                    - stride[1]
                                                )
                                                / 2.0,
                                            )
                                            pad_w = max(
                                                0,
                                                (
                                                    stride[2]
                                                    * math.ceil(
                                                        in_shape[4] / float(stride[2])
                                                    )
                                                    - in_shape[4]
                                                    + dilated_kernel[2]
                                                    - stride[2]
                                                )
                                                / 2.0,
                                            )

                                            # Depth
                                            padding[0] = int(math.floor(pad_d))
                                            padding[1] = int(math.ceil(pad_d))
                                            # Height
                                            padding[2] = int(math.floor(pad_h))
                                            padding[3] = int(math.ceil(pad_h))
                                            # Width
                                            padding[4] = int(math.floor(pad_w))
                                            padding[5] = int(math.ceil(pad_w))
                                        elif padding_mode == "valid":
                                            # Set to zero for PyTorch padding
                                            padding = [0] * 6
                                        elif padding_mode == "custom":
                                            # No-op: valid ignores padding and custom uses the
                                            # specified padding
                                            pass

                                        input_features = [
                                            ("data", datatypes.Array(*in_shape))
                                        ]
                                        output_features = [("output", None)]
                                        input_channels = in_shape[1]
                                        # [output_channels, kernel_channels, depth, height, width]
                                        weights_shape = [
                                            output_channels,
                                            int(input_channels / groups),
                                            kernel[0],
                                            kernel[1],
                                            kernel[2],
                                        ]

                                        # Init random input
                                        input_tensor = np.random.normal(size=in_shape)
                                        input_torch = torch.tensor(input_tensor)
                                        # Init random weights
                                        weights_tensor = np.random.normal(
                                            size=weights_shape
                                        )
                                        weights_torch = torch.DoubleTensor(
                                            weights_tensor
                                        )
                                        # Init random bias if applicable
                                        if has_bias:
                                            bias_tensor = np.random.normal(
                                                size=output_channels
                                            )
                                            bias_torch = torch.DoubleTensor(bias_tensor)
                                        else:
                                            bias_tensor = None
                                            bias_torch = None

                                        builder = neural_network.NeuralNetworkBuilder(
                                            input_features,
                                            output_features,
                                            disable_rank5_shape_mapping=True,
                                        )
                                        builder.add_convolution3d(
                                            name="conv3d",
                                            input_channels=input_channels,
                                            output_channels=output_channels,
                                            depth=kernel[0],
                                            height=kernel[1],
                                            width=kernel[2],
                                            W=weights_tensor,
                                            b=bias_tensor,
                                            has_bias=has_bias,
                                            groups=groups,
                                            stride_depth=stride[0],
                                            stride_height=stride[1],
                                            stride_width=stride[2],
                                            dilation_depth=dilation[0],
                                            dilation_height=dilation[1],
                                            dilation_width=dilation[2],
                                            padding_mode=padding_mode,
                                            padding_front=padding[0],
                                            padding_back=padding[1],
                                            padding_top=padding[2],
                                            padding_bottom=padding[3],
                                            padding_left=padding[4],
                                            padding_right=padding[5],
                                            input_name="data",
                                            output_name="output",
                                        )

                                        # Get PyTorch output to compare ours to
                                        # First pad, since PyTorch Conv3d only supports custom and
                                        # same symmetric padding. Padding shape is
                                        # (left, right, top, bottom, front, back)
                                        padded_input = input_torch
                                        if any(p > 0 for p in padding):
                                            torch_padding = (
                                                padding[4],
                                                padding[5],
                                                padding[2],
                                                padding[3],
                                                padding[0],
                                                padding[1],
                                            )
                                            pad_layer = torch.nn.ConstantPad3d(
                                                torch_padding, 0
                                            )
                                            padded_input = pad_layer(input_torch)
                                        # Check if dilated kernel size exceeds padded input size in
                                        # any dimension. If it does, it's not a valid convolution
                                        if (
                                            dilated_kernel[0] > padded_input.shape[2]
                                            or dilated_kernel[1] > padded_input.shape[3]
                                            or dilated_kernel[2] > padded_input.shape[4]
                                        ):
                                            print(
                                                "SKIPPING: Dilated kernel exceeds padded input."
                                            )
                                            continue
                                        # Using Sequential with a padding layer first produces
                                        # incorrect convolution output
                                        model = torch.nn.Sequential(
                                            torch.nn.Conv3d(
                                                input_channels,
                                                output_channels,
                                                kernel,
                                                stride=stride,
                                                padding=0,
                                                dilation=dilation,
                                                groups=groups,
                                                bias=False,
                                            )
                                        )
                                        with torch.no_grad():
                                            model[0].weight = torch.nn.Parameter(
                                                weights_torch
                                            )
                                            if has_bias:
                                                model[0].bias = torch.nn.Parameter(
                                                    bias_torch
                                                )
                                        torch_expected = model(padded_input)

                                        test_case = test_case_format_str.format(
                                            in_shape,
                                            output_channels,
                                            groups,
                                            weights_shape,
                                            stride,
                                            padding,
                                            padding_mode,
                                            dilation,
                                            has_bias,
                                        )
                                        try:
                                            self._test_model(
                                                builder.spec,
                                                {"data": input_tensor},
                                                {
                                                    "output": torch_expected.detach().numpy()
                                                },
                                                useCPUOnly=cpu_only,
                                                test_metric="SNR",
                                                SNR=40,
                                                validate_shapes_only=False,
                                            )
                                        except AssertionError as e:
                                            print(test_case)
                                            raise

    def test_conv3d_cpu_basic(self):
        self._test_conv3d(cpu_only=True, full_test=False)

    @pytest.mark.slow
    def test_conv3d_cpu_slow(self):
        self._test_conv3d(cpu_only=True, full_test=True)

    def test_conv3d_gpu_basic(self):
        self._test_conv3d(cpu_only=False, full_test=False)

    @pytest.mark.slow
    def test_conv3d_gpu_slow(self):
        self._test_conv3d(cpu_only=False, full_test=True)


@pytest.mark.slow
@unittest.skipUnless(
    _is_macos() and _macos_version() >= LAYERS_11_0_MACOS_VERSION,
    "Only supported on macOS 10.16+",
)
class ReorganizeDataTests(CorrectnessTest):
    def _to_rank_4(self, x):
        from_rank = len(x.shape)
        if from_rank == 3:
            return np.reshape(x, [1] + list(x.shape))
        elif from_rank == 4:
            return x
        elif from_rank == 5:
            return np.squeeze(x, axis=0)

    def _from_rank_4(self, x, to_rank):
        if to_rank == 3:
            return np.squeeze(x, axis=0)
        elif to_rank == 4:
            return x
        elif to_rank == 5:
            return np.reshape(x, [1] + list(x.shape))

    @unittest.skipIf(not _HAS_TF, MSG_TF1_NOT_FOUND)
    def test_depth_to_space_cpu(self, cpu_only=True):

        params_dict = {
            "block_size": [2, 3, 4],
            "channels_div_bsq": [1, 2, 3, 7],
            "spatial": [[2, 3], [4, 4], [1, 1]],
            "batch_size": [None, 1, 2],
            "seq_length": [None, 1],
        }
        params_product = list(itertools.product(*params_dict.values()))
        for param in params_product:
            param = dict(zip(params_dict.keys(), param))
            # Create input based on params
            block_size = param["block_size"]
            bsq = block_size * block_size
            input_shape = [bsq * param["channels_div_bsq"]] + param["spatial"]
            if param["batch_size"] is not None:
                input_shape = [param["batch_size"]] + input_shape
            if param["seq_length"] is not None:
                input_shape = [param["seq_length"]] + input_shape
            rank = len(input_shape)
            x = np.random.random(input_shape)
            input = {"data": x}

            # Set up network
            input_features = [("data", datatypes.Array(*input_shape))]
            output_features = [("output", None)]
            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )
            builder.add_reorganize_data(
                "reorganize_data",
                "data",
                "output",
                mode="DEPTH_TO_SPACE",
                block_size=block_size,
            )

            # Run tensorflow to calculate expected values
            with tf.Session() as sess:
                # TensorFlow requires rank 4, NHWC order on CPU
                x_tf = self._to_rank_4(x).transpose(0, 2, 3, 1)
                out_tf = sess.run(
                    tf.nn.depth_to_space(x_tf, block_size, data_format="NHWC")
                )
                out = self._from_rank_4(out_tf.transpose(0, 3, 1, 2), to_rank=rank)
                expected = {"output": out}

            # Run model to calculate CoreML values and compare with expected
            self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    def test_depth_to_space_gpu(self):
        self.test_depth_to_space_cpu(cpu_only=False)

    @unittest.skipIf(
        _macos_version() < LAYERS_11_0_MACOS_VERSION,
        "macOS 11.0+ required. Skipping tests.",
    )
    def test_pixel_shuffle_cpu(self, cpu_only=True):

        params_dict = {
            "block_size": [2, 3, 4],
            "channels_div_bsq": [1, 2, 3, 7],
            "spatial": [[2, 3], [4, 4], [1, 1]],
            "batch_size": [None, 1, 2],
            "seq_length": [None, 1],
        }
        params_product = list(itertools.product(*params_dict.values()))
        for param in params_product:
            param = dict(zip(params_dict.keys(), param))
            # Create input based on params
            block_size = param["block_size"]
            bsq = block_size * block_size
            input_shape = [bsq * param["channels_div_bsq"]] + param["spatial"]
            if param["batch_size"] is not None:
                input_shape = [param["batch_size"]] + input_shape
            if param["seq_length"] is not None:
                input_shape = [param["seq_length"]] + input_shape
            rank = len(input_shape)
            x = np.random.random(input_shape)
            input = {"data": x}

            # Set up network
            input_features = [("data", datatypes.Array(*input_shape))]
            output_features = [("output", None)]
            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features, disable_rank5_shape_mapping=True
            )
            builder.add_reorganize_data(
                "reorganize_data",
                "data",
                "output",
                mode="PIXEL_SHUFFLE",
                block_size=block_size,
            )

            # Run pytorch to calculate expected values
            x_torch = torch.from_numpy(self._to_rank_4(x))
            out_torch = torch.pixel_shuffle(x_torch, upscale_factor=block_size)
            out = self._from_rank_4(out_torch.numpy(), to_rank=rank)
            expected = {"output": out}

            # Run model to calculate CoreML values and compare with expected
            self._test_model(builder.spec, input, expected, useCPUOnly=cpu_only)

    @unittest.skipIf(
        _macos_version() < LAYERS_11_0_MACOS_VERSION,
        "macOS 10.16+ required. Skipping tests.",
    )
    def test_pixel_shuffle_gpu(self):
        self.test_pixel_shuffle_cpu(cpu_only=False)

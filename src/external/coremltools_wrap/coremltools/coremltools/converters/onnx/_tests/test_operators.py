from __future__ import absolute_import as _
from __future__ import division as _
from __future__ import print_function as _
from __future__ import unicode_literals as _

import unittest
import numpy as np
from coremltools._deps import _HAS_ONNX, MSG_ONNX_NOT_FOUND

if _HAS_ONNX:
    import onnx
    from onnx.numpy_helper import from_array
    from coremltools.converters.onnx import convert
    from ._test_utils import (
        _onnx_create_single_node_model,
        _test_single_node,
        _random_array,
        _conv_pool_output_size,
        _assert_outputs,
    )

from typing import Text

from coremltools.models.utils import _macos_version

MIN_MACOS_VERSION_10_15 = (10, 15)

ONNX_SHAPE_INFERENCE_FAILS = True


@unittest.skipUnless(_HAS_ONNX, MSG_ONNX_NOT_FOUND)
class SingleOperatorTest(unittest.TestCase):
    def test_conv(self):  # type: () -> None
        kernel_shape = (3, 2)
        strides = (2, 3)
        pads = (4, 2, 4, 2)
        dilations = (1, 2)
        group = 1
        weight = from_array(_random_array((16, 3, 3, 2)), name="weight")

        input_shape = (1, 3, 224, 224)
        output_size = _conv_pool_output_size(
            input_shape, dilations, kernel_shape, pads, strides
        )

        output_shape = (1, int(weight.dims[0]), output_size[0], output_size[1])

        _test_single_node(
            "Conv",
            [input_shape],
            [output_shape],
            initializer=[weight],
            dilations=dilations,
            group=group,
            kernel_shape=kernel_shape,
            pads=pads,
            strides=strides,
        )

    def test_conv_transpose(self):  # type: () -> None
        kernel_shape = (3, 3)
        pads = (0, 0, 0, 0)
        C_in = 3
        C_out = 12
        H_in, W_in = 30, 30
        strides = (2, 2)

        input_shape = (1, C_in, H_in, W_in)
        weight = from_array(
            _random_array((C_in, C_out, kernel_shape[0], kernel_shape[1])),
            name="weight",
        )

        H_out = (H_in - 1) * strides[0] + kernel_shape[0] - pads[0] - pads[2]
        W_out = (W_in - 1) * strides[1] + kernel_shape[1] - pads[1] - pads[3]
        output_shape = (1, C_out, H_out, W_out)

        _test_single_node(
            "ConvTranspose",
            [input_shape],
            [output_shape],
            initializer=[weight],
            # Default values for other attributes: dilations=[1, 1], group=1
            strides=strides,
            kernel_shape=kernel_shape,
            pads=pads,
            output_padding=(0, 0),
        )

    def test_conv_without_pads(self):  # type: () -> None
        kernel_shape = (3, 2)
        strides = (2, 3)
        dilations = (1, 2)
        group = 1
        weight = from_array(_random_array((16, 3, 3, 2)), name="weight")

        input_shape = (1, 3, 224, 224)
        output_size = _conv_pool_output_size(
            input_shape, dilations, kernel_shape, [0, 0, 0, 0], strides
        )

        output_shape = (1, int(weight.dims[0]), output_size[0], output_size[1])
        _test_single_node(
            "Conv",
            [input_shape],
            [output_shape],
            initializer=[weight],
            dilations=dilations,
            group=group,
            kernel_shape=kernel_shape,
            strides=strides,
        )

    def test_max_pool(self):  # type: () -> None
        kernel_shape = (5, 3)
        pads = (2, 1, 2, 1)
        strides = (1, 2)

        input_shape = (1, 3, 224, 224)

        output_size = _conv_pool_output_size(
            input_shape, [1, 1], kernel_shape, pads, strides
        )

        output_shape = (1, 3, output_size[0], output_size[1])

        _test_single_node(
            "MaxPool",
            [input_shape],
            [output_shape],
            test_name="test_max_pool_1",
            kernel_shape=kernel_shape,
            pads=pads,
            strides=strides,
        )

        output_size = _conv_pool_output_size(
            input_shape, [1, 1], kernel_shape, [0, 0, 0, 0], strides
        )
        output_shape = (1, 3, output_size[0], output_size[1])
        _test_single_node(
            "MaxPool",
            [input_shape],
            [output_shape],
            test_name="test_max_pool_2",
            kernel_shape=kernel_shape,
            strides=strides,
        )

    @unittest.skip("Skip due to internal CoreML CPU backend issue")
    def test_avg_pool(self):  # type: () -> None
        kernel_shape = (5, 3)
        pads = (2, 1, 2, 1)
        strides = (1, 2)

        input_shape = (1, 3, 224, 224)
        output_size = _conv_pool_output_size(
            input_shape, (1, 1), kernel_shape, pads, strides
        )
        output_shape = (1, 3, output_size[0], output_size[1])
        _test_single_node(
            "AveragePool",
            [input_shape],
            [output_shape],
            test_name="test_avg_pool_1",
            kernel_shape=kernel_shape,
            pads=pads,
            strides=strides,
        )

        output_size = _conv_pool_output_size(
            input_shape, (1, 1), kernel_shape, [0, 0, 0, 0], strides
        )
        output_shape = (1, 3, output_size[0], output_size[1])
        _test_single_node(
            "AveragePool",
            [input_shape],
            [output_shape],
            test_name="test_avg_pool_2",
            kernel_shape=kernel_shape,
            strides=strides,
        )

    def test_bn(self):  # type: () -> None
        scale = from_array(_random_array((3,)), name="scale")
        bias = from_array(_random_array((3,)), name="bias")
        mean = from_array(_random_array((3,)), name="mean")
        var = from_array(_random_array((3,)), name="var")

        epsilon = 1e-5
        momentum = 0.001

        op_types = ["BatchNormalization", "SpatialBN"]
        for op_type in op_types:
            _test_single_node(
                "BatchNormalization",
                [(1, 3, 224, 224)],
                [(1, 3, 224, 224)],
                initializer=[scale, bias, mean, var],
                epsilon=epsilon,
                momentum=momentum,
            )

            # epsilon by default
            _test_single_node(
                "BatchNormalization",
                [(1, 3, 224, 224)],
                [(1, 3, 224, 224)],
                initializer=[scale, bias, mean, var],
                # epsilon=epsilon,
                momentum=momentum,
            )

    def test_gemm(self, minimum_ios_deployment_target="12"):  # type: () -> None
        input_shape = (1, 2048)
        output_shape = (1, 5)
        W = from_array(_random_array((output_shape[1], input_shape[1])), name="weight")
        b = from_array(_random_array((output_shape[1],)), name="bias")
        _test_single_node(
            "Gemm",
            [input_shape],
            [output_shape],
            initializer=[W, b],
            decimal=3,
            transB=1,
            minimum_ios_deployment_target=minimum_ios_deployment_target,
        )

    @unittest.skipIf(
        _macos_version() < MIN_MACOS_VERSION_10_15,
        "macOS 10.15+ required. Skipping test.",
    )
    def test_gemm_ios13(self):
        self.test_gemm(minimum_ios_deployment_target="13")

    def test_gemm_transB_off(
        self, minimum_ios_deployment_target="12"
    ):  # type: () -> None
        input_shape = (1, 2048)
        output_shape = (1, 5)
        W = from_array(_random_array((input_shape[1], output_shape[1])), name="weight")
        b = from_array(_random_array((output_shape[1],)), name="bias")
        _test_single_node(
            "Gemm",
            [input_shape],
            [output_shape],
            initializer=[W, b],
            decimal=3,
            transB=0,
            minimum_ios_deployment_target=minimum_ios_deployment_target,
        )

    @unittest.skipIf(
        _macos_version() < MIN_MACOS_VERSION_10_15,
        "macOS 10.15+ required. Skipping test.",
    )
    def test_gemm_transB_off_ios13(self):
        self.test_gemm_transB_off(minimum_ios_deployment_target="13")

    def test_lrn(self):  # type: () -> None
        _test_single_node(
            "LRN",
            [(1, 3, 224, 224)],
            [(1, 3, 224, 224)],
            alpha=9.99e-5,
            beta=0.75,
            bias=1.0,
            size=5,
        )

    @unittest.skipIf(
        _macos_version() < MIN_MACOS_VERSION_10_15,
        "macOS 10.15+ required. Skipping test.",
    )
    @unittest.skip("Failing: wrong input type")
    def test_split_axis_0_rank_3(
        self, minimum_ios_deployment_target="12"
    ):  # type: () -> None
        _test_single_node(
            "Split",
            [(2, 1, 200)],
            [(1, 1, 200), (1, 1, 200)],
            axes=0,
            minimum_ios_deployment_target=minimum_ios_deployment_target,
        )

    @unittest.skipIf(
        _macos_version() < MIN_MACOS_VERSION_10_15,
        "macOS 10.15+ required. Skipping test.",
    )
    def test_concat(self, minimum_ios_deployment_target="13"):  # type: () -> None
        _test_single_node(
            "Concat",
            [(1, 2, 200), (1, 2, 200)],
            [(2, 2, 200)],
            axis=0,
            minimum_ios_deployment_target=minimum_ios_deployment_target,
        )

    @unittest.skipIf(
        _macos_version() < MIN_MACOS_VERSION_10_15,
        "macOS 10.15+ required. Skipping test.",
    )
    @unittest.skip("Failing: wrong input type")
    def test_gather(self, minimum_ios_deployment_target="13"):  # type: () -> None
        _test_single_node(
            "Gather",
            [(5, 4, 3), (3,)],
            [(3, 4, 3)],
            axis=0,
            minimum_ios_deployment_target=minimum_ios_deployment_target,
        )

    @unittest.skipIf(
        _macos_version() < MIN_MACOS_VERSION_10_15,
        "macOS 10.15+ required. Skipping test.",
    )
    @unittest.skip("Failing: wrong input type")
    def test_reshape_same_rank(
        self, minimum_ios_deployment_target="13"
    ):  # type: () -> None
        _test_single_node(
            "Reshape",
            [(5, 4, 3), (3,)],
            [(4, 5, 3)],
            minimum_ios_deployment_target=minimum_ios_deployment_target,
        )

    @unittest.skipIf(
        _macos_version() < MIN_MACOS_VERSION_10_15,
        "macOS 10.15+ required. Skipping test.",
    )
    @unittest.skip("Failing: wrong input type")
    def test_reshape_same_rank_infer_shape(
        self, minimum_ios_deployment_target="13"
    ):  # type: () -> None
        _test_single_node(
            "Reshape",
            [(5, 4, 3), (3,)],
            [(5, 2, 6)],
            minimum_ios_deployment_target=minimum_ios_deployment_target,
        )

    # TODO: add test_reshape_diff_rank_infer_shape where shape is Constant and known
    # to test rank-4 into rank-3 reshape with shape inferencing
    @unittest.skipIf(
        _macos_version() < MIN_MACOS_VERSION_10_15,
        "macOS 10.15+ required. Skipping test.",
    )
    @unittest.skip("Failing: wrong input type")
    def test_reshape_dynamic(
        self, minimum_ios_deployment_target="13"
    ):  # type: () -> None
        _test_single_node(
            "Reshape",
            [(5, 4, 3, 2), (3,)],
            [(2, 3, 20)],
            minimum_ios_deployment_target=minimum_ios_deployment_target,
        )

    @unittest.skipIf(
        _macos_version() < MIN_MACOS_VERSION_10_15,
        "macOS 10.15+ required. Skipping test.",
    )
    def test_squeeze(self, minimum_ios_deployment_target="13"):  # type: () -> None
        _test_single_node(
            "Squeeze",
            [(5, 1, 3, 1, 1)],
            [(5, 3)],
            axes=[1, 3, 4],
            minimum_ios_deployment_target=minimum_ios_deployment_target,
        )

    @unittest.skipIf(
        _macos_version() < MIN_MACOS_VERSION_10_15,
        "macOS 10.15+ required. Skipping test.",
    )
    def test_transpose_default(
        self, minimum_ios_deployment_target="13"
    ):  # type: () -> None
        _test_single_node(
            "Transpose",
            [(5, 3, 4, 6, 2)],
            [(2, 6, 4, 3, 5)],
            minimum_ios_deployment_target=minimum_ios_deployment_target,
        )

    @unittest.skipIf(
        ONNX_SHAPE_INFERENCE_FAILS,
        "ONNX Shape inference fails to recongnize correct shape",
    )
    @unittest.skipIf(
        _macos_version() < MIN_MACOS_VERSION_10_15,
        "macOS 10.15+ required. Skipping test.",
    )
    def test_transpose_permute(
        self, minimum_ios_deployment_target="13"
    ):  # type: () -> None
        _test_single_node(
            "Transpose",
            [(5, 3, 4, 6, 2)],
            [(2, 3, 4, 6, 5)],
            axes=[4, 1, 2, 3, 0],
            minimum_ios_deployment_target=minimum_ios_deployment_target,
        )

    @unittest.skipIf(
        ONNX_SHAPE_INFERENCE_FAILS,
        "ONNX Shape inference fails to recongnize correct shape",
    )
    @unittest.skipIf(
        _macos_version() < MIN_MACOS_VERSION_10_15,
        "macOS 10.15+ required. Skipping test.",
    )
    def test_unsqueeze(self, minimum_ios_deployment_target="13"):  # type: () -> None
        _test_single_node(
            "Unsqueeze",
            [(5, 3, 4)],
            [(1, 5, 1, 3, 4)],
            axes=[0, 1],
            minimum_ios_deployment_target=minimum_ios_deployment_target,
        )

    @unittest.skip(
        "Error while preparing Caffe2 backend. Maybe something is incorrect in ONNX model definition"
    )
    def skip_test_lstm(self):  # type: () -> None
        x = 4
        h = 2
        seq_length = 3
        W = from_array(_random_array((4 * h, x)), name="gate_weights")
        R = from_array(_random_array((4 * h, h)), name="recursion_weights")
        B = from_array(_random_array((8 * h,)), name="biases")
        seq_lens_input = from_array(
            np.array([seq_length]).astype(np.int32), name="seq_lens_input"
        )
        initial_h = from_array(np.zeros((1, 1, h)).astype(np.float32), name="initial_h")
        initial_c = from_array(np.zeros((1, 1, h)).astype(np.float32), name="initial_c")

        input_shape = (seq_length, 1, x)
        output_shape_all = (seq_length, 1, h)
        output_shape_last = (1, 1, h)

        onnx_model = _onnx_create_single_node_model(
            "LSTM",
            [input_shape],
            [output_shape_all, output_shape_last],
            initializer=[W, R, B, seq_lens_input, initial_h, initial_c],
            hidden_size=h,
        )
        X = np.random.rand(*input_shape).astype("float32")  # type: ignore
        import caffe2.python.onnx.backend

        prepared_backend = caffe2.python.onnx.backend.prepare(onnx_model)
        out = prepared_backend.run({"input0": X})
        caffe2_out_all = out["output0"]
        caffe2_out_last = out["output1"]

        coreml_model = convert(onnx_model)
        inputdict = {}
        inputdict["input0"] = X
        inputdict["initial_h"] = np.zeros((h), dtype=np.float32)
        inputdict["initial_c"] = np.zeros((h), dtype=np.float32)
        coreml_out_dict = coreml_model.predict(inputdict, useCPUOnly=True)
        coreml_out_all = coreml_out_dict["output0"]
        coreml_out_last = coreml_out_dict["output1"]

        _assert_outputs(caffe2_out_all.flatten(), coreml_out_all.flatten(), decimal=5)
        _assert_outputs(caffe2_out_last.flatten(), coreml_out_last.flatten(), decimal=5)

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil import testing_reqs
from coremltools.converters.mil.mil import get_new_symbol
from coremltools.converters.mil.testing_reqs import *

from .testing_utils import run_compare_builder

backends = testing_reqs.backends


class TestConvTranspose:

    @pytest.mark.skipif(not testing_reqs._HAS_TORCH, reason="PyTorch not installed.")
    @pytest.mark.parametrize(
        ",".join(
            [
                "use_cpu_only",
                "backend",
                "conv_dim",
                "padding",
                "DHWKdKhKw",
                "stride",
                "dilation",
                "has_bias",
                "groups",
                "test_symbolic",
                "test_output_shape",
            ]
        ),
        itertools.product(
            [True, False],
            backends,
            ["conv1d", "conv2d"], # FIXME: rdar://65198011 (Re-enable Conv3dTranspose and DynamicTile unit tests)
            [(1, 2, 3), (2, 2, 2)],
            [(7, 7, 7, 2, 2, 2), (10, 12, 14, 3, 2, 4)],
            [(1, 1, 1), (2, 1, 2)],
            [(1, 1, 1), (1, 2, 1)],
            [True, False],
            # TODO: rdar://64018048 (Deconv3d: Incorrect output for group > 1)
            [1],
            [True, False],
            # TODO: rdar://63968613 ([deconv3d] Deconv_3d top_shapes_for_bottom_shapes does not sets output channel if output shape is provided)
            [False],
        ),
    )
    def test_builder_to_backend_stress(
        self,
        use_cpu_only,
        backend,
        conv_dim,
        padding,
        DHWKdKhKw,
        stride,
        dilation,
        has_bias,
        groups,
        test_symbolic,
        test_output_shape,
    ):
        D, H, W, Kd, Kh, Kw = DHWKdKhKw
        N, C_in, C_out = 1, 1 * groups, 2 * groups

        import torch
        import torch.nn as nn

        isDeconv1d = conv_dim == "conv1d"
        isDeconv2d = conv_dim == "conv2d"

        if isDeconv1d:
            strides = [stride[0]]
            dilations = [dilation[0]]
            kernels = [Kh]
            m = nn.ConvTranspose1d(
                C_in,
                C_out,
                kernels,
                stride=strides,
                dilation=dilations,
                bias=has_bias,
                groups=groups,
                padding=padding[0],
            )
            input_shape = [N, C_in, H]
            paddings = [padding[0], padding[0]]

        elif isDeconv2d:
            strides = [stride[0], stride[1]]
            dilations = [dilation[0], dilation[1]]
            kernels = [Kh, Kw]
            m = nn.ConvTranspose2d(
                C_in,
                C_out,
                kernels,
                stride=strides,
                dilation=dilations,
                bias=has_bias,
                groups=groups,
                padding=(padding[0], padding[1]),
            )
            input_shape = [N, C_in, H, W]
            paddings = [padding[0], padding[0], padding[1], padding[1]]
        else:
            strides = [stride[0], stride[1], stride[2]]
            dilations = [dilation[0], dilation[1], dilation[2]]
            kernels = [Kd, Kh, Kw]
            m = nn.ConvTranspose3d(
                C_in,
                C_out,
                kernels,
                stride=strides,
                dilation=dilations,
                bias=has_bias,
                groups=groups,
                padding=padding,
            )
            input_shape = [N, C_in, D, H, W]
            paddings = [
                padding[0],
                padding[0],
                padding[1],
                padding[1],
                padding[2],
                padding[2],
            ]

        wts = m.state_dict()
        weight = wts["weight"].detach().numpy()
        bias = wts["bias"].detach().numpy() if has_bias else None

        # Reshape to CoreML format
        # PyTorch weight format: C_in, C_out, H, W
        # MIL weight format: C_out, C_in, H, W
        if isDeconv1d:
            weight = np.transpose(weight, [1, 0, 2])
        elif isDeconv2d:
            weight = np.transpose(weight, [1, 0, 2, 3])
        else:
            weight = np.transpose(weight, [1, 0, 2, 3, 4])

        input = torch.randn(*input_shape)
        output = m(input)
        output = output.detach().numpy()
        input = input.detach().numpy()

        output_shape = list(output.shape)
        if test_symbolic:
            # For symbolic input test
            # Make Batch Size and input channel as symbolic
            symbolic_batch_size = get_new_symbol()
            input_shape[0] = symbolic_batch_size
            output_shape[0] = symbolic_batch_size

        expected_output_types = tuple(output_shape[:]) + (types.fp32,)
        expected_outputs = [output]

        input_placeholders = {"x": mb.placeholder(shape=input_shape)}
        input_values = {"x": input}

        def build(x):
            arguments = {
                "x": x,
                "weight": weight,
                "pad": paddings,
                "pad_type": "custom",
                "strides": strides,
                "dilations": dilations,
                "groups": groups,
            }
            if has_bias:
                arguments["bias"] = bias
            if test_output_shape:
                arguments["output_shape"] = output.shape[2:]
            return mb.conv_transpose(**arguments)

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestConv:
    @pytest.mark.skipif(not testing_reqs._HAS_TORCH, reason="PyTorch not installed.")
    @pytest.mark.parametrize(
        ",".join(
            [
                "use_cpu_only",
                "backend",
                "conv_dim",
                "padding",
                "DHWKdKhKw",
                "stride",
                "dilation",
                "has_bias",
                "groups",
                "symbolic",
            ]
        ),
        itertools.product(
            [True, False],
            backends,
            ["conv1d", "conv2d", "conv3d"],
            [(1, 1, 1), (2, 2, 2)],
            [(5, 5, 5, 4, 4, 4), (10, 12, 14, 3, 2, 4)],
            [(1, 1, 1), (1, 2, 1)],
            [(1, 1, 1), (1, 2, 1)],
            [True, False],
            [1, 2],
            [True, False],
        ),
    )
    def test_builder_to_backend_stress(
        self,
        use_cpu_only,
        backend,
        conv_dim,
        padding,
        DHWKdKhKw,
        stride,
        dilation,
        has_bias,
        groups,
        symbolic,
    ):
        D, H, W, Kd, Kh, Kw = DHWKdKhKw
        N, C_in, C_out = 1, 1 * groups, 2 * groups

        import torch
        import torch.nn as nn

        isConv1d = conv_dim == "conv1d"
        isConv2d = conv_dim == "conv2d"

        if isConv1d:
            strides = [stride[0]]
            dilations = [dilation[0]]
            kernels = [Kh]
            m = nn.Conv1d(
                C_in,
                C_out,
                kernels,
                stride=strides,
                dilation=dilations,
                bias=has_bias,
                groups=groups,
                padding=padding[0],
            )
            input_shape = [N, C_in, H]
            paddings = [padding[0], padding[0]]
        elif isConv2d:
            strides = [stride[0], stride[1]]
            dilations = [dilation[0], dilation[1]]
            kernels = [Kh, Kw]
            m = nn.Conv2d(
                C_in,
                C_out,
                kernels,
                stride=strides,
                dilation=dilations,
                bias=has_bias,
                groups=groups,
                padding=(padding[0], padding[1]),
            )
            input_shape = [N, C_in, H, W]
            paddings = [padding[0], padding[0], padding[1], padding[1]]
        else:
            strides = [stride[0], stride[1], stride[2]]
            dilations = [dilation[0], dilation[1], dilation[2]]
            kernels = [Kd, Kh, Kw]
            m = nn.Conv3d(
                C_in,
                C_out,
                kernels,
                stride=strides,
                dilation=dilations,
                bias=has_bias,
                groups=groups,
                padding=padding,
            )
            input_shape = [N, C_in, D, H, W]
            paddings = [
                padding[0],
                padding[0],
                padding[1],
                padding[1],
                padding[2],
                padding[2],
            ]

        wts = m.state_dict()
        weight = wts["weight"].detach().numpy()
        bias = wts["bias"].detach().numpy() if has_bias else None

        # PyTorch and CoreML weight format is same
        # PyTorch weight format: C_out, C_in, H, W
        # MIL weight format: C_out, C_in, H, W

        input = torch.randn(*input_shape)
        output = m(input)
        output = output.detach().numpy()
        input = input.detach().numpy()

        output_shape = list(output.shape)
        if symbolic:
            # For symbolic input test
            # Make Batch Size and input channel as symbolic
            symbolic_batch_size = get_new_symbol()
            input_shape[0] = symbolic_batch_size
            output_shape[0] = symbolic_batch_size

        expected_output_types = tuple(output_shape[:]) + (types.fp32,)
        expected_outputs = [output]

        input_placeholders = {"x": mb.placeholder(shape=input_shape)}
        input_values = {"x": input}

        def build(x):
            arguments = {
                "x": x,
                "weight": weight,
                "pad": paddings,
                "pad_type": "custom",
                "strides": strides,
                "dilations": dilations,
                "groups": groups,
            }
            if has_bias:
                arguments["bias"] = bias
            return mb.conv(**arguments)

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

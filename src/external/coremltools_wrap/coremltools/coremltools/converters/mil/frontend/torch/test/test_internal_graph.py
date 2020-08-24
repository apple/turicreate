#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import itertools

import numpy as np
import pytest

torch = pytest.importorskip("torch")

import torch.nn as nn
import torch.nn.functional as F

from coremltools.converters.mil.mil import types
from coremltools.converters.mil.mil import Builder as mb
from coremltools.converters.mil.mil import Function, get_new_symbol
from coremltools.converters.mil.mil.var import Var

from .. import ops
from ..converter import TorchConverter, TranscriptionContext
from ..internal_graph import InternalTorchIRNode


class TestTorchOps:
    """Class containing tests for converting TorchIR -> CoreML ops.

    These tests interface with only the InternalTorchIRGraph and do not
    build a torch module. Thus, they are much faster then the numerical tests.
    However, for some ops it is necessary to use the torch module to verify
    numerical output so they are placed the numerical tests.

    NOTE: Confused where @context is coming from? Its from the pytest fixture defined below.
    """

    @pytest.fixture
    def context(self):
        return TranscriptionContext()

    @pytest.fixture
    def set_random_seeds(self):
        torch.manual_seed(1)
        np.random.seed(1)

    @pytest.mark.parametrize("dtype", [torch.bool, torch.float, torch.int])
    def test_constant(self, context, dtype):
        test_data = torch.ones(1, dtype=dtype)
        node = InternalTorchIRNode(
            attr={"value": test_data}, kind="constant", inputs=[], outputs=["1"]
        )
        ssa = self._construct_test_graph(context, ops.constant, node, "1")
        assert np.allclose(test_data, ssa.val)
        assert test_data.shape == ssa.shape

    def test_constant_magic(self, context):
        test_val = ops.PYTORCH_MAGIC_DEFAULT
        node = InternalTorchIRNode(
            attr={"value": test_val}, kind="constant", inputs=[], outputs=["1"]
        )
        ssa = self._construct_test_graph(context, ops.constant, node, "1")
        # We expect the magic default to get converted to None
        assert ssa is None

    @staticmethod
    def _gen_constants(size, vals):
        """Helper function. Generates a list of internal constant nodes.

        Arguments:
            size: number of constants to generate
            vals: Either a list of values for each constant or one value used for all constants."""
        is_list = isinstance(vals, list)
        if is_list:
            if len(vals) != size:
                raise ValueError("len(@vals): {} != size: {}".format(len(vals), size))
        constants = []
        for index in range(size):
            if is_list:
                val = vals[index]
            else:
                val = vals
            constants.append(
                InternalTorchIRNode(
                    attr={"value": val},
                    kind="constant",
                    inputs=[],
                    outputs=[str(index)],
                )
            )
        input_list = [str(i) for i in range(size)]
        output_name = str(len(input_list))
        return constants, input_list, output_name

    @staticmethod
    def _construct_test_graph(
        context, test_op, test_node, output_name=None, graph_inputs=None, constants=None
    ):
        """ Construct an Function for the given @graph_inputs, @constants,
            and @test_node. Returns the output of the graph, which is the ssa
            Var of the given @output_name.
        """
        if graph_inputs is None:
            graph_inputs = {}
        if constants is None:
            constants = []

        with Function(inputs=graph_inputs) as ssa_func:
            for name in ssa_func.inputs.keys():
                context.add(ssa_func.inputs[name])
            for node in constants:
                ops.constant(context, node)
            test_op(context, test_node)

        ssa = None
        if output_name:
            ssa = context[output_name]
        return ssa

    def _test_elementwise_binary(
        self, context, op_name, op, test_input, num_constants, expected_result
    ):
        """Helper function, runs op on test input and compares against expected result"""
        constants, input_list, output_name = self._gen_constants(
            num_constants, test_input
        )
        eb_node = InternalTorchIRNode(
            kind=op_name, inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, op, eb_node, output_name, constants=constants
        )
        np.testing.assert_allclose(expected_result, ssa.val, atol=1e-7)

    def _test_cast(self, context, test_val, op_kind, op_func, python_type):
        constants, input_list, output_name = self._gen_constants(1, [test_val])
        node = InternalTorchIRNode(
            kind=op_kind, inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, op_func, node, output_name, constants=constants
        )
        assert ssa.val == python_type(test_val)

    def _test_activation(
        self, context, input_shape, constants_list, op_kind, op_func, torch_func, atol
    ):
        test_input = torch.rand(input_shape)
        constants, input_list, output_name = self._gen_constants(
            len(constants_list) + 1, [test_input] + constants_list
        )
        node = InternalTorchIRNode(
            kind=op_kind, inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, op_func, node, output_name, constants=constants
        )
        expected_result = torch_func(test_input).numpy()
        np.testing.assert_allclose(expected_result, ssa.val, atol=atol)

    def test_add(self, context):
        test_input_1 = np.random.rand(2, 3)
        test_input_2 = np.random.rand(2, 3)
        scale_factor = 1
        self._test_elementwise_binary(
            context,
            "Add",
            ops.add,
            [test_input_1, test_input_2, scale_factor],
            3,
            test_input_1 + test_input_2,
        )

    def test_add_no_scale_factor(self, context):
        test_input_1 = np.random.rand(2, 3)
        test_input_2 = np.random.rand(2, 3)
        self._test_elementwise_binary(
            context,
            "Add",
            ops.add,
            [test_input_1, test_input_2],
            2,
            test_input_1 + test_input_2,
        )

    @pytest.mark.parametrize(
        "test_input_1, test_input_2",
        [(np.random.rand(3, 2), np.random.rand(3, 2)), (np.random.rand(3, 2), 5),],
    )
    def test_sub(self, context, test_input_1, test_input_2):
        scale_factor = 1
        self._test_elementwise_binary(
            context,
            "Sub",
            ops.sub,
            [test_input_1, test_input_2, scale_factor],
            3,
            test_input_1 - test_input_2,
        )

    @pytest.mark.parametrize(
        "test_input_1, test_input_2",
        [(np.random.rand(3, 2), np.random.rand(3, 2)), (np.random.rand(3, 2), 5),],
    )
    def test_rsub(self, context, test_input_1, test_input_2):
        scale_factor = 1
        self._test_elementwise_binary(
            context,
            "rsub",
            ops.sub,
            [test_input_1, test_input_2, scale_factor],
            3,
            # Note the reversal of arg ordering relative to 'sub'
            test_input_2 - test_input_1,
        )

    def test_mul(self, context):
        test_input_1 = np.random.rand(3, 2)
        test_input_2 = np.random.rand(3, 2)
        self._test_elementwise_binary(
            context,
            "Mul",
            ops.mul,
            [test_input_1, test_input_2],
            2,
            test_input_1 * test_input_2,
        )

    def test_div(self, context):
        test_input_1 = np.random.rand(3, 2)
        test_input_2 = np.random.rand(3, 2)
        self._test_elementwise_binary(
            context,
            "Div",
            ops.div,
            [test_input_1, test_input_2],
            2,
            np.divide(test_input_1, test_input_2),
        )

    def test_floor_divide(self, context):
        test_input_1 = np.random.randint(low=1, high=100, size=(3, 2))
        test_input_2 = np.random.randint(low=1, high=100, size=(3, 2))
        self._test_elementwise_binary(
            context,
            "floor_divide",
            ops.floor_divide,
            [test_input_1, test_input_2],
            2,
            np.floor_divide(test_input_1, test_input_2),
        )

    def test_pow(self, context):
        test_input_1 = np.random.rand(3, 2)
        test_input_2 = np.random.rand(3, 2)
        self._test_elementwise_binary(
            context,
            "Pow",
            ops.pow_,
            [test_input_1, test_input_2],
            2,
            np.power(test_input_1, test_input_2),
        )

    def test_eq(self, context):
        test_input_1 = torch.zeros([2, 3, 4, 5, 6]).float()
        test_input_2 = torch.ones([2, 3, 4, 5, 6]).float()
        test_input_2[0][0][0][0][0] = 0
        expected_output = (test_input_1 == test_input_2).float()

        self._test_elementwise_binary(
            context, "Eq", ops.eq, [test_input_1, test_input_2], 2, expected_output
        )

    def test_ne(self, context):
        test_input_1 = torch.zeros([2, 3, 4, 5, 6]).float()
        test_input_2 = torch.ones([2, 3, 4, 5, 6]).float()
        test_input_2[0][0][0][0][0] = 0
        expected_output = (test_input_1 != test_input_2).float()

        self._test_elementwise_binary(
            context, "ne", ops.ne, [test_input_1, test_input_2], 2, expected_output
        )

    def test_le(self, context):
        test_input_1 = torch.zeros([2, 3, 4, 5, 6]).float()
        test_input_2 = torch.ones([2, 3, 4, 5, 6]).float()
        test_input_2[0][0][0][0][0] = 0
        expected_output = (test_input_1 <= test_input_2).float()

        self._test_elementwise_binary(
            context, "Le", ops.le, [test_input_1, test_input_2], 2, expected_output
        )

    def test_lt(self, context):
        test_input_1 = torch.zeros([2, 3, 4, 5, 6]).float()
        test_input_2 = torch.ones([2, 3, 4, 5, 6]).float()
        test_input_2[0][0][0][0][0] = 0
        expected_output = (test_input_1 < test_input_2).float()

        self._test_elementwise_binary(
            context, "Lt", ops.lt, [test_input_1, test_input_2], 2, expected_output
        )

    def test_ge(self, context):
        test_input_1 = torch.zeros([2, 3, 4, 5, 6]).float()
        test_input_2 = torch.ones([2, 3, 4, 5, 6]).float()
        test_input_2[0][0][0][0][0] = 0
        expected_output = (test_input_1 >= test_input_2).float()

        self._test_elementwise_binary(
            context, "Ge", ops.ge, [test_input_1, test_input_2], 2, expected_output
        )

    def test_gt(self, context):
        test_input_1 = torch.zeros([2, 3, 4, 5, 6]).float()
        test_input_2 = torch.ones([2, 3, 4, 5, 6]).float()
        test_input_2[0][0][0][0][0] = 0
        expected_output = (test_input_1 > test_input_2).float()

        self._test_elementwise_binary(
            context, "Gt", ops.gt, [test_input_1, test_input_2], 2, expected_output
        )

    @pytest.mark.parametrize(
        "size, array_type",
        itertools.product(
            [1, 5, 7],
            [
                ("ListConstruct", ops.listconstruct),
                ("TupleConstruct", ops.tupleconstruct),
            ],
        ),
    )
    def test_arrayconstruct_scalars(self, context, size, array_type):
        constant_vals = list(range(size))
        array_kind = array_type[0]
        array_op = array_type[1]
        constants, input_list, output_name = self._gen_constants(size, constant_vals)
        ac_node = InternalTorchIRNode(
            kind=array_kind, inputs=input_list, outputs=[output_name],
        )
        ssa = self._construct_test_graph(
            context, array_op, ac_node, output_name, constants=constants
        )
        expected_val = np.arange(size)
        np.testing.assert_equal(ssa.shape, (size,))
        np.testing.assert_array_equal(ssa.val, expected_val)

    @pytest.mark.parametrize(
        "shape1, shape2, array_type",
        itertools.product(
            [(1, 2), (3, 4, 5), (2,)],
            [(2, 1), (1, 4, 5), (3,)],
            [
                ("ListConstruct", ops.listconstruct),
                ("TupleConstruct", ops.tupleconstruct),
            ],
        ),
    )
    def test_arrayconstruct_nonscalar(self, context, shape1, shape2, array_type):
        tensor1 = torch.rand(shape1)
        tensor2 = torch.rand(shape2)
        array_kind = array_type[0]
        array_op = array_type[1]
        constants, input_list, output_name = self._gen_constants(2, [tensor1, tensor2])
        ac_node = InternalTorchIRNode(
            kind=array_kind, inputs=input_list, outputs=[output_name],
        )
        ssa = self._construct_test_graph(
            context, array_op, ac_node, output_name, constants=constants
        )
        expected_val = (tensor1.numpy(), tensor2.numpy())
        np.testing.assert_equal(len(ssa), 2)
        for x, y in zip(ssa, expected_val):
            np.testing.assert_allclose(x.val, y)

    @pytest.mark.parametrize(
        "input_shape, dim0, dim1",
        [
            x
            for x in itertools.product(
                [(1, 2, 3), (1, 2, 3, 4), (1, 2, 3, 4, 5)], [0, 1, -1], [0, 2, -2],
            )
        ]
        + [((1, 2), None, None)],
    )
    def test_transpose(self, context, input_shape, dim0, dim1):
        test_input = torch.rand(input_shape)

        constant_list = [test_input]
        if len(input_shape) > 2:
            constant_list += [dim0, dim1]
            kind = "transpose"
            expected_result = torch.transpose(test_input, dim0, dim1)
        else:
            kind = "t"
            expected_result = test_input.t()

        constants, input_list, output_name = self._gen_constants(
            len(constant_list), constant_list
        )
        transpose_node = InternalTorchIRNode(
            kind=kind, inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.transpose, transpose_node, output_name, constants=constants,
        )
        np.testing.assert_array_equal(expected_result.shape, ssa.shape)
        np.testing.assert_allclose(expected_result, ssa.val)

    @pytest.mark.parametrize(
        "dim1, dim2, dim3", itertools.product([1, 2, 5], [2, 5, 10], [1, 2, 5]),
    )
    def test_matmul(self, context, dim1, dim2, dim3):
        mat1 = torch.rand((dim1, dim2))
        mat2 = torch.rand((dim2, dim3))
        constant_vals = [
            mat1,
            mat2,
        ]
        constants, input_list, output_name = self._gen_constants(2, constant_vals)

        matmul_node = InternalTorchIRNode(
            kind="matmul", inputs=input_list, outputs=[output_name],
        )

        ssa = self._construct_test_graph(
            context, ops.matmul, matmul_node, output_name, constants=constants
        )
        expected_result = torch.matmul(mat1, mat2).detach().numpy()
        assert np.allclose(expected_result, ssa.val)

    @pytest.mark.parametrize(
        "input_shape, axis, expected_shape",
        [
            ((1, 2), None, (2,)),
            ((1, 2), 0, (2,)),
            ((1, 2, 1), None, (2,)),
            ((1, 2, 1, 1), None, (2,)),
            ((1, 2, 1, 1), 2, (1, 2, 1)),
            ((1, 2, 1, 1, 1), None, (2,)),
        ],
    )
    def test_squeeze(self, context, input_shape, axis, expected_shape):
        test_data = torch.rand(input_shape)
        if axis is None:
            constants, input_list, output_name = self._gen_constants(1, test_data)
        else:
            constants, input_list, output_name = self._gen_constants(
                2, [test_data, axis]
            )
        squeeze_node = InternalTorchIRNode(
            kind="Squeeze", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.squeeze, squeeze_node, output_name, constants=constants
        )
        if axis is None:
            expected_result = torch.squeeze(test_data)
        else:
            expected_result = torch.squeeze(test_data, axis)
        assert np.allclose(expected_result, ssa.val)
        assert expected_result.size() == torch.Size(expected_shape)

    @pytest.mark.parametrize(
        "input_shape, axis, expected_shape",
        [
            ((2,), 0, (1, 2)),
            ((2,), 1, (2, 1)),
            ((2,), -1, (2, 1)),
            ((2, 3), 1, (2, 1, 3)),
        ],
    )
    def test_unsqueeze(self, context, input_shape, axis, expected_shape):
        test_data = torch.rand(input_shape)
        constants, input_list, output_name = self._gen_constants(2, [test_data, axis])
        unsqueeze_node = InternalTorchIRNode(
            kind="Unsqueeze", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.unsqueeze, unsqueeze_node, output_name, constants=constants
        )
        expected_result = torch.unsqueeze(test_data, axis)
        assert np.allclose(expected_result, ssa.val)
        assert expected_result.size() == torch.Size(expected_shape)

    @pytest.mark.parametrize(
        "input_shape, start, end",
        [
            ((2, 1, 1, 2), 1, 3),
            ((2, 2, 1, 1), 1, -2),
            ((1, 1, 1), 0, 2),
            ((1, 2), 0, 1),
            ((1, 2), 1, 1),
            ((1, 1), 1, -1),
            ((1,), 0, 0),
        ],
    )
    def test_flatten(self, context, input_shape, start, end):
        test_data = torch.rand(input_shape)
        constants, input_list, output_name = self._gen_constants(
            3, [test_data, start, end]
        )
        flatten_node = InternalTorchIRNode(
            kind="Flatten", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.flatten, flatten_node, output_name, constants=constants
        )
        expected_result = torch.flatten(test_data, start, end)
        assert np.allclose(expected_result, ssa.val)

    @pytest.mark.parametrize(
        "start, end", [(0, -5), (100, 2), (2, 100), (-3, -4),],
    )
    def test_flatten_exception(self, context, start, end):
        test_data = torch.rand(1, 1, 1, 1)
        constants, input_list, output_name = self._gen_constants(
            3, [test_data, start, end]
        )
        flatten_node = InternalTorchIRNode(
            kind="Flatten", inputs=input_list, outputs=[output_name]
        )
        with pytest.raises(ValueError):
            self._construct_test_graph(
                context, ops.flatten, flatten_node, output_name, constants=constants,
            )

    @pytest.mark.parametrize(
        "input_shape", [(2, 3), (2, 3, 4), (2, 3, 4, 5), (2, 3, 4, 5, 6),],
    )
    def test_permute(self, context, input_shape):
        test_data = torch.rand(*input_shape)
        permutation = list(range(len(input_shape)))
        np.random.shuffle(permutation)
        constants, input_list, output_name = self._gen_constants(
            2, [test_data, permutation]
        )
        permute_node = InternalTorchIRNode(
            kind="Permute", inputs=input_list, outputs=[output_name],
        )
        ssa = self._construct_test_graph(
            context, ops.permute, permute_node, output_name, constants=constants
        )
        expected_result = test_data.permute(*permutation)
        assert expected_result.shape == ssa.shape

    @pytest.mark.parametrize(
        "in_features, out_features, scaling",
        itertools.product([10, 25, 100], [3, 6], [1.0, 0.5]),
    )
    def test_addmm(self, context, in_features, out_features, scaling):
        input_data = torch.rand((1, in_features))
        weight_data = torch.rand((in_features, out_features))
        bias_data = torch.rand((out_features))
        constant_vals = [
            scaling,
            input_data,
            weight_data,
            bias_data,
        ]
        constants, _, output_name = self._gen_constants(4, constant_vals)

        addmm_node = InternalTorchIRNode(
            kind="addmm", inputs=["3", "1", "2", "0", "0"], outputs=[output_name],
        )

        ssa = self._construct_test_graph(
            context, ops.addmm, addmm_node, output_name, constants=constants
        )
        torch_linear = nn.Linear(in_features=in_features, out_features=out_features,)
        expected_shape = tuple(torch_linear(input_data).shape)
        assert expected_shape == ssa.shape

    @pytest.mark.parametrize(
        "height, width, kernel_size, stride, padding, dilation",
        itertools.product([5, 6], [5, 7], [1, 3], [1, 3], [1, 3], [1, 3]),
    )
    def test_convolution2d(
        self,
        context,
        height,
        width,
        kernel_size,
        stride,
        padding,
        dilation,
        groups=1,
        in_channels=1,
        out_channels=2,
    ):
        test_input = torch.rand(1, in_channels, height, width)
        constant_vals = [
            1,  # None argument
            test_input,
            np.random.rand(
                out_channels, in_channels, kernel_size, kernel_size
            ),  # weights
            np.random.rand(out_channels),  # bias
            np.array([stride, stride]),
            np.array([padding, padding]),
            np.array([dilation, dilation]),
            False,  # transposed
            np.array([0, 0]),  # output_pad
            groups,
        ]
        constants, _, output_name = self._gen_constants(
            len(constant_vals), constant_vals
        )
        # For reference, the values for `kind` and `inputs` indices are determined from the definition for Torch's
        # `at::_convolution` used for all convolutions. The link below is approximately correct at the time of writing.
        # https://github.com/pytorch/pytorch/blob/bd604mb5b7ae4f6388aca461891d620b0d485fbb/aten/src/ATen/native/Convolution.cpp#L544
        conv_node = InternalTorchIRNode(
            kind="_convolution",
            inputs=["1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "0", "0"],
            outputs=[output_name],
        )

        ssa = self._construct_test_graph(
            context, ops._convolution, conv_node, output_name, constants=constants
        )
        torch_conv = nn.Conv2d(
            in_channels=in_channels,
            out_channels=out_channels,
            kernel_size=kernel_size,
            stride=stride,
            padding=padding,
            dilation=dilation,
        )
        expected_shape = tuple(torch_conv(test_input).shape)
        assert ssa.val == None
        assert expected_shape == ssa.shape

    @pytest.mark.parametrize(
        "depth, height, width, kernel_size, stride, padding, dilation, groups",
        itertools.product(
            [5, 5],
            [5, 6],
            [5, 7],
            [1, 3],
            [(1, 1, 1), (3, 2, 1)],
            [(1, 1, 1), (1, 3, 2)],
            [(1, 1, 1), (1, 2, 3)],
            [
                1,
                -1,
            ],  # -1 groups indicates it should be set to the number of input channels for depthwise convolution
        ),
    )
    def test_convolution3d(
        self,
        context,
        depth,
        height,
        width,
        kernel_size,
        stride,
        padding,
        dilation,
        groups,
        in_channels=2,
        out_channels=4,
    ):
        if groups == -1:
            groups = in_channels
        test_input = torch.rand(1, in_channels, depth, height, width)
        constant_vals = [
            1,  # None argument
            test_input,
            np.random.rand(
                out_channels,
                in_channels // groups,
                kernel_size,
                kernel_size,
                kernel_size,
            ),  # weights
            np.random.rand(out_channels),  # bias
            # PyTorch's Conv3d accepts either an int (for all dimensions) or a 3-tuple of ints (one per dimension)
            np.array([stride[0], stride[1], stride[2]]),
            np.array([padding[0], padding[1], padding[2]]),
            np.array([dilation[0], dilation[1], dilation[2]]),
            False,  # transposed
            np.array([0, 0, 0]),  # out_pad
            groups,
        ]
        constants, _, output_name = self._gen_constants(
            len(constant_vals), constant_vals
        )
        # For reference, the values for `kind` and `inputs` indices are determined from the definition for Torch's
        # `at::_convolution` used for all convolutions. The link below is approximately correct at the time of writing.
        # https://github.com/pytorch/pytorch/blob/bd604mb5b7ae4f6388aca461891d620b0d485fbb/aten/src/ATen/native/Convolution.cpp#L544
        conv_node = InternalTorchIRNode(
            kind="_convolution",
            inputs=["1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "0", "0"],
            outputs=[output_name],
        )

        ssa = self._construct_test_graph(
            context, ops._convolution, conv_node, output_name, constants=constants
        )
        torch_conv = nn.Conv3d(
            in_channels=in_channels,
            out_channels=out_channels,
            kernel_size=kernel_size,
            stride=stride,
            padding=padding,
            dilation=dilation,
            groups=groups,
        )
        expected_result = torch_conv(test_input)
        expected_shape = tuple(expected_result.shape)
        assert ssa.val is None
        assert expected_shape == ssa.shape

    @pytest.mark.parametrize(
        "height, width, kernel_size, stride, padding, dilation",
        itertools.product([5, 6], [5, 7], [1, 3], [2, 3], [0, 1], [1, 3]),
    )
    def test_convolution_transpose2d(
        self,
        context,
        height,
        width,
        kernel_size,
        stride,
        padding,
        dilation,
        groups=1,
        in_channels=1,
        out_channels=2,
    ):
        test_input = torch.rand(1, in_channels, height, width)

        constant_vals = [
            np.random.rand(
                in_channels, out_channels, kernel_size, kernel_size
            ),  # weights
            np.random.rand(out_channels),  # bias
            np.array([stride, stride]),
            np.array([padding, padding]),
            np.array([dilation, dilation]),
            True,  # transposed,
            np.array([0, 0]),  # output_pad
            groups,
            False,
            False,
            False,
        ]
        graph_inputs = {"input": mb.placeholder(test_input.shape, dtype=types.float)}

        constants, input_list, output_name = self._gen_constants(
            len(constant_vals), constant_vals
        )
        conv_node = InternalTorchIRNode(
            kind="_convolution", inputs=["input"] + input_list, outputs=[output_name],
        )

        ssa = self._construct_test_graph(
            context,
            ops._convolution,
            conv_node,
            output_name,
            constants=constants,
            graph_inputs=graph_inputs,
        )
        torch_conv = nn.ConvTranspose2d(
            in_channels=in_channels,
            out_channels=out_channels,
            kernel_size=kernel_size,
            stride=stride,
            padding=padding,
            dilation=dilation,
        )
        expected_shape = tuple(torch_conv(test_input).shape)
        assert ssa.val == None
        assert expected_shape == ssa.shape

    @pytest.mark.parametrize(
        "input_shape, dim, keepdim",
        itertools.product([(3, 20, 20), (1, 50, 50)], [0, 1, 2, [0, 2]], [True, False]),
    )
    def test_mean(self, context, input_shape, dim, keepdim):
        test_input = torch.rand(*input_shape)

        constants, input_list, output_name = self._gen_constants(
            4, [test_input, dim, keepdim, None]
        )
        mean_node = InternalTorchIRNode(
            kind="mean", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.mean, mean_node, output_name, constants=constants
        )
        expected_result = torch.mean(test_input, dim, keepdim)
        assert np.allclose(expected_result, ssa.val)

    def test_mean_no_dims(self, context):
        test_input = torch.rand((3, 20, 20))

        constants, input_list, output_name = self._gen_constants(2, [test_input, None])
        mean_node = InternalTorchIRNode(
            kind="mean", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.mean, mean_node, output_name, constants=constants
        )
        expected_result = torch.mean(test_input)
        assert np.allclose(expected_result, ssa.val)

    def test_embedding(self, context):
        EMBEDDING_DIMENSION = 10
        NUM_EMBEDDINGS = 20
        input_shape = (NUM_EMBEDDINGS, EMBEDDING_DIMENSION)
        # size is arbitrary for indices
        indices = np.random.randint(NUM_EMBEDDINGS, size=100)
        test_input = torch.rand(input_shape)
        constants, input_list, output_name = self._gen_constants(
            2, [test_input, indices]
        )
        gather_node = InternalTorchIRNode(
            kind="embedding", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.embedding, gather_node, output_name, constants=constants
        )
        torch_embedding = nn.Embedding.from_pretrained(test_input)
        expected_result = torch_embedding(torch.LongTensor(indices))
        assert np.allclose(expected_result, ssa.val)

    @pytest.mark.parametrize(
        "dim", [0, 1, 2, 3, 4],
    )
    def test_size(self, context, dim):
        test_input = torch.rand(1, 2, 3, 4, 5)

        graph_inputs = {"input": mb.placeholder(test_input.shape, dtype=types.float)}
        constants, input_list, output_name = self._gen_constants(1, [dim])
        size_node = InternalTorchIRNode(
            kind="size", inputs=["input"] + input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context,
            ops.size,
            size_node,
            output_name,
            constants=constants,
            graph_inputs=graph_inputs,
        )
        expected_result = test_input.shape[dim]
        assert expected_result == ssa.val

    @pytest.mark.parametrize(
        "dim", [0, 1],
    )
    def test_size_symbolic(self, context, dim):
        test_shape = (3, get_new_symbol())
        graph_inputs = {"input": mb.placeholder(shape=test_shape, dtype=types.float)}
        constants, input_list, output_name = self._gen_constants(1, [dim])
        size_node = InternalTorchIRNode(
            kind="size", inputs=["input"] + input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context,
            ops.size,
            size_node,
            output_name,
            constants=constants,
            graph_inputs=graph_inputs,
        )
        expected_result = test_shape[dim]
        assert expected_result == ssa.sym_val

    @pytest.mark.parametrize(
        "input_size, shape",
        itertools.product([(5, 12), (1, 4, 15), (3, 5, 4)], [(3, 20), (-1, 6), (60,)],),
    )
    def test_view(self, context, input_size, shape):
        test_input = torch.rand(input_size)

        constants, input_list, output_name = self._gen_constants(2, [test_input, shape])
        view_node = InternalTorchIRNode(
            kind="view", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.view, view_node, output_name, constants=constants
        )
        expected_result = test_input.view(shape)
        assert np.allclose(expected_result, ssa.val)

    @pytest.mark.parametrize(
        "input_shape, output_shape",
        itertools.product(
            [(1, 3, 15, 15), (1, 1, 2, 2), (1, 3, 10, 10)], [(1, 1), (2, 2), (2, 1)],
        ),
    )
    def test_adaptive_avg_pool2d(self, context, input_shape, output_shape):
        test_input = torch.rand(input_shape)

        constants, input_list, output_name = self._gen_constants(
            2, [test_input, output_shape]
        )

        adaptive_avg_pool2d_node = InternalTorchIRNode(
            kind="adaptive_avg_pool2d", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context,
            ops.adaptive_avg_pool2d,
            adaptive_avg_pool2d_node,
            output_name,
            constants=constants,
        )
        expected_result = torch._adaptive_avg_pool2d(test_input, output_shape)
        expected_shape = tuple(expected_result.shape)
        assert expected_shape == ssa.shape
        # We only expect numerical output when reducing to global average.
        if output_shape == (1, 1):
            assert np.allclose(expected_result, ssa.val)

    def test_adaptive_avg_pool2d_exception(self, context):
        # For this test, the input tensor HW channels are dynamic.
        input_shape = [1, 3, get_new_symbol(), get_new_symbol()]
        graph_inputs = {"input": mb.placeholder(input_shape, dtype=types.float)}
        constants, input_list, output_name = self._gen_constants(1, [(2, 1)])
        adaptive_avg_pool2d_node = InternalTorchIRNode(
            kind="adaptive_avg_pool2d",
            inputs=["input"] + input_list,
            outputs=[output_name],
        )
        with pytest.raises(ValueError):
            ssa = self._construct_test_graph(
                context,
                ops.adaptive_avg_pool2d,
                adaptive_avg_pool2d_node,
                output_name,
                constants=constants,
                graph_inputs=graph_inputs,
            )

    @pytest.mark.parametrize("input_shape", [(1, 3, 15, 15), (1, 1, 1, 1)])
    def test_batch_norm(self, context, input_shape):
        test_input = torch.rand(input_shape)
        channels = input_shape[1]
        constants, input_list, output_name = self._gen_constants(
            9,
            [
                torch.rand(input_shape),  # input
                torch.rand(channels),  # weight
                torch.rand(channels),  # bias
                torch.rand(channels),  # running mean
                torch.rand(channels),  # running var
                0,  # training
                0.1,  # momentum
                1e-6,  # eps
                1,  # cudnn_enabled
            ],
        )

        batch_norm_node = InternalTorchIRNode(
            kind="batch_norm", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.batch_norm, batch_norm_node, output_name, constants=constants
        )
        assert ssa.val == None
        assert ssa.shape == tuple(test_input.shape)

    @pytest.mark.parametrize("input_shape", [(1, 3, 15, 15), (1, 1, 1, 1)])
    def test_instance_norm(self, context, input_shape):
        test_input = torch.rand(input_shape)
        channels = input_shape[1]
        constants, input_list, output_name = self._gen_constants(
            9,
            [
                torch.rand(input_shape),  # input
                torch.rand(channels),  # weight
                torch.rand(channels),  # bias
                torch.rand(channels),  # running mean
                torch.rand(channels),  # running var
                0,  # training
                0.1,  # momentum
                1e-6,  # eps
                1,  # cudnn_enabled
            ],
        )

        instant_norm_node = InternalTorchIRNode(
            kind="instance_norm", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.instance_norm, instant_norm_node, output_name, constants=constants
        )
        assert ssa.val == None
        assert ssa.shape == tuple(test_input.shape)

    @pytest.mark.parametrize(
        "min_val, max_val", [(-1.0, 1.0), (0.0, 0.1), (1.0, 3.0), (-1.0, 6.0),]
    )
    def test_hardtanh(self, context, min_val, max_val):
        self._test_activation(
            context,
            (3, 4, 5),
            [min_val, max_val],
            "hardtanh_",
            ops.hardtanh_,
            nn.Hardtanh(min_val, max_val).eval(),
            atol=1e-6,
        )

    @pytest.mark.parametrize("axis", [1, 2, 3])
    def test_cat(self, context, axis):
        input_shape = (1, 3, 240, 320)

        test_input1 = torch.rand(input_shape)
        test_input2 = torch.rand(input_shape)
        const_input = torch.rand(input_shape)

        graph_inputs = {
            "input1": mb.placeholder(input_shape, dtype=types.float),
            "input2": mb.placeholder(input_shape, dtype=types.float),
        }
        dim_node = InternalTorchIRNode(
            attr={"value": axis}, kind="constant", inputs=[], outputs=["0"],
        )
        const_tensor_node = InternalTorchIRNode(
            attr={"value": const_input.numpy()},
            kind="constant",
            inputs=[],
            outputs=["1"],
        )
        listconstruct_node = InternalTorchIRNode(
            kind="listconstruct", inputs=["1", "input1", "input2"], outputs=["2"]
        )
        cat_node = InternalTorchIRNode(
            kind="cat", inputs=["2", "0"], outputs=["output"]
        )

        with Function(inputs=graph_inputs) as ssa_func:
            context.add(ssa_func.inputs["input1"])
            context.add(ssa_func.inputs["input2"])
            ops.constant(context, dim_node)
            ops.constant(context, const_tensor_node)
            ops.listconstruct(context, listconstruct_node)
            ops.cat(context, cat_node)

        ssa = context["output"]
        expected_result = torch.cat(
            (const_input, test_input1, test_input2), dim=axis
        ).numpy()
        assert np.allclose(expected_result.shape, ssa.shape)

    @pytest.mark.parametrize("axis", [0, 1, 2, 3, 4])
    def test_stack(self, context, axis):
        input_shape = (1, 3, 240, 320)

        test_input1 = torch.rand(input_shape)
        test_input2 = torch.rand(input_shape)
        const_input = torch.rand(input_shape)

        graph_inputs = {
            "input1": mb.placeholder(input_shape, dtype=types.float),
            "input2": mb.placeholder(input_shape, dtype=types.float),
        }
        dim_node = InternalTorchIRNode(
            attr={"value": axis}, kind="constant", inputs=[], outputs=["0"],
        )
        const_tensor_node = InternalTorchIRNode(
            attr={"value": const_input.numpy()},
            kind="constant",
            inputs=[],
            outputs=["1"],
        )
        listconstruct_node = InternalTorchIRNode(
            kind="listconstruct", inputs=["1", "input1", "input2"], outputs=["2"]
        )
        stack_node = InternalTorchIRNode(
            kind="stack", inputs=["2", "0"], outputs=["output"]
        )

        with Function(inputs=graph_inputs) as ssa_func:
            context.add(ssa_func.inputs["input1"])
            context.add(ssa_func.inputs["input2"])
            ops.constant(context, dim_node)
            ops.constant(context, const_tensor_node)
            ops.listconstruct(context, listconstruct_node)
            ops.stack(context, stack_node)

        ssa = context["output"]
        expected_result = np.stack((const_input, test_input1, test_input2), axis=axis)
        assert np.allclose(expected_result.shape, ssa.shape)

    def test_item(self, context):
        const_val = 0
        constants, input_list, output_name = self._gen_constants(1, [const_val])
        item_node = InternalTorchIRNode(
            kind="item", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.item, item_node, output_name, constants=constants
        )
        assert ssa.val == const_val

    def test_item_exception(self, context):
        const_val = [0, 1]
        constants, input_list, output_name = self._gen_constants(1, [const_val])
        item_node = InternalTorchIRNode(
            kind="item", inputs=input_list, outputs=[output_name]
        )
        with pytest.raises(ValueError):
            ssa = self._construct_test_graph(
                context, ops.item, item_node, output_name, constants=constants,
            )

    @pytest.mark.parametrize("test_val", [1, 1.5, False])
    def test_bool(self, context, test_val):
        self._test_cast(context, test_val, "bool", ops._bool, bool)

    @pytest.mark.parametrize("test_val", [1, 1.5, -0.3])
    def test_int(self, context, test_val):
        self._test_cast(context, test_val, "int", ops._int, int)

    @pytest.mark.parametrize("input_shape", [(1, 3, 15, 15), (1, 1, 1, 1)])
    def test_layer_norm(self, context, input_shape):
        graph_inputs = {"input": mb.placeholder(input_shape, dtype=types.float)}
        channels = input_shape[1]
        constants, input_list, output_name = self._gen_constants(
            5,
            [
                input_shape,  # normalized shape
                torch.rand(channels),  # weight
                torch.rand(channels),  # running bias
                1e-6,
                1,  # cudnn enabled
            ],
        )

        layer_norm_node = InternalTorchIRNode(
            kind="layer_norm", inputs=["input"] + input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context,
            ops.layer_norm,
            layer_norm_node,
            output_name,
            graph_inputs=graph_inputs,
            constants=constants,
        )
        assert ssa.val == None
        assert ssa.shape == input_shape

    @pytest.mark.parametrize("shape", [(1, 2), (2, 3, 4, 5), (3, 4, 5),])
    def test_ones(self, context, shape):
        constants, constant_input_list, output_name = self._gen_constants(
            6, [shape, 1, 1, 1, 1, 1]
        )
        ones_node = InternalTorchIRNode(
            kind="ones", inputs=constant_input_list, outputs=[output_name],
        )
        ssa = self._construct_test_graph(
            context, ops.ones, ones_node, output_name, constants=constants,
        )
        assert ssa.shape == shape

    @pytest.mark.parametrize("input_shape", [(1, 2), (2, 3, 4, 5), (3, 4, 5),])
    def test_ones_like(self, context, input_shape):
        graph_inputs = {"input": mb.placeholder(input_shape, dtype=types.float)}
        constants, constant_input_list, output_name = self._gen_constants(5, 1)
        ones_node = InternalTorchIRNode(
            kind="ones_like",
            inputs=["input"] + constant_input_list,
            outputs=[output_name],
        )
        ssa = self._construct_test_graph(
            context,
            ops.ones_like,
            ones_node,
            output_name,
            graph_inputs=graph_inputs,
            constants=constants,
        )
        assert ssa.shape == input_shape

    @pytest.mark.parametrize(
        "input_size, dim, index",
        itertools.product(
            [(13, 43, 10), (39, 14, 11, 9)], [0, 1, 2], [0, 1, 3, 8, -1],
        ),
    )
    def test_select(self, context, input_size, dim, index):
        graph_inputs = {"input1": mb.placeholder(input_size, dtype=types.float)}
        constants, constant_input_list, output_name = self._gen_constants(
            2, [dim, index]
        )
        select_node = InternalTorchIRNode(
            kind="select",
            inputs=["input1"] + constant_input_list,
            outputs=[output_name],
        )
        ssa = self._construct_test_graph(
            context,
            ops.select,
            select_node,
            output_name,
            graph_inputs=graph_inputs,
            constants=constants,
        )
        select_index = index
        if index < 0:
            select_index += input_size[dim]
        expected_shape = tuple(
            torch.rand(input_size)
            .index_select(dim, torch.tensor([select_index]))
            .squeeze(dim)
            .shape
        )
        assert np.allclose(ssa.shape, expected_shape)

    @pytest.mark.parametrize(
        "dynamic, test_tuple", itertools.product([True, False], [True, False])
    )
    def test_tuple_and_list_unpack(self, context, dynamic, test_tuple):
        """
            if @dynamic is True then packs up a dynamic input
            if @test_tuple is True tests tupleUnpack else tests listUnpack
        """
        if test_tuple:
            construct_op = ops.tupleconstruct
            construct_name = "TupleConstruct"
            unpack_name = "TupleUnpack"
        else:
            construct_op = ops.listconstruct
            construct_name = "ListConstruct"
            unpack_name = "ListUnpack"

        input_shape = (1, 2, 3)
        constant_vals = [str(i) for i in range(1, 6)]
        constants_unpacked = [str(i) for i in range(6, 11)]
        constants, input_list, _ = self._gen_constants(5, constant_vals)
        output_list = constants_unpacked[:]
        graph_inputs = {}
        if dynamic:
            graph_input_name = "input1"
            graph_inputs = {
                graph_input_name: mb.placeholder(input_shape, dtype=types.float)
            }
            input_list += [graph_input_name]
            output_list += [graph_input_name + "_out"]

        construct_node = InternalTorchIRNode(
            kind=construct_name, inputs=input_list, outputs=["construct"],
        )
        unpack_node = InternalTorchIRNode(
            kind=unpack_name, inputs=["construct"], outputs=output_list
        )
        with Function(inputs=graph_inputs) as ssa_func:
            if dynamic:
                context.add(ssa_func.inputs["input1"])
            for node in constants:
                ops.constant(context, node)
            construct_op(context, construct_node)
            ops.tupleunpack(context, unpack_node)

        ssa_constants = []
        for name in constants_unpacked:
            ssa_constants.append(context[name].val)
        assert ssa_constants == constant_vals

        if dynamic:
            ssa_dyanmic = context[graph_input_name + "_out"]
            assert ssa_dyanmic.val is None
            assert ssa_dyanmic.shape == input_shape

    def _test_pool(
        self, context, test_input, param_list, op_kind, op_func, expected_result
    ):
        constants, input_list, output_name = self._gen_constants(
            len(param_list) + 1, [test_input] + param_list,
        )

        pool_node = InternalTorchIRNode(
            kind=op_kind, inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, op_func, pool_node, output_name, constants=constants,
        )
        expected_shape = tuple(expected_result.shape)
        assert expected_shape == ssa.shape

    @pytest.mark.parametrize(
        "input_shape, kernel_size, stride, pad, include_pad, ceil_mode",
        itertools.product(
            [(1, 3, 15), (1, 1, 7), (1, 3, 10)],
            [1, 3],
            [1, 2],
            [0, 1],
            [True, False],
            [False, True],
        ),
    )
    def test_avg_pool1d(
        self, context, input_shape, kernel_size, stride, pad, include_pad, ceil_mode,
    ):
        if pad > kernel_size / 2:
            return
        test_input = torch.rand(input_shape)
        expected_result = F.avg_pool1d(
            test_input,
            kernel_size=kernel_size,
            stride=stride,
            padding=pad,
            ceil_mode=ceil_mode,
            count_include_pad=include_pad,
        )
        self._test_pool(
            context,
            test_input,
            [[kernel_size], [stride], [pad], ceil_mode, not include_pad],
            "avg_pool1d",
            ops.avg_pool1d,
            expected_result,
        )

    @pytest.mark.parametrize(
        "input_shape, kernel_size, stride, pad, include_pad, ceil_mode",
        itertools.product(
            [(1, 3, 15, 15), (1, 1, 7, 7), (1, 3, 10, 10)],
            [1, 3],
            [1, 2],
            [0, 1],
            [True, False],
            [False, True],
        ),
    )
    def test_avg_pool2d(
        self, context, input_shape, kernel_size, stride, pad, include_pad, ceil_mode,
    ):
        if pad > kernel_size / 2:
            return
        test_input = torch.rand(input_shape)
        expected_result = F.avg_pool2d(
            test_input,
            kernel_size=kernel_size,
            stride=stride,
            padding=pad,
            ceil_mode=ceil_mode,
            count_include_pad=include_pad,
        )
        self._test_pool(
            context,
            test_input,
            [
                [kernel_size, kernel_size],
                [stride, stride],
                [pad, pad],
                ceil_mode,
                not include_pad,
                None,
            ],
            "avg_pool2d",
            ops.avg_pool2d,
            expected_result,
        )

    @pytest.mark.parametrize(
        "input_shape, kernel_size, stride, pad, ceil_mode",
        itertools.product(
            [(1, 3, 15), (1, 1, 7), (1, 3, 10)], [1, 3], [1, 2], [0, 1], [False, True]
        ),
    )
    @pytest.mark.xfail(reason="torch converter for max_pool1d not implemented")
    def test_max_pool1d(
        self, context, input_shape, kernel_size, stride, pad, ceil_mode
    ):
        if pad > kernel_size / 2:
            # Because this test is xfail, we have to fail rather than
            # just return here, otherwise these test cases unexpectedly pass.
            # This can be changed to `return` once the above radar
            # is fixed and the test is no longer xfail.
            raise ValueError("pad must be less than half the kernel size")
        test_input = torch.rand(input_shape)
        expected_result = F.max_pool1d(
            test_input,
            kernel_size=kernel_size,
            stride=stride,
            padding=pad,
            ceil_mode=ceil_mode,
        )
        self._test_pool(
            context,
            test_input,
            [[kernel_size], [stride], [pad], [1], ceil_mode],
            "max_pool1d",
            ops.max_pool1d,
            expected_result,
        )

    @pytest.mark.parametrize(
        "input_shape, kernel_size, stride, pad, ceil_mode",
        itertools.product(
            [(1, 3, 15, 15), (1, 1, 7, 7), (1, 3, 10, 10)],
            [1, 3],
            [1, 2],
            [0, 1],
            [False, True],
        ),
    )
    def test_max_pool2d(
        self, context, input_shape, kernel_size, stride, pad, ceil_mode,
    ):
        if pad > kernel_size / 2:
            return
        test_input = torch.rand(input_shape)
        expected_result = F.max_pool2d(
            test_input,
            kernel_size=kernel_size,
            stride=stride,
            padding=pad,
            ceil_mode=ceil_mode,
        )
        self._test_pool(
            context,
            test_input,
            [
                [kernel_size, kernel_size],
                [stride, stride],
                [pad, pad],
                [1, 1,],  # dilation
                ceil_mode,
            ],
            "max_pool2d",
            ops.max_pool2d,
            expected_result,
        )

    @pytest.mark.parametrize("dim", [0, 1, 2])
    def test_softmax(self, context, dim):
        self._test_activation(
            context,
            (3, 4, 5),
            [dim, None],
            "softmax",
            ops.softmax,
            nn.Softmax(dim=dim).eval(),
            atol=1e-6,
        )

    def test_relu(self, context):
        self._test_activation(
            context, (3, 4, 5), [], "relu", ops.relu, nn.ReLU().eval(), atol=1e-6
        )

    @pytest.mark.parametrize("alpha", [0.1, 2.0, 1.5])
    def test_leaky_relu(self, context, alpha):
        self._test_activation(
            context, (3, 4, 5), [alpha], "leaky_relu", ops.leaky_relu, nn.LeakyReLU(negative_slope=alpha).eval(), atol=1e-6
        )

    @pytest.mark.parametrize("dim", [0, 1, 2])
    def test_log_softmax(self, context, dim):
        self._test_activation(
            context,
            (3, 4, 5),
            [dim, None],
            "log_softmax",
            ops.log_softmax,
            nn.LogSoftmax(dim=dim).eval(),
            atol=1e-6,
        )

    def test_sigmoid(self, context):
        self._test_activation(
            context,
            (3, 4, 5),
            [],
            "sigmoid",
            ops.sigmoid,
            nn.Sigmoid().eval(),
            atol=1e-6,
        )

    def test_gelu(self, context):
        self._test_activation(
            context, (3, 4, 5), [], "gelu", ops.gelu, nn.GELU().eval(), atol=1e-6
        )

    @pytest.mark.parametrize(
        "dim, start, end, step",
        itertools.product([0, 1, 2], [0, 1, 2], [3, 4, 5, None], [1, 2]),
    )
    def test_slice(self, context, dim, start, end, step):
        test_input = torch.rand(5, 5, 5)
        constants, input_list, output_name = self._gen_constants(
            5, [test_input, dim, start, end, step]
        )
        node = InternalTorchIRNode(
            kind="slice", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops._slice, node, output_name, constants=constants
        )
        if end is None:
            end = test_input.shape[dim]
        expected_result = test_input.index_select(
            dim, torch.LongTensor(range(start, end, step))
        )
        np.testing.assert_allclose(expected_result, ssa.val)

    @pytest.mark.parametrize(
        "split_sizes, dim, make_explicit",
        itertools.product([2, 3], [0, 1, 2], [True, False]),
    )
    def test_split(self, context, split_sizes, dim, make_explicit):
        test_input = torch.rand(3, 4, 5)
        if make_explicit:
            # Explicitly provide the size of each split. This will be two
            # splits, the given size and the remainder.
            split_sizes = [split_sizes, test_input.shape[dim] - split_sizes]
        constants, input_list, output_name = self._gen_constants(
            3, [test_input, split_sizes, dim]
        )
        node = InternalTorchIRNode(
            kind="split", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.split, node, output_name, constants=constants
        )
        expected_result = torch.split(test_input, split_sizes, dim)
        if not isinstance(ssa, list):
            ssa = [ssa]

        for ex_res, ssa_res in zip(expected_result, ssa):
            np.testing.assert_allclose(ex_res.numpy(), ssa_res.val, atol=1e-6)

    @pytest.mark.parametrize(
        "num_args, dtype", itertools.product([4, 5, 6], [0, 1, 2, 3, 4, 5, 6, 7, 11])
    )
    def test_to(self, context, num_args, dtype):
        test_input = torch.rand(1, 2, 3)
        # These args should be unused
        copy = True
        non_blocking = True
        device = 1337

        constants_list = [non_blocking, copy]
        if num_args == 4:
            constants_list = [dtype] + constants_list
        elif num_args == 5:
            constants_list = [device, dtype] + constants_list
        else:
            constants_list = [device, dtype, copy] + constants_list
        constants_list = [test_input] + constants_list
        constants, input_list, output_name = self._gen_constants(
            len(constants_list), constants_list
        )
        to_node = InternalTorchIRNode(
            kind="to", inputs=input_list, outputs=[output_name]
        )
        if num_args == 6:
            with pytest.raises(ValueError):
                ssa = self._construct_test_graph(
                    context, ops.to, to_node, output_name, constants=constants,
                )
        else:
            ssa = self._construct_test_graph(
                context, ops.to, to_node, output_name, constants=constants,
            )
            if num_args == 3:
                expected_result = test_input.numpy()
            else:
                expected_result = test_input.to(
                    dtype=ops.NUM_TO_TORCH_DTYPE[dtype]
                ).numpy()
            assert np.allclose(expected_result, ssa.val)

    def test_floor(self, context):
        test_input = torch.rand(1, 2, 3) * 10
        constants, input_list, output_name = self._gen_constants(1, test_input)
        floor_node = InternalTorchIRNode(
            kind="floor", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.floor, floor_node, output_name, constants=constants,
        )
        expected_result = test_input.floor()
        assert np.allclose(expected_result, ssa.val)

    def test_erf(self, context):
        test_input = torch.rand(1, 2, 3, 4)
        constants, input_list, output_name = self._gen_constants(1, test_input)
        node = InternalTorchIRNode(kind="erf", inputs=input_list, outputs=[output_name])
        ssa = self._construct_test_graph(
            context, ops.erf, node, output_name, constants=constants
        )
        expected_result = test_input.erf()
        assert np.allclose(expected_result, ssa.val)

    def test_implicittensortonum(self, context):
        input_shape = (1,)
        graph_input_name = "input1"
        graph_inputs = {
            graph_input_name: mb.placeholder(input_shape, dtype=types.float)
        }
        output_name = "1"
        node = InternalTorchIRNode(
            kind="implicittensortonum", inputs=["input1"], outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context,
            ops.implicittensortonum,
            node,
            output_name,
            graph_inputs=graph_inputs,
        )
        assert ssa.shape == ()

    @pytest.mark.parametrize(
        "chunks, dim", itertools.product([2, 3, 5], [0, 1, 2, 3]),
    )
    def test_constantchunk(self, context, chunks, dim):
        test_input = torch.rand(5, 8, 9, 11)
        expected_result = test_input.chunk(chunks, dim=dim)
        constants, input_list, first_output = self._gen_constants(1, [test_input])
        outputs = [str(int(first_output) + i) for i in range(len(expected_result))]
        node = InternalTorchIRNode(
            attr={"chunks": chunks, "dim": dim},
            kind="constantchunk",
            inputs=input_list,
            outputs=outputs,
        )
        self._construct_test_graph(
            context, ops.constantchunk, node, first_output, constants=constants
        )
        actual_result = [context[name] for name in outputs]

        np.testing.assert_equal(len(expected_result), len(actual_result))
        for ex_res, ssa_res in zip(expected_result, actual_result):
            np.testing.assert_allclose(ex_res.numpy(), ssa_res.val, atol=1e-6)

    @pytest.mark.parametrize(
        "input_shape, shape",
        [
            ((3, 1), (3, 4)),
            ((3, 1), (-1, 4)),
            ((3, 1, 1), (3, 4, 1)),
            ((3, 1, 1), (3, -1, 5)),
            ((3, 1, 1), (3, 4, 5)),
            ((1, 3, 1, 1), (2, 3, -1, 1)),
            ((1, 3, 4, 1), (2, 3, -1, 5)),
        ],
    )
    def test_expand(self, context, input_shape, shape):
        test_input = torch.rand(input_shape)
        constants, input_list, output_name = self._gen_constants(2, [test_input, shape])
        node = InternalTorchIRNode(
            kind="expand", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.expand, node, output_name, constants=constants
        )
        expected_result = test_input.expand(shape)
        np.testing.assert_allclose(expected_result, ssa.val)

    @pytest.mark.parametrize(
        "input_shape, other_shape",
        [
            ((3, 1), (3, 4)),
            ((3, 1, 1), (3, 4, 1)),
            ((3, 1, 1), (3, 4, 5)),
            ((1, 3, 1, 1), (2, 3, 4, 1)),
            ((1, 3, 4, 1), (2, 3, 4, 5)),
            ((1, 3, 4, 1), (1, 3, 4, 5)),
        ],
    )
    def test_expand_as(self, context, input_shape, other_shape):
        test_input = torch.rand(input_shape)
        other = torch.rand(other_shape)
        constants, input_list, output_name = self._gen_constants(2, [test_input, other])
        node = InternalTorchIRNode(
            kind="expand_as", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.expand_as, node, output_name, constants=constants
        )
        expected_result = test_input.expand_as(other)
        np.testing.assert_allclose(expected_result, ssa.val)

    @pytest.mark.parametrize(
        "start, end, step",
        [x for x in itertools.product((None, 0, 2), (5, 10), (None,),)]
        + [x for x in itertools.product((0, 2), (5, 10), (1, 2))],
    )
    def test_arange(self, context, start, end, step):
        # Arange can get [end], [start, end], or [start, end, step]
        args = [x for x in [start, end, step] if x is not None]
        args += [0, 0, 0, False]  # Extra args needed but ignored by arange
        constants, input_list, output_name = self._gen_constants(len(args), args)
        node = InternalTorchIRNode(
            kind="arange", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.arange, node, output_name, constants=constants
        )
        kwargs = {"end": end}
        if start is not None:
            kwargs["start"] = start
        if step is not None:
            kwargs["step"] = step
        expected_result = torch.arange(**kwargs)
        np.testing.assert_allclose(expected_result, ssa.val)

    @pytest.mark.parametrize(
        "input_shape, axis",
        [((2, 3), 0), ((2, 3, 4), 1), ((2, 3, 4, 5), 0), ((2, 3, 4, 5), 2),],
    )
    def test_masked_fill(self, context, input_shape, axis):
        mask_shape = list(input_shape)
        mask_shape[axis] = 1
        mask = torch.randint(0, 1, mask_shape, dtype=torch.bool)
        input_data = torch.rand(input_shape)
        value = -1.0
        constants, input_list, output_name = self._gen_constants(
            3, [input_data, mask, value]
        )
        node = InternalTorchIRNode(
            kind="masked_fill", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.masked_fill, node, output_name, constants=constants
        )
        expected_result = input_data.masked_fill(mask, value)
        np.testing.assert_allclose(expected_result, ssa.val)

    @pytest.mark.parametrize("sizes", itertools.permutations([1, 2, 3]))
    def test_meshgrid(self, context, sizes):
        input_tensors = [torch.rand(size) for size in sizes]
        expected_results = torch.meshgrid(input_tensors)
        constants, input_list, output_name = self._gen_constants(3, input_tensors)
        node = InternalTorchIRNode(
            kind="meshgrid", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.meshgrid, node, output_name, constants=constants,
        )
        for expected_result, ssa_result in zip(expected_results, ssa):
            np.testing.assert_allclose(expected_result.numpy(), ssa_result.val)

    @pytest.mark.parametrize(
        "noop_kind",
        ["dropout", "dropout_", "feature_dropout", "contiguous", "device", "detach"],
    )
    def test_noops(self, context, noop_kind):
        test_input = torch.rand(3, 4, 5)
        constants, input_list, output_name = self._gen_constants(
            3, [test_input, "test", "test"]
        )
        node = InternalTorchIRNode(
            kind=noop_kind, inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.noop, node, output_name, constants=constants
        )
        assert np.allclose(test_input.numpy(), ssa.val)

    def test_tanh(self, context):
        test_input = torch.rand(3, 4, 5)
        constants, input_list, output_name = self._gen_constants(1, [test_input])
        node = InternalTorchIRNode(
            kind="tanh", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.tanh, node, output_name, constants=constants
        )
        expected_result = torch.tanh(test_input)
        assert np.allclose(expected_result.numpy(), ssa.val)

    # TODO: test for @keepdim==True when the backend bug is fixed.
    # rdar://62566799
    @pytest.mark.parametrize(
        "input_shape, dim, keepdim",
        itertools.product([(3, 20, 20), (1, 50, 50)], [0, 1, 2], [False]),
    )
    def test_argmax(self, context, input_shape, dim, keepdim):
        test_input = torch.rand(*input_shape)

        constants, input_list, output_name = self._gen_constants(
            4, [test_input, dim, keepdim, None]
        )
        node = InternalTorchIRNode(
            kind="argmax", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.argmax, node, output_name, constants=constants
        )
        expected_result = torch.argmax(test_input, dim, keepdim)
        np.testing.assert_allclose(expected_result, ssa.val)

    @pytest.mark.parametrize(
        "size, dtype", itertools.product([(1, 2, 3, 4), (1,)], [11, 0, 1, 6]),
    )
    def test_zeros(self, context, size, dtype):
        layout = 0  # unused
        device = 0  # unused
        pin_memory = 0  # unused
        constants, input_list, output_name = self._gen_constants(
            5, [size, dtype, layout, device, pin_memory]
        )
        node = InternalTorchIRNode(
            kind="zeros", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops.zeros, node, output_name, constants=constants
        )
        expected_result = torch.zeros(size, dtype=ops.NUM_TO_TORCH_DTYPE[dtype])
        np.testing.assert_allclose(expected_result, ssa.val)

    # TODO: Reduce rtol
    # rdar://62868763 (Numerical discrepancy between torch.exp and coreml MIL exp operation)
    @pytest.mark.parametrize("input_size", [(1, 2, 3, 4), (1,)])
    def test_exp(self, context, input_size):
        test_input = torch.rand(input_size)
        constants, input_list, output_name = self._gen_constants(1, test_input)
        node = InternalTorchIRNode(kind="exp", inputs=input_list, outputs=[output_name])
        ssa = self._construct_test_graph(
            context, ops.exp, node, output_name, constants=constants
        )
        expected_result = torch.exp(test_input)
        np.testing.assert_allclose(expected_result, ssa.val, rtol=1e-06)

    @pytest.mark.parametrize(
        "input_size, dim, keepdim",
        itertools.product([(1, 2, 3, 4)], [0, 1, 2], [True, False]),
    )
    def test_max(self, context, input_size, dim, keepdim):
        test_input = torch.rand(input_size)
        constants, input_list, _ = self._gen_constants(3, [test_input, dim, keepdim])
        node = InternalTorchIRNode(
            kind="max", inputs=input_list, outputs=["out1", "out2"],
        )
        ssa = self._construct_test_graph(context, ops.max, node, constants=constants)
        index_result = context["out1"].val
        max_result = context["out2"].val
        expected_index, expected_max = torch.max(test_input, dim=dim, keepdim=keepdim)

    @pytest.mark.parametrize(
        "input_size, dim, descending",
        itertools.product([(2, 3, 4), (1, 2, 3, 4)], [0, 1, 2], [True, False]),
    )
    def test_sort(self, context, input_size, dim, descending):
        test_input = torch.rand(input_size)
        constants, input_list, output_name = self._gen_constants(
            3, [test_input, dim, descending]
        )
        node = InternalTorchIRNode(
            kind="sort", inputs=input_list, outputs=["out1", "out2"],
        )
        ssa = self._construct_test_graph(context, ops.sort, node, constants=constants)
        expected_sort, expected_index = torch.sort(
            test_input, dim=dim, descending=descending
        )
        sort_result = context["out1"].val
        index_result = context["out2"].val
        np.testing.assert_allclose(expected_sort, sort_result)
        np.testing.assert_allclose(expected_index, index_result)

    def test_abs(self, context):
        test_input = torch.rand(3, 4, 5)
        constants, input_list, output_name = self._gen_constants(1, [test_input])
        node = InternalTorchIRNode(
            kind="abs", inputs=input_list, outputs=[output_name]
        )
        ssa = self._construct_test_graph(
            context, ops._abs, node, output_name, constants=constants
        )
        expected_result = torch.abs(test_input)
        assert np.allclose(expected_result.numpy(), ssa.val)

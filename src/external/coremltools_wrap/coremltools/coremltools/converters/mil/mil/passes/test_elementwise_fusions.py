#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil.mil import Builder as mb
from coremltools.converters.mil.testing_utils import (
    assert_op_count_match,
    assert_model_is_valid,
    get_op_types_in_program,
    apply_pass_and_basic_check,
)

import pytest
import numpy as np
import itertools

np.random.seed(1984)


class TestElementwiseOptimizationPasses:
    """
    Input graph:
                                    Const
                                      |
                                      V
    input -----> convolution -----> add/sub  ----> relu ---> out

    Output graph:
    input -----> convolution -----> relu ----> out
    """

    @pytest.mark.parametrize(
        "conv_dim, \
                             flip_add_input_order, \
                             add_batch_dim_to_const, \
                             use_sub_instead, \
                             prebuilt_bias, \
                             scalar_elementwise, \
                             use_conv_transpose",
        itertools.product(
            [
                2,
                3,
            ],  # 1D conv conversion broken even without the pass: rdar://problem/62960720
            [True, False],  # flip_add_input_order
            [True, False],  # add_batch_dim_to_const
            [True, False],  # use_sub_instead
            [True, False],  # prebuilt_bias
            [True, False],  # scalar_elementwise
            [True, False],  # use_conv_transpose
        ),
    )
    def test_fuse_bias_conv(
        self,
        conv_dim,
        flip_add_input_order,
        add_batch_dim_to_const,
        use_sub_instead,
        prebuilt_bias,
        scalar_elementwise,
        use_conv_transpose,
    ):

        if flip_add_input_order and use_sub_instead:
            return

        if use_conv_transpose and conv_dim != 2:
            return

        input_shape = None
        W = None
        Cout = 8
        Cin = 3
        D = 10
        const = (
            np.random.rand(Cout) if add_batch_dim_to_const else np.random.rand(1, Cout)
        )
        const = np.expand_dims(const, axis=-1)

        if conv_dim == 1:
            input_shape = (1, Cin, D)
            W = np.random.rand(Cout, Cin, 1)
        elif conv_dim == 2:
            input_shape = (1, Cin, D, D)
            W = np.random.rand(Cout, Cin, 1, 1)
            const = np.expand_dims(const, axis=-1)
        elif conv_dim == 3:
            input_shape = (1, Cin, D, D, D)
            W = np.random.rand(Cout, Cin, 1, 1, 1)
            const = np.expand_dims(const, axis=-1)
            const = np.expand_dims(const, axis=-1)

        output_shape = list(input_shape)
        output_shape[1] = Cout

        if scalar_elementwise:
            const = np.random.uniform(0)

        @mb.program(input_specs=[mb.TensorSpec(shape=input_shape)])
        def prog(x):
            kwargs = {
                "x": x,
                "weight": W,
                "pad_type": "valid",
                "dilations": [1] * conv_dim,
                "strides": [1] * conv_dim,
            }
            if prebuilt_bias:
                kwargs["bias"] = np.random.rand(Cout)

            x = mb.conv_transpose(**kwargs) if use_conv_transpose else mb.conv(**kwargs)

            if use_sub_instead:
                x = mb.sub(x=x, y=const)
            else:
                x = mb.add(
                    x=const if flip_add_input_order else x,
                    y=x if flip_add_input_order else const,
                )
            x = mb.relu(x=x)
            return x

        element_op = "sub" if use_sub_instead else "add"
        conv_op = "conv" if not use_conv_transpose else "conv_transpose"

        prev_prog, prev_block, block = apply_pass_and_basic_check(
            prog, "common::fuse_bias_conv"
        )
        assert get_op_types_in_program(prev_prog) == [conv_op, element_op, "relu"]
        assert get_op_types_in_program(prog) == [conv_op, "relu"]

        old_bias = prev_block.find_ops(op_type=conv_op)[0].inputs.get("bias", None)
        old_bias_val = 0 if old_bias is None else old_bias.val
        assert old_bias_val is not None
        assert block.find_ops(op_type=conv_op)[0].inputs["bias"] is not None
        new_bias_val = block.find_ops(op_type=conv_op)[0].inputs["bias"].val
        assert new_bias_val is not None
        if use_sub_instead:
            np.testing.assert_almost_equal(
                old_bias_val - np.squeeze(const), new_bias_val
            )
        else:
            np.testing.assert_almost_equal(
                old_bias_val + np.squeeze(const), new_bias_val
            )

        assert_model_is_valid(
            prog,
            {"x": input_shape},
            expected_output_shapes={block.outputs[0].name: tuple(output_shape)},
        )

    """
    Input graph:
                                 Const     Const
                                   |         |
                                   V         V
    input -----> transpose -----> mul ----> add ---> out

    Output graph:
    input -----> transpose -----> batchnorm ----> out
    """

    @pytest.mark.parametrize(
        "flip_mul_input_order, flip_add_input_order, rank_3_const_input",
        itertools.product([False, True], [False, True], [False, True]),
    )
    def test_mul_add_fusion_to_batchnorm(
        self, flip_mul_input_order, flip_add_input_order, rank_3_const_input
    ):

        C = 3
        gamma = np.random.rand(1, C, 1, 1)
        beta = np.random.rand(1, C, 1, 1)
        if rank_3_const_input:
            gamma = np.squeeze(gamma, axis=0)
            beta = np.squeeze(beta, axis=0)

        @mb.program(input_specs=[mb.TensorSpec(shape=(1, 10, 10, C))])
        def prog(x):
            x = mb.transpose(x=x, perm=[0, 3, 1, 2])
            if flip_mul_input_order:
                x = mb.mul(x=gamma, y=x)
            else:
                x = mb.mul(x=x, y=gamma)
            if flip_add_input_order:
                x = mb.add(x=beta, y=x)
            else:
                x = mb.add(x=x, y=beta)
            return x

        prev_prog, prev_block, block = apply_pass_and_basic_check(
            prog, "common::fuse_elementwise_to_batchnorm"
        )
        assert get_op_types_in_program(prev_prog) == ["transpose", "mul", "add"]
        assert get_op_types_in_program(prog) == ["transpose", "batch_norm"]
        assert_model_is_valid(
            prog,
            {"x": (1, 10, 10, C)},
            expected_output_shapes={block.outputs[0].name: (1, C, 10, 10)},
        )

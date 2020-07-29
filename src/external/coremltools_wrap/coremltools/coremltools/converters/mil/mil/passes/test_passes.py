#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil.mil import Builder as mb
from coremltools.converters.mil.testing_utils import (
    assert_op_count_match,
    assert_model_is_valid,
    assert_same_output_names,
    get_op_types_in_program,
    apply_pass_and_basic_check,
)
from coremltools.converters.mil.mil import Symbol, types
from coremltools.converters.mil.mil.passes.pass_registry import PASS_REGISTRY
import copy
import pytest

import numpy as np

np.random.seed(1984)
validate_model = True


# TODO: rdar://58993652 (Add recursive block test cases for graph pass tests)


def test_const_elimination():
    @mb.program(input_specs=[mb.TensorSpec(shape=(2, 4))])
    def prog(x):
        a = np.random.rand(2, 4).astype(np.float32)
        double_a = mb.add(x=a, y=a)
        return mb.add(x=x, y=double_a)

    assert_op_count_match(prog, expect=2, op="const")
    prev_prog = copy.deepcopy(prog)
    PASS_REGISTRY["common::const_elimination"](prog)
    assert_same_output_names(prev_prog, prog)
    assert_op_count_match(prog, expect=3, op="const")

    if validate_model:
        assert_model_is_valid(prog, {"x": (2, 4)})


def test_divide_to_multiply():
    @mb.program(input_specs=[mb.TensorSpec(shape=(2, 4))])
    def prog(x):
        div_val = np.random.rand(2, 4).astype(np.float32)
        div_const = mb.const(val=div_val, mode="immediate_value")

        div_val_1 = np.random.rand(2, 4).astype(np.float32)
        div_const_1 = mb.const(val=div_val_1, mode="immediate_value")

        real_div = mb.real_div(x=x, y=div_const)

        return mb.real_div(x=real_div, y=div_const_1)

    assert_op_count_match(prog, expect=2, op="real_div")
    assert_op_count_match(prog, expect=0, op="mul")
    prev_prog = copy.deepcopy(prog)
    PASS_REGISTRY["common::divide_to_multiply"](prog)
    assert_same_output_names(prev_prog, prog)
    assert_op_count_match(prog, expect=0, op="real_div")
    assert_op_count_match(prog, expect=2, op="mul")

    if validate_model:
        assert_model_is_valid(prog, {"x": (2, 4)})


def test_fuse_matmul_weight_bias():
    @mb.program(input_specs=[mb.TensorSpec(shape=(2, 4))])
    def prog(x):
        weights_val = np.random.rand(2, 4).T.astype(np.float32)
        weights = mb.const(val=weights_val, mode="immediate_value")
        bias_val = np.random.rand(2).astype(np.float32)
        bias = mb.const(val=bias_val, mode="immediate_value")

        matmul = mb.matmul(x=x, y=weights)
        return mb.add(x=matmul, y=bias)

    assert_op_count_match(prog, expect=1, op="matmul")
    assert_op_count_match(prog, expect=0, op="linear")
    prev_prog = copy.deepcopy(prog)
    PASS_REGISTRY["common::fuse_matmul_weight_bias"](prog)
    assert_same_output_names(prev_prog, prog)
    assert_op_count_match(prog, expect=0, op="matmul")
    assert_op_count_match(prog, expect=1, op="linear")

    if validate_model:
        assert_model_is_valid(prog, {"x": (2, 4)})


def test_dead_code_elimination():
    @mb.program(
        input_specs=[mb.TensorSpec(shape=(2, 4)), mb.TensorSpec(shape=(2, 4)),]
    )
    def program0(x, y):
        # following three unused op should be eliminated
        a = mb.const(val=np.zeros(shape=(1,)), mode="immediate_value")
        b = mb.const(val=np.zeros(shape=(1,)), mode="immediate_value")
        _ = mb.add(x=a, y=b)
        return mb.add(x=x, y=y)

    assert_op_count_match(program0, expect=4)
    prev_prog = copy.deepcopy(program0)
    PASS_REGISTRY["common::dead_code_elimination"](program0)
    assert_same_output_names(prev_prog, program0)
    assert_op_count_match(program0, expect=1)

    if validate_model:
        assert_model_is_valid(program0, {"x": (2, 4), "y": (2, 4)})

    @mb.program(input_specs=[mb.TensorSpec(shape=(2, 4))])
    def program1(x):
        weights_val = np.random.rand(2, 4).T.astype(np.float32)
        weights = mb.const(val=weights_val, mode="immediate_value")
        bias_val = np.random.rand(4).astype(np.float32)
        bias = mb.const(val=bias_val, mode="immediate_value")

        # unused op and its inputs should be eliminated
        mb.matmul(x=x, y=weights)

        return mb.linear(x=x, weight=weights, bias=bias)

    assert_op_count_match(program1, expect=6)
    prev_prog = copy.deepcopy(program1)
    PASS_REGISTRY["common::dead_code_elimination"](program1)
    assert_same_output_names(prev_prog, program1)
    assert_op_count_match(program1, expect=3)

    if validate_model:
        assert_model_is_valid(program1, {"x": (2, 4)})


def test_remove_symbolic_reshape():
    sym_b = Symbol("s0")
    original_shape = (sym_b, Symbol("s1"), 2)
    reshape_name = "reshape"

    @mb.program(input_specs=[mb.TensorSpec(shape=(sym_b, 4))])
    def prog(x):
        # const cannot represent symbolic values. Use _const_symbolic
        shape = mb._const_symbolic(val=original_shape)
        return mb.reshape(x=x, shape=shape, name=reshape_name)

    reshape_op = prog.find_ops(
        prefix=reshape_name, op_type="reshape", exactly_one=True
    )[0]
    shape_var = reshape_op.shape
    reshaped_var = reshape_op.outputs[0]
    assert np.all(shape_var.sym_val == original_shape)
    assert np.all(reshaped_var.shape == (sym_b, 2, 2))

    # Note: we cannot deepcopy prog with symbol.
    prev_outputs = [o.name for o in prog["main"].outputs]
    PASS_REGISTRY["common::remove_symbolic_reshape"](prog)
    curr_outputs = [o.name for o in prog["main"].outputs]
    assert curr_outputs == prev_outputs

    reshape_op = prog.find_ops(
        prefix=reshape_name, op_type="reshape", exactly_one=True
    )[0]
    shape_var = reshape_op.shape
    reshaped_var = reshape_op.outputs[0]
    # shape param cannot be symbolic after the pass
    assert np.all(shape_var.sym_val == (-1, 2, 2))
    # output shape is still symbolic
    assert np.all(reshaped_var.shape == (sym_b, 2, 2))

    if validate_model:
        assert_model_is_valid(prog, {"x": (3, 4)})


def test_loop_invariant_elimination1():
    """
    Invariant pattern: Block input vars are returned as block output vars.
    """

    def body(a, b):
        return mb.add(x=a, y=b), b

    def cond(a, b):
        a_mean = mb.reduce_mean(x=a, axes=[0, 1])
        b_mean = mb.reduce_mean(x=b, axes=[0, 1])
        return mb.less(x=a_mean, y=b_mean)

    @mb.program(
        input_specs=[mb.TensorSpec(shape=(1, 2)), mb.TensorSpec(shape=(1, 2)),]
    )
    def prog(a, b):
        # b is loop invariant
        return mb.while_loop(_cond=cond, _body=body, loop_vars=(a, b))

    while_op = prog.find_ops(op_type="while_loop", exactly_one=True)[0]
    assert len(while_op.blocks[0].inputs) == 2
    assert len(while_op.outputs) == 2
    assert len(while_op.loop_vars) == 2
    assert while_op.blocks[0].inputs[0].name == "a.x"
    assert while_op.blocks[0].inputs[1].name == "b.x"

    prev_prog = copy.deepcopy(prog)
    PASS_REGISTRY["common::loop_invariant_elimination"](prog)
    assert_same_output_names(prev_prog, prog)

    while_op = prog.find_ops(op_type="while_loop", exactly_one=True)[0]
    assert len(while_op.blocks[0].inputs) == 1
    assert len(while_op.outputs) == 1
    assert len(while_op.loop_vars) == 1
    assert while_op.blocks[0].inputs[0].name == "a.x"

    if validate_model:
        assert_model_is_valid(prog, {"a": (1, 2), "b": (1, 2)})


def test_loop_invariant_elimination2():
    """
    Invariant pattern: Block outputs var from outside of the block
    """

    @mb.program(
        input_specs=[mb.TensorSpec(shape=(1, 2)), mb.TensorSpec(shape=(1, 2)),]
    )
    def prog(a, b):
        def body(a, bx):
            return mb.add(x=a, y=b), b

        def cond(a, bx):
            a_mean = mb.reduce_mean(x=a, axes=[0, 1])
            b_mean = mb.reduce_mean(x=bx, axes=[0, 1])
            return mb.less(x=a_mean, y=b_mean)

        # b is loop invariant
        return mb.while_loop(_cond=cond, _body=body, loop_vars=(a, b))

    while_op = prog.find_ops(op_type="while_loop", exactly_one=True)[0]
    assert len(while_op.blocks[0].inputs) == 2
    assert len(while_op.outputs) == 2
    assert len(while_op.loop_vars) == 2
    assert while_op.blocks[0].inputs[0].name == "a.x"
    assert while_op.blocks[0].inputs[1].name == "b.x"

    prev_prog = copy.deepcopy(prog)
    PASS_REGISTRY["common::loop_invariant_elimination"](prog)
    assert_same_output_names(prev_prog, prog)

    while_op = prog.find_ops(op_type="while_loop", exactly_one=True)[0]
    assert len(while_op.blocks[0].inputs) == 1
    assert len(while_op.outputs) == 1
    assert len(while_op.loop_vars) == 1
    assert while_op.blocks[0].inputs[0].name == "a.x"

    if validate_model:
        assert_model_is_valid(prog, {"a": (1, 2), "b": (1, 2)})


def test_gelu_tanh_approximation():
    """
    Detect gelu tanh approx pattern, found in the TF bert model.
    y = ( tanh((.0447)x^3 + x ) * (sqrt(2/pi)) + 1 ) * 0.5 * x
    """

    @mb.program(input_specs=[mb.TensorSpec(shape=(3, 5, 6))])
    def prog(x):
        x1 = mb.pow(x=x, y=3)
        x1 = mb.mul(x=0.044715, y=x1)
        x1 = mb.add(x=x1, y=x)
        x1 = mb.mul(x=x1, y=np.sqrt(2 / np.pi))
        x1 = mb.tanh(x=x1)
        x1 = mb.add(x=1, y=x1)
        x1 = mb.mul(x=0.5, y=x1)
        x1 = mb.mul(x=x, y=x1)
        return x1

    prev_prog, prev_block, block = apply_pass_and_basic_check(
        prog, "common::fuse_gelu_tanh_approximation"
    )
    assert get_op_types_in_program(prev_prog) == [
        "pow",
        "mul",
        "add",
        "mul",
        "tanh",
        "add",
        "mul",
        "mul",
    ]
    assert get_op_types_in_program(prog) == ["gelu"]
    assert_model_is_valid(
        prog,
        {"x": (3, 5, 6)},
        expected_output_shapes={block.outputs[0].name: (3, 5, 6)},
    )


@pytest.mark.parametrize("axes_size", [1, 2, 3])
def test_layernorm_fusion(axes_size):
    """
    Detect layer norm pattern, found in the TF bert model.
    y = x * [gamma * rsqrt(variance + eps)] + (beta - mean * [gamma * rsqrt(variance + eps)])

    where mean and variance are computed along axes [-1] or [-1,-2] and so on
    and gamma and beta are constants with rank equal to the length of the axes parameter.
    """
    shape = (3, 5, 6)
    rank = len(shape)
    axes = list(range(rank - axes_size, rank))

    @mb.program(input_specs=[mb.TensorSpec(shape=shape)])
    def prog(x):
        x1 = mb.reduce_mean(x=x, axes=axes, keep_dims=True)
        x2 = mb.sub(x=x, y=x1)
        x2 = mb.square(x=x2)
        x2 = mb.reduce_mean(x=x2, axes=axes, keep_dims=True)
        x2 = mb.add(x=x2, y=1e-5)
        x2 = mb.rsqrt(x=x2)
        x3 = mb.mul(x=np.random.rand(*shape[-len(axes) :]), y=x2)
        x4 = mb.mul(x=x3, y=x1)
        x5 = mb.mul(x=x, y=x3)
        x4 = mb.sub(x=np.random.rand(*shape[-len(axes) :]), y=x4)
        y = mb.add(x=x4, y=x5)
        return y

    prev_prog, prev_block, block = apply_pass_and_basic_check(
        prog, "common::fuse_layernorm_or_instancenorm"
    )
    assert get_op_types_in_program(prev_prog) == [
        "reduce_mean",
        "sub",
        "square",
        "reduce_mean",
        "add",
        "rsqrt",
        "mul",
        "mul",
        "mul",
        "sub",
        "add",
    ]
    assert get_op_types_in_program(prog) == ["layer_norm"]
    assert_model_is_valid(
        prog, {"x": shape}, expected_output_shapes={block.outputs[0].name: shape}
    )


def test_instancenorm_fusion():
    """
    Detect instance norm pattern
    y = x * [gamma * rsqrt(variance + eps)] + (beta - mean * [gamma * rsqrt(variance + eps)])

    where input is rank 4, (N,C,H,W), axis=[2, 3], along which reduction happens,
    and gamma and beta are of shape (1,C,1,1)
    """
    shape = (3, 5, 6, 7)

    @mb.program(input_specs=[mb.TensorSpec(shape=shape)])
    def prog(x):
        x1 = mb.reduce_mean(x=x, axes=[2, 3], keep_dims=True)
        x2 = mb.sub(x=x, y=x1)
        x2 = mb.square(x=x2)
        x2 = mb.reduce_mean(x=x2, axes=[2, 3], keep_dims=True)
        x2 = mb.add(x=x2, y=1e-5)
        x2 = mb.rsqrt(x=x2)
        x3 = mb.mul(x=np.random.rand(1, shape[1], 1, 1), y=x2)
        x4 = mb.mul(x=x3, y=x1)
        x5 = mb.mul(x=x, y=x3)
        x4 = mb.sub(x=np.random.rand(1, shape[1], 1, 1), y=x4)
        y = mb.add(x=x4, y=x5)
        return y

    prev_prog, prev_block, block = apply_pass_and_basic_check(
        prog, "common::fuse_layernorm_or_instancenorm"
    )
    assert get_op_types_in_program(prev_prog) == [
        "reduce_mean",
        "sub",
        "square",
        "reduce_mean",
        "add",
        "rsqrt",
        "mul",
        "mul",
        "mul",
        "sub",
        "add",
    ]
    assert get_op_types_in_program(prog) == ["instance_norm"]
    assert_model_is_valid(
        prog, {"x": shape}, expected_output_shapes={block.outputs[0].name: shape}
    )


@pytest.mark.parametrize("rank", [1, 2, 3, 4])
def test_onehot_matmul_to_gather_fusion(rank):
    """
    Input:
        %2 = one_hot(%1, on_value=1, off_value=0, axis=-1)
        %3 = const() # rank 2
        %4  = matmul(%2, %3)

    Output:
        %4 = gather(%3, %2, axis=0)
    """
    rank4_shape = (10, 3, 6, 7)
    input_shape = rank4_shape[-rank:]
    vocab_size = 15
    embedding_size = 12

    @mb.program(input_specs=[mb.TensorSpec(shape=input_shape, dtype=types.int32)])
    def prog(x):
        x = mb.one_hot(
            indices=x, on_value=1, off_value=0, axis=-1, one_hot_vector_size=vocab_size
        )
        x = mb.matmul(x=x, y=np.random.rand(vocab_size, embedding_size))
        return x

    prev_prog, prev_block, block = apply_pass_and_basic_check(
        prog, "common::fuse_onehot_matmul_to_gather"
    )
    assert get_op_types_in_program(prev_prog) == ["one_hot", "matmul"]
    assert get_op_types_in_program(prog) == ["gather"]
    assert_model_is_valid(
        prog,
        {"x": input_shape},
        expected_output_shapes={block.outputs[0].name: input_shape + (embedding_size,)},
    )

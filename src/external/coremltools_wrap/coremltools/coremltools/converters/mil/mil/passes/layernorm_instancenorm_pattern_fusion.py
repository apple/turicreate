# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from coremltools.converters.mil.mil.passes.pass_registry import register_pass
from coremltools.converters.mil.mil import Builder as mb
import numpy as np


def _check_child_op_type(op, child_op_type):
    """
    :param op: operation
    :param child_op_type: str
    :return: Return True if op has 1 child and type of that child matches child_op_type
    """
    if len(op.outputs) != 1:
        return False
    child_ops = list(op.outputs[0].child_ops)
    if len(child_ops) != 1:
        return False
    if child_ops[0].op_type == child_op_type:
        return True
    return False


def try_to_transform(reduce_mean_op, block):
    all_ops = [reduce_mean_op]
    root_var = reduce_mean_op.x

    input_shape = root_var.shape
    if input_shape is None:
        return False

    rank = len(input_shape)

    # check that root_var feeds into exactly 3 ops
    if len(list(root_var.child_ops)) != 3:
        return False

    # check 1st reduce_mean op
    if not (
        reduce_mean_op.keep_dims.val is not None
        and reduce_mean_op.keep_dims.val == True
    ):
        return False
    axes = reduce_mean_op.axes.val
    if axes is None:
        return False

    # check 1st sub op
    child_ops_reduce_mean = list(reduce_mean_op.outputs[0].child_ops)
    if len(child_ops_reduce_mean) != 2:
        return False
    op_a = child_ops_reduce_mean[0]
    op_b = child_ops_reduce_mean[1]
    if not (
        (op_a.op_type == "sub" and op_b.op_type == "mul")
        or (op_a.op_type == "mul" and op_b.op_type == "sub")
    ):
        return False
    sub_op1 = op_a if op_a.op_type == "sub" else op_b
    if not (sub_op1.x == root_var and sub_op1.y == reduce_mean_op.outputs[0]):
        return False
    all_ops.append(sub_op1)

    # check square op
    if not _check_child_op_type(sub_op1, "square"):
        return False
    square_op = list(sub_op1.outputs[0].child_ops)[0]
    all_ops.append(square_op)

    # check second reduce mean
    if not _check_child_op_type(square_op, "reduce_mean"):
        return False
    reduce_mean_op2 = list(square_op.outputs[0].child_ops)[0]
    if not (
        reduce_mean_op2.keep_dims.val is not None
        and reduce_mean_op2.keep_dims.val == True
    ):
        return False
    if not (
        (reduce_mean_op2.axes.val is not None)
        and (axes == reduce_mean_op2.axes.val).all()
    ):
        return False
    all_ops.append(reduce_mean_op2)

    # check add op (with epsilon)
    if not _check_child_op_type(reduce_mean_op2, "add"):
        return False
    add_op1 = list(reduce_mean_op2.outputs[0].child_ops)[0]
    epsilon_var = add_op1.y if add_op1.x == reduce_mean_op2.outputs[0] else add_op1.x
    if epsilon_var.val is None:
        return False
    if len(epsilon_var.val.shape) != 0:
        # must be scalar
        return False
    all_ops.append(add_op1)

    # check rsqrt op
    if not _check_child_op_type(add_op1, "rsqrt"):
        return False
    rsqrt_op = list(add_op1.outputs[0].child_ops)[0]
    all_ops.append(rsqrt_op)

    # check mul (gamma)
    if not _check_child_op_type(rsqrt_op, "mul"):
        return False
    mul_op1 = list(rsqrt_op.outputs[0].child_ops)[0]
    gamma_var = mul_op1.y if mul_op1.x == rsqrt_op.outputs[0] else mul_op1.x
    if gamma_var.val is None:
        return False
    all_ops.append(mul_op1)

    # check 2 muls after the gamma mul
    child_ops = list(mul_op1.outputs[0].child_ops)
    if len(child_ops) != 2:
        return False
    mul_op2 = child_ops[0]
    mul_op3 = child_ops[1]
    if not (mul_op2.op_type == "mul" and mul_op3.op_type == "mul"):
        return False
    mul_op2_other_var = mul_op2.x if mul_op2.y == mul_op1.outputs[0] else mul_op2.y
    mul_op3_other_var = mul_op3.x if mul_op3.y == mul_op1.outputs[0] else mul_op3.y
    if not (
        (
            mul_op2_other_var == root_var
            and mul_op3_other_var == reduce_mean_op.outputs[0]
        )
        or (
            mul_op2_other_var == reduce_mean_op.outputs[0]
            and mul_op3_other_var == root_var
        )
    ):
        return False
    if mul_op2_other_var == root_var:
        mul_root_op = mul_op2
        mul_mean_op = mul_op3
    else:
        mul_root_op = mul_op3
        mul_mean_op = mul_op2
    all_ops.append(mul_mean_op)
    all_ops.append(mul_root_op)

    # check sub with beta
    if not _check_child_op_type(mul_mean_op, "sub"):
        return False
    sub_op2 = list(mul_mean_op.outputs[0].child_ops)[0]
    if sub_op2.y != mul_mean_op.outputs[0]:
        return False
    beta_var = sub_op2.x
    if beta_var.val is None:
        return False
    all_ops.append(sub_op2)

    # check last add op
    if not _check_child_op_type(sub_op2, "add"):
        return False
    add_op2 = list(sub_op2.outputs[0].child_ops)[0]
    if not (add_op2.x == mul_root_op.outputs[0] or add_op2.y == mul_root_op.outputs[0]):
        return False
    all_ops.append(add_op2)

    # check that none of the op in this pattern is connected to the output
    # (except the last add op)
    for i, op in enumerate(all_ops):
        if i == len(all_ops) - 1:
            continue
        for out in op.outputs:
            if out in block.outputs:
                return False

    # check whether the pattern is instance_norm or layer_norm
    is_layernorm = False
    is_instancenorm = False

    negative_axes = [a - rank if a >= 0 else a for a in axes]
    negative_axes.sort()

    if len(gamma_var.val.shape) == len(axes) and len(beta_var.val.shape) == len(axes):
        # axes for layer_norm must be [-1] or [-1,-2] or [-1,-2,-3] and so on
        if negative_axes == list(range(-len(negative_axes), 0)):
            is_layernorm = True

    if negative_axes == [-2, -1] and rank == 4:
        if (
            len(np.squeeze(gamma_var.val).shape) == 1
            and len(np.squeeze(beta_var.val).shape) == 1
        ):
            is_instancenorm = True

    if not (is_instancenorm or is_layernorm):
        return False

    # remove all the ops, and replace with a layer_norm or instance_norm op
    out_name = add_op2.outputs[0].name

    if is_instancenorm:
        x = mb.instance_norm(
            x=root_var,
            gamma=np.squeeze(gamma_var.val),
            beta=np.squeeze(beta_var.val),
            epsilon=epsilon_var,
            name=out_name,
            before_op=add_op2,
        )
    else:
        x = mb.layer_norm(
            x=root_var,
            axes=axes,
            gamma=gamma_var,
            beta=beta_var,
            epsilon=epsilon_var,
            name=out_name,
            before_op=add_op2,
        )

    add_op2.enclosing_block.replace_uses_of_var_after_op(
        anchor_op=add_op2, old_var=add_op2.outputs[0], new_var=x
    )
    # Remove all the ops at once
    block.remove_ops(all_ops)
    return True


def fuse_layernorm_or_instancenorm_block(block):
    fusion_status = False
    for i, op in enumerate(list(block.operations)):
        for b in op.blocks:
            block_changed = True
            while block_changed:
                block_changed = fuse_layernorm_or_instancenorm_block(b)
        if len(op.blocks) > 0:
            # This op can't be pow
            continue

        # start pattern match if reduce_mean op is encountered
        if op.op_type == "reduce_mean":
            with block:
                fusion_status = try_to_transform(op, block)
            # has to break as the downstream iterator is affected.
            if fusion_status:
                return fusion_status
    return fusion_status


@register_pass(namespace="common")
def fuse_layernorm_or_instancenorm(prog):
    """
    Identify the pattern:

    y = gamma * (x - mean) / sqrt(variance + epsilon) + beta

    y = x * [gamma * rsqrt(variance + eps)] + (beta - mean * [gamma * rsqrt(variance + eps)])

    [....] ---> reduce_mean --->  sub -->square -->reduce_mean --> add(epsilon)-->rsqrt
       |             |             ^                                                |
       |             |             |                                                |
       |             |             |                                                V
       |---------------------------                                               mul (gamma)
       |             |                                                              |
       |             |                                                      --------|---------
       |             |                                                      |                |
       |             |                                                      |                V
       |             |------------------------------------------------------------------->  mul
       |                                                                    |                |
       |                                                                    |                |
       |                                                                    V                |
       |-----------------------------------------------------------------> mul               |
                                                                            |                |
                                                                            |                V
                                                                            |              sub (beta) --> add --> [...]
                                                                            |                              ^
                                                                            |                              |
                                                                            |-------------------------------


    This pattern corresponds to either layer_norm or instance_norm.

    It is instance_norm if all of the following are true:
        - input is rank 4
        - axes of reduce_mean is [-2, -1]
        - gamma and beta are rank 1, after squeeze

    It is layer_norm if all of the following are true:
        - axes is either [-1] or [-1,-2] or [-1,-2,-3] and so on
        - rank of gamma and beta is equal to the length of the axes
    """
    for f_name, f in prog.functions.items():
        block_changed = True
        while block_changed:
            block_changed = fuse_layernorm_or_instancenorm_block(f)

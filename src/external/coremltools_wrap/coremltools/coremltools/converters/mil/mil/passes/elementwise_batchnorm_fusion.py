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


def match_pattern(op):
    if op.outputs[0] in op.enclosing_block.outputs:
        return None

    if op.op_type == "mul":
        # find add
        child_ops = op.outputs[0].child_ops
        if len(child_ops) == 1:
            add_op_candidate = list(child_ops)[0]
            if add_op_candidate.op_type == "add":
                return add_op_candidate
    return None


def _find_const_input_val(op):
    if op.x.val is not None:
        return op.x.val
    if op.y.val is not None:
        return op.y.val
    return None


def _check_shape(arr):
    """
    return True if shape is of form
    (1,C,1,1) or (C,1,1)
    """
    rank = len(arr.shape)
    if not (rank == 3 or rank == 4):
        return False
    C = arr.shape[-3]
    if not (arr.shape == (1, C, 1, 1) or arr.shape == (C, 1, 1)):
        return False
    return True


def try_to_transform(mul_op, add_op, block):
    non_const_input_mul = mul_op.x if mul_op.x.val is None else mul_op.y
    if non_const_input_mul.rank != 4:
        return False

    gamma = _find_const_input_val(mul_op)
    beta = _find_const_input_val(add_op)
    if gamma is None or beta is None:
        return False

    if not (isinstance(gamma, np.ndarray) and isinstance(beta, np.ndarray)):
        return False

    # check that gamma and beta have shape (1,C,1,1) or (C,1,1)
    # that is they are doing vector addition on the axis=-3, which is what the
    # batchnorm layer does (batchnorm layer only works on rank 4 input tensors)
    if not (_check_shape(gamma) and _check_shape(beta)):
        return False

    C = gamma.shape[-3]
    if C == 1:
        return False

    out_name = add_op.outputs[0].name
    x = mb.batch_norm(
        x=non_const_input_mul,
        mean=np.zeros((C,), np.float32),
        variance=np.ones((C,), np.float32),
        gamma=np.squeeze(gamma),
        beta=np.squeeze(beta),
        name=out_name,
        before_op=mul_op,
    )

    add_op.enclosing_block.replace_uses_of_var_after_op(
        anchor_op=add_op, old_var=add_op.outputs[0], new_var=x
    )
    # Remove all the ops at once
    block.remove_ops([mul_op, add_op])
    return True


def fuse_elementwise_to_batchnorm_block(block):
    fusion_status = False
    for op in list(block.operations):
        for b in op.blocks:
            block_changed = True
            while block_changed:
                block_changed = fuse_elementwise_to_batchnorm_block(b)
        if len(op.blocks) > 0:
            # This op can't be mul
            continue

        add_op = match_pattern(op)
        if add_op is not None:
            with block:
                fusion_status = try_to_transform(op, add_op, block)
            # has to break as the downstream iterator is affected.
            if fusion_status:
                return fusion_status
    return fusion_status


@register_pass(namespace="common")
def fuse_elementwise_to_batchnorm(prog):
    """
    Fold mul + add into a batch norm,
    if the const feeding into the mul/add is of shape (1,C,1,1) or (C,1,1)
    and input to mul is of rank 4.

    Given:
             [Const]   [Const]
                |         |
                V         V
    [...] --> [Mul] --> [Add] --> [...]

    That is,

        %2 = op1(%1)
        %3 = mul(%2, constant)
        %4 = add(%3, constant)
        %5 = op2(%4)
        ...

    Result:

    [...] --> [BatchNorm] --> [...]

    That is,
        %2 = op1(%1)
        %4 = batchnorm(%2)
        %5 = op2(%4)
        ...

    """
    for f_name, f in prog.functions.items():
        block_changed = True
        while block_changed:
            block_changed = fuse_elementwise_to_batchnorm_block(f)

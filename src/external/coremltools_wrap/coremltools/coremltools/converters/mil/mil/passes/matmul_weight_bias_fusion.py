# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import numpy as np
from coremltools.converters.mil.mil.passes.pass_registry import register_pass
from coremltools.converters.mil.mil import Builder as mb

child_op_types = ["add", "sub"]


def match_pattern(op):
    if op.op_type == "matmul":
        # find add
        child_ops = op.outputs[0].child_ops
        if len(child_ops) == 1:
            add_op_candidate = list(child_ops)[0]
            if add_op_candidate.op_type in child_op_types:
                return add_op_candidate
    return None


def transpose(v, before_op):
    """
    Transpose the last 2 dims.
    v: Var (must be a tensor)
    """
    perm = list(range(v.rank))
    perm[-2], perm[-1] = perm[-1], perm[-2]
    return mb.transpose(x=v, perm=perm, before_op=before_op)


def try_to_transform(matmul_op, add_op, block):
    if matmul_op.x.val is None and matmul_op.y.val is None:
        # This is a dynamic matmul.
        return False
    if add_op.x.val is None and add_op.y.val is None:
        # This is a dynamic add.
        return False

    x_is_weight = matmul_op.x.val is not None
    if x_is_weight:
        weight, linear_x = matmul_op.x, matmul_op.y
        transpose_weight = matmul_op.transpose_x.val
        transpose_x = matmul_op.transpose_y.val
    else:
        weight, linear_x = matmul_op.y, matmul_op.x
        transpose_weight = matmul_op.transpose_y.val
        transpose_x = matmul_op.transpose_x.val

    if linear_x.rank < 2 or weight.rank != 2:
        # We don't support these cases yet.
        return False

    d_out = weight.shape[1] if not transpose_weight else weight.shape[0]
    bias = add_op.x.val if add_op.x.val is not None else add_op.y.val
    if len(bias.shape) > 1:
        if any([d != 1 for d in bias.shape[:-1]]):
            return  # cannot transform

        # squeeze leading dims of size 1
        bias = np.squeeze(bias)

    if len(bias.shape) != 1 or bias.shape[0] != d_out:
        return  # cannot transform

    if add_op.op_type == "sub":
        bias = -bias
    out_name = add_op.outputs[0].name

    if x_is_weight:
        # If transpose_x == transpose_weight == False:
        # w*x = (x^T w^T)^T = linear(x^T, w)^T
        x_transposed = (
            transpose(linear_x, before_op=matmul_op) if not transpose_x else linear_x
        )
        w_no_transpose = (
            weight if not transpose_weight else transpose(weight, before_op=matmul_op)
        )
        x = mb.linear(
            x=x_transposed, weight=w_no_transpose, bias=bias, before_op=matmul_op
        )
        x = transpose(x, before_op=matmul_op, name=out_name)
    else:
        # If transpose_x == transpose_weight == False
        # x*w = x*(w^T)^T = linear(x, w^T)
        x_no_transpose = (
            transpose(linear_x, before_op=matmul_op) if transpose_x else linear_x
        )
        w_transposed = (
            weight if transpose_weight else transpose(weight, before_op=matmul_op)
        )
        x = mb.linear(
            x=x_no_transpose,
            weight=w_transposed,
            bias=bias,
            before_op=matmul_op,
            name=out_name,
        )

    add_op.enclosing_block.replace_uses_of_var_after_op(
        anchor_op=add_op, old_var=add_op.outputs[0], new_var=x
    )
    # Remove all the ops at once
    block.remove_ops([matmul_op, add_op])
    return True


def fuse_matmul_weight_bias_block(block):
    fusion_status = False
    for op in list(block.operations):
        for b in op.blocks:
            block_changed = True
            while block_changed:
                block_changed = fuse_matmul_weight_bias_block(b)
        if len(op.blocks) > 0:
            # This op can't be matmul
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
def fuse_matmul_weight_bias(prog):
    """
    Convert matmul + add/sub to linear whenever possible.

    Given:
        %3 = matmul(x=%1, y=%2)  # %1 or %2 is const and rank 2 (weight)
        ...
        %5 = add(x=%3, y=%4) # %4 is const. add(x=%4, y=%3) is equivalent
                             # sub is similar.

    Result:
        # assuming %2 above is const and rank 2
        %5 = linear(x=%1, weight=%2, bias=%4)

    Inputs:

        prog: Program
    """
    for f_name, f in prog.functions.items():
        block_changed = True
        while block_changed:
            block_changed = fuse_matmul_weight_bias_block(f)

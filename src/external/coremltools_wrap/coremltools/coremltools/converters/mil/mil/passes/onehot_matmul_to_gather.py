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


def _check_var_scalar_value(x, val, tol=1e-3):
    """
    :param x: var
    :param val: a scalar value
    :return: True if the value of var is equal to val otherwise return False
    """
    if x.val is None:
        return False
    if not isinstance(x.val, (np.ndarray, np.generic)):
        return False

    if isinstance(x.val, np.ndarray):
        if x.val.size != 1:
            return False
        x_val = x.val[:][0]
    else:
        x_val = x.val

    if abs(x_val - val) < tol:
        return True
    return False


def try_to_transform(onehot_op, block):
    root_var = onehot_op.indices

    # check that the output of the onehot op is not a block output
    if onehot_op.outputs[0] in block.outputs:
        return False

    # check that onehot op has axis=-1, on_value=1 and off_value=0
    # and constant one_hot_vector_size
    axis = onehot_op.axis.val
    if axis is None:
        return False
    if onehot_op.indices.shape is None:
        return False
    rank = len(onehot_op.indices.shape)
    if axis >= 0:
        axis -= rank
    if axis != -1:
        return False
    if not _check_var_scalar_value(onehot_op.on_value, 1):
        return False
    if not _check_var_scalar_value(onehot_op.off_value, 0):
        return False
    if onehot_op.one_hot_vector_size.val is None:
        return False

    # checks for the following matmul op
    if not _check_child_op_type(onehot_op, "matmul"):
        return False
    matmul_op = list(onehot_op.outputs[0].child_ops)[0]
    if matmul_op.x != onehot_op.outputs[0]:
        return False
    if matmul_op.transpose_x.val or matmul_op.transpose_y.val:
        return False
    W_var = matmul_op.y
    if W_var.val is None:
        return False
    if len(W_var.val.shape) != 2:
        return False

    # remove onehot and matmul and replace with gather op
    out_name = matmul_op.outputs[0].name
    x = mb.gather(x=W_var, indices=root_var, axis=0, name=out_name, before_op=matmul_op)

    matmul_op.enclosing_block.replace_uses_of_var_after_op(
        anchor_op=matmul_op, old_var=matmul_op.outputs[0], new_var=x
    )
    # Remove all the ops at once
    block.remove_ops([onehot_op, matmul_op])
    return True


def fuse_onehot_matmul_to_gather_block(block):
    fusion_status = False
    for i, op in enumerate(list(block.operations)):
        for b in op.blocks:
            block_changed = True
            while block_changed:
                block_changed = fuse_onehot_matmul_to_gather_block(b)
        if len(op.blocks) > 0:
            # This op can't be pow
            continue

        # start pattern match if one_hot op is encountered
        if op.op_type == "one_hot":
            with block:
                fusion_status = try_to_transform(op, block)
            # has to break as the downstream iterator is affected.
            if fusion_status:
                return fusion_status
    return fusion_status


@register_pass(namespace="common")
def fuse_onehot_matmul_to_gather(prog):
    """
    Detect if onehot (axis=-1, on_value=1, off_value=0) is followed by a matmul op (no bias),
    then they can be replaced by a gather op.

    Input:
        %2 = one_hot(%1, on_value=1, off_value=0, axis=-1)
        %3 = const() # rank 2
        %4  = matmul(%2, %3)

    Output:
        %4 = gather(%3, %2, axis=0)

    """
    for f_name, f in prog.functions.items():
        block_changed = True
        while block_changed:
            block_changed = fuse_onehot_matmul_to_gather_block(f)

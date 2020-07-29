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

child_op_types = ["add", "sub"]


def match_pattern(op):
    if op.op_type == "conv" or op.op_type == "conv_transpose":
        # abort fusion if op output is also a block output
        if op.outputs[0] in op.enclosing_block.outputs:
            return None
        # find add
        child_ops = op.outputs[0].child_ops
        if len(child_ops) == 1:
            add_op_candidate = list(child_ops)[0]
            if add_op_candidate.op_type in child_op_types:
                return add_op_candidate
    return None


def try_to_transform(conv_op, add_op, block):
    if add_op.op_type == "sub":
        bias_var = add_op.y
    else:
        bias_var = add_op.x if add_op.x.val is not None else add_op.y
    bias_value = bias_var.val

    # check that the bias value is a constant array or a scalar constant
    if not isinstance(bias_value, (np.ndarray, np.generic)):
        return False

    is_bias_scalar = False
    if not isinstance(bias_value, np.ndarray):
        is_bias_scalar = True

    # find rank of the conv input
    rank = conv_op.x.rank
    if rank is None:
        return False
    if not (rank == 3 or rank == 4 or rank == 5):
        return False

    # check compatibility of bias value with the rank of the conv op
    # either bias value should be a scalar or:
    # rank=3 ==> (B,C,D), which means bias must be (1,C,1) or (C,1)
    # rank=4 ==> (B,C,D1,D2), which means bias must be (1,C,1,1) or (C,1,1)
    # rank=5 ==> (B,C,D1,D2,D3), which means bias must be (1,C,1,1,1) or (C,1,1,1)

    if is_bias_scalar:
        bias_value = np.array([bias_value])
    else:
        # check that there is at most one dimension in the shape that is not 1
        if len(np.squeeze(bias_value).shape) > 1:
            return False
        # check that addition is not happening on the batch dimension
        if len(bias_value) == rank:
            if bias_value.shape[0] != 1:
                return False
        # check that last rank-2 entries in the shape vector are all 1s
        if np.prod(bias_value.shape[-(rank - 2) :]) != 1:
            return False
        bias_value = np.squeeze(bias_value)

    if add_op.op_type == "sub":
        bias_value *= -1

    # everything looks good, now find the new updated bias
    old_bias = conv_op.inputs.get("bias", None)
    old_bias_value = None
    if old_bias is not None and old_bias.val is not None:
        old_bias_value = old_bias.val
    if old_bias is None:
        # need to create a fresh numpy array for bias
        if np.prod(bias_value.shape) == 1:
            # its a scalar bias
            # need to find the value of Cout to form a new bias
            if conv_op.weight.val is None:
                return False
            Cout = conv_op.weight.val.shape[0]
            new_bias_value = np.broadcast_to(bias_value, (Cout,))
        else:
            new_bias_value = bias_value
    else:
        # just need to update the existing bias array
        try:
            new_bias_value = old_bias_value + bias_value
        except:
            return False

    # create a new conv op with the new bias value, copying rest of the attributes
    out_name = add_op.outputs[0].name
    new_bias_var = mb.const(val=new_bias_value, mode="file_value", before_op=conv_op)

    conv_kargs = {"bias": new_bias_var, "name": out_name, "before_op": conv_op}

    for k, v in conv_op.inputs.items():
        if k == "bias":
            continue
        conv_kargs[k] = v

    if conv_op.op_type == "conv":
        x = mb.conv(**conv_kargs)
    else:
        x = mb.conv_transpose(**conv_kargs)

    add_op.enclosing_block.replace_uses_of_var_after_op(
        anchor_op=add_op, old_var=add_op.outputs[0], new_var=x
    )
    # Remove all the ops at once
    block.remove_ops([conv_op, add_op])
    return True


def fuse_bias_conv_block(block):
    fusion_status = False
    for op in list(block.operations):
        for b in op.blocks:
            block_changed = True
            while block_changed:
                block_changed = fuse_bias_conv_block(b)
        if len(op.blocks) > 0:
            # This op can't be conv or conv_transpose
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
def fuse_bias_conv(prog):
    """
    Fold add/sub into bias of conv and conv_transpose
    That is, convert conv + add/sub to conv, when add/sub is adding a constant

    Given:
        %2 = conv(%1)
        ...
        %3 = add(%2, constant) # where constant has shape (1,C,1)/(C,1) for 1d conv, (1,C,1,1)/(C,1,1) for 2d conv etc
        ...

    Result:
        %3 = conv(%1)
        ...

    """
    for f_name, f in prog.functions.items():
        block_changed = True
        while block_changed:
            block_changed = fuse_bias_conv_block(f)

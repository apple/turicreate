# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import numpy as np
from coremltools.converters.mil.mil.types.symbolic import (
    is_symbolic,
    any_variadic,
    num_symbolic,
)
from coremltools.converters.mil.mil import Builder as mb
from coremltools.converters.mil.mil.passes.pass_registry import register_pass
import logging


def remove_symbolic_reshape_block(block):
    num_changes = 0
    for op in list(block.operations):
        for b in op.blocks:
            num_changes += remove_symbolic_reshape_block(b)
        if op.op_type != "reshape":
            continue
        if op.shape.val is not None:
            # shape does not contain symbol.
            continue
        if op.shape.sym_val is None:
            # shape is runtime determined.
            continue
        # Use output shape as `shape`
        shape = op.outputs[0].shape
        if any_variadic(shape):
            msg = (
                "Cannot reshape to variadic from a compile time "
                + "shape argument. Variadic shape can only be achieved "
                + "via runtime shape argument. op: {}"
            )
            raise ValueError(msg.format(op))
        num_symbols = num_symbolic(shape)
        if num_symbols > 1:
            continue
        # Convert the one symbol to -1
        integer_shape = [-1 if is_symbolic(i) else i for i in shape]
        with block:
            shape_const = mb.const(
                val=integer_shape,
                mode="immediate_value",
                name=op.shape.name + "x",
                before_op=op,
            )
            reshaped = mb.reshape(x=op.x, shape=shape_const, name=op.name, before_op=op)
            op.enclosing_block.replace_uses_of_var_after_op(
                anchor_op=op, old_var=op.outputs[0], new_var=reshaped
            )
            # Remove all the ops at once
            block.remove_ops([op, op.shape.op])
        num_changes += 1
    return num_changes


@register_pass(namespace="common")
def remove_symbolic_reshape(prog):
    """
    Convert symbolic shape in `reshape` to integers.

    Given:

        main(%x: (s0, 4, fp32)) {
          block0() {
            %reshape_0_shape_0: (3,i32)^ = const(val=(s0, s1, 2))
            %reshape_0: (s0, 2, 2, fp32) = reshape(x=%x, shape=%reshape_0_shape_0)
          } -> (%reshape_0)
        }

    Result:
        main(%x: (s0, 4, fp32)) {
          block0() {
            %reshape_0_shape_0x: (3,i32)* = const(val=[-1, 2, 2])
            %reshape_0: (-1, 2, 2, fp32) = reshape(x=%x, shape=%reshape_0_shape_0x)
          } -> (%reshape_0)
        }

    Comment: Currently it does not perform any optimization, but simply
    replacing symbols with positive integer if solved from volumetric
    constraint, or -1. Therefore this pass fails if more than one symbols
    need to be resolve to -1.

    TODO (rdar://59165842): Use expand_dims, squeeze etc to use 0 instead
    of dynamic reshape with -1.

    Inputs:

        prog: Program
    """
    for f_name, f in prog.functions.items():
        num_changes = remove_symbolic_reshape_block(f)
        msg = "remove_symbolic_reshape: changed {} reshapes."
        logging.info(msg.format(num_changes))

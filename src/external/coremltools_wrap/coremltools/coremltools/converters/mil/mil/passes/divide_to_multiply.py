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


def divide_to_multiply_block(block):
    for op in list(block.operations):
        for b in op.blocks:
            divide_to_multiply_block(b)
        if len(op.blocks) > 0:
            # This op can't be divide.
            continue

        if op.op_type == "real_div" and op.y.val is not None:
            with block:
                x = mb.mul(
                    x=op.x, y=1.0 / op.y.val, name="_inversed_" + op.name, before_op=op
                )
                op.enclosing_block.replace_uses_of_var_after_op(
                    anchor_op=op, old_var=op.outputs[0], new_var=x
                )
                block.remove_ops([op])


@register_pass(namespace="common")
def divide_to_multiply(prog):
    """
    Convert divide into multiply if divisor is const.
    """
    for f_name, f in prog.functions.items():
        divide_to_multiply_block(f)

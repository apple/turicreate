# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from coremltools.converters.mil.mil import Builder as mb
from coremltools.converters.mil.mil.passes.pass_registry import register_pass
import logging


def remove_vacuous_cond_block(block):
    num_changes = 0
    for op in list(block.operations):
        for b in op.blocks:
            num_changes += remove_vacuous_cond_block(b)

        if op.op_type != "cond":
            continue

        then_ops = op.blocks[0].operations
        else_ops = op.blocks[1].operations

        if len(then_ops) > 1 or len(else_ops) > 1:
            continue

        # Pattern 1: dynamic length TensorList generates this pattern. See
        # conversion functions of TensorList* ops for details. TF2's graph
        # contains a tf.cond op with 2 sub-graphs. The condition is either
        # `less_equal` or `greater_equal` op. 1 sub-graph contains only an
        # identity op forwarding the original TensorList, another sub-graph
        # contains TensorListResize op to generate a new TensorList. But in
        # backend, list length is handled dynamically in list_write/scatter
        # and thus, the entire tf.cond and it's sub-graphs can be removed.
        if len(then_ops) == 0 and len(else_ops) == 0:
            if op.pred.op.op_type not in {"less_equal", "greater_equal"}:
                continue

            # cond op must have pred
            pred_x = op.pred.op.x.op
            pred_y = op.pred.op.y.op

            if pred_x is None and pred_y is None:
                continue

            if op.pred.op.op_type == "less_equal":
                if pred_x.op_type != "list_length":
                    continue
                new_var = pred_x.ls

            else:  # op.pred.op.op_type == 'greather_equal':
                if pred_y.op_type != "list_length":
                    continue
                new_var = pred_y.ls

            with block:
                op.enclosing_block.replace_uses_of_var_after_op(
                    anchor_op=op, old_var=op.outputs[0], new_var=new_var
                )
                block.remove_ops([op])  # rely on DCE to remove extra cond inputs
            num_changes += 1

        # Pattern 2: both than and else branch contains exactly 1 identity op
        if len(then_ops) == 1 and len(then_ops) == 1:
            if then_ops[0].op_type != "identity" or else_ops[0].op_type != "identity":
                continue
            if then_ops[0].x != else_ops[0].x:
                continue

            with block:
                new_var = mb.identity(x=then_ops[0].x, before_op=op, name=op.name)
                op.enclosing_block.replace_uses_of_var_after_op(
                    anchor_op=op, old_var=op.outputs[0], new_var=new_var
                )
                block.remove_ops([op])  # rely on DCE to remove extra cond inputs
            num_changes += 1

    return num_changes


@register_pass(namespace="tensorflow2")
def remove_vacuous_cond(prog):
    """
    Remove cond op and it's sub-graphs that produces identity on both then and
    else branch. One example use case is the TensorListReverse op, in Core ML,
    we dynamically resize in write operations, and thus, both branches of the
    cond op will be a skip (identity) op.

    Given:

        main(%a: (1, bool),
         %b: (2, 3, fp32)) {
          block0() {
            %squeeze_0: (bool) = squeeze(x=%a, name="squeeze_0")
            %cond_0: (2, 3, fp32) = cond(pred=%squeeze_0, name="cond_0")
              cond_0_true() {
                %identity_0: (2, 3, fp32) = identity(x=%b, name="identity_0")
              } -> (%identity_0)
              cond_0_false() {
                %identity_1: (2, 3, fp32) = identity(x=%b, name="identity_1")
              } -> (%identity_1)
          } -> (%cond_0)
        }

    Result:

        main(%a: (1, bool),
             %b: (2, 3, fp32)) {
          block0() {
            %squeeze_0: (bool) = squeeze(x=%a, name="squeeze_0")
            %cond_0: (2, 3, fp32) = identity(x=%b, name="cond_0")
          } -> (%cond_0)
        }
    """
    for f_name, f in prog.functions.items():
        num_changes = remove_vacuous_cond_block(f)
        msg = "remove_vacuous_cond: changed {} ops in function '{}'"
        logging.info(msg.format(num_changes, f_name))

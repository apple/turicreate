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
from coremltools.converters.mil.mil import types


@register_pass(namespace="tensorflow")
def backfill_make_list_elem_type(prog):
    """
    TF's TensorArrayV3 (represented as make_list in mil) doesn't necessarily
    contain elem shape/type, which is known when write is performed. We
    backfill elem type info to make_list

    Inputs:

        prog: Program
    """
    for f_name, f in prog.functions.items():
        backfill_make_list_elem_type_block(f)


def backfill_make_list_elem_type_block(block):
    # shallow copy hides changes on f.operations during the loop
    for op in block.operations[:]:
        for b in op.blocks:
            backfill_make_list_elem_type_block(b)

        if op.op_type != "tf_make_list":
            continue
        # op is `make_list`

        if op.outputs[0].elem_type != types.unknown:
            # elem_type of the list is known
            continue

        list_var = op.outputs[0]
        elem_type = infer_elem_type(list_var)  # types.tensor
        if elem_type is None:
            msg = (
                "No list_write or list_scatter op to infer make_list "
                + "'{}' element type. Block:\n{}"
            )
            raise ValueError(msg.format(op.name, op.enclosing_block))

        with block:
            new_list = mb.make_list(
                init_length=op.init_length,
                dynamic_length=op.dynamic_length,
                # elem_shape cannot be symbolic by definition of list.
                elem_shape=elem_type.get_shape(),
                dtype=op.inputs["dtype"],
                before_op=op,
                name=op.name,
            )

        block.replace_uses_of_var_after_op(
            anchor_op=op, old_var=op.outputs[0], new_var=new_list
        )
        block.remove_ops([op])


def infer_elem_type(list_var):
    """
    Returns types.tensor. None if failed to infer element type.
    Example:

    Given:

    main(%update: (2,fp32)) {
      block0() {
        %list: List[unknown] = tf_make_list(...) # unknown elem type
        %while_loop_0:0: (i32), %while_loop_0:1: List[(2,fp32)] = while_loop(loop_vars=(...))
          while_loop_0_body(...) {
            %list_write_0: List[(2,fp32)] = list_write(index=..., ls=%list, value=%update)
          } -> (%add_0, %list_write_0)

        Result:

        main(%update: (2,fp32)) {
          block0() {
        %list: List[(2,fp32)] = tf_make_list(...) # Get the elem type from list_write
        %while_loop_0:0: (i32), %while_loop_0:1: List[(2,fp32)] = while_loop(loop_vars=(...))
          while_loop_0_body(...) {
            %list_write_0: List[(2,fp32)] = list_write(index=..., ls=%list, value=%update)
          } -> (%add_0, %list_write_0)
    """
    # Search for child op that have informative element types
    for o in list_var.child_ops:
        if o.op_type in ["list_write", "list_scatter"]:
            return o.outputs[0].elem_type
        if o.op_type == "while_loop":
            idx = list(o.loop_vars).index(list_var)
            block = o.blocks[0]
            # the corresponding Var in body block
            block_var = block.inputs[idx]
            elem_type = infer_elem_type(block_var)
            if elem_type is not None:
                return elem_type
            # otherwise continue to other block_var (a list_var can be
            # passed into while_loop twice).
    return None

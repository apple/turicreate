# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from coremltools.converters.mil.mil.passes.pass_registry import register_pass


def commingle_loop_vars_block(block):
    for op in list(block.operations):
        for b in op.blocks:
            commingle_loop_vars_block(b)

        if op.op_type != "while_loop":
            continue

        block = op.blocks[0]

        for v_out, vx_in in zip(op.outputs, block.inputs):
            # Disable check as v_out is not visible in block.
            block.replace_uses_of_var_after_op(
                anchor_op=None,
                old_var=vx_in,
                new_var=v_out,
                no_check_var_visibility=True,
            )

        # replace block inputs
        block._block_inputs = op.outputs


@register_pass(namespace="nn_backend")
def commingle_loop_vars(prog):
    """
    prog: Program

    # NN backend expects output vars as loop vars. Example:
    #
    # Given:
    #    main(%a: (1, 2, fp32),
    #         %b: (1, 2, fp32)) {
    #      block0() {
    #        %loop:0: (1, 2, fp32), %loop:1: (1, 2, fp32) = \
    #        while_loop(loop_vars=(%a, %b))
    #          loop_cond(%a.x, %b.x) {
    #            %cond_var: (bool) = some_op(x=%a.x, y=%b.x)
    #          } -> (%cond_var)
    #          loop_body(%a.x, %b.x) {
    #            %add_0: (1, 2, fp32) = add(x=%a.x, y=%b.x)
    #          } -> (%add_0, %b.x)
    #      } -> (%loop:0, %loop:1)
    #    }
    #
    # Result:
    #    main(%a: (1, 2, fp32),
    #         %b: (1, 2, fp32)) {
    #      block0() {
    #        %loop:0: (1, 2, fp32), %loop:1: (1, 2, fp32) = \
    #        while_loop(loop_vars=(%a, %b))
    #          loop_cond(%loop:0, %loop:1) {
    #            %cond_var: (bool) = some_op(x=%loop:0, y=%loop:1)
    #          } -> (%cond_var)
    #          loop_body(%loop:0, %loop:1) {
    #            %add_0: (1, 2, fp32) = add(x=%loop:0, y=%loop:1)
    #          } -> (%add_0, %loop:1)
    #      } -> (%loop:0, %loop:1)
    #    }
    #
    # Comment: The resulting program is no longer SSA (multiple assignments on
    # %loop:0).
    """
    for f_name, f in prog.functions.items():
        commingle_loop_vars_block(f)

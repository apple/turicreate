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


def handle_return_inputs_as_outputs_func(f):
    returned_inputs = []
    for v_name, v in f.inputs.items():
        if v not in f.outputs:
            continue
        returned_inputs.append(v)

    with f:
        for v in returned_inputs:
            # copy twice since NN layer cannot have input name == output name
            v_tmp = mb.identity(x=v, name=v.name + "_tmp")
            res = mb.identity(x=v_tmp, name=v.name)
            res.op.enclosing_block.replace_uses_of_var_after_op(
                anchor_op=res.op, old_var=v, new_var=res
            )


@register_pass(namespace="nn_backend")
def handle_return_inputs_as_outputs(prog):
    """
    prog: Program

    # NN cannot handle returning input as output. Insert an identity op for
    # those cases. Example:
    #
    # Given:
    #    main(%a: (1, 2, fp32),
    #         %b: (1, 2, fp32)) {
    #      block0() {
    #        %mul_0_y_0: (i32)* = const(val=2)
    #        %mul_0: (1, 2, fp64) = mul(x=%a, y=%mul_0_y_0)
    #      } -> (%mul_0, %b)
    #    }
    #
    # (Notice that %b is returned from input. This causes error in NN)
    #
    # Result:
    #    main(%a: (1, 2, fp32),
    #         %b: (1, 2, fp32)) {
    #      block0() {
    #        %mul_0_y_0: (i32)* = const(val=2)
    #        %mul_0: (1, 2, fp64) = mul(x=%a, y=%mul_0_y_0)
    #        %b_tmp: (1, 2, fp32) = identity(x=%b)
    #        %b: (1, 2, fp32) = identity(x=%b_tmp)
    #      } -> (%mul_0, %b)
    #    }
    #
    # where identity is applied twice since NN layer cannot have
    # input name == output name
    """
    for f_name, f in prog.functions.items():
        handle_return_inputs_as_outputs_func(f)

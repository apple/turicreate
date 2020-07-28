#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil.mil.passes.pass_registry import register_pass
import logging


def dead_code_elimination_block(block):
    used_vars = set()
    ops_to_remove = list()

    # mark block's outputs to used
    used_vars.update(block.outputs)

    for op in reversed(block.operations):
        # if none of op's output is used, delete op
        if not set(op.outputs).intersection(used_vars):
            ops_to_remove.append(op)
            continue

        # mark all op's inputs to used
        for _, input_var in op.inputs.items():
            if isinstance(input_var, (tuple, list)):
                used_vars.update(list(input_var))
            else:
                used_vars.update([input_var])

        for b in op.blocks:
            used_in_block = dead_code_elimination_block(b)
            used_vars.update(used_in_block)

    for op in ops_to_remove:
        logging.info('Removing op "{}" (type: {})'.format(op.name, op.op_type))
        op.remove_from_block()

    return used_vars


@register_pass(namespace="common")
def dead_code_elimination(program):
    """
    Eliminate unused ops in program.

    Parameters
    ----------
    program: Program SSA Program before graph pass

    Returns
    -------
    program: Program SSA Program after graph pass

    Example
    -------

        Given:
        main(%x: (2, 4, fp32)) {
          block0() {
            %const_2: (4, 2, fp32)* = const(val=[...])
            %const_3: (4, fp32)* = const(val=[...])
            %tx_0: (bool)* = const(val=False)
            %ty_0: (bool)* = const(val=False)
            %matmul_0: (2, 2, fp32) = matmul(x=%x, y=%const_2, transpose_x=%tx_0, transpose_y=%ty_0)
            %linear_0: (2, 4, fp32) = linear(x=%x, weight=%const_2, bias=%const_3)
          } -> (%linear_0)
        }

        Result:
        main(%x: (2, 4, fp32)) {
          block0() {
            %const_2: (4, 2, fp32)* = const(val=[...])
            %const_3: (4, fp32)* = const(val=[...])
            %linear_0: (2, 4, fp32) = linear(x=%x, weight=%const_2, bias=%const_3)
          } -> (%linear_0)
        }

    Ops whose outputs are not contributed to final outputs will be deleted.
    In this example, %matmul_0 is an op that's not used in the computation,
    this op and its input ops (%tx_0 and %ty_0) are eliminated in this pass.
    """

    for name, f in program.functions.items():
        dead_code_elimination_block(f)

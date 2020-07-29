#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import logging

from coremltools.converters.mil.testing_reqs import _converter as converter
from coremltools.converters.mil.mil.types.symbolic import is_symbolic
from coremltools.converters.mil.mil import Program, Function
from coremltools.converters.mil.testing_utils import compare_backend

UNK_VARIADIC = "*s_unk"
UNK_SYM = "s_unk"


def run_compare_builder(
    build,
    input_placeholders,
    input_values,
    expected_output_types=None,
    expected_outputs=None,
    use_cpu_only=False,
    frontend_only=False,
    backend="nn_proto",
    atol=1e-04,
    rtol=1e-05,
):
    """
    Inputs:
        - build: python function taking input of Vars and returning Var or
          list[Var]. Each input argument in build must match a key in
          input_values / input_placeholders.

        - input_placeholders: str -> placeholder. It may not be an empty
                              dict as MLModel doesn't support function with
                              no input.

        - expected_output_types: list[(shape, builtin_type)] or (shape,
          builtin_type).  None skips type inference validation.

        - expected_outputs: list[np.array] or np.array. Required iff
          frontend_only == False

        - frontend_only: True to test up to proto generation.
    """
    if not isinstance(expected_output_types, list):
        expected_output_types = [expected_output_types]

    if expected_outputs is not None and not isinstance(expected_outputs, list):
        expected_outputs = [expected_outputs]

    prog = Program()
    with Function(input_placeholders) as ssa_func:
        output_vars = build(**ssa_func.inputs)
        if isinstance(output_vars, tuple):
            output_vars = list(output_vars)
        elif not isinstance(output_vars, list):
            output_vars = [output_vars]
        ssa_func.set_outputs(output_vars)
        prog.add_function("main", ssa_func)

    # get output names for output_vars
    output_names = [x.name for x in output_vars]

    # Validate type inference
    msg = (
        "Provided expected outputs types {} should match number of output"
        + " variables {}"
    )
    assert_msg = msg.format(len(expected_output_types), len(output_vars))
    assert len(output_vars) == len(expected_output_types), assert_msg

    for out_var, s in zip(output_vars, expected_output_types):
        if out_var.dtype != s[-1]:
            raise ValueError(
                "Output {} type: expect {}, got {}. Program:\n{}".format(
                    out_var.name, s[-1], out_var.dtype, prog
                )
            )
        if UNK_VARIADIC in s[:-1]:
            msg = "Skip type checking for UNK_VARIADIC. Output shape: {} vs expected shape: {}"
            logging.debug(msg.format(out_var.shape, s[:-1]))
            continue
        expected_shape = s[:-1]
        msg = "Output {} shape: expect {}, got {}. Program:\n{}".format(
            out_var.name, expected_shape, out_var.shape, prog
        )
        # No more variadic here.
        if len(out_var.shape) != len(expected_shape):
            raise ValueError(msg)
        # replace UNK_SYM in out_var.shape.
        output_shape = [
            0 if es == UNK_SYM else os for os, es in zip(out_var.shape, expected_shape)
        ]
        expected_shape = [0 if es == UNK_SYM else es for es in expected_shape]
        # convert float etc to int.
        output_shape = [i if is_symbolic(i) else int(i) for i in output_shape]
        expected_shape = [i if is_symbolic(i) else int(i) for i in expected_shape]
        if output_shape != expected_shape:
            raise ValueError(msg)

    proto = converter._convert(prog, convert_from="mil", convert_to=backend)

    if frontend_only:
        return

    if expected_outputs:
        assert len(output_vars) == len(expected_outputs), (
            "Provided expected_outputs {}"
            " should match number of output"
            " variables {}".format(len(expected_outputs), len(output_vars))
        )

        expected_outputs = {
            name: val for name, val in zip(output_names, expected_outputs)
        }

    compare_backend(
        proto=proto,
        input_key_values=input_values,
        expected_outputs=expected_outputs,
        use_cpu_only=use_cpu_only,
        atol=atol,
        rtol=rtol,
        also_compare_shapes=False,
    )

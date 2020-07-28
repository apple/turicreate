#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from collections import defaultdict
import copy
import logging
import six
import numbers
import numpy as np

from coremltools.converters.mil.mil.types.symbolic import any_symbolic

from . import curr_block, Program, Function, Placeholder, is_internal_input
from .input_type import (
    _InputType,
    InternalStringInputType,
    InternalScalarOrTensorInputType,
    ScalarOrTensorInputType,
    TupleInputType,
    InputSpec,
    InternalInputType,
    PyFunctionInputType,
)
from .var import InternalVar, Var


def get_const_mode(val):
    # Heuristics to decide between file_value and immediate_value
    if isinstance(val, (np.ndarray, np.generic)) and val.size > 10:
        return "file_value"
    return "immediate_value"


def is_python_value(val):
    return (
        isinstance(val, (np.generic, np.ndarray))
        or isinstance(val, numbers.Number)
        or isinstance(val, six.string_types)
        or isinstance(val, bool)
        or (isinstance(val, (tuple, list)) and all(is_python_value(v) for v in val))
    )


class Builder:
    """
    Singleton builder.

    Example:

    from coremltools.converters.mil.mil import Builder as mb
    from coremltools.converters.mil.mil import Program, Function

    prog = Program()
    func_inputs = {"x": mb.placeholder(_shape=[2,3]),
                   "y": mb.placeholder(_shape=[2,3])}
    with Function(func_inputs) as ssa_fun:
      x, y = ssa_fun.inputs['x'], ssa_fun.inputs['x']
      res_var = mb.add(x=x, y=y) # created within ssa_fun block
      ssa_fun.set_outputs([res_var])
    prog.add_function("main", ssa_fun)
    """

    name_count = defaultdict(int)

    @classmethod
    def _get_free_name(cls, name):
        new_name = name + "_" + str(cls.name_count[name])
        cls.name_count[name] += 1
        return new_name

    @classmethod
    def _maybe_set_name(cls, kwargs, op_type):
        if "name" not in kwargs:
            kwargs["name"] = cls._get_free_name(op_type)
        return kwargs

    @classmethod
    def _add_const(cls, val, name, before_op):
        if not is_python_value(val):
            raise ValueError("Cannot add const {}".format(val))
        if any_symbolic(val):
            msg = (
                "Python native vals (list, tuple), np.array that are"
                + "operation inputs cannot have symbolic values. Consider feeding"
                + "symbolic shape in through placeholder and use mb.shape() "
                + "operator. Input {}: {}"
            )
            raise ValueError(msg.format(name, val))
        const_name = cls._get_free_name(name)
        mode = get_const_mode(val)
        logging.debug("Adding const op '{}'".format(const_name))
        output_var = cls.const(mode=mode, val=val, name=const_name, before_op=before_op)
        return output_var

    @classmethod
    def _create_input_vars(cls, input_spec, op_name, op_cls, before_op, kwargs):
        """
        1. Create Var for optional input types with default values that's not
        specified.

        2. Convert python primitive types to Var.

        Inputs:

        input_spec (InputSpec)
        op_name (str): op name.
        before_op: created all vars / const op will come right before
                   `before_op` in the block's order. None to append at the end.
        """
        update_dict = {}
        for in_name, in_type in input_spec.input_types.items():
            new_var_name = op_name + "_" + in_name
            if not in_type.optional and in_name not in kwargs:
                raise ValueError(
                    "Input '{}' is required for op '{}'.".format(in_name, op_cls.__name__)
                )

            if in_name in kwargs and isinstance(kwargs[in_name], Var):
                # check const
                if in_type.const and kwargs[in_name].val is None:
                    msg = "Input '{}' of op '{}' ({}) must be const at compile time."
                    raise ValueError(msg.format(in_name, op_name, op_cls.__name__))

            elif in_name in kwargs:
                # Provided value is not Var. Create a Var from kwargs[in_name]
                val = kwargs[in_name]
                # create Var for numpy / python primitive
                if isinstance(in_type, InternalInputType):
                    # Shove all internal inputs to InternalVar (unknown type).
                    var = InternalVar(val, name=new_var_name)
                    curr_block().add_internal_var(var)
                else:
                    if isinstance(in_type, TupleInputType):
                        var = []
                        for i, v in enumerate(val):
                            if isinstance(v, Var):
                                var.append(v)
                                continue
                            var.append(
                                cls._add_const(v, new_var_name + str(i), before_op)
                            )
                    elif isinstance(in_type, ScalarOrTensorInputType):
                        var = cls._add_const(val, new_var_name, before_op)
                    else:
                        msg = "Cannot convert input '{}' of type {} to Var (op: {})"
                        raise ValueError(
                            msg.format(in_name, type(in_type).__name__, op_name)
                        )
                update_dict[in_name] = var

            elif in_name not in kwargs and in_type.default is not None:
                if isinstance(in_type, PyFunctionInputType):
                    msg = "Default value is not allowed for PyFunctionInputType"
                    raise ValueError(msg)
                # Create a Var from the default value.
                if is_internal_input(in_name):
                    var = InternalVar(in_type.default, name=new_var_name)
                    curr_block().add_internal_var(var)
                elif isinstance(in_type, TupleInputType):
                    var = tuple(
                        cls._add_const(v, new_var_name + str(i), before_op)
                        for i, v in enumerate(in_type.default)
                    )
                else:
                    var = cls._add_const(in_type.default, new_var_name, before_op)
                update_dict[in_name] = var

        kwargs.update(update_dict)

        return kwargs

    @classmethod
    def _add_op(cls, op_cls, **kwargs):
        """
        Add an op of type `op_cls` (e.g., convolution) to current block.
        """
        kwargs = cls._maybe_set_name(kwargs, op_cls.__name__)
        logging.info(
            "Adding op '{}' of type {}".format(kwargs["name"], op_cls.__name__)
        )
        before_op = kwargs.get("before_op", None)
        kwargs = {k: v for k, v in kwargs.items() if v is not None}
        kwargs = cls._create_input_vars(
            op_cls.input_spec, kwargs["name"], op_cls, before_op, kwargs
        )
        new_op = op_cls(**kwargs)
        curr_block()._insert_op_before(new_op, before_op=before_op)
        new_op.build_nested_blocks()
        new_op.type_value_inference()
        if len(new_op.outputs) == 1:
            return new_op.outputs[0]
        return new_op.outputs

    @staticmethod
    def placeholder(shape, dtype=None):
        return Placeholder(shape, dtype)

    @staticmethod
    def TensorSpec(shape, dtype=None):
        return Placeholder(shape, dtype)

    @staticmethod
    def program(input_specs=None):
        """
        Usage:

        @mb.program(input_specs=[mb.TensorSpec(shape=(1,2))])
        def prog(a):
            return mb.add(x=a, y=2)
        """
        if input_specs is None:
            input_specs = []

        def wrapper(main_block):
            program = Program()
            num_args = main_block.__code__.co_argcount
            arg_names = list(main_block.__code__.co_varnames)[:num_args]
            if len(input_specs) != num_args:
                msg = "{} expects {} inputs: {}. Got {} input_specs."
                raise ValueError(
                    msg.format(
                        main_block.__name__, num_args, arg_names, len(input_specs)
                    )
                )
            input_spec_dict = {k: v for k, v in zip(arg_names, input_specs)}
            with Function(input_spec_dict) as func:
                input_vars = [func.inputs[a] for a in arg_names]
                outputs = main_block(*input_vars)
                if isinstance(outputs, tuple):
                    outputs = list(outputs)
                elif not isinstance(outputs, list):
                    outputs = [outputs]
                func.set_outputs(outputs)
                program.add_function("main", func)
            return program

        return wrapper


"""importing ops triggers installation of all ops into Builder"""
from .ops import defs as _ops

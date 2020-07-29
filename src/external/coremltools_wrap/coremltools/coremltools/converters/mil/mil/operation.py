#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import logging
import numpy as np
import six
from coremltools.converters.mil.mil import types
from coremltools.converters.mil.mil.types.symbolic import is_symbolic, any_symbolic
from . import SPACES
from .block import curr_block, _check_is_compatible_type
from .input_type import TupleInputType
from .var import Var, InternalVar, ListVar

VALUE = 1
SYMBOL = 2
NONE = 4
ALL = 7


def _is_compatible_symbolic_array(a, b):
    """
    A helper function which check if two numpy array with symbolic value.
    For instance, a = np.array([is0, is2])
                  b = np.array([is1, 1])
    are considered compatible.
                  a = np.array([is0, 1])
                  b = np.array([is1, -1])
    are not.
    """
    if not a.shape == b.shape:
        return False
    a = a.flatten()
    b = b.flatten()
    for t, v in zip(a, b):
        if not is_symbolic(t) and not is_symbolic(v):
            if t != v:
                return False
    return True


def precondition(allow=ALL):
    """
    A helper decorator for value_inference method.
    Decorate value_inference with parameter VALUE/SYMBOL/NONE or ALL.
    For VALUE/SYMBOL/NONE use logical or ( | ) for multiple allowance.
    Note that:
        1. ALL == VALUE | SYMBOL | NONE
        2. Chosen flag (some or all VALUE/SYMBOL/NONE) must be satisfied
           by EVERY INPUTS for the precondition to be satisfied.

    The meaning for each flag is:
    VALUE: value that can be materialized during compile time
    SYMBOL: value that cannot be materialized by exist as a symbol value
    NONE: a None value

    Usage:
    @precondition(allow=VALUE|SYMBOL)
    def value_inference(self):
        '''some value_inference implementation'''
    """
    ALLOW_VALUE = allow & VALUE
    ALLOW_SYMBOL = allow & SYMBOL
    ALLOW_NONE = allow & NONE

    def process(v, has_value, has_symbol, has_none):
        """
        v: Var

        Return updated has_value, has_symbol, has_none
        """
        if any_symbolic(v.sym_val):
            return has_value, True, has_none
        elif v.val is None:
            return has_value, has_symbol, True
        return True, has_symbol, has_none

    def decorator(func):
        def wrapper(self):
            HAS_VALUE = False
            HAS_SYMBOL = False
            HAS_NONE = False
            for in_name, in_type in self._input_types.items():
                if in_type.optional:
                    # Optional inputs are not required to invoke value_inference()
                    continue

                if isinstance(in_type, TupleInputType):
                    for v in self._input_vars[in_name]:
                        HAS_VALUE, HAS_SYMBOL, HAS_NONE = process(
                            v, HAS_VALUE, HAS_SYMBOL, HAS_NONE
                        )
                else:
                    HAS_VALUE, HAS_SYMBOL, HAS_NONE = process(
                        self._input_vars[in_name], HAS_VALUE, HAS_SYMBOL, HAS_NONE
                    )

            if HAS_VALUE and not ALLOW_VALUE:
                msg = "Implementation of value_inference() for op {} doesn't support input with VALUE"
                raise NotImplementedError(msg.format(self.op_type))
            elif HAS_SYMBOL and not ALLOW_SYMBOL:
                msg = "Implementation of value_inference() for op {} doesn't support input with SYMBOL"
                raise NotImplementedError(msg.format(self.op_type))
            elif HAS_NONE and not ALLOW_NONE:
                msg = "Implementation of value_inference() for op {} doesn't support input with NONE"
                raise NotImplementedError(msg.format(self.op_type))
            else:
                return func(self)

        return wrapper

    return decorator


def is_internal_input(arg_name):
    return arg_name[0] == "_"


class Operation(object):
    """
    Represents Operation in MIL.

    # Properties
    name (str):
        The name of the operation

    input_types (InputSpec, class attr):
        Read-only named input types from all subclasses. Input types are used
        to validate `inputs`.

    inputs [_input_vars] (dict of str --> Var):
        An Operation (subclass of Operation) only has access to input Var,
        which is already validated against `input_spec`.

    outputs [_output_vars] (list of Var):
        List of output var based on type inference. Read-only
    """

    def __init__(self, **kwargs):
        self._input_types = self.input_spec.input_types
        self.name = kwargs.get("name", None)

        self._output_vars = None
        self._input_vars = {}
        self.blocks = []
        self.enclosing_block = curr_block()
        self._validate_and_set_inputs(**kwargs)

    def set_inputs(self, **kwargs):
        self._validate_and_set_inputs(**kwargs)
        if not kwargs.get("no_check_var_types", False):
            self.type_value_inference()

    def get_flattened_inputs(self):
        """
        Returns:
        list[Var]. Flatten all tuple inputs
        """
        flat_inputs = []
        for v in self.inputs.values():
            if isinstance(v, (list, tuple)):
                flat_inputs.extend(v)
            else:
                flat_inputs.append(v)
        return flat_inputs

    def type_value_inference(self, overwrite_output=False):
        """
        Perform type inference and auto_val computation based on new input Vars
        in kwargs. If self._output_vars is None then we generate _output_vars;
        otherwise no new Var is created, but type inference result is verified
        against existing _output_vars, if overwrite_output is False.

        If overwrite_output is True, then the type inference result overwrites the
        existing _output_vars
        """
        output_types = self.type_inference()
        if not isinstance(output_types, tuple):
            output_types = (output_types,)
        output_vals = self._auto_val(output_types)
        try:
            output_names = self.output_names()
            if not isinstance(output_names, tuple):
                output_names = (output_names,)
        except NotImplementedError as e:
            if len(output_types) > 1:
                output_names = tuple(str(i) for i, _ in enumerate(output_types))
            else:
                output_names = ("",)  # output name same as op name.

        # Combine (output_names, output_types, output_vals) to create output
        # Vars.
        if self._output_vars is None:
            self._output_vars = []
            for i, (n, sym_type, sym_val) in enumerate(
                zip(output_names, output_types, output_vals)
            ):
                name = self.name + ":" + n if n != "" else self.name
                if types.is_list(sym_type):
                    new_var = ListVar(
                        name,
                        elem_type=sym_type.T[0],
                        init_length=sym_type.T[1],
                        dynamic_length=sym_type.T[2],
                        op=self,
                        op_output_idx=i,
                    )
                else:
                    new_var = Var(name, sym_type, sym_val, op=self, op_output_idx=i)
                self._output_vars.append(new_var)
        else:
            # Check new inference result against existing self._output_vars.
            for i, (n, sym_type, sym_val) in enumerate(
                zip(output_names, output_types, output_vals)
            ):
                out_var = self._output_vars[i]
                # Check type inference
                if overwrite_output:
                    out_var._sym_type = sym_type
                elif not _check_is_compatible_type(sym_type, out_var.sym_type):
                    msg = "Output Var {} in op {} type changes with new input Vars"
                    raise ValueError(msg.format(out_var.name, self.name))

                # Check value inference
                if overwrite_output:
                    out_var._sym_val = sym_val

                if sym_val is not None and out_var.sym_val is not None:
                    if np.any(sym_val.val != out_var.sym_val):
                        if overwrite_output:
                            out_var._sym_val = sym_val
                        else:
                            msg = 'value_inference differs for var {} in op {}'
                            if not _is_compatible_symbolic_array(sym_val.val, out_var.sym_val):
                                raise ValueError(msg.format(out_var.name, self.name))

    def _auto_val(self, output_types):
        """
        # Evaluation is two stage:
        #
        # Stage 1: Check whether the method value_inference() is implemented
        #
        # Stage 2: Check if there's an value_inference() implementation
        #          for given input types.
        #
        # Suppose input are all SYMBOL:
        # Case 1: No value_inference() implemented => fail at stage 1
        # Case 2: If value_inference() implemented, but requires all VALUE not
        #         SYMBOL => fail at stage 2
        # Case 3: If value_inference() implemented, and has no restriction on
        #         input types => Success
        #
        # If either stage fails, outputs[i].val is None.
        # Otherwise, output[i].sym_val is not None.

        output_types: tuple of builtin types

        Returns:
            output_vals: tuple of builtin type with value, or tuple of None
        """
        do_auto_val = True

        if do_auto_val:
            # Is self.value_inference implemented for corresponding input?
            try:
                vals = self.value_inference()
            except NotImplementedError as e:
                do_auto_val = False

        if not do_auto_val:
            # No auto_val possible.
            return tuple(None for _ in output_types)

        if not isinstance(vals, (tuple, list)):
            vals = (vals,)
        for val in vals:
            if val is None:
                do_auto_val = False
        if not do_auto_val:
            # No auto_val possible.
            return tuple(None for _ in output_types)

        auto_val = []
        for t, v in zip(output_types, vals):
            builtin_val = t()
            builtin_val.val = v
            auto_val.append(builtin_val)
        return auto_val

    def value_inference(self):
        """
        Optional Python implementation of the op based on (materialized) values
        in `self.input_var`. Return a builtin value (single output) or a tuple of
        builtin values (multi-outputs) of the same length as returned by `
        type_inference`
        """
        msg = "value_inference() is not implemented by op {}"
        raise NotImplementedError(msg.format(self.op_type))

    def output_names(self):
        """
        Optional. If implemented, we set the output var i name as
        self.name + "/" + output_names[i]

        Returns a string (single output) or tuple of strings
        """
        msg = "output_names() is not implemented by op {}"
        raise NotImplementedError(msg.format(self.op_type))

    def type_inference(self):
        """
        Return (builtin_type, builtin_val) pair from type inference.
        builtin_val may be None if symbolic_value is not attainable at compile
        time.
        """
        raise NotImplementedError("This function must be implemented by each op")

    def build_nested_blocks(self):
        """
        Build nested blocks (for cond and while_loop and other composite
        blocks)
        """
        pass

    def _validate_and_set_inputs(self, **kwargs):
        non_attributes = [
            "name",
            "symbolic_datatype",
            "datatype",
            "symbolic_value",
            "value",
            "version",
            "before_op",
            "no_check_var_visibility",  # no_check_var_visibility==True to deviate from SSA
            "no_check_var_types",  # no_check_var_types==True to force set inputs, even if type does not match with earlier ones
        ]
        op_inputs = list(self._input_types.keys())
        legal_args = op_inputs + non_attributes
        no_check_var_visibility = kwargs.get("no_check_var_visibility", False)
        no_check_var_types = kwargs.get("no_check_var_types", False)

        for key in kwargs.keys():
            if key not in legal_args:
                raise RuntimeError(
                    "Unknown input '{}' for op '{}'".format(key, self.op_type)
                )

        def check_and_detach(v_new, v_old, op, no_check_var_types):
            # Check new var's sym_type is compatible with the
            # existing's sym_type.
            if (
                not _check_is_compatible_type(v_new.sym_type, v_old.sym_type)
                and not no_check_var_types
            ):
                msg = "New var type {} not a subtype of " + "existing var type {}"
                raise ValueError(msg.format(v_new.sym_type, v_old.sym_type))
            v_old.remove_child_op(op, no_check_var_types)

        parsed_inputs = self.input_spec.parse_inputs(kwargs)
        for (name, var) in parsed_inputs:
            setattr(self, name, var)
            if var is not None and not isinstance(var, InternalVar):
                # Remove this operation itself from existing input Var's child_ops
                existing_input_var = self._input_vars.get(name, None)
                if existing_input_var is not None:
                    if isinstance(existing_input_var, (list, tuple)):
                        for v_old, v_new in zip(existing_input_var, var):
                            check_and_detach(v_new, v_old, self, no_check_var_types)
                    else:
                        check_and_detach(
                            var, existing_input_var, self, no_check_var_types
                        )

                # Set var as input_var
                if isinstance(var, Var):
                    var.add_child_op(self)
                elif isinstance(var, (tuple, list)):
                    for v in var:
                        v.add_child_op(self)
                # ignore function inputs
                self._input_vars[name] = var

    @property
    def inputs(self):
        return self._input_vars

    @property
    def outputs(self):
        return self._output_vars

    @property
    def op_type(self):
        return type(self).__name__

    def remove_from_block(self):
        """
        Remove / detach itself from the enclosing block. See Block.remove_ops
        for details.
        """
        self.enclosing_block.remove_ops([self])

    @staticmethod
    def var_to_str(v):
        if isinstance(v, (tuple, list)):
            return "(" + ", ".join(["%" + s.name for s in v]) + ")"
        else:
            return "%" + v.name

    def indented_str(self, indent=""):
        s = indent
        if self.outputs is not None:
            s += ", ".join([str(o) for o in self.outputs])
        s += " = " + self.op_type + "("
        if self.op_type == "const":
            if self.mode.val == "immediate_value":
                if isinstance(self.val.sym_val, (np.generic, np.ndarray)):
                    val_str = str(self.val.sym_val.tolist())
                else:
                    val_str = (
                        '"' + self.val.sym_val + '"'
                        if isinstance(self.val.sym_val, six.string_types)
                        else str(self.val.sym_val)
                    )
                s += "val=" + val_str
            else:
                s += "val=(file_value)"
        else:
            s += ", ".join(
                [
                    k + "=" + Operation.var_to_str(self.inputs[k])
                    for k in self._input_types.keys()
                    if k in self.inputs and not is_internal_input(k)
                ]
            )
        s += ', name="{}")\n'.format(self.name)
        for b in self.blocks:
            s += b.indented_str(indent=indent + SPACES)
        return s

    def __repr__(self):
        return str(self)

    def __str__(self):
        return self.indented_str(SPACES)

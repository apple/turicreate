#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import six
from coremltools.converters.mil.mil import types
from coremltools.converters.mil.mil.types.type_mapping import (
    numpy_val_to_builtin_val,
    is_subtype,
)
import copy
from coremltools.converters.mil.mil import Block, SYMBOL, NONE
from coremltools.converters.mil.mil.var import Var
from coremltools.converters.mil.mil import get_new_symbol
from ._op_reqs import *
import logging


@register_op(doc_str="")
class cond(Operation):
    """
    Conditional execution. The return types must be identical between the true
    and false branches.

    Parameters
    ----------
    pred: tensor<[], bool> (Required)
        * 0D tensor (scalar) predicate to switch between true and fall branches.

    _true_fn: Python function (Required)
	* A Python function that will be executed if ``cond`` evaluates to ``True``. It must take 0 input and return one or more values, whose types will be taken to be the return type of the operation.

    _false_fn: Python function (Required)
	* A Python function to be executed if ``cond`` evaluates to ``False``. It must take 0 input and has return types must match those of if_branch.

    Returns
    -------
    Python tuple
        * Tuple of ``Variables`` from one of the branches.
    """

    input_spec = InputSpec(
        pred=BoolInputType(),
        _true_fn=PyFunctionInputType(),
        _false_fn=PyFunctionInputType(),
    )

    def __init__(self, **kwargs):
        super(cond, self).__init__(**kwargs)

    def build_nested_blocks(self):
        # Cond block
        true_block_name = self.name + "_true"
        with Block(name=true_block_name, outer_op=self) as true_block:
            true_func = self._true_fn.val
            true_ret_vars = true_func()
            if isinstance(true_ret_vars, tuple):
                true_ret_vars = list(true_ret_vars)
            if not isinstance(true_ret_vars, list):
                true_ret_vars = [true_ret_vars]
            true_block.set_outputs(true_ret_vars)
            self.blocks.append(true_block)

        false_block_name = self.name + "_false"
        with Block(name=false_block_name, outer_op=self) as false_block:
            false_func = self._false_fn.val
            false_ret_vars = false_func()
            if isinstance(false_ret_vars, tuple):
                false_ret_vars = list(false_ret_vars)
            if not isinstance(false_ret_vars, list):
                false_ret_vars = [false_ret_vars]
            false_block.set_outputs(false_ret_vars)
            self.blocks.append(false_block)

    def type_inference(self):
        true_ret_vars = self.blocks[0].outputs
        false_ret_vars = self.blocks[1].outputs
        # Verify true_ret_vars has the same types as false_ret_vars
        for i, (vt, vf) in enumerate(zip(true_ret_vars, false_ret_vars)):
            if vt.sym_type != vf.sym_type:
                msg = (
                    "true branch output {} type {} mismatch false branch"
                    + " output type {}"
                )
                raise ValueError(msg.format(vt.name, vt.sym_type, vf.sym_type))

        return tuple(v.sym_type for v in true_ret_vars)


@register_op(doc_str="")
class const(Operation):
    input_spec = InputSpec(
        mode=InternalStringInputType(const=True, default="immediate_value"),
        val=InternalScalarOrTensorInputType(const=True),
    )

    def __init__(self, **kwargs):
        super(const, self).__init__(**kwargs)

    def type_inference(self):
        builtin_type, _ = self._get_type_val(self.val.val)
        return builtin_type

    def value_inference(self):
        _, val = self._get_type_val(self.val.val)
        return val

    def _get_type_val(self, value):

        if isinstance(value, (float, np.float64)):
            value = np.float32(value)
        elif isinstance(value, bool):
            value = np.bool(value)
        elif isinstance(value, (six.integer_types, np.int64)):
            value = np.int32(value)
        elif isinstance(value, (tuple, list, np.ndarray)):
            value = np.array(value)
            if value.dtype == np.int64:
                # We use int32 by default.
                value = value.astype(np.int32)

            if value.dtype == np.float64:
                # We use float32 by default.
                value = value.astype(np.float32)

        if not isinstance(value, (np.generic, np.ndarray, six.string_types, bool)):
            raise ValueError("Unknown value for constant: {}".format(value))

        _, builtin_type = numpy_val_to_builtin_val(value)
        return builtin_type, value


# Internal const can have symbolic value (for testing purpose)
@register_op(doc_str="")
class _const_symbolic(const):
    def __init__(self, **kwargs):
        super(_const_symbolic, self).__init__(**kwargs)

    def type_inference(self):
        builtin_type, _ = self._get_type_val(self.val.sym_val)
        return builtin_type

    def value_inference(self):
        # We allow symbolic values in _const_symbolic
        _, val = self._get_type_val(self.val.sym_val)
        return val


@register_op(doc_str="")
class select(Operation):
    """
    Returns the elements selected from either ``a`` or ``b``, depending on
    ``cond``. Shape of ``cond``, ``a``, ``b`` must be broadcastable.

    ``a, b`` must be provided together, or neither is provided. If neither is
    provided, returns the indices of ``cond`` that are ``True``.

    Parameters
    ----------
    cond: tensor<[*D1], T> (Required)
        * Tensor, when True (non-zero), select element from x, otherwise, y

    a: tensor<[*D2], T> (Optional. Default to None)
        * Values selected at indices where ``cond`` is True

    b: tensor<[*D3], T> (Optional. Default to None)
        * Values selected at indices where ``cond`` is False

    Returns
    -------
    tensor<[*D_out], T> or tensor<[n, len(D1)], int32>
        *  If ``a, b`` are both provided, return shape is based on broadcast rules from ``cond, a, b``. If ``a, b`` are ``None``, returns shape is 2D, where first dimension ``n`` is the number of matching indices in ``cond`` and ``len(D1)`` is the rank of ``cond``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        cond=TensorInputType(), a=TensorInputType(), b=TensorInputType()
    )

    def __init__(self, **kwargs):
        super(select, self).__init__(**kwargs)

    def type_inference(self):
        a_type = self.a.sym_type
        b_type = self.b.sym_type
        if all([a_type, b_type]):
            compatible, ret_type = types.is_tensor_and_is_compatible_general_shape(
                a_type, b_type
            )
            if compatible:
                return ret_type
            elif a_type == b_type:
                return a_type
            else:
                raise ValueError("Type mismatch {} vs. {}".format(a_type, b_type))
        return a_type if a_type is not None else b_type

    @precondition(allow=VALUE)
    def value_inference(self):
        return np.where(self.cond.val, self.a.val, self.b.val)


@register_op(doc_str="")
class while_loop(Operation):
    """
    Perform body repeatly while the condition cond is true.

    Parameters
    ----------
    _cond: Python function  (Required)
	* A Python function that takes ``loop_vars`` as positional arguments. The function must return a bool Var.

    _body: Python function  (Required)
	* A Python function that takes ``loop_vars`` as positional arguments. The function must return the same number of output vars as ``loop_var`` with the same types.

    loop_vars: Python tuple (Required)
	* Python tuple of ``Variables``.

    Returns
    -------
    Python tuple
        * Same type as ``loop_vars``
    """

    input_spec = InputSpec(
        # arg name with underscore prefix won't be printed.
        _cond=PyFunctionInputType(),
        _body=PyFunctionInputType(),
        loop_vars=TupleInputType(),
    )

    def __init__(self, **kwargs):
        super(while_loop, self).__init__(**kwargs)

    @staticmethod
    def _check_is_compatible_type(type1, type2):
        if not types.is_subtype(type1, type2):
            is_comp, _ = types.is_tensor_and_is_compatible(type1, type2)
            return is_comp
        return True

    @staticmethod
    def _check_equal_value(val1, val2):
        if val1 is None and val2 is None:
            return True
        if val1 is None or val2 is None:
            return False
        if isinstance(val1, np.ndarray) and isinstance(val2, np.ndarray):
            return np.array_equal(val1, val2)
        return val1 == val2

    @staticmethod
    def clean_up_child_ops(block):
        for op in list(block.operations):

            for b in op.blocks:
                while_loop.clean_up_child_ops(b)

            inputs = op.get_flattened_inputs()
            for in_var in inputs:
                in_var.remove_child_op(op)

    def build_block(self, block_inputs):
        block_name = self.name + '_block'
        with Block(block_inputs=block_inputs, outer_op=self,
                name=block_name) as block:
            # Body func
            body_func = self._body.val
            exit_vars = body_func(*block.inputs)

            # Cond func:
            cond_func = self._cond.val
            cond_var = cond_func(*block.inputs)
            cond_vars = cond_var if isinstance(cond_var, list) else [cond_var]

            # Concatenate the outputs
            block.set_outputs(cond_vars + list(exit_vars))
        return block, exit_vars

    def build_nested_blocks(self):
        # self.loop_vars is python tuple of Vars.

        # block_inputs Var are not produced by any op.
        # We assume block_inputs have the same types as self.loop_var. If not
        # (e.g., when certain dimensions change shape during iterate), we'd
        # adjust later.

        # We assume that sym_val is unchanging across the block iterate. If it
        # changes, we rebuild the block and rerun type and value inference.

        block_inputs = tuple(copy.copy(v) for v in self.loop_vars)
        for v in block_inputs:
            v._op = None
            v.op_output_idx = None
            v._child_ops = list()
            v.name = v.name + ".x"
            v._sym_val = v._sym_val
            v.consuming_blocks = list()

        block, exit_vars = self.build_block(block_inputs)

        # Verify exit_vars has the same types as loop_vars
        block_input_type_change = False
        for i, (v_in, v_out) in enumerate(zip(block_inputs, exit_vars)):
            if not is_subtype(v_out.sym_type, v_in.sym_type):
                compat_shape = while_loop.get_compat_shape(v_out.sym_type,
                        v_in.sym_type)
                if compat_shape is None:
                    msg = "loop_vars '{}' changes in the body of " \
                          "while_loop '{}':\n {} -> {}"
                    raise ValueError(msg.format(
                        v_in.name, self.name,
                        v_in.sym_type, v_out.sym_type))
                else:
                    block_inputs[i]._sym_type = types.tensor(
                            v_in.dtype, compat_shape)
                    block_input_type_change = True
            if not while_loop._check_equal_value(v_out.sym_val, v_in.sym_val):
                block_inputs[i]._sym_val = None
                block_input_type_change = True

        if block_input_type_change:
            # Since we are going to build the block again, we first need to remove ops
            # in the block from vars's _child_ops.
            while_loop.clean_up_child_ops(block)

            # Rebuild our block to invoke type inference.
            block, exit_vars = self.build_block(block_inputs)
            for i, (v_in, v_out) in enumerate(zip(block_inputs, exit_vars)):
                if not is_subtype(v_out.sym_type, v_in.sym_type):
                    msg = 'Block output {}: {} is not a subtype of ' +\
                            'block input {}: {} after factoring shape changes'
                    raise ValueError(msg.format(v_out.name. v.sym_type,
                        v_in.name, v_in.sym_type))
                if not while_loop._check_equal_value(v_out.sym_val, v_in.sym_val):
                    msg = 'Block output {}: {} is not equal to ' +\
                            'block input {}: {} after value changes'
                    raise ValueError(msg.format(v_out.name. v.sym_val,
                        v_in.name, v_in.sym_val))
        self.blocks.append(block)

    @staticmethod
    def get_compat_shape(type1, type2):
        """
        For tensor types `type1`, `type2` that are of the same rank, return
        compat_shape (python list) where compat_shape[i] is integer iff type1
        and type2 have the same integer shape on dim i. compat_shape[i] is
        symbolic otherwise.

        Return None if `type1`, `type2` have different rank or non-tensor
        type.
        """
        if not types.is_tensor(type1) or not types.is_tensor(type2):
            return None

        s1 = type1.get_shape()
        s2 = type2.get_shape()

        if len(s1) != len(s2):
            return None

        compat_shape = []
        for d1, d2 in zip(s1, s2):
            if d1 != d2:
                compat_shape.append(get_new_symbol())
            else:
                compat_shape.append(d1)
        return compat_shape

    def type_inference(self):
        # Skip the conditional var
        return tuple(v.sym_type for v in self.blocks[0].outputs[1:])



# identity is used for renaming and is rarely necessary. See
# `loop_invariant_elimination` pass for a rare use case.
@register_op(doc_str="")
class identity(Operation):
    input_spec = InputSpec(x=ListOrScalarOrTensorInputType())

    def __init__(self, **kwargs):
        super(identity, self).__init__(**kwargs)

    def type_inference(self):
        return self.x.sym_type

    @precondition(allow=VALUE | SYMBOL)
    def value_inference(self):
        return self.x.sym_val


@register_op(doc_str="")
class make_list(Operation):
    input_spec = InputSpec(
        init_length=IntInputType(optional=True, default=1),
        dynamic_length=BoolInputType(optional=True, default=True),
        elem_shape=TensorInputType(const=True),
        dtype=StringInputType(const=True, optional=True, default="fp32"),
    )

    def __init__(self, **kwargs):
        super(make_list, self).__init__(**kwargs)

    def type_inference(self):
        builtin_dtype = types.string_to_builtin(self.dtype.val)
        if builtin_dtype is None:
            raise ValueError("Unsupported dtype {}".format(self.dtype.val))
        elem_type = types.tensor(builtin_dtype, self.elem_shape.sym_val)
        return types.list(
            elem_type,
            init_length=self.init_length.val,
            dynamic_length=self.dynamic_length.val,
        )


@register_op(doc_str="")
class list_length(Operation):
    input_spec = InputSpec(ls=ListInputType(),)

    def __init__(self, **kwargs):
        super(list_length, self).__init__(**kwargs)

    def type_inference(self):
        return types.int32

    @precondition(allow=VALUE | SYMBOL | NONE)
    def value_inference(self):
        if not self.ls.dynamic_length:
            return self.ls.init_length
        raise NotImplementedError()


@register_op(doc_str="")
class list_write(Operation):
    input_spec = InputSpec(
        ls=ListInputType(), index=IntInputType(), value=TensorInputType(),
    )

    def __init__(self, **kwargs):
        super(list_write, self).__init__(**kwargs)

    def type_inference(self):
        list_elem_type = self.ls.elem_type
        value_type = self.value.sym_type
        dynamic_length = self.ls.dynamic_length
        init_length = self.ls.init_length

        if list_elem_type is None:
            # fill in the elem type using value's type info.
            return types.list(
                value_type, init_length=init_length, dynamic_length=dynamic_length
            )
        if list_elem_type == types.unknown:
            msg = "Input ls elem type unknown. Override with {}"
            logging.warning(msg.format(value_type))
            return types.list(
                value_type, init_length=init_length, dynamic_length=dynamic_length
            )
        if not types.is_subtype(value_type, list_elem_type):
            msg = "Elem type mismatch: ls elem type {} vs " + "value type {}"
            raise ValueError(msg.format(list_elem_type, value_type))
        return self.ls.sym_type


@register_op(doc_str="")
class list_read(Operation):
    input_spec = InputSpec(ls=ListInputType(), index=IntInputType(),)

    def __init__(self, **kwargs):
        super(list_read, self).__init__(**kwargs)

    def type_inference(self):
        list_elem_type = self.ls.elem_type
        if list_elem_type is None:
            msg = (
                "Unknown element type. The List might not have been "
                + "written to ({})"
            )
            raise ValueError(msg.format(self.name))
        return list_elem_type


@register_op(doc_str="")
class list_gather(Operation):
    input_spec = InputSpec(ls=ListInputType(), indices=IntTensorInputType(),)

    def __init__(self, **kwargs):
        super(list_gather, self).__init__(**kwargs)

    def type_inference(self):
        list_elem_type = self.ls.elem_type
        if list_elem_type == types.unknown:
            msg = (
                "Unknown element type. The List might not have been "
                + "written to ({})"
            )
            raise ValueError(msg.format(self.name))
        elem_shape = list_elem_type.get_shape()
        dtype = list_elem_type.get_primitive()
        ret_shape = [self.indices.shape[0]] + list(elem_shape)
        return types.tensor(dtype, tuple(ret_shape))


@register_op(doc_str="")
class list_scatter(Operation):
    input_spec = InputSpec(
        ls=ListInputType(), indices=IntTensorInputType(), value=TensorInputType(),
    )

    def __init__(self, **kwargs):
        super(list_scatter, self).__init__(**kwargs)

    def type_inference(self):
        num_indices = self.indices.shape[0]
        num_values = self.value.shape[0]
        if num_values != num_indices:
            raise ValueError(
                "Cannot scatter {} values to {} indices".format(num_values, num_indices)
            )
        list_elem_type = self.ls.elem_type
        value_type = self.value.sym_type
        dynamic_length = self.ls.dynamic_length
        init_length = self.ls.init_length

        elem_type = types.tensor(value_type.get_primitive(), value_type.get_shape()[1:])
        if list_elem_type == types.unknown:
            # fill in the elem type using value's type info.
            return types.list(
                elem_type, dynamic_length=dynamic_length, init_length=init_length
            )
        if not types.is_subtype(elem_type, list_elem_type):
            msg = "Elem type mismatch: ls elem type {} vs " + "value type {}"
            raise ValueError(msg.format(list_elem_type, elem_type))
        return self.ls.sym_type

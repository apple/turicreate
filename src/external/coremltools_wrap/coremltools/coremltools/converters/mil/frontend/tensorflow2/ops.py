#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import numpy as _np

from coremltools.converters.mil.frontend.tensorflow.ops import (
    _transpose_NHWC_to_NCHW,
    _transpose_NCHW_to_NHWC,
)

# TF 2.x now imports and registers all TF 1.x op against the new registry
# (separated from TF 1.x registry). Overwrite might needed in case the op
# semantics are different between TF 1.x and TF 2.x.<
from coremltools.converters.mil.frontend.tensorflow.ops import *
from coremltools.converters.mil.frontend.tensorflow.dialect_ops import *


@register_tf_op(override=True)
def FusedBatchNormV3(context, node):
    # Get attributes
    data_format = node.attr.get("data_format", "NHWC")
    epsilon = node.attr.get("epsilon", None)

    # Get inputs
    x = context[node.inputs[0]]
    scale = context[node.inputs[1]]
    offset = context[node.inputs[2]]
    mean = context[node.inputs[3]]
    variance = context[node.inputs[4]]
    if data_format == "NHWC":
        # TF's FusedBatchNorm is only for 4D inputs
        x = _transpose_NHWC_to_NCHW(x)
        x = mb.batch_norm(
            x=x, mean=mean, variance=variance, gamma=scale, beta=offset, epsilon=epsilon
        )
        x = _transpose_NCHW_to_NHWC(x, node.name)
    else:
        x = mb.batch_norm(
            x=x,
            mean=mean,
            variance=variance,
            gamma=scale,
            beta=offset,
            epsilon=epsilon,
            name=node.name,
        )
    # Inference only batch norm does not have meaningful outputs for
    # batch_mean, batch_variance etc.
    context.add(node.name, x)


@register_tf_op(tf_alias=["If"], override=True)
def StatelessIf(context, node):
    pred = context[node.inputs[0]][0]
    then_graph = context.get_graph(node.attr.get("then_branch"))
    else_graph = context.get_graph(node.attr.get("else_branch"))

    def then_fn():
        context.stack_func_inputs(context[node.inputs[0]])
        then_output_var = convert_graph(context, then_graph)
        context.unstack_func_inputs()
        return then_output_var

    def else_fn():
        context.stack_func_inputs(context[node.inputs[0]])
        else_output_var = convert_graph(context, else_graph)
        context.unstack_func_inputs()
        return else_output_var

    x = mb.cond(pred=pred, _true_fn=then_fn, _false_fn=else_fn, name=node.name)

    # wraps x as tuple for get_tuple that always follow the cond node.
    x = (x,) if not isinstance(x, (tuple, list)) else x

    context.add(node.name, x)


@register_tf_op(tf_alias=["While"], override=True)
def StatelessWhile(context, node):
    # inputs are loop_counter, max_iterations, [loop_vars]
    loop_vars = context[node.inputs[0]][2:]

    cond_graph = context.get_graph(node.attr.get("cond"))
    body_graph = context.get_graph(node.attr.get("body"))

    def cond(*loop_vars):
        context.stack_func_inputs(loop_vars)
        cond_output_vars = convert_graph(context, cond_graph)
        context.unstack_func_inputs()
        return cond_output_vars

    def body(*loop_vars):
        context.stack_func_inputs(loop_vars)
        body_output_vars = convert_graph(context, body_graph)
        context.unstack_func_inputs()
        return body_output_vars

    x = mb.while_loop(_cond=cond, _body=body, loop_vars=loop_vars, name=node.name)

    # wraps x as tuple for get_tuple that always follow the while node.
    x = (x,) if not isinstance(x, (tuple, list)) else x

    context.add(node.name, x)


@register_tf_op
def TensorListFromTensor(context, node):
    value = context[node.inputs[0]]
    element_shape = context[node.inputs[1]]
    element_dtype = node.attr.get("element_dtype")
    dtype_str = types.builtin_to_string(element_dtype)

    length = mb.shape(x=value)
    length = mb.slice_by_index(x=length, begin=[0], end=[1], squeeze_mask=[True])

    if element_shape is not None and all(_np.atleast_1d(element_shape.val) != -1):
        ls = mb.make_list(init_length=length, elem_shape=element_shape, dtype=dtype_str)
    else:
        ls = mb.tf_make_list(init_length=length, dtype=dtype_str)

    indices = mb.range_1d(end=length, start=0, step=1)
    ls = mb.list_scatter(ls=ls, indices=indices, value=value, name=node.name)
    context.add(node.name, ls)


@register_tf_op
def TensorListGather(context, node):
    ls = context[node.inputs[0]]
    indices = context[node.inputs[1]]
    tensor = mb.list_gather(ls=ls, indices=indices, name=node.name)
    context.add(node.name, tensor)


@register_tf_op
def TensorListGetItem(context, node):
    ls = context[node.inputs[0]]
    index = context[node.inputs[1]]
    new_ls = mb.list_read(ls=ls, index=index, name=node.name)
    context.add(node.name, new_ls)


@register_tf_op
def TensorListLength(context, node):
    ls = context[node.inputs[0]]
    length = mb.list_length(ls=ls, name=node.name)
    context.add(node.name, length)


@register_tf_op
def TensorListResize(context, node):
    # skip here as the list will be dynamically resized when
    # necessary in downstream list_write or list_scatter ops
    Identity(context, node)


@register_tf_op
def TensorListReserve(context, node):
    element_shape = context[node.inputs[0]]
    num_elements = context[node.inputs[1]]
    element_dtype = node.attr.get("element_dtype")
    dtype = types.builtin_to_string(element_dtype)

    if element_shape is not None and all(_np.atleast_1d(element_shape.val) != -1):
        ls = mb.make_list(
            init_length=num_elements,
            elem_shape=element_shape,
            dtype=dtype,
            name=node.name,
        )
    else:
        ls = mb.tf_make_list(init_length=num_elements, dtype=dtype, name=node.name)
    context.add(node.name, ls)


@register_tf_op
def TensorListScatterIntoExistingList(context, node):
    ls = context[node.inputs[0]]
    value = context[node.inputs[1]]
    indices = context[node.inputs[2]]
    ls = mb.list_scatter(ls=ls, indices=indices, value=value, name=node.name)
    context.add(node.name, ls)


@register_tf_op
def TensorListSetItem(context, node):
    ls = context[node.inputs[0]]
    index = context[node.inputs[1]]
    value = context[node.inputs[2]]
    new_ls = mb.list_write(ls=ls, index=index, value=value, name=node.name)
    context.add(node.name, new_ls)


@register_tf_op
def TensorListStack(context, node):
    ls = context[node.inputs[0]]
    length = mb.list_length(ls=ls)
    indices = mb.range_1d(end=length, start=0, step=1)
    x = mb.list_gather(ls=ls, indices=indices, name=node.name)
    context.add(node.name, x)

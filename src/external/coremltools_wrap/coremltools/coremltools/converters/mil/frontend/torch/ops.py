#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import logging as _logging

import math as _math
import numpy as _np
from tqdm import tqdm as _tqdm

from coremltools.converters.mil.mil import types
from coremltools.converters.mil.mil import Builder as mb
from coremltools.converters.mil.mil.var import Var, ListVar
from coremltools.converters.mil.mil import Placeholder, Symbol
from .internal_graph import *
from .torch_op_registry import _TORCH_OPS_REGISTRY, register_torch_op

# The pytorch args for many of the below ops were sourced from
# https://github.com/pytorch/pytorch/blob/d971007c291c0ead1003d12cd553d18ddb582207/torch/csrc/jit/mobile/register_mobile_ops.cpp#L216


# This is a magic number in PyTorch. It's used as a default value in many
# functions.
PYTORCH_MAGIC_DEFAULT = 9223372036854775807


def _all_outputs_present(context, graph):
    """ Returns true if all the symbols in the graph's output list are
        present in context."""
    for outp in graph.outputs:
        try:
            context[outp]
        except ValueError:
            return False
    return True


def convert_nodes(context, graph):
    """Iterate over the nodes of a graph or block and convert to MIL.

        Arguments:
            context: A TranscriptionContext object to pull node inputs and
                assign node outputs.
            graph: An InternalTorchIRGraph or InternalTorchIRBlock object.
    """
    for node in _tqdm(graph.nodes, desc="Converting Frontend ==> MIL Ops", unit=" ops"):
        _add_op = _TORCH_OPS_REGISTRY.get(node.kind, None)
        _logging.info("Converting op {} : {}".format(node.name, node.kind))
        if _add_op is None:
            raise RuntimeError(
                "PyTorch convert function for op '{}' not implemented.".format(node.kind)
            )
        else:
            _add_op(context, node)

        # We've generated all the outputs the graph needs, terminate conversion.
        if _all_outputs_present(context, graph):
            break


def convert_block(context, block, inputs):
    """Convert a block (sub-graph) to MIL. Conversion happens within a new
        context frame.

        Arguments:
            context: A TranscriptionContext object to pull node inputs and
                assign node outputs.
            block: An InternalTorchIRBlock object.
            inputs: List of Vars from the outer context that map to the block's
                expected inputs. The number of inputs provided must match the
                number expected by the block.
    """

    assert len(block.inputs) == len(inputs)

    # Start a new context frame.
    context.push((block.inputs, inputs))

    # Add the block ops.
    convert_nodes(context, block)

    # Collect the block outputs.
    outputs = [context[outp] for outp in block.outputs]

    # Return to the previous context frame.
    context.pop()
    return outputs


# Some ops will receive a dtype input as an integer
# which maps to a torch dtype. The below mapping was found by
# converting test models with different dtypes passed to ones.
NUM_TO_TORCH_DTYPE = {
    0: torch.uint8,
    1: torch.int8,
    2: torch.int16,
    3: torch.int32,
    4: torch.int64,
    5: torch.float16,
    6: torch.float32,
    7: torch.float64,
    11: torch.bool,
}


def decide_immediate_or_file(val):
    if (
        val is not None
        and isinstance(val, (_np.ndarray, _np.generic))
        and val.size >= 10
    ):
        return "file_value"
    return "immediate_value"


def _get_inputs(context, node, expected=None):
    """Look up a node's inputs in @context and return them as a list. If
        @expected is not None, also verifies the number of inputs matches the
        value of @expcted.
    """
    inputs = [context[name] for name in node.inputs]
    if expected is not None and len(inputs) != expected:
        raise ValueError(
            "node {} ({}) got {} input(s), expected {}".format(
                node.name, node.kind, len(inputs), expected
            )
        )
    return inputs


def _list_select(ls, index):
    """ Sometimes we need to select a specific item from a list. If that item
        is known at compile time, extract it as a const. Otherwise, if it's
        symbolic, use gather.
    """
    # TODO: gather doesn't work when the shape is known size.
    if ls.sym_val is not None and not isinstance(ls.sym_val[index], Symbol):
        res = mb.const(val=ls.sym_val[index])
    else:
        res = mb.gather(x=ls, indices=index)
    return res


def _construct_constant(val, name):
    # Converter cannot handle torch tensors.
    if isinstance(val, torch.Tensor):
        val = val.numpy()

    # MIL casts ints to int32, which can't represent the 64 bit magic number.
    # So we instead represent it with None, and any ops that might get the
    # value will check for None instead.
    if isinstance(val, int) and val == PYTORCH_MAGIC_DEFAULT:
        val = None

    mode = decide_immediate_or_file(val)
    if val is None:
        return None
    else:
        return mb.const(mode=mode, val=val, name=name)


@register_torch_op
def constant(context, node):
    assert len(node.inputs) == 0
    assert len(node.outputs) == 1

    name = node.name
    val = node.attr["value"]

    const = _construct_constant(val, name)
    context.add(const, torch_name=name)


def _array_construct(context, node, array_type):
    assert len(node.outputs) == 1
    inputs = _get_inputs(context, node)
    scalar_inputs = [
        inp
        for inp in inputs
        if isinstance(inp, Var) and inp.val is not None and len(inp.shape) == 0
    ]

    if len(scalar_inputs) == len(inputs):
        # All the list items are compile-time scalar constants, so let's create
        # a new const that concatenates them.
        mode = "immediate_value"
        val = array_type([inp.val for inp in inputs])
        const = mb.const(mode=mode, val=val, name=node.name)
        context.add(const)
    else:
        # If at least one input to the construct op is non-const, collect
        # the inputs and add them directly to the context. Ops that use this
        # node's output will take the list directly as input.
        context.add(array_type(inputs), node.name)


@register_torch_op
def tupleconstruct(context, node):
    _array_construct(context, node, array_type=tuple)


@register_torch_op
def listconstruct(context, node):
    _array_construct(context, node, array_type=list)


@register_torch_op
def eq(context, node):
    inputs = _get_inputs(context, node, expected=2)
    equal_to = mb.equal(x=inputs[0], y=inputs[1], name=node.name)
    context.add(equal_to)


@register_torch_op
def ne(context, node):
    inputs = _get_inputs(context, node, expected=2)
    equal_to = mb.not_equal(x=inputs[0], y=inputs[1], name=node.name)
    context.add(equal_to)


@register_torch_op
def le(context, node):
    inputs = _get_inputs(context, node, expected=2)
    less_equal = mb.less_equal(x=inputs[0], y=inputs[1], name=node.name)
    context.add(less_equal)


@register_torch_op
def lt(context, node):
    inputs = _get_inputs(context, node, expected=2)
    less = mb.less(x=inputs[0], y=inputs[1], name=node.name)
    context.add(less)


@register_torch_op
def ge(context, node):
    inputs = _get_inputs(context, node, expected=2)
    greater_equal = mb.greater_equal(x=inputs[0], y=inputs[1], name=node.name)
    context.add(greater_equal)


@register_torch_op
def gt(context, node):
    inputs = _get_inputs(context, node, expected=2)
    greater = mb.greater(x=inputs[0], y=inputs[1], name=node.name)
    context.add(greater)


@register_torch_op(torch_alias=["t", "transpose_"])
def transpose(context, node):
    assert len(node.outputs) == 1
    inputs = _get_inputs(context, node)
    x = inputs[0]

    if len(node.inputs) == 1:
        # PyTorch has several tranpose ops that can be emitted. This one is only
        # emitted when .t() is called on a tensor, which means it can only be
        # called on a matrix.
        if len(x.shape) > 2:
            raise ValueError("transpose without dims for rank > 2 is unsupported")
        res = mb.transpose(x=x, perm=[1, 0], name=node.name)
    else:
        assert len(inputs) == 3
        ax0 = inputs[1].val
        ax1 = inputs[2].val

        perm = list(range(len(x.shape)))
        perm[ax0] = ax1
        perm[ax1] = ax0

        res = mb.transpose(x=x, perm=perm, name=node.name)
    context.add(res)


@register_torch_op
def permute(context, node):
    inputs = _get_inputs(context, node, expected=2)
    perm = mb.transpose(x=inputs[0], perm=inputs[1], name=node.name)
    context.add(perm)

@register_torch_op
def pixel_shuffle(context, node):
    inputs = _get_inputs(context, node, expected=2)
    perm = mb.pixel_shuffle(x=inputs[0], upscale_factor=inputs[1], name=node.name)
    context.add(perm)


@register_torch_op
def matmul(context, node):
    inputs = _get_inputs(context, node, expected=2)
    res = mb.matmul(x=inputs[0], y=inputs[1], name=node.name)
    context.add(res)

@register_torch_op
def add(context, node):
    add_inputs = _get_inputs(context, node)
    assert len(node.outputs) == 1

    # TODO (sberardi): 3rd param to aten::add is a scale factor, need to handle that.
    # out=input+alpha x other
    # rdar://60175736
    if len(add_inputs) > 2 and add_inputs[2].val != 1:
        raise ValueError("ADD does not support scale factor param")

    add_node = mb.add(x=add_inputs[0], y=add_inputs[1], name=node.name)
    context.add(add_node)


@register_torch_op
def addmm(context, node):
    # addmm(Tensor input, Tensor mat1, Tensor mat2, Scalar beta=1, Scalar alpha=1)
    # output = beta * input + alpha * mat1 * mat2

    assert len(node.inputs) == 5
    assert len(node.outputs) == 1

    inputs = [context[name] for name in node.inputs]
    bias = inputs[0]
    mat1 = inputs[1]
    mat2 = inputs[2]
    beta = inputs[3]
    alpha = inputs[4]

    if beta.val != 1.0:
        # Apply scaling factor beta to the bias.
        bias = mb.mul(x=beta, y=bias, name=bias.name + "_scaled")
        context.add(bias)

    if alpha.val != 1.0:
        # Apply scaling factor alpha to the input.
        mat1 = mb.mul(x=alpha, y=mat1, name=mat1.name + "_scaled")
        context.add(mat1)

    # MIL linear will transpose mat2, but addmm expects that mat1 and mat2
    # can multiply as is. So we add a tranpose.
    mat2 = mb.transpose(x=mat2, perm=[1, 0], name=mat2.name + "_transposed")
    context.add(mat2)

    addmm_node = mb.linear(x=mat1, weight=mat2, bias=bias, name=node.name)
    context.add(addmm_node)


@register_torch_op(torch_alias=["conv2d"])
def _convolution(context, node):
    inputs = _get_inputs(context, node)

    x = inputs[0]
    weight = inputs[1]
    bias = inputs[2]
    strides = inputs[3]

    # Expand padding. Torch accepts either an int (for all dimensions) or an n-tuple of ints (one per dimension), but
    # we require a (2 * n)-tuple, where n is the number of spatial dimensions, start and end for each spatial dimension
    pad = inputs[4].val

    if weight.val.ndim in (3, 4):
        # 1D and 2D: Need to explicitly state L-R, T-B pad
        pad = _np.repeat(pad, 2)
    elif weight.val.ndim == 5:
        # 3D: Need to explicitly state F-Bk, L-R, T-B pad
        if type(pad) == int:
            pad = _np.repeat(pad, 6)
        elif len(pad) == 3:
            pad = _np.repeat(pad, 2)
    else:
        raise ValueError(
            "Invalid weight dimension. Must be 3, 4, or 5 for 1D, 2D, or 3D convolution, respectively."
        )

    dilations = inputs[5]
    out_pad = None
    if len(inputs) == 12:
        transposed = inputs[6].val
        out_pad = inputs[7].val
        group = inputs[8]
    elif len(inputs) == 7:
        transposed = False
        group = inputs[6]
    else:
        raise ValueError(
            "unexpected number of inputs for node {} ({}): {}".format(
                node.name, node.kind, len(inputs)
            )
        )

    kwargs = {
        "x": x,
        "strides": strides,
        "pad_type": "custom",
        "pad": pad,
        "dilations": dilations,
        "groups": group,
        "name": node.name,
    }

    # Bias is optional in PyTorch's convolution.
    if bias is not None:
        kwargs["bias"] = bias

    if transposed is True:
        # Transposed convolution

        # PyTorch weight ordering [Cin, Cout, H, W]
        # MIL expects [Cout, Cin, H, W]
        weight_transpose = mb.transpose(
            x=weight, perm=[1, 0, 2, 3], name=weight.name + "_transpose"
        )

        # Handle output_padding using pre-pad or post-crop
        pre_pad = [0] * len(pad)
        post_crop = [0] * len(pad)

        if out_pad is not None and any(out_pad):
            output_padding = [0] * len(pad)
            # output padding adds additional padding on one of the side of dimension
            # i.e. bottom from top-bottom,
            #      right  from left-right
            #      back   from front-back
            # CoreML padding structure is similar [top, bottom, left, right]
            # mapping output_padding to simplify further processing!
            #
            # For ConvTranspose2d: [bottom, right] -> [0, b, 0, r]
            output_padding = [0 if i % 2 == 0 else out_pad[i//2] for i in range(len(pad))]
            # TODO: rdar://65588783 ([PyTorch] Define and error out on unsupported configuration for output_padding)
            # error out here with unsupported configuration along with output padding
            if sum(pad) == 0 and any(output_padding):
                raise ValueError("ConvTransponse configuration of padding=0 and output_padding > 0 not supported!")
            post_crop = pad.copy()
            pad *= 0
            for i in range(0, len(pad)):
                if post_crop[i] >= output_padding[i]:
                    post_crop[i] -= output_padding[i]
                else:
                    pre_pad[i] = output_padding[i] - post_crop[i]
            kwargs["pad"] = pre_pad
            if any(pre_pad):
                # Constant pad requires pad to be of lenght 2*input_rank
                pre_pad = [0] * 2 * (len(x.shape) - 2) + pre_pad
                x = mb.pad(x=x, pad=pre_pad)
                kwargs["x"] = x
            if any(post_crop):
                del kwargs["name"]

        kwargs["weight"] = weight_transpose
        conv = mb.conv_transpose(**kwargs)
        if any(post_crop):
            # TODO: rdar://65575826 (PyTorch converter: output_padding mapping to slice
            # instead of crop layer for 1 and 3D ConvTranspose)
            if len(post_crop) != 4:
                raise ValueError("output_padding is supported only for ConvTranspose2D!")
            conv = mb.crop(x=conv, crop_height=post_crop[:2], crop_width=post_crop[2:4], name=node.name)
    else:
        # Normal convolution
        kwargs["weight"] = weight
        conv = mb.conv(**kwargs)

    context.add(conv)

@register_torch_op
def sinh(context, node):
    inputs = _get_inputs(context, node, expected=1)
    res = mb.sinh(x=inputs[0], name=node.name)
    context.add(res)

@register_torch_op
def softmax(context, node):
    inputs = _get_inputs(context, node)

    x = inputs[0]
    axis = inputs[1]
    res = mb.softmax(x=x, axis=axis, name=node.name)
    context.add(res)


@register_torch_op
def flatten(context, node):
    inputs = _get_inputs(context, node)

    x = inputs[0]
    dims = list(x.shape)
    start = inputs[1].val
    end_val = inputs[2].val

    total = 1
    if end_val < 0:
        end = len(dims) + end_val
    else:
        end = end_val

    if start > len(dims) or end > len(dims) or start < 0 or end < 0:
        raise ValueError(
            "Invalid start and end. (start, end) == ({}, {})".format(start, end_val)
        )
    if start > end:
        raise ValueError(
            "Start must be before end. (start, end) == ({}, {})".format(start, end_val)
        )
    for dim in dims[start : end + 1]:
        total *= dim
    dims = dims[:start] + [total] + dims[end + 1 :]

    reshape = mb.reshape(x=x, shape=dims, name=node.name)
    context.add(reshape)


@register_torch_op(torch_alias=["relu_"])
def relu(context, node):
    inputs = _get_inputs(context, node, expected=1)

    res = mb.relu(x=inputs[0], name=node.name)
    context.add(res)

@register_torch_op(torch_alias=["leaky_relu_"])
def leaky_relu(context, node):
    inputs = _get_inputs(context, node, expected=2)

    res = mb.leaky_relu(x=inputs[0], alpha=inputs[1], name=node.name)
    context.add(res)

def _adjust_pad_for_ceil_mode(input_shape, kernel_sizes, stride_sizes, pad_sizes):
    """ TODO Given an input tensor and pooling parameters, add the extra input
        padding needed to replicate ceil_mode. If no padding is needed, returns
        the original input. Otherwise, returns the Var returned by the new
        padding op.
    """
    new_pad = pad_sizes
    for idx in range(len(input_shape)):
        dim = input_shape[idx]
        kernel = kernel_sizes[idx]
        stride = stride_sizes[idx]
        pad = pad_sizes[idx * 2 : idx * 2 + 2]
        out_numerator = dim + pad[0] + pad[1] - kernel
        remainder = out_numerator % stride
        # Additional padding is added only on one side.
        # https://stackoverflow.com/questions/59906456/in-pytorchs-maxpool2d-is-padding-added-depending-on-ceil-mode
        if remainder > 0:
            # MIL pooling does not support ceil_mode natively, but we can
            # workaround by padding the input appropriately.
            # rdar://60634390
            _logging.warning("pooling padding adjusted to support ceil_mode=True")
            new_pad[2 * idx + 1] += stride - remainder

    return new_pad


@register_torch_op
def max_pool2d(context, node):
    inputs = _get_inputs(context, node)

    x = inputs[0]
    kernel_sizes = inputs[1]
    strides = inputs[2]
    pad_type = "custom"

    # Need to explicity state L-R, T-B pad
    pad = inputs[3]
    pad = _np.repeat(pad.val, 2)
    dilation = inputs[4].val
    ceil_mode = inputs[5].val
    if _np.any(dilation > 1):
        # See: rdar://60633736 (Implement dilation for mil op max_pool)
        raise ValueError("@max_pool2d does not support dilation > 1")
    if ceil_mode is True:
        pad = _adjust_pad_for_ceil_mode(
            x.shape[-2:], kernel_sizes.val, strides.val, pad
        )

    pool = mb.max_pool(
        x=x,
        kernel_sizes=kernel_sizes,
        strides=strides,
        pad_type=pad_type,
        pad=pad,
        name=node.name,
    )
    context.add(pool)


@register_torch_op
def div(context, node):
    inputs = _get_inputs(context, node, expected=2)

    res = mb.real_div(x=inputs[0], y=inputs[1], name=node.name)
    context.add(res)


@register_torch_op
def floor_divide(context, node):
    inputs = _get_inputs(context, node, expected=2)
    res = mb.floor_div(x=inputs[0], y=inputs[1], name=node.name)
    context.add(res)


@register_torch_op
def mul(context, node):
    inputs = _get_inputs(context, node, expected=2)

    res = mb.mul(x=inputs[0], y=inputs[1], name=node.name)
    context.add(res)


@register_torch_op(torch_alias=["pow"])
def pow_(context, node):
    inputs = _get_inputs(context, node, expected=2)

    res = mb.pow(x=inputs[0], y=inputs[1], name=node.name)
    context.add(res)


@register_torch_op(torch_alias=["rsub"])
def sub(context, node):
    inputs = _get_inputs(context, node, expected=3)
    assert len(node.outputs) == 1

    if node.kind == "rsub":
        # rsub reverses the order of arguments
        y = inputs[0]
        x = inputs[1]
    else:
        x = inputs[0]
        y = inputs[1]
    alpha = inputs[2].val

    # TODO (sberardi): 3rd param to aten::sub is a scale factor, need to handle that.
    # out=input-alpha x other
    # rdar://60175736
    if alpha != 1:
        raise ValueError("SUB does not support scale factor param")

    res = mb.sub(x=x, y=y, name=node.name)
    context.add(res)


@register_torch_op
def mean(context, node):
    inputs = _get_inputs(context, node)

    kwargs = {"x": inputs[0], "name": node.name}

    # @axes is optional, so omit if None.
    axes = inputs[1]
    if axes is not None:
        # @axes needs to be a list, but if only one axis was specified in the
        # model, it will be constructed as an int. Construct a new constant as a
        # list.
        if not isinstance(axes.val, _np.ndarray):
            axes = mb.const(val=[axes.val], name=axes.name + "_list")
            context.add(axes)
        kwargs["axes"] = axes

    # @keep_dims is optional.
    if len(inputs) >= 3:
        keep_dims = inputs[2]
        kwargs["keep_dims"] = keep_dims

    # Last input to mean is an optional output tensor. We always expect this to
    # be None or absent.
    assert len(inputs) <= 3 or inputs[3] is None
    res = mb.reduce_mean(**kwargs)
    context.add(res)


@register_torch_op
def squeeze(context, node):
    inputs = _get_inputs(context, node)
    if len(inputs) == 1:
        res = mb.squeeze(x=inputs[0], name=node.name)
    elif len(inputs) == 2:
        squeeze_dim = inputs[1].val
        res = mb.squeeze(x=inputs[0], axes=(squeeze_dim,), name=node.name)
    context.add(res)


@register_torch_op
def unsqueeze(context, node):
    inputs = _get_inputs(context, node, expected=2)
    unsqueeze = mb.expand_dims(x=inputs[0], axes=[inputs[1].val], name=node.name)
    context.add(unsqueeze)


@register_torch_op
def size(context, node):
    inputs = _get_inputs(context, node, expected=2)

    # Get the shape of the tensor.
    shape_node = mb.shape(x=inputs[0], name=node.name + "_shape")
    # Get the size of the tensor along the input dimension.
    dim = inputs[1].val
    size_node = _list_select(shape_node, dim)
    context.add(size_node, node.name)


@register_torch_op(torch_alias=["reshape"])
def view(context, node):
    inputs = _get_inputs(context, node, expected=2)
    x = inputs[0]
    shape = inputs[1]

    if isinstance(shape, ListVar):
        length = mb.list_length(ls=shape)
        indices = mb.range_1d(start=0, end=length, step=1)
        shape = mb.list_gather(ls=shape, indices=indices)
    view = mb.reshape(x=x, shape=shape, name=node.name)
    context.add(view)


@register_torch_op
def adaptive_avg_pool2d(context, node):
    inputs = _get_inputs(context, node, expected=2)

    _input = inputs[0]
    output_size = inputs[1].val
    assert isinstance(output_size, _np.ndarray)
    output_size = tuple(output_size)

    if output_size == (1, 1):
        # Represent (1,1) output size via @reduce_mean
        # Assume channel first ordering, reduce the last two (HW) dims.
        axes = mb.const(val=[-2, -1], name=node.name + "_axes")
        keep_dims = mb.const(val=True, name=node.name + "_keep_dims")

        avg_pool = mb.reduce_mean(
            x=_input, axes=axes, keep_dims=keep_dims, name=node.name
        )
    elif _input.shape is not None:
        # TODO: The calculations to convert adaptive_pool to standard pool,
        # given a known input size, come from
        # https://stackoverflow.com/questions/53841509/how-does-adaptive-pooling-in-pytorch-work
        # However, as indicated in that SO, this isn't quite how PyTorch
        # computes adaptive pooling, leading to inaccuracies in model outputs.
        # rdar://60900834
        strides = [ind // outd for ind, outd in zip(_input.shape[-2:], output_size)]
        pad_type = "valid"
        # Need to explicity state L-R, T-B pad
        pad = [0, 0, 0, 0]
        dilation = [1, 1]
        kernel_sizes = [
            ind - s * (outd - 1)
            for ind, outd, s in zip(_input.shape[-2:], output_size, strides)
        ]
        avg_pool = mb.avg_pool(
            x=_input,
            kernel_sizes=kernel_sizes,
            strides=strides,
            pad_type=pad_type,
            pad=pad,
            name=node.name,
        )
    else:
        raise ValueError(
            "adaptive_avg_pool2d only supported when input tensor size is known or output size == (1,1). Recived: input size == {}, output size == {}".format(
                _input.shape_str(), output_size,
            )
        )

    context.add(avg_pool)


@register_torch_op
def batch_norm(context, node):
    inputs = _get_inputs(context, node, expected=9)
    # inputs skipped:
    #   bool training (5)
    #   float momentum (6)
    #   bool cudnn_enabled (8)
    _input = inputs[0]
    weight = inputs[1]
    bias = inputs[2]
    running_mean = inputs[3]
    running_var = inputs[4]
    eps = inputs[7]
    batch_norm = mb.batch_norm(
        x=_input,
        mean=running_mean,
        variance=running_var,
        gamma=weight,
        beta=bias,
        epsilon=eps,
        name=node.name,
    )
    context.add(batch_norm)


@register_torch_op
def instance_norm(context, node):
    inputs = _get_inputs(context, node, expected=9)
    x = inputs[0]
    weight = inputs[1]
    bias = inputs[2]
    eps = inputs[7]
    x = mb.instance_norm(
        x=x,
        gamma=weight,
        beta=bias,
        epsilon=eps,
        name=node.name,
    )
    context.add(x)


@register_torch_op
def embedding(context, node):
    inputs = _get_inputs(context, node)
    _input = inputs[0]
    indices = inputs[1]

    padding_idx = -1
    scale_grad_by_freq = False
    sparse = False
    if len(inputs) >= 3:
        padding_idx = inputs[2].val
    if len(inputs) >= 4:
        scale_grad_by_freq = inputs[3].val
    if len(inputs) >= 5:
        sparse = inputs[4].val

    if padding_idx != -1 or scale_grad_by_freq or sparse:
        _logging.warning(
            "CoreML embedding (gather) layer does not support any "
            "inputs besides the weights and indices. Those given "
            "will be ignored."
        )

    #  Changing the axis from 0 is not an option in torch, so we don't expose it
    gather = mb.gather(x=_input, indices=indices, name=node.name)
    context.add(gather)


@register_torch_op
def hardtanh_(context, node):
    inputs = _get_inputs(context, node, expected=3)
    _input = inputs[0]
    min_val = inputs[1].val
    max_val = inputs[2].val

    res = mb.clip(x=_input, alpha=min_val, beta=max_val, name=node.name)
    context.add(res)


@register_torch_op
def cat(context, node):
    inputs = _get_inputs(context, node)

    values = inputs[0]
    if len(values) == 1:
        # Only one item to "concatenate", so treat it as a no-OP. Otherwise,
        # NN concatND layer will complain it only has one input.
        context.add(values[0], node.name)
        return

    if len(inputs) < 2:
        axis = 0
    else:
        axis = inputs[1]

    concat = mb.concat(values=values, axis=axis, name=node.name)
    context.add(concat)


@register_torch_op
def stack(context, node):
    inputs = _get_inputs(context, node)

    values = inputs[0]

    if len(inputs) < 2:
        axis = 0
    else:
        axis = inputs[1]

    res = mb.stack(values=values, axis=axis, name=node.name)
    context.add(res)


@register_torch_op
def item(context, node):
    inputs = _get_inputs(context, node, expected=1)

    if inputs[0].shape == ():
        # MIL ops that reduce already output a scalar, so no need to do
        # anything.
        res = inputs[0]
    elif _np.all([d == 1 for d in inputs[0].shape]):
        # Item only makes sense when called on a length 1 tensor. We use
        # reduce_max as a workaround for not having a way to extract a scalar
        # from a symbolic tensor.
        res = mb.reduce_max(x=inputs[0], name=node.name)
    else:
        raise ValueError("expected input to be a scalar or a length 1 tensor")
    context.add(res, node.name)


def _cast(context, node, dtype, dtype_name):
    inputs = _get_inputs(context, node, expected=1)
    x = inputs[0]
    # Input must either be a scalar or a (1 x 1 x ... x 1) tensor
    if not (len(x.shape) == 0 or _np.all([d == 1 for d in x.shape])):
        raise ValueError("input to cast must be either a scalar or a length 1 tensor")

    if x.val is not None:
        # If x is a compile-time constant, directly cast it to @dtype if it's
        # not one already.
        if not isinstance(x.val, dtype):
            res = mb.const(val=dtype(x.val), name=node.name)
        else:
            res = x
    else:
        if len(x.shape) > 0:
            # TODO: There's no MIL op to extract a value from a symbolic tensor,
            # so as a workaround we use reduce_max to convert it to a scalar.
            x = mb.reduce_max(x=x, name=node.name + "_item")
        res = mb.cast(x=x, dtype=dtype_name, name=node.name)
    context.add(res, node.name)


@register_torch_op(torch_alias=["bool"])
def _bool(context, node):
    inputs = _get_inputs(context, node, expected=1)

    x = inputs[0]
    # TODO: this is a hack and should be replaced once MIL supports cast to
    # bool.
    if x.val is not None and not isinstance(x.val, bool):
        x = mb.const(val=bool(x.val), name=node.name)
    context.add(x, node.name)


@register_torch_op(torch_alias=["int"])
def _int(context, node):
    _cast(context, node, int, "int32")


@register_torch_op
def layer_norm(context, node):
    inputs = _get_inputs(context, node, expected=6)
    _input = inputs[0]
    normalized_shape = inputs[1]
    weight = inputs[2]
    bias = inputs[3]
    eps = inputs[4]
    # cudnn_enable = inputs[5] unused
    layer_norm = mb.layer_norm(
        x=_input,
        axes=normalized_shape,
        gamma=weight,
        beta=bias,
        epsilon=eps,
        name=node.name,
    )
    context.add(layer_norm)


@register_torch_op
def numtotensor(context, node):
    inputs = _get_inputs(context, node, expected=1)
    x = inputs[0]
    if x.shape != ():
        raise ValueError(
            "numtotensor expected scalar input, got tensor with shape {}".format(
                x.shape
            )
        )
    if isinstance(x.sym_val, Symbol):
        context.add(x, node.name)
    else:
        res = mb.const(val=[x.sym_val], name=node.name)
        context.add(res)


def _ifzo_to_ifoz(weights, name):
    """
        i, f, z, o -> i, f, o, z
        where weights_split[0] == i, etc.
        Used to transform lstm weights from pytorch
        to CoreML format
    """
    split_size = weights.shape[0] // 4
    weights_split = mb.split(x=weights, split_sizes=_np.array([split_size] * 4), axis=0)
    weights_concat = mb.concat(
        values=[weights_split[0], weights_split[1], weights_split[3], weights_split[2]],
        axis=0,
    )
    # make transpose a noOP for 0/1d tensors
    return mb.transpose(
        x=weights_concat, perm=([1, 0] if len(weights.shape) > 1 else [0]), name=name
    )


def _pytorch_hidden_to_coreml_milops(x, name):
    """
        Used to transform lstm state values (hn, cn)
        from pytorch to CoreML format.
    """
    split_size = x.shape[0] // 2
    x_split = mb.split(x=x, split_sizes=_np.array([split_size] * 2), axis=0)
    x_concat = mb.concat(values=[x_split[0], x_split[1]], axis=2,)
    # (4.) See docstring to @lstm
    return mb.squeeze(x=x_concat, axes=_np.array([0]), name=name)


@register_torch_op
def lstm(context, node):
    inputs = _get_inputs(context, node, expected=9)

    _input = inputs[0]
    h0, c0 = inputs[1]
    weights = inputs[2]
    bias = inputs[3].val
    num_layers = inputs[4].val
    dropout = inputs[5]
    bidirectional = inputs[7].val
    batch_first = inputs[8].val

    if num_layers != 1:
        raise ValueError(
            "CoreML does not support stacked LSTM layers (LSTM "
            "with num_layers > 1). Received {}. Redefine as "
            " multiple layers if this is the desired "
            "implementation.".format(num_layers)
        )

    if batch_first:
        _input = mb.transpose(x=_input, perm=[1, 0, 2], name=_input.name + "_batch_first_transpose")

    expected_num_weights = 2 * num_layers * (int(bidirectional) + 1) * (int(bias) + 1)
    if len(weights) != expected_num_weights:
        raise ValueError(
            "Incorrect weights shape for lstm layer: Expected: {}. Recieved {}".format(
                expected_num_weights, len(weights)
            )
        )

    # NOTE:
    # Most of this code is to transform the tensors into
    # a shape acceptable by the CoreML implementation of LSTM.
    # Since this transforming is complicated and unintuitive we include
    # a description of what is happening:

    # For weights, biases and per direction, pytorch uses two tensors:
    # (ii, if, ig, io) stacked on top of eachother for each layer (tensor 1)
    # and (hi, hf, hg, ho) stacked on top of eachother for each layer (tensor 2)
    # These weights are used in the calculation of the layers found in the torch.nn documentation:
    # https://pytorch.org/docs/stable/nn.html

    # The CoreML LSTM op expects two tensors, weight and bias. So
    # the tensors for weight and bias are seperated from pytorch's @weights list (1.).
    # For each individual weight and bias tensor, the CoreML LSTM op expects the form
    # ii, if, io, ig and hi, hf, ho, hg, requiring the ifzo_to_ifoz function (2.).
    # Each seperate weight and bias tensor is concatinated to
    # form the two weight and bias tensors. (3.)
    # In the bidirectional case, the forward and backward weights and biases
    # are stacked on top of eachother instead of stored as seperate tensors in
    # the @weights list. (4.)

    # In the initial cell and hidden states, pytorch's tensor's 0th
    # dimension stores each layer and direction.
    # However, since CoreML's LSTM allows only one layer, the direction is squeezed out the state
    # tensor. (4.)
    # In the bidirectional case, the forward and backward state tensors are stacked on top of eachother.
    # instead of stored in the layer and direction dimension
    # using @_pytorch_hidden_to_coreml_milops (5.).

    # For output: The CoreML LSTM op returns the final states with the first dimension: @num_layers
    # squeezed out. To fit with the rest of the shapes expected down the line in
    # the TorchIR graph- we unsqueeze that dimension in the final state output. (6.)
    if bidirectional:
        if bias:
            # (1.)
            biases = weights[2:4] + weights[6:8]
            weights = weights[0:2] + weights[4:6]

            # (2.)
            assert len(biases) == 4
            for index in range(len(biases)):
                biases[index] = _ifzo_to_ifoz(
                    biases[index],
                    name="{}_lstm_bias_reshape_{}".format(node.name, index),
                )

            # (4.)
            f_stack = mb.stack(values=biases[0:2], axis=0,)
            r_stack = mb.stack(values=biases[2:4], axis=0,)
            # (3.)
            final_biases = mb.concat(
                values=(f_stack, r_stack),
                axis=1,
                name=node.name + "_lstm_biases_concat",
            )

        # (4.)
        forward_concat = mb.concat(
            values=[weights[0], weights[1]],
            axis=1,
            name=node.name + "_lstm_weights_forward_concat",
        )
        backward_concat = mb.concat(
            values=[weights[2], weights[3]],
            axis=1,
            name=node.name + "_lstm_weights_backward_concat",
        )
        # (2.)
        forward_transformed = _ifzo_to_ifoz(
            forward_concat, name=node.name + "_lstm_forward_weights_ifoz_to_ifzo",
        )
        backward_transformed = _ifzo_to_ifoz(
            backward_concat, name=node.name + "_lstm_backward_weights_ifoz_to_ifzo"
        )
        # (3.)
        final_weights = mb.concat(
            values=[forward_transformed, backward_transformed],
            axis=1,
            name=node.name + "_lstm_weights_final_concat",
        )

        # (5.)
        h = _pytorch_hidden_to_coreml_milops(h0, name="_lstm_h0_reshaped")
        c = _pytorch_hidden_to_coreml_milops(c0, name="_lstm_c0_reshaped")

    else:
        if bias:
            # (1.)
            biases = weights[len(weights) // 2 :]
            weights = weights[: len(weights) // 2]
            ih_b = biases[0]
            hh_b = biases[1]

            # (2.)
            ih_b_transformed = _ifzo_to_ifoz(
                ih_b, name=node.name + "_lstm_ih_bias_transformed",
            )
            hh_b_transformed = _ifzo_to_ifoz(
                hh_b, name=node.name + "_lstm_hh_bias_transformed",
            )

            # (3.)
            final_biases = mb.stack(
                values=(ih_b_transformed, hh_b_transformed),
                axis=0,
                name=node.name + "_lstm_bias_stacked",
            )

        # (3.)
        weights_concat = mb.concat(
            values=weights, axis=1, name=node.name + "_lstm_weights_concat"
        )
        # (2.)
        final_weights = _ifzo_to_ifoz(
            weights_concat, name=node.name + "_lstm_weights_ifoz_to_ifzo",
        )

        # (4.)
        h = mb.squeeze(x=h0, axes=_np.array([0]), name=node.name + "_lstm_h0_squeeze")
        c = mb.squeeze(x=c0, axes=_np.array([0]), name=node.name + "_lstm_c0_squeeze")

    lstm = mb.lstm(
        x=_input,
        initial_h=h,
        initial_c=c,
        weight=final_weights,
        bias=(final_biases if bias else None),
        direction=("bidirectional" if bidirectional is True else "forward"),
        output_sequence=True,
        name=node.name if not batch_first else node.name + "_batch_first",
    )

    # (6.)
    for index, (name, output) in enumerate(zip(node.outputs, lstm)):
        if index > 0:
            # Add in @num_layers in first dimension to hn, cn output
            unsqueeze = mb.expand_dims(x=output, axes=[0], name=name)
            context.add(unsqueeze)
        else:
            if batch_first:
                output = mb.transpose(x=output, perm=[1, 0, 2], name=name)
            context.add(output, name)

def _get_scales_from_output_size(output_size, input_shape):
    scales = []
    if output_size is not None:
        # @output_size will be a list if scales was provided or a
        # single var if output size was provided
        if isinstance(output_size, list):
            output_size = [output_size[0].val, output_size[1].val]
        if isinstance(output_size, Var):
            output_size = [output_size.val[0], output_size.val[1]]

        # output size is computed using the formula
        # floor (scale * input_size) in Core ML (and PyTorch)
        # Thus, when computing the scales from the output size,
        # add a small positive constant to the output size,
        # to make sure that the floor formula results in the correct output
        # size and not 1 unit smaller, due to float precision issues
        # e.g. if output size = 34 and input size = 2, then scale will be
        # 17, which can get represented as 16.9999, resulting in an output size of 33
        # instead of 34, without this correction.
        scales_h = (output_size[0] + 1e-4) / float(input_shape[-2])
        scales_w = (output_size[1] + 1e-4) / float(input_shape[-1])
        scales = [scales_h, scales_w]
    return scales

@register_torch_op
def upsample_bilinear2d(context, node):
    inputs = _get_inputs(context, node)
    _input = inputs[0]
    output_size = inputs[1]
    align_corners = bool(inputs[2].val)

    if len(inputs) == 5:
        # For torch==1.5.0, upsample_bilinear2d has 5 inputs.
        scales_h = inputs[3]
        scales_w = inputs[4]

    scales = _get_scales_from_output_size(output_size, _input.shape)
    if scales:
        scales_h, scales_w = scales

    upsample_bilinear = mb.upsample_bilinear(
        x=_input,
        scale_factor_height=scales_h,
        scale_factor_width=scales_w,
        align_corners=align_corners,
        name=node.name,
    )
    context.add(upsample_bilinear)

@register_torch_op
def upsample_nearest2d(context, node):
    inputs = _get_inputs(context, node)
    _input = inputs[0]
    output_size = inputs[1]
    if len(inputs) == 4:
        scales_h = inputs[2]
        scales_w = inputs[3]

    scales = _get_scales_from_output_size(output_size, _input.shape)
    if scales:
        scales_h, scales_w = scales

    if (
        abs(scales_h - round(scales_h)) > 0.001
        or abs(scales_w - round(scales_w)) > 0.001
    ):
        raise ValueError("Layer upsample_nearest2d only supports integral scales. Provided scales: {}. "
                         "Please use upsample_bilinear2d for fractional scales".format(scales))

    upsample_nearest2d = mb.upsample_nearest_neighbor(
        x=_input,
        upscale_factor_height=int(round(scales_h)),
        upscale_factor_width=int(round(scales_w)),
        name=node.name,
    )
    context.add(upsample_nearest2d)

@register_torch_op(torch_alias=["listunpack"])
def tupleunpack(context, node):
    inputs = _get_inputs(context, node, expected=1)
    values = inputs[0]
    # Node input could have been turned into constant array in @tupleconstruct
    if not isinstance(values, tuple) and not isinstance(values, list):
        values = values.val
    if len(values) != len(node.outputs):
        raise ValueError(
            "unpack node expected {} outputs, got {}".format(
                len(node.outputs), len(values)
            )
        )
    assert len(values) == len(node.outputs)
    # @value is either a numpy primitive or a Var object
    for value, output in zip(values, node.outputs):
        if not isinstance(value, Var):
            value = _construct_constant(value, name=output)
        assert isinstance(value, Var)
        context.add(value, output)


@register_torch_op
def loop(context, node):
    """ In TorchIR, a loop looks like:
            %y_1, ..., %y_r = prim::Loop(%max_trip_count, %initial_condition, %x_1, ..., %x_r)
            block0(%i, %a_1, ..., %a_r):
                %b_1, ..., %b_m = some::node(%a_value_from_outer_block, %a_1)
                %iter_condition = some::other_node(%a_2)
                -> (%iter_condition, %b_1, ..., %b_r)

        This translates to pseudo code as:
            y_1, ..., y_r = x_1, ..., x_r
            condition = initial_condition
            i = 0
            while condition and i < max_trip_count:
                a_1, ..., a_r = y_1, ..., y_r

                ############################################################
                # Actual body of the loop
                b_1, ..., b_m = some::node(a_value_from_outside_of_the_loop, a_1)
                iter_condition = some::node(a_2)
                ############################################################

                y_1, ..., y_r = b_1, ..., b_r
                condition = iter_condition
                i += 1

        Which further translates to MIL while_loop as:
            loop_vars = (0, initial_condition, x_1, ..., x_r)
            _cond = {
                return (loop_vars[1] and loop_vars[0] < max_trip_count)
            }
            _body = {
                a_1, ..., a_r = loop_vars[2], ..., loop_vars[-1]
                b_1, ..., b_m = some::node(a_value_from_outside_of_the_loop, a_1)
                iter_condition = some::node(a_2)
                return (loop_vars[0] + 1, iter_condition, b_1, ..., b_r)
            }

        For loops pass True for %initial_condition and %iter_condition
        While loops set %max_trip_count to INT_MAX and %i is unused
    """
    name = node.name
    # inputs[0]: max iter count
    # inputs[1]: initial condition
    # inputs[2]: block input 0
    # ...
    # inputs[N+2]: block input N
    inputs = _get_inputs(context, node)
    max_iter_count = inputs[0]

    # Magic default signals this is a while-only loop, so no iteration count
    # is needed.
    has_iter_count = max_iter_count is not None

    # Create an interation count. This will only be used if this is a for loop.
    iter_count = mb.const(val=0, name=node.name + "_iter")
    # @loop_vars is tuple(iter_count, cond, inputs...)
    loop_vars = tuple([iter_count] + inputs[1:])

    def _loop_cond(*loop_vars):
        cond = loop_vars[1]

        # Check the iteration count if we're keeping track.
        if has_iter_count:
            iter_count = loop_vars[0]
            iter_cond = mb.less(
                x=iter_count, y=max_iter_count, name=node.name + "_cond"
            )
            return mb.logical_and(x=cond, y=iter_cond)
        else:
            return mb.identity(x=cond)

    def _shapes_are_equivalent(shape1, shape2):
        """ Compares two sets of tensor shapes and returns True if they are
            equivalent. That is, they are the same rank, and each dimension
            is the same or symbolic.
        """
        if len(shape1) != len(shape2):
            return False

        # Each dimension must have the same integer length, or else be
        # symbolic.
        all_equivalent = [
            s1 == s2 or (isinstance(s1, Symbol) and isinstance(s2, Symbol))
            for s1, s2 in zip(shape1, shape2)
        ]
        return all_equivalent

    def _loop_body(*loop_vars):
        block = node.blocks[0]
        iter_var = loop_vars[0]
        inputs = (iter_var,) + loop_vars[2:]
        res = convert_block(context, block, inputs)

        for input_var, output_var in zip(loop_vars[2:], res[1:]):
            if not _shapes_are_equivalent(input_var.shape, output_var.shape):
                _logging.warning(
                    "detected change in shape of loop variable. this could lead to incorrect inference results!"
                )
                _logging.warning(
                    "{}:{} -> {}:{}".format(
                        input_var.name,
                        input_var.shape,
                        output_var.name,
                        output_var.shape,
                    )
                )

        # Update the iteration count if we're keeping track.
        if has_iter_count:
            iter_var = mb.add(x=iter_var, y=1, name=iter_var.name + "_inc")
        else:
            iter_var = mb.identity(x=iter_var)

        # Must return tuple with same length and types as @loop_vars.
        return tuple([iter_var,] + res)

    loop = mb.while_loop(
        _cond=_loop_cond, _body=_loop_body, loop_vars=loop_vars, name=name
    )

    # Make sure the loop returned the expected number of outputs. Note that the
    # first two loop outputs are the iteration count and condition.
    assert len(loop) - 2 == len(node.outputs)
    for output_name, output_var in zip(node.outputs, loop[2:]):
        context.add(output_var, torch_name=output_name)


@register_torch_op(torch_alias=["if"])
def _if(context, node):
    """ In TorchIR, a conditional looks like:
            %y_1, ..., %y_r = prim::If(%condition)
            block0():  # TRUE BRANCH, never takes arguments, has to return r outputs
                %t_1, ..., %t_k = some::node(%a_value_from_outer_block)
                -> (%t_1, ..., %t_r)
            block1():  # FALSE BRANCH, never takes arguments, has to return r outputs
                %f_1, ..., %f_m = some::node(%a_value_from_outer_block)
                -> (%f_1, ..., %f_r)

        This translates to pseudo code as:
            if (condition):
                t_1, ..., t_k = some::node(a_value_from_outer_block)
                y_1, ..., y_r = t_1, ..., t_r
            else:
                f_1, ..., f_m = some::node(a_value_from_outer_block)
                y_1, ..., y_r = f_1, ..., f_r

        Which further translates to MIL cond as:
            _true = {
                t_1, ..., t_k = some::node(a_value_from_outer_block)
                return (t_1, ..., t_r)
            }
            _false = {
                f_1, ..., f_m = some::node(a_value_from_outer_block)
                return (f_1, ..., f_m)
            }
    """
    name = node.name
    # inputs[0]: condition
    inputs = _get_inputs(context, node, expected=1)
    condition = inputs[0]

    assert len(node.blocks) == 2
    true_block = node.blocks[0]
    false_block = node.blocks[1]

    def _true_path():
        res = convert_block(context, true_block, [])
        return tuple(res)

    def _false_path():
        res = convert_block(context, false_block, [])
        return tuple(res)

    cond = mb.cond(
        pred=condition, _true_fn=_true_path, _false_fn=_false_path, name=name
    )
    # If the condition only returns one item, wrap it in a tuple.
    if not isinstance(cond, tuple):
        cond = (cond,)

    # Make sure the condition returned the expected number of outputs.
    assert len(cond) == len(node.outputs)
    for output_name, output_var in zip(node.outputs, cond):
        context.add(output_var, torch_name=output_name)


@register_torch_op
def select(context, node):
    inputs = _get_inputs(context, node, expected=3)
    _input = inputs[0]
    dim = inputs[1].val
    index = inputs[2].val

    assert dim.shape == ()
    assert index.shape == ()
    assert _input.val is None

    # NOTE:
    # Each index in @begin_array/@end_array corresponds to a dimension of @_input
    # Each val of those arrays corresponds to the start/end index to slice in that dimension
    begin_array = [0] * len(_input.shape)
    begin_array[dim] = index
    end_array = [s if isinstance(s, int) else 0 for s in _input.shape]
    end_mask = [True] * len(_input.shape)
    if index != -1:
        end_array[dim] = index + 1
        end_mask[dim] = False

    slice_by_index = mb.slice_by_index(
        x=_input,
        begin=begin_array,
        end=end_array,
        end_mask=end_mask,
        name=node.name + "_slice_by_index",
    )
    # Now we squeeze the dimension we have selected from to remove it
    squeeze = mb.squeeze(
        x=slice_by_index, axes=_np.array([dim]), name=node.name + "_squeeze"
    )
    context.add(squeeze, node.name)


@register_torch_op
def ones(context, node):
    inputs = _get_inputs(context, node, expected=6)
    size = inputs[0]
    # dtype = NUM_TO_TORCH_DTYPE[inputs[1].val] unused
    # layout = inputs[2] unused
    # device = inputs[3] unused
    # requires_grad = inputs[4] unused
    # out = inputs[5] unused
    fill = mb.fill(shape=size, value=1.0, name=node.name)
    context.add(fill)


@register_torch_op
def ones_like(context, node):
    inputs = _get_inputs(context, node, expected=6)
    size = mb.shape(x=inputs[0])
    # dtype = NUM_TO_TORCH_DTYPE[inputs[1].val] unused
    # layout = inputs[2] unused
    # device = inputs[3] unused
    # requires_grad = inputs[4] unused
    # out = inputs[5] unused
    fill = mb.fill(shape=size, value=1.0, name=node.name)
    context.add(fill)


def _avg_pool(context, node, inputs):
    x = inputs[0]
    kernel_sizes = inputs[1]
    strides = inputs[2]
    pad_type = "custom"
    # Need to explicity state L-R, T-B pad
    pad = inputs[3]
    pad = _np.repeat(pad.val, 2)
    ceil_mode = inputs[4]
    if ceil_mode.val is True:
        rank = len(pad) // 2
        pad = _adjust_pad_for_ceil_mode(
            x.shape[-rank:], kernel_sizes.val, strides.val, pad
        )
    include_pad = inputs[5].val

    pool = mb.avg_pool(
        x=x,
        kernel_sizes=kernel_sizes,
        strides=strides,
        pad_type=pad_type,
        pad=pad,
        name=node.name,
        exclude_padding_from_average=not include_pad,
    )
    context.add(pool)


@register_torch_op
def avg_pool1d(context, node):
    inputs = _get_inputs(context, node, expected=6)
    _avg_pool(context, node, inputs)


@register_torch_op
def avg_pool2d(context, node):
    inputs = _get_inputs(context, node, expected=7)
    divisor_override = inputs[6]
    if divisor_override is not None:
        raise ValueError("divisor_override is not supported for avg_pool2d")
    _avg_pool(context, node, inputs)


@register_torch_op
def log_softmax(context, node):
    inputs = _get_inputs(context, node)

    x = inputs[0]
    axis = inputs[1]
    out = inputs[2]  # Ignored.
    assert out is None
    res = mb.softmax(x=x, axis=axis, name=node.name + "_softmax")
    res = mb.log(x=res, name=node.name)
    context.add(res)


@register_torch_op
def sigmoid(context, node):
    inputs = _get_inputs(context, node, expected=1)

    res = mb.sigmoid(x=inputs[0], name=node.name)
    context.add(res)


@register_torch_op
def gelu(context, node):
    inputs = _get_inputs(context, node, expected=1)

    res = mb.gelu(x=inputs[0], name=node.name)
    context.add(res)


@register_torch_op(torch_alias=["slice"])
def _slice(context, node):
    inputs = _get_inputs(context, node, expected=5)
    x = inputs[0]
    dim = inputs[1].val
    start = inputs[2].val if inputs[2].val is not None else 0
    end = inputs[3].val if inputs[3] is not None else None
    step = inputs[4].val

    if start == 0 and end is None and step == 1:
        # Handling x[:], just pass through the tensor.
        context.add(x, node.name)
        return

    begin_array = [0] * len(x.shape)
    begin_array[dim] = start
    end_array = [s if isinstance(s, int) else 0 for s in x.shape]
    end_mask = [True] * len(x.shape)
    if end is not None:
        end_array[dim] = end
        end_mask[dim] = False

    kwargs = {
        "x": x,
        "begin": begin_array,
        "end": end_array,
        "end_mask": end_mask,
        "name": node.name,
    }

    if step != 1:
        stride_array = _np.array([1] * len(x.shape))
        stride_array[dim] = step
        kwargs["stride"] = stride_array

    res = mb.slice_by_index(**kwargs)
    context.add(res)


@register_torch_op(torch_alias=["split_with_sizes"])
def split(context, node):
    inputs = _get_inputs(context, node, expected=3)
    x = inputs[0]
    split_sizes = inputs[1]
    dim = inputs[2].val

    if not isinstance(split_sizes.val, _np.ndarray):
        shape = mb.shape(x=x)
        dim_size = _list_select(shape, dim)
        # MIL split op needs the size of each split to be given explicitly.
        num_whole_splits = mb.floor_div(x=dim_size, y=split_sizes)
        remainder = mb.mod(x=dim_size, y=split_sizes)

        # MIL doesn't have a way of turning a scalar into a tensor (list write
        # only supports tensors). As a workaround, we create a constant [1]
        # tensor and multiply it by the scalar value, thus creating a tensor
        # with the scalar value in it.
        tmp = mb.const(val=[1])
        whole_sizes = mb.mul(x=tmp, y=split_sizes)
        reps = mb.mul(x=tmp, y=num_whole_splits)
        whole_sizes = mb.tile(x=whole_sizes, reps=reps)
        if remainder.val == 0:
            split_sizes = whole_sizes
        else:
            partial_size = mb.mul(x=tmp, y=remainder)
            split_sizes = mb.concat(values=[whole_sizes, partial_size], axis=0)
    res = mb.split(x=x, split_sizes=split_sizes, axis=dim, name=node.name)
    context.add(res, torch_name=node.name)


@register_torch_op
def to(context, node):
    # @non_blocking and @copy are unused
    inputs = _get_inputs(context, node)
    if len(inputs) == 5:
        _input = inputs[0]
        device = inputs[1]
        dtype = inputs[2].val
        # non_blocking = inputs[3]
        # copy = inputs[4]
    elif len(inputs) == 4:
        _input = inputs[0]
        dtype = inputs[1].val
        # non_blocking = inputs[2]
        # copy = inputs[3]
    elif len(inputs) == 3:
        # Since @non_blocking and @copy are unused, add back to context
        _input = inputs[0]
        # non_blocking = inputs[1]
        # copy = inputs[2]
        context.add(_input, torch_name=node.name)
        return
    else:
        raise ValueError(
            "Received invalid arguments for PyTorch conversion of op {}".format(node)
        )

    torch_dtype = NUM_TO_TORCH_DTYPE[dtype]
    if isinstance(_input, Var):
        _input = _input.val

    # numpy -> torch -> torch cast -> numpy
    # This path is needed to use the mapping of passed in dtypes to torch dtypes.
    casted_input = torch.tensor(_input).type(torch_dtype).numpy()
    const = mb.const(mode="immediate_value", val=casted_input, name=node.name)
    context.add(const)


@register_torch_op
def floor(context, node):
    inputs = _get_inputs(context, node, expected=1)
    _input = inputs[0]
    floor = mb.floor(x=_input, name=node.name)
    context.add(floor)


@register_torch_op
def erf(context, node):
    inputs = _get_inputs(context, node, expected=1)
    _input = inputs[0]
    erf = mb.erf(x=_input, name=node.name)
    context.add(erf)


@register_torch_op
def implicittensortonum(context, node):
    inputs = _get_inputs(context, node, expected=1)
    _input = inputs[0]
    assert _input.shape == (1,)
    # shape: (1,) -> ()
    squeeze = mb.squeeze(x=_input, name=node.name)
    context.add(squeeze)


@register_torch_op
def constantchunk(context, node):
    inputs = _get_inputs(context, node, expected=1)
    x = inputs[0]
    # ConstantChunk gets its parameters as attributes of the node.
    chunks = node.attr["chunks"]
    dim = node.attr["dim"]

    total = x.shape[dim]
    size = int(_math.ceil(float(total) / float(chunks)))
    split_sizes = [size] * int(_math.floor(total / size))
    remainder = total - sum(split_sizes)
    if remainder > 0:
        split_sizes.append(remainder)

    res = mb.split(x=x, split_sizes=split_sizes, axis=dim, name=node.name)
    for val, name in zip(res, node.outputs):
        context.add(val, name)


def _expand(context, name, tensor, shape):
    reps = [ds if ds > 0 and ts == 1 else 1 for ts, ds in zip(tensor.shape, shape)]
    res = mb.tile(x=tensor, reps=reps, name=name)
    context.add(res)


@register_torch_op
def expand(context, node):
    inputs = _get_inputs(context, node, expected=2)
    x = inputs[0]
    shape = inputs[1].val

    _expand(context, node.name, x, shape)


@register_torch_op
def expand_as(context, node):
    inputs = _get_inputs(context, node, expected=2)
    x = inputs[0]
    other = inputs[1]

    _expand(context, node.name, x, other.shape)


@register_torch_op
def arange(context, node):
    inputs = _get_inputs(context, node)
    # dtype = inputs[-4]
    # layout = inputs[-3]
    # device = inputs[-2]
    # pin_memory = inputs[-1]
    if len(inputs) == 5:
        # inputs are [end, dtype, layout, device, pin_memory]
        start = 0
        end = inputs[0]
        step = 1
    elif len(inputs) == 6:
        # inputs are [start, end, dtype, layout, device, pin_memory]
        start = inputs[0]
        end = inputs[1]
        step = 1
    elif len(inputs) == 7:
        # inputs are [start, end, step, dtype, layout, device, pin_memory]
        start = inputs[0]
        end = inputs[1]
        step = inputs[2]
    else:
        raise ValueError(
            "arange must have exactly 5, 6, or 7 inputs, got {}".format(len(inputs))
        )

    res = mb.range_1d(start=start, end=end, step=step, name=node.name)
    context.add(res)


@register_torch_op(torch_alias=["masked_fill_"])
def masked_fill(context, node):
    inputs = _get_inputs(context, node, expected=3)
    x = inputs[0]
    mask = inputs[1]
    value = inputs[2]
    # @mb.select does not properly broadcast scalar input, so as a workaround
    # we create a full sized tensor.
    # rdar://61463562

    if types.is_int(value.dtype):
        # @mb.fill cannot handle value with dtype integer
        # so we cast the value.
        value = mb.cast(x=value, dtype="fp32")
    value = mb.fill(shape=x.shape, value=value, name=node.name + "_value")
    res = mb.select(cond=mask, a=value, b=x, name=node.name)
    context.add(res)


@register_torch_op
def meshgrid(context, node):
    """
    For N input tensors, a meshgrid is constructed by viewing each tensor as an N-dimension tensor
    with values in the dimension corresponding it its order in the args. (a.)
    Then, it is expanded along dimensions corresponding to the dimensions of each
    1d tensor in the order that they were passed in. (b.)

    Each output tensor is put into a tuple that is returned. These tuples form
    N, N-dimenional grids, where the ith grid is defined as expanding the ith input over
    dimensions defined by the other inputs.
    """
    inputs = _get_inputs(context, node)
    if len(inputs) < 2:
        raise ValueError("Requires > 2 tensor inputs.")

    # scalar inputs will be considered 1d tensors
    tensor_inputs = []
    for tensor_var in inputs:
        if not isinstance(tensor_var.val, _np.ndarray):
            tensor_inputs.append(_np.array(tensor_var.val))
        else:
            tensor_inputs.append(_np.array(tensor_var))

    if any([len(tensor_var.shape) > 1 for tensor_var in inputs]):
        raise ValueError("meshgrid recieved non-1d tensor.")

    dim_tuple = tuple(tensor_var.shape[0] for tensor_var in inputs)

    grids = []
    size = len(inputs)
    for i in range(size):
        view_shape = [1] * size
        view_shape[i] = -1
        view_shape = tuple(view_shape)
        tensor = torch.tensor(inputs[i].val)
        # (a.) in docstring
        view = mb.reshape(
            x=inputs[i], shape=view_shape, name=node.name + "_view_" + str(i)
        )

        # (b.) in docstring
        reps = [
            ds if ds > 0 and ts == 1 else 1 for ts, ds in zip(view.shape, dim_tuple)
        ]
        expand = mb.tile(x=view, reps=reps, name=node.name + "_expand_" + str(i))
        grids.append(expand)

    context.add(tuple(grids), node.name)


@register_torch_op
def tanh(context, node):
    inputs = _get_inputs(context, node, expected=1)
    _input = inputs[0]
    tanh = mb.tanh(x=_input, name=node.name)
    context.add(tanh)


# Defines all the nodes that are noOps
@register_torch_op(
    torch_alias=[
        "dropout",
        "dropout_",
        "feature_dropout",
        "contiguous",
        "device",
        "detach",
        "clone",
    ]
)
def noop(context, node):
    _logging.info("Setting pytorch op: {} to no-op.".format(node))
    inputs = _get_inputs(context, node)
    _input = inputs[0]
    context.add(_input, torch_name=node.name)


@register_torch_op
def argmax(context, node):
    inputs = _get_inputs(context, node)
    x = inputs[0]
    axis = inputs[1]
    keep_dims = inputs[2]
    res = mb.reduce_argmax(x=x, axis=axis, keep_dims=keep_dims, name=node.name)
    context.add(res)


@register_torch_op
def zeros(context, node):
    inputs = _get_inputs(context, node, expected=5)
    size = inputs[0].val
    dtype = inputs[1].val
    # layout = inputs[2] unused
    # device = inputs[3] unused
    # pin_memory = inputs[4] unused

    torch_dtype = NUM_TO_TORCH_DTYPE[dtype]
    zeros_array = torch.zeros(tuple(size)).type(torch_dtype).numpy()
    const = mb.const(mode="immediate_value", val=zeros_array, name=node.name)
    context.add(const)


@register_torch_op
def exp(context, node):
    inputs = _get_inputs(context, node, expected=1)
    exp = mb.exp(x=inputs[0], name=node.name)
    context.add(exp)


@register_torch_op
def max(context, node):
    inputs = _get_inputs(context, node, expected=3)
    _input = inputs[0]
    dim = inputs[1].val
    keepdim = inputs[2].val

    values = mb.reduce_max(x=_input, axes=[dim], keep_dims=keepdim)
    indices = mb.reduce_argmax(x=_input, axis=dim, keep_dims=keepdim)
    assert len(node.outputs) == 2
    values_name = node.outputs[0]
    indices_name = node.outputs[1]
    context.add(values, torch_name=values_name)
    context.add(indices, torch_name=indices_name)


@register_torch_op
def sort(context, node):
    inputs = _get_inputs(context, node)
    _input = inputs[0]
    axis = inputs[1].val
    descending = inputs[2].val
    # NOTE: This is actually descending
    # rdar://62901267 (argsort ascending is actually descending)
    indices = mb.argsort(x=_input, axis=axis, ascending=descending)
    values = mb.gather_along_axis(x=_input, indices=indices, axis=axis)

    values_name = node.outputs[0]
    indices_name = node.outputs[1]
    context.add(values, torch_name=values_name)
    context.add(indices, torch_name=indices_name)


@register_torch_op
def append(context, node):
    # Note: by applying torchir_passes.transform_inplace_ops the meaning of
    # this op is changed from the original TorchIR. This op expects a python
    # list or MIL List as its first input. If an MIL List, the second input
    # must be a tensor of whatever shape the List expects. If not an MIL List,
    # the second input can by anything. The result will be the second input
    # joined to the first input, either by list_write if an MIL list, or
    # append if a python list.
    inputs = _get_inputs(context, node, expected=2)
    ls = inputs[0]
    value = inputs[1]

    if isinstance(ls, list):
        context.add(ls + [value], node.name)
    elif isinstance(ls, ListVar):
        index = mb.list_length(ls=ls, name=node.name + "_index")
        res = mb.list_write(ls=ls, index=index, value=value, name=node.name)
        context.add(res)
    else:
        raise ValueError("can only append to Python list or MIL ListVar, got {}.".format(type(inputs[0])))


@register_torch_op(torch_alias=['abs'])
def _abs(context, node):
    inputs = _get_inputs(context, node, expected=1)
    context.add(mb.abs(x=inputs[0], name=node.name))

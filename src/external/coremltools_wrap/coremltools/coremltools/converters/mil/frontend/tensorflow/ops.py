#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import logging as _logging
from six import string_types as _string_types
import numpy as _np

from coremltools.converters.mil.mil.ops import get_const_mode
from coremltools.converters.mil.mil import Builder as mb
from coremltools.converters.mil.mil.ops.defs._utils import broadcast_shapes
from .convert_utils import convert_graph
from .tf_op_registry import register_tf_op
from coremltools.converters.mil.mil import types
from coremltools.converters.mil.mil.types.symbolic import is_symbolic


def _is_scalar(type_):
    if type_ is None:
        return False
    result = types.is_int(type_) or types.is_float(type_) or types.is_bool(type_)
    if types.is_tensor(type_) and (len(type_.get_shape()) == 0):
        result = True
    return result


def _transpose_NHWC_to_NCHW(x):
    return mb.transpose(x=x, perm=[0, 3, 1, 2])


def _transpose_NCHW_to_NHWC(x, node_name):
    return mb.transpose(x=x, perm=[0, 2, 3, 1], name=node_name)


def _transpose_NDHWC_to_NCDHW(x):
    return mb.transpose(x=x, perm=[0, 4, 1, 2, 3])


def _transpose_NCDHW_to_NDHWC(x, node_name):
    return mb.transpose(x=x, perm=[0, 2, 3, 4, 1], name=node_name)


def _check_axes_type(x):
    if x is None or x.val is None:
        return None
    if isinstance(x.val, _np.int32):
        return _np.array([x.val])
    return x.val


def _value_at(x, idx):
    """
    input x: 1D tensor (vector).
    return value at index idx. x[idx].
    """
    assert x.rank == 1
    return mb.slice_by_index(x=x, begin=[idx], end=[0], squeeze_mask=[True])


@register_tf_op(tf_alias=["BiasAdd", "AddV2"])
def Add(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    x = mb.add(x=x, y=y, name=node.name)
    context.add(node.name, x)


@register_tf_op
def AddN(context, node):
    values = [context[name] for name in node.inputs]
    if len(values) == 1:
        Identity(context, node)
        return
    prev_var = values[0]
    for idx, var in enumerate(values[1:]):
        if var == values[-1]:
            x = mb.add(x=prev_var, y=var, name=node.name)
        else:
            prev_var = mb.add(x=prev_var, y=var, name=node.name+"_tmpAddN_"+str(idx))
    context.add(node.name, x)


@register_tf_op
def Abs(context, node):
    x = context[node.inputs[0]]
    x = mb.abs(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Acos(context, node):
    x = context[node.inputs[0]]
    x = mb.acos(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def All(context, node):
    x = context[node.inputs[0]]
    axes = _check_axes_type(context[node.inputs[1]])
    keep_dims = node.attr.get("keep_dims", False)
    x = mb.reduce_prod(x=x, axes=axes, keep_dims=keep_dims, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Any(context, node):
    x = context[node.inputs[0]]
    axes = _check_axes_type(context[node.inputs[1]])
    keep_dims = node.attr.get("keep_dims", False)
    x = mb.reduce_sum(x=x, axes=axes, keep_dims=keep_dims)
    x = mb.greater(x=x, y=0.0, name=node.name)
    context.add(node.name, x)


@register_tf_op
def ArgMax(context, node):
    x = context[node.inputs[0]]
    axis = context[node.inputs[1]]
    x = mb.reduce_argmax(x=x, axis=axis, name=node.name)
    context.add(node.name, x)


@register_tf_op
def ArgMin(context, node):
    x = context[node.inputs[0]]
    axis = context[node.inputs[1]]
    x = mb.reduce_argmin(x=x, axis=axis, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Asin(context, node):
    x = context[node.inputs[0]]
    x = mb.asin(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Atan(context, node):
    x = context[node.inputs[0]]
    x = mb.atan(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Atanh(context, node):
    x = context[node.inputs[0]]
    x = mb.atanh(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def AvgPool(context, node):
    x = context[node.inputs[0]]
    in_shape = x.sym_type.get_shape()
    d_rank = len(in_shape) - 2
    data_format = node.attr.get("data_format", "NHWC")
    ksize = node.attr.get("ksize", None)
    kernel_sizes = _pool_pads_or_strides(ksize, data_format, d_rank)
    strides = node.attr.get("strides", None)
    if strides is not None:
        strides = _pool_pads_or_strides(strides, data_format, d_rank)
    pad_type = node.attr["padding"].lower()
    if data_format == "NHWC":
        x = _transpose_NHWC_to_NCHW(x)
        x = mb.avg_pool(
            x=x,
            kernel_sizes=kernel_sizes,
            strides=strides,
            pad_type=pad_type,
            exclude_padding_from_average=True,
        )
        x = _transpose_NCHW_to_NHWC(x, node.name)
    else:
        x = mb.avg_pool(
            x=x,
            kernel_sizes=kernel_sizes,
            strides=strides,
            pad_type=pad_type,
            exclude_padding_from_average=True,
            name=node.name,
        )
    context.add(node.name, x)


@register_tf_op
def AvgPool3D(context, node):
    x = context[node.inputs[0]]
    d_rank = x.rank - 2
    data_format = node.attr.get("data_format", "NDHWC")
    ksize = node.attr.get("ksize", None)
    kernel_sizes = _pool_pads_or_strides(ksize, data_format, d_rank)
    strides = node.attr.get("strides", None)
    if strides is not None:
        strides = _pool_pads_or_strides(strides, data_format, d_rank)
    pad_type = node.attr["padding"].lower()
    if data_format == "NDHWC":
        x = _transpose_NDHWC_to_NCDHW(x)
        x = mb.avg_pool(
            x=x,
            kernel_sizes=kernel_sizes,
            strides=strides,
            pad_type=pad_type,
            exclude_padding_from_average=True,
        )
        x = _transpose_NCDHW_to_NDHWC(x, node.name)
    else:
        x = mb.avg_pool(
            x=x,
            kernel_sizes=kernel_sizes,
            strides=strides,
            pad_type=pad_type,
            exclude_padding_from_average=True,
            name=node.name,
        )

    context.add(node.name, x)


@register_tf_op
def BatchToSpaceND(context, node):
    x = context[node.inputs[0]]
    block_shape = context[node.inputs[1]].val
    crops = context[node.inputs[2]].val

    if x.rank != 3 and x.rank != 4:
        raise NotImplementedError("rank of input must be 3 or 4!")

    if block_shape is None or crops is None:
        raise NotImplementedError(
            "Not support dynamic block_shape and paddings for BatchToSpaceND!"
        )

    if len(block_shape.flatten()) > 2:
        raise NotImplementedError("rank of spatial shape > 2 is not yet supported")

    if x.rank == 3 or (x.rank == 4 and len(block_shape) == 1):

        input_shape = mb.shape(x=x)
        rank = x.rank
        spatial_rank = len(block_shape)

        # reshape input to [block_shape] + [batch_size/prod(block_shape)] + x.shape[1:]
        batch_size = _value_at(input_shape, 0)
        block_shape_prod = _np.prod(block_shape)
        resize_batch_size = mb.real_div(x=batch_size, y=block_shape_prod)
        resize_batch_size = [mb.cast(x=resize_batch_size, dtype="int32")]
        remain_dims = [_value_at(input_shape, i) for i in range(1, rank)]
        block_dims = [dim for dim in block_shape]
        reshape_values = block_dims + resize_batch_size + remain_dims
        reshape_shape = mb.concat(values=reshape_values, axis=0)
        reshaped = mb.reshape(x=x, shape=reshape_shape)

        # permute the tensor to shape [batch / prod(block_shape)] +
        #                             [input_shape[1], block_shape[0], ..., input_shape[M], block_shape[M-1]] +
        #                             [input_shape[M+1], ..., input_shape[N-1]]
        block_shape_dims = list(range(spatial_rank))
        batch_dim = [spatial_rank]
        input_shape_dims = list(range(spatial_rank + 1, reshaped.rank))
        perm = [batch_dim[0]]
        for i in range(spatial_rank):
            perm += [input_shape_dims[i], block_shape_dims[i]]
        perm += input_shape_dims[spatial_rank:]
        permuted = mb.transpose(x=reshaped, perm=perm)

        # reshape tensor to shape [batch / prod(block_shape)] +
        #                         [input_shape[1] * block_shape[0], ..., input_shape[M] * block_shape[M-1]] +
        #                         [input_shape[M+1], ..., input_shape[N-1]]
        spatial_dims = []
        for i in range(spatial_rank):
            spatial_dims.append(
                mb.mul(x=_value_at(input_shape, i + 1), y=block_shape[i])
            )
        remain_dims = [_value_at(input_shape, i) for i in range(spatial_rank + 1, rank)]
        reshape_values = resize_batch_size + spatial_dims + remain_dims
        reshape_shape = mb.concat(values=reshape_values, axis=0)
        reshape_permuted = mb.reshape(x=permuted, shape=reshape_shape)

        # crop the tensor using stride slice
        begin = [0]
        for i in range(spatial_rank):
            begin.append(crops[i][0])
        for i in range(spatial_rank + 1, rank):
            begin.append(0)
        end = [resize_batch_size[0]]
        for i in range(spatial_rank):
            end.append(mb.sub(x=spatial_dims[i], y=crops[i][1]))
        end += remain_dims
        end = mb.concat(values=end, axis=0)
        x = mb.slice_by_index(x=reshape_permuted, begin=begin, end=end, name=node.name)
    else:
        if len(block_shape.flatten()) != 2:
            raise NotImplementedError(
                "rank of spatial shape != 2 is not yet supported for 4d input."
            )
        if block_shape[0] != block_shape[1]:
            raise NotImplementedError("non-equal block shape is not yet supported")

        needs_cropping = any(crops.flatten())

        x = mb.transpose(x=x, perm=[3, 0, 1, 2])

        x = mb.depth_to_space(x=x, block_size=block_shape[0])
        if needs_cropping:
            x = mb.crop(
                x=x,
                crop_height=[crops[0][0], crops[0][1]],
                crop_width=[crops[1][0], crops[1][1]],
            )

        x = mb.transpose(x=x, perm=[1, 2, 3, 0], name=node.name)
    context.add(node.name, x)


@register_tf_op
def Ceil(context, node):
    x = context[node.inputs[0]]
    x = mb.ceil(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Const(context, node):
    if node.value is None:
        raise ValueError("Const node '{}' cannot have no value".format(node.name))
    mode = get_const_mode(node.value.val)
    x = mb.const(val=node.value.val, mode=mode, name=node.name)
    context.add(node.name, x)


def _conv2d3d_strides_or_dilations(name, value, data_format, default_value=1):
    """Compute strides or dilation values for 2D and 3D convolutions."""
    if value is None:
        value = default_value
    if not isinstance(value, (int, list)):
        raise ValueError("{} must be an int or list".format(name))

    # Parse number of spatial dimensions from `data_format`, assuming N (batch) and C
    # (input channels) are present
    n_dims = len(data_format) - 2

    if isinstance(value, int):
        return [value] * n_dims

    if len(value) == 1:
        return value * n_dims
    if len(value) == n_dims:
        return value
    if len(value) != n_dims + 2:
        raise ValueError(
            "{} must have length 1, {}, or {}".format(name, n_dims, n_dims + 2)
        )

    if data_format == "NHWC":
        # Only support stride/dilation along N, C == 1
        if not (value[0] == value[3] == 1):
            raise ValueError(
                "{} along N and C other than 1 not implemented".format(name)
            )
        return value[1:3]
    elif data_format == "NCHW" or data_format == "NCDHW":
        if not (value[0] == value[1] == 1):
            raise ValueError(
                "{} along N and C other than 1 not implemented".format(name)
            )
        return value[2:]
    # "NDHWC"
    if not (value[0] == value[4] == 1):
        raise ValueError("{} along N and C other than 1 not implemented".format(name))
    return value[1:4]


@register_tf_op
def Cos(context, node):
    x = context[node.inputs[0]]
    x = mb.cos(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Cosh(context, node):
    x = context[node.inputs[0]]
    x = mb.cosh(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Einsum(context, node):
    equation = node.attr["equation"]
    if equation == "bnqd,bnkd->bnqk":
        a = context[node.inputs[0]]
        b = context[node.inputs[1]]
        x = mb.matmul(x=a, y=b, transpose_x=False, transpose_y=True, name=node.name)
        context.add(node.name, x)
    elif equation == "abc,cd->abd":
        a = context[node.inputs[0]]
        b = context[node.inputs[1]]
        x = mb.matmul(x=a, y=b, transpose_x=False, transpose_y=False, name=node.name)
        context.add(node.name, x)
    elif equation == "abc,cde->abde":
        a = context[node.inputs[0]]
        b = context[node.inputs[1]]
        x_1 = mb.reshape(x=a, shape=[a.shape[0] * a.shape[1], a.shape[2]])
        x_2 = mb.reshape(x=b, shape=[b.shape[0], b.shape[1] * b.shape[2]])
        x = mb.matmul(x=x_1, y=x_2, transpose_x=False, transpose_y=False)
        x = mb.reshape(
            x=x, shape=[a.shape[0], a.shape[1], b.shape[1], b.shape[2]], name=node.name
        )
        context.add(node.name, x)
    elif equation == "BTNH,BFNH->BNFT":
        a = context[node.inputs[0]]
        b = context[node.inputs[1]]
        a = mb.transpose(x=a, perm=[0, 2, 1, 3])
        b = mb.transpose(x=b, perm=[0, 2, 1, 3])
        x = mb.matmul(x=b, y=a, transpose_x=False, transpose_y=True, name=node.name)
        context.add(node.name, x)
    elif equation == "BNFT,BTNH->BFNH":
        a = context[node.inputs[0]]
        b = context[node.inputs[1]]
        b = mb.transpose(x=b, perm=[0, 2, 1, 3])
        x = mb.matmul(x=a, y=b, transpose_x=False, transpose_y=False)
        x = mb.transpose(x=x, perm=[0, 2, 1, 3], name=node.name)
        context.add(node.name, x)
    elif equation == "abcd,cde->abe":
        a = context[node.inputs[0]]
        b = context[node.inputs[1]]
        x_1 = mb.reshape(x=a, shape=[a.shape[0], a.shape[1], a.shape[2] * a.shape[3]])
        x_2 = mb.reshape(x=b, shape=[b.shape[0] * b.shape[1], b.shape[2]])
        x = mb.matmul(
            x=x_1, y=x_2, transpose_x=False, transpose_y=False, name=node.name
        )
        context.add(node.name, x)
    else:
        raise NotImplementedError(
            "Einsum unsupported equation format: ", node.attr["equation"]
        )


@register_tf_op
def Equal(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    x = mb.equal(x=x, y=y, name=node.name)
    context.add(node.name, x)


@register_tf_op
def ExtractImagePatches(context, node):
    x = context[node.inputs[0]]
    sizes = node.attr.get("ksizes")
    strides = node.attr.get("strides")
    rates = node.attr.get("rates")
    padding = node.attr.get("padding")
    if x.rank != 4:
        raise ValueError("input for ExtractImagePatches should be a 4D tensor.")
    if not all([rate == 1 for rate in rates]):
        raise NotImplementedError(
            "only rates with all 1s is implemented for ExtractImagePatches."
        )
    if len(sizes) != 4 or sizes[0] != 1 or sizes[3] != 1:
        raise ValueError(
            "ExtractImagePatches only supports sizes (4D tensor) with 1s for batch and channel dimensions."
        )
    if len(sizes) != 4 or strides[0] != 1 or strides[3] != 1:
        raise ValueError(
            "ExtractImagePatches only supports strides (4D tensor) with 1s for batch and channel dimensions."
        )
    if not padding in ["VALID", "SAME"]:
        raise ValueError("non-supported padding for ExtractImagePatches.")
    h, w = x.shape[1], x.shape[2]

    # padding for SAME mode
    if padding == "SAME":
        delta_h = h % strides[1] if h % strides[1] != 0 else strides[1]
        delta_w = w % strides[2] if w % strides[2] != 0 else strides[2]
        last_h = h - delta_h + 1
        last_w = w - delta_w + 1
        pad_h = max(0, last_h + sizes[1] - 1 - h)
        pad_w = max(0, last_w + sizes[2] - 1 - w)
        pad_h = [pad_h // 2, pad_h // 2 if pad_h % 2 == 0 else pad_h // 2 + 1]
        pad_w = [pad_w // 2, pad_w // 2 if pad_w % 2 == 0 else pad_w // 2 + 1]
        pad = _np.array([[0, 0], pad_h, pad_w, [0, 0]]).astype(_np.int32)
        pad = pad.reshape(-1)
        if not all(pad == 0):
            x = mb.pad(x=x, pad=pad, mode="constant", constant_val=0.0)
            h, w = x.shape[1], x.shape[2]

    # compute boxes
    batch = x.shape[0]
    boxes = []
    h_index = list(range(0, h - sizes[1] + 1, strides[1]))
    w_index = list(range(0, w - sizes[2] + 1, strides[2]))
    for hi in h_index:
        for wi in w_index:
            boxes.append((hi, wi, hi + sizes[1] - 1, wi + sizes[2] - 1))

    boxes = _np.array(boxes)
    box_indices = _np.arange(batch)
    box_indices = _np.tile(box_indices, (len(boxes), 1))
    box_indices = _np.transpose(box_indices)
    box_indices = box_indices.reshape(-1, 1)
    boxes = _np.tile(boxes, (batch, 1))
    boxes = _np.concatenate([box_indices, boxes], axis=1)
    boxes = boxes.reshape(boxes.shape[0], 1, boxes.shape[1], 1, 1)

    # use crop_and_resize
    x = _transpose_NHWC_to_NCHW(x)
    x = mb.crop_resize(
        x=x,
        roi=boxes,
        target_height=sizes[1],
        target_width=sizes[2],
        normalized_coordinates=False,
        spatial_scale=1.0,
        box_coordinate_mode="CORNERS_HEIGHT_FIRST",
        sampling_mode="ALIGN_CORNERS",
    )
    x = mb.squeeze(x=x, axes=[1])
    x = _transpose_NCHW_to_NHWC(x, node_name=node.name + "_transpose_to_nhwc")
    x = mb.reshape(x=x, shape=(batch, len(h_index), len(w_index), -1), name=node.name)
    context.add(node.name, x)


@register_tf_op
def Exp(context, node):
    x = context[node.inputs[0]]
    x = mb.exp(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Floor(context, node):
    x = context[node.inputs[0]]
    x = mb.floor(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def FloorDiv(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    x = mb.floor_div(x=x, y=y, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Greater(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    x = mb.greater(x=x, y=y, name=node.name)
    context.add(node.name, x)


@register_tf_op
def GreaterEqual(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    x = mb.greater_equal(x=x, y=y, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Less(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    x = mb.less(x=x, y=y, name=node.name)
    context.add(node.name, x)


@register_tf_op
def LessEqual(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    x = mb.less_equal(x=x, y=y, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Log(context, node):
    x = context[node.inputs[0]]
    x = mb.log(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def LogicalAnd(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    x = mb.logical_and(x=x, y=y, name=node.name)
    context.add(node.name, x)


@register_tf_op
def LogicalNot(context, node):
    x = context[node.inputs[0]]
    x = mb.logical_not(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def LogicalOr(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    x = mb.logical_or(x=x, y=y, name=node.name)
    context.add(node.name, x)


@register_tf_op
def LogicalXor(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    x = mb.logical_xor(x=x, y=y, name=node.name)
    context.add(node.name, x)


@register_tf_op
def LRN(context, node):
    x = context[node.inputs[0]]
    depth_radius = node.attr.get("depth_radius")
    size = (depth_radius * 2) + 1
    alpha = node.attr.get("alpha") * size
    beta = node.attr.get("beta")
    bias = node.attr.get("bias")
    x = _transpose_NHWC_to_NCHW(x)
    x = mb.local_response_norm(x=x, size=size, alpha=alpha, beta=beta, k=bias)
    x = _transpose_NCHW_to_NHWC(x, node.name)
    context.add(node.name, x)


@register_tf_op
def Maximum(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    x = mb.maximum(x=x, y=y, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Minimum(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    x = mb.minimum(x=x, y=y, name=node.name)
    context.add(node.name, x)


@register_tf_op
def FloorMod(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    floor = mb.floor_div(x=x, y=y, name=node.name + "_floor_div")
    floor_mutiply = mb.mul(x=floor, y=y, name=node.name + "_multiply")
    x = mb.sub(x=x, y=floor_mutiply, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Mul(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    x = mb.mul(x=x, y=y, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Neg(context, node):
    x = context[node.inputs[0]]
    x = mb.mul(x=x, y=-1, name=node.name)
    context.add(node.name, x)


@register_tf_op
def NotEqual(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    x = mb.not_equal(x=x, y=y, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Pow(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    x = mb.pow(x=x, y=y, name=node.name)
    context.add(node.name, x)


@register_tf_op
def DepthwiseConv2dNative(context, node):
    # [kH, kW, C_in, multiplier]
    W_hwim = context[node.inputs[1]]  # m = multiplier
    # [kH, kW, 1, C_in * multipler]
    shape_hw1o = list(W_hwim.shape[:2]) + [1, W_hwim.shape[2] * W_hwim.shape[3]]
    W_hw1o = mb.reshape(x=W_hwim, shape=shape_hw1o)
    # [C_in * multipler, 1, kH, kW]. Note that C_in * multiplier = C_out in
    # MIL. C_in / groups = 1 in depthwise conv.
    W_o1hw = mb.transpose(x=W_hw1o, perm=[3, 2, 0, 1])
    data_format = node.attr.get("data_format", "NHWC")
    HW_dilations = _conv2d3d_strides_or_dilations(
        "dilations", node.attr.get("dilations"), data_format
    )
    HW_strides = _conv2d3d_strides_or_dilations(
        "strides", node.attr.get("strides"), data_format
    )

    pad_type = node.attr.get("padding")
    if pad_type not in ["VALID", "SAME"]:
        raise ValueError("Invalid padding type for tf.nn.depthwise_conv2d")

    pad_type = pad_type.lower()
    x = context[node.inputs[0]]
    C_in = x.shape[-1]
    if data_format == "NHWC":
        x = _transpose_NHWC_to_NCHW(x)
    # Only the last op should have the same name as node.name
    conv_name = node.name + "x" if data_format == "NHWC" else node.name
    x = mb.conv(
        x=x,
        weight=W_o1hw,
        pad_type=pad_type,
        strides=HW_strides,
        dilations=HW_dilations,
        groups=C_in,
        name=conv_name,
    )
    if data_format == "NHWC":
        x = _transpose_NCHW_to_NHWC(x, node.name)
    context.add(node.name, x)


@register_tf_op
def Conv2D(context, node):
    W_hwio = context[node.inputs[1]]
    W_oihw = mb.transpose(x=W_hwio, perm=[3, 2, 0, 1])
    data_format = node.attr.get("data_format", "NHWC")
    HW_dilations = _conv2d3d_strides_or_dilations(
        "dilations", node.attr.get("dilations"), data_format
    )
    HW_strides = _conv2d3d_strides_or_dilations(
        "strides", node.attr.get("strides"), data_format
    )

    pad_type = node.attr.get("padding")
    pad_type = pad_type.lower()
    pad_type = "custom" if pad_type == "explicit" else pad_type
    assert pad_type in {"same", "valid", "custom"}
    x = context[node.inputs[0]]
    if data_format == "NHWC":
        x = _transpose_NHWC_to_NCHW(x)
        if pad_type == "custom":
            pad_val = node.attr["explicit_paddings"]
            pad_val = pad_val[2:-2]
    elif data_format == "NCHW" and pad_type == "custom":
        pad_val = node.attr["explicit_paddings"]
        pad_val = pad_val[4:]
    # Only the last op should have the same name as node.name
    conv_name = node.name + "x" if data_format == "NHWC" else node.name
    if pad_type == "custom":
        x = mb.conv(
            x=x,
            weight=W_oihw,
            pad_type=pad_type,
            strides=HW_strides,
            dilations=HW_dilations,
            name=conv_name,
            pad=pad_val,
        )
    else:
        x = mb.conv(
            x=x,
            weight=W_oihw,
            pad_type=pad_type,
            strides=HW_strides,
            dilations=HW_dilations,
            name=conv_name,
        )
    if data_format == "NHWC":
        x = _transpose_NCHW_to_NHWC(x, node.name)
    context.add(node.name, x)


@register_tf_op
def Conv3D(context, node):
    W_dhwio = context[node.inputs[1]]
    W_oidhw = mb.transpose(x=W_dhwio, perm=[4, 3, 0, 1, 2])
    data_format = node.attr.get("data_format", "NDHWC")
    DHW_dilations = _conv2d3d_strides_or_dilations(
        "dilations", node.attr.get("dilations"), data_format
    )
    DHW_strides = _conv2d3d_strides_or_dilations(
        "strides", node.attr.get("strides"), data_format
    )

    pad_type = node.attr.get("padding")
    if not isinstance(pad_type, _string_types):
        pad_type = "custom"
        raise NotImplementedError("Custom padding not implemented for TF")
    pad_type = pad_type.lower()
    x = context[node.inputs[0]]
    if data_format == "NDHWC":
        # Convert input to NCDHW
        x = _transpose_NDHWC_to_NCDHW(x)
    # Only the last op should have the same name as node.name
    conv_name = node.name + "x" if data_format == "NDHWC" else node.name
    x = mb.conv(
        x=x,
        weight=W_oidhw,
        pad_type=pad_type,
        strides=DHW_strides,
        dilations=DHW_dilations,
        name=conv_name,
    )
    if data_format == "NDHWC":
        # Convert input back to NDHWC (from NCDHW)
        x = _transpose_NCDHW_to_NDHWC(x, node.name)
    context.add(node.name, x)


@register_tf_op
def Conv3DBackpropInputV2(context, node):
    # Output shape: [N, D_out, H_out, W_out, C_out]
    output_shape = context[node.inputs[0]].val
    # Weight shape: [D, H, W, C_out, C_in]
    W_dhwoi = context[node.inputs[1]]
    W_oidhw = mb.transpose(x=W_dhwoi, perm=[3, 4, 0, 1, 2])
    # Input shape: [N, D_in, H_in, W_in, C_in]
    x = context[node.inputs[2]]

    data_format = node.attr.get("data_format", "NDHWC")
    DHW_dilations = _conv2d3d_strides_or_dilations(
        "dilations", node.attr.get("dilations"), data_format
    )
    DHW_strides = _conv2d3d_strides_or_dilations(
        "strides", node.attr.get("strides"), data_format
    )
    pad_type = node.attr.get("padding", None)

    if pad_type is None:
        raise ValueError("Padding type not specified for op: {}".format(node.name))

    if not isinstance(pad_type, _string_types):
        pad_type = "custom"
        raise NotImplementedError("Custom padding not implemented for TF")
    pad_type = pad_type.lower()

    if data_format == "NDHWC":
        # Convert input to NCDHW
        x = _transpose_NDHWC_to_NCDHW(x)
        if output_shape is not None:
            output_shape = [output_shape[1], output_shape[2], output_shape[3]]
    else:
        if output_shape is not None:
            output_shape = [output_shape[2], output_shape[3], output_shape[4]]

    # Only the last op should have the same name as node.name
    conv_name = node.name + "_x" if data_format == "NDHWC" else node.name
    # Pass output shape provided above
    # TODO: rdar://63968613 ([deconv3d] Deconv_3d top_shapes_for_bottom_shapes does not sets output channel if output shape is provided)
    x = mb.conv_transpose(
        x=x,
        weight=W_oidhw,
        pad_type=pad_type,
        strides=DHW_strides,
        output_shape=None,
        dilations=DHW_dilations,
        name=conv_name,
    )
    if data_format == "NDHWC":
        # Convert input back to NDHWC (from NCDHW)
        x = _transpose_NCDHW_to_NDHWC(x, node.name)
    context.add(node.name, x)


@register_tf_op
def DepthToSpace(context, node):
    x = context[node.inputs[0]]
    block_size = node.attr.get("block_size")
    data_format = node.attr.get("data_format", "NHWC")
    if data_format == "NHWC":
        x = _transpose_NHWC_to_NCHW(x)
        x = mb.depth_to_space(x=x, block_size=block_size)
        x = _transpose_NCHW_to_NHWC(x, node.name)
    else:
        x = mb.depth_to_space(x=x, block_size=block_size, name=node.name)
    context.add(node.name, x)


@register_tf_op
def EuclideanNorm(context, node):
    x = context[node.inputs[0]]
    axes = _check_axes_type(context[node.inputs[1]])
    keep_dims = node.attr.get("keep_dims", False)
    x = mb.reduce_l2_norm(x=x, axes=axes, keep_dims=keep_dims, name=node.name)
    context.add(node.name, x)


@register_tf_op
def ExpandDims(context, node):
    x = context[node.inputs[0]]
    axis = context[node.inputs[1]]
    if axis.op.op_type == "const" and (axis.val is not None and axis.val.size == 1):
        axis = axis.val[0] if axis.shape == (1,) else axis.val
    else:
        raise ValueError("Expand Dims: Invalid value for parameter axis")
    x = mb.expand_dims(x=x, axes=[axis], name=node.name)
    context.add(node.name, x)


@register_tf_op(tf_alias=["FusedBatchNormV2", "FusedBatchNormV3"])
def FusedBatchNorm(context, node):
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
        x = _transpose_NCHW_to_NHWC(x, node.name + ":0")
    else:
        x = mb.batch_norm(
            x=x,
            mean=mean,
            variance=variance,
            gamma=scale,
            beta=offset,
            epsilon=epsilon,
            name=node.name + ":0",
        )
    # Inference only batch norm does not have meaningful outputs for
    # batch_mean, batch_variance etc.
    context.add(node.name, [x, mean, variance])


@register_tf_op
def Fill(context, node):
    shape = context[node.inputs[0]]
    value = context[node.inputs[1]]
    x = mb.fill(shape=shape, value=value, name=node.name)
    context.add(node.name, x)


@register_tf_op
def RealDiv(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    x = mb.real_div(x=x, y=y, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Rsqrt(context, node):
    x = context[node.inputs[0]]
    x = mb.rsqrt(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Sub(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    x = mb.sub(x=x, y=y, name=node.name)
    context.add(node.name, x)


@register_tf_op
def StopGradient(context, node):
    Identity(context, node)


@register_tf_op
def Identity(context, node):
    x = context[node.inputs[0]]
    # In many cases we can skip and just make downstream ops reference the
    # pre-identity op. However, when identity is an output or pre-identity
    # is a placeholder, an identity op, or mb.mul(x, 1.0) is required.
    if len(node.outputs) != 0 or x.op is not None:
        context.add(node.name, x, is_new_var=False)
    else:
        x = mb.mul(x=x, y=1.0, name=node.name)
        context.add(node.name, x)


@register_tf_op
def Print(context, node):
    Identity(context, node)


@register_tf_op
def Placeholder(context, node):
    # no-op as we add Placeholder separately.
    pass


def _pool_pads_or_strides(tf_spec, data_format, d_rank):
    if tf_spec is None:
        d_spec = [1] * d_rank
    elif not isinstance(tf_spec, list):
        d_spec = [tf_spec] * d_rank
    elif len(tf_spec) == 2:
        d_spec = tf_spec
    elif len(tf_spec) == 4:
        if data_format == "NHWC":
            d_spec = tf_spec[1:3]
        else:
            d_spec = tf_spec[2:]
    elif len(tf_spec) == 5:
        if data_format == "NDHWC":
            d_spec = tf_spec[1:4]
        else:
            # NCDHW
            d_spec = tf_spec[2:]
    else:
        raise ValueError("Unsupported tf_spec: %s" % tf_spec)
    return d_spec


@register_tf_op(tf_alias=["BatchMatMul", "BatchMatMulV2"])
def MatMul(context, node):
    a = context[node.inputs[0]]
    b = context[node.inputs[1]]
    transpose_a = node.attr.get("adj_x", False) or node.attr.get("transpose_a", False)
    transpose_b = node.attr.get("adj_y", False) or node.attr.get("transpose_b", False)
    x = mb.matmul(
        x=a, y=b, transpose_x=transpose_a, transpose_y=transpose_b, name=node.name
    )
    context.add(node.name, x)


@register_tf_op
def MaxPool(context, node):
    x = context[node.inputs[0]]
    in_shape = x.sym_type.get_shape()
    d_rank = len(in_shape) - 2
    data_format = node.attr.get("data_format", "NHWC")
    ksize = node.attr.get("ksize", None)
    kernel_sizes = _pool_pads_or_strides(ksize, data_format, d_rank)
    strides = node.attr.get("strides", None)
    if strides is not None:
        strides = _pool_pads_or_strides(strides, data_format, d_rank)
    pad_type = node.attr["padding"].lower()
    if data_format == "NHWC":
        x = _transpose_NHWC_to_NCHW(x)
        x = mb.max_pool(
            x=x, kernel_sizes=kernel_sizes, strides=strides, pad_type=pad_type
        )
        x = _transpose_NCHW_to_NHWC(x, node.name)
    else:
        x = mb.max_pool(
            x=x,
            kernel_sizes=kernel_sizes,
            strides=strides,
            pad_type=pad_type,
            name=node.name,
        )
    context.add(node.name, x)


@register_tf_op
def MaxPool3D(context, node):
    x = context[node.inputs[0]]
    d_rank = x.rank - 2
    data_format = node.attr.get("data_format", "NDHWC")
    ksize = node.attr.get("ksize", None)
    kernel_sizes = _pool_pads_or_strides(ksize, data_format, d_rank)
    strides = node.attr.get("strides", None)
    if strides is not None:
        strides = _pool_pads_or_strides(strides, data_format, d_rank)
    pad_type = node.attr["padding"].lower()
    if data_format == "NDHWC":
        x = _transpose_NDHWC_to_NCDHW(x)
        x = mb.max_pool(
            x=x, kernel_sizes=kernel_sizes, strides=strides, pad_type=pad_type
        )
        x = _transpose_NCDHW_to_NDHWC(x, node.name)
    else:
        x = mb.max_pool(
            x=x,
            kernel_sizes=kernel_sizes,
            strides=strides,
            pad_type=pad_type,
            name=node.name,
        )

    context.add(node.name, x)


@register_tf_op
def MatrixBandPart(context, node):
    x = context[node.inputs[0]]
    lower = context[node.inputs[1]]
    upper = context[node.inputs[2]]
    x = mb.band_part(x=x, lower=lower, upper=upper, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Max(context, node):
    x = context[node.inputs[0]]
    axes = _check_axes_type(context[node.inputs[1]])
    keep_dims = node.attr.get("keep_dims", False)
    x = mb.reduce_max(x=x, axes=axes, keep_dims=keep_dims, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Min(context, node):
    x = context[node.inputs[0]]
    axes = _check_axes_type(context[node.inputs[1]])
    keep_dims = node.attr.get("keep_dims", False)
    x = mb.reduce_min(x=x, axes=axes, keep_dims=keep_dims, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Prod(context, node):
    x = context[node.inputs[0]]
    axes = _check_axes_type(context[node.inputs[1]])
    keep_dims = node.attr.get("keep_dims", False)
    x = mb.reduce_prod(x=x, axes=axes, keep_dims=keep_dims, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Cast(context, node):
    type_map = {
        types.float: "fp32",
        types.double: "fp64",
        types.int32: "int32",
        types.int64: "int64",
    }
    if node.attr["DstT"] not in type_map.keys():
        raise NotImplementedError(
            "Cast: Provided destination type {} not "
            "supported.".format(types.get_type_info(node.attr["DstT"]))
        )
    x = context[node.inputs[0]]
    dtype = type_map[node.attr["DstT"]]
    x = mb.cast(x=x, dtype=dtype, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Round(context, node):
    x = context[node.inputs[0]]
    x = mb.round(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Sign(context, node):
    x = context[node.inputs[0]]
    x = mb.sign(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Sin(context, node):
    x = context[node.inputs[0]]
    x = mb.sin(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Sinh(context, node):
    x = context[node.inputs[0]]
    x = mb.sinh(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Slice(context, node):
    x = context[node.inputs[0]]
    begin = context[node.inputs[1]]
    size = context[node.inputs[2]]
    res = mb.slice_by_size(x=x, begin=begin, size=size, name=node.name)
    context.add(node.name, res)


@register_tf_op
def Sqrt(context, node):
    x = context[node.inputs[0]]
    x = mb.sqrt(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Square(context, node):
    x = context[node.inputs[0]]
    x = mb.square(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def StridedSlice(context, node):
    x = context[node.inputs[0]]
    begin = context[node.inputs[1]]
    end = context[node.inputs[2]]
    stride = context[node.inputs[3]]

    def bitmask_to_array(bit):
        arr = []
        while bit > 0:
            if bit & 1:
                arr.append(True)
            else:
                arr.append(False)
            bit >>= 1
        return arr

    begin_mask = bitmask_to_array(node.attr.get("begin_mask", 0))
    end_mask = bitmask_to_array(node.attr.get("end_mask", 0))
    squeeze_mask = bitmask_to_array(node.attr.get("shrink_axis_mask", 0))
    ellipsis_mask = bitmask_to_array(node.attr.get("ellipsis_mask", 0))
    new_axis_mask = bitmask_to_array(node.attr.get("new_axis_mask", 0))

    def _pad_mask(
        x,
        begin,
        end,
        stride,
        begin_mask,
        end_mask,
        squeeze_mask,
        ellipsis_mask,
        new_axis_mask,
    ):
        # This function pad the masks, stride, begin and end to the same rank as the input tensor.
        if begin.rank != 1:
            raise ValueError(
                "begin should be 1-D tensor, got {}-D tensor instead".format(begin.rank)
            )
        if end.rank != 1:
            raise ValueError(
                "end should be 1-D tensor, got {}-D tensor instead".format(end.rank)
            )

        # check if inputs can be determined
        begin_cache = begin
        end_cache = end
        begin = [] if begin.val is None else begin.val.tolist()
        end = [] if end.val is None else end.val.tolist()
        stride = [] if stride is None else stride.val.tolist()

        # pad masks function
        new_dims = sum(i == True for i in new_axis_mask)
        if new_dims > 0:
            x_rank = x.rank + new_dims
        else:
            x_rank = x.rank

        def pad_array(arr, max_rank, idx, default_value):
            """
            This function pads the arr to x_rank with default_value.
            idx is the index where ellipis_mask = True.
            max_rank is the maximum rank of the masks, stride, begin and end.
            """
            mask = arr[:]
            mask += [default_value] * (x_rank - len(mask))
            new_mask = []

            for i in range(max_rank):
                num = 1 if i != idx else x_rank - max_rank + 1
                new_mask += [mask[i]] * num
            return new_mask

        mask_list = [
            begin_mask,
            end_mask,
            squeeze_mask,
            ellipsis_mask,
            new_axis_mask,
            stride,
            begin,
            end,
        ]
        max_rank = max([len(arr) for arr in mask_list])

        # If ellipsis_mask is given, the last element of it would be True
        # Otherwise, we simply pad each mask by appending default value
        if ellipsis_mask != []:
            rank = max_rank
            idx = len(ellipsis_mask) - 1
        else:
            rank = x_rank
            idx = -1

        begin_mask = pad_array(begin_mask, rank, idx, False)
        end_mask = pad_array(end_mask, rank, idx, False)
        squeeze_mask = pad_array(squeeze_mask, rank, idx, False)
        ellipsis_mask = pad_array(ellipsis_mask, rank, idx, False)
        new_axis_mask = pad_array(new_axis_mask, rank, idx, False)
        stride = pad_array(stride, rank, idx, 1)

        # pad begin and end if they are determined during compile time
        if begin != []:
            begin = pad_array(begin, rank, idx, 0)
        if end != []:
            end = pad_array(end, rank, idx, 0)

        # make sure begin_mask, end_mask, and stride are consistent with ellipsis mask
        # begin_mask and end_mask should be True, and stride should be 1.
        for i, mask in enumerate(ellipsis_mask):
            if mask:
                begin_mask[i] = True
                end_mask[i] = True
                stride[i] = 1

        # make sure begin_mask, end_mask, and stride are consistent with new axis mask
        # begin_mask and end_mask should be True, and stride should be 1.
        for i, mask in enumerate(new_axis_mask):
            if mask:
                begin_mask[i] = True
                end_mask[i] = True
                stride[i] = 1

        # convert begin and end back to cache value if they are run-time determined
        if begin == []:
            begin = begin_cache

        if end == []:
            end = end_cache

        # check which mask is adding by our default value
        # This happens when the given index is less than the tensor rank,
        # for instance, indexing a 3D tensor A with A[:1, :1] is equivalent to
        # A[:1, :1, :]. In this case we should append True to begin_mask and end_mask
        if ellipsis_mask == [False] * x_rank:
            for i in range(max_rank, x_rank):
                begin_mask[i] = True
                end_mask[i] = True

        return begin, end, stride, begin_mask, end_mask, squeeze_mask, new_axis_mask

    begin, end, stride, begin_mask, end_mask, squeeze_mask, new_axis_mask = _pad_mask(
        x,
        begin,
        end,
        stride,
        begin_mask,
        end_mask,
        squeeze_mask,
        ellipsis_mask,
        new_axis_mask,
    )

    if sum(i == True for i in new_axis_mask) > 0:
        axes = [i for i, val in enumerate(new_axis_mask) if val == True]
        x = mb.expand_dims(x=x, axes=axes, name=node.name + "_new_axes")

    x = mb.slice_by_index(
        x=x,
        name=node.name,
        begin=begin,
        end=end,
        stride=stride,
        begin_mask=begin_mask,
        end_mask=end_mask,
        squeeze_mask=squeeze_mask,
    )

    context.add(node.name, x)


@register_tf_op
def Sum(context, node):
    x = context[node.inputs[0]]
    axes = _check_axes_type(context[node.inputs[1]])
    keep_dims = node.attr.get("keep_dims", False)
    x = mb.reduce_sum(x=x, axes=axes, keep_dims=keep_dims, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Tan(context, node):
    x = context[node.inputs[0]]
    x = mb.tan(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def get_tuple(context, node):
    x = context[node.inputs[0]]
    if not isinstance(x, (list, tuple)):
        raise ValueError(
            "Op '{}' should return multiple output.".format(node.inputs[0])
        )
    idx = node.attr["index"]
    if idx >= len(x):
        msg = "Index {} out of range, op '{}' only has {} outputs: {}"
        raise IndexError(msg.format(idx, node.inputs[0], len(x), [v.name for v in x]))
    context.add(node.name, x[idx], is_new_var=False)


@register_tf_op
def Mean(context, node):
    x = context[node.inputs[0]]
    axes = _check_axes_type(context[node.inputs[1]])
    keep_dims = node.attr.get("keep_dims", False)
    x = mb.reduce_mean(x=x, axes=axes, keep_dims=keep_dims, name=node.name)
    context.add(node.name, x)


@register_tf_op
def MatrixDiag(context, node):
    x = context[node.inputs[0]]
    if x.rank != 1:
        raise NotImplementedError('Only support MatrixDiag op with input rank = 1.')
    length = mb.shape(x=x)
    x = mb.expand_dims(x=x, axes=[0])
    reps = mb.concat(values=[length,[1]], axis=0)
    x = mb.tile(x=x, reps=reps)
    x = mb.band_part(x=x, lower=0, upper=0, name=node.name)
    context.add(node.name, x)


@register_tf_op
def MirrorPad(context, node):
    x = context[node.inputs[0]]
    pad = context[node.inputs[1]]
    constant_val = node.attr.get("constant_val", 0.0)

    if pad is None:
        raise ValueError("TF `paddings` in Pad op must be const.")

    mode = node.attr.get("mode", "reflect").lower()
    in_rank = len(x.sym_type.get_shape())

    if in_rank > 5 or in_rank < 2:
        raise ValueError(
            "Unsupported Pad configuration with input rank {}!".format(str(in_rank))
        )

    if pad.val.shape != (in_rank, 2):
        raise ValueError("Padding must have length as input tensor rank.")

    pad = pad.val

    # get axix which is non zero
    non_zero_axis = []
    for i in range(len(pad)):
        if not all(pad[i] == 0):
            non_zero_axis.append(i)

    if len(non_zero_axis) > 2:
        raise ValueError("Unsupported configuration for Pad layer!")

    # make padding a 2 x 2 tensor if len(non_zero_axis) < 2
    if len(non_zero_axis) == 0:
        non_zero_axis = [0, 1]

    if len(non_zero_axis) == 1:
        if non_zero_axis[0] != len(pad) - 1:
            non_zero_axis.append(len(pad) - 1)
        else:
            non_zero_axis = [0, non_zero_axis[0]]

    # transpose the input such that the padding dim is the last two
    perm = [i for i in range(in_rank) if i not in non_zero_axis] + non_zero_axis
    x = mb.transpose(x=x, perm=perm, name=node.name + "_transpose_1")
    pad = pad[non_zero_axis, :]
    pad = pad.reshape(-1)
    x = mb.pad(
        x=x, pad=pad, name=node.name + "_pad", constant_val=constant_val, mode=mode
    )
    inverse_perm = [-1] * len(perm)
    for i, index in enumerate(perm):
        inverse_perm[index] = i
    x = mb.transpose(x=x, perm=inverse_perm, name=node.name)

    context.add(node.name, x)


@register_tf_op
def Pad(context, node):
    x = context[node.inputs[0]]
    pad = context[node.inputs[1]]

    mode = node.attr.get("mode", "constant").lower()
    constant_val = node.attr.get("constant_val", 0.0)
    in_rank = len(x.sym_type.get_shape())

    if in_rank > 5:
        raise ValueError("Unsupported Pad configuration!")

    if pad.val is None:
        pad = mb.reshape(x=pad, shape=[-1])
    else:
        pad = pad.val.reshape(-1)

    x = mb.pad(x=x, pad=pad, name=node.name, mode=mode, constant_val=constant_val)
    context.add(node.name, x)


@register_tf_op
def PadV2(context, node):
    # compared to tf.raw_ops.Pad, tf.raw_ops.PadV2 allow constant values rather than 0.
    x = context[node.inputs[0]]
    pad = context[node.inputs[1]]
    constant_val = context[node.inputs[2]]

    if constant_val.shape != ():
        raise NotImplementedError(
            "TF `constant_values` in PadV2 op must be const scalar."
        )
    in_rank = x.rank
    if in_rank > 5:
        raise ValueError("Unsupported Pad configuration!")

    if pad.val is None:
        pad = mb.reshape(x=pad, shape=[-1])
    else:
        pad = pad.val.reshape(-1)

    constant_val = constant_val.val
    if constant_val == -_np.inf:
        INT_MIN = -_np.iinfo(_np.int64).max - 1
        constant_val = _np.float(INT_MIN)

    if constant_val == _np.inf:
        INT_MAX = _np.iinfo(_np.int64).max
        constant_val = _np.float(INT_MAX)

    x = mb.pad(x=x, pad=pad, name=node.name, mode="constant", constant_val=constant_val)
    context.add(node.name, x)


@register_tf_op
def Relu(context, node):
    x = context[node.inputs[0]]
    x = mb.relu(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Reciprocal(context, node):
    x = context[node.inputs[0]]
    x = mb.inverse(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Relu6(context, node):
    x = context[node.inputs[0]]
    x = mb.relu6(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Reshape(context, node):
    x = context[node.inputs[0]]
    new_shape = context[node.inputs[1]]
    x = mb.reshape(x=x, shape=new_shape, name=node.name)
    context.add(node.name, x)


@register_tf_op(tf_alias=["ReverseV2"])
def Reverse(context, node):
    x = context[node.inputs[0]]
    axes = context[node.inputs[1]]
    x = mb.reverse(x=x, axes=axes, name=node.name)
    context.add(node.name, x)


@register_tf_op
def ReverseSequence(context, node):
    x = context[node.inputs[0]]
    lengths = context[node.inputs[1]]
    seq_axis = node.attr.get("seq_dim")
    batch_axis = node.attr.get("batch_dim")
    x = mb.reverse_sequence(
        x=x, lengths=lengths, seq_axis=seq_axis, batch_axis=batch_axis, name=node.name
    )
    context.add(node.name, x)


@register_tf_op
def Transpose(context, node):
    x = context[node.inputs[0]]
    perm = context[node.inputs[1]]
    x = mb.transpose(x=x, perm=perm, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Squeeze(context, node):
    x = context[node.inputs[0]]
    axes = node.attr.get("squeeze_dims", [])
    if axes == []:
        axes = None
    x = mb.squeeze(x=x, axes=axes, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Multinomial(context, node):
    x = context[node.inputs[0]]
    size = context[node.inputs[1]]
    x = mb.random_categorical(x=x, size=size, name=node.name)
    context.add(node.name, x)


@register_tf_op(tf_alias=["Elu"])
def ELU(context, node):
    x = context[node.inputs[0]]
    x = mb.elu(x=x, alpha=1.0, name=node.name)
    context.add(node.name, x)


@register_tf_op(tf_alias=["Erf"])
def ERF(context, node):
    x = context[node.inputs[0]]
    x = mb.erf(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op(tf_alias=["LeakyRelu"])
def LeakyReLU(context, node):
    x = context[node.inputs[0]]
    alpha = node.attr["alpha"]
    x = mb.leaky_relu(x=x, alpha=alpha, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Selu(context, node):
    x = context[node.inputs[0]]
    x = mb.elu(x=x, alpha=1.6732632423543772)
    x = mb.mul(x=x, y=1.0507009873554805, name=node.name)
    context.add(node.name, x)


@register_tf_op(tf_alias=["SelectV2"])
def Select(context, node):
    cond = context[node.inputs[0]]
    a = context[node.inputs[1]]
    b = context[node.inputs[2]]

    # broadcast vector type cond
    rank_cond = cond.rank
    rank_a = a.rank
    if rank_cond == 1 and rank_a > 1:
        axes = [-i - 1 for i in range(rank_a - rank_cond)]
        cond = mb.expand_dims(x=cond, axes=axes)

    x = mb.select(cond=cond, a=a, b=b, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Sigmoid(context, node):
    x = context[node.inputs[0]]
    x = mb.sigmoid(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Softplus(context, node):
    x = context[node.inputs[0]]
    x = mb.softplus(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Softsign(context, node):
    x = context[node.inputs[0]]
    x = mb.softsign(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Softmax(context, node):
    logit = context[node.inputs[0]]
    axis = node.attr.get("axis")
    x = mb.softmax(x=logit, axis=axis, name=node.name)
    context.add(node.name, x)


@register_tf_op
def SpaceToBatchND(context, node):
    x = context[node.inputs[0]]
    block_shape = context[node.inputs[1]].val
    paddings = context[node.inputs[2]].val

    if x.rank != 3 and x.rank != 4:
        raise NotImplementedError("rank of input must be 3 or 4!")

    if block_shape is None or paddings is None:
        raise NotImplementedError(
            "Not support dynamic block_shape and paddings for SpaceToBatchND!"
        )

    if len(block_shape.flatten()) > 2:
        raise NotImplementedError("rank of spatial shape > 2 is not yet supported")

    # use sequence of ops to implement spacetobatch for cases:
    # (1) x.rank == 3
    # (2) x.rank == 4 and len(block_shape) == 1
    if x.rank == 3 or (x.rank == 4 and len(block_shape) == 1):

        rank = x.rank
        spatial_rank = len(block_shape)

        # expand padding to have shape [x.rank, 2]
        paddings = _np.concatenate(
            [[[0, 0]], paddings, _np.zeros(shape=(3, 2), dtype=_np.int32)], axis=0
        )
        paddings = paddings[: x.rank, :]
        needs_paddings = any(paddings.flatten())
        if needs_paddings:
            padded = mb.pad(x=x, pad=paddings.flatten(), mode="constant")
        else:
            padded = x
        padded_shape = mb.shape(x=padded)

        # padded_shape = [batch_size] + [spatial_dims] + [remaining_dims]
        batch_size = [_value_at(padded_shape, 0)]
        spatial_dims = [_value_at(padded_shape, i) for i in range(1, spatial_rank + 1)]
        remaining_dims = [
            _value_at(padded_shape, i) for i in range(spatial_rank + 1, rank)
        ]

        # padded_shape = [batch_size] + [s0, s1, ..., sm] + [remaining_dims]
        # reshape_shape = [batch_size] +
        #                 [s0/block_shape[0],block_shape[0],...,sm/block_shape[m],block_shape[m]] +
        #                 [remaining_dims]
        values = []
        for i in range(spatial_rank):
            dim = mb.real_div(x=spatial_dims[i], y=block_shape[i])
            values.append(mb.cast(x=dim, dtype="int32"))
            values.append(block_shape[i])
        values = batch_size + values + remaining_dims
        reshape_shape = mb.concat(values=values, axis=0)
        reshaped_padded = mb.reshape(x=padded, shape=reshape_shape)

        # permute the shape to : [block_shape] + [batch_size] +
        #                        [s0/block_shape[0],...,sm/block_shape[m]] +
        #                        [remaining_dims]
        batch_axis = [0]
        block_shape_axis = [2 + 2 * i for i in range(spatial_rank)]
        spatial_axis = [1 + 2 * i for i in range(spatial_rank)]
        remaining_axis = list(range(block_shape_axis[-1] + 1, len(values)))
        perm = block_shape_axis + batch_axis + spatial_axis + remaining_axis
        permuted_reshaped_padded = mb.transpose(x=reshaped_padded, perm=perm)

        # reshape the tensor to [prod(block_shape)*batch_size] +
        #                       [s0/block_shape[0],...,sm/block_shape[m],block_shape[m]] +
        #                       [remaining_dims]
        prod_block_shape = _np.prod(block_shape.flatten())
        resize_batch_size = [mb.mul(x=values[0], y=prod_block_shape)]
        resize_spatial_dims = [values[1 + 2 * i] for i in range(spatial_rank)]
        final_reshape_values = resize_batch_size + resize_spatial_dims + remaining_dims
        final_shape = mb.concat(values=final_reshape_values, axis=0)
        x = mb.reshape(x=permuted_reshaped_padded, shape=final_shape, name=node.name)
    else:

        if block_shape[0] != block_shape[1]:
            raise NotImplementedError(
                "non-equal block shape is not yet supported for 4d input."
            )
        needs_paddings = any(paddings.flatten())

        x = mb.transpose(x=x, perm=[3, 0, 1, 2])

        if needs_paddings:
            x = mb.pad(x=x, pad=paddings.flatten(), mode="constant")

        x = mb.space_to_depth(x=x, block_size=block_shape[0])
        x = mb.transpose(x=x, perm=[1, 2, 3, 0], name=node.name)

    context.add(node.name, x)


@register_tf_op
def SpaceToDepth(context, node):
    x = context[node.inputs[0]]
    block_size = node.attr.get("block_size")
    data_format = node.attr.get("data_format", "NHWC")
    if data_format == "NHWC":
        x = _transpose_NHWC_to_NCHW(x)
        x = mb.space_to_depth(x=x, block_size=block_size)
        x = _transpose_NCHW_to_NHWC(x, node.name)
    else:
        x = mb.space_to_depth(x=x, block_size=block_size, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Tanh(context, node):
    x = context[node.inputs[0]]
    x = mb.tanh(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op(tf_alias=["TopKV2"])
def TopK(context, node):
    x = context[node.inputs[0]]
    k = context[node.inputs[1]]
    x = mb.topk(x=x, k=k.val, axis=-1, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Cumsum(context, node):
    x = context[node.inputs[0]]
    axis = context[node.inputs[1]]
    exclusive = node.attr.get("exclusive", False)
    reverse = node.attr.get("reverse", False)
    x = mb.cumsum(x=x, axis=axis, exclusive=exclusive, reverse=reverse, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Gather(context, node):
    x = context[node.inputs[0]]
    indices = context[node.inputs[1]]
    axis = 0
    x = mb.gather(x=x, indices=indices, axis=axis, name=node.name)
    context.add(node.name, x)


@register_tf_op
def GatherV2(context, node):
    x = context[node.inputs[0]]
    indices = context[node.inputs[1]]
    axis = context[node.inputs[2]]
    x = mb.gather(x=x, indices=indices, axis=axis, name=node.name)
    context.add(node.name, x)


@register_tf_op
def GatherNd(context, node):
    x = context[node.inputs[0]]
    indices = context[node.inputs[1]]
    x = mb.gather_nd(x=x, indices=indices, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Tile(context, node):
    x = context[node.inputs[0]]
    reps = context[node.inputs[1]]
    x = mb.tile(x=x, reps=reps, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Where(context, node):
    x = context[node.inputs[0]]
    x = mb.non_zero(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def SquaredDifference(context, node):
    x = context[node.inputs[0]]
    y = context[node.inputs[1]]
    x = mb.sub(x=x, y=y)
    x = mb.square(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Conv2DBackpropInput(context, node):
    # Output shape: [N, H_out, W_out, C_out]
    output_shape = context[node.inputs[0]].val
    # Weight shape: [H, W, C_out, C_in]
    W_hwoi = context[node.inputs[1]]
    W_oihw = mb.transpose(x=W_hwoi, perm=[2, 3, 0, 1])
    # Input shape: [N, H_in, W_in, C_in]
    x = context[node.inputs[2]]

    data_format = node.attr.get("data_format", "NHWC")
    HW_dilations = _conv2d3d_strides_or_dilations(
        "dilations", node.attr.get("dilations"), data_format
    )
    HW_strides = _conv2d3d_strides_or_dilations(
        "strides", node.attr.get("strides"), data_format
    )
    pad_type = node.attr.get("padding")

    if not isinstance(pad_type, _string_types):
        pad_type = "custom"
        raise NotImplementedError("Custom padding not implemented for TF")

    pad_type = pad_type.lower()
    # CoreML expects input to be in NCHW format
    # Transpose input to NCHW format
    if data_format == "NHWC":
        x = _transpose_NHWC_to_NCHW(x)
        if output_shape is not None:
            output_shape = [output_shape[1], output_shape[2]]
    else:
        if output_shape is not None:
            output_shape = [output_shape[2], output_shape[3]]

    # Only the last op should have the same name as node.name
    conv_name = node.name + "x" if data_format == "NHWC" else node.name
    # Pass output shape provided above
    x = mb.conv_transpose(
        x=x,
        weight=W_oihw,
        pad_type=pad_type,
        output_shape=output_shape,
        strides=HW_strides,
        dilations=HW_dilations,
        name=conv_name,
    )

    # Convert NCHW output back to NHWC format
    if data_format == "NHWC":
        x = _transpose_NCHW_to_NHWC(x, node.name)
    context.add(node.name, x)


@register_tf_op
def Range(context, node):
    start = context[node.inputs[0]]
    end = context[node.inputs[1]]
    step = context[node.inputs[2]]
    x = mb.range_1d(start=start, end=end, step=step, name=node.name)
    context.add(node.name, x)


@register_tf_op
def RandomUniform(context, node):
    shape = context[node.inputs[0]]
    seed = node.attr["seed"]
    x = mb.random_uniform(shape=shape, seed=seed, name=node.name)
    context.add(node.name, x)


@register_tf_op
def RandomStandardNormal(context, node):
    shape = context[node.inputs[0]]
    seed = node.attr["seed"]
    x = mb.random_normal(shape=shape, seed=seed, name=node.name)
    context.add(node.name, x)


@register_tf_op
def OneHot(context, node):
    indices = context[node.inputs[0]]
    depth = context[node.inputs[1]]
    on_value = context[node.inputs[2]]
    off_value = context[node.inputs[3]]
    axis = node.attr.get("axis", -1)
    x = mb.one_hot(
        indices=indices,
        one_hot_vector_size=depth,
        axis=axis,
        on_value=on_value,
        off_value=off_value,
        name=node.name,
    )
    context.add(node.name, x)


@register_tf_op(tf_alias=["NonMaxSuppressionV3"])
def NonMaxSuppression(context, node):
    boxes = context[node.inputs[0]]
    scores = context[node.inputs[1]]
    max_boxes = context[node.inputs[2]]
    iou_threshold = context[node.inputs[3]]
    score_threshold = context[node.inputs[4]]
    if score_threshold.val == float("-inf"):
        # TensorFlow's default value for score_threshold, Core ML does not
        # have float('-inf') support, converted to minimum float32 instead
        score_threshold = -3.4e38
    boxes = mb.expand_dims(x=boxes, axes=[0])
    scores = mb.expand_dims(x=scores, axes=[0, -1])
    _, _, x, _ = mb.non_maximum_suppression(
        boxes=boxes,
        scores=scores,
        max_boxes=max_boxes,
        iou_threshold=iou_threshold,
        score_threshold=score_threshold,
    )
    num_boxes = boxes.shape[1]
    if not is_symbolic(num_boxes) and num_boxes < max_boxes.val:
        x = mb.squeeze(x=x, axes=[0])
        x = mb.slice_by_index(x=x, begin=[0], end=[num_boxes], name=node.name)
    else:
        x = mb.squeeze(x=x, axes=[0], name=node.name)
    context.add(node.name, x)


@register_tf_op
def Shape(context, node):
    x = context[node.inputs[0]]
    x = mb.shape(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def ResizeNearestNeighbor(context, node):
    # "ResizeNearestNeighbor" op in TF is always in the channel last mode
    # instead of upsample factor, it uses output size, which is the second input
    x = context[node.inputs[0]]

    input_shape = x.shape  # (N,Hin,Win,C)
    if len(input_shape) != 4:
        raise ValueError('"ResizeNearestNeighbor" op: input rank is not 4')
    Hin, Win = input_shape[1:3]

    if context[node.inputs[1]].val is None:
        raise ValueError(
            '"ResizeNearestNeighbor" op: the second input, which is the output size, must be known statically'
        )

    if len(context[node.inputs[1]].val) != 2:
        raise ValueError(
            '"ResizeNearestNeighbor" op: the second input, which is the output size, must have 2 elements'
        )

    Hout, Wout = context[node.inputs[1]].val

    if not (
        isinstance(Hout, (_np.int32, _np.int64))
        and isinstance(Wout, (_np.int32, _np.int64))
    ):
        raise ValueError(
            '"ResizeNearestNeighbor" op: the second input, which is the output size, must have elements of type int32 or int64'
        )

    if Hout < Hin and Wout < Win:
        ResizeBilinear(context, node)
        return

    if Hout % Hin > 0 or Wout % Win > 0:
        raise ValueError(
            '"ResizeNearestNeighbor" op: fractional upsampling factors not supported'
        )

    scaling_factor_h = int(Hout / Hin)
    scaling_factor_w = int(Wout / Win)

    # first transpose to from channel last to channel first format for coreml
    x = _transpose_NHWC_to_NCHW(x)
    # add the upsample layer
    x = mb.upsample_nearest_neighbor(
        x=x,
        upscale_factor_height=scaling_factor_h,
        upscale_factor_width=scaling_factor_w,
        name=node.name + "_channel_first_upsample",
    )
    # transpose again
    x = _transpose_NCHW_to_NHWC(x, node.name)

    context.add(node.name, x)


@register_tf_op
def ResizeBilinear(context, node):
    # "ResizeBilinear" op in TF is always in the channel last mode
    # second input is the output size

    x = context[node.inputs[0]]
    input_shape = x.shape  # (N,Hin,Win,C)
    if len(input_shape) != 4:
        raise ValueError('"ResizeBilinear" op: input rank is not 4')
    Hin, Win = input_shape[1:3]

    if context[node.inputs[1]].val is None:
        raise ValueError(
            '"ResizeBilinear" op: the second input, which is the output size, must be known statically'
        )

    if len(context[node.inputs[1]].val) != 2:
        raise ValueError(
            '"ResizeBilinear" op: the second input, which is the output size, must have 2 elements'
        )

    Hout, Wout = context[node.inputs[1]].val

    if not (isinstance(Hout, (_np.int32, _np.int64)) and isinstance(Wout, (_np.int32, _np.int64))):
        raise ValueError(
            '"ResizeBilinear" op: the second input, which is the output size, must have elements of type int32 or int64'
        )

    align_corners = node.attr.get("align_corners", False)
    half_pixel_centers = node.attr.get("half_pixel_centers", False)

    # first transpose to from channel last to channel first format for coreml
    x = _transpose_NHWC_to_NCHW(x)

    # add either the resize_bilinear layer or the upsample layer

    # [align_corners = True, half_pixel_centers = False]
    if align_corners and not half_pixel_centers:
        x = mb.resize_bilinear(
            x=x,
            target_size_height=Hout,
            target_size_width=Wout,
            sampling_mode="STRICT_ALIGN_CORNERS",
            name=node.name + "_channel_first_resize_bilinear",
        )

    # [align_corners = False, half_pixel_centers = False]
    elif not align_corners and not half_pixel_centers:
        x = mb.resize_bilinear(
            x=x,
            target_size_height=Hout,
            target_size_width=Wout,
            sampling_mode="DEFAULT",
            name=node.name + "_channel_first_resize_bilinear",
        )

    # [align_corners = False, half_pixel_centers = True]
    elif not align_corners and half_pixel_centers:
        x = mb.upsample_bilinear(
            x=x,
            scale_factor_height=(float(Hout) + 1e-2) / float(Hin),
            scale_factor_width=(float(Wout) + 1e-2) / float(Win),
            align_corners=False,
            name=node.name + "_channel_first_upsample_bilinear",
        )

    else:
        # we should not come here since TF does not support align_corners=True and half_pixel_centers=True
        raise ValueError(
            '"ResizeBilinear" op: "align_corners" and "half_pixel_centers" are both True and this mode is not supported'
        )

    # transpose again
    x = _transpose_NCHW_to_NHWC(x, node.name)
    context.add(node.name, x)


@register_tf_op
def make_tuple(context, node):
    res = tuple([context[in_name] for in_name in node.inputs])
    context.add(node.name, res)


@register_tf_op
def function_entry(context, node):
    if context.get_func_inputs() is None:
        msg = (
            "function_entry requires function inputs stored in "
            + "context.curr_func_inputs"
        )
        raise ValueError(msg)
    context.add(node.name, context.get_func_inputs())


@register_tf_op(tf_alias=["while"])
def While(context, node):
    # TF while will never have break statement, because break can always be
    # transformed into while and condition. Example:
    #
    #   while pred:
    #    a = op1(...)
    #    if a == 0:
    #      break
    #    b = op2(...)
    #
    # is equivalent to
    #
    #   while pred and not break_a:
    #    a = op1(...)
    #    break_a = a == 0
    #    if not break_a:
    #      b = op2(...)

    # node.inputs[0] == 'make_tuple_X' (always a make_tuple)
    loop_vars = context[node.inputs[0]]  # python tuple of Vars
    cond_graph = context.get_graph(node.attr["cond_function"])
    body_graph = context.get_graph(node.attr["body_function"])

    def cond(*loop_vars):
        context.stack_func_inputs(loop_vars)

        # convert_graph uses context to convert cond_graph. During conversion
        # it constructs operations (mb.some_op). Note that cond(*loop_vars) is
        # only evaluated inside while_loop's type_inference(), not here. In
        # other words, we use python's deferred function evaluation to defer
        # the SSA block construction until inside while_loop Operation.
        res = convert_graph(context, cond_graph)
        # Done with translating the function
        context.unstack_func_inputs()
        return res

    def body(*loop_vars):
        context.stack_func_inputs(loop_vars)
        res = convert_graph(context, body_graph)
        # Done with translating the function
        context.unstack_func_inputs()
        return res

    x = mb.while_loop(_cond=cond, _body=body, loop_vars=loop_vars, name=node.name)
    # wraps x as tuple for get_tuple that always follow the while node.
    if not isinstance(x, (tuple, list)):
        x = (x,)
    context.add(node.name, x)


@register_tf_op
def iff(context, node):
    pred = context[node.inputs[0]]

    # this is always a tensor, as TF uses one iff op for each returned value.
    #
    # Example TF program:
    #
    #  x = tf.placeholder(tf.float32, shape=(1,))
    #  y = tf.placeholder(tf.float32, shape=(1,))
    #  z = tf.multiply(x, y)
    #  pred = tf.less(tf.math.reduce_mean(x), tf.math.reduce_mean(y))
    #  def true_fn(): return tf.add(x, z), x
    #  def false_fn(): return tf.square(y), z
    #  res = tf.cond(pred, true_fn, false_fn)
    #
    # There will be 2 iffs:
    #
    # iff('cond/pred_id', 'cond/Add', 'cond/Square')
    # iff('cond/pred_id', 'cond/Add/Switch', 'cond/Switch_1')
    #
    # where
    #   'cond/pred_id': pred
    #   'cond/Add': tf.add(x, z)
    #   'cond/Square': tf.square(y)
    #   'cond/Add/Switch': x
    #   'cond/Switch_1': z
    #
    # And both branches are executed, and one of the results will be
    # discarded at iff nodes.
    #
    # Note that the above program would translate to two cond ops, each with
    # two blocks.
    true_output_var = context[node.inputs[1]]
    false_output_var = context[node.inputs[2]]

    def true_fn():
        return mb.identity(x=true_output_var)

    def false_fn():
        return mb.identity(x=false_output_var)

    x = mb.cond(pred=pred, _true_fn=true_fn, _false_fn=false_fn, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Concat(context, node):
    values = [context[input] for input in node.inputs[1:]]
    axis = context[node.inputs[0]]
    x = mb.concat(values=values, axis=axis, name=node.name)
    context.add(node.name, x)


@register_tf_op
def ConcatV2(context, node):
    values = [context[input] for input in node.inputs[:-1]]
    axis = context[node.inputs[-1]]
    x = mb.concat(values=values, axis=axis, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Pack(context, node):
    values = [context[name] for name in node.inputs]
    axis = node.attr["axis"]
    if axis < 0:
        # TF axis = -1 creates new dim at the end
        axis += values[0].rank + 1
    if len(values) == 1:
        # for example:
        # y = tf.raw_ops.Pack(values=[2], axis=0).
        # or y = tf.raw_ops.Pack(values=[tf.constant([1,2])], axis=0)
        input_type = values[0].sym_type
        if _is_scalar(input_type):
            x = mb.mul(x=_np.array([1], dtype=_np.int32), y=values[0], name=node.name)
        else:
            x = mb.expand_dims(x=values[0], axes=[axis], name=node.name)
    else:
        if all([_is_scalar(input.sym_type) for input in values]):
            x = mb.concat(values=values, axis=axis, name=node.name)
        else:
            x = mb.stack(values=values, axis=axis, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Unpack(context, node):
    x = context[node.inputs[0]]
    axis = int(node.attr["axis"])
    num_splits = node.attr.get("num", None)
    if num_splits is None:
        num_splits = x.shape[axis]
    y = mb.split(x=x, num_splits=num_splits, axis=axis, name=node.name + "_unsqueezed")
    output_vars = []
    for i in range(num_splits):
        output_vars.append(
            mb.squeeze(x=y[i], axes=[axis], name=node.name + ":{}".format(i))
        )

    context.add(node.name, output_vars)


@register_tf_op
def SplitV(context, node):
    x = context[node.inputs[0]]
    split_sizes = context[node.inputs[1]]
    axis = context[node.inputs[2]]
    if "num_split" not in node.attr:
        raise ValueError("num_splits not found in TF op {}".format(node.name))
    num_splits = node.attr["num_split"]
    if num_splits == 1:
        Identity(context, node)
    else:
        x = mb.split(
            x=x,
            num_splits=num_splits,
            split_sizes=split_sizes,
            axis=axis,
            name=node.name,
        )
        context.add(node.name, x)


@register_tf_op
def ScatterNd(context, node):
    indices = context[node.inputs[0]]
    updates = context[node.inputs[1]]
    shape = context[node.inputs[2]]
    x = mb.fill(shape=shape, value=0)
    x = mb.scatter_nd(data=x, indices=indices, updates=updates, name=node.name)
    context.add(node.name, x)


@register_tf_op
def ZerosLike(context, node):
    x = context[node.inputs[0]]
    if x.rank == 0:
        np_type = types.nptype_from_builtin(x.sym_type)
        x = mb.const(val=np_type(0), name=node.name)
    else:
        np_type = types.nptype_from_builtin(x.sym_type.get_primitive())
        x = mb.fill(shape=mb.shape(x=x), value=np_type(0), name=node.name)
    context.add(node.name, x)


@register_tf_op
def IsFinite(context, node):
    x = context[node.inputs[0]]
    x = mb.isfinite(x=x, name=node.name)
    context.add(node.name, x)


@register_tf_op
def Split(context, node):
    axis = context[node.inputs[0]]
    x = context[node.inputs[1]]
    if "num_split" not in node.attr:
        raise ValueError("num_splits not found in TF op {}".format(node.name))
    num_splits = node.attr["num_split"]
    if num_splits == 1:
        if len(node.outputs) == 0:
            x = mb.mul(x=x, y=1.0, name=node.name)
            context.add(node.name, x)
        else:
            # Don't change tfssa. Just make downstream ops reference the pre-identity op.
            context.add(node.name, [x], is_new_var=False)
    else:
        x = mb.split(x=x, num_splits=num_splits, axis=axis, name=node.name)
        context.add(node.name, x)
        # TODO (rdar://60358242) If tf.split output is returned, there's no
        # get_tuple nodes. Some graph pass is needed. Example:
        #
        #    x = tf.placeholder(tf.float32, shape=input_shape1)
        #    res = tf.split(x, 3, axis=0)
        #
        # res are ['split:0', 'split:1', 'split']
        #
        # but node.outputs == ['gto_1', 'gto_2', 'gto_3']


@register_tf_op
def CropAndResize(context, node):
    x = context[node.inputs[0]]
    input_shape = x.shape  # (B, h_in, w_in, C)
    if len(input_shape) != 4:
        raise ValueError(
            '"CropResize" op: expected input rank 4, got {}'.format(x.rank)
        )
    Hin, Win = input_shape[1:3]

    const_box_info = True
    if context[node.inputs[1]].val is None or context[node.inputs[2]].val is None:
        const_box_info = False

    crop_size = context[node.inputs[3]].val
    method = "bilinear" if len(node.inputs) < 5 else context[node.inputs[4]].val
    extrapolation_value = 1.0 if len(node.inputs) < 6 else context[node.inputs[5]].val

    # CoreML index information along with boxes
    if const_box_info:
        boxes = context[node.inputs[1]].val
        box_indices = context[node.inputs[2]].val
        box_indices = _np.expand_dims(box_indices, axis=1)
        boxes = _np.concatenate([box_indices, boxes], axis=1)
        # CoreML expects boxes/ROI in
        # [N, 1, 5, 1, 1] format
        boxes = boxes.reshape(boxes.shape[0], 1, boxes.shape[1], 1, 1)
    else:
        box_indices = context[node.inputs[2]]
        boxes = context[node.inputs[1]]
        box_indices = mb.expand_dims(x=box_indices, axes=[1])
        boxes = mb.concat(values=(box_indices, boxes), axis=1)
        # TODO: Dynamic rank: Use GetShape and select indices dynamically
        boxes = mb.reshape(x=boxes, shape=[boxes.shape[0], 1, boxes.shape[1], 1, 1])

    # Get Height and Width of crop
    h_out, w_out = crop_size[0], crop_size[1]

    # TF `nearest` mode not supported
    method_map = {"bilinear": "ALIGN_CORNERS"}
    if method not in method_map:
        raise ValueError(
            "CropResize op: Unsupported method {}. Supports {}".format(
                method, method_map.keys()
            )
        )
    method = method_map[method]

    # TF input format: [B, h_in, w_in, C]
    # CoreML input format: [B, C, h_in, w_in]
    x = _transpose_NHWC_to_NCHW(x)

    # Crop Resize
    x = mb.crop_resize(
        x=x,
        roi=boxes,
        target_height=h_out,
        target_width=w_out,
        normalized_coordinates=True,
        spatial_scale=extrapolation_value,
        box_coordinate_mode="CORNERS_HEIGHT_FIRST",
        sampling_mode=method,
    )

    # CoreML output format: [N, 1, C, h_out, w_out]
    # TF output format: [N, h_out, w_out, C]
    x = mb.squeeze(x=x, axes=[1])
    x = _transpose_NCHW_to_NHWC(x, node.name)
    context.add(node.name, x)


@register_tf_op
def TensorArrayV3(context, node):
    if "infer_shape" in node.attr:
        if not node.attr["infer_shape"]:
            raise ValueError("Only fixed size TensorArray is supported")

    dynamic_length = node.attr.get("dynamic_size", True)
    elem_shape = node.attr.get("element_shape", None)
    size = node.attr.get("size", None)
    if size is None:
        size = context[node.inputs[0]]
    builtin_dtype = node.attr["dtype"]
    dtype_str = types.builtin_to_string(builtin_dtype)
    if elem_shape is not None:
        ls = mb.make_list(
            init_length=size,
            dtype=dtype_str,
            elem_shape=elem_shape,
            dynamic_length=dynamic_length,
            name=node.name,
        )
    else:
        ls = mb.tf_make_list(
            init_length=size,
            dtype=dtype_str,
            dynamic_length=dynamic_length,
            name=node.name,
        )
    context.add(node.name, ls)


@register_tf_op
def TensorArrayWriteV3(context, node):
    index = context[node.inputs[0]]
    new_val = context[node.inputs[1]]
    ls = context[node.inputs[2]]
    new_list = mb.list_write(ls=ls, index=index, value=new_val, name=node.name)
    context.add(node.name, new_list)


@register_tf_op
def TensorArraySizeV3(context, node):
    ls = context[node.inputs[0]]
    length = mb.list_length(ls=ls, name=node.name)
    context.add(node.name, length)


@register_tf_op
def TensorArrayGatherV3(context, node):
    indices = context[node.inputs[0]]
    ls = context[node.inputs[1]]
    tensor = mb.list_gather(ls=ls, indices=indices, name=node.name)
    context.add(node.name, tensor)


@register_tf_op
def TensorArrayReadV3(context, node):
    idx = context[node.inputs[0]]
    ls = context[node.inputs[1]]
    ls = mb.list_read(ls=ls, index=idx, name=node.name)
    context.add(node.name, ls)


@register_tf_op
def TensorArrayScatterV3(context, node):
    indices = context[node.inputs[0]]
    value = context[node.inputs[1]]
    ls = context[node.inputs[2]]
    ls = mb.list_scatter(ls=ls, indices=indices, value=value, name=node.name)
    context.add(node.name, ls)


@register_tf_op
def BroadcastTo(context, node):
    x = context[node.inputs[0]]
    shape = context[node.inputs[1]]
    if shape.val is None:  # dynamic shape
        raise NotImplementedError("dynamic shape not yet supported")
    else:  # static shape
        target_shape = tuple(shape.val)
        broadcast_shape = broadcast_shapes(x.shape, target_shape)
        if target_shape != broadcast_shape:
            msg = "shapes are not broadcastable: {} vs. {}"
            raise ValueError(msg.format(x.shape, target_shape))
        target_rank = len(target_shape)
        if x.rank != target_rank:
            axes = [i for i in range(target_rank - x.rank)]
            x = mb.expand_dims(x=x, axes=axes)
        reps = [1] * target_rank
        for i in range(target_rank):
            reps[i] = target_shape[i] // x.shape[i]

    x = mb.tile(x=x, reps=reps, name=node.name)
    context.add(node.name, x)


@register_tf_op()
def get_global(context, node):
    # Design comment: This is only works if variable doesn't cross block
    # boundary (e.g. while_loop, cond, function)
    variable_name = node.attr["variable"]
    x = context[variable_name]  # This must've been set by set_global
    context.add(node.name, x, is_new_var=False)


@register_tf_op()
def set_global(context, node):
    x = context[node.inputs[0]]
    variable_name = node.attr["variable"]
    context.add(variable_name, x, is_new_var=False)


def _get_const_or_raise(variable):
    if variable.val is None:
        raise ValueError("Var {} must be const".format(variable.name))
    return variable.val


@register_tf_op()
def LSTMBlockCell(context, node):
    x = context[node.inputs[0]]  # [batch, input_dim]
    c_prev = context[node.inputs[1]]  # [b, hidden_dim]
    h_prev = context[node.inputs[2]]  # [b, hidden_dim]
    # W layout is ifco
    W = context[node.inputs[3]]  # [input_dim + hidden_dim, 4*hidden_dim]

    kwargs = {}
    use_peephole = node.attr["use_peephole"]
    if use_peephole:
        peep_i = context[node.inputs[4]]  # [hidden_dim,]
        peep_f = context[node.inputs[5]]  # [hidden_dim,]
        peep_o = context[node.inputs[6]]  # [hidden_dim,]
        kwargs["weight_peep_i"] = peep_i
        kwargs["weight_peep_f"] = peep_f
        kwargs["weight_peep_o"] = peep_o

    bias = context[node.inputs[7]]  # [4*hidden_dim,]

    forget_bias = node.attr["forget_bias"]
    cell_clip = None
    if node.attr["cell_clip"] is not None and node.attr["cell_clip"] > 0:
        cell_clip = node.attr["cell_clip"]

    res = mb.tf_lstm_block_cell(
        x=x,
        c_prev=c_prev,
        h_prev=h_prev,
        weight=W,
        bias=bias,
        forget_bias=forget_bias,
        cell_clip=cell_clip,
        use_peephole=use_peephole,
        name=node.name,
        **kwargs
    )
    context.add(node.name, res)


@register_tf_op()
def BlockLSTM(context, node):
    seq_len = context[node.inputs[0]]  # int
    x = context[node.inputs[1]]  # [padded_len, batch, input_dim]
    init_c = context[node.inputs[2]]  # [1, hidden_dim]
    init_h = context[node.inputs[3]]  # [1, hidden_dim]
    weight = context[node.inputs[4]]  # [input_dim + hidden_dim, 4*hidden_dim]

    kwargs = {}
    use_peephole = node.attr["use_peephole"]
    if use_peephole:
        peep_i = context[node.inputs[5]]  # [hidden_dim,]
        peep_f = context[node.inputs[6]]  # [hidden_dim,]
        peep_o = context[node.inputs[7]]  # [hidden_dim,]
        kwargs["weight_peep_i"] = peep_i
        kwargs["weight_peep_f"] = peep_f
        kwargs["weight_peep_o"] = peep_o

    bias = context[node.inputs[8]]  # [4*hidden_dim,]

    forget_bias = node.attr["forget_bias"]
    cell_clip = None
    if node.attr["cell_clip"] is not None and node.attr["cell_clip"] > 0:
        cell_clip = node.attr["cell_clip"]

    res = mb.tf_lstm_block(
        seq_len=seq_len,
        x=x,
        c_prev=init_c,
        h_prev=init_h,
        weight=weight,
        bias=bias,
        forget_bias=forget_bias,
        cell_clip=cell_clip,
        use_peephole=use_peephole,
        name=node.name,
        **kwargs
    )
    context.add(node.name, res)

@register_tf_op
def ClipByValue(context, node):
    x = context[node.inputs[0]]
    min_value = context[node.inputs[1]]
    max_value = context[node.inputs[2]]
    x = mb.clip(x=x, alpha=min_value, beta=max_value, name=node.name)
    context.add(node.name, x)

@register_tf_op
def Size(context, node):
    x = context[node.inputs[0]]
    x = mb.shape(x=x)
    x = mb.reduce_prod(x=x, axes=[0], name=node.name)
    context.add(node.name, x)

@register_tf_op
def LogSoftmax(context, node):
    x = context[node.inputs[0]]
    axis = node.attr.get('axis', -1)
    y = mb.reduce_log_sum_exp(x=x, axes=[axis], keep_dims=True)
    x = mb.sub(x=x, y=y, name=node.name)
    context.add(node.name, x)

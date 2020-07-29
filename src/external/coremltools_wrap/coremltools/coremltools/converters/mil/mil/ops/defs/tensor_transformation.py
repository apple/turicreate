#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import functools
import sympy as sm
from coremltools.converters.mil.mil.types.symbolic import (
    is_symbolic,
    isscalar,
    any_symbolic,
    any_variadic,
)
from coremltools.converters.mil.mil import (
    get_new_symbol,
    get_new_variadic_symbol,
    VALUE,
    SYMBOL,
    types,
)
from ._op_reqs import *


@register_op(doc_str="")
class depth_to_space(Operation):
    """
    Rearranges elements in a tensor from depth (channel) into spatial dimensions.

    Parameters
    ----------
    x: tensor<[n, C, H, W], T> (Required)
        * Input tensor of rank ``4``.
    block_size: const i32 (Required)
        * The size of the spatial block. Must be greater than ``1`` and divisible by channel dimension ``C``.

    Returns
    -------
    tensor<[n, C / block_size^2, H x block_size, W x block_size], T>
        * where ``b`` is the block size.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(x=TensorInputType(), block_size=IntInputType(const=True),)

    def __init__(self, **kwargs):
        super(depth_to_space, self).__init__(**kwargs)

    def type_inference(self):
        x_type = self.x.dtype
        n, c, h, w = self.x.shape
        bs = self.block_size.val
        ret_shape = (n, c // (bs * bs), h * bs, w * bs)
        return types.tensor(x_type, ret_shape)


@register_op(doc_str="")
class expand_dims(Operation):
    """
    Insert a single-dimension in a 1D or higher tensor at each axis in axes.

    Parameters
    ----------
    x: tensor<*?, T> (Required)
        * Scalar or tensor.
    axes: const tensor<[K], i32> Required
        * ``K`` is the number of dimensions expanded.
        * Insert single dimension at dimension index at each axes.
        * Negative value to index from the end. ``-d-1 <= axis <= d``
          where ``d`` is the rank of ``x``.

    Returns
    -------
    tensor<*(rank(x)+K), T>
        * Same type as the input ``x`` with rank ``rank(x)+K``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=ScalarOrTensorInputType(), axes=IntTensorInputType(const=True),
    )

    def __init__(self, **kwargs):
        super(expand_dims, self).__init__(**kwargs)

    def type_inference(self):
        x_rank = self.x.rank
        x_type = self.x.dtype
        x_shape = list(self.x.shape)
        axes = self.axes.val
        out_rank = x_rank + len(axes)

        for axis in axes:
            if axis <= -out_rank - 1 or axis >= out_rank:
                msg = 'Axis value {} is out of bounds for {} node "{}" of shape {}'
                raise IndexError(
                    msg.format(axis, self.op_type, self.name, self.x.shape)
                )

        ret_shape = x_shape
        axes = sorted([out_rank + axis if axis < 0 else axis for axis in axes])
        for axis in axes:
            ret_shape.insert(axis, 1)

        return types.tensor(x_type, tuple(ret_shape))

    @precondition(allow=VALUE)
    def value_inference(self):
        axes = self.axes.val
        out_rank = self.x.rank + len(axes)

        for axis in axes:
            if axis <= -out_rank - 1 or axis >= out_rank:
                msg = 'Axis value {} is out of bounds for {} node "{}" of shape {}'
                raise IndexError(
                    msg.format(axis, self.op_type, self.name, self.x.shape)
                )

        axes = sorted([out_rank + axis if axis < 0 else axis for axis in axes])
        ret_shape = list(self.x.shape)
        for axis in axes:
            ret_shape.insert(axis, 1)
        return np.reshape(self.x.val, ret_shape)


def reshape_with_symbol(v, shape):
    """
    Perform basic reshape if v is symbolic (not array of symbols).
    """
    if is_symbolic(v):
        return np.array(v).reshape(shape)
    shape = [int(s) for s in shape]
    return v.reshape(shape)


@register_op(doc_str="")
class reshape(Operation):
    """
    Returns a tensor that has the same values as ``x`` with shape ``shape``.
    ``shape`` must have the same volume (number of elements) as ``x``.

    Parameters
    ----------
    x: tensor<*?, T> (Required)
        * A nd tensor or a scalar.
        * If ``x`` is fixed rank (and possibly contains symbolic dimension),
          shape may contain elements that are not positive integers (see below).
        * If ``x`` is variadic rank, shape can only contain positive integers.
    shape: tensor<[K], i32> (Required)
        * A 1D tensor, with elements from the followings:
          * Positive integers.
          * Symbols: All but one symbol in shape must be present in ``x.shape``.
            The new symbol that is not present in ``x.shape`` represent dimension
            such that the total size remains constant. Symbol is illegal
            if ``x`` is variadic rank.
          * -1: ``-1`` introduces a new symbol (see Symbols). Therefore, ``-1`` is
            allowed if all symbols in shape appear in ``x.shape``. ``-1`` is illegal
            if ``x`` is variadic rank.
          * 0: If ``K == rank(x)`` then ``0`` means inheriting from the corresponding
            dimension in ``x.shape``. ``0`` is illegal if ``x`` is variadic rank.

    Returns
    -------
    tensor<*?, T>
        * Tensor with shape determined by the input shape.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(x=ScalarOrTensorInputType(), shape=IntTensorInputType(),)

    def __init__(self, **kwargs):
        super(reshape, self).__init__(**kwargs)

    def type_inference(self):
        if any_symbolic(self.shape.shape):
            # We can't infer any shape if shape has variable length.
            return types.tensor(self.x.dtype, (get_new_variadic_symbol(),))

        # shape has fixed length here.
        if self.shape.sym_val is None:
            shape = tuple([get_new_symbol() for _ in range(self.shape.shape[0])])
            return types.tensor(self.x.dtype, shape)
        t, _ = self._get_type_val()
        return t

    @precondition(allow=VALUE | SYMBOL)
    def value_inference(self):
        _, val = self._get_type_val()
        return val

    def _get_type_val(self):
        x_type = self.x.dtype
        x_shape = self.x.shape
        x_vol = np.prod(x_shape)
        # shape is const, and thus sym_val is not None
        sym_shape = self.shape.sym_val
        sym_shape = [get_new_symbol() if d == -1 else d for d in sym_shape]
        try:
            ret_shape = reshape.enforce_volumetric_constraint(x_vol, sym_shape)
        except:
            ret_shape = sym_shape
        ret_val = None
        if self.x.val is not None and all(isscalar(a) and not is_symbolic(a) for a in ret_shape):
            ret_val = reshape_with_symbol(self.x.val, ret_shape)
        return types.tensor(x_type, tuple(ret_shape)), ret_val

    @staticmethod
    def enforce_volumetric_constraint(left_volume, inshape):
        left_symbols = set()
        if is_symbolic(left_volume):
            left_symbols = left_volume.free_symbols
        # Generally, we want to solve for right in terms of left. But this
        # is kinda annoying actually.
        shape = list(inshape)

        # Handling when reshape is given 0 instead of actual input
        # input tensor shape: [4, 3, 2], reshape:[0, -1], output tensor shape: [4, 6]
        if shape.count(-1) > 1:
            raise ValueError(
                "Reshape op supports only one dimension to be -1. Given {}".format(
                    shape.count(-1)
                )
            )

        infer_dim_index = shape.index(-1) if -1 in shape else None
        right_volume = 1
        for i in shape:
            if i != -1:
                right_volume = right_volume * i

        if infer_dim_index:
            shape[infer_dim_index] = left_volume // right_volume

        if not is_symbolic(right_volume):
            return shape

        constraints = [left_volume - right_volume]
        solve_for = [s for s in shape if is_symbolic(s)]

        for rightsym in solve_for:
            sol = sm.solve(constraints, [rightsym], dict=True)
            if not isinstance(sol, list):
                sol = [sol]
            # look for an acceptable solution
            for s in sol:
                if 0 in s.values():
                    continue
                for i in range(len(shape)):
                    if shape[i] in s:
                        v = s[shape[i]]
                        if len(v.free_symbols - left_symbols) > 0:
                            continue
                        try:
                            shape[i] = int(v)
                        except:
                            shape[i] = v
        return shape


@register_op(doc_str="")
class reverse(Operation):
    """
    Reverses the order of the input tensor ``x`` along specified ``axes``(dimensions).

    Parameters
    ----------
    x: tensor<*?, T> (Required)
        * Input tensor.
    axes: const<D, i32> Optional
        * Dimension(s) to reverse. Each axis must be in the range ``[-rank(x), rank(x)]``.
        * Defaults to None (reverse on all dimensions).

    Returns
    ------
    tensor<*?, T>
        - same type and shape as the input tensor.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=TensorInputType(), axes=IntTensorInputType(const=True, optional=True),
    )

    def __init__(self, **kwargs):
        super(reverse, self).__init__(**kwargs)

    def type_inference(self):
        return self.x.sym_type

    @precondition(allow=VALUE)
    def value_inference(self):
        res = self.x.val
        axes = self.axes.val if self.axes is not None else range(self.x.rank)
        for axis in axes:
            res = np.flip(res, axis=axis)
        return res


@register_op(doc_str="")
class reverse_sequence(Operation):
    """
    Reverses variable length slices for specified axes / dimensions of the input
    tensor. This op first slice input tensor along the ``batch_axis`` dimension, then
    partially reverse the elements along the ``seq_axis`` for the first ``lengths[i]``
    elements.

    Parameters
    ----------
    x: tensor<*?, T> (Required)
        * Input tensor.
    lengths: const<L, i32> (Required)
        * 1-dimensional tensor of length ``x.shape[batch_axis]`` specifying the length
          of the sequence to reverse.
        * Values must be in range ``[0, x.shape[seq_axis]]``.
    seq_axis: const<i32> (Optional)
        * The dimension to reverse.
        * Defaults to ``0``.
    batch_axis: const<i32> (Optional)
        * Dimension for slicing.
        * Defaults to ``0``.

    Returns
    -------
    tensor<*?, T>
        * same type and shape as the input tensor.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=TensorInputType(),
        lengths=IntTensorInputType(),
        seq_axis=IntInputType(const=True, default=0),
        batch_axis=IntInputType(const=True, default=0),
    )

    def __init__(self, **kwargs):
        super(reverse_sequence, self).__init__(**kwargs)

    def type_inference(self):
        return self.x.sym_type

    @precondition(allow=VALUE)
    def value_inference(self):
        raise NotImplementedError("TODO")


@register_op(doc_str="")
class slice_by_size(Operation):
    """
    Slicing input tensor with given ``size`` for each rank.

    Parameters
    ----------
    x: tensor<*?, T> (Required)
        * Input tensor.
    begin: tensor<[rank(x)], i32> Required
        * The begin index for slice.
    size: tensor<[rank(x)], i32> Required
        * The size that is to be sliced. If ``size`` is ``-1``, we slice all the remaining elements.

    Returns
    -------
    tensor<*?, T>
        * Scalar or tensor. Same type as the input tensor.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=TensorInputType(), begin=IntTensorInputType(), size=IntTensorInputType(),
    )

    def __init__(self, **kwargs):
        super(slice_by_size, self).__init__(**kwargs)

    def type_inference(self):
        if self.begin.rank != 1:
            raise ValueError(
                "begin should be 1-D tensor, got {}-D tensor instead".format(
                    self.begin.rank
                )
            )
        if self.size.rank != 1:
            raise ValueError(
                "size should be 1-D tensor, got {}-D tensor instead".format(
                    self.size.rank
                )
            )
        if self.x.rank != self.begin.shape[0]:
            raise ValueError(
                "Length of begin {} doesn't equal to input rank {}.".format(
                    len(self.begin.shape[0]), len(self.x.rank)
                )
            )
        if self.x.rank != self.size.shape[0]:
            raise ValueError(
                "Length of size {} doesn't equal to input rank {}.".format(
                    len(self.size.shape[0]), len(self.x.rank)
                )
            )

        x_shape = self.x.shape
        ret_shape = []
        if self.size.sym_val is None:
            ret_shape = [get_new_symbol() for _ in range(self.x.rank)]
            return types.tensor(self.x.dtype, tuple(ret_shape))

        for idx, s in enumerate(self.size.sym_val):
            if is_symbolic(s):
                ret_shape.append(s)
            elif s != -1:
                ret_shape.append(s)
            elif self.begin.sym_val is not None:
                ret_shape.append(x_shape[idx] - self.begin.sym_val[idx])
            else:
                ret_shape.append(get_new_symbol())

        return types.tensor(self.x.dtype, tuple(ret_shape))

    @precondition(allow=VALUE | SYMBOL)
    def value_inference(self):
        if any_symbolic(self.begin.sym_val):
            return None
        if any_symbolic(self.size.sym_val):
            return None
        if self.x.val is None:
            return None
        slices = []
        for i in range(self.x.rank):
            begin_val = self.begin.val[i]
            if begin_val < 0:
                if is_symbolic(self.x.shape[i]):
                    return None
                begin_val += self.x.shape[i]
            if self.size.val[i] > 0:
                slices.append(slice(begin_val, begin_val + self.size.val[i]))
            else:
                slices.append(slice(begin_val, None, None))
        return self.x.val[tuple(slices)]


@register_op(doc_str="")
class space_to_depth(Operation):
    """
    Rearranges elements in a tensor from spatial into depth (channel) dimension.

    Parameters
    ----------
    x: tensor<[n, C, H, W], T> (Required)
        * Input tensor of rank ``4``.
    block_size: const<i32> (Required)
        * The size of the spatial block. Must be greater than ``1`` and divisible by spatial dimensions ``H, W``.

    Returns
    -------
    tensor<[n, C x block_size^2, H / block_size, W / block_size], T>
        * where ``b`` is the block size.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(x=TensorInputType(), block_size=IntInputType(const=True),)

    def __init__(self, **kwargs):
        super(space_to_depth, self).__init__(**kwargs)

    def type_inference(self):
        x_type = self.x.dtype
        n, c, h, w = self.x.shape
        bs = self.block_size.val
        ret_shape = (n, c * (bs * bs), h // bs, w // bs)
        return types.tensor(x_type, ret_shape)


@register_op(doc_str="")
class squeeze(Operation):
    """
    Remove single-dimension dimensions in a 1D or higher tensor.

    Parameters
    ----------
    x: tensor<*?,T> (Required)
        * Must be at least 1D.
    axes: const<K,i32> (Optional)
        * Axes to squeeze out.
        * default to remove all single-dimensions.

    Returns
    -------
    tensor<*(rank(x)-K),T>
        * Tensor with same type as input ``x`` and rank ``rank(x)-K``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=TensorInputType(), axes=IntTensorInputType(const=True, optional=True),
    )

    def __init__(self, **kwargs):
        super(squeeze, self).__init__(**kwargs)

    def type_inference(self):
        x_type = self.x.dtype
        x_shape = self.x.shape
        squeezed_shape = list(x_shape)
        if self.axes is None:
            # Squeeze all single-dim, assuming symbolic dims != 1
            squeezed_shape = [s for s in squeezed_shape if s != 1]
        else:
            axes = self.axes.val
            axes = [axis if axis >= 0 else axis + self.x.rank for axis in axes]
            for i in sorted(axes)[::-1]:  # descending order
                if len(squeezed_shape) <= i:
                    raise ValueError(
                        "Cannot squeeze dim {} for shape"
                        + " {}".format(i, squeezed_shape)
                    )
                squeezed_shape.pop(i)

        return types.tensor(x_type, tuple(squeezed_shape))

    @precondition(allow=VALUE)
    def value_inference(self):
        if self.x.val is None:
            return None
        if self.axes is None:
            return np.squeeze(self.x.val)
        else:
            return np.squeeze(self.x.val, axis=tuple(self.axes.val))


@register_op(doc_str="")
class transpose(Operation):
    """
    Permutes tensor ``x`` dimensions according to ``perm``.

    Parameters
    ----------
    x: tensor<*?, T> (Required)
        * Must be at least 1D. ``x`` may have symbolic shape.
    perm: const<[rank(x)], i32> (Required)
        * Permutation order. Must be non-negative integers.

    Returns
    -------
    tensor<*?,T>
        * Tensor with same rank and type as ``x``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(x=TensorInputType(), perm=IntTensorInputType(const=True),)

    def __init__(self, **kwargs):
        super(transpose, self).__init__(**kwargs)

    def type_inference(self):
        x_type = self.x.dtype
        perm = self.perm.val
        x_shape = np.array(self.x.shape)
        if len(perm) != self.x.rank:
            msg = "perm should have the same length as rank(x): {} != {}"
            raise ValueError(msg.format(len(perm), self.x.rank))
        if self.x.rank == 0:
            return self.x.sym_type  # scalar cannot be transposed
        if any_variadic(self.x.shape):
            ret_shape = get_new_variadic_symbol()
        else:
            ret_shape = x_shape[perm]
        return types.tensor(x_type, tuple(ret_shape))

    @precondition(allow=VALUE | SYMBOL)
    def value_inference(self):
        return np.transpose(self.x.val, axes=self.perm.val)


@register_op(doc_str="")
class pixel_shuffle(Operation):
    """
    Rearranges elements in a tensor from depth (channel) into spatial dimensions.
    Equivalent to PyTorch's pixel_shuffle.

    Parameters
    ----------
    x: tensor<[n, C x f^2, H, W], T> (Required)
        * Input tensor of rank ``4``.
    upscale_factor: const<i32>
        * Factor to increase spatial resolution by.

    Returns
    -------
    tensor<[n, C, H x f, W x f], T>
        * where ``f`` is the upscale factor.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=TensorInputType(), upscale_factor=IntInputType(const=True),
    )

    def __init__(self, **kwargs):
        super(pixel_shuffle, self).__init__(**kwargs)

    def type_inference(self):
        x_type = self.x.dtype
        n, c, h, w = self.x.shape
        f = self.upscale_factor.val
        ret_shape = (n, c // (f * f), h * f, w * f)
        return types.tensor(x_type, ret_shape)


@register_op(doc_str="")
class sliding_windows(Operation):
    """
    Returns a tensor containing all windows of ``size``, separated by stride along the given ``axis``.

    Parameters
    ----------
    x: tensor<[*d0, d_axis, *dn], T>
        * Input tensor
    axis: const<i32>
        * Axis to perform the operation.
    size: const<i32>
        * Number of elements in the sliding window.
    stride: const<i32> Optional
        * Default to ``1``.
        * The stride of the input elements in the sliding window.

    Returns
    -------
    tensor<[*d0, d_axis - size // stride + 1, size, *dn], T>
        * The output will be a tensor of rank ``N+1`` where ``N`` is the input tensor rank.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=TensorInputType(),
        axis=IntInputType(const=True),
        size=IntInputType(const=True),
        stride=IntInputType(const=True, default=1),
    )

    def __init__(self, **kwargs):
        super(sliding_windows, self).__init__(**kwargs)

    def type_inference(self):
        x_shape = self.x.shape
        axis = self.axis.val
        size = self.size.val
        stride = self.stride.val
        ret_shape = list(x_shape)
        ret_shape[axis] = (x_shape[axis] - size) // stride + 1
        pos_axis = axis if axis >= 0 else axis + self.x.rank
        ret_shape.insert(pos_axis + 1, size)
        return types.tensor(self.x.dtype, tuple(ret_shape))

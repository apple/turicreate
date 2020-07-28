#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import math
from coremltools.converters.mil.mil.types.symbolic import (
    is_symbolic,
    any_symbolic,
    is_compatible_symbolic_vector,
)
from coremltools.converters.mil.mil import (
    get_new_symbol,
    get_new_variadic_symbol,
    SYMBOL,
    VALUE,
    NONE,
)

from coremltools.converters.mil.mil import types
from ._op_reqs import *
from ._utils import promoted_primitive_type


@register_op(doc_str="")
class band_part(Operation):
    """
    Returns a tensor setting everything outside a center band to zeros for the innermost matrix. Special cases:

    - ``band_part(x, 0, -1)`` returns upper triangular part.
    - ``band_part(x, -1, 0)`` returns lower triangular part.
    - ``band_part(x, 0, 0)`` returns diagonal.

    Parameters
    ----------
    x: tensor<*?, T> (Required)
        * Input tensor.
    lower: const<i32> (Optional)
        * Number of lower / below sub-diagonals to keep. If negative, keep entire lower triangle.
        * Defaults to ``-1`` (keep the entire lower triangle)
    upper: const<i32> (Optional)
        * Number of upper / above sub-diagonals to keep. If negative, keep entire lower triangle.
        * Defaults to ``-1`` (keep the entire upper triangle)

    Returns
    -------
    tensor<*?, T>
        * Same type and shape as the input tensor.
    """

    input_spec = InputSpec(
        x=TensorInputType(),
        lower=IntInputType(const=True, default=-1),
        upper=IntInputType(const=True, default=-1),
    )

    def __init__(self, **kwargs):
        super(band_part, self).__init__(**kwargs)

    def type_inference(self):
        return self.x.sym_type


@register_op(doc_str="")
class cumsum(Operation):
    """
    Returns the cumulative sum the input along the given axis.

    Parameters
    ----------
    x: tensor<*?, T> (Required)
        * Input tensor.
    axis: const<i32> (Optional)
        * default to ``0``.
        * Axis for which the cumulative sum is computed.
    exclusive: const<bool> (Optional)
        * default to ``False``.
        * When set to ``False``, inclusive cumsum is computed, that is the first element of
          the output is identical to the first element in the input.
        * When set to ``True``, exclusive cumsum is computed, which makes the first element
          of output to ``0``.
    reverse: const<bool> (Optional)
        * default to ``False``.
        * When set to ``True``, perform cumsum in the reverse order.

    Returns
    -------
    tensor<*?, T>
        * Same type and shape as the input tensor.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=TensorInputType(),
        axis=IntInputType(const=True, default=0),
        exclusive=BoolInputType(const=True, default=False),
        reverse=BoolInputType(const=True, default=False),
    )

    def __init__(self, **kwargs):
        super(cumsum, self).__init__(**kwargs)

    @precondition(allow=VALUE)
    def value_inference(self):
        data = np.copy(self.x.val)
        axis = self.axis.val
        reverse = self.reverse.val
        exclusive = self.exclusive.val
        if reverse:
            data = np.flip(data, axis=axis)
        data = np.cumsum(data, axis=axis)
        if exclusive:
            zero_shape = np.copy(data.shape)
            zero_shape[axis] = 1
            data = np.concatenate((np.zeros(zero_shape, data)), axis=axis)
        if reverse:
            data = np.flip(data, axis=axis)
        return data

    def type_inference(self):
        # Check range of axis
        if self.axis.val < -1 or self.axis.val > self.x.rank - 1:
            raise ValueError(
                "axis should be in the range [-1, {}]".format(self.x.rank - 1)
            )

        return self.x.sym_type


@register_op(doc_str="")
class fill(Operation):
    """
    Returns a tensor with given shape filled with a constant value.

    Parameters
    ----------
    shape: tensor<[K], i32> (Required)
        * Target output tensor shape.
        * ``K`` is the rank of the output tensor. ``shape[k] > 0`` for ``k = 0,..., K-1``.
    value: const<f32> (Optional)
        * default to ``0``.
        * Constant value to fill in.

    Returns
    -------
    tensor<*?, T>
        * Tensor with shape determined by the input shape.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        shape=IntTensorInputType(),
        value=ScalarOrTensorInputType(const=True, default=0.0),
    )

    def __init__(self, **kwargs):
        super(fill, self).__init__(**kwargs)

    def type_inference(self):
        if any_symbolic(self.shape.shape):
            # We can't infer any shape if shape has variable length.
            return types.tensor(self.value.dtype, (get_new_variadic_symbol(),))

        # shape has fixed length here.
        if self.shape.sym_val is None:
            ret_shape = tuple([get_new_symbol() for _ in range(self.shape.shape[0])])
            return types.tensor(self.value.dtype, ret_shape)

        return types.tensor(self.value.dtype, tuple(self.shape.sym_val.tolist()))

    @precondition(allow=VALUE)
    def value_inference(self):
        return np.full(shape=self.shape.val, fill_value=self.value.val)


@register_op(doc_str="")
class non_maximum_suppression(Operation):
    """
    Applies non-maximum suppression (NMS) on the input box coordinates according
    to their intersection-over-union (IoU). NMS selects as subset of bounding
    boxes with the descending scores. Removes boxes that have high
    intersection-over-union (IOU) overlap with previously selected boxes.

    Parameters
    ---------
    boxes: tensor<[n, B, 4], T> (Required)
        * Box coordinates to perform NMS on.
    scores: tensor<[n, B, K], T> (Required)
        * Scores for each one of the boxes
    iou_threshold: const<T> (Required)
        * The intersection over union (``IoU``) threshold over which boxes are suppressed. NMS remove all overlapping boxes with ``IoU > iou_threshold``.
    score_threshold: const<T> (Required)
        * Before IoU suppression is performed, boxes with class scores below this threshold are rejected.
    max_boxes: const<i32> (Required)
        * Maximum number of boxes to select. If the number of surviving boxes are less, output is padded up to this number.
    per_class_suppression: const<bool> (Optional)
        * Default to ``False``.
        * If ``True``, suppression is performed independently within boxes of each class.

    Returns
    -------
    tensor<[n, max_boxes, 4], T>
        * Coordinates of selected boxes.
    tensor<[n, max_boxes, K], T>
        * Scores of selected boxes.
    tensor<[n, max_boxes], i32>
        * Indices of selected boxes.
    tensor<[n], i32>
        * Number of boxes selected for each batch.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        boxes=TensorInputType(),
        scores=TensorInputType(),
        iou_threshold=FloatInputType(const=True),
        score_threshold=FloatInputType(const=True),
        max_boxes=IntInputType(const=True),
        per_class_suppression=BoolInputType(const=True, default=False),
    )

    def __init__(self, **kwargs):
        super(non_maximum_suppression, self).__init__(**kwargs)

    def type_inference(self):
        boxes_dtype = self.boxes.dtype
        scores_dtype = self.scores.dtype
        n_batch, _, n_score = self.scores.shape
        max_boxes = self.max_boxes.val

        return (
            types.tensor(boxes_dtype, (n_batch, max_boxes, 4)),
            types.tensor(scores_dtype, (n_batch, max_boxes, n_score)),
            types.tensor(types.int32, (n_batch, max_boxes)),
            types.tensor(types.int32, (n_batch,)),
        )


@register_op(doc_str="")
class non_zero(Operation):
    """
    Returns the indices of the elements in the given tensor that are non-zero.

    Parameters
    ----------
    x: tensor<*?, T> (Required)
        * Tensor, values selected at indices where its values is not equal to ``0``.

    Returns
    -------
    tensor<[N, R], T>
        * 2-dimensional tensor contains indices of elements that are non-zero. Each
          row is the index for a non-zero value.
        * ``N`` is the number of non-zero elements, ``R`` is the rank of the input.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(x=TensorInputType())

    def __init__(self, **kwargs):
        super(non_zero, self).__init__(**kwargs)

    def type_inference(self):
        shape = tuple([get_new_symbol(), self.x.rank])
        return types.tensor(types.int, shape)

    @precondition(allow=VALUE)
    def value_inference(self):
        return np.transpose(np.nonzero(self.x.val))


@register_op(doc_str="")
class one_hot(Operation):
    """
    Returns one hot vectors whose locations represented in ``indices`` take the ``on_value``,
    while other locations take the ``off_value``.

    Parameters
    ----------
    indices: tensor<[D],T> (Required)
        * Tensor, values indicated the locations for each one hot vector to take the ``on_value``.
    one_got_vector_size: i32 (Required)
        * Indicates the number of returning vectors.
    axis: const i32 (Optional)
        * Indicates which dimension to append the new axis.
        * If the input indices is rank ``D``, the output tensor will have rank ``D+1``.
        * Default to ``-1`` (the last dimension).
    on_value: const i32 (Optional)
        * Values for locations where defined in ``indices``.
        * Default to ``1``.
    off_value: const i32 (Optional)
        * Default to ``0``.

    Returns
    -------
    tensor<*?,T>
        * A tensor contains one hot vectors.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        indices=IntTensorInputType(),
        one_hot_vector_size=IntInputType(),
        axis=IntInputType(const=True, default=-1),
        on_value=IntOrFloatInputType(const=True, default=1),
        off_value=IntOrFloatInputType(const=True, default=0),
    )

    def __init__(self, **kwargs):
        super(one_hot, self).__init__(**kwargs)

    def type_inference(self):
        on_type = self.on_value.dtype
        off_type = self.off_value.dtype

        if on_type != off_type:
            raise TypeError(
                "Parameters on_value and off_value must have same input types."
            )

        if self.axis.val < -self.indices.rank - 1 or self.axis.val > self.indices.rank:
            raise IndexError(
                "Axis value {} is out of bounds for {} node {}".format(
                    self.axis.val, self.op_type, self.name
                )
            )

        indices_shape = list(self.indices.shape)

        depth_value = self.one_hot_vector_size.sym_val
        if depth_value is None:
            depth_value = get_new_symbol()
        elif depth_value < 0:
            raise ValueError("Parameter one_hot_vector_size must be non-negative")

        retshape = indices_shape

        if self.axis.val < 0:
            cut = len(retshape) + self.axis.val + 1
        else:
            cut = self.axis.val
        retshape = retshape[0:cut] + [depth_value] + retshape[cut:]

        return types.tensor(on_type, retshape)


@register_op(doc_str="")
class pad(Operation):
    """
    Pad a tensor.

    Parameters
    ----------
    x: tensor<[*D_in],T>  (Required)
    * pad: tensor<[2*N],i32> (Required)
        * ``N <= D_in``: last ``N`` dimensions of ``x`` are padded as follows:
        * For each dimension ``i`` of ``x`` if ``i >= D_in - N``
            * pad ``pad[2*i]`` elements before ``x[..,i,..]``
            * pad ``pad[2*i+1]`` elements after ``x[..,i,..]``
        * If mode is "reflect" then ``pad[2*i]`` and ``pad[2*i+1]`` can be at most ``D[i]-1``.
        * If mode is "replicate" then ``pad[2*i]`` and ``pad[2*i+1]`` can be at most ``D[i]``.
    * mode: const<str> (Optional)
        * Default to 'constant'.
        * Must be one of the following values:
            * constant
            * reflect
            * replicate
    * constant_val: const<T> (Optional)
        * Default to ``0``.
        * Constant value to pad. Ignored if ``mode != constant``.

    Returns
    -------
    tensor<[*D_out],T>
        % Tensor with same type as the input.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=TensorInputType(),
        pad=IntTensorInputType(),
        mode=StringInputType(const=True, default="constant"),
        constant_val=FloatInputType(const=True, default=0.0),
    )

    def __init__(self, **kwargs):
        super(pad, self).__init__(**kwargs)

    def type_inference(self):
        in_shape = self.x.shape
        ret_shape = list(in_shape)
        pad = self.pad
        if len(pad.shape) != 1:
            raise ValueError("Pad should be a 1D tensor!")
        if pad.val is None:
            for i in range(self.pad.shape[0]//2):
                ret_shape[-self.pad.shape[0]//2+i] = get_new_symbol()
        else:
            pad = pad.val
            pad = pad.copy()
            pad = pad.reshape(-1, 2)

            for i in range(len(pad)):
                ret_shape[-len(pad) + i] = ret_shape[-len(pad) + i] + pad[i][0] + pad[i][1]

        return types.tensor(self.x.dtype, tuple(ret_shape))

    @precondition(allow=VALUE)
    def value_inference(self):
        # NumPy `edge` mode is equivalent to `replicate` mode of PyTorch and CoreML
        mode = "edge" if self.mode.val == "replicate" else self.mode.val
        pad_val = self.pad.val

        if pad_val is None:
            return None

        if len(self.x.val.shape) > (pad_val.shape[0] // 2):
            updated_pad = np.zeros(len(self.x.val.shape) * 2)
            updated_pad[-pad_val.shape[0] :] = pad_val
            pad_val = updated_pad
        pad_val = pad_val.reshape(-1, 2).astype(np.int32)
        if mode == "constant":
            return np.pad(
                self.x.val, pad_val, mode, constant_values=self.constant_val.val
            )
        # NumPy does not support non-constant mode and constant_values argument
        return np.pad(self.x.val, pad_val, mode)


@register_op(doc_str="")
class range_1d(Operation):
    """
    Returns a numpy-like 1d range sequence.

    Parameters
    ----------
    end: <T> (Required)
        * The upper limit of the sequence, exclusive.
    start: <T> (Required)
        * The start point of the sequence.
    step: <T> (Required)
        * Number that increments ``start``.

    Returns
    -------
    tensor<M, T>
        * An 1D tensor. where ``M`` is the length of the sequence.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        end=IntOrFloatInputType(),
        start=IntOrFloatInputType(),
        step=IntOrFloatInputType(),
    )

    def __init__(self, **kwargs):
        super(range_1d, self).__init__(**kwargs)

    @precondition(allow=VALUE)
    def value_inference(self):
        start = self.start.val
        end = self.end.val
        step = self.step.val
        return np.arange(start, end, step).astype(np.int32)

    def type_inference(self):
        start = self.start.sym_val
        end = self.end.sym_val
        step = self.step.sym_val

        if (
            (self.start.dtype != self.end.dtype)
            or (self.start.dtype != self.step.dtype)
            or (self.end.dtype != self.step.dtype)
        ):
            raise TypeError(
                "All inputs to the range operation must have same input types."
            )

        if all(sym_val is not None for sym_val in (start, end, step)):
            shape = (end - start) / step
            shape = shape if is_symbolic(shape) else int(math.ceil(shape))
            shape = tuple([shape])
        else:
            shape = tuple([get_new_symbol(),])

        return types.tensor(self.start.dtype, shape)


@register_op(doc_str="")
class tile(Operation):
    """
    Returns a new tensor by replicating input ``x`` multiples times.
    The ``i``th dimention of ``x`` will be replicated ``reps[i]`` times.

    Parameters
    ----------
    x: tensor<*?, T> (Required)
        * Input tensor.
    reps: tensor<[rank(x)], T> (Required)
        * A 1D tensor with length ``rank(x)`` which indicates number to replicate the input along each dimension.

    Returns
    -------
    tensor<*?, T>:
        * An Nd tensor with same type as the input.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(x=TensorInputType(), reps=TensorInputType(),)

    def __init__(self, **kwargs):
        super(tile, self).__init__(**kwargs)

    def type_inference(self):
        x_type = self.x.dtype
        x_shape = np.array(self.x.shape)
        reps = self.reps.val
        if reps is None:
            out_shape = tuple([get_new_symbol() for _ in range(self.x.rank)])
            return types.tensor(x_type, out_shape)

        if len(reps) == 0 or len(reps) > self.x.rank:
            msg = (
                "Length of the reps ({}) must be at least 1, and "
                "not greater than the rank of the input x ({})"
            )
            raise ValueError(msg.format(len(reps), self.x.rank))

        if any(i <= 0 for i in reps):
            raise ValueError("All entries of reps parameter must be greater than 0")

        if len(reps) < self.x.rank:
            reps = [1] * (self.x.rank - len(reps)) + list(reps)

        out_shape = tuple([reps[i] * x_shape[i] for i in range(len(reps))])

        return types.tensor(x_type, out_shape)

    @precondition(allow=VALUE)
    def value_inference(self):
        # Infer only if don't have symbolic values.
        if self.reps.val is None:
            return None
        return np.tile(self.x.val, reps=self.reps.val)


@register_op(doc_str="")
class argsort(Operation):
    """
    Returns a tensor containing the indices of the sorted values along given axis
    of the input tensor.

    Paramters
    ---------
    x: <*?, T> (Required)
        * Input tensor.
    * axis: const<i32> (Optional)
        * Default to ``-1`` (the last dimension).
        * Axis to perform the operation.
    * ascending: const<bool> (Optional)
        * True to sort in ascending order. Default to ``Flattensalse``, sort in descending order.

    Returns
    -------
    tensor<*?, T>
        * Tensor containing the indices of the sorted values

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=TensorInputType(),
        axis=IntInputType(const=True, default=-1),
        ascending=BoolInputType(const=True, default=False),
    )

    def __init__(self, **kwargs):
        super(argsort, self).__init__(**kwargs)

    def type_inference(self):
        return types.tensor(types.int32, self.x.shape)

    @precondition(allow=VALUE)
    def value_inference(self):
        if self.ascending.val:
            return np.argsort(-self.x.val, axis=self.axis.val)
        return np.argsort(self.x.val, axis=self.axis.val)


@register_op(doc_str="")
class topk(Operation):
    """
    Returns a tensor containing top or bottom k values and the corresponding
    indices of the input tensor along a given axis.

    Parameters
    ----------
    x: <*?, T> (Required)
        * Input tensor.
    k: const<i32> (Optional)
        * Default to ``1``.
        * Number of values/indices to be computed along each axis.
    * axis: const<i32> (Optional)
        * Defaults to ``1`` (channel dimension).
        * Axis to perform the operation.
    * ascending: const<bool> (Optional)
        * Default to ``False``.
        * Whether or not to sort in ascending order, sort in descending order.

    Returns
    -------
    tensor<*?, T>
        * Values of top/bottom ``k`` elements
    tensor<*?, T>
        * Indices of the top/bottom ``k`` elements along axis

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=TensorInputType(),
        k=IntInputType(const=True, default=1),
        axis=IntInputType(const=True, default=-1),
        ascending=BoolInputType(const=True, default=False),
    )

    def __init__(self, **kwargs):
        super(topk, self).__init__(**kwargs)

    def type_inference(self):
        x_type = self.x.dtype
        x_shape = self.x.shape
        k = self.k.val
        axis = self.axis.val

        if not is_symbolic(x_shape[axis]) and k > x_shape[axis]:
            msg = "K={} is greater than size of the given axis={}"
            raise ValueError(msg.format(k, axis))

        ret_shape = list(x_shape)
        ret_shape[axis] = k
        return types.tensor(x_type, ret_shape), types.tensor(types.int32, ret_shape)

    @precondition(allow=VALUE)
    def value_inference(self):
        indices = np.argsort(self.x.val, axis=self.axis.val)
        if not self.ascending.val:
            indices = np.argsort(-self.x.val, axis=self.axis.val)
        slc = [slice(None)] * self.x.rank
        slc[self.axis.val] = slice(0, self.k.val)
        indices = indices[tuple(slc)]
        values = np.take_along_axis(self.x.val, indices, axis=self.axis.val)
        return values, indices


@register_op(doc_str="")
class flatten(Operation):
    """
    Flattens input tensor into 2d tensor by flattening dimensions before and after the provided axis

    Parameters
    ----------
    x: tensor<[*d], T> (Required)
        * Input tensor.
    * axis: const<f32>  (Optional)
        * Defaults to ``1``.
        * negative axis is supported.

    Returns
    -------
    tensor<d_prior, d_post, T>
        * ``d_prior`` is product of dimensions ``x[:axis]``
        * ``d_post`` is product of dimensions ``x[axis:]``

    Examples
    --------
        1. ``input_shape = (3, ), axis = -1, output_shape = (1, 3)``
        2. ``input_shape = (3, ), axis = 1, output_shape = (3, 1)``
        3. ``input_shape = (4, 3), axis = -1, output_shape = (4, 3)``
        4. ``input_shape = (2, 3, 2), axis = -1, output_shape = (6, 2)``
        5. ``input_shape = (5, 5, 2), axis = 1, output_shape = (2, 10)``

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=TensorInputType(), axis=IntInputType(const=True, default=1)
    )

    def __init__(self, **kwargs):
        super(flatten, self).__init__(**kwargs)

    def type_inference(self):
        shape = list(self.x.shape)
        axis = self.axis.val
        dim_pre_axis = np.prod(shape[:axis])
        dim_post_axis = np.prod(shape[axis:])
        new_shape = [dim_pre_axis, dim_post_axis]
        return types.tensor(self.x.dtype, tuple(new_shape))

    @precondition(allow=VALUE | SYMBOL)
    def value_inference(self):
        shape = self.x.shape
        axis = self.axis.val

        dim_pre_axis = np.prod(shape[:axis])
        dim_post_axis = np.prod(shape[axis:])
        return self.x.val.reshape(dim_pre_axis, dim_post_axis)


@register_op(doc_str="")
class shape(Operation):
    """
    Returns 1-dimensional tensor with shape of input tensor

    Parameters
    ----------
    x: tensor<[*?], T> (Required)
        * Input tensor.

    Returns
    -------
    tensor<K, i32>
        * Shape of input tensor.
        * ``K = x.rank``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(x = ScalarOrTensorInputType())

    def __init__(self, **kwargs):
        super(shape, self).__init__(**kwargs)

    def type_inference(self):
        input_rank = self.x.rank
        return types.tensor(types.int32, tuple([input_rank]))

    def value_inference(self):
        if any_symbolic(self.x.shape):
            # convert elements in shape to int32
            res = [x if is_symbolic(x) else np.int32(x) for x in self.x.shape]
            return np.array(res)
        else:
            return np.array(self.x.shape).astype(np.int32)


@register_op(doc_str="")
class concat(Operation):
    """
    Concatenates tensors along a dimension.

    Parameters
    ----------
    values: Tuple[tensor<[d0, d1, ..., d_axis_i, ..., d_n],T>]  (Required)
        * The number of dimensions of the input tensors must match, and all dimensions except ``axis`` must be equal.
        * The tensors may be variadic, but the number of tensors must be determined at compile time (i.e. a tuple).
    axis: const<int32> (Required)
        * The dimension along which to concatenate. Must be in the range ``[-rank(values[i]), rank(values[i]))`` for all ``i``.

    Returns
    -------
    tensor<[d0, d1,...d_axis_out, ..., d_n],T>
        * where ``d_axis_out = sum(d_axis_i)``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(values=TupleInputType(), axis=IntInputType(const=True),)

    def __init__(self, **kwargs):
        super(concat, self).__init__(**kwargs)

    def type_inference(self):
        concat_dim_len = 0
        if len(self.values) == 0:
            raise ValueError("Concat {} got 0 values".format(self.name))

        # Validate values have the same rank
        rank = self.values[0].rank
        for v in self.values:
            if v.rank != rank:
                msg = "Input {} has rank {} != other inputs rank {}"
                raise ValueError(msg.format(v.name, v.rank, rank))

        # Check concat axis is within (-rank, rank)
        concat_axis = self.axis.val
        if concat_axis < 0:
            concat_axis += rank
        if rank > 0 and (concat_axis < 0 or concat_axis >= rank):
            msg = "In {} of op_type {}: axis out of bound for input " + "(rank {})"
            raise ValueError(msg.format(self.name, self.op_type, rank))

        # Validate primitive types are compatible
        dtype = self.values[0].dtype
        for v in self.values[1:]:
            new_dtype = promoted_primitive_type(v.dtype, dtype)
            if new_dtype is None:
                msg = "Incompatible primitive types concat: {} vs {}"
                raise ValueError(msg.format(v.dtype, dtype))
            dtype = new_dtype

        # validate that non-axis dimensions match
        retshape = list(self.values[0].shape)
        for v in self.values[1:]:
            for i in range(rank):
                if is_symbolic(retshape[i]) or is_symbolic(v.shape[i]):
                    continue
                if i != concat_axis and retshape[i] != v.shape[i]:
                    msg = 'Dimension mismatch in {} ("{}"): shapes {} vs. {}'
                    raise ValueError(
                        msg.format(self.op_type, self.name, retshape, v.shape)
                    )

        # Get length of concat dim
        concat_dim_len = 0
        for v in self.values:
            if len(v.shape) == 0:
                taxis = 1
            else:
                taxis = v.shape[concat_axis]
            if is_symbolic(taxis):
                concat_dim_len = get_new_symbol()
                break
            concat_dim_len += taxis

        if len(retshape) == 0:
            retshape = [concat_dim_len]
        else:
            retshape[concat_axis] = concat_dim_len

        return types.tensor(dtype, retshape)

    @precondition(allow=VALUE | SYMBOL | NONE)
    def value_inference(self):

        is_all_rank_zero = all([v.rank == 0 for v in self.values])
        values = [
            v.sym_val if v.sym_val is not None else get_new_symbol()
            for v in self.values
        ]

        # we only infer values for values whose ranks are all zero,
        # or don't have symbolic values.
        # Note that cases like values = [[1, is0], [2]] aren't in such case.
        if any([is_symbolic(v) for v in values]) and not is_all_rank_zero:
            return None

        if not isinstance(values[0], np.ndarray) or values[0].shape == ():
            return np.stack(values, axis=self.axis.val)

        return np.concatenate(values, axis=self.axis.val)


@register_op(doc_str="")
class split(Operation):
    """
    Split tensors into a tuple

    Parameters
    ----------
    x: <*?,T>  (Required)
        * The tensor to split.
        * The tensors may be variadic, but the number of tensors must be determined at compile time (i.e. a tuple).
    num_splits: <i32> (Optional)
        * If specified, divide ``x`` into ``num_splits`` tensors along ``axis``. Its behavior depends on ``split_sizes``:
            * If ``split_sizes`` is defined, ``num_splits == S``, and the output sizes may be uneven
            * If ``split_sizes`` is not defined, ``value.shape[axis]`` must be divisible by ``num_splits``, and the output sizes must be even
        * At least one of ``num_splits`` or ``split_sizes`` must be provided. If ``split_sizes`` length ``S`` cannot be determined at compile time, ``num_splits`` must be supplied to determine the number of outputs.
    * split_sizes: const<S,i32> (Optional)
        * Sizes to split to. The sum of ``split_sizes`` must equal to ``value.shape[axis]``.
    * axis: const<i32> (Required)
        * The dimension along which to concatenate. Must be in the range ``[-rank(x), rank(x))``.

    Returns
    -------
    Tuple[tensor<*?,T>]
        * where the length of the tuple is the number of splits (determined from ``num_splits`` or ``split_sizes``).

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=TensorInputType(),
        num_splits=IntInputType(const=True, optional=True),
        split_sizes=IntTensorInputType(const=True, optional=True),
        axis=IntInputType(const=True),
    )

    def __init__(self, **kwargs):
        super(split, self).__init__(**kwargs)

    def type_inference(self):
        num_splits, sizes = self._get_num_splits_and_sizes()
        x_shape = list(self.x.shape)
        ret_shapes = [x_shape[:] for _ in range(num_splits)]
        axis = self.axis.val
        for i, d in enumerate(sizes):
            ret_shapes[i][axis] = d
        self.sizes = sizes
        return tuple([types.tensor(self.x.dtype, s) for s in ret_shapes])

    def _get_num_splits_and_sizes(self):
        """
        Return:
        - num_splits: int
        - sizes: list of int/symbols. Of length num_splits

        Raise ValueError if num_splits cannot be determined.
        """
        if self.num_splits is None and self.split_sizes is None:
            msg = (
                "At least one of num_splits and split_sizes "
                + "must be specified in split op {}"
            )
            raise ValueError(msg.format(self.name))

        axis = self.axis.val

        if self.num_splits is not None:
            num_splits = self.num_splits.val
            if self.split_sizes is None:
                # Even split
                if (
                    not is_symbolic(self.x.shape[axis])
                    and self.x.shape[axis] % num_splits != 0
                ):
                    msg = "num_split {} does not divide split " + "dim (length = {})"
                    raise ValueError(msg.format(num_splits, self.x.shape[axis]))
                size = self.x.shape[axis] / num_splits
                return num_splits, [size] * num_splits

            # self.split_sizes is not None
            if self.split_sizes.sym_val is not None:
                return num_splits, self.split_sizes.sym_val

            # self.split_size.sym_val is None.
            sizes = [get_new_symbol() for _ in range(num_splits)]
            return num_splits, sizes

        # self.num_splits is None, self.split_sizes is not None
        if self.split_sizes.sym_val is not None:
            return len(self.split_sizes.sym_val), self.split_sizes.sym_val

        # self.num_splits is None, self.split_sizes is not None
        # self.split_sizes.sym_val is None
        if any_symbolic(self.split_sizes.shape):
            raise ValueError("Unable to determine number of splits")

        num_splits = len(self.split_sizes.shape)
        sizes = [get_new_symbol() for _ in range(num_splits)]
        return num_splits, sizes

    @precondition(allow=VALUE | SYMBOL | NONE)
    def value_inference(self):
        num_splits, sizes = self._get_num_splits_and_sizes()
        if self.x.sym_val is None or any_symbolic(sizes):
            raise NotImplementedError()

        if num_splits == 1:
            # No split_indices possible.
            return self.x.sym_val

        split_indices = np.cumsum(sizes).astype(np.int)
        return tuple(np.split(self.x.sym_val, split_indices[:-1], axis=self.axis.val))


@register_op(doc_str="")
class stack(Operation):
    """
    Concatenates tensors along a dimension.

    Parameters
    ----------
    values: Tuple[tensor<[d0, d1,...d_axis_i, ..., d_n],T>]  (Required)
        * All tensors must have identical shape.
    axis: const<i32> (Required)
        * The dimension along which to concatenate. Must be in the range ``[-rank(values[i]), rank(values[i]))`` for all ``i``.

    Returns
    -------
    tenor<[d0, d1,...d_axis_out, ..., d_n],T>
        * where ``d_axis_out = sum(d_axis_i)``

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(values=TupleInputType(), axis=IntInputType(const=True),)

    def __init__(self, **kwargs):
        super(stack, self).__init__(**kwargs)

    def type_inference(self):

        num_tensors = len(self.values)
        if num_tensors == 0:
            raise ValueError("Cannot stack 0 tensor")

        # get the first value without symbolic shape
        t_shape = None
        for value in self.values:
            if not any_symbolic(value.shape):
                t_shape = value.shape
                break
        t_shape = self.values[0].shape if t_shape is None else t_shape

        # compare all shape
        for t in self.values:
            if not is_compatible_symbolic_vector(t.shape, t_shape):
                msg = "Component tensor {} has shape {}, others have {}"
                raise ValueError(msg.format(t.name, t.shape, t_shape))
        ret_shape = list(t_shape)
        ret_shape.insert(self.axis.val, num_tensors)
        return types.tensor(self.values[0].dtype, ret_shape)

    @precondition(allow=VALUE | SYMBOL | NONE)
    def value_inference(self):

        is_all_rank_zero = all([v.rank == 0 for v in self.values])
        values = [
            v.sym_val if v.sym_val is not None else get_new_symbol()
            for v in self.values
        ]

        if any([is_symbolic(v) for v in values]) and not is_all_rank_zero:
            return None

        return np.stack(values, self.axis.val)


@register_op(doc_str="")
class isfinite(Operation):
    input_spec = InputSpec(x=ScalarOrTensorInputType(),)
    """
    Should deprecate this op.
    """

    def __init__(self, **kwargs):
        super(isfinite, self).__init__(**kwargs)

    def type_inference(self):
        return types.tensor(types.bool, list(self.x.shape))

    @precondition(allow=VALUE)
    def value_inference(self):
        return np.isfinite(self.x.val)

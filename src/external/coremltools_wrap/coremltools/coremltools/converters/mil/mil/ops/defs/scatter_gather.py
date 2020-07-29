#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil.mil import SYMBOL, VALUE
from coremltools.converters.mil.mil.types.symbolic import is_compatible_symbolic_vector
from ._op_reqs import *
import numbers


@register_op(doc_str="")
class gather(Operation):
    """
    Gather slices from input ``x`` along dimension ``axis`` according to ``indices``, similar to ``tf.gather``.

    If ``indices`` is scalar (0-D):
    ``output[p_0, ..., p_{axis-1},            ,p_{axis+1}, ..., p_{rank(x)-1}]
    = x[p_0, ..., p_{axis-1}, indices, p_{axis+1}, ..., p_{rank(x)-1}]``
    where ``rank(x)`` is the rank of ``x``.\n
    The output has rank ``rank(x) - 1``.

    If indices is 1-D tensor:
    ``output[p_0, ..., p_{axis-1},            i, p_{axis+1}, ..., p_{rank(*D)-1}]
    = x[p_0, ..., p_{axis-1}, indices[i], p_{axis+1}, ..., p_{rank(*D)-1}]``.
    The output has rank ``rank(x)``.

    In general,
    ``output[p_0, ..., p_{axis-1},           i_0, ..., i_{M-1} , p_{axis+1}, ..., p_{rank(x)-1}]
    = x[p_0, ..., p_{axis-1}, indices[i_0, ..., i_{M-1}], p_{axis+1}, ..., p_{rank(x)-1}]``.

    where ``M = rank(x)``.

    Parameters
    ----------
    * x: tensor<*D,T> (Required)
    * indices: tensor<*N,i32> (Required)
        * Indices values may be negative. More precisely, ``-D[axis]<= v < D[axis]`` for ``v`` in ``indices``.
    * axis: const i32 (Optional)
        * Default to ``0``.

    Returns
    -------
    tensor<*K,T>
        * where ``K = D[:axis] + N + D[axis+1:]``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=TensorInputType(),
        indices=IntOrIntTensorInputType(),
        axis=IntInputType(const=True, default=0),
    )

    def __init__(self, **kwargs):
        super(gather, self).__init__(**kwargs)

    @precondition(allow=VALUE | SYMBOL)
    def value_inference(self):
        x = self.x.sym_val
        indices = self.indices.val
        if indices is None:
            # only allow x to be symbolic. indices cannot.
            return None
        scalar_indices = isinstance(indices, numbers.Integral)
        axis = self.axis.val
        if scalar_indices:
            res = np.take(x, [indices], axis)
            res2 = np.squeeze(res, axis=axis)
            if isinstance(res2, np.ndarray) and len(res2.shape) == 0:
                # res2 is a scalar, but represented as np.array(symbol,
                # dtype=np.object) which np.squeeze can't remove.
                return res2.item()
            return res2
        return np.take(x, indices, axis)

    def type_inference(self):
        out_type = self.x.dtype

        if self.axis.val < -self.x.rank or self.axis.val >= self.x.rank:
            raise IndexError(
                "Axis value {} is out of bounds for {} node {}".format(
                    self.axis.val, self.op_type, self.name
                )
            )

        output_rank = self.x.rank - 1 + self.indices.rank
        if output_rank == 0:
            # output scalar
            return out_type

        axis = self.axis.val
        axis = axis if axis >= 0 else axis + self.x.rank
        out_shape = self.x.shape[:axis] + self.indices.shape + self.x.shape[axis + 1 :]
        return types.tensor(out_type, out_shape)


@register_op(doc_str="")
class scatter(Operation):
    """
    Scatter ``updates`` to ``data`` at locations ``indices`` at dimension ``axis`` by operation ``mode``.
    ``mode == update`` for instance.
    ``
    output[p_0, ..., p_{axis-1}, indice[i], p_{axis+1}, ..., p_D] =
    updates[p_0, ..., p_{axis-1}, i, p_{axis+1}, ..., p_D]
    `` for ``i`` in ``[0, len(indices)]``.
    Otherwise
    ``
    output[p_0, ..., p_{axis-1}, j, p_{axis+1}, ..., p_D] =
    data[p_0, ..., p_{axis-1}, j, p_{axis+1}, ..., p_D]
    ``

    ``mode == add`` for instance.
    ``
    output[p_0, ..., p_{axis-1}, indice[i], p_{axis+1}, ..., p_D] =
    updates[p_0, ..., p_{axis-1}, i, p_{axis+1}, ..., p_D] +
    x[p_0, ..., p_{axis-1}, indice[i], p_{axis+1}, ..., p_D]
    `` for ``i`` in ``[0, len(indices)]
    Otherwise
    ``
    output[p_0, ..., p_{axis-1}, j, p_{axis+1}, ..., p_D] =
    data[p_0, ..., p_{axis-1}, j, p_{axis+1}, ..., p_D]
    ``

    Parameters
    ----------
    data: tensor<*D, T> (Required)
    indices: tensor<[C],T> (Required)
        * 1D tensor.
    updates: tensor<*K, T> (Required)
        * ``K = data.shape[:axis] + [len(indices)] + data.shape[axis+1:]``.
    axis: const i32 (Optional)
        * Default to ``0``.
    mode: const string (Optional)
        * Can be the following modes:
            * update
            * add
            * sub
            * mul
            * div
            * max
            * min

    Returns
    -------
    tensor<*D, T>
        * with the same type and shape as input ``x``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        data=TensorInputType(),
        indices=IntTensorInputType(),
        updates=TensorInputType(),
        axis=IntInputType(const=True, default=0),
        mode=StringInputType(const=True, default="add"),
    )

    def __init__(self, **kwargs):
        super(scatter, self).__init__(**kwargs)

    def type_inference(self):
        if self.axis.val < -self.data.rank or self.axis.val >= self.data.rank:
            raise IndexError(
                "Axis value {} is out of bounds for {} node {}".format(
                    self.axis.val, self.op_type, self.name
                )
            )

        axis = self.axis.val
        axis = axis if axis >= 0 else axis + self.data.rank
        expected_updates_shape = (
            self.data.shape[:axis] + self.indices.shape + self.data.shape[axis + 1 :]
        )
        np.testing.assert_equal(self.updates.shape, np.array(expected_updates_shape))

        return self.data.sym_type


@register_op(doc_str="")
class gather_along_axis(Operation):
    """
    Take the values along ``axis`` at locations ``indices``.
    ``
    idx = indices[p_0, ..., p_{axis-1}, i, p_{axis+1}, ..., p_D],
    output[p_0, ..., p_{axis-1}, i, p_{axis+1}, ..., p_D] =
    = x[p_0, ..., p_{axis-1}, idx, p_{axis+1}, ..., p_D]
    ``

    Parameters
    ----------
    x: tensor<*D, T> (Required)
    indices: tensor<*K, T> (Required)
        * ``rank(indices) == rank(x)``.
    axis: const i32 (Optional):
        * Default to ``0``.

    Returns
    -------
    tensor<*D, T>:
        * output tensor has the same shape as ``indices``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=TensorInputType(),
        indices=IntTensorInputType(),
        axis=IntInputType(const=True, default=0),
    )

    def __init__(self, **kwargs):
        super(gather_along_axis, self).__init__(**kwargs)

    @precondition(allow=VALUE)
    def value_inference(self):
        x = self.x.val
        indices = self.indices.val
        axis = self.axis.val
        return np.take_along_axis(x, indices, axis)

    def type_inference(self):

        if self.x.rank != self.indices.rank:
            raise ValueError(
                "Rank mismatch between input and indices. \
                              Input rank: {}, indices rank: {}".format(
                    self.x.rank, self.indices.rank
                )
            )

        if self.axis.val < -self.x.rank or self.axis.val >= self.x.rank:
            raise IndexError(
                "Axis value {} is out of bounds for {} node {}".format(
                    self.axis.val, self.op_type, self.name
                )
            )

        axis = self.axis.val
        axis = axis if axis >= 0 else axis + self.x.rank

        for i in range(self.x.rank):
            if i != axis:
                assert self.x.shape[i] == self.indices.shape[i]

        return types.tensor(self.x.dtype, self.indices.shape)


@register_op(doc_str="")
class scatter_along_axis(Operation):
    """
    Scatter ``updates`` to ``data`` at locations ``indices`` at dimension ``axis`` by operation ``mode``.
    ``mode == update`` for instance.
    ``
    idx = indices[p_0, ..., p_{axis-1}, i, p_{axis+1}, ..., p_D]
    output[p_0, ..., p_{axis-1}, idx, p_{axis+1}, ..., p_D] =
    updates[p_0, ..., p_{axis-1}, i, p_{axis+1}, ..., p_D]
    `` for ``i`` in ``[0, len(indices)]``.
    Otherwise
    ``
    output[p_0, ..., p_{axis-1}, j, p_{axis+1}, ..., p_D] =
    data[p_0, ..., p_{axis-1}, j, p_{axis+1}, ..., p_D]
    ``

    ``mode == add`` for instance.
    ``
    idx = indices[p_0, ..., p_{axis-1}, i, p_{axis+1}, ..., p_D]
    output[p_0, ..., p_{axis-1}, idx, p_{axis+1}, ..., p_D] =
    updates[p_0, ..., p_{axis-1}, i, p_{axis+1}, ..., p_D] +
    x[p_0, ..., p_{axis-1}, indice[i], p_{axis+1}, ..., p_D]
    `` for ``i`` in ``[0, len(indices)]
    Otherwise
    ``
    output[p_0, ..., p_{axis-1}, j, p_{axis+1}, ..., p_D] =
    data[p_0, ..., p_{axis-1}, j, p_{axis+1}, ..., p_D]
    ``

    Parameters
    ----------
    data: tensor<*D, T> (Required)
    indices: tensor<*K,T> (Required)
        * ``rank(indices) == rank(data)``.
    updates: tensor<*K, T> (Required)
        * must be the same shape as ``indices``.
    axis: const i32 (Optional)
        * Default to ``0``.
    mode: const string (Optional)
        * Default to ``add``.
        * Can be the following modes:
            * ``update``
            * ``add``
            * ``sub``
            * ``mul``
            * ``div``
            * ``max``
            * ``min``

    Returns
    -------
    tensor<*D, T>
        * with the same type and shape as input ``x``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        data=TensorInputType(),
        indices=IntTensorInputType(),
        updates=TensorInputType(),
        axis=IntInputType(const=True, default=0),
        mode=StringInputType(const=True, default="add"),
    )

    def __init__(self, **kwargs):
        super(scatter_along_axis, self).__init__(**kwargs)

    @precondition(allow=VALUE)
    def value_inference(self):
        data = np.copy(self.data.val)
        indices = self.indices.val
        updates = self.updates.val
        axis = self.axis.val
        np_output = data
        np.put_along_axis(np_output, indices, updates, axis=axis)
        return np_output

    def type_inference(self):
        if self.axis.val < -self.data.rank or self.axis.val >= self.data.rank:
            raise IndexError(
                "Axis value {} is out of bounds for {} node {}".format(
                    self.axis.val, self.op_type, self.name
                )
            )

        axis = self.axis.val
        axis = axis if axis >= 0 else axis + self.data.rank

        assert self.indices.shape == self.updates.shape
        assert self.data.rank == self.indices.rank
        for i in range(self.data.rank):
            if i != axis:
                assert self.data.shape[i] == self.indices.shape[i]

        return self.data.sym_type


@register_op(doc_str="")
class gather_nd(Operation):
    """
    Gather slices from ``x`` according to ``indices``, similar to ``tf.gather_nd``.

    ``indices`` is an K-dim tensor, where ``indices[i_0,...,i_{K-2}]`` defines a slice of ``x``:

    ``output[i_0, ..., i_{K-2}]= x[indices[i_0, ..., i_{K-2}]]``

    where ``K = rank(indices)`` , and ``x[indices[i_0, ..., i_{K-2}]]`` has rank ``rank(x) - indices.shape[-1]``.

    Parameters
    ----------
    * x: tensor<*D,T>  (Required)
    * indices: tensor<*K,i32> (Required)

    Returns
    -------
    * tensor<*V,T>
        * V = K[:-1] + D[K[-1]:], where ``D = x.shape`` and ``K = indices.shape``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(x=TensorInputType(), indices=IntTensorInputType(),)

    def __init__(self, **kwargs):
        super(gather_nd, self).__init__(**kwargs)

    def type_inference(self):
        assert self.indices.shape[-1] <= self.x.rank
        out_type = self.x.dtype
        out_shape = self.indices.shape[:-1] + self.x.shape[self.indices.shape[-1] :]
        return types.tensor(out_type, out_shape)


@register_op(doc_str="")
class scatter_nd(Operation):
    """
    Scatter ``updates`` to ``data`` at locations ``indices``.\n

    ``indices`` is an K-dim tensor, where ``indices[i_0,...,i_{K-2}]`` defines a slice of ``data``,
    where ``K = rank(indices)`` , and ``data[indices[i_0, ..., i_{K-2}]]`` has rank ``rank(data) - indices.shape[-1]``.\n

    Take ``mode == update`` for instance:
    ``output`` is set to ``data`` initially, and the op updates ``output`` as following:
    ``output[indices[i_0, ..., i_{K-2}]]= updates[indices[i_0, ..., i_{K-2}]]``,
    One more example for ``mode == add``, the update rule with be:
    ``output[indices[i_0, ..., i_{K-2}]] += updates[indices[i_0, ..., i_{K-2}]]``

    Parameters
    ----------
    data: tensor<*D,T>  (Required)
    indices: tensor<*K,i32> (Required)
    updates: tensor<*K, T> (Required)
        * must be the shape as ``K[:-1]+data.shape[K[-1]:]``.
    mode: const string (Optional)
        * Default to ``add``.
        * Can be the following modes:
            * ``update``
            * ``add``
            * ``sub``
            * ``mul``
            * ``div``
            * ``max``
            * ``min``
    Returns
    -------
    tensor<*D,T>
        * a tensor with the same shape and type as ``data``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        data=TensorInputType(),
        indices=IntTensorInputType(),
        updates=TensorInputType(),
        mode=StringInputType(const=True, default="add"),
    )

    def __init__(self, **kwargs):
        super(scatter_nd, self).__init__(**kwargs)

    def type_inference(self):
        assert self.indices.shape[-1] <= self.data.rank
        expected_updates_shape = (
            self.indices.shape[:-1] + self.data.shape[self.indices.shape[-1] :]
        )
        assert is_compatible_symbolic_vector(
            self.updates.shape, tuple(expected_updates_shape)
        )
        return self.data.sym_type

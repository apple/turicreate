#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil.mil.types.symbolic import is_symbolic
from ._op_reqs import *
from ._utils import broadcast_shapes

@register_op(doc_str="")
class linear(Operation):
    """
    Performs  ``x * weight.T + bias`` where ``weight`` and ``bias`` are const at compile time.

    Parameters
    ----------
    x: tensor<[*D,D_in], T> (Required)
        * ``1 <= rank <= 3``.
        * ``0 <= rank(*D) <= 2``.
    weight: const tensor<[D_out,D_in], T> (Required)
    bias: const tensor<[D_out],T> (Optional)
        * Default to ``0``.

    Returns
    -------
    tensor<[*D,D_out], T>
        * same rank as the input ``x``.

    Attributes
    ----------
    T: fp32
    """
    input_spec = InputSpec(
        x=TensorInputType(),
        weight=TensorInputType(const=True),
        bias=TensorInputType(const=True, optional=True),
    )

    def __init__(self, **kwargs):
        super(linear, self).__init__(**kwargs)

    def type_inference(self):
        x_type = self.x.dtype
        x_shape = self.x.shape
        weight_shape = self.weight.shape
        assert len(weight_shape) == 2

        shape = list(x_shape)
        shape[-1] = weight_shape[0]
        return types.tensor(x_type, tuple(shape))

    @precondition(allow=VALUE)
    def value_inference(self):
        res = np.matmul(self.x.val, np.transpose(self.weight.val))
        if self.bias is not None:
            res += self.bias.val
        return res


@register_op(doc_str="")
class matmul(Operation):
    """
    N-D batch matrix multiplication with Numpy like broadcasting:

    * If both ``x, y`` are 1-D, return scalar from dot product.
    * If both ``x, y`` are 2D or higher, perform broadcast on the batch dims
      (all dims except the last ``2``). Ex: ``x.shape == (10, 4, 3)``, ``y.shape == (5, 10, 3, 2)``,
      ``Matmul(x, y).shape == (5, 10, 4, 2)`` Conventional matrix multiplication is
      a special case where both ``x, y`` are exactly 2D. Ex: ``x.shape == (4, 3), y.shape == (3, 2)``,
      ``Matmul(x, y).shape == (4, 2)``.
    * If ``x`` is 1-D, and ``y`` is N-D where ``N >= 2``, ``x`` is first promoted to matrix ``xm``
      by prepending a ``1`` to its dimension, and the resulting ``xm`` is broadcast to ``y`` following
      rule (2) above. Remove the inserted dim afterwards. Ex: ``x.shape == (4), y.shape == (10, 4, 3)``,
      ``xm.shape == (1, 4)``. ``Matmul(xm, y).shape == (10, 1, 3)`` removing inserted dim resulting in
      ``Matmul(x, y).shape == (10, 3)`` (``xm, Matmul(xm, y)`` are immaterial and for illustration only).
    * If ``x`` is N-D where ``N >= 2``, and ``y`` is 1-D, ``y`` is first promoted to matrix ``ym`` by
      appending a ``1`` to its dimension, and the resulting ``ym`` is broadcast to ``x`` following rule
      (2) above. Remove the inserted dim afterwards. Ex: ``x.shape == (10, 3, 4), y.shape == (4,)``,
      ``ym.shape == (4, 1)``. ``Matmul(x, ym).shape == (10, 3, 1)`` removing inserted dim resulting in
      ``Matmul(x, y).shape == (10, 3)`` (``ym, Matmul(x, ym)`` are immaterial and for illustration only).

    Parameters
    ----------
    x: tensor<[*,K1], T> (Required)
        * ``x`` must be 1D or higher.
    y: tensor<[*,K2], T> (Required)
        * ``y`` must be 1D or higher.
    transpose_x: const bool (Optional)
        * default to ``False``.
        * ``True`` to transpose the last two dimensions of ``x`` before multiplication. It has no effect when
          ``x`` is 1D.
    transpose_y: const bool (Optional)
        * default to ``False``.
        * ``True`` to transpose the last two dimensions of ``y`` before multiplication. It has no effect when
          ``y`` is 1D.

    Returns
    -------
    tensor<*, T>
        * Scalar or tensor output.

    Attributes
    ----------
    T: fp32
    """
    input_spec = InputSpec(
        x=TensorInputType(),
        y=TensorInputType(),
        transpose_x=BoolInputType(const=True, default=False),
        transpose_y=BoolInputType(const=True, default=False),
    )

    def __init__(self, **kwargs):
        super(matmul, self).__init__(**kwargs)

    def type_inference(self):
        x_type = self.x.dtype
        x_shape = list(self.x.shape)
        y_shape = list(self.y.shape)

        if self.transpose_x.val:
            x_shape = list(x_shape)
            x_shape[-1], x_shape[-2] = x_shape[-2], x_shape[-1]
            x_shape = tuple(x_shape)
        if self.transpose_y.val:
            y_shape = list(y_shape)
            y_shape[-1], y_shape[-2] = y_shape[-2], y_shape[-1]
            y_shape = tuple(y_shape)
        if not (
            x_shape[-1] == y_shape[-2]
            or is_symbolic(x_shape[-1])
            or is_symbolic(y_shape[-2])
        ):
            msg = "Op {} (matmul): x {}, y {} are not broadcastable"
            raise ValueError(msg.format(self.name, self.x.shape, self.y.shape))

        ret_shape = list(broadcast_shapes(x_shape[:-2], y_shape[:-2]))
        ret_shape += [x_shape[-2], y_shape[-1]]
        return types.tensor(x_type, tuple(ret_shape))

    @precondition(allow=VALUE)
    def value_inference(self):
        x = self.x.val
        if self.transpose_x.val:
            x = np.transpose(x)
        y = self.y.val
        if self.transpose_y.val:
            y = np.transpose(y)
        return np.matmul(x, y)

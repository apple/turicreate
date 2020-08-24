#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import scipy
from scipy import special
from ._op_reqs import *
from .elementwise_unary import elementwise_unary


@register_op(doc_str="")
class clamped_relu(Operation):
    """
    Returns elementwise ``min(beta, x)`` if ``x >= 0``, ``min(beta, alpha * x)`` otherwise.

    Parameters
    ----------
    x: tensor<*?, T> (Required)
    alpha: const fp32 (Required)
    beta: const fp32 (Required)

    Returns
    -------
    tensor<*?, T>
        * a tensor of the same type and shape as ``x``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=ScalarOrTensorInputType(),
        alpha=FloatInputType(const=True),
        beta=FloatInputType(const=True),
    )

    def __init__(self, **kwargs):
        super(clamped_relu, self).__init__(**kwargs)

    @precondition(allow=VALUE)
    def value_inference(self):
        x = np.minimum(np.maximum(self.x.val, 0), self.beta.val)
        y = np.minimum(np.minimum(self.x.val, 0) * self.alpha.val, self.beta.val)
        return x + y

    def type_inference(self):
        return self.x.sym_type


@register_op(doc_str="")
class elu(Operation):
    """
    Returns elementwise ``x`` if ``x > 0``,  ``alpha * e^(x - 1)`` otherwise.

    Parameters
    ----------
    x: tensor<*?, T> (Required)
    alpha: const fp32 (Optional)
        * Default to ``1``

    Returns
    -------
    tensor<*?, T>
        * a tensor of the same shape and type as ``x``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=ScalarOrTensorInputType(), alpha=FloatInputType(const=True, default=1),
    )

    def __init__(self, **kwargs):
        super(elu, self).__init__(**kwargs)

    @precondition(allow=VALUE)
    def value_inference(self):
        b = np.copy(self.x.val)
        b[b < 0] = self.alpha.val * (np.exp(b[b < 0]) - 1)
        return b

    def type_inference(self):
        return self.x.sym_type


@register_op(doc_str="")
class gelu(Operation):
    """
    Returns the elementwise gaussian error linear unit activation on ``x``.

    Parameters
    ----------
    x: tensor<*?, T> (Required)
    mode : const str (Optional)
        * Default to 'EXACT'.
        * Can take values:
            *"EXACT" : ``f(x) = 0.5x\left ( 1+\rm{erf}\left ( \frac{x}{\sqrt{2}} \right ) \right )``
            *"TANH_APPROXIMATION" : ``f(x) = 0.5x\left ( 1+\rm{tanh}\left ( \sqrt{2/\pi}\left ( x + 0.044715x^3 \right ) \right ) \right )``
            *"SIGMOID_APPROXIMATION" : ``f(x) = x*\rm{sigmoid}(1.702x)``

    Returns
    -------
    tensor<*?, T>
        * a tensor of the same shape and type as ``x``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=ScalarOrTensorInputType(), mode=StringInputType(const=True, default="EXACT"),
    )

    def __init__(self, **kwargs):
        super(gelu, self).__init__(**kwargs)

    @precondition(allow=VALUE)
    def value_inference(self):
        if self.mode.val == "TANH_APPROXIMATION":
            a = np.sqrt(2 / np.pi) * (self.x.val + 0.044715 * np.power(self.x.val, 3))
            return 0.5 * self.x.val * (1 + np.tanh(a))
        elif self.mode.val == "SIGMOID_APPROXIMATION":
            return self.x.val * (1 / (1 + np.exp(-(1.702 * self.x.val))))
        else:
            return 0.5 * self.x.val * (1 + scipy.special.erf(self.x.val / np.sqrt(2)))

    def type_inference(self):
        allowed_values = {"EXACT", "TANH_APPROXIMATION", "SIGMOID_APPROXIMATION"}
        if self.mode.val not in allowed_values:
            msg = '"gelu" op: unrecognized value of mode: "{}". Allowed values are {}'
            raise ValueError(msg.format(self.mode.val, allowed_values))
        return self.x.sym_type


@register_op(doc_str="")
class leaky_relu(Operation):
    """
    Elementwise apply ``x`` if ``x >= 0`` else ``alpha * x``.

    Parameters
    ----------
    x: <*?, T> (Required)
    alpha: const fp32 (Optional)
        * Default to ``0.01``.

    Returns
    -------
    tensor<*?, f32>
        * a tensor of the same shape and type as ``x``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=ScalarOrTensorInputType(), alpha=FloatInputType(const=True, default=0.01),
    )

    def __init__(self, **kwargs):
        super(leaky_relu, self).__init__(**kwargs)

    @precondition(allow=VALUE)
    def value_inference(self):
        b = np.copy(self.x.val)
        b[b < 0] *= self.alpha.val
        return b

    def type_inference(self):
        return self.x.sym_type


@register_op(doc_str="")
class linear_activation(Operation):
    """
    Applies elementwise ``x * alpha + beta``.

    Parameters
    ----------
    x: tensor<*?, T> (Required)
    alpha: const fp32 (Required)
    beta: const fp32 (Optional)
        * Default to ``0``.

    Returns
    -------
    tensor<*?, T>
        * a tensor of the same shape and type as ``x``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=ScalarOrTensorInputType(),
        alpha=FloatInputType(const=True),
        beta=FloatInputType(const=True, default=0.0),
    )

    def __init__(self, **kwargs):
        super(linear_activation, self).__init__(**kwargs)

    @precondition(allow=VALUE)
    def value_inference(self):
        return self.alpha.val * self.x.val + self.beta.val

    def type_inference(self):
        return self.x.sym_type


@register_op(doc_str="")
class prelu(Operation):
    """
    Returns ``x_i`` if ``x_i > 0``, ``alpha_i * x_i`` otherwise, where ``i = 1 ... C``.

    Parameters
    ----------
    x: tensor<[b, C, n, m], T> (Required)
    alpha: const tensor<[C], T>, (Required)

    Returns
    -------
    tensor<[b, C, n, m], f32>
        * a tensor of the same shape as ``x``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(x=TensorInputType(), alpha=TensorInputType(const=True),)

    def __init__(self, **kwargs):
        super(prelu, self).__init__(**kwargs)

    @precondition(allow=VALUE)
    def value_inference(self):
        alpha_br = self.alpha.val
        for i in range(1, len(self.x.shape)):
            alpha_br = np.expand_dims(alpha_br, i)
        x_pos = np.maximum(self.x.val, 0)
        b = np.minimum(self.x.val, 0)
        return x_pos + b * alpha_br

    def type_inference(self):
        if len(self.x.shape) < 3:
            raise ValueError("x should be at least rank 3")
        if len(self.alpha.val.shape) != 1:
            raise ValueError("alpha should be rank 1")
        if self.x.shape[-3] != self.alpha.val.shape[0]:
            raise ValueError(
                "Size of dimension 0 of alpha should be the same as "
                + "the size of dimension -3 of x."
            )
        return self.x.sym_type


@register_op(doc_str="")
class relu(elementwise_unary):
    """
    Returns elementwise applied rectified linear activation: ``min(x, 0)``.

    Parameters
    ----------
    x: tensor<*?, f32> (Required)

    Returns
    -------
    tensor<*?, f32>
        * a tensor of the same shape and type as ``x``.

    Attributes
    ----------
    T: fp32
    """

    def __init__(self, **kwargs):
        super(relu, self).__init__(**kwargs)

    @precondition(allow=VALUE)
    def value_inference(self):
        return np.maximum(self.x.val, 0)


@register_op(doc_str="")
class relu6(elementwise_unary):
    """
    Returns elementwise applied rectified linear activation: ``max(min(x, 0), 6)``.

    Parameters
    ----------
    x: tensor<*?, T> (Required)

    Returns
    -------
    tensor<*?, T>
        * a tensor of the same shape and type as ``x``.

    Attributes
    ----------
    T: fp32
    """

    def __init__(self, **kwargs):
        super(relu6, self).__init__(**kwargs)

    @precondition(allow=VALUE)
    def value_inference(self):
        return np.minimum(np.maximum(self.x.val, 0), 6)


@register_op(doc_str="")
class scaled_tanh(Operation):
    """
    Returns ``alpha * tan(beta * x)`` element-wise.

    Parameters
    ----------
    x: tensor<*?, T> (Required)
        * Input range is ``(-inf, inf)``.
    alpha: const f32 (Optional)
        * Default to ``1``.
    beta: const f32 (Optional)
        * Default to ``1``.

    Returns
    -------
    tensor<*?, f32>
        * a tensor of the same shape and type as ``x``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=ScalarOrTensorInputType(),
        alpha=FloatInputType(const=True, default=1),
        beta=FloatInputType(const=True, default=1),
    )

    def __init__(self, **kwargs):
        super(scaled_tanh, self).__init__(**kwargs)

    @precondition(allow=VALUE)
    def value_inference(self):
        return self.alpha.val * np.tanh(self.x.val * self.beta.val)

    def type_inference(self):
        return self.x.sym_type


@register_op(doc_str="")
class sigmoid(elementwise_unary):
    """
    Returns ``sigmoid(x)`` element-wise.

    Parameters
    ----------
    x: tensor<*?, T> (Required)

    Returns
    -------
    tensor<*?, T>
        * a tensor of the same shape as ``x``.

    Attributes
    ----------
    T: fp32
    """

    def __init__(self, **kwargs):
        super(sigmoid, self).__init__(**kwargs)

    @precondition(allow=VALUE)
    def value_inference(self):
        return 1 / (1 + np.exp(-self.x.val))


@register_op(doc_str="")
class sigmoid_hard(Operation):
    """
    Returns ``min( max( alpha * x + beta, 0 ), 1 )`` elementwise.

    Parameters
    ----------
    x: tensor<*?, T> (Required)
    alpha: const f32 (Optional)
        * Default to ``0.2``.
    beta: const f32 (Optional)
        * Default to ``0.5``.

    Returns
    -------
    tensor<*?, f32>, a tensor of the same shape and type as ``x``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=ScalarOrTensorInputType(),
        alpha=FloatInputType(const=True, default=0.2),
        beta=FloatInputType(const=True, default=0.5),
    )

    def __init__(self, **kwargs):
        super(sigmoid_hard, self).__init__(**kwargs)

    @precondition(allow=VALUE)
    def value_inference(self):
        return np.minimum(
            np.maximum((self.alpha.val * self.x.val) + self.beta.val, 0), 1
        )

    def type_inference(self):
        return self.x.sym_type


@register_op(doc_str="")
class softplus(elementwise_unary):
    """
    Returns ``log( 1 + e^x )`` elementwise.

    Parameters
    ----------
    x: tensor<*?, T> (Required)

    Returns
    -------
    tensor<*?, T>
        * a tensor of the same shape and type as ``x``.

    Attributes
    ----------
    T: fp32
    """

    def __init__(self, **kwargs):
        super(softplus, self).__init__(**kwargs)

    @precondition(allow=VALUE)
    def value_inference(self):
        return np.log(1 + np.exp(-np.abs(self.x.val))) + np.maximum(self.x.val, 0)


@register_op(doc_str="")
class softplus_parametric(Operation):
    """
    Returns ``alpha_i * log( 1 + e^( beta_i * x_i ) )``, where ``i = 1 ... C``.

    Parameters
    ----------
    x: tensor<[b, C, n, m], T> (Required)
    alpha: const tensor<[C], f32> (Required)
    beta: const tensor<[C], f32> (Required)

    Returns
    -------
    tensor<[b, C, n, m], T>
        * a tensor of the same shape as ``x``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=TensorInputType(),
        alpha=TensorInputType(const=True),
        beta=TensorInputType(const=True),
    )

    def __init__(self, **kwargs):
        super(softplus_parametric, self).__init__(**kwargs)

    @precondition(allow=VALUE)
    def value_inference(self):
        alpha_br = np.copy(self.alpha.val)
        beta_br = np.copy(self.beta.val)
        for i in range(1, len(self.x.val.shape)):
            alpha_br = np.expand_dims(alpha_br, i)
            beta_br = np.expand_dims(beta_br, i)
        return alpha_br * np.log(1 + np.exp(self.x.val * beta_br))

    def type_inference(self):
        if len(self.x.shape) < 3:
            raise ValueError("x should be at least rank 3")
        if len(self.alpha.val.shape) != 1:
            raise ValueError("alpha should be rank 1")
        if self.x.shape[-3] != self.alpha.val.shape[0]:
            raise ValueError(
                "Size of dimension 0 of alpha should be the same as "
                + "the size of dimension -3 of x."
            )
        if len(self.beta.val.shape) != 1:
            raise ValueError("beta should be rank 1")
        if self.x.shape[-3] != self.beta.val.shape[0]:
            raise ValueError(
                "Size of dimension 0 of beta should be the same as "
                + "the size of dimension -3 of x."
            )
        return self.x.sym_type


@register_op(doc_str="")
class softmax(Operation):
    """
    Returns ``exp(x) / tf.reduce_sum(tf.exp(x), axis)``.

    Parameters
    ----------
    x: tensor<*?, T> (Required)
    axis: const i32 (Optional)
        * Default to ``-1``.

    Returns
    -------
    tensor<*?, f32>, a tensor of the same shape and type as ``x``.

    Attributes
    ----------
    T: fp32
    """

    input_spec = InputSpec(
        x=TensorInputType(), axis=IntInputType(const=True, default=-1),
    )

    def __init__(self, **kwargs):
        super(softmax, self).__init__(**kwargs)

    def type_inference(self):
        return self.x.sym_type

    @precondition(allow=VALUE)
    def value_inference(self):
        x = self.x.val
        axis = self.axis.val
        return scipy.special.softmax(x, axis=axis)


@register_op(doc_str="")
class softsign(elementwise_unary):
    """
    Returns ``x / ( 1 + |x| )`` applied elementwise.

    Parameters
    ----------
    x: tensor<*?, T> (Required)

    Returns
    -------
    tensor<*?, T>
        * a tensor of the same shape and type as ``x``.
    """

    def __init__(self, **kwargs):
        super(softsign, self).__init__(**kwargs)

    @precondition(allow=VALUE)
    def value_inference(self):
        return self.x.val / (1 + np.abs(self.x.val))


@register_op(doc_str="")
class thresholded_relu(Operation):
    """
    Returns ``x`` if ``x >= alpha``, ``0`` otherwise.

    Parameters
    ----------
    x: tensor<*?, T> (Required)
    alpha: const f32 (Optional)
        * Default to ``1``.

    Returns
    -------
    tensor<*, T>
        * a tensor of the same shape and type as ``x``.
    """

    input_spec = InputSpec(
        x=ScalarOrTensorInputType(), alpha=FloatInputType(const=True, default=1),
    )

    def __init__(self, **kwargs):
        super(thresholded_relu, self).__init__(**kwargs)

    def type_inference(self):
        return self.x.sym_type

    @precondition(allow=VALUE)
    def value_inference(self):
        return np.maximum(self.x.val - self.alpha.val, 0)

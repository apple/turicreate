#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil.mil.types.symbolic import any_symbolic
from coremltools.converters.mil.mil import get_new_symbol, get_new_variadic_symbol
from ._op_reqs import *

"""
Random Op Superclass
"""


class RandomDistribution(Operation):
    input_spec = InputSpec(shape=IntTensorInputType(),)

    def __init__(self, **kwargs):
        super(RandomDistribution, self).__init__(**kwargs)

    def type_inference(self):
        if any_symbolic(self.shape.shape):
            # We can't infer any shape if shape has variable length.
            return types.tensor(types.fp32, (get_new_variadic_symbol(),))

        # shape has fixed length here.
        if self.shape.sym_val is None:
            shape = tuple([get_new_symbol() for _ in range(self.shape.shape[0])])
            return types.tensor(types.fp32, shape)

        return types.tensor(types.fp32, tuple(self.shape.sym_val.tolist()))


"""
Random Op Implementation(s)
"""


@register_op(
    doc_str=r"""
Returns a tensor with specified shape with random values from a Bernoulli distribution.

.. math::

    f(k) = \begin{cases}1-p  &\text{if } k = 0\\
                        p    &\text{if } k = 1\end{cases}

for :math:`k` in :math:`\{0, 1\}`.

Parameters
----------
shape: <K, i32>, required
    Target output tensor shape.
    K is the rank of the output tensor. shape[k] > 0 for k = 0,..., K-1.
prob: const<f32>, optional
    The probability of sampling 1. Defaults to 0.5.
seed: const<i32>, optional
    Seed to create a reproducible sequence of values across multiple invokes.

Returns
-------
<*, T>, a tensor of given target output shape filled with random values.

See Also
--------
random_categorical, random_normal, random_uniform
"""
)
class random_bernoulli(RandomDistribution):
    input_spec = (
        InputSpec(
            shape=IntTensorInputType(),
            prob=FloatInputType(const=True, default=0.5),
            seed=IntInputType(const=True, default=-1),
        )
        + RandomDistribution.input_spec
    )

    def __init__(self, **kwargs):
        super(random_bernoulli, self).__init__(**kwargs)


@register_op(
    doc_str=r"""
Returns random values from a categorical distribution.

Parameters
----------
shape: <*D_in, T>
    N-dimensional tensor, one of logits (event log-probabilities) or probs
    (event probabilities). The first N - 1 dimensions specifies distributions,
    the last dimension represents a vector of probabilities.
mode: const<str>, optional
    One of ['logits', 'probs']. Defaults to 'logits'.
size: const<i32>, optional
    Number of samples to draw. Defaults to 1.
seed: const<i32>, optional
    Seed to create a reproducible sequence of values across multiple invokes.

Returns
-------
<*D_in[:-1] + [size], T>, a tensor of given target output shape filled with random values.

See Also
--------
random_bernoulli, random_normal, random_uniform
"""
)
class random_categorical(Operation):
    input_spec = InputSpec(
        x=TensorInputType(),
        mode=StringInputType(const=True, default="logits"),
        size=IntInputType(const=True, default=1),
        seed=IntInputType(const=True, default=-1),
    )

    def __init__(self, **kwargs):
        super(random_categorical, self).__init__(**kwargs)

    def type_inference(self):
        output_shape = self.x.shape[:-1] + (self.size.val,)
        return types.tensor(types.fp32, output_shape)


@register_op(
    doc_str=r"""
Returns a tensor with specified shape with random values from a normal distribution.

.. math::

    f(x) = \frac{\exp(-x^2/2)}{\sqrt{2\pi}}

for a real number :math:`x`.

Parameters
----------
shape: <K, i32>, required
    Target output tensor shape.
    K is the rank of the output tensor. shape[k] > 0 for k = 0,..., K-1.
mean: const<f32>, optional
    The mean (center) of the normal distribution. Defaults to 0.0.
stddev: const<f32>, optional
    The standard deviation (width) of the normal distribution. Defaults to 1.0.
seed: const<i32>, optional
    Seed to create a reproducible sequence of values across multiple invokes.

Returns
-------
<*, T>, a tensor of given target output shape filled with random values.

See Also
--------
random_categorical, random_bernoulli, random_uniform
"""
)
class random_normal(RandomDistribution):
    input_spec = (
        InputSpec(
            shape=IntTensorInputType(),
            mean=FloatInputType(const=True, default=0.0),
            stddev=FloatInputType(const=True, default=1.0),
            seed=IntInputType(const=True, default=-1),
        )
        + RandomDistribution.input_spec
    )

    def __init__(self, **kwargs):
        super(random_normal, self).__init__(**kwargs)


@register_op(
    doc_str=r"""
Returns a tensor with specified shape with random values from a normal distribution.

.. math::

    p(x) = \frac{1}{high - low}

for a real number :math:`x`.

Parameters
----------
shape: <K, i32>, required
    Target output tensor shape.
    K is the rank of the output tensor. shape[k] > 0 for k = 0,..., K-1.
low: const<f32>, optional
    Lower boundary of the output interval (inclusive). Defaults to 0.0.
high: const<f32>, optional
    Upper boundary of the output interval (exclusive). Defaults to 1.0.
seed: const<i32>, optional
    Seed to create a reproducible sequence of values across multiple invokes.

Returns
-------
<*, T>, a tensor of given target output shape filled with random values.

See Also
--------
random_categorical, random_bernoulli, random_normal
"""
)
class random_uniform(RandomDistribution):
    input_spec = (
        InputSpec(
            shape=IntTensorInputType(),
            low=FloatInputType(const=True, default=0.0),
            high=FloatInputType(const=True, default=1.0),
            seed=IntInputType(const=True, default=-1),
        )
        + RandomDistribution.input_spec
    )

    def __init__(self, **kwargs):
        super(random_uniform, self).__init__(**kwargs)

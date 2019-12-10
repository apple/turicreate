# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

"""
Neural Network optimizer utilities.
"""


class AdamParams(object):
    """
    Adam - A Method for Stochastic Optimization.

    Attributes
    ----------
    lr: float
        The learning rate that controls learning step size. Adjustable in progress, default: 0.01.
    batch: int
        The mini-batch size, number of examples used to compute single gradient step, default: 10.
    beta1: float
        Controls the exponential decay rate for the first moment estimates, default: 0.9.
    beta2: float
        Controls the exponential decay rate for the second moment estimates, default: 0.999.
    eps: float
        The epsilon, a very small number to prevent any division by zero in the implementation, default: 1e-8.

    Methods
    -------
    set_lr(value, min, max)
        Set value for learning rate.
    set_batch(value, allow_set)
        Set value for batch size.
    set_beta1(value, min, max)
        Set value for beta1.
    set_beta2(value, min, max)
        Set value for beta2.
    set_eps(value, min, max)
        Set value for epsilon.
    """

    def __init__(self, lr=1e-2, batch=10, beta1=0.9, beta2=0.999, eps=1e-8):
        self._lr = RangeParam(lr)
        self._batch = Batch(batch)
        self._beta1 = RangeParam(beta1)
        self._beta2 = RangeParam(beta2)
        self._eps = RangeParam(eps)

    def set_lr(self, value, min, max):
        self._lr = RangeParam(value, min, max)

    def set_batch(self, value, allowed_set):
        self._batch = Batch(value, allowed_set)

    def set_beta1(self, value, min, max):
        self._beta1 = RangeParam(value, min, max)

    def set_beta2(self, value, min, max):
        self._beta2 = RangeParam(value, min, max)

    def set_eps(self, value, min, max):
        self._eps = RangeParam(value, min, max)

    @property
    def lr(self):
        return self._lr

    @property
    def batch(self):
        return self._batch

    @property
    def beta1(self):
        return self._beta1

    @property
    def beta2(self):
        return self._beta2

    @property
    def eps(self):
        return self._eps


class SgdParams(object):
    """
    SGD - Stochastic Gradient Descent optimizer.

    Attributes
    ----------
    lr: float
        The learning rate that controls learning step size. Adjustable in progress, default: 0.01.
    batch: int
        The mini-batch size, number of examples used to compute single gradient step, default: 10.
    momentum: float
        The momentum factor that helps accelerate gradients vectors in the right direction, default 0.

    Methods
    -------
    set_lr(value, min, max)
        Set value for learning rate.
    set_batch(value, allow_set)
        Set value for batch size.
    set_momentum(value, min, max)
        Set value for momentum.
    """

    def __init__(self, lr=1e-2, batch=10, momentum=0):
        self._lr = RangeParam(lr)
        self._batch = Batch(batch)
        self._momentum = RangeParam(momentum)

    def set_lr(self, value, min, max):
        self._lr = RangeParam(value, min, max)

    def set_batch(self, value, allowed_set):
        self._batch = Batch(value, allowed_set)

    def set_momentum(self, value, min, max):
        self._momentum = RangeParam(value, min, max)

    @property
    def lr(self):
        return self._lr

    @property
    def batch(self):
        return self._batch

    @property
    def momentum(self):
        return self._momentum


class RangeParam:
    def __init__(self, value, min=0, max=1):
        self._value = value
        if min >= max:
            raise ValueError("min value must be less than max value.")
        self._min = min
        self._max = max

    @property
    def value(self):
        return self._value

    @property
    def min(self):
        return self._min

    @property
    def max(self):
        return self._max


class Batch:
    def __init__(self, value, allowed_set=None):
        self._value = value
        if allowed_set is None:
            self._allowed_set = [value]
        else:
            if len(allowed_set) > len(set(allowed_set)):
                raise ValueError("values in allowed_set must be unique.")
            self._allowed_set = allowed_set

    @property
    def value(self):
        return self._value

    @property
    def allowed_set(self):
        return self._allowed_set

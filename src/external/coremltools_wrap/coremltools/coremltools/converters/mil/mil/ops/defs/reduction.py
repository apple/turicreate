#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import scipy
from ._op_reqs import *

"""
Reduction Op Superclasses
"""


class ReductionAxes(Operation):
    input_spec = InputSpec(
        x=TensorInputType(),
        axes=IntTensorInputType(const=True, optional=True, default=None),
        keep_dims=BoolInputType(const=True, optional=True, default=False),
    )

    def __init__(self, **kwargs):
        super(ReductionAxes, self).__init__(**kwargs)

    def type_inference(self):
        x_type = self.x.dtype
        x_shape = self.x.shape
        axes = self.axes.val if self.axes is not None else None
        if axes is None:
            axes = range(self.x.rank)
        keep_dims = self.keep_dims.val

        reduced_shape = list(x_shape)
        if keep_dims:
            for i in axes:
                reduced_shape[i] = 1
        else:
            # sort reverse so we can delete shape elements back to front
            axes = [axis if axis >= 0 else axis + len(reduced_shape) for axis in axes]
            for i in sorted(axes)[::-1]:
                reduced_shape.pop(i)
        if len(reduced_shape) == 0:
            return x_type  # scalar

        return types.tensor(x_type, tuple(reduced_shape))

    @precondition(allow=VALUE)
    def value_inference(self):
        axes = tuple(self.axes.val) if self.axes is not None else None
        return self.get_operator()(self.x.val, axis=axes, keepdims=self.keep_dims.val)

    def get_operator(self):
        raise NotImplementedError()


class ReductionAxis(Operation):
    input_spec = InputSpec(
        x=TensorInputType(),
        axis=IntInputType(const=True, optional=True, default=-1),
        keep_dims=BoolInputType(const=True, optional=True, default=False),
    )

    def __init__(self, **kwargs):
        super(ReductionAxis, self).__init__(**kwargs)

    def type_inference(self):
        x_type = self.x.dtype
        x_shape = self.x.shape
        axis = self.axis.val

        reduced_shape = list(x_shape)
        axis = axis if axis >= 0 else axis + len(reduced_shape)
        if self.keep_dims.val:
            reduced_shape[axis] = 1
        else:
            reduced_shape.pop(axis)

        return types.tensor(x_type, tuple(reduced_shape))

    @precondition(allow=VALUE)
    def value_inference(self):
        return self.get_operator()(self.x.val, axis=self.axis.val)

    def get_operator(self):
        raise NotImplementedError()


"""
Reduction op implementations
"""


@register_op(doc_str="TODO")
class reduce_arg(ReductionAxis):
    def __init__(self, **kwargs):
        super(reduce_arg, self).__init__(**kwargs)

    def type_inference(self):
        x_shape = self.x.shape
        axis = self.axis.val

        reduced_shape = list(x_shape)
        axis = axis if axis >= 0 else axis + len(reduced_shape)
        if self.keep_dims.val:
            reduced_shape[axis] = 1
        else:
            reduced_shape.pop(axis)

        return types.tensor(types.int32, tuple(reduced_shape))


@register_op(doc_str="TODO")
class reduce_argmax(reduce_arg):
    def __init__(self, **kwargs):
        super(reduce_argmax, self).__init__(**kwargs)

    def get_operator(self):
        return np.argmax


@register_op(doc_str="TODO")
class reduce_argmin(reduce_arg):
    def __init__(self, **kwargs):
        super(reduce_argmin, self).__init__(**kwargs)

    def get_operator(self):
        return np.argmin


@register_op(doc_str="TODO")
class reduce_l1_norm(ReductionAxes):
    def __init__(self, **kwargs):
        super(reduce_l1_norm, self).__init__(**kwargs)

    def get_operator(self):
        def l1_norm(x, axis=None, keepdims=False):
            return np.sum(np.abs(x), axis=axis, keepdims=keepdims)

        return l1_norm


@register_op(doc_str="TODO")
class reduce_l2_norm(ReductionAxes):
    def __init__(self, **kwargs):
        super(reduce_l2_norm, self).__init__(**kwargs)

    def get_operator(self):
        def l2_norm(x, axis=None, keepdims=False):
            return np.sqrt(np.sum(np.square(x), axis=axis, keepdims=keepdims))

        return l2_norm


@register_op(doc_str="TODO")
class reduce_log_sum(ReductionAxes):
    def __init__(self, **kwargs):
        super(reduce_log_sum, self).__init__(**kwargs)

    def get_operator(self):
        def log_sum(x, axis=None, keepdims=False):
            return np.log(np.sum(x, axis=axis, keepdims=keepdims))

        return log_sum


@register_op(doc_str="TODO")
class reduce_log_sum_exp(ReductionAxes):
    def __init__(self, **kwargs):
        super(reduce_log_sum_exp, self).__init__(**kwargs)

    def get_operator(self):
        return scipy.special.logsumexp


@register_op(doc_str="TODO")
class reduce_max(ReductionAxes):
    def __init__(self, **kwargs):
        super(reduce_max, self).__init__(**kwargs)

    def get_operator(self):
        return np.max


@register_op(doc_str="TODO")
class reduce_mean(ReductionAxes):
    def __init__(self, **kwargs):
        super(reduce_mean, self).__init__(**kwargs)

    def get_operator(self):
        return np.mean


@register_op(doc_str="TODO")
class reduce_min(ReductionAxes):
    def __init__(self, **kwargs):
        super(reduce_min, self).__init__(**kwargs)

    def get_operator(self):
        return np.min


@register_op(doc_str="TODO")
class reduce_prod(ReductionAxes):
    def __init__(self, **kwargs):
        super(reduce_prod, self).__init__(**kwargs)

    def get_operator(self):
        return np.prod


@register_op(doc_str="TODO")
class reduce_sum(ReductionAxes):
    def __init__(self, **kwargs):
        super(reduce_sum, self).__init__(**kwargs)

    def get_operator(self):
        return np.sum


@register_op(doc_str="TODO")
class reduce_sum_square(ReductionAxes):
    def __init__(self, **kwargs):
        super(reduce_sum_square, self).__init__(**kwargs)

    def get_operator(self):
        def sum_squre(x, axis=None, keepdims=False):
            return np.sum(np.square(x), axis=axis, keepdims=keepdims)

        return sum_squre

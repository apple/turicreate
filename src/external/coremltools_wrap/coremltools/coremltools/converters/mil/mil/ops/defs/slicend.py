#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import division

from coremltools.converters.mil.mil.types.symbolic import is_symbolic
from coremltools.converters.mil.mil import get_new_symbol
from ._op_reqs import *


@register_op(doc_str="")
class slice_by_index(Operation):
    """
    Method for numpy style indexing and slicing.
    Suppose we have a tensor ``x``, this method achieves:
    ``result = x[begin[0]: end[0]: stride[0], begin[1]: end[1]: stride[1], ...]``
    Note this method does not support pure indexing. You would need to do squeeze if indexing is intended.

    Parameters
    ----------
    x: tensor<*?, T> (Required)
        * Input tensor
    begin: tensor<[rank<x>], i32> (Required)
        * Starting index for the dimension of slicing.
    end: tensor<[rank(x)], i32> (Required)
        * Ending index for the dimension of slicing.
    stride: tensor<[rank(x)], i32> (Optional)
        * Default as all ``1``s.
        * Stride for the dimension of slicing.
    begin_mask: tensor<[rank(x)], bool> (Optional)
        * Default to all ``False``.
        * If ``begin_mask[i]==True``, neglect ``begin[i]``, and set ``begin[i]`` to ``0``.
    end_mask: tensor<[rank(x)], bool> (Optional)
        * Default to all ``False``.
        * If ``end_mask[i]==True``, neglect ``end[i]``, and set ``end[i]`` to ``x.shape[i]``.
    squeeze_mask: tensor<[rank(x)], bool> (Optional)
        * Default to all ``False``.
        * If ``squeeze_mask[i]==true``, neglect ``end[i]``, and do the pure index at ``begin[i]``.

    Returns
    -------
    tensor<*?, T>
        - Scalar or tensor.

    Attributes
    ----------
    T: fp32

    """

    input_spec = InputSpec(
        x=TensorInputType(),
        begin=IntTensorInputType(),
        end=IntTensorInputType(),
        stride=IntTensorInputType(const=True, optional=True),
        begin_mask=BoolTensorInputType(const=True, optional=True),
        end_mask=BoolTensorInputType(const=True, optional=True),
        squeeze_mask=BoolTensorInputType(const=True, optional=True),
    )

    def __init__(self, **kwargs):
        super(slice_by_index, self).__init__(**kwargs)

    def type_inference(self):

        # get tensor and set default value
        begin = self.begin.val
        end = self.end.val
        x_rank = self.x.rank
        stride = self.stride.val if self.stride is not None else [1] * x_rank
        begin_mask = (
            self.begin_mask.val if self.begin_mask is not None else [False] * x_rank
        )
        end_mask = self.end_mask.val if self.end_mask is not None else [False] * x_rank
        squeeze_mask = (
            self.squeeze_mask.val if self.squeeze_mask is not None else [False] * x_rank
        )

        # solve shape
        x_shape = self.x.shape
        ret_shape = []

        if begin is None or len(begin) == 0:
            begin = [None] * len(x_shape)
        if end is None or len(end) == 0:
            end = [None] * len(x_shape)

        # solve for shape inference
        for idx in range(len(x_shape)):
            # skip if we want to squeeze the dimension
            if squeeze_mask[idx]:
                continue

            # for those a[:] cases
            if begin_mask[idx] and end_mask[idx]:
                if is_symbolic(x_shape[idx]):
                    if stride[idx] == -1 or stride[idx] == 1:
                        ret_shape.append(x_shape[idx])
                    else:
                        ret_shape.append(get_new_symbol())
                    continue
                else:
                    num = np.ceil(float(x_shape[idx]) / abs(stride[idx])).astype(
                        np.int32
                    )
                    ret_shape.append(num)
                    continue

            # for symbolic case
            if is_symbolic(x_shape[idx]):
                ret_shape.append(get_new_symbol())
                continue

            # when begin and end are not determined
            if begin[idx] is None and not begin_mask[idx]:
                ret_shape.append(get_new_symbol())
                continue
            if end[idx] is None and not end_mask[idx]:
                ret_shape.append(get_new_symbol())
                continue

            # parse negative dimention
            if begin[idx] is not None and begin[idx] < 0:
                begin[idx] = max(0, begin[idx] + x_shape[idx])
            if end[idx] is not None and end[idx] < 0:
                end[idx] = max(0, end[idx] + x_shape[idx])

            # compute shape
            low, high = [0, x_shape[idx]] if stride[idx] > 0 else [-1, x_shape[idx] - 1]
            begin_idx, end_idx = (
                [begin[idx], end[idx]] if stride[idx] > 0 else [end[idx], begin[idx]]
            )
            is_begin_mask, is_end_mask = (
                [begin_mask[idx], end_mask[idx]]
                if stride[idx] > 0
                else [end_mask[idx], begin_mask[idx]]
            )
            if is_begin_mask:
                begin_idx = low
            end_idx = high if is_end_mask else min(end_idx, high)
            num = np.ceil(float(end_idx - begin_idx) / abs(stride[idx])).astype(
                np.int32
            )
            ret_shape.append(max(0.0, num))

        if len(ret_shape) == 0:
            # Scalar case.
            return self.x.dtype
        else:
            return types.tensor(self.x.dtype, tuple(ret_shape))

    def value_inference(self):
        if self.x.sym_val is None or self.begin.val is None or self.end.val is None:
            return None
        x_shape = self.x.shape
        begin = [int(i) for i in list(self.begin.val[:])]
        end = [int(i) for i in list(self.end.val[:])]
        stride = [1] * self.x.rank if self.stride is None else self.stride.val
        begin_mask = (
            [False] * self.x.rank if self.begin_mask is None else self.begin_mask.val
        )
        end_mask = [False] * self.x.rank if self.end_mask is None else self.end_mask.val
        squeeze_mask = (
            [False] * self.x.rank
            if self.squeeze_mask is None
            else self.squeeze_mask.val
        )

        slices = []
        for idx, mask in enumerate(begin_mask):
            if mask:
                begin[idx] = None
        for idx, mask in enumerate(end_mask):
            if mask:
                end[idx] = None
        squeeze_axes = []
        for idx, mask in enumerate(squeeze_mask):
            if mask:
                end[idx] = None
                stride[
                    idx
                ] = 2147483647  # We slice out only 1 element by setting stride to INF
                squeeze_axes.append(idx)
        for idx in range(self.x.rank):
            slices.append(slice(begin[idx], end[idx], stride[idx]))

        slices = tuple(slices)
        res = self.x.sym_val[slices]

        # remove squeezed axes
        if len(squeeze_axes) > 0:
            if len(squeeze_axes) == len(res.shape):
                if len(res) == 0:
                    logging.warning("%s seems to be a 0 sized tensor", self.name)
                    return np.array([])
                res = res.tolist()[0]
                if is_symbolic(res):
                    return res
                elif self.x.dtype == types.int32 or self.x.dtype == types.int64:
                    res = np.int32(res)
                elif self.x.dtype == types.float or self.x.dtype == types.double:
                    res = np.float32(res)
                else:
                    raise ValueError(
                        "Unable to convert type {}".format(self.x.sym_val.dtype)
                    )
            else:
                res = np.squeeze(res, axis=tuple(squeeze_axes))
        return res

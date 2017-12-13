# -*- coding: utf-8 -*-
# pylint: disable= too-many-lines, redefined-builtin
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""SFrame Data Iterator. (Requires SFrame package)"""
from __future__ import absolute_import as _
from __future__ import print_function as _
from __future__ import division as _

import ctypes
import array as _array
import numpy as np
from collections import OrderedDict

from mxnet.io import DataIter
from mxnet.ndarray import NDArray
from mxnet.ndarray import array


from turicreate import SFrame, SArray, Image
from turicreate import extensions as sf_extension


def _copy_from_sframe(sf, buf, start, end, field_length, bias=0):
    assert isinstance(sf, SFrame)
    sf_extension.sframe_load_to_numpy(sf, buf.ctypes.data + buf.strides[0] * bias, buf.strides, field_length, start, end)


def _copy_from_sarray(sa, buf, start, end, field_length, bias=0):
    assert isinstance(sa, SArray)
    sf = SFrame({'__tmp__': sa})
    _copy_from_sframe(sf, buf, start, end, [field_length], bias)


def _init_data(data, allow_empty, default_name):
    """Convert data into canonical form."""
    assert (data is not None) or allow_empty
    if data is None:
        data = []

    if isinstance(data, (np.ndarray, NDArray)):
        data = [data]
    if isinstance(data, list):
        if not allow_empty:
            assert(len(data) > 0)
        if len(data) == 1:
            data = OrderedDict([(default_name, data[0])])
        else:
            data = OrderedDict([('_%d_%s' % (i, default_name), d) for i, d in enumerate(data)])
    if not isinstance(data, dict):
        raise TypeError("Input must be NDArray, numpy.ndarray, " + \
                "a list of them or dict with them as values")
    for k, v in data.items():
        if isinstance(v, NDArray):
            data[k] = v.asnumpy()
    for k, v in data.items():
        if not isinstance(v, np.ndarray):
            raise TypeError(("Invalid type '%s' for %s, "  % (type(v), k)) + \
                    "should be NDArray or numpy.ndarray")
    return list(data.items())

class SFrameIter(DataIter):
    """DataIter from SFrame
    Provides DataIter interface for SFrame, a highly scalable columnar DataFrame.
    The iterator can simultaneously iterate over multiple columns indicated by `data_field` and `label_field`.
    `data_field` can refer either a single image typed column or multiple numerical columns (int, float or array).
    `label_field` con only refer to a single numerical column (int, float or array).

    Parameters
    ----------
    sframe : SFrame object
        source SFrame
    data_field : string or list(string)
        data fields of the SFrame. The selected fields may be either a single image typed column,
        or multiple numerical columns (int, float, array).
    label_field : string, optional
        label field in SFrame
    batch_size : int, optional
        batch size

    Examples
    --------
    >>> import turicreate as tc
    >>> import mxnet as mx

    >>> data = tc.SFrame({'x': [1,2,3], 'y': [.1, .5, .5], 'z': [[1,1,1], [2,2,2,], [3,3,3]]})
    >>> dataiter = mx.io.SFrameIter(sframe=data, data_field=['x', 'z'], label_field='z')

    Notes
    -----
    - Image column must contain images of the same size.
    - Array column must contain arrays of the same length.
    """

    def __init__(self, sframe, data_field, label_field=None, batch_size=1, data_name='data', label_name='softmax_label'):

        super(SFrameIter, self).__init__()
        if not isinstance(sframe, SFrame):
            raise TypeError
        if not (isinstance(data_field, str) or isinstance(data_field, list)):
            raise TypeError
        if not (label_field is None or isinstance(label_field, str)):
            raise TypeError

        if type(data_field) is str:
            data_field = [data_field]

        self._type_check(sframe, data_field, label_field)
        self.data_field = data_field
        self.label_field = label_field
        self.data_sframe = sframe[data_field]
        if label_field is not None:
            self.label_sframe = sframe[label_field]

        # allocate ndarray
        inferred_shape = self.infer_shape()
        data_shape = list(inferred_shape["final_shape"])
        data_shape.insert(0, batch_size)
        self.data_shape = tuple(data_shape)
        self.label_shape = (batch_size, )
        self.field_length = inferred_shape["field_length"]
        self.data_ndarray = np.zeros(self.data_shape, dtype=np.float32)
        self.label_ndarray = np.zeros(self.label_shape, dtype=np.float32)
        self.data_mx_ndarray = None
        self.label_mx_ndarray = None
        self.data = _init_data(self.data_ndarray, allow_empty=False, default_name=data_name)
        self.label = _init_data(self.label_ndarray, allow_empty=True, default_name=label_name)
        # size
        self.batch_size = batch_size
        self.data_size = len(sframe)
        self.reset()

    @property
    def provide_data(self):
        """The name and shape of data provided by this iterator"""
        return [(k, tuple([self.batch_size] + list(v.shape[1:]))) for k, v in self.data]

    @property
    def provide_label(self):
        """The name and shape of label provided by this iterator"""
        return [(k, tuple([self.batch_size] + list(v.shape[1:]))) for k, v in self.label]

    def reset(self):
        self.pad = 0
        self.cursor = 0
        self.has_next = True

    def _type_check(self, sframe, data_field, label_field):
        if label_field is not None:
            label_column_type = sframe[label_field].dtype
            if label_column_type not in [int, float]:
                raise TypeError('Unexpected type for label_field \"%s\". Expect int or float, got %s' %
                                (label_field, str(label_column_type)))
        for col in data_field:
            col_type = sframe[col].dtype
            if col_type not in [int, float, _array.array, Image]:
                raise TypeError('Unexpected type for data_field \"%s\". Expect int, float, array or image, got %s' %
                               (col, str(col_type)))

    def _infer_column_shape(self, sarray):
        dtype = sarray.dtype
        if (dtype in [int, float]):
            return (1, )
        elif dtype is _array.array:
            lengths = sarray.item_length()
            if lengths.min() != lengths.max():
                raise ValueError('Array column does not have the same length')
            else:
                return (lengths.max(), )
        elif dtype is Image:
            first_image = sarray.head(1)[0]
            if first_image is None:
                raise ValueError('Column cannot contain missing value')
            return (first_image.channels, first_image.height, first_image.width)

    def infer_shape(self):
        ret = {"field_length": [], "final_shape": None}
        features = self.data_sframe.column_names()
        assert len(features) > 0
        if len(features) > 1:
            # If more than one feature, all features must be numeric or array
            shape = 0
            for col in features:
                colshape = self._infer_column_shape(self.data_sframe[col])
                if len(colshape) != 1:
                    raise ValueError('Only one column is allowed if input is image typed')
                shape += colshape[0]
                ret["field_length"].append(colshape[0])
            ret["final_shape"] = (shape,)
        else:
            col_shape = self._infer_column_shape(self.data_sframe[features[0]])
            ret["final_shape"] = col_shape
            length = 1
            for x in col_shape:
                length = length * x
            ret["field_length"].append(length)
        return ret

    def _copy(self, start, end, bias=0):
        _copy_from_sframe(self.data_sframe, self.data_ndarray, start, end, self.field_length, bias)
        self.data_mx_ndarray = None
        if self.label_field is not None:
            _copy_from_sarray(self.label_sframe, self.label_ndarray, start, end, 1, bias)
            self.label_mx_ndarray = None

    def iter_next(self):
        if self.has_next:
            start = self.cursor
            end = start + self.batch_size
            if end >= self.data_size:
                self.has_next = False
                self.pad = end - self.data_size
                end = self.data_size
            self._copy(start, end)
            if self.pad > 0:
                bias = self.batch_size - self.pad
                start = 0
                end = self.pad
                self._copy(start, end, bias)
                self.cursor = self.pad
            else:
                self.cursor += self.batch_size
            return True
        else:
            return False

    def getdata(self):
        if self.data_mx_ndarray is None:
            self.data_mx_ndarray = array(self.data_ndarray)
        return [self.data_mx_ndarray]

    def getlabel(self):
        if self.label_field is None:
            return None
        if self.label_mx_ndarray is None:
            self.label_mx_ndarray = array(self.label_ndarray)
        return [self.label_mx_ndarray]

    def getpad(self):
        return self.pad


class SFrameImageIter(SFrameIter):
    """Image Data Iterator from SFrame
    Provide the SFrameIter like interface with options to normalize and augment image data.

    Parameters
    ----------
    sframe : SFrame object
        source SFrame
    data_field : string
        image data field of the SFrame.
    label_field : string, optional
        label field in SFrame
    batch_size : int, optional
        batch size
    mean_r : float, optional
        normalize the image by subtracting the mean value of r channel, or the first channel for
    mean_g : float, optional
        normalize the image by subtracting the mean value of g channel
    mean_b : float, optional
        normalize the image by subtracting the mean value of b channel
    mean_nd : np.ndarray, optional
        normalize the image by subtracting the ndarray of mean pixel values.
        The mean_nd array stores the pixel values in the order of [height, width, channel]
        This option will suppress mean_r, mean_g, and mean_b.
    scale : float, optional
        multiply each pixel value by the scale (this operation is performed after mean subtraction)
    random_flip : bool, optional
        Randomly flip horizontally on the fly, useful to augment data for training neural network.
    **kwargs :
        placeholder for new parameters

    Examples
    --------
    >>> import turicreate as tc
    >>> import mxnet as mx

    >>> image_data = tc.image_analysis.load_images('/path/to/directory/with/images')
    >>> image_data_iter = mx.io.SFrameImageIter(sframe=data, data_field=['image'], label_field='label', batch_size=100,
                                                mean_r=117, scale=0.5)

    Notes
    -----
    - Image column must contain images of the same size.
    """

    def __init__(self, sframe, data_field, label_field=None, batch_size=1,
                 data_name='data', label_name='softmax_label',
                 mean_r=0.0,
                 mean_g=0.0,
                 mean_b=0.0,
                 mean_nd=None,
                 scale=1.0,
                 random_flip=False,
                 **kwargs):
        super(SFrameImageIter, self).__init__(sframe, data_field, label_field, batch_size,
                                              data_name, label_name)

        # Mean subtraction parameters
        self._rgb_mask = np.zeros(self.data_shape)
        if mean_nd is None:
            nchannels = self.data_shape[1]
            mean_per_channel = [mean_r, mean_g, mean_b][:nchannels]
            for i in range(nchannels):
                self._rgb_mask[:, i, :, :] = mean_per_channel[i]
        elif type(mean_nd) == np.ndarray:
            mean_nd = np.swapaxes(mean_nd, 0, 2) # h, w, c -> c, w, h
            mean_nd = np.swapaxes(mean_nd, 1, 2) # c, w, h -> c, h, w
            if mean_nd.shape == self.data_shape[1:]:
                for i in range(self.data_shape[0]):
                    self._rgb_mask[i,:] = mean_nd
            else:
                raise ValueError('Shape mismatch. mean_nd has different shape from input image')
        else:
            raise TypeError('mean_nd must be type np.ndarray')
        self._rgb_mask = array(self._rgb_mask)

        # Rescale parameters
        self._scale = scale

        #Augmentation parameters
        self._random_flip = random_flip
    def _type_check(self, sframe, data_field, label_field):
        if label_field is not None:
            label_column_type = sframe[label_field].dtype
            if label_column_type not in [int, float]:
                raise TypeError('Unexpected type for label_field \"%s\". Expect int or float, got %s' %
                                (label_field, str(label_column_type)))
        for col in data_field:
            col_type = sframe[col].dtype
            if col_type not in [Image]:
                raise TypeError('Unexpected type for data_field \"%s\". Expect or image, got %s' %
                               (col, str(col_type)))

    def _infer_column_shape(self, sarray):
        dtype = sarray.dtype
        if not dtype is Image:
            raise TypeError('Data column must be image type')

        first_image = sarray.head(1)[0]
        if first_image is None:
            raise ValueError('Column cannot contain missing value')
        return (first_image.channels, first_image.height, first_image.width)

    def iter_next(self):
        ret = super(self.__class__, self).iter_next()
        # Postprocess: normalize by mean, scale, ...
        self.data_ndarray = (self.data_ndarray - self._rgb_mask.asnumpy()) * self._scale
        # random flip
        if self._random_flip:
            self.data_ndarray = array(self.data_ndarray[:,:,:,::(np.random.randint(2)- 0.5) * 2])
        return ret

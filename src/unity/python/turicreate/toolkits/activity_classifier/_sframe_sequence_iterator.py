# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
SFrame iterator for data containing multiple time-series sequences.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from math import ceil as _ceil
import time as _time
import mxnet as _mx
import numpy as _np

from turicreate import extensions as _extensions


def prep_data(data, features, session_id, prediction_window, predictions_in_chunk, target=None, verbose=True):
    """
    Convert SFrame to batch form, where each row contains a sequence of length
    predictions_in_chunk * prediction_window, and there is a single label per
    prediction window.
    """
    if target is None:
        target = ""
    if verbose:
        result_dict = _extensions._activity_classifier_prepare_data_verbose(
            data, features, session_id, prediction_window, predictions_in_chunk, target)
    else:
        result_dict = _extensions._activity_classifier_prepare_data(
            data, features, session_id, prediction_window, predictions_in_chunk, target)


    return result_dict["converted_data"] , result_dict["num_of_sessions"]


def _ceil_dev(x, m):
    return int(_ceil(x / float(m)))


def _load_into_numpy(sf, np_array, start, end, strides=None, shape=None):
    """Loads into numpy array from SFrame, assuming SFrame stores data flattened"""
    np_array[:] = 0.0
    np_array_2d = np_array.reshape((np_array.shape[0], np_array.shape[1] * np_array.shape[2]))
    _extensions.sframe_load_to_numpy(sf, np_array.ctypes.data,
                                     np_array_2d.strides, np_array_2d.shape[1:], start, end)


class SFrameSequenceIter(_mx.io.DataIter):
    def __init__(self, chunked_dataset, features_size, prediction_window,
                 predictions_in_chunk, batch_size, use_target=False, use_pad=False):

        self.dataset = chunked_dataset
        self.use_target = use_target
        self.batch_size = batch_size
        self.use_pad = use_pad

        self.num_rows = len(self.dataset)
        self.num_batches = _ceil_dev(len(self.dataset), self.batch_size)

        self.batch_idx = 0

        data_len = predictions_in_chunk * prediction_window
        self.data_shape = (batch_size, data_len, features_size)
        self.target_shape = (batch_size, predictions_in_chunk, 1)
        self._provide_data = [('data', self.data_shape)]
        self.features_sf = self.dataset[['features']]

        if self.use_target:
            self._provide_label = [('target', self.target_shape),
                                   ('weights', self.target_shape)]
            self.targets_sf = self.dataset[['target']]
            self.weights_sf = self.dataset[['weights']]

    def fetch_curr_batch_as_numpy(self):
        start = self.batch_idx * self.batch_size
        end = start + self.batch_size

        actual_end = min(self.num_rows, end)

        np_features = _np.empty((self.batch_size,) + self.data_shape[1:], dtype=_np.float32)
        _load_into_numpy(self.features_sf, np_features, start, actual_end)
        if self.use_target:
            np_targets = _np.empty((self.batch_size,) + self.target_shape[1:], dtype=_np.float32)
            np_weights = _np.empty((self.batch_size,) + self.target_shape[1:], dtype=_np.float32)
            _load_into_numpy(self.targets_sf, np_targets, start, actual_end)
            _load_into_numpy(self.weights_sf, np_weights, start, actual_end)
        else:
            np_targets = np_weights = None

        return (np_features, np_targets, np_weights)

    @property
    def provide_data(self):
        return self._provide_data

    @property
    def provide_label(self):
        return self._provide_label

    def reset(self):
        self.batch_idx = 0

    def next(self):
        if self.batch_idx < self.num_batches:

            np_features , np_targets , np_weights = self.fetch_curr_batch_as_numpy()
            features = _mx.nd.array(np_features)

            if self.use_pad and (self.batch_idx == self.num_batches - 1):
                pad = self.batch_size - self.num_rows % self.batch_size
            else:
                pad = 0

            self.batch_idx += 1

            if not self.use_target:
                return _mx.io.DataBatch([features], label=None, pad=pad)
            else:
                targets = _mx.nd.array(np_targets)
                weights = _mx.nd.array(np_weights)
                return _mx.io.DataBatch(
                    [features], [targets, weights], pad=pad)
        else:
            raise StopIteration

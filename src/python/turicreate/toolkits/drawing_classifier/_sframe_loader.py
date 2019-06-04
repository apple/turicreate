# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import numpy as _np
import turicreate as _tc
from .._mxnet import mxnet as _mx
from six.moves.queue import Queue as _Queue
from threading import Thread as _Thread
from turicreate.toolkits._main import ToolkitError as _ToolkitError

_TMP_COL_RANDOM_ORDER = '_random_order'

# Encapsulates an SFrame, iterating over each row and returning an
# (example, label, index) tuple.
class _SFrameDataSource:
    def __init__(self, sframe, feature_column, target_column,
                 load_labels=True, shuffle=True, samples=None):
        self.target_column = target_column
        self.feature_column = feature_column
        self.load_labels = load_labels
        self.shuffle = shuffle
        self.num_samples = samples
        self.cur_sample = 0

        # Make shallow copy, so that temporary columns do not change input
        self.sframe = sframe.copy()

    def __iter__(self):
        return self

    def __next__(self):
        return self._next()

    def next(self):
        return self._next()

    def _next(self):
        if self.cur_sample == self.num_samples:
            raise StopIteration

        # If we're about to begin a new epoch, shuffle the SFrame if requested.
        row_index = self.cur_sample % len(self.sframe)
        if row_index == 0 and self.cur_sample > 0 and self.shuffle:
            self.sframe[_TMP_COL_RANDOM_ORDER] = _np.random.uniform(
                size = len(self.sframe))
            self.sframe = self.sframe.sort(_TMP_COL_RANDOM_ORDER)
        self.cur_sample += 1

        # Copy the image data for this row into a NumPy array.
        row = self.sframe[row_index]

        bitmap_width, bitmap_height = 28, 28
        drawing_feature = row[self.feature_column]
        is_stroke_input = (not isinstance(drawing_feature, _tc.Image))
        if is_stroke_input:
            pixels_01 = drawing_feature.pixel_data.reshape(
                1, bitmap_width, bitmap_height) / 255.
        else:
            image = _tc.image_analysis.resize(
                drawing_feature, bitmap_width, bitmap_height, 1)
            pixels_01 = image.pixel_data.reshape(
                1, bitmap_width, bitmap_height) / 255.
        
        label = row[self.target_column] if self.load_labels else None
        
        return pixels_01, label, row_index

    def reset(self):
        self.cur_sample = 0


class SFrameClassifierIter(_mx.io.DataIter):
    def __init__(self,
                 sframe,
                 batch_size,
                 class_to_index,
                 input_shape = [28,28],
                 feature_column="drawing",
                 target_column="label",
                 load_labels=True,
                 shuffle=True,
                 iterations=None,
                 training_steps=None):

        # Some checks (these errors are not well-reported by the threads)
        # @TODO: this check must be activated in some shape or form
        # if sframe[feature_column].dtype != _tc.Image:
        #     raise ValueError('Feature column must be of type Image')

        self.batch_size = batch_size
        self.dataset_size = len(sframe)
        self.input_shape = input_shape
        self.class_to_index = class_to_index
        self.cur_step = 0
        self.load_labels = load_labels
        self.num_iterations = iterations
        self.num_steps = training_steps

        # Compute the number of times we'll read a row from the SFrame.
        sample_limits = []
        if training_steps is not None:
            sample_limits.append(training_steps * batch_size)
        if iterations is not None:
            sample_limits.append(iterations * len(sframe))
        samples = min(sample_limits) if len(sample_limits) > 0 else None
        self.data_source = _SFrameDataSource(
            sframe, feature_column, target_column,
            load_labels = load_labels, shuffle = shuffle, samples = samples)

        self._provide_data = [
            _mx.io.DataDesc(name=feature_column,
                            shape=(batch_size, 1, 
                                self.input_shape[0], self.input_shape[1]),
                            layout='NCHW')
        ]

        self._provide_label = [
            _mx.io.DataDesc(name='label_map',
                            shape=(batch_size, 1),
                            layout='NCHW')
        ]

    def __iter__(self):
        return self

    def reset(self):
        self.data_source.reset()
        self.cur_step = 0

    def __next__(self):
        return self._next()

    def next(self):
        return self._next()

    @property
    def provide_data(self):
        return self._provide_data

    @property
    def provide_label(self):
        return self._provide_label

    def _next(self):
        images = []
        labels = []
        classes = []
        if self.cur_step == self.num_steps:
            raise StopIteration

        pad = None
        for b in range(self.batch_size):
            try:
                row = next(self.data_source)
            except StopIteration:
                if b == 0:
                    # Don't return an empty batch.
                    raise
                else:
                    # It's time to finish, so we need to pad the batch
                    pad = self.batch_size - b
                    for p in range(pad):
                        images.append(_mx.nd.zeros(images[0].shape))
                        if self.load_labels:
                            labels.append(0)
                    break

            raw_image, label, cur_sample = row
            # label will be None if self.load_labels is False
            
            image = _mx.nd.array(raw_image)
            images.append(image)
            if self.load_labels:
                labels.append(self.class_to_index[label])
            
        b_images = _mx.nd.stack(*images)
        if self.load_labels:
            b_labels = _mx.nd.array(labels, dtype='int32')
            batch = _mx.io.DataBatch([b_images], [b_labels], pad=pad)
        else:
            batch = _mx.io.DataBatch([b_images], pad=pad)
        batch.training_step = self.cur_step
        batch.iteration = int(batch.training_step * self.batch_size / self.dataset_size)
        self.cur_step += 1
        return batch

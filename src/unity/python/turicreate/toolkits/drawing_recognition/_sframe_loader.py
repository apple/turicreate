# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import numpy as _np
import turicreate as _tc
import mxnet as _mx
from six.moves.queue import Queue as _Queue
from threading import Thread as _Thread
from turicreate.toolkits._main import ToolkitError as _ToolkitError

_TMP_COL_RANDOM_ORDER = '_random_order'

# Encapsulates an SFrame, iterating over each row and returning an
# (image, label, index) tuple.
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
            self.sframe[_TMP_COL_RANDOM_ORDER] = _np.random.uniform(size=len(self.sframe))
            self.sframe = self.sframe.sort(_TMP_COL_RANDOM_ORDER)
        self.cur_sample += 1

        # Copy the image data for this row into a NumPy array.
        row = self.sframe[row_index]

        drawing_feature = row[self.feature_column]
        is_stroke_input = (type(drawing_feature) != _tc.Image)
        if is_stroke_input:
            pixels_01 = drawing_feature
        else:    
            image = _tc.image_analysis.resize(drawing_feature, 28, 28, 1)
            pixels_01 = image.pixel_data.reshape(1, 28, 28) / 255.
        
        # Copy the annotated bounding boxes for this image, if requested.
        if self.load_labels:
            label = row[self.target_column]
            if label == None:
                label = []
            elif type(label) == dict:
                label = [label]
        else:
            label = None

        return pixels_01, label, row_index

    def reset(self):
        self.cur_sample = 0


# A wrapper around _SFrameDataSource that uses a dedicated worker thread for
# performing SFrame operations.
class _SFrameAsyncDataSource:
    def __init__(self, sframe, feature_column, target_column,
                 load_labels=True, shuffle=True, samples=None, buffer_size=256):
        # This buffer_reset_queue will be used to communicate to the background
        # thread. Each "message" is itself a _Queue that the background thread
        # will use to communicate with us.
        buffer_reset_queue = _Queue()
        def worker():
            data_source = _SFrameDataSource(
                sframe, feature_column, target_column,
                load_labels=load_labels, shuffle=shuffle, samples=samples)
            while True:
                buffer = buffer_reset_queue.get()
                if buffer is None:
                    break  # No more work to do, exit this thread.

                for row in data_source:
                    buffer.put(row)

                    # Check if we've been reset (or told to exit).
                    if not buffer_reset_queue.empty():
                        break

                # Always end each output buffer with None to signal completion.
                buffer.put(None)
                data_source.reset()
        self.worker_thread = _Thread(target=worker)
        self.worker_thread.daemon = True
        self.worker_thread.start()

        self.buffer_reset_queue = buffer_reset_queue
        self.buffer_size = buffer_size

        # Create the initial buffer and send it to the background thread, so
        # that it begins sending us annotated images.
        self.buffer = _Queue(self.buffer_size)
        self.buffer_reset_queue.put(self.buffer)

    def __del__(self):
        # Tell the background thread to shut down.
        self.buffer_reset_queue.put(None)

        # Drain self.buffer to ensure that the background thread isn't stuck
        # waiting to put something into it (and therefore never receives the
        # shutdown request).
        if self.buffer is not None:
            while self.buffer.get() is not None:
                pass

    def __iter__(self):
        return self

    def __next__(self):
        return self._next()

    def next(self):
        return self._next()

    def _next(self):
        # Guard against repeated calls after we've finished.
        if self.buffer is None:
            raise StopIteration

        result = self.buffer.get()
        if result is None:
            # Any future attempt to get from self.buffer will block forever,
            # since the background thread won't put anything else into it.
            self.buffer = None
            raise StopIteration

        return result

    def reset(self):
        # Send a new buffer to the background thread, telling it to reset.
        buffer = _Queue(self.buffer_size)
        self.buffer_reset_queue.put(buffer)

        # Drain self.buffer to ensure that the background thread isn't stuck
        # waiting to put something into it (and therefore never receives the
        # new buffer).
        if self.buffer is not None:
            while self.buffer.get() is not None:
                pass

        self.buffer = buffer


class SFrameRecognitionIter(_mx.io.DataIter):
    def __init__(self,
                 sframe,
                 batch_size,
                 class_to_index,
                 input_shape = [28,28],
                 feature_column='bitmap',
                 target_column='label',
                 load_labels=True,
                 shuffle=True,
                 io_thread_buffer_size=0,
                 epochs=None,
                 iterations=None):

        # Some checks (these errors are not well-reported by the threads)
        # @TODO: this check must be activated in some shape or form
        # if sframe[feature_column].dtype != _tc.Image:
        #     raise ValueError('Feature column must be of type Image')

        self.batch_size = batch_size
        self.input_shape = input_shape
        self.class_to_index = class_to_index
        self.cur_iteration = 0
        self.num_epochs = epochs
        self.num_iterations = iterations

        # Compute the number of times we'll read a row from the SFrame.
        sample_limits = []
        if iterations is not None:
            sample_limits.append(iterations * batch_size)
        if epochs is not None:
            sample_limits.append(epochs * len(sframe))
        samples = min(sample_limits) if len(sample_limits) > 0 else None
        if io_thread_buffer_size > 0:
            # Delegate SFrame operations to a background thread, leaving this
            # thread to Python-based work of copying to MxNet and scheduling
            # augmentation work in the MXNet backend.
            self.data_source = _SFrameAsyncDataSource(
                sframe, feature_column, target_column,
                load_labels=load_labels, shuffle=shuffle, samples=samples,
                buffer_size=io_thread_buffer_size * batch_size)
        else:
            self.data_source = _SFrameDataSource(
                sframe, feature_column, target_column,
                load_labels=load_labels, shuffle=shuffle, samples=samples)

        self._provide_data = [
            _mx.io.DataDesc(name='bitmap',
                            shape=(batch_size, 1, 28, 28),
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
        self.cur_iteration = 0

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
        if self.cur_iteration == self.num_iterations:
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
                        labels.append(0)
                    break

            raw_image, label, cur_sample = row

            image = _mx.nd.array(raw_image)
            images.append(image)
            labels.append(self.class_to_index[label])
            
        b_images = _mx.nd.stack(*images)
        b_labels = _mx.nd.array(labels, dtype='int32')

        batch = _mx.io.DataBatch([b_images], [b_labels], pad=pad)
        batch.iteration = self.cur_iteration
        self.cur_iteration += 1
        return batch

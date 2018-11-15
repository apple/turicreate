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
from ._detection import yolo_boxes_to_yolo_map as _yolo_boxes_to_yolo_map

_TMP_COL_RANDOM_ORDER = '_random_order'


def _convert_image_to_raw(image):
    # Decode image and make sure it has 3 channels
    return _tc.image_analysis.resize(image, image.width, image.height, 3, decode=True)

def _is_rectangle_annotation(ann):
    return 'type' not in ann or ann['type'] == 'rectangle'


def _is_valid_annotation(ann):
    if not isinstance(ann, dict):
        return False
    if not _is_rectangle_annotation(ann):
        # Not necessarily valid, but we bypass stricter checks (we simply do
        # not care about non-rectangle types)
        return True
    return ('coordinates' in ann and
            isinstance(ann['coordinates'], dict) and
            set(ann['coordinates'].keys()) == {'x', 'y', 'width', 'height'} and
            ann['coordinates']['width'] > 0 and
            ann['coordinates']['height'] > 0 and
            'label' in ann)


def _is_valid_annotations_list(anns):
    return all([_is_valid_annotation(ann) for ann in anns])


# Encapsulates an SFrame, iterating over each row and returning an
# (image, label, index) tuple.
class _SFrameDataSource:
    def __init__(self, sframe, feature_column, annotations_column,
                 load_labels=True, shuffle=True, samples=None):
        self.annotations_column = annotations_column
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

        turi_image = row[self.feature_column]        
        if turi_image.channels!=3:
            turi_image = _convert_image_to_raw(turi_image)
        image = turi_image.pixel_data

        # Copy the annotated bounding boxes for this image, if requested.
        if self.load_labels:
            label = row[self.annotations_column]
            if label == None:
                label = []
            elif type(label) == dict:
                label = [label]
        else:
            label = None

        return image, label, row_index

    def reset(self):
        self.cur_sample = 0


# A wrapper around _SFrameDataSource that uses a dedicated worker thread for
# performing SFrame operations.
class _SFrameAsyncDataSource:
    def __init__(self, sframe, feature_column, annotations_column,
                 load_labels=True, shuffle=True, samples=None, buffer_size=256):
        # This buffer_reset_queue will be used to communicate to the background
        # thread. Each "message" is itself a _Queue that the background thread
        # will use to communicate with us.
        buffer_reset_queue = _Queue()
        def worker():
            data_source = _SFrameDataSource(
                sframe, feature_column, annotations_column,
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


class SFrameDetectionIter(_mx.io.DataIter):
    def __init__(self,
                 sframe,
                 batch_size,
                 input_shape,
                 output_shape,
                 anchors,
                 feature_column,
                 annotations_column,
                 class_to_index,
                 aug_params={},
                 loader_type='augmented',
                 load_labels=True,
                 shuffle=True,
                 io_thread_buffer_size=0,
                 epochs=None,
                 iterations=None):

        # Some checks (these errors are not well-reported by the threads)
        if sframe[feature_column].dtype != _tc.Image:
            raise ValueError('Feature column must be of type Image')

        num_classes = len(class_to_index)
        num_anchors = len(anchors)

        # Load images with random pertubations
        if loader_type == 'augmented':
            augs = _mx.image.CreateDetAugmenter(data_shape=(3,) + tuple(input_shape),
                    resize=aug_params['aug_resize'],
                    rand_crop=aug_params['aug_rand_crop'],
                    rand_pad=aug_params['aug_rand_pad'],
                    rand_mirror=aug_params['aug_horizontal_flip'],
                    rand_gray=aug_params['aug_rand_gray'],
                    mean=_np.zeros(3), std=_np.ones(3),
                    brightness=aug_params['aug_brightness'],
                    contrast=aug_params['aug_contrast'],
                    saturation=aug_params['aug_saturation'],
                    hue=aug_params['aug_hue'],
                    pca_noise=aug_params['aug_pca_noise'],
                    inter_method=aug_params['aug_inter_method'],
                    min_object_covered=aug_params['aug_min_object_covered'],
                    aspect_ratio_range=(1/aug_params['aug_aspect_ratio'], aug_params['aug_aspect_ratio']),
                    pad_val=(128, 128, 128),
                    min_eject_coverage=aug_params['aug_min_eject_coverage'],
                    area_range=aug_params['aug_area_range'])

        elif loader_type == 'stretched':
            augs = _mx.image.CreateDetAugmenter(data_shape=(3,) + tuple(input_shape),
                    rand_crop=0.0, rand_pad=0.0, rand_mirror=False,
                    mean=_np.zeros(3), std=_np.ones(3), brightness=0.0,
                    contrast=0.0, saturation=0.0, hue=0, pca_noise=0.0,
                    inter_method=1)
        else:
            raise ValueError('Unknown loader_type: {}'.format(loader_type))

        self.augmentations = augs
        self.batch_size = batch_size
        self.input_shape = input_shape
        self.output_shape = output_shape
        self.num_classes = num_classes
        self.anchors = anchors
        self.class_to_index = class_to_index
        self.cur_iteration = 0
        self.num_epochs = epochs
        self.num_iterations = iterations

        if load_labels:
            is_annotations_list = sframe[annotations_column].dtype == list
            # Check that all annotations are valid
            if is_annotations_list:
                valids = sframe[annotations_column].apply(_is_valid_annotations_list)
            else:
                valids = sframe[annotations_column].apply(_is_valid_annotation)
                # Deal with Nones, which are valid (pure negatives)
                valids = valids.fillna(1)

            if valids.nnz() != len(sframe):
                # Fetch invalid row ids
                invalid_ids = _tc.SFrame({'valid': valids}).add_row_number()[valids == 0]['id']
                count = len(invalid_ids)
                num_examples = 5

                s = ""
                for row_id in invalid_ids[:num_examples]:
                    # Find which annotations were invalid in the list
                    s += "\n\nRow ID {}:".format(row_id)
                    anns = sframe[row_id][annotations_column]
                    if not isinstance(anns, list):
                        anns = [anns]
                    for ann in anns:
                        if not _is_valid_annotation(ann):
                            s += "\n" + str(ann)

                if count > num_examples:
                    s += "\n\n... ({} row(s) omitted)".format(count - num_examples)

                # There were invalid rows
                raise _ToolkitError("Invalid object annotations discovered.\n\n"
                    "A valid annotation is a dictionary that defines 'label' and 'coordinates',\n"
                    "the latter being a dictionary that defines 'x', 'y', 'width', and 'height'.\n"
                    "The following row(s) did not conform to this format:" + s)

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
                sframe, feature_column, annotations_column,
                load_labels=load_labels, shuffle=shuffle, samples=samples,
                buffer_size=io_thread_buffer_size * batch_size)
        else:
            self.data_source = _SFrameDataSource(
                sframe, feature_column, annotations_column,
                load_labels=load_labels, shuffle=shuffle, samples=samples)

        self._provide_data = [
            _mx.io.DataDesc(name='image',
                            shape=(batch_size, 3) + tuple(input_shape),
                            layout='NCHW')
        ]

        output_size = (num_classes + 5) * num_anchors
        self._provide_label = [
            _mx.io.DataDesc(name='label_map',
                            shape=(batch_size, output_size) + tuple(output_shape),
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
        ymaps = []
        indices = []
        orig_shapes = []
        bboxes = []
        classes = []
        if self.cur_iteration == self.num_iterations:
            raise StopIteration

        # Since we pre-screened the annotations, at this point we just want to
        # check that it's the right type (rectangle) and the class is included
        def is_valid(ann):
            return _is_rectangle_annotation(ann) and ann['label'] in self.class_to_index

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
                        ymaps.append(_mx.nd.zeros(ymaps[0].shape))
                        indices.append(0)
                        orig_shapes.append([0, 0, 0])
                    break

            raw_image, label, cur_sample = row

            orig_image = _mx.nd.array(raw_image)
            image = orig_image
            oshape = orig_image.shape

            if label is not None:
                # Unchanged boxes, for evaluation
                raw_bbox = _np.array([[
                        ann['coordinates']['x'],
                        ann['coordinates']['y'],
                        ann['coordinates']['width'],
                        ann['coordinates']['height'],
                    ] for ann in label if is_valid(ann)]).reshape(-1, 4)

                # MXNet-formatted boxes for input to data augmentation
                bbox = _np.array([[
                        self.class_to_index[ann['label']],
                        (ann['coordinates']['x'] - ann['coordinates']['width'] / 2) / orig_image.shape[1],
                        (ann['coordinates']['y'] - ann['coordinates']['height'] / 2) / orig_image.shape[0],
                        (ann['coordinates']['x'] + ann['coordinates']['width'] / 2) / orig_image.shape[1],
                        (ann['coordinates']['y'] + ann['coordinates']['height'] / 2) / orig_image.shape[0],
                    ] for ann in label if is_valid(ann)]).reshape(-1, 5)
            else:
                raw_bbox = _np.zeros((0, 4))
                bbox = _np.zeros((0, 5))

            for aug in self.augmentations:
                try:
                    image, bbox = aug(image, bbox)
                except ValueError:
                    # It is extremely rare, but mxnet can fail for some reason.
                    # If this happens, remove all boxes.
                    bbox = _np.zeros((0, 5))

            image01 = image / 255.0

            np_ymap = _yolo_boxes_to_yolo_map(bbox,
                                              input_shape=self.input_shape,
                                              output_shape=self.output_shape,
                                              num_classes=self.num_classes,
                                              anchors=self.anchors)

            ymap = _mx.nd.array(np_ymap)

            images.append(_mx.nd.transpose(image01, [2, 0, 1]))
            ymaps.append(ymap)
            indices.append(cur_sample)
            orig_shapes.append(oshape)
            bboxes.append(raw_bbox)
            classes.append(bbox[:, 0].astype(_np.int32))

        b_images = _mx.nd.stack(*images)
        b_ymaps = _mx.nd.stack(*ymaps)
        b_indices = _mx.nd.array(indices)
        b_orig_shapes = _mx.nd.array(orig_shapes)

        batch = _mx.io.DataBatch([b_images], [b_ymaps, b_indices, b_orig_shapes], pad=pad)
        # Attach additional information
        batch.raw_bboxes = bboxes
        batch.raw_classes = classes
        batch.iteration = self.cur_iteration
        self.cur_iteration += 1
        return batch

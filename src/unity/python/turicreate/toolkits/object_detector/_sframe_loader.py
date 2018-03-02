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
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from ._detection import yolo_boxes_to_yolo_map as _yolo_boxes_to_yolo_map

_TMP_COL_RAW_IMAGE = '_raw_image_data'
_TMP_COL_RANDOM_ORDER = '_random_order'


def _convert_image_to_raw(image):
    # Decode image and make sure it has 3 channels
    return _tc.image_analysis.resize(image, image.width, image.height, 3, decode=True)


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
                 epochs=None):

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
        self.cur_batch = 0
        self.batch_size = batch_size
        self.input_shape = input_shape
        self.output_shape = output_shape
        self.num_classes = num_classes
        self.feature_column = feature_column
        self.annotations_column = annotations_column
        self.anchors = anchors
        self.load_labels = load_labels
        self.class_to_index = class_to_index
        self.shuffle = shuffle
        self.cur_epoch = 0
        self.cur_sample = 0
        self.num_epochs = epochs

        # Make shallow copy, so that temporary columns do not change input
        self.sframe = sframe.copy()

        # Convert images to raw to eliminate overhead of decoding
        self.sframe[_TMP_COL_RAW_IMAGE] = self.sframe[self.feature_column].apply(_convert_image_to_raw)

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
        self.cur_sample = 0
        self.cur_batch = 0

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
        if self.cur_epoch == self.num_epochs or len(self.sframe) == 0:
            raise StopIteration

        pad = None
        for b in range(self.batch_size):
            row = self.sframe[self.cur_sample]
            orig_image = _mx.nd.array(row[_TMP_COL_RAW_IMAGE].pixel_data)

            image = orig_image
            oshape = orig_image.shape
            if self.load_labels:
                label = row[self.annotations_column]
                if label == None:
                    label = []
                elif type(label) == dict:
                    label = [label]

                def is_valid(ann):
                    is_rect = ('type' not in ann or ann['type'] == 'rectangle')
                    if not is_rect:
                        # Not valid, but we bypass stricter checks (we simply
                        # do not care about non rectangle types)
                        return False
                    ok_required = ('coordinates' in ann and
                                   isinstance(ann['coordinates'], dict) and
                                   set(ann['coordinates'].keys()) == {'x', 'y', 'width', 'height'} and
                                   'label' in ann)
                    if not ok_required:
                        raise _ToolkitError("Detected an bounding box annotation with improper format: {}".format(ann))
                    ok_optional = ann['label'] in self.class_to_index
                    return ok_optional

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
            indices.append(self.cur_sample)
            orig_shapes.append(oshape)
            bboxes.append(raw_bbox)
            classes.append(bbox[:, 0].astype(_np.int32))

            self.cur_sample += 1
            if self.cur_sample == len(self.sframe):
                self.cur_epoch += 1
                if self.cur_epoch == self.num_epochs:
                    # It's time to finish, so we need to pad the batch
                    pad = self.batch_size - b - 1
                    for p in range(pad):
                        images.append(_mx.nd.zeros(images[0].shape))
                        ymaps.append(_mx.nd.zeros(ymaps[0].shape))
                        indices.append(0)
                        orig_shapes.append([0, 0, 0])
                    break
                self.cur_sample = 0
                if self.shuffle:
                    self.sframe[_TMP_COL_RANDOM_ORDER] = _np.random.uniform(size=len(self.sframe))
                    self.sframe = self.sframe.sort(_TMP_COL_RANDOM_ORDER)

        b_images = _mx.nd.stack(*images)
        b_ymaps = _mx.nd.stack(*ymaps)
        b_indices = _mx.nd.array(indices)
        b_orig_shapes = _mx.nd.array(orig_shapes)

        batch = _mx.io.DataBatch([b_images], [b_ymaps, b_indices, b_orig_shapes], pad=pad)
        # Attach additional information
        batch.raw_bboxes = bboxes
        batch.raw_classes = classes
        return batch

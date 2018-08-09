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

_TMP_COL_PREP_IMAGE = '_prepared_image'
_TMP_COL_RANDOM_ORDER = '_random_order'

def _resize_if_too_large(image, max_shape):
    width_f = image.width / max_shape[1]
    height_f = image.height / max_shape[0]
    f = max(width_f, height_f)
    if f > 1.0:
        width, height = int(image.width / f), int(image.height / f)
    else:
        width, height = image.width, image.height

    # make sure we exactly abide by the max_shape, so that a rounding error did
    # not occur
    width = min(width, max_shape[1])
    height = min(height, max_shape[0])

    # Decode image and make sure it has 3 channels
    return _tc.image_analysis.resize(image, width, height, 3, decode=True,
                                     resample='bilinear')


def _stretch_resize(image, shape):
    height, width = shape
    return _tc.image_analysis.resize(image, width, height, 3, decode=True,
                                     resample='bilinear')


class SFrameSTIter(_mx.io.DataIter):

    def __init__(self, sframe, batch_size, shuffle, feature_column,
                 input_shape, num_epochs=None, repeat_each_image=1,
                 loader_type='stretch', aug_params={}, sequential=True):

        if sframe[feature_column].dtype != _tc.Image:
            raise _ToolkitError('Feature column must be of type Image')

        if loader_type in {'stretch', 'stretch-with-augmentation'}:
            img_prep_fn = lambda img: _stretch_resize(img, input_shape)
        elif loader_type in {'pad', 'pad-with-augmentation', 'favor-native-size'}:
            img_prep_fn = lambda img: _resize_if_too_large(img, input_shape)
        else:
            raise ValueError('Unknown loader-type')

        if loader_type.endswith('-with-augmentation'):
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
                    pad_val=(128, 128, 128),
                    area_range=aug_params['aug_area_range'])
        else:
            augs = []

        self.augmentations = augs
        self.cur_batch = 0
        self.batch_size = batch_size
        self.input_shape = input_shape
        self.shuffle = shuffle
        self.feature_column = feature_column
        self.cur_epoch = 0
        self.cur_sample = 0
        self.cur_repeat = 0
        self.num_epochs = num_epochs
        self.repeat_each_image = repeat_each_image
        self.loader_type = loader_type

        # Make shallow copy, so that temporary columns do not change input
        self.sframe = sframe.copy()

        # Convert images to raw to eliminate overhead of decoding
        if sequential:
            builder = _tc.SArrayBuilder(_tc.Image)
            for img in self.sframe[self.feature_column]:
                builder.append(img_prep_fn(img))
            self.sframe[_TMP_COL_PREP_IMAGE] = builder.close()
        else:
            self.sframe[_TMP_COL_PREP_IMAGE] = self.sframe[self.feature_column].apply(img_prep_fn)

        self._provide_data = [
            _mx.io.DataDesc(name='image',
                            shape=(batch_size, 3) + tuple(input_shape),
                            layout='NCHW')
        ]

    def __iter__(self):
        return self

    def reset(self):
        self.cur_sample = 0
        self.cur_batch = 0
        self.cur_repeat = 0

    def __next__(self):
        return self._next()

    def next(self):
        return self._next()

    @property
    def provide_data(self):
        return self._provide_data

    def _next(self):

        if self.cur_epoch == self.num_epochs or len(self.sframe) == 0:
            raise StopIteration

        if self.loader_type == 'favor-native-size':
            b_images = []
        else:
            b_images = _mx.nd.ones((self.batch_size, 3,) + self.input_shape) / 2

        pad = None
        indices = _np.zeros(self.batch_size, dtype=_np.int64)
        repeat_indices = _np.zeros(self.batch_size, dtype=_np.int64)
        crop = _np.zeros((self.batch_size, 4), dtype=_np.int64)
        for b in range(self.batch_size):
            row = self.sframe[self.cur_sample]
            orig_image = _mx.nd.array(row[_TMP_COL_PREP_IMAGE].pixel_data)

            indices[b] = self.cur_sample
            repeat_indices[b] = self.cur_repeat

            image = orig_image

            for aug in self.augmentations:
                image, _ = aug(image, _np.zeros((0, 5)))
            image /= 255.0
            image_transposed = _mx.nd.transpose(image, [2, 0, 1])
            h, w = image_transposed.shape[1:3]

            if self.loader_type == 'favor-native-size':
                b_images.append(image_transposed)
                crop[b] = [0, h, 0, w]
            else:
                offset0 = (self.input_shape[0] - image_transposed.shape[1]) // 2
                offset1 = (self.input_shape[1] - image_transposed.shape[2]) // 2
                crop[b] = [offset0, offset0+h, offset1, offset1+w]
                b_images[b, :, offset0:offset0+h, offset1:offset1+w] = image_transposed

            self.cur_repeat += 1
            if self.cur_repeat == self.repeat_each_image:
                self.cur_repeat = 0
                self.cur_sample += 1

            if self.cur_sample == len(self.sframe):
                self.cur_epoch += 1
                if self.cur_epoch == self.num_epochs:
                    # It's time to finish, so we need to indicate that the
                    # batch is padded
                    pad = self.batch_size - b - 1
                    break
                self.cur_sample = 0
                if self.shuffle:
                    self.sframe[_TMP_COL_RANDOM_ORDER] = _np.random.uniform(
                        size=len(self.sframe))
                    self.sframe = self.sframe.sort(_TMP_COL_RANDOM_ORDER)

        batch = _mx.io.DataBatch([b_images], pad=pad)
        batch.indices = indices
        batch.repeat_indices = repeat_indices
        batch.crop = crop
        return batch

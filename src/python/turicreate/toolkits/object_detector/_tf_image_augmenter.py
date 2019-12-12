# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import numpy as np
import tensorflow.compat.v1 as tf
from tensorflow.python.ops import array_ops
from tensorflow.python.framework import ops
from tensorflow.python.ops import control_flow_ops
from tensorflow.python.ops import gen_image_ops
from tensorflow.python.ops import check_ops
from tensorflow.python.ops import math_ops
from tensorflow.python.ops import random_ops
from tensorflow.python.ops import logging_ops
from tensorflow.python.ops import variables
import turicreate.toolkits._tf_utils as _utils

tf.disable_v2_behavior()

_DEFAULT_AUG_PARAMS = {
  'max_hue_adjust' : 0.05,
  'max_brightness' : 0.05,
  'max_contrast' : 1.25,
  'max_saturation' : 1.25,
  'flip_pr' : True,
  'skip_probability' : 0.1,
  'min_aspect_ratio' : 0.8,
  'max_aspect_ratio' : 1.25,
  'min_area_fraction' : 1.0,
  'max_area_fraction' : 2.0,
  'max_attempts' : 50,
  'output_shape' : (416, 416),
}

def hue_augmenter(image, annotation,
                  max_hue_adjust=_DEFAULT_AUG_PARAMS["max_hue_adjust"]):

    # Sample a random rotation around the color wheel.
    hue_adjust = 0.0
    if (max_hue_adjust is not None) and (max_hue_adjust > 0.0):
        hue_adjust += np.pi * np.random.uniform(-max_hue_adjust, max_hue_adjust)

    # Apply the rotation to the hue
    image = tf.image.random_hue(image, max_delta=max_hue_adjust)
    image = tf.clip_by_value(image, 0, 1)

    # No geometry changes, so just copy the annotations.
    return image, annotation

def color_augmenter(image, annotation,
                    max_brightness=_DEFAULT_AUG_PARAMS["max_brightness"],
                    max_contrast=_DEFAULT_AUG_PARAMS["max_contrast"],
                    max_saturation=_DEFAULT_AUG_PARAMS["max_saturation"]):

    # Sample a random adjustment to brightness.
    if max_brightness is not None and max_brightness > 0:
        image = tf.image.random_brightness(image, max_delta=max_brightness)

    # Sample a random adjustment to contrast.
    if max_saturation is not None and max_saturation > 1.0:
        log_sat = np.log(max_saturation)
        image = tf.image.random_saturation(image, lower=np.exp(-log_sat), upper=np.exp(log_sat))

    # Sample a random adjustment to saturation.
    if max_contrast is not None and max_contrast > 1.0:
        log_con = np.log(max_contrast)
        image = tf.image.random_contrast(image, lower=np.exp(-log_con), upper=np.exp(log_con))

    image = tf.clip_by_value(image, 0, 1)

    # No geometry changes, so just copy the annotations.
    return image, annotation

def resize_augmenter(image, annotation,
                     output_shape = _DEFAULT_AUG_PARAMS["output_shape"]):

    new_height = tf.cast(output_shape[0], dtype=tf.int32)
    new_width = tf.cast(output_shape[1], dtype=tf.int32)

    # Determine the affine transform to apply and apply to the image itself.
    image_scaled = tf.squeeze(tf.image.resize_bilinear(
                          tf.expand_dims(image, 0), [new_height, new_width]), [0])
    image_clipped = tf.clip_by_value(image_scaled, 0, 1)

    # No geometry changes (because of relative co-ordinate system)
    return image_clipped, annotation

def complete_augmenter(img_tf, ann_tf):
    #img_tf, ann_tf = self.crop_augmenter(img_tf, ann_tf)
    #img_tf, ann_tf = self.padding_augmenter(img_tf, ann_tf)
    #img_tf, ann_tf = resize_augmenter(img_tf, ann_tf)
    #img_tf, ann_tf = self.horizontal_flip_augmenter(img_tf, ann_tf)
    #img_tf, ann_tf = color_augmenter(img_tf, ann_tf)
    img_tf, ann_tf = hue_augmenter(img_tf, ann_tf)
    return img_tf, ann_tf


class DataAugmenter(object):
    def __init__(self, params = {}):
        self.batch_size = 32
        self.graph = tf.Graph()
        with self.graph.as_default():
            self.img_tf = [tf.placeholder(tf.float32, [None, None, 3]) for x in range(0, self.batch_size )]
            self.ann_tf = [tf.placeholder(tf.float32, [None, 6]) for x in range(0, self.batch_size )]
            self.resize_op_batch = []
            for i in range(0, self.batch_size):
                aug_img_tf, aug_ann_tf = complete_augmenter(self.img_tf[i], self.ann_tf[i])
                self.resize_op_batch.append([[aug_img_tf, aug_ann_tf]])

    def get_augmented_data(self, images, annotations, output_height, output_width, resize_only):
        with tf.Session(graph=self.graph) as session:
            feed_dict = dict()
            for i in range(0, self.batch_size):
                feed_dict[self.img_tf[i]] = _utils.convert_shared_float_array_to_numpy(images[i])
                feed_dict[self.ann_tf[i]] = _utils.convert_shared_float_array_to_numpy(annotations[i])
            aug_output = session.run(self.resize_op_batch, feed_dict=feed_dict)
            processed_images = []
            processed_annotations = []
            for o in aug_output:
                processed_images.append(o[0])
                processed_annotations.append(o[1], dtype=float32)
            return tuple((processed_images, processed_annotations))

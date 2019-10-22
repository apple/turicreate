# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate.toolkits._tf_utils as _utils
import tensorflow.compat.v1 as tf
tf.disable_v2_behavior()
import numpy as np
from tensorflow.python.ops import array_ops
from tensorflow.python.framework import ops
from tensorflow.python.ops import control_flow_ops
from tensorflow.python.ops import gen_image_ops
from tensorflow.python.ops import check_ops
from tensorflow.python.ops import math_ops
from tensorflow.python.ops import random_ops
from tensorflow.python.ops import logging_ops
from tensorflow.python.ops import variables


def get_augmented_data(images, annotations, resize_only=False):
    input_shape = (416, 416)
    session = tf.Session()
    aug_images = []
    augmented_annotations = []
    # TODO : Get resize_only from C++ and call get_resized_image()
    for i in range(len(images)):
        image = images[i] 
        image = _utils.convert_shared_float_array_to_numpy(image)
        height, width = image.shape[0], image.shape[1]
        image, transformation  = augment_each_image(image)
        img, trans = session.run([image, transformation])
        aug_images.append(img)
        ann = annotations[i]
        annotation = _utils.convert_shared_float_array_to_numpy(ann)
        if annotations:
            identifier = np.expand_dims(annotation[:, 0], axis=1)
            box = np.zeros(annotation[:, 1:5].shape)
            for j in range(len(annotation)):
                box[j][0] = annotation[j][2]*float(height)
                box[j][1] = annotation[j][1]*float(width)
                box[j][2] = (annotation[j][4]+annotation[j][2])*float(height)
                box[j][3] = (annotation[j][3]+annotation[j][1])*float(width) 
            confidence = np.expand_dims(annotation[:, 5], axis=1)
            bbox_out = apply_bounding_box_transformation(box, trans, input_shape)
            bbox = np.zeros(bbox_out.shape)
            for k in range(len(bbox_out)):
              bbox[k][0] = bbox_out[k][1]/input_shape[1]
              bbox[k][1] = bbox_out[k][0]/input_shape[0]
              bbox[k][2] = (bbox_out[k][3] - bbox_out[k][1])/input_shape[1]
              bbox[k][3] = (bbox_out[k][2] - bbox_out[k][0])/input_shape[0]
            an = np.hstack((np.hstack((identifier, bbox)), confidence))
        augmented_annotations.append(an)
    augmented_images = np.array(aug_images)
    return tuple((augmented_images, augmented_annotations))

def is_tensor(x):
    # Checks if `x` is a symbolic tensor-like object.
    return isinstance(x, (ops.Tensor, variables.Variable))

def image_dimensions(images, static_only=True):
    # Returns the dimensions of an image tensor.
    if static_only or images.get_shape().is_fully_defined():
        return images.get_shape().as_list()
    else:
        return tf.unstack(tf.shape(images))

def get_image_dimensions(image, rank):
    # Returns the dimensions of an image tensor.
    if image.get_shape().is_fully_defined():
        return image.get_shape().as_list()
    else:
        static_shape = image.get_shape().with_rank(rank).as_list()
        dynamic_shape = array_ops.unstack(array_ops.shape(image), rank)
        return [s if s is not None else d for s, d in zip(static_shape, dynamic_shape)]

def check_three_dim_image(image, require_static=True):
    # Assert image is three dimensional
    try:
        image_shape = image.get_shape().with_rank(3)
    except ValueError:
        raise ValueError("'image' must be three-dimensional.")
    if require_static and not image_shape.is_fully_defined():
        raise ValueError("'image' must be fully defined.")
    if any(x == 0 for x in image_shape):
        raise ValueError("all dims of 'image.shape' must be > 0: %s" %
                                         image_shape)
    if not image_shape.is_fully_defined():
        return [check_ops.assert_positive(array_ops.shape(image),
                ["all dims of 'image.shape' must be > 0."])]
    else:
        return []

def check_atlease_three_dim_image(image, require_static=True):
    # Assert image is atleast three dimensional
    try:
        if image.get_shape().ndims is None:
            image_shape = image.get_shape().with_rank(3)
        else:
            image_shape = image.get_shape().with_rank_at_least(3)
    except ValueError:
        raise ValueError("'image' must be at least three-dimensional.")
    if require_static and not image_shape.is_fully_defined():
        raise ValueError('\'image\' must be fully defined.')
    if any(x == 0 for x in image_shape):
        raise ValueError('all dims of \'image.shape\' must be > 0: %s' %
                                       image_shape)
    if not image_shape.is_fully_defined():
        return [check_ops.assert_positive(array_ops.shape(image),["all dims of 'image.shape' "
                                                                           "must be > 0."])]
    else:
        return []

def pad_to_ensure_size(image, target_height, target_width, random=True):
    """
    Pads if below target size, but does nothing if above.

    If `width` or `height` is smaller than the specified `target_width` or
    `target_height` respectively, this op centrally pads with 0 along that
    dimension.
    """
    image = ops.convert_to_tensor(image, name='image')

    assert_ops = []
    assert_ops += check_three_dim_image(image, require_static=False)
    
    image = control_flow_ops.with_dependencies(assert_ops, image)
    # `crop_to_bounding_box` and `pad_to_bounding_box` have their own checks.
    # Make sure our checks come first, so that error messages are clearer.
    if is_tensor(target_height):
        target_height = control_flow_ops.with_dependencies(assert_ops, target_height)
    if is_tensor(target_width):
        target_width = control_flow_ops.with_dependencies(assert_ops, target_width)

    def max_(x, y):
        if is_tensor(x) or is_tensor(y):
            return math_ops.maximum(x, y)
        else:
            return max(x, y)

    height, width, _ = image_dimensions(image, static_only=False)
    width_diff = target_width - width
    offset_crop_width = max_(-width_diff // 2, 0)
    if random:
        offset_pad_width = tf.random_uniform([], minval=0, maxval=max_(width_diff, 1), dtype=tf.int32)
    else:
        offset_pad_width = max_(width_diff // 2, 0)

    height_diff = target_height - height
    offset_crop_height = max_(-height_diff // 2, 0)
    if random:
        offset_pad_height = tf.random_uniform([], minval=0, maxval=max_(height_diff, 1), dtype=tf.int32)
    else:
        offset_pad_height = max_(height_diff // 2, 0)

    # Maybe pad if needed.
    resized = pad_to_bounding_box(image, offset_pad_height, offset_pad_width,
                                           max_(target_height, height), max_(target_width, width))

    if resized.get_shape().ndims is None:
        raise ValueError('resized contains no shape.')

    resized_height, resized_width, _ = \
        image_dimensions(resized, static_only=False)
    return resized, (offset_pad_height, offset_pad_width)

def pad_to_bounding_box(image, offset_height, offset_width, target_height,
                                      target_width):
    """
    Pad `image` with zeros to the specified `height` and `width`.
    Adds `offset_height` rows of zeros on top, `offset_width` columns of
    zeros on the left, and then pads the image on the bottom and right
    with zeros until it has dimensions `target_height`, `target_width`.
    This op does nothing if `offset_*` is zero and the image already has size
    `target_height` by `target_width`
    """

    image = ops.convert_to_tensor(image, name='image')

    is_batch = True
    image_shape = image.get_shape()
    if image_shape.ndims == 3:
        is_batch = False
        image = array_ops.expand_dims(image, 0)
    elif image_shape.ndims is None:
        is_batch = False
        image = array_ops.expand_dims(image, 0)
        image.set_shape([None] * 4)
    elif image_shape.ndims != 4:
        raise ValueError('\'image\' must have either 3 or 4 dimensions.')

    assert_ops = check_atlease_three_dim_image(image, require_static=False)

    batch, height, width, depth = get_image_dimensions(image, rank=4)

    after_padding_width = target_width - offset_width - width
    after_padding_height = target_height - offset_height - height

    assert_ops += _assert(offset_height >= 0, ValueError,
                                              'offset_height must be >= 0')
    assert_ops += _assert(offset_width >= 0, ValueError,
                                              'offset_width must be >= 0')
    assert_ops += _assert(after_padding_width >= 0, ValueError,
                                              'width must be <= target - offset')
    assert_ops += _assert(after_padding_height >= 0, ValueError,
                                              'height must be <= target - offset')
    image = control_flow_ops.with_dependencies(assert_ops, image)

    # Do not pad on the depth dimensions.
    paddings = array_ops.reshape(array_ops.stack([
                  0, 0,
                  offset_height, after_padding_height,
                  offset_width, after_padding_width,
                  0, 0]), [4, 2])
    padded = array_ops.pad(image, paddings, constant_values=0.5)

    padded_shape = [None if is_tensor(i) else i
                                  for i in [batch, target_height, target_width, depth]]
    padded.set_shape(padded_shape)

    if not is_batch:
        padded = array_ops.squeeze(padded, squeeze_dims=[0])

    return padded

def apply_bounding_box_transformation(bbox, transformation, clip_to_shape=None):

  # The bounding box is [n, 4] reshaped and ones added to multiply to tranformation matrix
  v = np.concatenate([bbox.reshape(-1, 2), np.ones((bbox.shape[0]*2, 1), dtype=np.float32)], axis=1)
  # Transform
  v = np.dot(v, np.transpose(transformation))
  # Reverse shape
  bbox_out = v[:, :2].reshape(-1, 4)
  # Make points correctly ordered (lower < upper)
  for i in range(len(bbox_out)):
      if bbox_out[i][0] > bbox_out[i][2]:
          bbox_out[i][0], bbox_out[i][2] = bbox_out[i][2], bbox_out[i][0]
      if bbox_out[i][1] > bbox_out[i][3]:
          bbox_out[i][1], bbox_out[i][3] = bbox_out[i][3], bbox_out[i][1]

      if clip_to_shape is not None:
          bbox_out[:, 0::2] = np.clip(bbox_out[:, 0::2], 0, clip_to_shape[0])
          bbox_out[:, 1::2] = np.clip(bbox_out[:, 1::2], 0, clip_to_shape[1])

  return bbox_out

def _assert(cond, ex_type, msg):
    # A polymorphic assert, works with tensors and boolean expressions.
    # If `cond` is not a tensor, behave like an ordinary assert statement, except
    # that a empty list is returned. If `cond` is a tensor, return a list
    # containing a single TensorFlow assert op.

    if is_tensor(cond):
       return [control_flow_ops.Assert(cond, [msg])]
    else:
        if not cond:
            raise ex_type(msg)
        else:
            return []

def augment_each_image(image):
    # Applies augmentation on the given image based on the options

    # Augmentation option
    grid_shape = [13, 13]
    min_scale=1/1.5 
    max_scale=1.5 
    max_aspect_ratio=1.5 
    max_hue=0.05 
    max_brightness=0.05 
    max_saturation=1.25 
    max_contrast=1.25 
    horizontal_flip=True 
    input_shape = (416, 416)

    height, width, _ = tf.unstack(tf.shape(image))
    scale_h = tf.random_uniform([], minval=min_scale, maxval=max_scale)
    scale_w = scale_h * tf.exp(tf.random_uniform([], minval=-np.log(max_aspect_ratio), maxval=np.log(max_aspect_ratio)))
    new_height = tf.to_int32(tf.to_float(height) * scale_h)
    new_width = tf.to_int32(tf.to_float(width) * scale_w)

    image_scaled = tf.squeeze(tf.image.resize_bilinear(tf.expand_dims(image, 0), [new_height, new_width]), [0])
    # Image padding
    pad_image, pad_offset = pad_to_ensure_size(image_scaled, input_shape[0], input_shape[1])

    new_height = tf.maximum(input_shape[0], new_height)
    new_width = tf.maximum(input_shape[1], new_width)

    slice_offset = (tf.random_uniform([], minval=0, maxval=new_height - input_shape[0] + 1, dtype=tf.int32),
                    tf.random_uniform([], minval=0, maxval=new_width - input_shape[1] + 1, dtype=tf.int32))
    augmented_image = array_ops.slice(pad_image, [slice_offset[0], slice_offset[1], 0], [input_shape[0], input_shape[1], 3])

    if horizontal_flip:
        uniform_random = random_ops.random_uniform([], 0, 1.0)
        did_horiz_flip = math_ops.less(uniform_random, .5)
        augmented_image = control_flow_ops.cond(did_horiz_flip,
                                         lambda: array_ops.reverse(augmented_image, [1]),
                                         lambda: augmented_image)
        flip_sign = 1 - tf.to_float(did_horiz_flip) * 2
    else:
        flip_sign = 1
        did_horiz_flip = tf.constant(False)

    ty = tf.to_float(pad_offset[0] - slice_offset[0])
    tx = flip_sign * tf.to_float(pad_offset[1] - slice_offset[1]) + tf.to_float(did_horiz_flip) * input_shape[1]
    transformation = tf.reshape(tf.stack([
        scale_h, 0.0,                  ty,
        0.0,     flip_sign * scale_w,  tx,
        0.0,     0.0,                 1.0]
        ), (3, 3))

    if max_hue is not None and max_hue > 0:
        image = tf.image.random_hue(augmented_image, max_delta=max_hue)

    if max_brightness is not None and max_brightness > 0:
        image = tf.image.random_brightness(augmented_image, max_delta=max_brightness)

    if max_saturation is not None and max_saturation > 1.0:
        log_sat = np.log(max_saturation)
        image = tf.image.random_saturation(augmented_image, lower=np.exp(-log_sat), upper=np.exp(log_sat))

    if max_contrast is not None and max_contrast > 1.0:
        log_con = np.log(max_contrast)
        image = tf.image.random_contrast(augmented_image, lower=np.exp(-log_con), upper=np.exp(log_con))

    augmented_image = tf.clip_by_value(augmented_image, 0, 1)
    return augmented_image, transformation

def get_resized_image(image):

    # Resizes the input image to fixes size
    input_shape = (416, 416)
    height, width, _ = tf.unstack(tf.shape(image))
    orig_shape = (height, width)
    scale_h = tf.constant(input_shape[0], dtype=tf.float32) / tf.to_float(height)
    scale_w = tf.constant(input_shape[1], dtype=tf.float32) / tf.to_float(width)
    new_height = tf.to_int32(tf.to_float(height) * scale_h)
    new_width = tf.to_int32(tf.to_float(width) * scale_w)

    image_scaled = tf.squeeze(tf.image.resize_bilinear(tf.expand_dims(image, 0), [new_height, new_width]), [0])

    pad_image, pad_offset = pad_to_ensure_size(image_scaled, input_shape[0], input_shape[1],
              random=False)

    new_height = tf.maximum(input_shape[0], new_height)
    new_width = tf.maximum(input_shape[1], new_width)

    slice_offset = (tf.random_uniform([], minval=0, maxval=new_height - input_shape[0] + 1, dtype=tf.int32),
                      tf.random_uniform([], minval=0, maxval=new_width - input_shape[1] + 1, dtype=tf.int32))
    image = array_ops.slice(pad_image, [slice_offset[0], slice_offset[1], 0], [input_shape[0], input_shape[1], 3])
    image = tf.clip_by_value(image, 0, 1)
    return image

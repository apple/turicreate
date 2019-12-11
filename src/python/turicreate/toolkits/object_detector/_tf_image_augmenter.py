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

def convert_to_numpy(images, annotations):
    for image in images:
        image = _utils.convert_shared_float_array_to_numpy(image)
        images.append(image)
    for annotation in annotations:
        annotation = _utils.convert_shared_float_array_to_numpy(annotation)
        annotations.append(annotation)
    return images, annotations

def get_augmented_data(images, annotations, output_height, output_width, resize_only):

    options = {
    'max_hue' : 0.05,
    'max_brightness' : 0.05,
    'max_contrast' : 1.25,
    'max_saturation' : 1.25,
    'flip' : True,
    'skip_probability' : 0.1,
    'min_aspect_ratio' : 0.8, 
    'max_aspect_ratio' : 1.25, 
    'min_area_fraction' : 1.0, 
    'max_area_fraction' : 2.0, 
    'max_attempts' : 50.0
    }
    
    graph = tf.Graph()
    with graph.as_default():
        with tf.Session() as session:
            output_shape = (output_height, output_width)
            images, annotations = convert_to_numpy(images, annotations)
            if resize_only:
                resize_op = resize_augmenter(images, None, output_shape)
                session.run(tf.global_variables_initializer)
                resized_out = session.run(resize_op)
            else:
                images = tf.placeholder(tf.float32, [32, None, None, 3])
                annotations = tf.placeholder(tf.float32, [32, None, 6])
                images, annotations = crop_augmenter(images, annotations, options)
                images, annotations = padding_augmenter(images, annotations, options)
                images, annotations = horizontal_flip_augmenter(images, annotations, options)
                images, annotations = color_augmenter(images, annotations, options)
                images, annotations = hue_augmenter(images, annotations, options)
                resize_op = resize_augmenter(images, annotations, output_shape)
                session.run(tf.global_variables_initializer)
                
                resize_out = session.run(resize_op)  
            return tuple(resized_out)

def hue_augmenter(images, annotations, options):
    max_hue = options['max_hue']
    hue_images = []
    for image in images:
        if max_hue is not None and max_hue > 0:
            image = tf.image.random_hue(image, max_delta=max_hue)
        image = tf.clip_by_value(image, 0, 1)
        hue_images.append(image)
    return hue_images, annotations

def color_augmenter(images, annotations, options):
    max_brightness = options['max_brightness']
    max_contrast = options['max_contrast']
    max_saturation = options['max_saturation']
    colored_images = []
    for image in images:
        if max_brightness is not None and max_brightness > 0:
            image = tf.image.random_brightness(image, max_delta=max_brightness)

        if max_saturation is not None and max_saturation > 1.0:
            log_sat = np.log(max_saturation)
            image = tf.image.random_saturation(image, lower=np.exp(-log_sat), upper=np.exp(log_sat))

        if max_contrast is not None and max_contrast > 1.0:
            log_con = np.log(max_contrast)
            image = tf.image.random_contrast(image, lower=np.exp(-log_con), upper=np.exp(log_con))
        
        image = tf.clip_by_value(image, 0, 1)
        colored_images.append(image)
    return colored_images, annotations

def _is_intersection(bounding_box1, bounding_box2):
    bounding_box1_unstacked = tf.unstack(bounding_box1)
    bounding_box2_unstacked = tf.unstack(bounding_box2)
    is_intersection_cond = math_ops.logical_and(math_ops.logical_and(math_ops.less(bounding_box1_unstacked[0], bounding_box2_unstacked[2]),
                                                                     math_ops.less(bounding_box1_unstacked[1], bounding_box2_unstacked[3])),
                                                math_ops.logical_and(math_ops.less(bounding_box2_unstacked[0], bounding_box1_unstacked[2]),
                                                                     math_ops.less(bounding_box2_unstacked[1], bounding_box1_unstacked[3])))
    
    return control_flow_ops.cond(is_intersection_cond,
                                lambda: True,
                                lambda: False)
def _get_middle_points(bounding_box1, bounding_box2):
    stacked_points = tf.transpose(tf.stack([bounding_box1, bounding_box2]))
    uts = tf.unstack(stacked_points)

    flip_values_0 = math_ops.greater(uts[0][1], uts[0][0])
    uts[0] = control_flow_ops.cond(flip_values_0,
                        lambda: array_ops.reverse(uts[0], [0]),
                        lambda: uts[0])
    
    flip_values_1 = math_ops.greater(uts[1][1], uts[1][0])
    uts[1] = control_flow_ops.cond(flip_values_1,
                        lambda: array_ops.reverse(uts[1], [0]),
                        lambda: uts[1])
    
    flip_values_2 = math_ops.less(uts[2][1], uts[2][0])
    uts[2] = control_flow_ops.cond(flip_values_2,
                        lambda: array_ops.reverse(uts[2], [0]),
                        lambda: uts[2])
    
    flip_values_3 = math_ops.less(uts[3][1], uts[3][0])
    uts[3] = control_flow_ops.cond(flip_values_3,
                        lambda: array_ops.reverse(uts[3], [0]),
                        lambda: uts[3])

    restack_points = tf.stack(uts)[:, :1]
    reshaped_points = tf.reshape(restack_points, [ -1, 4])
    
    return tf.squeeze(reshaped_points)

def _get_intersection_point(bounding_box1, bounding_box2):
    intersection_cond = _is_intersection(bounding_box1, bounding_box2)
    return control_flow_ops.cond(intersection_cond,
                                lambda: _get_middle_points(bounding_box1, bounding_box2),
                                lambda: np.array([0.0, 0.0, 0.0, 0.0], dtype=np.float32))
def _get_area(bounds):
    points = tf.unstack(bounds)
    return ((points[2] - points[0]) * (points[3] - points[1]))

def _crop_augmenter_loop_conditional(num_attempts, max_attempts, perform_cropped_annotations):
    return math_ops.logical_and(math_ops.less_equal(num_attempts, max_attempts), math_ops.logical_not(perform_cropped_annotations))

def _crop_augmenter_loop_perform_crop(image_box, keep_annotations, annotations, num_attempts, max_attempts, perform_cropped_annotations, aspect_ratio_var, min_aspect_ratio, max_aspect_ratio, min_height_var, max_height_var, image_height, image_width, min_area_fraction, max_area_fraction):
    cropped_height = random_ops.random_uniform([], min_height_var, max_height_var)
    cropped_width = cropped_height * aspect_ratio_var
    
    x_offset = random_ops.random_uniform([], 0.0, (image_width - cropped_width))
    y_offset = random_ops.random_uniform([], 0.0, (image_height - cropped_height))
        
    unstacked_image_box = tf.unstack(image_box)
    
    unstacked_image_box[0] = tf.to_float(tf.to_int32(y_offset))
    unstacked_image_box[1] = tf.to_float(tf.to_int32(x_offset))
    unstacked_image_box[2] = tf.to_float(tf.to_int32(cropped_height))
    unstacked_image_box[3] = tf.to_float(tf.to_int32(cropped_width))

    image_box = tf.stack(unstacked_image_box)
    
    intersection_image_box = tf.unstack(image_box)
    
    intersection_image_box[0] = tf.to_float(tf.to_int32(y_offset))
    intersection_image_box[1] = tf.to_float(tf.to_int32(x_offset))
    intersection_image_box[2] = tf.to_float(tf.to_int32(cropped_height + y_offset))
    intersection_image_box[3] = tf.to_float(tf.to_int32(cropped_width + x_offset))

    intersection_bounding_box = tf.stack(intersection_image_box)
    
    identifier, box, confidence = decompose_annotations(annotations, tf.to_float(image_height), tf.to_float(image_width))
    
    unstacked_ann = tf.unstack(box)
    new_annotations_arr = []
    
    ty = tf.to_float(tf.to_int32(y_offset))
    tx = tf.to_float(tf.to_int32(x_offset))

    # Make the transformation matrix
    transformation = tf.reshape(tf.stack([
        1.0,     0.0,     -ty,
        0.0,     1.0,     -tx,
        0.0,     0.0,    1.0]
    ), (3, 3))

    for ann in unstacked_ann:
        intersection_points = _get_intersection_point(intersection_bounding_box, ann)
        area_intersection = _get_area(intersection_points)
        area_annotation = _get_area(ann)
        area_fraction = area_intersection / area_annotation
        
        # TODO: Replace this condition soon
    
        mat_intersection_points = tf.expand_dims(intersection_points, 0)
        transformed_intersection_points = apply_transformation(mat_intersection_points, transformation)
        
        overlap_condition = math_ops.greater_equal(area_fraction, 0.5)
        new_annotations_arr.append(control_flow_ops.cond(overlap_condition,
                                                        lambda: tf.squeeze(transformed_intersection_points),
                                                        lambda: np.array([0.0, 0.0, 0.0, 0.0], dtype=np.float32)))
    
    
    transformed_box = tf.stack(new_annotations_arr, axis=0)
    
    annotations = recompose_annotations(identifier, transformed_box, confidence, tf.to_float(cropped_height), tf.to_float(cropped_width))
    
    perform_cropped_annotations = True
    
    return image_box, keep_annotations, annotations, num_attempts, max_attempts, perform_cropped_annotations, aspect_ratio_var, min_aspect_ratio, max_aspect_ratio, min_height_var, max_height_var, image_height, image_width, min_area_fraction, max_area_fraction

def _crop_augmenter_loop_no_op(image_box, keep_annotations, annotations, num_attempts, max_attempts, perform_cropped_annotations, aspect_ratio_var, min_aspect_ratio, max_aspect_ratio, min_height_var, max_height_var, image_height, image_width, min_area_fraction, max_area_fraction):
    return image_box, keep_annotations, annotations, num_attempts, max_attempts, perform_cropped_annotations, aspect_ratio_var, min_aspect_ratio, max_aspect_ratio, min_height_var, max_height_var, image_height, image_width, min_area_fraction, max_area_fraction

def _crop_augmenter_loop_body(image_box, keep_annotations, annotations, num_attempts, max_attempts, perform_cropped_annotations, aspect_ratio_var, min_aspect_ratio, max_aspect_ratio, min_height_var, max_height_var, image_height, image_width, min_area_fraction, max_area_fraction):
    # increment the num attempts
    num_attempts = tf.add(num_attempts, 1.0)
    aspect_ratio_var = random_ops.random_uniform([], min_aspect_ratio, max_aspect_ratio)
    max_height_var = image_height
    
    max_height_from_width = tf.to_float(image_width) / aspect_ratio_var
    height_condition = math_ops.greater(max_height_var, max_height_from_width)
    max_height_var = control_flow_ops.cond(height_condition,
                                    lambda: max_height_from_width,
                                    lambda: max_height_var)
    
    max_height_from_area = tf.math.sqrt(max_area_fraction * image_height * image_width / aspect_ratio_var)
    area_condition = math_ops.greater(max_height_var, max_height_from_area)
    max_height_var = control_flow_ops.cond(area_condition,
                                    lambda: max_height_from_area,
                                    lambda: max_height_var)
    
    min_height_var = tf.math.sqrt(min_area_fraction * image_height * image_width / aspect_ratio_var)
    crop_condition = math_ops.greater(min_height_var, max_height_var)
    return control_flow_ops.cond(crop_condition,
                                lambda: _crop_augmenter_loop_no_op(image_box, keep_annotations, annotations, num_attempts, max_attempts, perform_cropped_annotations, aspect_ratio_var, min_aspect_ratio, max_aspect_ratio, min_height_var, max_height_var, image_height, image_width, min_area_fraction, max_area_fraction),
                                lambda: _crop_augmenter_loop_perform_crop(image_box, keep_annotations, annotations, num_attempts, max_attempts, perform_cropped_annotations, aspect_ratio_var, min_aspect_ratio, max_aspect_ratio, min_height_var, max_height_var, image_height, image_width, min_area_fraction, max_area_fraction))

def _crop_augmenter_worker(image, annotation, num_attempts_var, max_attempts, perform_cropped_annotations, image_height, image_width, min_height_var, max_height_var, aspect_ratio_var, min_aspect_ratio, max_aspect_ratio, min_area_fraction, max_area_fraction, image_box, keep_annotations):
    image_box, keep_annotations, annotation, num_attempts_var, max_attempts, perform_cropped_annotations, aspect_ratio_var, min_aspect_ratio, max_aspect_ratio, min_height_var, max_height_var, image_height, image_width, min_area_fraction, max_area_fraction = tf.while_loop(lambda image_box, keep_annotations, annotation, num_attempts_var, max_attempts, perform_cropped_annotations, aspect_ratio_var, min_aspect_ratio, max_aspect_ratio, min_height_var, max_height_var, image_height, image_width, min_area_fraction, max_area_fraction:_crop_augmenter_loop_conditional(num_attempts_var, max_attempts, perform_cropped_annotations),
                                                                                                                                                                                                                                                                                lambda image_box, keep_annotations, annotation, num_attempts_var, max_attempts, perform_cropped_annotations, aspect_ratio_var, min_aspect_ratio, max_aspect_ratio, min_height_var, max_height_var, image_height, image_width, min_area_fraction, max_area_fraction:_crop_augmenter_loop_body(image_box, keep_annotations, annotation, num_attempts_var, max_attempts, perform_cropped_annotations, aspect_ratio_var, min_aspect_ratio, max_aspect_ratio, min_height_var, max_height_var, image_height, image_width, min_area_fraction, max_area_fraction),
                                                                                                                                                                                                                                                                   [image_box, keep_annotations, annotation, num_attempts_var, max_attempts, perform_cropped_annotations, aspect_ratio_var, min_aspect_ratio, max_aspect_ratio, min_height_var, max_height_var, image_height, image_width, min_area_fraction, max_area_fraction])
    return image_box, annotation

def crop_augmenter(images, annotations, skip_probability=0.1, min_aspect_ratio=0.8, max_aspect_ratio=1.25, min_area_fraction=0.15, max_area_fraction=1.0, min_object_covered=0.0, max_attempts=50.0, min_eject_coverage=0.5):
    crop_images = []
    crop_annotations = []
    for image, annotation in zip(images, annotations):
        uniform_random = random_ops.random_uniform([], 0, 1.0)
        skip_image = math_ops.greater(uniform_random, 0.0)
        
        image_height, image_width, _ = tf.unstack(tf.shape(image))
        image_height = tf.to_float(image_height)
        image_width = tf.to_float(image_width)
        min_height_var = tf.Variable(image_height)
        max_height_var = tf.Variable(image_height)
        
        image_box = tf.Variable([0, 0, image_height, image_width])
        
        num_annotations = tf.shape(annotation)[0]
        keep_annotations = tf.ones([num_annotations])
        
        aspect_ratio_var  = tf.Variable(1.0)
        num_attempts_var = tf.Variable(0.0)
        
        perform_cropped_annotations = tf.Variable(False)
    
        output = control_flow_ops.cond(skip_image,
                                       lambda: _crop_augmenter_worker(image, annotation, num_attempts_var, max_attempts, perform_cropped_annotations, image_height, image_width, min_height_var, max_height_var, aspect_ratio_var, min_aspect_ratio, max_aspect_ratio, min_area_fraction, max_area_fraction, image_box, keep_annotations),
                                       lambda: (image_box, annotation))
        
        cropped_image = tf.image.crop_to_bounding_box(image, tf.to_int32(output[0][0]), tf.to_int32(output[0][1]), tf.to_int32(output[0][2]), tf.to_int32(output[0][3]))
        
        crop_images.append(cropped_image)
        crop_annotations.append(output[1])
    return crop_images, crop_annotations

def get_image_dimensions(image, rank):
    # Returns the dimensions of an image tensor.
    if image.get_shape().is_fully_defined():
        return image.get_shape().as_list()
    else:
        static_shape = image.get_shape().with_rank(rank).as_list()
        dynamic_shape = array_ops.unstack(array_ops.shape(image), rank)
        return [s if s is not None else d for s, d in zip(static_shape, dynamic_shape)]
    
def _padding_augmenter_loop_conditional(min_height, max_height, num_attempts, max_attempts, aspect_ratio, min_aspect_ratio, max_aspect_ratio, image_height, image_width, min_area_fraction, max_area_fraction):
    return tf.math.logical_and(math_ops.greater_equal(min_height, max_height), math_ops.less(num_attempts, max_attempts))

def _padding_augmenter_loop_body(min_height, max_height, num_attempts, max_attempts, aspect_ratio, min_aspect_ratio, max_aspect_ratio, image_height, image_width, min_area_fraction, max_area_fraction):
    num_attempts = tf.add(num_attempts, 1.0)
    aspect_ratio = random_ops.random_uniform([], tf.to_float(min_aspect_ratio), tf.to_float(max_aspect_ratio))
    min_height = image_height
    min_height_from_width = image_width / aspect_ratio;
    
    height_condition = math_ops.less(min_height, min_height_from_width)
    min_height = control_flow_ops.cond(height_condition,
                                lambda: min_height_from_width,
                                lambda: min_height)
    
    min_height_from_area = tf.math.sqrt(min_area_fraction * image_height * image_width / aspect_ratio)
    
    area_condition = math_ops.less(min_height, min_height_from_area)
    min_height = control_flow_ops.cond(area_condition,
                                lambda: min_height_from_area,
                                lambda: min_height)
    
    max_height = tf.math.sqrt(max_area_fraction * image_height * image_width / aspect_ratio)
    return min_height, max_height, num_attempts, max_attempts, aspect_ratio, min_aspect_ratio, max_aspect_ratio, image_height, image_width, min_area_fraction, max_area_fraction

def  _padding_augmentation_apply(image, annotation, min_height, max_height, aspect_ratio, image_height, image_width):
    padded_height = random_ops.random_uniform([], min_height, max_height)
    padded_width = padded_height * aspect_ratio
    
    x_offset = random_ops.random_uniform([], 0.0, (padded_width  - image_width))
    y_offset = random_ops.random_uniform([], 0.0, (padded_height  - image_height))
    
    image = array_ops.expand_dims(image, 0)
    batch, height, width, depth = get_image_dimensions(image, rank=4)
    
    after_padding_width = padded_width - width - x_offset
    after_padding_height = padded_height - height - y_offset
    
    padd_arr = array_ops.stack([
                  tf.to_float(tf.to_int32(y_offset)), tf.to_float(tf.to_int32(after_padding_height)),
                  tf.to_float(tf.to_int32(x_offset)), tf.to_float(tf.to_int32(after_padding_width)),
                  tf.to_float(tf.to_int32(height)), tf.to_float(tf.to_int32(width))])

    paddings = array_ops.reshape(array_ops.stack([
                  0, 0,
                  tf.to_int32(y_offset), tf.to_int32(after_padding_height),
                  tf.to_int32(x_offset), tf.to_int32(after_padding_width),
                  0, 0]), [4, 2])
    
    padded = array_ops.pad(image, paddings, constant_values=0.5)
    padded = array_ops.squeeze(padded, squeeze_dims=[0])
    
    ty = tf.to_float(y_offset) 
    tx = tf.to_float(x_offset)

    # Make the transformation matrix
    transformation = tf.reshape(tf.stack([
        1.0,     0.0,     ty,
        0.0,     1.0,     tx,
        0.0,     0.0,    1.0]
        ), (3, 3))
    
    identifier, box, confidence = decompose_annotations(annotation, tf.to_float(height), tf.to_float(width))
    transformed_box = apply_transformation(box, transformation)
    recomp_annotation = recompose_annotations(identifier, transformed_box, confidence, tf.to_float(padded_height), tf.to_float(padded_width))
        
    return (padded, recomp_annotation)
    
def _padding_augmenter_worker(image, annotation, min_aspect_ratio, max_aspect_ratio, min_area_fraction, max_area_fraction, max_attempts, aspect_ratio, image_height, image_width, min_height, max_height, num_attempts):
    min_height, max_height, num_attempts, max_attempts, aspect_ratio, min_aspect_ratio, max_aspect_ratio, image_height, image_width, min_area_fraction, max_area_fraction = tf.while_loop(lambda min_height, max_height, num_attempts, max_attempts, aspect_ratio, min_aspect_ratio, max_aspect_ratio, image_height, image_width, min_area_fraction, max_area_fraction: _padding_augmenter_loop_conditional(min_height, max_height, num_attempts, max_attempts, aspect_ratio, min_aspect_ratio, max_aspect_ratio, image_height, image_width, min_area_fraction, max_area_fraction),
                                                                                                                                                                                          lambda min_height, max_height, num_attempts, max_attempts, aspect_ratio, min_aspect_ratio, max_aspect_ratio, image_height, image_width, min_area_fraction, max_area_fraction: _padding_augmenter_loop_body(min_height, max_height, num_attempts, max_attempts, aspect_ratio, min_aspect_ratio, max_aspect_ratio, image_height, image_width, min_area_fraction, max_area_fraction),
                                                                                                                                                                                          [min_height, max_height, num_attempts, max_attempts, aspect_ratio, min_aspect_ratio, max_aspect_ratio, image_height, image_width, min_area_fraction, max_area_fraction])
    height_condition = math_ops.greater(min_height, max_height)
    return control_flow_ops.cond(height_condition,
                                lambda: (image, annotation),
                                lambda: _padding_augmentation_apply(image, annotation, min_height, max_height, aspect_ratio, image_height, image_width))

def padding_augmenter(images, annotations, options):
    
    skip_probability = options['skip_probability']
    min_aspect_ratio = options['min_aspect_ratio']
    max_aspect_ratio = options['max_aspect_ratio']
    min_area_fraction = options['min_area_fraction']
    max_area_fraction = options['max_area_fraction']
    max_attempts = options['max_attempts']

    padded_images = []
    padded_annotations = []
    
    for image, annotation in zip(images, annotations):
        image = _utils.convert_shared_float_array_to_numpy(image)
        uniform_random = random_ops.random_uniform([], 0, 1.0)
        skip_image = math_ops.greater(uniform_random, skip_probability)
        
        image_height, image_width, _ = tf.unstack(tf.shape(image))
        image_height = tf.to_float(image_height)
        image_width = tf.to_float(image_width)
        min_height_var = tf.Variable(image_height)
        max_height_var = tf.Variable(image_height)
        aspect_ratio_var  = tf.Variable(1.0)
        num_attempts_var = tf.Variable(0.0)
        
        output = control_flow_ops.cond(skip_image,
                                       lambda: _padding_augmenter_worker(image, annotation, min_aspect_ratio, max_aspect_ratio, min_area_fraction, max_area_fraction, max_attempts, aspect_ratio_var, image_height, image_width, min_height_var, max_height_var, num_attempts_var),
                                       lambda: (image, annotation))
        padded_images.append(output[0])
        padded_annotations.append(output[1])
    return padded_images, padded_annotations

def horizontal_flip_augmenter(images, annotations, options):
    flipped_images = []
    flipped_annotation = []

    flip = options['flip']
    
    for image, annotation in zip(images, annotations):
        # Handle the resize of the images
        # image = _utils.convert_shared_float_array_to_numpy(image)
        height, width, _ = tf.unstack(tf.shape(image))
        
        if flip:
            uniform_random = random_ops.random_uniform([], 0, 1.0)
            did_horiz_flip = math_ops.less(uniform_random, 0.5)
            image = control_flow_ops.cond(did_horiz_flip,
                                lambda: array_ops.reverse(image, [1]),
                                lambda: image)
            flip_sign = 1 - tf.to_float(did_horiz_flip) * 2
        else:
            flip_sign = 1.0
            did_horiz_flip = tf.constant(False)

        flipped_images.append(image)
        
        tx = tf.to_float(did_horiz_flip) * tf.to_float(width)
        transformation = tf.reshape(tf.stack([
            1.0,     0.0,                 0.0,
            0.0,     flip_sign,            tx,
            0.0,     0.0,                 1.0]
        ), (3, 3))
        
        identifier, box, confidence = decompose_annotations(annotation, tf.to_float(height), tf.to_float(width))
        transformed_box = apply_transformation(box, transformation)
        recomp_annotation = recompose_annotations(identifier, transformed_box, confidence, tf.to_float(height), tf.to_float(width))
        
        flipped_annotation.append(recomp_annotation)
        
    return flipped_images, flipped_annotation

def resize_augmenter(images, annotations, output_shape):
    resized_images = []
    resized_annotation = []
    
    if annotations is None:
        for image in images:
            image = _utils.convert_shared_float_array_to_numpy(image)
            new_height = tf.to_int32(tf.constant(output_shape[0], dtype=tf.float32))
            new_width = tf.to_int32(tf.constant(output_shape[1], dtype=tf.float32))

            image_scaled = tf.squeeze(tf.image.resize_bilinear(tf.expand_dims(image, 0), [new_height, new_width]), [0])

            image_clipped = tf.clip_by_value(image_scaled, 0, 1)

            resized_images.append(image_clipped)
            resize_annotation.append(len(resized_images)*[np.zeros(6)])
    else:       
    
        for image, annotation in zip(images, annotations):
            # Handle the resize of the images
            image = _utils.convert_shared_float_array_to_numpy(image)

            new_height = tf.to_int32(tf.constant(output_shape[0], dtype=tf.float32))
            new_width = tf.to_int32(tf.constant(output_shape[1], dtype=tf.float32))

            image_scaled = tf.squeeze(tf.image.resize_bilinear(tf.expand_dims(image, 0), [new_height, new_width]), [0])

            image_clipped = tf.clip_by_value(image_scaled, 0, 1)

            # Transformation Matrix
            height, width, _ = tf.unstack(tf.shape(image))

            scale_h = tf.constant(output_shape[0], dtype=tf.float32) / tf.to_float(height)
            scale_w = tf.constant(output_shape[1], dtype=tf.float32) / tf.to_float(width)

            transformation = tf.reshape(tf.stack([
                scale_h, 0.0,       0,
                0.0,     scale_w,   0,
                0.0,     0.0,       1.0]
            ), (3, 3))

            # Handle the Resize of the annotations
            annotation = _utils.convert_shared_float_array_to_numpy(annotation)


            identifier, box, confidence = decompose_annotations(annotation, tf.to_float(height), tf.to_float(width))

            transformed_box = apply_transformation(box, transformation)

            recomp_annotation = recompose_annotations(identifier, transformed_box, confidence, tf.to_float(height), tf.to_float(width))

            resized_images.append(image_clipped)
            resized_annotation.append(annotation)
    
    return resized_images, resized_annotation

def apply_transformation(box, transformation):
    reshaped_box = tf.reshape(box, [ -1, 2])
    
    ones_padding = tf.ones(tf.shape(reshaped_box)[0], dtype=tf.dtypes.float32)
    ones_padding = tf.expand_dims(ones_padding, 1)
    
    concat_box = tf.concat([reshaped_box, ones_padding], 1)
    
    transformed = tf.matmul(concat_box, tf.transpose(transformation))
    transformed_sliced = transformed[:, :2]
    
    # reshaped_transformation = tf.reshape(transformed_sliced, [ -1, 4])
    transpose_transformed_slices = tf.transpose(transformed_sliced)
    reshaped_trans_slice = tf.reshape(transpose_transformed_slices, [ -1, 2])
    unstacked_trans_slice = tf.unstack(reshaped_trans_slice, reshaped_trans_slice.shape[0], axis=0)
    
    flipped_slices = []
    for uts in unstacked_trans_slice:
        flip_values = math_ops.less(uts[1], uts[0])
        flipped_slices.append(control_flow_ops.cond(flip_values,
                                    lambda: array_ops.reverse(uts, [0]),
                                    lambda: uts))
    
    stacked_flipped_slices = tf.stack(flipped_slices, axis=0)
    reshaped_stacked_slices = tf.reshape(stacked_flipped_slices, [ -1, 4])
    transpose_flipped_slices = tf.transpose(reshaped_stacked_slices)
    corrected_flipped_slices = tf.reshape(transpose_flipped_slices, [ -1, 4])
    
    return corrected_flipped_slices

def recompose_annotations(identifier, box, confidence, height, width):
    unstacked_box = tf.unstack(box, box.shape[1], axis=1)
    conf = tf.squeeze(confidence, axis=1)
    iden = tf.squeeze(identifier, axis=1)
    
    ele_1 = unstacked_box[1] / width
    ele_2 = unstacked_box[0] / height
    ele_3 = (unstacked_box[3] - unstacked_box[1]) / width
    ele_4 = (unstacked_box[2] - unstacked_box[0]) / height

    return tf.stack([iden, ele_1, ele_2, ele_3, ele_4, conf], axis=1)

def decompose_annotations(annotation, height, width):
    identifier = tf.expand_dims(annotation[:, 0], 1, name=None)
    box = annotation[:, 1:5]
    unstacked_box = tf.unstack(box, box.shape[1], axis=1)
    
    top = unstacked_box[1] * height
    left = unstacked_box[0] * width
    bottom = (unstacked_box[3] + unstacked_box[1])  * height
    right = (unstacked_box[2] + unstacked_box[0]) * width
    
    stacked_boxes = tf.stack([top, left, bottom, right], axis=1)
    
    confidence = tf.expand_dims(annotation[:, 5], axis=1)
    return identifier, stacked_boxes, confidence
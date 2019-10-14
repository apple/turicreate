# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
import turicreate.toolkits._tf_utils as _utils
import tensorflow as tf
import numpy as _np

class DataAugmenter(object):
	def __init__(self, images, annotations, predictions):
		self.images = images
		self.annotations = annotations
		self.predictions = predictions
		for image in self.images:
			image = _utils.convert_shared_float_array_to_numpy(image)
		for ann in self.annotations:
			annotation = _utils.convert_shared_float_array_to_numpy(ann)
		for prediction in self.predictions:
			pred = _utils.convert_shared_float_array_to_numpy(pred)


	def get_augmented_images(self):
		return self.augmented_images

	def get_augmented_annotations(self):
		return self.augmented_annotations

	def augmented_loader(self,
        input_shape=(416, 416),
        min_scale=1.0,
        max_scale=1.0,
        max_aspect_ratio=1.5,
        max_hue=None,
        max_saturation=None,
        max_contrast=None,
        max_brightness=None,
        horizontal_flip=False,
        path_preparation_fn=None,
        seed=None):
    """
    Performs different transformations on the input image passed based on the values provided

    :param paths:
    :param input_shape:
    :param min_scale:
    :param max_scale:
    :param max_aspect_ratio:
    :param max_hue:
    :param max_saturation:
    :param max_contrast:
    :param max_brightness:
    :param horizontal_flip:
    :param path_preparation_fn:
    :param seed:
    :return:
    """

	    
	    for image in self.images:
	    	image.set_shape([None, None, 3])

	    imgshape = tf.shape(image_decoded)
	    #print("Image shape before unstacking:", imgshape)
	    height, width, _ = tf.unstack(imgshape)
	    print("Image shape after unstacking:")
	    print("Height: ", height)
	    print("Width: ", width)
	    orig_shape = (height, width)
	    #Performs a scaling operation
	    scale_h = tf.random_uniform([], minval=min_scale, maxval=max_scale, seed=seed)
	    scale_w = scale_h * tf.exp(tf.random_uniform([], minval=-np.log(max_aspect_ratio), maxval=np.log(max_aspect_ratio), seed=seed))

	    new_height = tf.to_int32(tf.to_float(height) * scale_h)
	    new_width = tf.to_int32(tf.to_float(width) * scale_w)

	    image_scaled = tf.squeeze(tf.image.resize_bilinear(tf.expand_dims(image_decoded, 0), [new_height, new_width]), [0])
	    # Image padding
	    pad_image, pad_offset = pad_to_ensure_size(image_scaled, input_shape[0], input_shape[1], seed=seed)

	    new_height = tf.maximum(input_shape[0], new_height)
	    new_width = tf.maximum(input_shape[1], new_width)

	    slice_offset = (tf.random_uniform([], minval=0, maxval=new_height - input_shape[0] + 1, dtype=tf.int32, seed=seed),
	                    tf.random_uniform([], minval=0, maxval=new_width - input_shape[1] + 1, dtype=tf.int32, seed=seed))
	    image = array_ops.slice(pad_image, [slice_offset[0], slice_offset[1], 0], [input_shape[0], input_shape[1], 3])

	    if horizontal_flip:
	        uniform_random = random_ops.random_uniform([], 0, 1.0, seed=seed)
	        did_horiz_flip = math_ops.less(uniform_random, .5)
	        image = control_flow_ops.cond(did_horiz_flip,
	                                       lambda: array_ops.reverse(image, [1]),
	                                       lambda: image)
	        flip_sign = 1 - tf.to_float(did_horiz_flip) * 2
	    else:
	        flip_sign = 1
	        did_horiz_flip = tf.constant(False)

	    ty = tf.to_float(pad_offset[0] - slice_offset[0])
	    tx = flip_sign * tf.to_float(pad_offset[1] - slice_offset[1]) + tf.to_float(did_horiz_flip) * input_shape[1]
	    transformation = tf.reshape(tf.stack([
	        scale_h, 0.0,                 ty,
	        0.0,     flip_sign * scale_w, tx,
	        0.0,     0.0,                 1.0]
	    ), (3, 3))

	    if max_hue is not None and max_hue > 0:
	        image = tf.image.random_hue(image, max_delta=max_hue, seed=seed)

	    if max_brightness is not None and max_brightness > 0:
	        image = tf.image.random_brightness(image, max_delta=max_brightness, seed=seed)

	    if max_saturation is not None and max_saturation > 1.0:
	        log_sat = np.log(max_saturation)
	        image = tf.image.random_saturation(image, lower=np.exp(-log_sat), upper=np.exp(log_sat), seed=seed)

	    if max_contrast is not None and max_contrast > 1.0:
	        log_con = np.log(max_contrast)
	        image = tf.image.random_contrast(image, lower=np.exp(-log_con), upper=np.exp(log_con), seed=seed)

	    image = tf.clip_by_value(image, 0, 1)

	    return image, img_id, orig_shape, transformation


	def centered_loader(paths,
	        input_shape=(416, 416),
	        horizontal_flip=False,
	        path_preparation_fn=None,
	        allow_scale_up=False,
	        seed=None):
	    """
	    Loads images centered in the fixed-size image, with padding.
	    """

	    filename_queue = tf.train.string_input_producer(paths)

	    reader = tf.TextLineReader()
	    key, img_id = reader.read(filename_queue)

	    if path_preparation_fn is not None:
	        full_path = path_preparation_fn(img_id)
	    else:
	        full_path = img_id

	    image_string = tf.read_file(full_path)
	    image_decoded = tf.to_float(decode_image(image_string, channels=3)) / 255
	    image_decoded.set_shape([None, None, 3])

	    imgshape = tf.shape(image_decoded)
	    height, width, _ = tf.unstack(imgshape)
	    orig_shape = (height, width)
	    scale = tf.minimum(tf.constant(input_shape[0], dtype=tf.float32) / tf.to_float(height),
	                       tf.constant(input_shape[1], dtype=tf.float32) / tf.to_float(width))

	    if not allow_scale_up:
	        scale = tf.minimum(1.0, scale)

	    new_height = tf.to_int32(tf.to_float(height) * scale)
	    new_width = tf.to_int32(tf.to_float(width) * scale)

	    image_scaled = tf.squeeze(tf.image.resize_bilinear(tf.expand_dims(image_decoded, 0), [new_height, new_width]), [0])

	    pad_image, pad_offset = pad_to_ensure_size(image_scaled, input_shape[0], input_shape[1], seed=seed,
	            random=False)

	    new_height = tf.maximum(input_shape[0], new_height)
	    new_width = tf.maximum(input_shape[1], new_width)

	    slice_offset = (tf.random_uniform([], minval=0, maxval=new_height - input_shape[0] + 1, dtype=tf.int32, seed=seed),
	                    tf.random_uniform([], minval=0, maxval=new_width - input_shape[1] + 1, dtype=tf.int32, seed=seed))
	    image = array_ops.slice(pad_image, [slice_offset[0], slice_offset[1], 0], [input_shape[0], input_shape[1], 3])

	    if horizontal_flip:
	        uniform_random = random_ops.random_uniform([], 0, 1.0, seed=seed)
	        did_horiz_flip = math_ops.less(uniform_random, .5)
	        image = control_flow_ops.cond(did_horiz_flip,
	                                       lambda: array_ops.reverse(image, [1]),
	                                       lambda: image)
	        flip_sign = 1 - tf.to_float(did_horiz_flip) * 2
	    else:
	        flip_sign = 1
	        did_horiz_flip = tf.constant(False)

	    ty = tf.to_float(pad_offset[0] - slice_offset[0])
	    tx = flip_sign * tf.to_float(pad_offset[1] - slice_offset[1]) + tf.to_float(did_horiz_flip) * input_shape[1]
	    transformation = tf.reshape(tf.stack([
	        scale, 0.0,                 ty,
	        0.0,     flip_sign * scale, tx,
	        0.0,     0.0,               1.0]
	    ), (3, 3))

	    image = tf.clip_by_value(image, 0, 1)

	    return image, img_id, orig_shape, transformation


	def stretched_loader(paths,
	        input_shape=(416, 416),
	        horizontal_flip=False,
	        path_preparation_fn=None,
	        allow_scale_up=True,
	        seed=None):
	    """
	    Loads images stretched to fit the fixed-size image.
	    """

	    filename_queue = tf.train.string_input_producer(paths)

	    reader = tf.TextLineReader()
	    key, img_id = reader.read(filename_queue)

	    if path_preparation_fn is not None:
	        full_path = path_preparation_fn(img_id)
	    else:
	        full_path = img_id

	    image_string = tf.read_file(full_path)
	    image_decoded = tf.to_float(decode_image(image_string, channels=3)) / 255
	    image_decoded.set_shape([None, None, 3])

	    imgshape = tf.shape(image_decoded)
	    height, width, _ = tf.unstack(imgshape)
	    orig_shape = (height, width)
	    scale_h = tf.constant(input_shape[0], dtype=tf.float32) / tf.to_float(height)
	    scale_w = tf.constant(input_shape[1], dtype=tf.float32) / tf.to_float(width)

	    if not allow_scale_up:
	        scale_w = tf.minimum(1.0, scale_w)
	        scale_h = tf.minimum(1.0, scale_h)

	    new_height = tf.to_int32(tf.to_float(height) * scale_h)
	    new_width = tf.to_int32(tf.to_float(width) * scale_w)

	    image_scaled = tf.squeeze(tf.image.resize_bilinear(tf.expand_dims(image_decoded, 0), [new_height, new_width]), [0])

	    pad_image, pad_offset = pad_to_ensure_size(image_scaled, input_shape[0], input_shape[1], seed=seed,
	            random=False)

	    new_height = tf.maximum(input_shape[0], new_height)
	    new_width = tf.maximum(input_shape[1], new_width)

	    slice_offset = (tf.random_uniform([], minval=0, maxval=new_height - input_shape[0] + 1, dtype=tf.int32, seed=seed),
	                    tf.random_uniform([], minval=0, maxval=new_width - input_shape[1] + 1, dtype=tf.int32, seed=seed))
	    image = array_ops.slice(pad_image, [slice_offset[0], slice_offset[1], 0], [input_shape[0], input_shape[1], 3])

	    if horizontal_flip:
	        uniform_random = random_ops.random_uniform([], 0, 1.0, seed=seed)
	        did_horiz_flip = math_ops.less(uniform_random, .5)
	        image = control_flow_ops.cond(did_horiz_flip,
	                                       lambda: array_ops.reverse(image, [1]),
	                                       lambda: image)
	        flip_sign = 1 - tf.to_float(did_horiz_flip) * 2
	    else:
	        flip_sign = 1
	        did_horiz_flip = tf.constant(False)

	    ty = tf.to_float(pad_offset[0] - slice_offset[0])
	    tx = flip_sign * tf.to_float(pad_offset[1] - slice_offset[1]) + tf.to_float(did_horiz_flip) * input_shape[1]
	    transformation = tf.reshape(tf.stack([
	        scale_h, 0.0,                 ty,
	        0.0,     flip_sign * scale_w, tx,
	        0.0,     0.0,               1.0]
	    ), (3, 3))

	    image = tf.clip_by_value(image, 0, 1)

	    return image, img_id, orig_shape, transformation


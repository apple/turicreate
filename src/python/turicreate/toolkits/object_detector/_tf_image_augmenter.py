# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import numpy as np
from PIL import Image
import tensorflow.compat.v1 as tf
import turicreate.toolkits._tf_utils as _utils
import turicreate as tc

tf.disable_v2_behavior()

_DEFAULT_AUG_PARAMS = {
    "max_hue_adjust": 0.05,
    "max_brightness": 0.05,
    "max_contrast": 1.25,
    "max_saturation": 1.25,
    "skip_probability_flip": 0.5,
    "min_aspect_ratio": 0.8,
    "max_aspect_ratio": 1.25,
    "min_area_fraction_crop": 0.15,
    "max_area_fraction_crop": 1.0,
    "min_area_fraction_pad": 1.0,
    "max_area_fraction_pad": 2.0,
    "max_attempts": 50,
    "skip_probability_pad": 0.1,
    "skip_probability_crop": 0.1,
    "min_object_covered": 0.0,
    "min_eject_coverage": 0.5,
    "resize_method": "turicreate",
}


def hue_augmenter(
    image, annotation, max_hue_adjust=_DEFAULT_AUG_PARAMS["max_hue_adjust"]
):

    # Sample a random rotation around the color wheel.
    hue_adjust = 0.0
    if (max_hue_adjust is not None) and (max_hue_adjust > 0.0):
        hue_adjust += np.pi * np.random.uniform(-max_hue_adjust, max_hue_adjust)

    # Apply the rotation to the hue
    image = tf.image.random_hue(image, max_delta=max_hue_adjust)
    image = tf.clip_by_value(image, 0, 1)

    # No geometry changes, so just copy the annotations.
    return image, annotation


def color_augmenter(
    image,
    annotation,
    max_brightness=_DEFAULT_AUG_PARAMS["max_brightness"],
    max_contrast=_DEFAULT_AUG_PARAMS["max_contrast"],
    max_saturation=_DEFAULT_AUG_PARAMS["max_saturation"],
):

    # Sample a random adjustment to brightness.
    if max_brightness is not None and max_brightness > 0:
        image = tf.image.random_brightness(image, max_delta=max_brightness)

    # Sample a random adjustment to contrast.
    if max_saturation is not None and max_saturation > 1.0:
        log_sat = np.log(max_saturation)
        image = tf.image.random_saturation(
            image, lower=np.exp(-log_sat), upper=np.exp(log_sat)
        )

    # Sample a random adjustment to saturation.
    if max_contrast is not None and max_contrast > 1.0:
        log_con = np.log(max_contrast)
        image = tf.image.random_contrast(
            image, lower=np.exp(-log_con), upper=np.exp(log_con)
        )

    image = tf.clip_by_value(image, 0, 1)

    # No geometry changes, so just copy the annotations.
    return image, annotation


def resize_augmenter(image, annotation, output_shape):

    resize_method = _DEFAULT_AUG_PARAMS["resize_method"]

    def resize_PIL_image(image, output_shape):
        image *= 255.0
        image = image.astype("uint8")
        pil_img = Image.fromarray(image)
        resize_img = pil_img.resize(
            (output_shape[1], output_shape[0]), resample=Image.BILINEAR
        )
        np_img = np.array(resize_img)
        np_img = np_img.astype(np.float32)
        np_img /= 255.0
        return np_img

    def resize_turicreate_image(image, output_shape):
        image *= 255.0
        image = image.astype("uint8")
        FORMAT_RAW = 2
        tc_image = tc.Image(
            _image_data=image.tobytes(),
            _width=image.shape[1],
            _height=image.shape[0],
            _channels=image.shape[2],
            _format_enum=FORMAT_RAW,
            _image_data_size=image.size,
        )
        tc_image = tc.image_analysis.resize(
            tc_image, output_shape[1], output_shape[0], resample="bilinear"
        )
        image = tc_image.pixel_data
        image = image.astype(np.float32)
        image /= 255.0
        return image

    if resize_method == "tensorflow":
        new_height = tf.cast(output_shape[0], dtype=tf.int32)
        new_width = tf.cast(output_shape[1], dtype=tf.int32)

        # Determine the affine transform to apply and apply to the image itself.
        image_scaled = tf.squeeze(
            tf.image.resize_bilinear(tf.expand_dims(image, 0), [new_height, new_width]),
            [0],
        )

    elif resize_method == "PIL":
        image_scaled = tf.numpy_function(
            func=resize_PIL_image, inp=[image, output_shape], Tout=[tf.float32]
        )

    elif resize_method == "turicreate":
        image_scaled = tf.numpy_function(
            func=resize_turicreate_image, inp=[image, output_shape], Tout=[tf.float32]
        )

    else:
        raise Exception("Non-supported resize method.")

    image_clipped = tf.clip_by_value(image_scaled, 0.0, 1.0)
    annotation = tf.clip_by_value(annotation, 0.0, 1.0)

    # No geometry changes (because of relative co-ordinate system)
    return image_clipped, annotation


def horizontal_flip_augmenter(
    image, annotation, skip_probability=_DEFAULT_AUG_PARAMS["skip_probability_flip"]
):

    if np.random.uniform(0.0, 1.0) < skip_probability:
        return image, annotation

    image_height, image_width, _ = image.shape
    flipped_image = np.flip(image, 1)

    # One image can have more than one annotation. so loop through annotations and flip across the horizontal axis
    for i in range(len(annotation)):
        # Only flip if the annotation is not an empty annotation.
        if np.any(annotation[i][1:5]):
            annotation[i][1] = 1 - annotation[i][1] - annotation[i][3]

    return flipped_image, annotation


def padding_augmenter(
    image,
    annotation,
    skip_probability=_DEFAULT_AUG_PARAMS["skip_probability_pad"],
    min_aspect_ratio=_DEFAULT_AUG_PARAMS["min_aspect_ratio"],
    max_aspect_ratio=_DEFAULT_AUG_PARAMS["max_aspect_ratio"],
    min_area_fraction=_DEFAULT_AUG_PARAMS["min_area_fraction_pad"],
    max_area_fraction=_DEFAULT_AUG_PARAMS["max_area_fraction_pad"],
    max_attempts=_DEFAULT_AUG_PARAMS["max_attempts"],
):
    if np.random.uniform(0.0, 1.0) < skip_probability:
        return np.array(image), annotation

    image_height, image_width, _ = image.shape

    # Randomly sample aspect ratios until one derives a non-empty range of
    # compatible heights, or until reaching the upper limit on attempts.
    for i in range(max_attempts):
        # Randomly sample an aspect ratio.
        aspect_ratio = np.random.uniform(min_aspect_ratio, max_aspect_ratio)

        # The padded height must be at least as large as the original height.
        # h' >= h
        min_height = float(image_height)

        # The padded width must be at least as large as the original width.
        # w' >= w IMPLIES ah' >= w IMPLIES h' >= w / a
        min_height_from_width = float(image_width) / aspect_ratio
        if min_height < min_height_from_width:
            min_height = min_height_from_width

        # The padded area must attain the minimum area fraction.
        # w'h' >= fhw IMPLIES ah'h' >= fhw IMPLIES h' >= sqrt(fhw/a)
        min_height_from_area = np.sqrt(
            image_height * image_width * min_area_fraction / aspect_ratio
        )
        if min_height < min_height_from_area:
            min_height = min_height_from_area

        # The padded area must not exceed the maximum area fraction.
        max_height = np.sqrt(
            image_height * image_width * max_area_fraction / aspect_ratio
        )

        if min_height >= max_height:
            break

    # We did not find a compatible aspect ratio. Just return the original data.
    if min_height > max_height:
        return np.array(image), annotation

    # Sample a final size, given the sampled aspect ratio and range of heights.
    padded_height = np.random.uniform(min_height, max_height)
    padded_width = padded_height * aspect_ratio

    # Sample the offset of the source image inside the padded image.
    x_offset = np.random.uniform(0.0, (padded_width - image_width))
    y_offset = np.random.uniform(0.0, (padded_height - image_height))

    # Compute padding needed on the image
    after_padding_width = padded_width - image_width - x_offset
    after_padding_height = padded_height - image_height - y_offset

    # Pad the image
    npad = (
        (int(y_offset), int(after_padding_height)),
        (int(x_offset), int(after_padding_width)),
        (0, 0),
    )
    padded_image = np.pad(image, pad_width=npad, mode="constant", constant_values=0.5)

    ty = float(y_offset)
    tx = float(x_offset)

    # Transformation matrix for the annotations
    transformation_matrix = np.array([[1.0, 0.0, ty], [0.0, 1.0, tx], [0.0, 0.0, 1.0]])

    # Use transformation matrix to augment annotations
    formatted_annotation = []
    for aug in annotation:
        identifier = aug[0:1]
        bounds = aug[1:5]
        confidence = aug[5:6]

        if not np.any(bounds):
            formatted_annotation.append(
                np.concatenate([identifier, np.array([0, 0, 0, 0]), confidence])
            )
            continue

        width = bounds[2]
        height = bounds[3]

        x1 = bounds[0] * image_width
        y1 = bounds[1] * image_height
        x2 = (bounds[0] + width) * image_width
        y2 = (bounds[1] + height) * image_height

        augmentation_coordinates = np.array([y1, x1, y2, x2], dtype=np.float32)

        v = np.concatenate(
            [
                augmentation_coordinates.reshape((2, 2)),
                np.ones((2, 1), dtype=np.float32),
            ],
            axis=1,
        )
        transposed_v = np.dot(v, np.transpose(transformation_matrix))
        t_intersection = np.squeeze(transposed_v[:, :2].reshape(-1, 4))

        # Sort the points top, left, bottom, right
        if t_intersection[0] > t_intersection[2]:
            t_intersection[0], t_intersection[2] = t_intersection[2], t_intersection[0]
        if t_intersection[1] > t_intersection[3]:
            t_intersection[1], t_intersection[3] = t_intersection[3], t_intersection[1]

        # Normalize the elements to the cropped width and height
        ele_1 = t_intersection[1] / padded_width
        ele_2 = t_intersection[0] / padded_height
        ele_3 = (t_intersection[3] - t_intersection[1]) / padded_width
        ele_4 = (t_intersection[2] - t_intersection[0]) / padded_height

        formatted_annotation.append(
            np.concatenate(
                [identifier, np.array([ele_1, ele_2, ele_3, ele_4]), confidence]
            )
        )

    return np.array(padded_image), np.array(formatted_annotation, dtype=np.float32)


def crop_augmenter(
    image,
    annotation,
    skip_probability=_DEFAULT_AUG_PARAMS["skip_probability_crop"],
    min_aspect_ratio=_DEFAULT_AUG_PARAMS["min_aspect_ratio"],
    max_aspect_ratio=_DEFAULT_AUG_PARAMS["max_aspect_ratio"],
    min_area_fraction=_DEFAULT_AUG_PARAMS["min_area_fraction_crop"],
    max_area_fraction=_DEFAULT_AUG_PARAMS["max_area_fraction_crop"],
    min_object_covered=_DEFAULT_AUG_PARAMS["min_object_covered"],
    max_attempts=_DEFAULT_AUG_PARAMS["max_attempts"],
    min_eject_coverage=_DEFAULT_AUG_PARAMS["min_eject_coverage"],
):

    if np.random.uniform(0.0, 1.0) < skip_probability:
        return np.array(image), annotation

    image_height, image_width, _ = image.shape

    # Sample crop rects until one satisfies our constraints (by yielding a valid
    # list of cropped annotations), or reaching the limit on attempts.
    for i in range(max_attempts):
        # Randomly sample an aspect ratio.
        aspect_ratio = np.random.uniform(min_aspect_ratio, max_aspect_ratio)

        # Next we'll sample a height (which combined with the now known aspect
        # ratio, determines the size and area). But first we must compute the range
        # of valid heights. The crop cannot be taller than the original image,
        # cannot be wider than the original image, and must have an area in the
        # specified range.

        # The cropped height must be no larger the original height.
        # h' <= h
        max_height = float(image_height)

        # The cropped width must be no larger than the original width.
        # w' <= w IMPLIES ah' <= w IMPLIES h' <= w / a
        max_height_from_width = float(image_width) / aspect_ratio
        if max_height > max_height_from_width:
            max_height = max_height_from_width

        # The cropped area must not exceed the maximum area fraction.
        max_height_from_area = np.sqrt(
            image_height * image_width * max_area_fraction / aspect_ratio
        )
        if max_height > max_height_from_area:
            max_height = max_height_from_area

        # The padded area must attain the minimum area fraction.
        min_height = np.sqrt(
            image_height * image_width * min_area_fraction / aspect_ratio
        )

        # If the range is empty, then crops with the sampled aspect ratio cannot
        # satisfy the area constraint.
        if min_height > max_height:
            continue

        # Sample a position for the crop, constrained to lie within the image.
        cropped_height = np.random.uniform(min_height, max_height)
        cropped_width = cropped_height * aspect_ratio

        x_offset = np.random.uniform(0.0, (image_width - cropped_width))
        y_offset = np.random.uniform(0.0, (image_height - cropped_height))

        crop_bounds_x1 = x_offset
        crop_bounds_y1 = y_offset
        crop_bounds_x2 = x_offset + cropped_width
        crop_bounds_y2 = y_offset + cropped_height

        formatted_annotation = []
        is_min_object_covered = True
        for aug in annotation:
            identifier = aug[0:1]
            bounds = aug[1:5]
            confidence = aug[5:6]

            width = bounds[2]
            height = bounds[3]

            x1 = bounds[0] * image_width
            y1 = bounds[1] * image_height

            x2 = (bounds[0] + width) * image_width
            y2 = (bounds[1] + height) * image_height

            # This tests whether the crop bounds are out of the annotated bounds, if not it returns an empty annotation
            if (
                crop_bounds_x1 < x2
                and crop_bounds_y1 < y2
                and x1 < crop_bounds_x2
                and y1 < crop_bounds_y2
            ):
                x_bounds = [x1, x2, x_offset, x_offset + cropped_width]
                y_bounds = [y1, y2, y_offset, y_offset + cropped_height]

                x_bounds.sort()
                y_bounds.sort()

                x_pairs = x_bounds[1:3]
                y_pairs = y_bounds[1:3]

                intersection = np.array(
                    [y_pairs[0], x_pairs[0], y_pairs[1], x_pairs[1]]
                )

                intersection_area = (intersection[3] - intersection[1]) * (
                    intersection[2] - intersection[0]
                )
                annotation_area = (y2 - y1) * (x2 - x1)

                area_coverage = intersection_area / annotation_area

                # Invalidate the crop if it did not sufficiently overlap each annotation and try again.
                if area_coverage < min_object_covered:
                    is_min_object_covered = False
                    break

                # If the area coverage is greater the min_eject_coverage, then actually keep the annotation
                if area_coverage >= min_eject_coverage:
                    # Transformation matrix for the annotations
                    transformation_matrix = np.array(
                        [[1.0, 0.0, -y_offset], [0.0, 1.0, -x_offset], [0.0, 0.0, 1.0]]
                    )

                    v = np.concatenate(
                        [
                            intersection.reshape((2, 2)),
                            np.ones((2, 1), dtype=np.float32),
                        ],
                        axis=1,
                    )
                    transposed_v = np.dot(v, np.transpose(transformation_matrix))
                    t_intersection = np.squeeze(transposed_v[:, :2].reshape(-1, 4))

                    # Sort the points top, left, bottom, right
                    if t_intersection[0] > t_intersection[2]:
                        t_intersection[0], t_intersection[2] = (
                            t_intersection[2],
                            t_intersection[0],
                        )
                    if t_intersection[1] > t_intersection[3]:
                        t_intersection[1], t_intersection[3] = (
                            t_intersection[3],
                            t_intersection[1],
                        )

                    # Normalize the elements to the cropped width and height
                    ele_1 = t_intersection[1] / cropped_width
                    ele_2 = t_intersection[0] / cropped_height
                    ele_3 = (t_intersection[3] - t_intersection[1]) / cropped_width
                    ele_4 = (t_intersection[2] - t_intersection[0]) / cropped_height

                    formatted_annotation.append(
                        np.concatenate(
                            [
                                identifier,
                                np.array([ele_1, ele_2, ele_3, ele_4]),
                                confidence,
                            ]
                        )
                    )
                else:
                    formatted_annotation.append(
                        np.concatenate(
                            [identifier, np.array([0.0, 0.0, 0.0, 0.0]), confidence]
                        )
                    )
            else:
                formatted_annotation.append(
                    np.concatenate(
                        [identifier, np.array([0.0, 0.0, 0.0, 0.0]), confidence]
                    )
                )

        if not is_min_object_covered:
            continue

        y_offset = int(y_offset)
        x_offset = int(x_offset)
        end_y = int(cropped_height + y_offset)
        end_x = int(cropped_width + x_offset)

        image_cropped = image[y_offset:end_y, x_offset:end_x]

        return np.array(image_cropped), np.array(formatted_annotation, dtype=np.float32)

    return np.array(image), annotation


def complete_augmenter(img_tf, ann_tf, output_height, output_width):
    img_tf, ann_tf = tf.numpy_function(
        func=crop_augmenter, inp=[img_tf, ann_tf], Tout=[tf.float32, tf.float32]
    )
    img_tf, ann_tf = tf.numpy_function(
        func=padding_augmenter, inp=[img_tf, ann_tf], Tout=[tf.float32, tf.float32]
    )
    img_tf, ann_tf = tf.numpy_function(
        func=horizontal_flip_augmenter,
        inp=[img_tf, ann_tf],
        Tout=[tf.float32, tf.float32],
    )
    img_tf, ann_tf = color_augmenter(img_tf, ann_tf)
    img_tf, ann_tf = hue_augmenter(img_tf, ann_tf)
    img_tf, ann_tf = resize_augmenter(img_tf, ann_tf, (output_height, output_width))
    return img_tf, ann_tf


class DataAugmenter(object):
    def __init__(self, output_height, output_width, batch_size, resize_only):
        self.batch_size = batch_size
        self.graph = tf.Graph()
        self.resize_only = resize_only
        with self.graph.as_default():
            self.img_tf = [
                tf.placeholder(tf.float32, [None, None, 3])
                for x in range(0, self.batch_size)
            ]
            self.ann_tf = [
                tf.placeholder(tf.float32, [None, 6]) for x in range(0, self.batch_size)
            ]
            self.resize_op_batch = []
            for i in range(0, self.batch_size):
                if resize_only:
                    aug_img_tf, aug_ann_tf = resize_augmenter(
                        self.img_tf[i], self.ann_tf[i], (output_height, output_width)
                    )
                    self.resize_op_batch.append([aug_img_tf, aug_ann_tf])
                else:
                    aug_img_tf, aug_ann_tf = complete_augmenter(
                        self.img_tf[i], self.ann_tf[i], output_height, output_width
                    )
                    self.resize_op_batch.append([aug_img_tf, aug_ann_tf])

    def get_augmented_data(self, images, annotations):
        with tf.Session(graph=self.graph) as session:
            feed_dict = dict()
            graph_op = self.resize_op_batch[0 : len(images)]
            for i in range(0, len(images)):
                feed_dict[self.img_tf[i]] = _utils.convert_shared_float_array_to_numpy(
                    images[i]
                )
                if self.resize_only:
                    feed_dict[self.ann_tf[i]] = self.batch_size * [np.zeros(6)]
                else:
                    feed_dict[
                        self.ann_tf[i]
                    ] = _utils.convert_shared_float_array_to_numpy(annotations[i])
            aug_output = session.run(graph_op, feed_dict=feed_dict)
            processed_images = []
            processed_annotations = []
            for o in aug_output:
                processed_images.append(o[0])
                processed_annotations.append(
                    np.ascontiguousarray(o[1], dtype=np.float32)
                )
            processed_images = np.array(processed_images, dtype=np.float32)
            processed_images = np.ascontiguousarray(processed_images, dtype=np.float32)
            return (processed_images, processed_annotations)

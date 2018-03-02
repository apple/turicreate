# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _tc
import numpy as _np
from turicreate.toolkits._internal_utils import _numeric_param_check_range


COLOR_NAMES = [
    'AliceBlue', 'Chartreuse', 'Aqua', 'Aquamarine', 'Azure', 'Beige',
    'Bisque', 'BlanchedAlmond', 'BlueViolet', 'BurlyWood', 'CadetBlue',
    'AntiqueWhite', 'Chocolate', 'Coral', 'CornflowerBlue', 'Cornsilk',
    'Crimson', 'Cyan', 'DarkCyan', 'DarkGoldenRod', 'DarkGrey', 'DarkKhaki',
    'DarkOrange', 'DarkOrchid', 'DarkSalmon', 'DarkSeaGreen', 'DarkTurquoise',
    'DarkViolet', 'DeepPink', 'DeepSkyBlue', 'DodgerBlue', 'FireBrick',
    'FloralWhite', 'ForestGreen', 'Fuchsia', 'Gainsboro', 'GhostWhite', 'Gold',
    'GoldenRod', 'Salmon', 'Tan', 'HoneyDew', 'HotPink', 'IndianRed', 'Ivory',
    'Khaki', 'Lavender', 'LavenderBlush', 'LawnGreen', 'LemonChiffon',
    'LightBlue', 'LightCoral', 'LightCyan', 'LightGoldenRodYellow',
    'LightGray', 'LightGrey', 'LightGreen', 'LightPink', 'LightSalmon',
    'LightSeaGreen', 'LightSkyBlue', 'LightSlateGray', 'LightSlateGrey',
    'LightSteelBlue', 'LightYellow', 'Lime', 'LimeGreen', 'Linen', 'Magenta',
    'MediumAquaMarine', 'MediumOrchid', 'MediumPurple', 'MediumSeaGreen',
    'MediumSlateBlue', 'MediumSpringGreen', 'MediumTurquoise',
    'MediumVioletRed', 'MintCream', 'MistyRose', 'Moccasin', 'NavajoWhite',
    'OldLace', 'Olive', 'OliveDrab', 'Orange', 'OrangeRed', 'Orchid',
    'PaleGoldenRod', 'PaleGreen', 'PaleTurquoise', 'PaleVioletRed',
    'PapayaWhip', 'PeachPuff', 'Peru', 'Pink', 'Plum', 'PowderBlue', 'Purple',
    'Red', 'RosyBrown', 'RoyalBlue', 'SaddleBrown', 'Green', 'SandyBrown',
    'SeaGreen', 'SeaShell', 'Sienna', 'Silver', 'SkyBlue', 'SlateBlue',
    'SlateGray', 'SlateGrey', 'Snow', 'SpringGreen', 'SteelBlue',
    'GreenYellow', 'Teal', 'Thistle', 'Tomato', 'Turquoise', 'Violet', 'Wheat',
    'White', 'WhiteSmoke', 'Yellow', 'YellowGreen'
]


def _annotate_image(pil_image, anns, confidence_threshold):
    from PIL import ImageDraw, ImageFont
    draw = ImageDraw.Draw(pil_image)
    font = ImageFont.load_default()
    BUF = 2

    # Reverse, to print the highest confidence on top
    for ann in reversed(anns):
        if 'confidence' in ann and ann['confidence'] < confidence_threshold:
            continue
        if 'label' in ann:
            color = COLOR_NAMES[hash(ann['label']) % len(COLOR_NAMES)]
        else:
            color = 'White'

        left = ann['coordinates']['x'] - ann['coordinates']['width'] / 2
        top = ann['coordinates']['y'] - ann['coordinates']['height'] / 2
        right = ann['coordinates']['x'] + ann['coordinates']['width'] / 2
        bottom = ann['coordinates']['y'] + ann['coordinates']['height'] / 2

        draw.line([(left, top), (left, bottom), (right, bottom),
                   (right, top), (left, top)], width=4, fill=color)


        if 'confidence' in ann:
            text = '{} {:.0%}'.format(ann['label'], ann['confidence'])
        else:
            text = ann['label']

        width, height = font.getsize(text)

        if top < height + 2 * BUF:
            label_top = bottom + height + 2 * BUF
        else:
            label_top = top
        draw.rectangle([(left - 1, label_top - height - 2 * BUF),
                        (left + width + 2 * BUF, label_top)], fill=color)

        draw.text((left + BUF, label_top - height - BUF),
                  text,
                  fill='black',
                  font=font)


def draw_bounding_boxes(images, annotations, confidence_threshold=0):
    """
    Visualizes bounding boxes (ground truth or predictions) by
    returning annotated copies of the images.

    Parameters
    ----------
    images: SArray or Image
        An `SArray` of type `Image`. A single `Image` instance may also be
        given.

    annotations: SArray or list
        An `SArray` of annotations (either output from the
        `ObjectDetector.predict` function or ground truth). A single list of
        annotations may also be given, provided that it is coupled with a
        single image.

    confidence_threshold: float
        Confidence threshold can limit the number of boxes to draw. By
        default, this is set to 0, since the prediction may have already pruned
        with an appropriate confidence threshold.

    Returns
    -------
    annotated_images: SArray or Image
        Similar to the input `images`, except the images are decorated with
        boxes to visualize the object instances.

    See also
    --------
    unstack_annotations
    """
    _numeric_param_check_range('confidence_threshold', confidence_threshold, 0.0, 1.0)
    from PIL import Image
    def draw_single_image(row):
        image = row['image']
        anns = row['annotations']
        if anns == None:
            anns = []
        elif type(anns) == dict:
            anns = [anns]
        pil_img = Image.fromarray(image.pixel_data)
        _annotate_image(pil_img, anns, confidence_threshold=confidence_threshold)
        image = _np.array(pil_img)
        FORMAT_RAW = 2
        annotated_image = _tc.Image(_image_data=image.tobytes(),
                                    _width=image.shape[1],
                                    _height=image.shape[0],
                                    _channels=image.shape[2],
                                    _format_enum=FORMAT_RAW,
                                    _image_data_size=image.size)
        return annotated_image

    if isinstance(images, _tc.Image) and isinstance(annotations, list):
        return draw_single_image({'image': images, 'annotations': annotations})
    else:
        return (_tc.SFrame({'image': images, 'annotations': annotations})
                .apply(draw_single_image))

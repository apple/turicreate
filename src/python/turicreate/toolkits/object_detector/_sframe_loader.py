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
from six.moves.queue import Queue as _Queue
from threading import Thread as _Thread
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from ._detection import yolo_boxes_to_yolo_map as _yolo_boxes_to_yolo_map

_TMP_COL_RANDOM_ORDER = "_random_order"


def _convert_image_to_raw(image):
    # Decode image and make sure it has 3 channels
    return _tc.image_analysis.resize(image, image.width, image.height, 3, decode=True)


def _is_rectangle_annotation(ann):
    return "type" not in ann or ann["type"] == "rectangle"


def _is_valid_annotation(ann):
    if not isinstance(ann, dict):
        return False
    if not _is_rectangle_annotation(ann):
        # Not necessarily valid, but we bypass stricter checks (we simply do
        # not care about non-rectangle types)
        return True
    return (
        "coordinates" in ann
        and isinstance(ann["coordinates"], dict)
        and set(ann["coordinates"].keys()) == {"x", "y", "width", "height"}
        and ann["coordinates"]["width"] > 0
        and ann["coordinates"]["height"] > 0
        and "label" in ann
    )


def _is_valid_annotations_list(anns):
    return all([_is_valid_annotation(ann) for ann in anns])

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
from turicreate.toolkits._main import ToolkitError as _ToolkitError

_TMP_COL_PREP_IMAGE = "_prepared_image"
_TMP_COL_RANDOM_ORDER = "_random_order"


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
    return _tc.image_analysis.resize(
        image, width, height, 3, decode=True, resample="bilinear"
    )


def _stretch_resize(image, shape):
    height, width = shape
    return _tc.image_analysis.resize(
        image, width, height, 3, decode=True, resample="bilinear"
    )

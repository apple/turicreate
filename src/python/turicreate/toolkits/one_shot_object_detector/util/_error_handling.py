# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import turicreate as _tc
import turicreate.toolkits._internal_utils as _tkutl
from turicreate.toolkits._main import ToolkitError as _ToolkitError


def check_one_shot_input(data, target, backgrounds):
    if backgrounds is not None and not (isinstance(backgrounds, _tc.SArray)):
        raise TypeError("'backgrounds' must be None or an SArray.")
    if isinstance(backgrounds, _tc.SArray) and len(backgrounds) == 0:
        raise _ToolkitError("Unable to train with no background images")
    if not isinstance(target, str):
        raise TypeError("'target' must be of type string.")
    if isinstance(data, _tc.SFrame):
        _tkutl._raise_error_if_column_exists(data, target, "data", target)
        image_column_name = _tkutl._find_only_image_column(data)
        target_column_name = target
        dataset_to_augment = data
    elif isinstance(data, _tc.Image):
        image_column_name = "image"
        target_column_name = "target"
        dataset_to_augment = _tc.SFrame(
            {image_column_name: [data], target_column_name: [target]}
        )
    else:
        raise TypeError("'data' must be of type SFrame or Image.")
    return dataset_to_augment, image_column_name, target_column_name

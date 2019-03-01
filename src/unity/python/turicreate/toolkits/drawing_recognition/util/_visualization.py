# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
import turicreate as _tc
from turicreate import extensions as _extensions

def draw_strokes(stroke_based_drawings):
    if type(stroke_based_drawings) != _tc.SArray:
        raise _ToolkitError("Input to draw_strokes must be of type " 
            + "turicreate.SArray")
    is_stroke_input = (stroke_based_drawings.dtype != _tc.Image)
    if is_stroke_input:
        sf = _tc.SFrame({"drawings": stroke_based_drawings})
        sf_with_drawings = _extensions._drawing_recognition_prepare_data(
            sf, "drawings")
        return sf_with_drawings["drawings"]
    else:
        # User passed in drawings that were already drawings
        return stroke_based_drawings

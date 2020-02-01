# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
import turicreate as _tc
from turicreate import extensions as _extensions
from turicreate.toolkits._main import ToolkitError as _ToolkitError


def draw_strokes(stroke_based_drawings):
    """
    Visualizes drawings (ground truth or predictions) by
    returning images to represent the stroke-based data from
    the user.

    Parameters
    ----------
    stroke_based_drawings: SArray or list
        An `SArray` of type `list`. Each element in the SArray
        should be a list of strokes, where each stroke is a list
        of points, and each point is represented as a dictionary
        with two keys, "x" and "y". A single stroke-based drawing
        is also supported, in which case, the type of the input
        would be list.

    Returns
    -------
    drawings: SArray or _tc.Image
        Each stroke-based drawing is converted into a 28x28
        grayscale drawing for the user to visualize what their
        strokes traced.

    """
    single_input = False
    if not isinstance(stroke_based_drawings, _tc.SArray) and not isinstance(
        stroke_based_drawings, list
    ):
        raise _ToolkitError(
            "Input to draw_strokes must be of type "
            + "turicreate.SArray or list (for a single stroke-based drawing)"
        )
    if (
        isinstance(stroke_based_drawings, _tc.SArray)
        and stroke_based_drawings.dtype != list
    ):
        raise _ToolkitError(
            "SArray input to draw_strokes must have dtype "
            + "list. Each element in the SArray should be a list of strokes, "
            + "where each stroke is a list of points, "
            + "and each point is represented as a dictionary "
            + 'with two keys, "x" and "y".'
        )
    if isinstance(stroke_based_drawings, list):
        single_input = True
        stroke_based_drawings = _tc.SArray([stroke_based_drawings])
    sf = _tc.SFrame({"drawings": stroke_based_drawings})
    sf_with_drawings = _extensions._drawing_classifier_prepare_data(sf, "drawings")
    if single_input:
        return sf_with_drawings["drawings"][0]
    return sf_with_drawings["drawings"]

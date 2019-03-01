import turicreate as _tc
import numpy as _np
from turicreate import extensions as _extensions

def draw_strokes(stroke_based_drawings):
    if type(stroke_based_drawings) != _tc.SArray:
        raise _ToolkitError("Input to draw_strokes must be of type " 
            + "turicreate.SArray")
    is_stroke_input = (stroke_based_drawings.dtype != _tc.Image)
    if is_stroke_input:
        sf = tc.SFrame({"drawings": stroke_based_drawings})
        sf_with_drawings = _extensions._drawing_recognition_prepare_data(
            sf, "drawings")
        return sf_with_drawings["drawings"]
    else:
        # User passed in drawings that were already drawings
        return stroke_based_drawings

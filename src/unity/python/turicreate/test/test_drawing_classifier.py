# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _tc
from turicreate.toolkits._main import ToolkitError as _ToolkitError
import numpy as _np
import tempfile as _tempfile
import coremltools as _coremltools
import unittest
import pytest

# def _build_bitmap_data():
#     '''
#     Create 10 random drawings and build an SFrame.
#     '''
#     from PIL import Image

#     num_rows_in_sframe = 10
#     drawings, labels = [], []
#     # Create random drawings
#     random = _np.random.RandomState(100)
#     for label in range(num_rows_in_sframe):
#         width, height = random.randint(1000), random.randint(1000)
#         cur_data = _np.full((width, height), 0, dtype=_np.uint8)
#         for x in range(width):
#             for y in range(height):
#                 cur_data[x][y] = random.randint(255)
#         pil_image = Image.fromarray(cur_data, mode='L')
#         image_data = bytearray([l for l in pil_image.getdata()])
#         image_data_size = len(image_data)
#         drawing = _tc.Image(_image_data = image_data,
#                             _width = width, _height = height,
#                             _channels = 1, _format_enum = 2,
#                             _image_data_size = image_data_size)
#         drawings.append(drawing)
#         labels.append(label)
#     return _tc.SFrame({"drawing": drawings, "label": labels})

# def _build_stroke_data():
#     num_rows_in_sframe = 10
#     drawings, labels = [], []
#     random = _np.random.RandomState(100)
#     def _generate_random_point(point = None):
#         if point is not None:
#             dx = random.choice([-1, 0, 1])
#             dy = random.choice([-1, 0, 1])
#             next_x, next_y = point["x"] + dx, point["y"] + dy
#         else:
#             next_x, next_y = random.randint(1000), random.randint(1000)
#         return {"x": next_x, "y": next_y}

#     for label in range(num_rows_in_sframe):
#         num_strokes = random.randint(10)
#         print("num_strokes = " + str(num_strokes))
#         drawing = []
#         for stroke_id in range(num_strokes):
#             print("stroke_id = " + str(stroke_id))
#             drawing.append([])
#             num_points = random.randint(500)
#             last_point = None
#             print("num_points = " + str(num_points))
#             for point_id in range(num_points):
#                 print("point_id = " + str(point_id))
#                 last_point = _generate_random_point(last_point)
#                 drawing[-1].append(last_point)
#         drawings.append(drawing)
#         labels.append(label)

#     return _tc.SFrame({"drawing": drawings, "label": labels})


class DrawingClassifierTest(unittest.TestCase):
    @classmethod
    def setUpClass(self, pretrained_model_url = None):
        print("reached here")
        self.feature = "drawing"
        self.target = "label"
        print("building bitmap_sf")
        # self.bitmap_sf = _build_bitmap_data()
        print("built bitmap_sf")
        # self.stroke_sf = _build_stroke_data()
        print("built stroke_sf")
        # self.bitmap_model = _tc.drawing_recognition.create(self.bitmap_sf, 
        #     feature=self.feature, target=self.target, num_epochs=1)
        print("built bitmap_model")
        # self.stroke_model = _tc.drawing_recognition.create(self.stroke_sf, 
        #     feature=self.feature, target=self.target, num_epochs=1)
        print("built stroke_model")

    # def test_create_with_missing_feature(self):
    #     with self.assertRaises(_ToolkitError):
    #         _tc.drawing_recognition.create(self.bitmap_sf, 
    #                                        feature="wrong_feature")

    # def test_create_with_missing_target(self):
    #     with self.assertRaises(_ToolkitError):
    #         _tc.drawing_recognition.create(self.bitmap_sf, 
    #                                        feature=self.feature, 
    #                                        target="wrong_target")

    # def test_create_with_empty_dataset(self):
    #     with self.assertRaises(_ToolkitError):
    #         _tc.drawing_recognition.create(self.bitmap_sf[:0], 
    #                                        feature=self.feature, 
    #                                        target=self.target)

    def test_create_with_missing_coordinates_in_stroke_input(self):
        pass

    def test_create_with_wrongly_typed_coordinates_in_stroke_input(self):
        pass

    def test_create_with_zero_strokes_in_stroke_input(self):
        pass

    def test_predict(self):
        pass

    def test_evaluate(self):
        pass

    def test_coreml_export(self):
        pass

    def test_draw_strokes(self):
        pass

    def test_export_coreml_with_predict(self):
        pass

    def test_save_and_load(self):
        pass

    def test_repr(self):
        pass

    def test_summary(self):
        pass


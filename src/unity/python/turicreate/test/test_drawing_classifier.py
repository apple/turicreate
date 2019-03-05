# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import os as _os
import turicreate as _tc
from turicreate.toolkits._main import ToolkitError as _ToolkitError
import numpy as _np
import tempfile as _tempfile
import coremltools as _coremltools
from copy import copy as _copy
import unittest
import pytest

def _build_bitmap_data(use_saved = True):
    '''
    Create 10 random drawings, or 10 saved drawings and build an SFrame.
    '''
    if use_saved:
        drawings_dir = _os.path.join(
            _os.path.dirname(_os.path.realpath(__file__)), 
            "images", "drawings")
        sf = _tc.image_analysis.load_images(drawings_dir, with_path=True)
        sf = sf.rename({"image": "drawing", "path": "label"})
        sf["label"] = sf["label"].apply(lambda path: path[-10:-5])
        return sf
    else:
        from PIL import Image
        num_rows_in_sframe = 10
        drawings, labels = [], []
        # Create random drawings
        random = _np.random.RandomState(100)
        for label in range(num_rows_in_sframe):
            width, height = random.randint(1000), random.randint(1000)
            cur_data = _np.full((width, height), 0, dtype=_np.uint8)
            for x in range(width):
                for y in range(height):
                    cur_data[x][y] = random.randint(255)
            pil_image = Image.fromarray(cur_data, mode='L')
            image_data = bytearray([l for l in pil_image.getdata()])
            image_data_size = len(image_data)
            drawing = _tc.Image(_image_data = image_data,
                                _width = width, _height = height,
                                _channels = 1, _format_enum = 2,
                                _image_data_size = image_data_size)
            drawings.append(drawing)
            labels.append(label)
        return _tc.SFrame({"drawing": drawings, "label": labels})

def _build_stroke_data():
    num_rows_in_sframe = 10
    drawings, labels = [], []
    random = _np.random.RandomState(100)
    def _generate_random_point(point = None):
        if point is not None:
            dx = random.choice([-1, 0, 1])
            dy = random.choice([-1, 0, 1])
            next_x, next_y = point["x"] + dx, point["y"] + dy
        else:
            next_x, next_y = random.randint(1000), random.randint(1000)
        return {"x": next_x, "y": next_y}

    for label in range(num_rows_in_sframe):
        num_strokes = random.randint(10)
        drawing = []
        for stroke_id in range(num_strokes):
            drawing.append([])
            num_points = random.randint(500)
            last_point = None
            for point_id in range(num_points):
                last_point = _generate_random_point(last_point)
                drawing[-1].append(last_point)
        drawings.append(drawing)
        labels.append(label)

    return _tc.SFrame({"drawing": drawings, "label": labels})

class DrawingClassifierTest(unittest.TestCase):
    @classmethod
    def setUpClass(self, pretrained_model_url = None):
        self.feature = "drawing"
        self.target = "label"
        self.random_bitmap_sf = _build_bitmap_data(use_saved = False)
        self.check_cross_sf = _build_bitmap_data(use_saved = True)
        self.stroke_sf = _build_stroke_data()
        self.random_bitmap_model = _tc.drawing_classifier.create(
            self.random_bitmap_sf, 
            feature=self.feature, target=self.target, num_epochs=1)
        self.check_cross_model = _tc.drawing_classifier.create(
            self.check_cross_sf, 
            feature=self.feature, target=self.target, num_epochs=10)
        self.stroke_model = _tc.drawing_classifier.create(self.stroke_sf, 
            feature=self.feature, target=self.target, num_epochs=1)

    def test_create_with_missing_feature(self):
        with self.assertRaises(_ToolkitError):
            _tc.drawing_classifier.create(self.random_bitmap_sf, 
                                           feature="wrong_feature")

    def test_create_with_missing_target(self):
        with self.assertRaises(_ToolkitError):
            _tc.drawing_classifier.create(self.random_bitmap_sf, 
                                           feature=self.feature, 
                                           target="wrong_target")

    def test_create_with_empty_dataset(self):
        with self.assertRaises(_ToolkitError):
            _tc.drawing_classifier.create(self.random_bitmap_sf[:0], 
                                           feature=self.feature, 
                                           target=self.target)

    def test_create_with_missing_coordinates_in_stroke_input(self):
        drawing = [[{"x": 1.0, "y": 1.0}], [{"x": 0.0}, {"y": 0.0}]]
        sf = _tc.SFrame({
            "drawing": [drawing], 
            "label": ["missing_coordinates"]
            })
        with self.assertRaises(_ToolkitError):
            _tc.drawing_classifier.create(sf)

    def test_create_with_wrongly_typed_coordinates_in_stroke_input(self):
        drawing = [[{"x": 1.0, "y": 0}], [{"x": "string_x?!", "y": 0.1}]]
        sf = _tc.SFrame({
            "drawing": [drawing], 
            "label": ["string_x_coordinate"]
            })
        with self.assertRaises(_ToolkitError):
            _tc.drawing_classifier.create(sf)

    def test_create_with_None_coordinates_in_stroke_input(self):
        drawing = [[{"x": 1.0, "y": None}], [{"x": 1.1, "y": 0.1}]]
        sf = _tc.SFrame({
            "drawing": [drawing], 
            "label": ["none_y_coordinate"]
            })
        with self.assertRaises(_ToolkitError):
            _tc.drawing_classifier.create(sf, 
                feature = self.feature, target = self.target)

    def test_create_with_empty_drawing_in_stroke_input(self):
        drawing = []
        sf = _tc.SFrame({
            "drawing": [drawing], 
            "label": ["empty_drawing"]
            })
        # Should not error out, it should silently ignore the empty drawing
        _tc.drawing_classifier.create(sf, 
            feature = self.feature, target = self.target, num_epochs=1)


    def test_create_with_empty_stroke_in_stroke_input(self):
        drawing = [[{"x": 1.0, "y": 0.0}], [], [{"x": 1.1, "y": 0.1}]]
        sf = _tc.SFrame({
            "drawing": [drawing], 
            "label": ["empty_drawing"]
            })
        # Should not error out, it should silently ignore the empty stroke
        _tc.drawing_classifier.create(sf, 
            feature = self.feature, target = self.target, num_epochs = 1)

    def test_predict_with_sframe(self):
        preds = self.check_cross_model.predict(self.check_cross_sf)
        assert (preds.dtype == self.check_cross_sf[self.target].dtype)
        assert (preds == self.check_cross_sf[self.target]).all()
        
    def test_predict_with_sarray(self):
        preds = self.check_cross_model.predict(self.check_cross_sf[self.feature])
        assert (preds.dtype == self.check_cross_sf[self.target].dtype)
        assert (preds == self.check_cross_sf[self.target]).all()

    def test_evaluate(self):
        pass

    def test_coreml_export(self):
        pass

    def test_draw_strokes_sframe(self):
        pass

    def test_draw_strokes_single_input(self):
        pass

    def test_export_coreml_with_predict(self):
        pass

    def test_save_and_load(self):
        pass

    def test_repr(self):
        pass

    def test_summary(self):
        pass


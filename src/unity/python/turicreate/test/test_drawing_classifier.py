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
import unittest
import pytest

def _get_data():
    '''
    Create 10 random drawings and build an SFrame.
    '''
    from PIL import Image

    drawings, labels = [], []
    # Create random drawings
    random = _np.random.RandomState(100)
    for label in range(10):
        width, height = random.randint(1000), random.randint(1000)
        cur_data = _np.full((width, height), 0, dtype=_np.uint8)
        for x in range(width):
            for y in range(height):
                cur_data[x][y] = random.randint(255)
        pil_image = Image.fromarray(cur_data, mode='L')
        image_data = bytearray([l for l in pil_image.getdata()])# for z in l ])
        image_data_size = len(image_data)
        drawing = _tc.Image(_image_data = image_data,
                            _width = width, _height = height,
                            _channels = 1, _format_enum = 2,
                            _image_data_size = image_data_size)
        drawings.append(drawing)
        labels.append(label)
    return _tc.SFrame({"drawing": drawings, "label": labels})


class DrawingClassifierTest(unittest.TestCase):
    @classmethod
    def setUpClass(self, pretrained_model_url = None):
        self.feature = "drawing"
        self.target = "label"
        self.sf = _get_data()

    @pytest.mark.xfail(raises = _ToolkitError)
    def test_create_with_missing_feature(self):
        _tc.drawing_recognition.create(self.sf, 
                                      feature="wrong_feature")

    @pytest.mark.xfail(raises = _ToolkitError)
    def test_create_with_missing_target(self):
        _tc.drawing_recognition.create(self.sf, 
                                   feature=self.feature, 
                                   target="wrong_target")

# class DrawingClassifierWithPreTrainedModelTest(DrawingClassifierTest):
#     @classmethod
#     def setUpClass(self):
#         super(DrawingClassifierWithoutPreTrainedModelTest, self).setUpClass()




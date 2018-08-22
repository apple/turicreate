# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
import pytest
from . import util as test_util
import turicreate as tc
import tempfile
import numpy as np
import platform
import sys
import os
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from turicreate.toolkits._internal_utils import _raise_error_if_not_sframe, _mac_ver
import coremltools


_NUM_STYLES = 4


def _get_data(feature, num_examples=100):
    from PIL import Image as _PIL_Image
    rs = np.random.RandomState(1234)
    def from_pil_image(pil_img, image_format='png'):
        # The above didn't work, so as a temporary fix write to temp files
        if image_format == 'raw':
            image = np.array(pil_img)
            FORMAT_RAW = 2
            return tc.Image(_image_data=image.tobytes(),
                            _width=image.shape[1],
                            _height=image.shape[0],
                            _channels=image.shape[2],
                            _format_enum=FORMAT_RAW,
                            _image_data_size=image.size)
        else:
            with tempfile.NamedTemporaryFile(mode='w+b', suffix='.' + image_format) as f:
                pil_img.save(f, format=image_format)
                return tc.Image(f.name)

    images = []
    FORMATS = ['png', 'jpeg', 'raw']
    for i in range(num_examples):
        # Randomly determine image size (should handle large and small)
        img_shape = tuple(rs.randint(100, 600, size=2)) + (3,)
        img = rs.randint(255, size=img_shape)

        pil_img = _PIL_Image.fromarray(img, mode='RGB')
        # Randomly select image format
        image_format = FORMATS[rs.randint(len(FORMATS))]
        images.append(from_pil_image(pil_img, image_format=image_format))

    data = tc.SFrame({
            feature: tc.SArray(images),
    })
    return data


class StyleTransferTest(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        """
        The setup class method for the basic test case with all default values.
        """
        self.style_feature = 'style_feature_name'
        self.content_feature = 'content_feature_name'
        self.pre_trained_model = 'resnet-16'
        ## Create the model
        # Model
        self.style_sf = _get_data(feature=self.style_feature, num_examples=_NUM_STYLES)
        self.content_sf = _get_data(feature=self.content_feature)
        self.num_styles = _NUM_STYLES
        self.model = tc.style_transfer.create(self.style_sf,
                                              self.content_sf,
                                              style_feature=self.style_feature,
                                              content_feature=self.content_feature,
                                              max_iterations=0,
                                              model=self.pre_trained_model)


    @pytest.mark.xfail(raises=_ToolkitError)
    def test_create_with_missing_style_feature(self):
        tc.style_transfer.create(self.style_sf, self.content_sf, style_feature='wrong_feature',
                                 max_iterations=0)

    @pytest.mark.xfail(raises=_ToolkitError)
    def test_create_with_missing_content_feature(self):
        tc.style_transfer.create(self.style_sf, self.content_sf, content_feature='wrong_feature',
                                 max_iterations=0)

    @pytest.mark.xfail(raises=_ToolkitError)
    def test_create_with_empty_style_dataset(self):
        tc.style_transfer.create(self.style_sf[:0], self.content_sf, max_iterations=0)

    @pytest.mark.xfail(raises=_ToolkitError)
    def test_create_with_empty_content_dataset(self):
        tc.style_transfer.create(self.style_sf, self.content_sf[:0], max_iterations=0)

    def _get_invalid_style_cases(self):
        style_cases = []
        # when style is an empty list
        style_cases.append([])
        # when style is a list, but index is not within the number of styles
        style_cases.append([self.num_styles + 10])
        # when style is a int, but index is not within the number of styles
        style_cases.append(self.num_styles + 10)
        # when style is a str, and is empty
        style_cases.append('')
        # when style is a str, and is not within label names
        style_cases.append('style_image_404')

        return style_cases

    def _get_valid_style_cases(self):
        style_cases = []
        # when style is None
        style_cases.append(None)
        # when style is a list containing single value
        style_cases.append([0])
        # when style is a list containing multiple values
        style_cases.append([0, 1, 2])
        # when style is a int
        style_cases.append(0)

        return style_cases

    def test_stylize_fail(self):
        style_cases = self._get_invalid_style_cases()
        model = self.model
        for style in style_cases:
            with self.assertRaises(_ToolkitError):
                model.stylize(self.content_sf[0:1], style=style)

    def test_stylize_success(self):
        sf = self.content_sf[0:1]
        model = self.model
        styles = self._get_valid_style_cases()

        for style in styles:
            stylized_out = model.stylize(sf, style=style)

            # Check the structure of the output
            _raise_error_if_not_sframe(stylized_out)
            if style is None:
                num_styles = self.num_styles
            elif isinstance(style, list):
                num_styles = len(style)
            else:
                num_styles = 1
            self.assertEqual(len(stylized_out), len(sf)*num_styles)

            # Check if input and output image have the same shape
            input_size = (sf[self.content_feature][0].width, sf[self.content_feature][0].height)
            feat_name = 'stylized_{}'.format(self.content_feature)
            output_size = (stylized_out[0][feat_name].width,
                           stylized_out[0][feat_name].height)
            self.assertEqual(input_size, output_size)

    def test_single_image(self):
        img = self.model.stylize(self.content_sf[self.content_feature][0], style=0)
        self.assertTrue(isinstance(img, tc.Image))

        # If style is a list, it should however return an SFrame
        sf = self.model.stylize(self.content_sf[self.content_feature][0], style=[0])
        self.assertTrue(isinstance(sf, tc.SFrame))
        self.assertEqual(len(sf), 1)

    def test_sarray(self):
        sarray = self.content_sf[self.content_feature][:2]
        imgs = self.model.stylize(sarray, style=0)
        self.assertTrue(isinstance(imgs, tc.SArray))
        self.assertEqual(len(imgs), len(sarray))

    def test_get_styles_fail(self):
        style_cases = self._get_invalid_style_cases()
        model = self.model
        for style in style_cases:
            with self.assertRaises(_ToolkitError):
                model.get_styles(style=style)

    def test_get_styles_success(self):

        style = [0,1,2]
        model = self.model
        model_styles = model.get_styles(style=style)

        # Check the structure of the output
        _raise_error_if_not_sframe(model_styles)
        self.assertEqual(len(model_styles), len(style))

    def _coreml_python_predict(self, coreml_model, img_fixed):
        from PIL import Image
        pil_img = Image.fromarray(img_fixed.pixel_data)
        if _mac_ver() >= (10, 13):
            index_data = np.zeros(self.num_styles)
            index_data[0] = 1
            coreml_output = coreml_model.predict(
                {self.content_feature: pil_img, 'index': index_data},
                usesCPUOnly=True)
            img = next(iter(coreml_output.values()))
            img = np.asarray(img)
            img = img[..., 0:3]
            return img

    def test_export_coreml(self):
        import coremltools
        model = self.model
        for flexible_shape_on in [True, False]:
            filename = tempfile.mkstemp('my_style_transfer.mlmodel')[1]
            model.export_coreml(filename, 
                include_flexible_shape = flexible_shape_on)
            if not flexible_shape_on or _mac_ver() >= (10,14):
                coreml_model = coremltools.models.MLModel(filename)

                mac_os_version_threshold = (10,14) if flexible_shape_on else (10,13)

                if _mac_ver() >= mac_os_version_threshold:
                    img = self.style_sf[0:2][self.style_feature][0]
                    img_fixed = tc.image_analysis.resize(img, 256, 256, 3)
                    img = self._coreml_python_predict(coreml_model, img_fixed)
                    self.assertEqual(img.shape, (256, 256, 3))

                    if flexible_shape_on:
                        # Test for flexible shape
                        img = self.style_sf[0:2][self.style_feature][1]
                        img_fixed = tc.image_analysis.resize(img, 512, 512, 3)
                        img = self._coreml_python_predict(coreml_model, img_fixed)
                        self.assertEqual(img.shape, (512, 512, 3))

                # Also check if we can train a second model and export it (there could
                # be naming issues in mxnet)
                filename2 = tempfile.mkstemp('my_style_transfer2.mlmodel')[1]
                # We also test at the same time if we can export a model with a single
                # class

                model2 = tc.style_transfer.create(self.style_sf, self.content_sf, max_iterations=1)
                model2.export_coreml(filename2)

    def test_repr(self):
        model = self.model
        self.assertEqual(type(str(model)), str)
        self.assertEqual(type(model.__repr__()), str)

    def test_save_and_load(self):
        with test_util.TempDirectory() as filename:
            self.model.save(filename)
            self.model = tc.load_model(filename)

            self.test_stylize_success()
            print("Stylize passed")
            self.test_get_styles_success()
            print("Get styles passed")


@unittest.skipIf(tc.util._num_available_cuda_gpus() == 0, 'Requires CUDA GPU')
@pytest.mark.gpu
class StyleTransferGPUTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.style_feature = self.content_feature = 'image'
        self.style_sf = _get_data(feature=self.style_feature)
        self.content_sf = _get_data(feature=self.content_feature)


    def test_gpu_save_load_export(self):
        old_num_gpus = tc.config.get_num_gpus()
        gpu_options = set([old_num_gpus, 0, 1])
        for in_gpus in gpu_options:
            for out_gpus in gpu_options:
                tc.config.set_num_gpus(in_gpus)
                model = tc.style_transfer.create(self.style_sf, self.content_sf, max_iterations=1)
                with test_util.TempDirectory() as path:
                    model.save(path)
                    tc.config.set_num_gpus(out_gpus)
                    model = tc.load_model(path)
                    model.export_coreml(os.path.join(path, 'model.mlmodel'))

        tc.config.set_num_gpus(old_num_gpus)

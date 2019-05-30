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
from turicreate.toolkits._internal_utils import _raise_error_if_not_sarray, _mac_ver
import coremltools

_CLASSES = ['logo_a', 'logo_b', 'logo_c', 'logo_d']

def _get_data(feature, annotations, target):
    from PIL import Image as _PIL_Image
    rs = np.random.RandomState(1234)
    def from_pil_image(pil_img, image_format='png'):
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

    num_examples = 100
    num_starter_images = 5
    max_num_boxes_per_image = 10
    classes = _CLASSES
    images = []
    anns = []
    FORMATS = ['png', 'jpeg', 'raw']
    for i in range(num_examples):
        # Randomly determine image size (should handle large and small)
        img_shape = tuple(rs.randint(100, 1000, size=2)) + (3,)
        img = rs.randint(255, size=img_shape)

        pil_img = _PIL_Image.fromarray(img, mode='RGB')
        # Randomly select image format
        image_format = FORMATS[rs.randint(len(FORMATS))]
        images.append(from_pil_image(pil_img, image_format=image_format))

        ann = []
        for j in range(rs.randint(max_num_boxes_per_image)):
            left, right = np.sort(rs.randint(0, img_shape[1], size=2))
            top, bottom = np.sort(rs.randint(0, img_shape[0], size=2))

            x = (left + right) / 2
            y = (top + bottom) / 2

            width = max(right - left, 1)
            height = max(bottom - top, 1)

            label = {
                'coordinates': {
                    'x': x,
                    'y': y,
                    'width': width,
                    'height': height,
                },
                'label': classes[rs.randint(len(classes))],
                'type': 'rectangle',
            }
            ann.append(label)
        anns.append(ann)

    starter_images = []
    starter_target = []
    for i in range(num_starter_images):
        img_shape = tuple(rs.randint(100, 1000, size=2)) + (3,)
        img = rs.randint(255, size=img_shape)
        pil_img = _PIL_Image.fromarray(img, mode='RGB')
        image_format = FORMATS[rs.randint(len(FORMATS))]
        starter_images.append(from_pil_image(pil_img, image_format=image_format))
        starter_target.append(i % len(_CLASSES))

    train = tc.SFrame({
        feature: tc.SArray(starter_images),
        target: tc.SArray(starter_target),
    })
    test = tc.SFrame({
        feature: tc.SArray(images),
        annotations: tc.SArray(anns),
    })
    backgrounds = test[feature].head(5)
    return train, test, backgrounds


class OneObjectDetectorSmokeTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        """
        The setup class method for the basic test case with all default values.
        """
        self.feature = 'myimage'
        self.target = 'mytarget'
        self.annotations = 'myannotations'

        ## Create the model
        self.def_opts = {
           'model': 'darknet-yolo',
           'max_iterations': 2,
        }

        # Model
        self.train, self.test, self.backgrounds = _get_data(feature=self.feature, target = self.target, annotations=self.annotations)
        self.model = tc.one_shot_object_detector.create(self.train,
                                                        target=self.target,
                                                        backgrounds=self.backgrounds,
                                                        batch_size=2,
                                                        max_iterations=1)

        ## Answers
        self.opts = self.def_opts.copy()
        self.opts['max_iterations'] = 1

        self.get_ans = {
           'classes': lambda x: x == sorted(_CLASSES),
           'target': lambda x: x == self.feature,
           'num_starter_images': lambda x: x > 0,
           'num_classes': lambda x: x == len(_CLASSES),
        }
        self.fields_ans = self.get_ans.keys()

    @pytest.mark.xfail(raises = _ToolkitError)
    def test_create_with_missing_target(self):
        tc.one_shot_object_detector.create(self.train, target='wrong_feature')

    @pytest.mark.xfail(raises = _ToolkitError)
    def test_create_with_empty_dataset(self):
        tc.one_shot_object_detector.create(self.train[:0])

    def test_predict(self):
        sf = self.test.head()
        # Make sure this does not need the annotations column to work
        del sf[self.annotations]

        pred = self.model.predict(sf.head())

        # Check the structure of the output
        _raise_error_if_not_sarray(pred)
        self.assertEqual(len(pred), len(sf))

        # Make sure SFrame was not altered
        self.assertEqual([col for col in sf.column_names() if col.startswith('_')],
                         [])

        # Predict should work on no input (and produce no predictions)
        pred0 = self.model.predict(sf[:0])
        self.assertEqual(len(pred0), 0)

    def test_predict_single_image(self):
        pred = self.model.predict(self.test[self.feature][0], confidence_threshold=0)
        self.assertTrue(isinstance(pred, list))
        self.assertTrue(isinstance(pred[0], dict))

    def test_predict_sarray(self):
        sarray = self.test.head()[self.feature]
        pred = self.model.predict(sarray, confidence_threshold=0)
        self.assertEqual(len(pred), len(sarray))

    def test_predict_confidence_threshold(self):
        sf = self.test.head()
        pred = self.model.predict(sf.head(), confidence_threshold=1.0)
        stacked = tc.object_detector.util.stack_annotations(pred)
        # This should return no predictions, especially with an unconverged model
        self.assertEqual(len(stacked), 0)

        pred = self.model.predict(sf.head(), confidence_threshold=0.0)
        stacked = tc.object_detector.util.stack_annotations(pred)
        # This should return predictions
        self.assertTrue(len(stacked) > 0)

    def test_evaluate(self):
        ret = self.model.evaluate(self.test.head(), metric='average_precision')

        self.assertTrue(set(ret), {'average_precision'})
        self.assertEqual(set(ret['average_precision'].keys()), set(_CLASSES))

        ret = self.model.evaluate(self.sf.head())
        self.assertEqual(set(ret), {'mean_average_precision_50', 'average_precision_50'})
        self.assertTrue(isinstance(ret['mean_average_precision_50'], float))
        self.assertEqual(set(ret['average_precision_50'].keys()), set(_CLASSES))

        # Empty dataset should not fail with error (although it should to 0
        # metrics)
        ret = self.model.evaluate(self.sf[:0])
        self.assertEqual(ret['mean_average_precision_50'], 0.0)

    @pytest.mark.xfail(raises = _ToolkitError)
    def test_evaluate_invalid_metric(self):
        self.model.evaluate(self.test.head(), metric='not-supported-metric', annotations = self.annotations)

    @pytest.mark.xfail(raises = _ToolkitError)
    def test_evaluate_invalid_format(self):
        self.model.evaluate(self.test.head(), output_type='not-supported-format', annotations = self.annotations)

    @pytest.mark.xfail(raises = _ToolkitError)
    def test_evaluate_missing_annotations_param(self):
        self.model.evaluate(self.test.head())

    @pytest.mark.xfail(raises = _ToolkitError)
    def test_evaluate_missing_annotations(self):
        sf = self.test.copy()
        del sf[self.annotations]
        self.model.evaluate(sf.head(), annotations = self.annotations)

    def test_export_coreml(self):
        from PIL import Image
        import coremltools
        filename = tempfile.mkstemp('bingo.mlmodel')[1]
        self.model.export_coreml(filename, 
            include_non_maximum_suppression=False)

        coreml_model = coremltools.models.MLModel(filename)
        img = self.sf[0:1][self.feature][0]
        img_fixed = tc.image_analysis.resize(img, 416, 416, 3)
        pil_img = Image.fromarray(img_fixed.pixel_data)
        if _mac_ver() >= (10, 13):
            ret = coreml_model.predict({self.feature: pil_img}, 
                usesCPUOnly = True)
            self.assertEqual(ret['coordinates'].shape[1], 4)
            self.assertEqual(ret['confidence'].shape[1], len(_CLASSES))
            self.assertEqual(ret['coordinates'].shape[0], 
                ret['confidence'].shape[0])
            # A numeric comparison of the resulting of top bounding boxes is
            # not that meaningful unless the model has converged

        # Also check if we can train a second model and export it (there could
        # be naming issues in mxnet)
        filename2 = tempfile.mkstemp('bingo2.mlmodel')[1]
        # We also test at the same time if we can export a model with a single
        # class
        sf = tc.SFrame({'image': [self.sf[self.feature][0]],
                        'ann': [self.sf[self.annotations][0][:1]]})
        model2 = tc.object_detector.create(sf, max_iterations=1)
        model2.export_coreml(filename2, 
            include_non_maximum_suppression=False)

    def test__list_fields(self):
        model = self.model
        fields = model._list_fields()
        self.assertEqual(set(fields), set(self.fields_ans))

    def test_get(self):
        model = self.model
        for field in self.fields_ans:
            ans = model._get(field)
            self.assertTrue(self.get_ans[field](ans),
                    '''Get failed in field {}. Output was {}.'''.format(field, ans))

    def test_summary(self):
        model = self.model
        model.summary()

    def test_repr(self):
        # Repr after fit
        model = self.model
        self.assertEqual(type(str(model)), str)
        self.assertEqual(type(model.__repr__()), str)

    def test_save_and_load(self):
        with test_util.TempDirectory() as filename:
            self.model.save(filename)
            self.model = tc.load_model(filename)

            self.test_predict()
            print("Predict passed")
            self.test_get()
            print("Get passed")
            self.test_summary()
            print("Summary passed")
            self.test__list_fields()
            print("List fields passed")

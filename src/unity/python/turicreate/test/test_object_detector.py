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

_CLASSES = ['person', 'cat', 'dog', 'chair']

def _get_data(feature, annotations):
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

    num_examples = 100
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

            width = right - left
            height = bottom - top

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

    data = tc.SFrame({
        feature: tc.SArray(images),
        annotations: tc.SArray(anns),
    })
    return data


class ObjectDetectorTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        """
        The setup class method for the basic test case with all default values.
        """
        self.feature = 'myimage'
        self.annotations = 'myannotations'
        self.pre_trained_model = 'darknet-yolo'
        ## Create the model
        self.def_opts = {
           'model': 'darknet-yolo',
           'max_iterations': 0,
        }

        # Model
        self.sf = _get_data(feature=self.feature, annotations=self.annotations)
        params = {
            'batch_size': 2,
        }
        self.model = tc.object_detector.create(self.sf,
                                               feature=self.feature,
                                               annotations=self.annotations,
                                               max_iterations=1,
                                               model=self.pre_trained_model,
                                               _advanced_parameters=params)

        ## Answers
        self.opts = self.def_opts.copy()
        self.opts['max_iterations'] = 1

        self.get_ans = {
           '_model': lambda x: True,
           '_class_to_index': lambda x: isinstance(x, dict),
           '_training_time_as_string': lambda x: isinstance(x, str),
           '_grid_shape': lambda x: tuple(x) == (13, 13),
           'model': lambda x: x == self.pre_trained_model,
           'anchors': lambda x: (isinstance(x, (list, tuple, np.ndarray)) and
                                  len(x) > 0 and len(x[0]) == 2),
           'input_image_shape': lambda x: tuple(x) == (3, 416, 416),
           'batch_size': lambda x: x == 2,
           'classes': lambda x: x == sorted(_CLASSES),
           'feature': lambda x: x == self.feature,
           'max_iterations': lambda x: x >= 0,
           'non_maximum_suppression_threshold': lambda x: 0 <= x <= 1,
           'training_time': lambda x: x > 0,
           'training_iterations': lambda x: x > 0,
           'training_epochs': lambda x: x >= 0,
           'num_bounding_boxes': lambda x: x > 0,
           'num_examples': lambda x: x > 0,
           'training_loss': lambda x: x > 0,
           'annotations': lambda x: x == self.annotations,
           'num_classes': lambda x: x == len(_CLASSES),
        }
        self.fields_ans = self.get_ans.keys()

    @pytest.mark.xfail(rases = _ToolkitError)
    def test_create_with_missing_feature(self):
        tc.object_detector.create(self.sf, feature='wrong_feature', annotations=self.annotations)

    @pytest.mark.xfail(rases = _ToolkitError)
    def test_create_with_missing_annotations(self):
        tc.object_detector.create(self.sf, feature=self.feature, annotations='wrong_annotations')

    @pytest.mark.xfail(rases = _ToolkitError)
    def test_create_with_invalid_annotations_list_coord(self):
        sf = self.sf.head()
        sf[self.annotations] = sf[self.annotations].apply(
                lambda x: [{'label': _CLASSES[0], 'coordinates': [100, 50, 20, 40]}])

        tc.object_detector.create(sf)

    @pytest.mark.xfail(rases = _ToolkitError)
    def test_create_with_invalid_annotations_not_dict(self):
        sf = self.sf.head()
        sf[self.annotations] = sf[self.annotations].apply(
                lambda x: [1])

        tc.object_detector.create(sf)

    @pytest.mark.xfail(rases = _ToolkitError)
    def test_create_with_empty_dataset(self):
        tc.object_detector.create(self.sf[:0])

    def test_invalid_num_gpus(self):
        num_gpus = tc.config.get_num_gpus()
        tc.config.set_num_gpus(-2)
        with self.assertRaises(_ToolkitError):
            tc.object_detector.create(self.sf)
        tc.config.set_num_gpus(num_gpus)

    def test_extra_classes(self):
        # Create while the data has extra classes
        model = tc.object_detector.create(self.sf, classes=_CLASSES[:2], max_iterations=1)
        self.assertEqual(len(model.classes), 2)

        # Evaluate while the data has extra classes
        ret = model.evaluate(self.sf.head())
        self.assertEqual(len(ret['average_precision']), 2)

    def test_predict(self):
        sf = self.sf.head()
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

    def test_confidence_threshold(self):
        sf = self.sf.head()
        pred = self.model.predict(sf.head(), confidence_threshold=1.0)
        stacked = tc.object_detector.util.stack_annotations(pred)
        # This should return no predictions, especially with an unconverged model
        self.assertEqual(len(stacked), 0)

        pred = self.model.predict(sf.head(), confidence_threshold=0.0)
        stacked = tc.object_detector.util.stack_annotations(pred)
        # This should return predictions
        self.assertTrue(len(stacked) > 0)

    def test_evaluate(self):
        ret = self.model.evaluate(self.sf.head(), metric='average_precision')

        self.assertTrue(set(ret), {'average_precision'})
        self.assertEqual(set(ret['average_precision'].keys()), set(_CLASSES))

        ret = self.model.evaluate(self.sf.head())

        self.assertTrue(set(ret), {'average_precision', 'mean_average_precision'})
        self.assertTrue(isinstance(ret['mean_average_precision'], float))

        # Empty dataset should not fail with error (although it should to 0
        # metrics)
        ret = self.model.evaluate(self.sf[:0])
        self.assertEqual(ret['mean_average_precision'], 0.0)

    @pytest.mark.xfail(rases = _ToolkitError)
    def test_evaluate_invalid_metric(self):
        self.model.evaluate(self.sf.head(), metric='not-supported-metric')

    @pytest.mark.xfail(rases = _ToolkitError)
    def test_evaluate_invalid_format(self):
        self.model.evaluate(self.sf.head(), output_type='not-supported-format')

    @pytest.mark.xfail(rases = _ToolkitError)
    def test_evaluate_missing_annotations(self):
        sf = self.sf.copy()
        del sf[self.annotations]
        self.model.evaluate(sf.head())

    def test_export_coreml(self):
        from PIL import Image
        import coremltools
        filename = tempfile.mkstemp('bingo.mlmodel')[1]
        self.model.export_coreml(filename)

        coreml_model = coremltools.models.MLModel(filename)
        img = self.sf[0:1][self.feature][0]
        img_fixed = tc.image_analysis.resize(img, 416, 416, 3)
        pil_img = Image.fromarray(img_fixed.pixel_data)
        if _mac_ver() >= (10, 13):
            ret = coreml_model.predict({self.feature: pil_img}, usesCPUOnly = True)
            self.assertEqual(ret['coordinates'].shape[1], 4)
            self.assertEqual(ret['confidence'].shape[1], len(_CLASSES))
            self.assertEqual(ret['coordinates'].shape[0], ret['confidence'].shape[0])
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
        model2.export_coreml(filename2)

    @unittest.skipIf(sys.platform != 'darwin', 'Only supported on Mac')
    def test_no_gpu_mac_support(self):
        num_gpus = tc.config.get_num_gpus()
        tc.config.set_num_gpus(1)
        with self.assertRaises(_ToolkitError):
            tc.object_detector.create(self.sf)
        tc.config.set_num_gpus(num_gpus)

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


@unittest.skipIf(tc.util._num_available_gpus() == 0, 'Requires GPU')
@pytest.mark.gpu
class ObjectDetectorGPUTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.feature = 'myimage'
        self.annotations = 'myannotations'
        self.sf = _get_data(feature=self.feature, annotations=self.annotations)

    def test_gpu_save_load_export(self):
        old_num_gpus = tc.config.get_num_gpus()
        gpu_options = set([old_num_gpus, 0, 1])
        for in_gpus in gpu_options:
            for out_gpus in gpu_options:
                tc.config.set_num_gpus(in_gpus)
                model = tc.object_detector.create(self.sf, max_iterations=1)
                with test_util.TempDirectory() as path:
                    model.save(path)
                    tc.config.set_num_gpus(out_gpus)
                    model = tc.load_model(path)
                    model.export_coreml(os.path.join(path, 'model.mlmodel'))

        tc.config.set_num_gpus(old_num_gpus)

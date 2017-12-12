# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import os
import sys
import unittest
import turicreate as tc
from turicreate.toolkits import _image_feature_extractor
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from turicreate.toolkits._internal_utils import _mac_ver
import tempfile
from . import util as test_util
import pytest
import coremltools
import numpy as np
import platform

def _get_data():
    from PIL import Image as _PIL_Image
    rs = np.random.RandomState(1234)
    _format = {'JPG': 0, 'PNG': 1, 'RAW': 2, 'UNDEFINED': 3}

    def from_pil_image(pil_img):
        height = pil_img.size[1]
        width = pil_img.size[0]
        if pil_img.mode == 'L':
            image_data = bytearray([z for z in pil_img.getdata()])
            channels = 1
        elif pil_img.mode == 'RGB':
            image_data = bytearray([z for l in pil_img.getdata() for z in l ])
            channels = 3
        else:
            image_data = bytearray([z for l in pil_img.getdata() for z in l])
            channels = 4
        format_enum = _format['RAW']
        image_data_size = len(image_data)
        img = tc.Image(_image_data=image_data,
                _width=width, _height=height,
                _channels=channels,
                _format_enum=format_enum,
                _image_data_size=image_data_size)
        return img

    num_examples = 100
    images = []
    random_labels = [rs.randint(0,5) for i in range(num_examples)]
    for i in range(num_examples):
        img_shape = tuple(rs.randint(100, 1000, size=2)) + (3,)
        img = rs.randint(255, size=img_shape)
        label = random_labels[i]
        # Give a slight color hint about the label
        img = (img + [label * 3, 0, -label * 3]).clip(0, 255)
        pil_img = _PIL_Image.fromarray(img, mode='RGB')
        images.append(from_pil_image(pil_img))
    data = tc.SFrame({'awesome_image': tc.SArray(images)})
    data['awesome_label'] = random_labels
    return data


class ImageClassifierTest(unittest.TestCase):
    @classmethod
    def setUpClass(self, model='resnet-50', input_image_shape=(3, 224, 224), tol=0.02):
        self.feature = 'awesome_image'
        self.target = 'awesome_label'
        self.input_image_shape = input_image_shape
        self.pre_trained_model = model
        self.tolerance = tol

        self.sf = _get_data()
        self.model = tc.image_classifier.create(self.sf, target=self.target,
                                                model=self.pre_trained_model,
                                                seed=42)
        self.nn_model = self.model.feature_extractor
        self.lm_model = self.model.classifier
        self.max_iterations = 10

        self.get_ans = {
           'classifier' : lambda x: type(x) == \
                        tc.logistic_classifier.LogisticClassifier,
           'feature': lambda x: x == self.feature,
           'classes': lambda x: x == self.lm_model.classes,
           'training_time': lambda x: x > 0,
           'input_image_shape': lambda x: x == self.input_image_shape,
           'target': lambda x: x == self.target,
           'feature_extractor' : lambda x: issubclass(type(x),
                 _image_feature_extractor.ImageFeatureExtractor),
           'training_loss': lambda x: x > 0,
           'max_iterations': lambda x: x == self.max_iterations,
           'num_features': lambda x: x == self.lm_model.num_features,
           'num_examples': lambda x: x == self.lm_model.num_examples,
           'model': lambda x: x == self.pre_trained_model,
           'num_classes': lambda x: x == self.lm_model.num_classes,
        }
        self.fields_ans = self.get_ans.keys()

    def assertListAlmostEquals(self, list1, list2, tol):
        self.assertEqual(len(list1), len(list2))
        for a, b in zip(list1, list2):
             self.assertAlmostEqual(a, b, delta = tol)

    @pytest.mark.xfail(rases = _ToolkitError)
    def test_create_with_missing_feature(self):
        tc.image_classifier.create(self.sf, feature='wrong_feature', target=self.target)

    @pytest.mark.xfail(rases = RuntimeError)
    def test_create_with_missing_label(self):
        tc.image_classifier.create(self.sf, feature=self.feature, target='wrong_annotations')

    @pytest.mark.xfail(rases = _ToolkitError)
    def test_create_with_empty_dataset(self):
        tc.image_classifier.create(self.sf[:0], target = self.target)

    def test_invalid_num_gpus(self):
        num_gpus = tc.config.get_num_gpus()
        tc.config.set_num_gpus(-2)
        with self.assertRaises(_ToolkitError):
            tc.image_classifier.create(self.sf, target=self.target)
        tc.config.set_num_gpus(num_gpus)

    def test_predict(self):
        model = self.model
        for output_type in ['score', 'probability', 'class']:
            preds = model.predict(self.sf.head())
            self.assertEqual(len(preds), len(self.sf.head()))

    def test_export_coreml(self):
        filename = tempfile.mkstemp('bingo.mlmodel')[1]
        self.model.export_coreml(filename)

    @unittest.skipIf(sys.platform != 'darwin', 'Only supported on Mac')
    def test_export_coreml_with_predict(self):
        filename = tempfile.mkstemp('bingo.mlmodel')[1]
        self.model.export_coreml(filename)

        coreml_model = coremltools.models.MLModel(filename)
        img = self.sf[0:1][self.feature][0]
        img_fixed = tc.image_analysis.resize(img, *reversed(self.input_image_shape))
        import PIL
        pil_img = PIL.Image.fromarray(img_fixed.pixel_data)

        if _mac_ver() >= (10, 13):
            classes = self.model.classifier.classes
            ret = coreml_model.predict({self.feature: pil_img}, usesCPUOnly = True)
            coreml_values = [ret[self.target + 'Probability'][l] for l in classes]

            self.assertListAlmostEquals(
               coreml_values,
               list(self.model.predict(self.sf[0:1], output_type = 'probability_vector')[0]),
               self.tolerance
            )

    def test_classify(self):
        model = self.model
        preds = model.classify(self.sf.head())
        self.assertEqual(len(preds), len(self.sf.head()))

    def test_predict_topk(self):
        model = self.model
        for output_type in ['score', 'probability', 'class']:
            preds = model.predict_topk(self.sf.head())
            self.assertEqual(len(preds), 3 * len(self.sf.head()))

            preds = model.predict_topk(self.sf.head(), k = 5)
            self.assertEqual(len(preds), 5 * len(self.sf.head()))

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
            self.test_predict_topk()
            print("Predict topk passed")
            self.test_classify()
            print("Classify passed")
            self.test_get()
            print("Get passed")
            self.test_summary()
            print("Summary passed")
            self.test__list_fields()
            print("List fields passed")


class ImageClassifierSqueezeNetTest(ImageClassifierTest):
    @classmethod
    def setUpClass(self):
        super(ImageClassifierSqueezeNetTest, self).setUpClass(model='squeezenet_v1.1',
                                                              input_image_shape=(3, 227, 227),
                                                              tol=0.005)


@unittest.skipIf(tc.util._num_available_gpus() == 0, 'Requires GPU')
@pytest.mark.gpu
class ImageClassifierGPUTest(unittest.TestCase):
    @classmethod
    def setUpClass(self, model='resnet-50', tol=0.005):
        self.feature = 'awesome_image'
        self.target = 'awesome_label'
        self.input_image_shape = (3, 224, 224)
        self.pre_trained_model = model
        self.tolerance = tol
        self.sf = _get_data()

    def test_gpu_save_load_export(self):
        old_num_gpus = tc.config.get_num_gpus()
        gpu_options = set([old_num_gpus, 0, 1])
        for in_gpus in gpu_options:
            for out_gpus in gpu_options:
                tc.config.set_num_gpus(in_gpus)
                model = tc.image_classifier.create(self.sf, target=self.target,
                                                   model=self.pre_trained_model)
                with test_util.TempDirectory() as path:
                    model.save(path)
                    tc.config.set_num_gpus(out_gpus)
                    model = tc.load_model(path)
                    model.export_coreml(os.path.join(path, 'model.mlmodel'))

        tc.config.set_num_gpus(old_num_gpus)


@unittest.skipIf(tc.util._num_available_gpus() == 0, 'Requires GPU')
@pytest.mark.gpu
class ImageClassifierSqueezeNetGPUTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(ImageClassifierSqueezeNetGPUTest, self).setUpClass(model='squeezenet_v1.1')

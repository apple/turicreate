# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import sys
import unittest
import pytest
import turicreate as tc
from turicreate.toolkits import _image_feature_extractor
import tempfile
from . import util as test_util
import coremltools
import numpy
import platform
from turicreate.toolkits._main import ToolkitError as _ToolkitError

def _get_data():
    from PIL import Image as _PIL_Image
    import random
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
    dims = (224, 224)
    total_dims = dims[0] * dims[1]
    images = []
    for i in range(num_examples):
        def rand_image():
            return [random.randint(0,255)] * total_dims

        pil_img = _PIL_Image.new('RGB', dims)
        pil_img.putdata(list(zip(rand_image(), rand_image(), rand_image())))
        images.append(from_pil_image(pil_img))
        random_labels = [random.randint(0,5) for i in range(num_examples)]

    data = tc.SFrame({'awesome_image': tc.SArray(images)})
    return data


class ImageSimilarityTest(unittest.TestCase):

    @classmethod
    def setUpClass(self, model = 'resnet-50'):
        """
        The setup class method for the basic test case with all default values.
        """
        self.feature = 'awesome_image'
        self.label = None
        self.input_image_shape = (3, 224, 224)
        self.pre_trained_model = model

        ## Create the model
        self.def_opts= {
           'model': 'resnet-50',
           'verbose': True,
        }

        # Model
        self.sf = _get_data()
        self.model = tc.image_similarity.create(self.sf, feature=self.feature,
                                                label=None, model=self.pre_trained_model)
        self.nn_model = self.model.feature_extractor
        self.lm_model = self.model.similarity_model
        self.opts = self.def_opts.copy()

        # Answers
        self.get_ans = {
           'similarity_model' : lambda x: type(x) == \
                        tc.nearest_neighbors.NearestNeighborsModel,
           'feature': lambda x: x == self.feature,
           'training_time': lambda x: x > 0,
           'input_image_shape': lambda x: x == self.input_image_shape,
           'label': lambda x: x == self.label,
           'feature_extractor' : lambda x: issubclass(type(x),
                 _image_feature_extractor.ImageFeatureExtractor),
           'num_features': lambda x: x == self.lm_model.num_features,
           'num_examples': lambda x: x == self.lm_model.num_examples,
           'model': lambda x: x == self.pre_trained_model,
        }
        self.fields_ans = self.get_ans.keys()

    def assertListAlmostEquals(self, list1, list2, tol):
        self.assertEqual(len(list1), len(list2))
        for a, b in zip(list1, list2):
             self.assertAlmostEqual(a, b, delta = tol)

    @pytest.mark.xfail(rases = _ToolkitError)
    def test_create_with_missing_feature(self):
        tc.image_similarity.create(self.sf, feature='wrong_feature', label=self.label)

    @pytest.mark.xfail(rases = _ToolkitError)
    def test_create_with_missing_label(self):
        tc.image_similarity.create(self.sf, feature=self.feature, label='wrong_label')

    @pytest.mark.xfail(rases = _ToolkitError)
    def test_create_with_empty_dataset(self):
        tc.image_similarity.create(self.sf[:0])

    def test_invalid_num_gpus(self):
        num_gpus = tc.config.get_num_gpus()
        tc.config.set_num_gpus(-2)
        with self.assertRaises(_ToolkitError):
            tc.image_similarity.create(self.sf)
        tc.config.set_num_gpus(num_gpus)

    def test_query(self):
        model = self.model
        preds = model.query(self.sf)
        self.assertEqual(len(preds), len(self.sf) * 5)

    def test_similarity_graph(self):
        model = self.model
        preds = model.similarity_graph()
        self.assertEqual(len(preds.edges), len(self.sf) * 5)

        preds = model.similarity_graph(output_type = 'SFrame')
        self.assertEqual(len(preds), len(self.sf) * 5)

    def test__list_fields(self):
        model = self.model
        fields = model._list_fields()
        self.assertEqual(set(fields), set(self.fields_ans))

    def test_get(self):
        """
        Check the get function. Compare with the answer supplied as a lambda
        function for each field.
        """
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

            self.test_query()
            print("Query passed")
            self.test_similarity_graph()
            print("Similarity graph passed")
            self.test_get()
            print("Get passed")
            self.test_summary()
            print("Summary passed")
            self.test__list_fields()
            print("List fields passed")


@unittest.skipIf(tc.util._num_available_gpus() == 0, 'Requires GPU')
@pytest.mark.gpu
class ImageSimilarityGPUTest(unittest.TestCase):
    @classmethod
    def setUpClass(self, model='resnet-50'):
        self.feature = 'awesome_image'
        self.label = None
        self.input_image_shape = (3, 224, 224)
        self.pre_trained_model = model
        self.sf = _get_data()

    def test_gpu_save_load_export(self):
        old_num_gpus = tc.config.get_num_gpus()
        gpu_options = set([old_num_gpus, 0, 1])
        for in_gpus in gpu_options:
            for out_gpus in gpu_options:
                tc.config.set_num_gpus(in_gpus)
                model = tc.image_similarity.create(self.sf, feature=self.feature,
                                                   label=None, model=self.pre_trained_model)
                with test_util.TempDirectory() as filename:
                    model.save(filename)
                    tc.config.set_num_gpus(out_gpus)
                    model = tc.load_model(filename)

        tc.config.set_num_gpus(old_num_gpus)

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
import pytest
import sys
import os
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from turicreate.toolkits._internal_utils import (
    _raise_error_if_not_sarray,
    _mac_ver,
    _read_env_var_cpp,
)
import coremltools

_CLASSES = ["logo_a", "logo_b", "logo_c", "logo_d"]


def _get_data(feature, target):
    from PIL import Image as _PIL_Image

    rs = np.random.RandomState(1234)

    def from_pil_image(pil_img, image_format="png"):
        if image_format == "raw":
            image = np.array(pil_img)
            FORMAT_RAW = 2
            return tc.Image(
                _image_data=image.tobytes(),
                _width=image.shape[1],
                _height=image.shape[0],
                _channels=image.shape[2],
                _format_enum=FORMAT_RAW,
                _image_data_size=image.size,
            )
        else:
            with tempfile.NamedTemporaryFile(
                mode="w+b", suffix="." + image_format
            ) as f:
                pil_img.save(f, format=image_format)
                return tc.Image(f.name)

    num_examples = 100
    num_starter_images = 5
    max_num_boxes_per_image = 10
    classes = _CLASSES
    images = []
    FORMATS = ["png", "jpeg", "raw"]
    for _ in range(num_examples):
        # Randomly determine image size (should handle large and small)
        img_shape = tuple(rs.randint(100, 1000, size=2)) + (3,)
        img = rs.randint(255, size=img_shape)

        pil_img = _PIL_Image.fromarray(img, mode="RGB")
        # Randomly select image format
        image_format = FORMATS[rs.randint(len(FORMATS))]
        images.append(from_pil_image(pil_img, image_format=image_format))

    starter_images = []
    starter_target = []
    for i in range(num_starter_images):
        img_shape = tuple(rs.randint(100, 1000, size=2)) + (3,)
        img = rs.randint(255, size=img_shape)
        pil_img = _PIL_Image.fromarray(img, mode="RGB")
        image_format = FORMATS[rs.randint(len(FORMATS))]
        starter_images.append(from_pil_image(pil_img, image_format=image_format))
        starter_target.append(_CLASSES[i % len(_CLASSES)])

    train = tc.SFrame(
        {feature: tc.SArray(starter_images), target: tc.SArray(starter_target),}
    )
    test = tc.SFrame({feature: tc.SArray(images),})
    backgrounds = test[feature].head(5)
    return train, test, backgrounds


class OneObjectDetectorSmokeTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        """
        The setup class method for the basic test case with all default values.
        """
        self.feature = "myimage"
        self.target = "mytarget"

        ## Create the model
        self.def_opts = {
            "model": "darknet-yolo",
            "max_iterations": 2,
        }

        # Model
        self.train, self.test, self.backgrounds = _get_data(
            feature=self.feature, target=self.target
        )
        self.model = tc.one_shot_object_detector.create(
            self.train,
            target=self.target,
            backgrounds=self.backgrounds,
            batch_size=2,
            max_iterations=1,
        )

        self.get_ans = {
            "target": lambda x: x == self.target,
            "num_starter_images": lambda x: len(self.train),
            "num_classes": lambda x: x == len(_CLASSES),
            "detector": lambda x: isinstance(
                x, tc.object_detector.object_detector.ObjectDetector
            ),
            "_detector_version": lambda x: x == 1,
        }

        self.fields_ans = self.get_ans.keys()

    def test_synthesis_with_single_image(self):
        image = self.train[0][self.feature]
        data = tc.one_shot_object_detector.util.preview_synthetic_training_data(
            image, "custom_logo", backgrounds=self.backgrounds
        )

    def test_create_with_single_image(self):
        image = self.train[0][self.feature]
        model = tc.one_shot_object_detector.create(
            image, "custom_logo", backgrounds=self.backgrounds, max_iterations=1
        )

    def test_create_with_missing_value(self):
        sf = self.train.append(
            tc.SFrame(
                {
                    self.feature: tc.SArray([None], dtype=tc.Image),
                    self.target: [self.train[self.target][0]],
                }
            )
        )
        with self.assertRaises(_ToolkitError):
            tc.one_shot_object_detector.create(sf, target=self.target)

    def test_create_with_missing_target(self):
        with self.assertRaises(_ToolkitError):
            tc.one_shot_object_detector.create(
                self.train,
                target="wrong_feature",
                backgrounds=self.backgrounds,
                max_iterations=1,
            )

    def test_create_with_empty_dataset(self):
        with self.assertRaises(_ToolkitError):
            tc.one_shot_object_detector.create(self.train[:0], target=self.target)

    def test_create_with_no_background_images(self):
        with self.assertRaises(_ToolkitError):
            tc.one_shot_object_detector.create(
                self.train, target=self.target, backgrounds=tc.SArray()
            )

    def test_create_with_wrong_type_background_images(self):
        with self.assertRaises(TypeError):
            tc.one_shot_object_detector.create(
                self.train, target=self.target, backgrounds="wrong_backgrounds"
            )

    def test_predict(self):
        sf = self.test.head()
        pred = self.model.predict(sf.head())

        # Check the structure of the output
        _raise_error_if_not_sarray(pred)
        self.assertEqual(len(pred), len(sf))

        # Make sure SFrame was not altered
        self.assertEqual([col for col in sf.column_names() if col.startswith("_")], [])

        # Predict should work on no input (and produce no predictions)
        pred0 = self.model.predict(sf[self.feature][:0])
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

    def test_export_coreml(self):
        from PIL import Image
        import coremltools
        import platform

        filename = tempfile.mkstemp("bingo.mlmodel")[1]
        self.model.export_coreml(filename, include_non_maximum_suppression=False)

        ## Test metadata
        coreml_model = coremltools.models.MLModel(filename)
        self.maxDiff = None
        self.assertDictEqual(
            {
                "com.github.apple.turicreate.version": tc.__version__,
                "com.github.apple.os.platform": platform.platform(),
                "type": "object_detector",
                "classes": ",".join(sorted(_CLASSES)),
                "feature": self.feature,
                "include_non_maximum_suppression": "False",
                "annotations": "annotation",
                "max_iterations": "1",
                "model": "darknet-yolo",
                "training_iterations": "1",
                "version": "1",
            },
            dict(coreml_model.user_defined_metadata),
        )
        expected_result = (
            "One shot object detector created by Turi Create (version %s)"
            % (tc.__version__)
        )
        self.assertEquals(expected_result, coreml_model.short_description)

        ## Test prediction
        img = self.train[0:1][self.feature][0]
        img_fixed = tc.image_analysis.resize(img, 416, 416, 3)
        pil_img = Image.fromarray(img_fixed.pixel_data)
        if _mac_ver() >= (10, 13):
            ret = coreml_model.predict({self.feature: pil_img}, usesCPUOnly=True)
            self.assertEqual(ret["coordinates"].shape[1], 4)
            self.assertEqual(ret["confidence"].shape[1], len(_CLASSES))
            self.assertEqual(ret["coordinates"].shape[0], ret["confidence"].shape[0])

        # Test export without non max supression
        filename2 = tempfile.mkstemp("bingo2.mlmodel")[1]
        self.model.export_coreml(filename2, include_non_maximum_suppression=True)
        coreml_model = coremltools.models.MLModel(filename)
        self.assertTrue(
            coreml_model.user_defined_metadata["include_non_maximum_suppression"]
        )

    def test__list_fields(self):
        model = self.model
        fields = model._list_fields()
        self.assertEqual(set(fields), set(self.fields_ans))

    def test_get(self,):
        model = self.model
        for field in self.fields_ans:
            ans = model._get(field)
            self.assertTrue(
                self.get_ans[field](ans),
                """Get failed in field {}. Output was {}.""".format(field, ans),
            )

    def test_summary(self):
        model = self.model
        model.summary()

    def test_summary_str(self):
        model = self.model
        self.assertTrue(isinstance(model.summary("str"), str))

    def test_summary_dict(self):
        model = self.model
        self.assertTrue(isinstance(model.summary("dict"), dict))

    def test_summary_invalid(self):
        model = self.model
        model.summary("invalid")

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
            self.test_get()
            self.test_summary()
            self.test__list_fields()

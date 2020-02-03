# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import os
import platform
import sys
import tempfile
import unittest

from turicreate.toolkits._main import ToolkitError as _ToolkitError
from turicreate.toolkits._internal_utils import (
    _mac_ver,
    _raise_error_if_not_sframe,
    _raise_error_if_not_sarray,
)

from . import util as test_util

import coremltools
import numpy as np
import pytest
import turicreate as tc


def get_test_data():
    """
    Create 5 all white images and 5 all black images. Then add some noise to
    each image.
    """
    from PIL import Image

    DIM = 224

    # Five all white images
    data = []
    for _ in range(5):
        data.append(np.full((DIM, DIM, 3), 255, dtype=np.uint8))

    # Five all black images
    for _ in range(5):
        data.append(np.full((DIM, DIM, 3), 0, dtype=np.uint8))

    # Add some random noise to each images
    random = np.random.RandomState(100)
    for cur_image in data:
        for _ in range(1000):
            x, y = random.randint(DIM), random.randint(DIM)
            rand_pixel_value = (
                random.randint(255),
                random.randint(255),
                random.randint(255),
            )
            cur_image[x][y] = rand_pixel_value

    # Convert to an array of tc.Images
    images = []
    for cur_data in data:
        pil_image = Image.fromarray(cur_data)
        image_data = bytearray([z for l in pil_image.getdata() for z in l])
        image_data_size = len(image_data)
        tc_image = tc.Image(
            _image_data=image_data,
            _width=DIM,
            _height=DIM,
            _channels=3,
            _format_enum=2,
            _image_data_size=image_data_size,
        )
        images.append(tc_image)

    labels = ["white"] * 5 + ["black"] * 5
    return tc.SFrame({"awesome_image": images, "awesome_label": labels})


data = get_test_data()


class ImageClassifierTest(unittest.TestCase):
    @classmethod
    def setUpClass(
        self,
        model="resnet-50",
        input_image_shape=(3, 224, 224),
        tol=0.02,
        num_examples=100,
        label_type=int,
    ):
        self.feature = "awesome_image"
        self.target = "awesome_label"
        self.input_image_shape = input_image_shape
        self.pre_trained_model = model
        self.tolerance = tol

        self.model = tc.image_classifier.create(
            data, target=self.target, model=self.pre_trained_model, seed=42
        )
        self.nn_model = self.model.feature_extractor
        self.lm_model = self.model.classifier
        self.max_iterations = 10

        self.get_ans = {
            "classifier": lambda x: type(x)
            == tc.logistic_classifier.LogisticClassifier,
            "feature": lambda x: x == self.feature,
            "classes": lambda x: x == self.lm_model.classes,
            "training_time": lambda x: x > 0,
            "input_image_shape": lambda x: x == self.input_image_shape,
            "target": lambda x: x == self.target,
            "feature_extractor": lambda x: callable(x.extract_features),
            "training_loss": lambda x: x > 0,
            "max_iterations": lambda x: x == self.max_iterations,
            "num_features": lambda x: x == self.lm_model.num_features,
            "num_examples": lambda x: x == self.lm_model.num_examples,
            "model": lambda x: (
                x == self.pre_trained_model
                or (
                    self.pre_trained_model == "VisionFeaturePrint_Screen"
                    and x == "VisionFeaturePrint_Scene"
                )
            ),
            "num_classes": lambda x: x == self.lm_model.num_classes,
        }
        self.fields_ans = self.get_ans.keys()

    def assertListAlmostEquals(self, list1, list2, tol):
        self.assertEqual(len(list1), len(list2))
        for a, b in zip(list1, list2):
            self.assertAlmostEqual(a, b, delta=tol)

    def test_create_with_missing_value(self):
        data_with_none = data.append(
            tc.SFrame(
                {
                    self.feature: tc.SArray([None], dtype=tc.Image),
                    self.target: [data[self.target][0]],
                }
            )
        )
        with self.assertRaises(_ToolkitError):
            tc.image_classifier.create(
                data_with_none, feature=self.feature, target=self.target
            )

    def test_create_with_missing_feature(self):
        with self.assertRaises(_ToolkitError):
            tc.image_classifier.create(
                data, feature="wrong_feature", target=self.target
            )

    def test_create_with_missing_label(self):
        with self.assertRaises(RuntimeError):
            tc.image_classifier.create(
                data, feature=self.feature, target="wrong_annotations"
            )

    def test_create_with_empty_dataset(self):
        with self.assertRaises(_ToolkitError):
            tc.image_classifier.create(data[:0], target=self.target)

    def test_predict(self):
        model = self.model
        for output_type in ["class", "probability_vector"]:
            preds = model.predict(data.head(), output_type=output_type)
            _raise_error_if_not_sarray(preds)
            self.assertEqual(len(preds), len(data.head()))
            if output_type == "class":
                self.assertTrue(all(preds[:5] == "white"))
                self.assertTrue(all(preds[5:] == "black"))

    def test_single_image(self):
        model = self.model
        single_image = data[0][self.feature]
        prediction = model.predict(single_image)
        self.assertTrue(isinstance(prediction, (int, str)))
        prediction = model.predict_topk(single_image, k=2)
        _raise_error_if_not_sframe(prediction)
        prediction = model.classify(single_image)
        self.assertTrue(
            isinstance(prediction, dict)
            and "class" in prediction
            and "probability" in prediction
        )

    def test_sarray(self):
        model = self.model
        sa = data[self.feature]
        predictions = model.predict(sa)
        _raise_error_if_not_sarray(predictions)
        predictions = model.predict_topk(sa, k=2)
        _raise_error_if_not_sframe(predictions)
        predictions = model.classify(sa)
        _raise_error_if_not_sframe(predictions)

    def test_junk_input(self):
        model = self.model
        with self.assertRaises(TypeError):
            predictions = model.predict("junk value")
        with self.assertRaises(TypeError):
            predictions = model.predict_topk(12)
        with self.assertRaises(TypeError):
            predictions = model.classify("more junk")

    def test_export_coreml(self):
        if self.model.model == "VisionFeaturePrint_Scene":
            pytest.xfail(
                "Expected failure until "
                + "https://github.com/apple/turicreate/issues/2744 is fixed"
            )
        filename = tempfile.mkstemp("bingo.mlmodel")[1]
        self.model.export_coreml(filename)

        coreml_model = coremltools.models.MLModel(filename)
        self.assertDictEqual(
            {
                "com.github.apple.turicreate.version": tc.__version__,
                "com.github.apple.os.platform": platform.platform(),
                "type": "ImageClassifier",
                "coremltoolsVersion": coremltools.__version__,
                "version": "1",
            },
            dict(coreml_model.user_defined_metadata),
        )
        expected_result = (
            "Image classifier (%s) created by Turi Create (version %s)"
            % (self.model.model, tc.__version__)
        )
        self.assertEquals(expected_result, coreml_model.short_description)

    @unittest.skipIf(sys.platform != "darwin", "Core ML only supported on Mac")
    def test_export_coreml_predict(self):
        filename = tempfile.mkstemp("bingo.mlmodel")[1]
        self.model.export_coreml(filename)

        coreml_model = coremltools.models.MLModel(filename)
        img = data[0:1][self.feature][0]
        img_fixed = tc.image_analysis.resize(img, *reversed(self.input_image_shape))
        from PIL import Image

        pil_img = Image.fromarray(img_fixed.pixel_data)

        if _mac_ver() >= (10, 13):
            classes = self.model.classifier.classes
            ret = coreml_model.predict({self.feature: pil_img})
            coreml_values = [ret[self.target + "Probability"][l] for l in classes]

            self.assertListAlmostEquals(
                coreml_values,
                list(self.model.predict(img_fixed, output_type="probability_vector")),
                self.tolerance,
            )

    def test_classify(self):
        model = self.model
        preds = model.classify(data.head())
        self.assertEqual(len(preds), len(data.head()))

    def test_predict_topk(self):
        model = self.model
        for output_type in ["margin", "probability", "rank"]:
            preds = model.predict_topk(data.head(), output_type=output_type, k=2)
            self.assertEqual(len(preds), 2 * len(data.head()))

    def test_list_fields(self):
        model = self.model
        fields = model._list_fields()
        self.assertEqual(set(fields), set(self.fields_ans))

    def test_get(self):
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

    def test_summary_invalid_input(self):
        model = self.model
        with self.assertRaises(_ToolkitError):
            model.summary(model.summary("invalid"))

        with self.assertRaises(_ToolkitError):
            model.summary(model.summary(0))

        with self.assertRaises(_ToolkitError):
            model.summary(model.summary({}))

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
            self.test_list_fields()
            print("List fields passed")

    def test_evaluate_explore(self):
        # Run the explore method and make sure we don't throw an exception.
        # This will test the JSON serialization logic.
        tc.visualization.set_target("none")
        evaluation = self.model.evaluate(data)
        evaluation.explore()


class ImageClassifierSqueezeNetTest(ImageClassifierTest):
    @classmethod
    def setUpClass(self):
        super(ImageClassifierSqueezeNetTest, self).setUpClass(
            model="squeezenet_v1.1",
            input_image_shape=(3, 227, 227),
            tol=0.005,
            num_examples=200,
        )


# TODO: if on skip OS, test negative case
@unittest.skipIf(
    _mac_ver() < (10, 14), "VisionFeaturePrint_Scene only supported on macOS 10.14+"
)
class VisionFeaturePrintSceneTest(ImageClassifierTest):
    @classmethod
    def setUpClass(self):
        super(VisionFeaturePrintSceneTest, self).setUpClass(
            model="VisionFeaturePrint_Scene",
            input_image_shape=(3, 299, 299),
            tol=0.005,
            num_examples=100,
            label_type=str,
        )

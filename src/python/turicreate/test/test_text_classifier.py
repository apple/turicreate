# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import unittest
import tempfile
import coremltools
import turicreate as tc
from turicreate.toolkits._main import ToolkitError
from turicreate.toolkits._internal_utils import _mac_ver
from . import util as test_util

import sys

if sys.version_info.major == 3:
    unittest.TestCase.assertItemsEqual = unittest.TestCase.assertCountEqual


class TextClassifierTest(unittest.TestCase):
    """
    Unit test class for an already trained model.
    """

    @classmethod
    def setUpClass(self):
        text = ["hello friend", "how exciting", "mostly exciting", "hello again"]
        score = [0, 1, 1, 0]
        self.docs = tc.SFrame({"text": text, "score": score})

        self.features = ["text"]
        self.num_features = 1
        self.target = "score"
        self.method = "bow-logistic"
        self.model = tc.text_classifier.create(
            self.docs, target=self.target, features=self.features, method="auto"
        )

        self.num_examples = 4

    def test__list_fields(self):
        """
        Check the model list fields method.
        """
        correct_fields = [
            "classifier",
            "features",
            "num_features",
            "method",
            "num_examples",
            "target",
        ]

        self.assertItemsEqual(self.model._list_fields(), correct_fields)

    def test_get(self):
        """
        Check the various 'get' methods against known answers for each field.
        """
        correct_fields = {
            "features": self.features,
            "num_features": self.num_features,
            "target": self.target,
            "method": self.method,
            "num_examples": self.num_examples,
        }

        print(self.model)
        for field, ans in correct_fields.items():
            self.assertEqual(self.model._get(field), ans, "{} failed".format(field))

    def test_model_access(self):
        m = self.model.classifier
        self.assertTrue(
            isinstance(m, tc.classifier.logistic_classifier.LogisticClassifier)
        )

    def test_summaries(self):
        """
        Unit test for __repr__, __str__, and model summary methods; should fail
        if they raise an Exception.
        """
        ans = str(self.model)
        print(self.model)
        self.model.summary()

    def test_evaluate(self):
        """
        Tests for evaluating the model.
        """
        self.model.evaluate(self.docs)

    def test_export_coreml(self):
        filename = tempfile.mkstemp("bingo.mlmodel")[1]
        self.model.export_coreml(filename)

        import platform

        coreml_model = coremltools.models.MLModel(filename)
        self.assertDictEqual(
            {
                "com.github.apple.turicreate.version": tc.__version__,
                "com.github.apple.os.platform": platform.platform(),
                "type": self.model.__class__.__name__,
            },
            dict(coreml_model.user_defined_metadata),
        )
        expected_result = (
            "Text classifier created by Turi Create (version %s)" % tc.__version__
        )
        self.assertEquals(expected_result, coreml_model.short_description)

    @unittest.skipIf(_mac_ver() < (10, 13), "Only supported on macOS 10.13+")
    def test_export_coreml_with_predict(self):
        filename = tempfile.mkstemp("bingo.mlmodel")[1]
        self.model.export_coreml(filename)
        preds = self.model.predict(self.docs, output_type="probability_vector")

        coreml_model = coremltools.models.MLModel(filename)
        coreml_preds = coreml_model.predict({"text": {"hello": 1, "friend": 1}})
        self.assertAlmostEqual(preds[0][0], coreml_preds["scoreProbability"][0])
        self.assertAlmostEqual(preds[0][1], coreml_preds["scoreProbability"][1])

    def test_save_and_load(self):
        """
        Ensure that model saving and loading retains all model information.
        """

        with test_util.TempDirectory() as f:
            self.model.save(f)
            self.model = tc.load_model(f)
            loaded_model = tc.load_model(f)

            self.test__list_fields()
            print("Saved model list fields passed")

            self.test_get()
            print("Saved model get passed")

            self.test_summaries()
            print("Saved model summaries passed")


class TextClassifierCreateTests(unittest.TestCase):
    @classmethod
    def setUpClass(self):

        self.data = tc.SFrame(
            {
                "rating": [1, 5, 2, 3, 3, 5],
                "place": ["a", "a", "b", "b", "b", "c"],
                "text": [
                    "The burrito was terrible and awful and I hated it",
                    "I will come here every day of my life because the burrito "
                    "is awesome and delicious",
                    "Meh, the waiter died while serving us. Other than that "
                    "the experience was OK, but the burrito was not great.",
                    "Mediocre burrito. Nothing much else to report.",
                    "My dad works here, so I guess I have to kinda like it. "
                    "Hate the burrito, though.",
                    "Love it! Mexican restaurant of my dreams and a burrito "
                    "from the gods.",
                ],
            }
        )

        self.rating_column = "rating"
        self.features = ["text"]
        self.keywords = ["burrito", "dad"]
        self.model = tc.text_classifier.create(
            self.data, target="rating", features=self.features
        )

    def test_sentiment_create_no_features(self):
        model = tc.text_classifier.create(self.data, target="rating")
        self.assertTrue(isinstance(model, tc.text_classifier.TextClassifier))

    def test_sentiment_create_string_target(self):
        data_str = self.data[:]
        data_str["rating"] = data_str["rating"].astype(str)
        model = tc.text_classifier.create(data_str, target="rating")
        self.assertTrue(isinstance(model, tc.text_classifier.TextClassifier))

    def test_invalid_data_set(self):
        # infer dtype str
        a = tc.SArray(["str", None])
        b = tc.SArray(["str", "str"])
        # target contains none
        sf = tc.SFrame({"a": a, "b": b})
        with self.assertRaises(ToolkitError):
            tc.text_classifier.create(
                sf, target="a", features=["b"], word_count_threshold=1
            )
        # feature contains none, Github #2402
        sf = tc.SFrame({"b": a, "a": b})
        with self.assertRaises(ToolkitError):
            tc.text_classifier.create(
                sf, target="b", features=["a"], word_count_threshold=1
            )

    def test_validation_set(self):
        train = self.data
        valid = self.data

        # Test with a validation set
        model = tc.text_classifier.create(train, target="rating", validation_set=valid)
        self.assertTrue(
            "Validation Accuracy" in model.classifier.progress.column_names()
        )

        # Test without a validation set
        model = tc.text_classifier.create(train, target="rating", validation_set=None)
        self.assertTrue(
            "Validation Accuracy" not in model.classifier.progress.column_names()
        )

        # Test 'auto' validation set
        big_data = train.append(
            tc.SFrame(
                {
                    "rating": [5] * 100,
                    "place": ["d"] * 100,
                    "text": [
                        "large enough data for %5 percent validation split to activate"
                    ]
                    * 100,
                }
            )
        )
        model = tc.text_classifier.create(
            big_data, target="rating", validation_set="auto"
        )
        self.assertTrue(
            "Validation Accuracy" in model.classifier.progress.column_names()
        )

        # Test bad validation set string
        with self.assertRaises(TypeError):
            tc.text_classifier.create(train, target="rating", validation_set="wrong")

        # Test bad validation set type
        with self.assertRaises(TypeError):
            tc.text_classifier.create(train, target="rating", validation_set=5)

    def test_sentiment_classifier(self):
        m = self.model
        self.assertEqual(m.classifier.classes, [1, 2, 3, 5])

    def test_predict(self):
        m = self.model
        preds = m.predict(self.data)
        self.assertTrue(isinstance(preds, tc.SArray))
        self.assertEqual(preds.dtype, int)

    def test_classify(self):
        m = self.model
        preds = m.classify(self.data)
        self.assertTrue(isinstance(preds, tc.SFrame))
        self.assertEqual(preds.column_names(), ["class", "probability"])

    def test_not_sframe_create_error(self):
        dataset = {"rating": [1, 5], "text": ["this is bad", "this is good"]}
        try:
            # dataset is NOT an SFrame
            tc.text_classifier.create(dataset, "rating", features=["text"])
        except ToolkitError as t:
            exception_msg = t.args[0]
            self.assertTrue(
                exception_msg.startswith("Input dataset is not an SFrame. ")
            )
        else:
            self.fail("This should have thrown an exception")


class TextClassifierCreateBadValues(unittest.TestCase):
    @classmethod
    def setUpClass(self):

        self.data = tc.SFrame(
            {
                "rating": [1, 5, 2, 3],
                "place": ["a", "a", "b", "b"],
                "text": [
                    "The burrito was terrible and awful and I hated it",
                    "I will come here a lot",
                    "......",
                    "",
                ],
            }
        )

        self.rating_column = "rating"
        self.features = ["text"]
        self.keywords = ["burrito", "dad"]

    def test_create(self):
        model = tc.text_classifier.create(
            self.data, target=self.rating_column, features=self.features
        )
        self.assertTrue(model is not None)

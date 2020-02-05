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
import turicreate as tc
import math
import uuid
import random
import copy
from turicreate.toolkits import evaluation
from turicreate.toolkits._main import ToolkitError
from turicreate.toolkits._internal_utils import _mac_ver
import shutil
import numpy as np
import coremltools
import sys
import platform
import array
import os

import pytest

dirname = os.path.dirname(__file__)
mushroom_dataset = os.path.join(dirname, "mushroom.csv")


class CoreMLExportTest(unittest.TestCase):
    def generate_data(self, testtype, n, code_string):

        # numeric; integer, string categorical, list categorical, dictionary,
        # array, nd array (1 dim), nd array (4 dim).

        if testtype == "regression":
            sf = tc.util.generate_random_regression_sframe(
                n, code_string, random_seed=1
            )
            test_sf = tc.util.generate_random_regression_sframe(
                n, code_string, random_seed=2
            )

        elif testtype == "classification":
            sf = tc.util.generate_random_classification_sframe(
                n, code_string, 2, random_seed=1
            )
            test_sf = tc.util.generate_random_classification_sframe(
                n, code_string, 2, random_seed=2
            )

        elif testtype == "multiclass":
            sf = tc.util.generate_random_classification_sframe(
                n, code_string, 10, random_seed=1
            )
            test_sf = tc.util.generate_random_classification_sframe(
                n, code_string, 10, random_seed=2
            )

        else:
            assert False

        return sf, test_sf

    def _test_coreml_export(
        self, model, test_sf, is_regression, has_probability=None, predict_topk=None
    ):

        if has_probability is None:
            has_probability = not is_regression

        if predict_topk is None:
            predict_topk = not is_regression

        # Act & Assert
        with tempfile.NamedTemporaryFile(mode="w", suffix=".mlmodel") as mlmodel_file:
            mlmodel_filename = mlmodel_file.name
            model.export_coreml(mlmodel_filename)
            coreml_model = coremltools.models.MLModel(mlmodel_filename)
            self.assertDictEqual(
                {
                    "com.github.apple.turicreate.version": tc.__version__,
                    "com.github.apple.os.platform": platform.platform(),
                    "type": model.__class__.__name__,
                },
                dict(coreml_model.user_defined_metadata),
            )

            if _mac_ver() < (10, 13):
                print("Skipping export test; model not supported on this platform.")
                return

            def array_to_numpy(row):
                import array
                import numpy
                import copy

                row = copy.copy(row)
                for r in row:
                    if type(row[r]) == array.array:
                        row[r] = numpy.array(row[r])
                return row

            for row in test_sf:

                coreml_prediction = coreml_model.predict(array_to_numpy(row))
                tc_prediction = model.predict(row)[0]

                if (is_regression == False) and (type(model.classes[0]) == str):
                    if not has_probability:
                        self.assertEqual(coreml_prediction["target"], tc_prediction)
                else:
                    self.assertAlmostEqual(
                        coreml_prediction["target"], tc_prediction, delta=1e-5
                    )

                # If applicable, compare probabilistic output
                if has_probability and not is_regression:
                    coreml_ret = coreml_prediction["targetProbability"]
                    _, values_tuple = zip(*sorted(coreml_ret.items()))
                    coreml_probs = np.array(values_tuple)
                    tc_probs = np.array(
                        model.predict(row, output_type="probability_vector")[0]
                    )
                    np.testing.assert_array_almost_equal(
                        coreml_probs, tc_probs, decimal=5
                    )

    #############################################################
    # Regression

    def test_linear_regression(self):
        if _mac_ver() < (10, 14):
            pytest.xfail("See https://github.com/apple/turicreate/issues/1332")
        for code_string in ["b" * 40, "nnnn", "v", "d", "A", "bnsCvAd"]:
            train, test = self.generate_data("regression", 100, code_string)
            model = tc.linear_regression.create(train, "target", validation_set=None)
            model.evaluate(test)  # Previous regression -- this caused errors.
            self._test_coreml_export(model, test, True)

    def test_decision_tree_regression_simple(self):
        for code_string in ["nnnn", "v"]:
            train, test = self.generate_data("regression", 100, code_string)
            model = tc.decision_tree_regression.create(
                train, "target", validation_set=None
            )
            model.evaluate(test)  # Previous regression -- this caused errors.
            self._test_coreml_export(model, test, True)

    @pytest.mark.xfail()
    def test_decision_tree_regression_advanced(self):
        for code_string in ["b" * 40, "sss", "d", "v", "Ad", "bnsCvAd"]:
            train, test = self.generate_data("regression", 100, code_string)
            model = tc.decision_tree_regression.create(
                train, "target", validation_set=None
            )
            model.evaluate(test)  # Previous regression -- this caused errors.
            self._test_coreml_export(model, test, True)

    def test_boosted_trees_regression_simple(self):
        for code_string in ["nnnn", "v"]:
            train, test = self.generate_data("regression", 100, code_string)
            model = tc.boosted_trees_regression.create(
                train, "target", validation_set=None
            )
            model.evaluate(test)  # Previous regression -- this caused errors.
            self._test_coreml_export(model, test, True)

    @pytest.mark.xfail()
    def test_boosted_trees_regression_advanced(self):
        for code_string in ["b" * 40, "sss", "d", "v", "Ad", "bnsCvAd"]:
            train, test = self.generate_data("regression", 100, code_string)
            model = tc.boosted_trees_regression.create(
                train, "target", validation_set=None
            )
            model.evaluate(test)  # Previous regression -- this caused errors.
            self._test_coreml_export(model, test, True)

    @pytest.mark.xfail()
    def test_random_forest_regression_simple(self):
        for code_string in ["nnnn", "bns", "sss"]:
            train, test = self.generate_data("regression", 100, code_string)
            model = tc.random_forest_regression.create(
                train, "target", validation_set=None
            )
            model.evaluate(test)  # Previous regression -- this caused errors.
            self._test_coreml_export(model, test, True)

    @pytest.mark.xfail()
    def test_random_forest_regression_advanced(self):
        for code_string in ["b" * 40, "d", "v", "Ad", "bnsCvAd"]:
            train, test = self.generate_data("regression", 100, code_string)
            model = tc.random_forest_regression.create(
                train, "target", validation_set=None
            )
            model.evaluate(test)  # Previous regression -- this caused errors.
            self._test_coreml_export(model, test, True)

    #############################################################
    # Classification

    def test_logistic_classifier(self):
        if _mac_ver() < (10, 14):
            pytest.xfail("See https://github.com/apple/turicreate/issues/1332")
        for code_string in ["b" * 40, "nnnn", "v", "d", "A", "bnsCvAd"]:
            train, test = self.generate_data("classification", 100, code_string)
            model = tc.logistic_classifier.create(train, "target", validation_set=None)
            model.evaluate(test)  # Previous regression -- this caused errors.
            self._test_coreml_export(model, test, False)

    def test_svm_classifier(self):
        if _mac_ver() < (10, 14):
            pytest.xfail("See https://github.com/apple/turicreate/issues/1332")
        for code_string in ["b" * 40, "nnnn", "v", "d", "A", "bnsCvAd"]:
            train, test = self.generate_data("classification", 100, code_string)
            model = tc.svm_classifier.create(train, "target", validation_set=None)
            model.evaluate(test)  # Previous regression -- this caused errors.
            self._test_coreml_export(model, test, False, has_probability=False)

    def test_decision_tree_classifier_simple(self):
        for code_string in ["nn"]:
            train, test = self.generate_data("classification", 100, code_string)
            model = tc.decision_tree_classifier.create(
                train, "target", validation_set=None, max_depth=3
            )
            model.evaluate(test)  # Previous classifier -- this caused errors.
            self._test_coreml_export(model, test, False)

    @pytest.mark.xfail()
    def test_decision_tree_classifier_advanced(self):
        for code_string in ["b" * 40, "nnnn", "sss", "d", "v", "Ad", "bnsCvAd"]:
            train, test = self.generate_data("classification", 100, code_string)
            model = tc.decision_tree_classifier.create(
                train, "target", validation_set=None, max_depth=3
            )
            model.evaluate(test)  # Previous classifier -- this caused errors.
            self._test_coreml_export(model, test, False)

    def test_boosted_trees_classifier_simple(self):
        for code_string in ["nn"]:
            train, test = self.generate_data("classification", 100, code_string)
            model = tc.boosted_trees_classifier.create(
                train, "target", validation_set=None, max_depth=3, max_iterations=5
            )
            model.evaluate(test)  # Previous classifier -- this caused errors.
            self._test_coreml_export(model, test, False)

    @pytest.mark.xfail()
    def test_boosted_trees_classifier_advanced(self):
        for code_string in ["b" * 40, "nnnn", "sss", "d", "v", "Ad", "bnsCvAd"]:
            train, test = self.generate_data("classification", 100, code_string)
            model = tc.boosted_trees_classifier.create(
                train, "target", validation_set=None, max_depth=3, max_iterations=5
            )
            model.evaluate(test)  # Previous classifier -- this caused errors.
            self._test_coreml_export(model, test, False)

    @pytest.mark.xfail()
    def test_random_forest_classifier_simple(self):
        for code_string in ["nnnn", "bns", "sss"]:
            train, test = self.generate_data("classification", 100, code_string)
            model = tc.random_forest_classifier.create(
                train, "target", validation_set=None, max_depth=3, max_iterations=5
            )
            model.evaluate(test)  # Previous classifier -- this caused errors.
            self._test_coreml_export(model, test, False)

    @pytest.mark.xfail()
    def test_random_forest_classifier_advanced(self):
        for code_string in ["b" * 40, "d", "v", "Ad", "bnsCvAd"]:
            train, test = self.generate_data("classification", 100, code_string)
            model = tc.random_forest_classifier.create(
                train, "target", validation_set=None, max_depth=3, max_iterations=5
            )
            model.evaluate(test)  # Previous classifier -- this caused errors.
            self._test_coreml_export(model, test, False)

    #############################################################
    #  Muliclass

    def test_logistic_multiclass(self):
        if _mac_ver() < (10, 14):
            pytest.xfail("See https://github.com/apple/turicreate/issues/1332")
        for code_string in ["b" * 40, "nnnn", "v", "d", "A", "bnsCvAd"]:
            train, test = self.generate_data("multiclass", 100, code_string)
            model = tc.logistic_classifier.create(
                train, "target", validation_set=None, max_iterations=5
            )
            model.evaluate(test)  # Previous regression -- this caused errors.
            self._test_coreml_export(model, test, False)

    def test_decision_tree_multiclass_simple(self):
        for code_string in ["nn"]:
            train, test = self.generate_data("multiclass", 100, code_string)
            model = tc.decision_tree_classifier.create(
                train, "target", validation_set=None, max_depth=3
            )
            model.evaluate(test)  # Previous classifier -- this caused errors.
            self._test_coreml_export(model, test, False)

    @pytest.mark.xfail()
    def test_decision_tree_multiclass_advanced(self):
        for code_string in ["b" * 40, "nnnn", "sss", "d", "v", "Ad", "bnsCvAd"]:
            train, test = self.generate_data("multiclass", 100, code_string)
            model = tc.decision_tree_classifier.create(
                train, "target", validation_set=None, max_depth=3
            )
            model.evaluate(test)  # Previous classifier -- this caused errors.
            self._test_coreml_export(model, test, False)

    def test_boosted_trees_multiclass_simple(self):
        for code_string in ["nn"]:
            train, test = self.generate_data("multiclass", 100, code_string)
            model = tc.boosted_trees_classifier.create(
                train, "target", validation_set=None, max_depth=3, max_iterations=5
            )
            model.evaluate(test)  # Previous classifier -- this caused errors.
            self._test_coreml_export(model, test, False)

    @pytest.mark.xfail()
    def test_boosted_trees_multiclass_advanced(self):
        for code_string in ["b" * 40, "nnnn", "sss", "d", "v", "Ad", "bnsCvAd"]:
            train, test = self.generate_data("multiclass", 100, code_string)
            model = tc.boosted_trees_classifier.create(
                train, "target", validation_set=None, max_depth=3, max_iterations=5
            )
            model.evaluate(test)  # Previous classifier -- this caused errors.
            self._test_coreml_export(model, test, False)

    @pytest.mark.xfail()
    def test_random_forest_multiclass_simple(self):
        for code_string in ["nnnn", "bns", "sss"]:
            train, test = self.generate_data("multiclass", 100, code_string)
            model = tc.random_forest_classifier.create(
                train, "target", validation_set=None, max_depth=3, max_iterations=5
            )
            model.evaluate(test)  # Previous classifier -- this caused errors.
            self._test_coreml_export(model, test, False)

    @pytest.mark.xfail()
    def test_random_forest_multiclass_advanced(self):
        for code_string in ["b" * 40, "d", "v", "Ad", "bnsCvAd"]:
            train, test = self.generate_data("multiclass", 100, code_string)
            model = tc.random_forest_classifier.create(
                train, "target", validation_set=None, max_depth=3, max_iterations=5
            )
            model.evaluate(test)  # Previous classifier -- this caused errors.
            self._test_coreml_export(model, test, False)

    #############################################################
    #  Muliclass with few examples; this gaurantees that some
    #  classes in the test set won't overlap.

    def test_logistic_multiclass_tiny(self):
        if _mac_ver() < (10, 14):
            pytest.xfail("See https://github.com/apple/turicreate/issues/1332")
        for code_string in ["b" * 40, "nnnn", "v", "d", "A", "bnsCvAd"]:
            train, test = self.generate_data("multiclass", 8, code_string)
            model = tc.logistic_classifier.create(train, "target", validation_set=None)
            model.evaluate(test)  # Previous regression -- this caused errors.
            self._test_coreml_export(model, test, False)

    def test_decision_tree_multiclass_simple_tiny(self):
        for code_string in ["nn"]:
            train, test = self.generate_data("multiclass", 8, code_string)
            model = tc.decision_tree_classifier.create(
                train, "target", validation_set=None, max_depth=3
            )
            model.evaluate(test)  # Previous classifier -- this caused errors.
            self._test_coreml_export(model, test, False)

    @pytest.mark.xfail()
    def test_decision_tree_multiclass_advanced_tiny(self):
        for code_string in ["b" * 40, "nnnn", "sss", "d", "v", "Ad", "bnsCvAd"]:
            train, test = self.generate_data("multiclass", 8, code_string)
            model = tc.decision_tree_classifier.create(
                train, "target", validation_set=None, max_depth=3
            )
            model.evaluate(test)  # Previous classifier -- this caused errors.
            self._test_coreml_export(model, test, False)

    def test_boosted_trees_multiclass_simple_tiny(self):
        for code_string in ["nn"]:
            train, test = self.generate_data("multiclass", 8, code_string)
            model = tc.boosted_trees_classifier.create(
                train, "target", validation_set=None, max_depth=3, max_iterations=5
            )
            model.evaluate(test)  # Previous classifier -- this caused errors.
            self._test_coreml_export(model, test, False)

    @pytest.mark.xfail()
    def test_boosted_trees_multiclass_advanced_tiny(self):
        for code_string in ["b" * 40, "nnnn", "sss", "d", "v", "Ad", "bnsCvAd"]:
            train, test = self.generate_data("multiclass", 8, code_string)
            model = tc.boosted_trees_classifier.create(
                train, "target", validation_set=None, max_depth=3, max_iterations=5
            )
            model.evaluate(test)  # Previous classifier -- this caused errors.
            self._test_coreml_export(model, test, False)

    def test_random_forest_multiclass_simple_tiny(self):
        for code_string in ["nnnn", "bns", "sss"]:
            train, test = self.generate_data("multiclass", 8, code_string)
            model = tc.random_forest_classifier.create(
                train, "target", validation_set=None, max_depth=3, max_iterations=5
            )
            model.evaluate(test)  # Previous classifier -- this caused errors.
            self._test_coreml_export(model, test, False)

    @pytest.mark.xfail()
    def test_random_forest_multiclass_advanced_tiny(self):
        for code_string in ["b" * 40, "d", "v", "Ad", "bnsCvAd"]:
            train, test = self.generate_data("multiclass", 8, code_string)
            model = tc.random_forest_classifier.create(
                train, "target", validation_set=None, max_depth=3, max_iterations=5
            )
            model.evaluate(test)  # Previous classifier -- this caused errors.
            self._test_coreml_export(model, test, False)

    def test_tree_export_issue_1831(self):
        SEED = 42
        data = tc.SFrame.read_csv(mushroom_dataset)
        data["target"] = data["label"]
        train_data, test_data = data.random_split(0.8, seed=SEED)
        model = tc.boosted_trees_classifier.create(
            train_data, target="target", max_iterations=2, max_depth=3
        )
        self._test_coreml_export(model, test_data, False)

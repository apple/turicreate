# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
import turicreate as tc
import math
import uuid
import random
import copy
from turicreate.toolkits import evaluation
from turicreate.toolkits._main import ToolkitError
import shutil
import numpy as np
from array import array


class TreeExtractFeaturesTest(unittest.TestCase):
    def _run_test(self, sf, expected_number_of_features):

        sf["target"] = [0 if random.random() < 0.5 else 1 for i in range(sf.num_rows())]

        for model in [
            tc.regression.boosted_trees_regression,
            tc.classifier.boosted_trees_classifier,
        ]:
            m = model.create(
                sf, "target", validation_set=None, max_iterations=5, max_depth=2
            )

            out = m.extract_features(sf)
            self.assertEqual(len(out), len(sf))

            out = m._extract_features_with_missing(sf)
            self.assertEqual(len(out), len(sf))

        for model in [
            tc.regression.random_forest_regression,
            tc.classifier.random_forest_classifier,
            tc.regression.decision_tree_regression,
            tc.classifier.decision_tree_classifier,
        ]:
            m = model.create(sf, "target", validation_set=None, max_depth=2)

            out = m.extract_features(sf)
            self.assertEqual(len(out), len(sf))

            out = m._extract_features_with_missing(sf)
            self.assertEqual(len(out), len(sf))

    def test_categorical_1(self):

        sf = tc.SFrame(
            {
                "cat1": ["1", "1", "2", "2", "2"] * 100,
                "cat2": ["1", "3", "3", "1", "1"] * 100,
            }
        )
        self._run_test(sf, 4)

    def test_categorical_2(self):
        sf = tc.SFrame(
            {
                "cat[1]": ["1", "1", "2", "2", "2"] * 100,
                "cat[2]": ["1", "3", "3", "1", "1"] * 100,
            }
        )
        self._run_test(sf, 4)

    def test_dict_1(self):
        sf = tc.SFrame(
            {
                "dict1": [
                    {"1": 1, "2": 3.2},
                    {"1": 3.1,},
                    {"1": 1, "b": 2},
                    {"1": 1, "b": 3},
                    {"a": 2, "b": 3},
                ]
                * 100
            }
        )
        self._run_test(sf, 4)

    def test_dict_2(self):
        sf = tc.SFrame(
            {
                "dict1": [
                    {"1": 1, "2": 3.2},
                    {"1": 3.1,},
                    {1: 1, "b": 2},
                    {1: 1, "b": 3},
                    {"a": 2, "b": 3},
                ]
                * 100
            }
        )
        self._run_test(sf, 5)

    def test_dict_3(self):
        sf = tc.SFrame(
            {
                "dict": [
                    {"1": 1, "2": 3.2},
                    {"1": 3.1,},
                    {1: 1, "b": 2},
                    {1: 1, "b": 3},
                    {"a": 2, "b": 3},
                ]
                * 100,
                "dict[2]": [
                    {"1": 1, "2": 3.2},
                    {"1": 3.1,},
                    {1: 1, "b": 2},
                    {1: 1, "b": 3},
                    {"a": 2, "b": 3},
                ]
                * 100,
                "dict[3]": [
                    {"1": 1, "2": 3.2},
                    {"1": 3.1,},
                    {1: 1, "b": 2},
                    {1: 1, "b": 3},
                    {"a": 2, "b": 3},
                ]
                * 100,
            }
        )

        self._run_test(sf, 15)

    def test_cat_dict_1(self):
        sf = tc.SFrame(
            {
                "cat1": [str(i) for i in range(500)],
                "dict2": [
                    {"1": 1, "2": 3.2},
                    {"1": 3.1,},
                    {1: 1, "b": 2},
                    {1: 1, "b": 3},
                    {"a": 2, "b": 3},
                ]
                * 100,
            }
        )

        self._run_test(sf, 505)

    def test_numeric_1(self):
        sf = tc.SFrame(
            {
                "num1": [1, 2, 3.5, 4, 5] * 100,
                "num2": [1, 2, 3.5, 4, 5] * 100,
                "num3": [1, 2, 3.5, 4, 5] * 100,
            }
        )

        self._run_test(sf, 3)

    def test_numeric_2(self):
        sf = tc.SFrame(
            {
                "num1": [1, 2, 3.5, 4, 5] * 100,
                "num2": [1, 2, 3.5, 4, 5] * 100,
                "vect": [[1, 2, 3.5, 4, 5]] * 500,
            }
        )

        self._run_test(sf, 7)

    def test_numeric_dict(self):
        sf = tc.SFrame(
            {
                "num1": [1, 2, 3.5, 4, 5] * 100,
                "num2": [1, 2, 3.5, 4, 5] * 100,
                "vect": [[1, 2, 3.5, 4, 5]] * 500,
                "dict[2]": [
                    {"1": 1, "2": 3.2},
                    {"1": 3.1,},
                    {1: 1, "b": 2},
                    {1: 1, "b": 3},
                    {"a": 2, "b": 3},
                ]
                * 100,
            }
        )

        self._run_test(sf, 12)

    def test_missing(self):
        sf = tc.SFrame(
            {
                "num1": [1, 2, 3.5, 4, None] * 100,
                "num2": [1, 2, 3.5, 4, None] * 100,
                "num3": [1, 2, 3.5, 4, None] * 100,
            }
        )

        self._run_test(sf, 3)

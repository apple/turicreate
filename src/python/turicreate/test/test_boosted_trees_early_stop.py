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
from turicreate.toolkits._main import ToolkitError
import random


class BoostedTreesRegressionEarlyStopTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sf = tc.SFrame(
            {
                "cat[1]": ["1", "1", "2", "2", "2"] * 20,
                "cat[2]": ["1", "3", "3", "1", "1"] * 20,
                "target": [random.random() for i in range(100)],
            }
        )
        cls.train, cls.test = sf.random_split(0.5, seed=5)
        cls.model = tc.boosted_trees_regression
        cls.metrics = ["rmse", "max_error"]
        return cls

    def _run_test(self, train, valid, early_stopping_rounds, metric="auto"):
        max_iterations = 50
        m = self.model.create(
            train,
            "target",
            validation_set=valid,
            max_depth=2,
            max_iterations=max_iterations,
            early_stopping_rounds=early_stopping_rounds,
            metric=metric,
        )
        self.assertTrue(m.num_trees < max_iterations)

    def test_one_round_early_stop(self):
        self._run_test(self.train, self.test, 1)

    def test_many_round_early_stop(self):
        self._run_test(self.train, self.test, 5)

    def test_single_metric(self):
        for m in self.metrics:
            self._run_test(self.train, self.test, 5, m)

    def test_many_metrics(self):
        self._run_test(self.train, self.test, 5, metric=self.metrics)

    def test_no_validation_exception(self):
        self.assertRaises(ToolkitError, lambda: self._run_test(self.train, None, 5))

    def test_no_metric_exception(self):
        self.assertRaises(
            ToolkitError, lambda: self._run_test(self.train, self.test, 5, metric=[])
        )


class BoostedTreesClassifierEarlyStopTest(BoostedTreesRegressionEarlyStopTest):
    @classmethod
    def setUpClass(cls):
        sf = tc.SFrame(
            {
                "cat[1]": ["1", "1", "2", "2", "2"] * 20,
                "cat[2]": ["1", "3", "3", "1", "1"] * 20,
                "target": [0, 1] * 50,
            }
        )
        cls.train, cls.test = sf.random_split(0.5, seed=5)
        cls.model = tc.boosted_trees_classifier
        cls.metrics = ["accuracy", "log_loss"]
        return cls

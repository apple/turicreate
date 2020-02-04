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
import array
from turicreate.toolkits._main import ToolkitError


class SupervisedLearningStringTargetBinary(unittest.TestCase):
    """
    Base class for missing values in supervised learning.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up (Run only once)
        """
        self.target = "y"
        self.sf = tc.SFrame()
        self.sf["y"] = tc.SArray(["t1", "t2", "t1"], str)
        self.sf["int"] = tc.SArray([1, 2, 3], int)
        self.sf["float"] = tc.SArray([1, 2, 3], float)
        self.sf["dict"] = tc.SArray([{"1": 3, "2": 2}, {"2": 1}, {}], dict)
        self.sf["array"] = tc.SArray([[1, 2], [3, 4], [5, 6]], array.array)
        self.sf["str"] = tc.SArray(["1", "2", "3"], str)

        test_sf = tc.SFrame()
        test_sf["y"] = tc.SArray(["foobar", "t1"], str)
        test_sf["int"] = tc.SArray([2, 1], int)
        test_sf["float"] = tc.SArray([2, 2.0], float)
        test_sf["dict"] = tc.SArray([{"1": 1, "2": 1}, {}], dict)
        test_sf["array"] = tc.SArray([[3, 4], [2, 2]], array.array)
        test_sf["str"] = tc.SArray(["2", "2"], str)
        self.test_sf = test_sf

        self.model = None
        self.support_multi_class = True

    def get_create_function_of_model(self, model):
        import sys

        mod_name = model.__module__
        mod = sys.modules[mod_name]
        return mod.create

    def test_create(self):
        if self.model is None:
            return

        train_sf = self.sf.copy()
        create_fun = self.get_create_function_of_model(self.model)

        # Missing value in each column
        # -------------------------------
        for col in ["int", "float", "dict", "array", "str"]:
            model = create_fun(train_sf, self.target, validation_set=None)

        # Missing value at top of each column
        # -------------------------------
        for col in ["int", "float", "dict", "array", "str"]:
            model = create_fun(train_sf, self.target, validation_set=None)

    def test_evaluate(self):
        if self.model is None:
            return

        model = self.model
        test_sf = self.test_sf.copy()

        # Should pass
        ev_train = model.evaluate(self.sf)
        ev_test = model.evaluate(test_sf)
        ev_test_one = model.evaluate(test_sf[0:1])


class SupervisedLearningStringTargetMulticlass(unittest.TestCase):
    """
    Base class for missing values in supervised learning.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up (Run only once)
        """
        self.target = "y"
        self.sf = tc.SFrame()
        self.sf["y"] = tc.SArray(["t1", "t2", "t3"], str)
        self.sf["int"] = tc.SArray([1, 2, 3], int)
        self.sf["float"] = tc.SArray([1, 2, 3], float)
        self.sf["dict"] = tc.SArray([{"1": 3, "2": 2}, {"2": 1}, {}], dict)
        self.sf["array"] = tc.SArray([[1, 2], [3, 4], [5, 6]], array.array)
        self.sf["str"] = tc.SArray(["1", "2", "3"], str)

        test_sf = tc.SFrame()
        test_sf["y"] = tc.SArray(["foobar", "t1"], str)
        test_sf["int"] = tc.SArray([2, 1], int)
        test_sf["float"] = tc.SArray([2, 2.0], float)
        test_sf["dict"] = tc.SArray([{"1": 1, "2": 1}, {}], dict)
        test_sf["array"] = tc.SArray([[3, 4], [2, 2]], array.array)
        test_sf["str"] = tc.SArray(["2", "2"], str)
        self.test_sf = test_sf

        self.model = None
        self.support_multi_class = True

    def get_create_function_of_model(self, model):
        import sys

        mod_name = model.__module__
        mod = sys.modules[mod_name]
        return mod.create

    def test_create(self):
        if self.model is None:
            return

        train_sf = self.sf.copy()
        create_fun = self.get_create_function_of_model(self.model)

        # Missing value in each column
        # -------------------------------
        for col in ["int", "float", "dict", "array", "str"]:
            model = create_fun(train_sf, self.target, validation_set=None)

        # Missing value at top of each column
        # -------------------------------
        for col in ["int", "float", "dict", "array", "str"]:
            model = create_fun(train_sf, self.target, validation_set=None)

    def test_evaluate(self):
        if self.model is None:
            return

        model = self.model
        test_sf = self.test_sf.copy()

        # Should pass
        ev_train = model.evaluate(self.sf)
        ev_test = model.evaluate(test_sf)
        ev_test_one = model.evaluate(test_sf[0:1])


class LogisticRegressionTest(SupervisedLearningStringTargetBinary):
    @classmethod
    def setUpClass(self):
        super(LogisticRegressionTest, self).setUpClass()
        self.model = tc.logistic_classifier.create(
            self.sf, self.target, validation_set=None
        )


class SVMClassifierTest(SupervisedLearningStringTargetBinary):
    @classmethod
    def setUpClass(self):
        super(SVMClassifierTest, self).setUpClass()
        self.model = tc.svm_classifier.create(self.sf, self.target, validation_set=None)


class RandomForestClassifierTest(SupervisedLearningStringTargetBinary):
    @classmethod
    def setUpClass(self):
        super(RandomForestClassifierTest, self).setUpClass()
        self.model = tc.random_forest_classifier.create(
            self.sf, self.target, validation_set=None
        )


class DecisionTreeClassifierTest(SupervisedLearningStringTargetBinary):
    @classmethod
    def setUpClass(self):
        super(DecisionTreeClassifierTest, self).setUpClass()
        self.model = tc.decision_tree_classifier.create(
            self.sf, self.target, validation_set=None
        )


class BoostedTreesClassifierTest(SupervisedLearningStringTargetBinary):
    @classmethod
    def setUpClass(self):
        super(BoostedTreesClassifierTest, self).setUpClass()
        self.model = tc.boosted_trees_classifier.create(
            self.sf, self.target, validation_set=None
        )


class MulticlassLogisticRegressionTest(SupervisedLearningStringTargetMulticlass):
    @classmethod
    def setUpClass(self):
        super(MulticlassLogisticRegressionTest, self).setUpClass()
        self.model = tc.logistic_classifier.create(
            self.sf, self.target, validation_set=None
        )


class MulticlassRandomForestClassifierTest(SupervisedLearningStringTargetMulticlass):
    @classmethod
    def setUpClass(self):
        super(MulticlassRandomForestClassifierTest, self).setUpClass()
        self.model = tc.random_forest_classifier.create(
            self.sf, self.target, validation_set=None
        )


class MulticlassDecisionTreeClassifierTest(SupervisedLearningStringTargetMulticlass):
    @classmethod
    def setUpClass(self):
        super(MulticlassDecisionTreeClassifierTest, self).setUpClass()
        self.model = tc.decision_tree_classifier.create(
            self.sf, self.target, validation_set=None
        )


class MulticlassBoostedTreesClassifierTest(SupervisedLearningStringTargetMulticlass):
    @classmethod
    def setUpClass(self):
        super(MulticlassBoostedTreesClassifierTest, self).setUpClass()
        self.model = tc.boosted_trees_classifier.create(
            self.sf, self.target, validation_set=None
        )

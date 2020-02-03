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


class SupervisedLearningMissingValueTest(unittest.TestCase):
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
        self.sf["y"] = tc.SArray([1, 2, 1], int)
        self.sf["int"] = tc.SArray([1, 2, 3], int)
        self.sf["float"] = tc.SArray([1, 2, 3], float)
        self.sf["dict"] = tc.SArray([{"1": 3, "2": 2}, {"2": 1}, {}], dict)
        self.sf["array"] = tc.SArray([[1, 2], [3, 4], [5, 6]], array.array)
        self.sf["str"] = tc.SArray(["1", "2", "3"], str)

        test_sf = tc.SFrame()
        test_sf["y"] = tc.SArray([2], int)
        test_sf["int"] = tc.SArray([2], int)
        test_sf["float"] = tc.SArray([2], float)
        test_sf["dict"] = tc.SArray([{"1": 1, "2": 1}], dict)
        test_sf["array"] = tc.SArray([[3, 4]], array.array)
        test_sf["str"] = tc.SArray(["2"], str)
        self.test_sf = test_sf

        self.model = None
        self.support_missing_value = False

    def fill_with_na(self, sf, colname):
        """
        Helper function
        Replace a column in the sframe with Nones
        Type of the original column is preserved
        """
        ret = sf.copy()
        if colname in sf.column_names():
            ret[colname] = tc.SArray([None] * len(sf), sf[colname].dtype)
        return ret

    def fill_some_na(self, sf, colname):
        """
        Helper function
        Replace the first element of the column in the sframe with None.
        """
        ret = sf.copy()
        if colname in sf.column_names():
            a = tc.SArray([None], sf[colname].dtype)
            b = sf[colname][1:]
            ret[colname] = a.append(b)
        return ret

    def get_create_function_of_model(self, model):
        import sys

        mod_name = model.__module__
        mod = sys.modules[mod_name]
        return mod.create

    def test_create(self):
        """
        Test create with missing value
        """
        if self.model is None or not self.support_missing_value:
            return

        train_sf = self.sf.copy()
        create_fun = self.get_create_function_of_model(self.model)

        # Missing value in each column
        # -------------------------------
        for col in ["int", "float", "dict", "array", "str"]:
            train_sf_with_na = self.fill_with_na(train_sf, col)
            model = create_fun(train_sf_with_na, self.target, validation_set=None)

        # Missing value at top of each column
        # -------------------------------
        for col in ["int", "float", "dict", "array", "str"]:
            train_sf_with_na = self.fill_some_na(train_sf, col)
            model = create_fun(train_sf_with_na, self.target, validation_set=None)

    def test_create_with_missing_target(self):

        if self.model is None or not self.support_missing_value:
            return

        train_sf = self.sf.copy()
        create_fun = self.get_create_function_of_model(self.model)

        with self.assertRaises(ToolkitError):
            train_sf_with_na = self.fill_with_na(train_sf, "y")
            model = create_fun(train_sf_with_na, self.target, validation_set=None)

        with self.assertRaises(ToolkitError):
            train_sf_with_na = self.fill_some_na(train_sf, "y")
            model = create_fun(train_sf_with_na, self.target, validation_set=None)

    def test_predict(self):
        """
        Test predict missing value
        """
        if self.model is None:
            return

        model = self.model
        test_sf = self.test_sf.copy()

        # Should pass
        pred = model.predict(test_sf, missing_value_action="auto")
        pred = model.predict(test_sf, missing_value_action="impute")
        pred = model.predict(test_sf, missing_value_action="error")

        # Missing value in each column
        # -------------------------------
        for col in ["int", "float", "dict", "array", "str"]:
            test_sf_with_na = self.fill_with_na(test_sf, col)
            pred_missing = model.predict(test_sf_with_na)

        for col in ["int", "float", "array"]:
            test_sf_with_na = self.fill_with_na(test_sf, col)
            self.assertRaises(
                ToolkitError,
                lambda: model.predict(test_sf_with_na, missing_value_action="error"),
            )

        # Missing entire columns
        # ----------------------------------
        for col in ["int", "float", "dict", "array", "str"]:
            del test_sf[col]
            pred = model.predict(test_sf)

    def test_extract_features(self):
        """
        Test extract missing value
        """
        if self.model is None:
            return

        if not hasattr(self.model, "extract_features"):
            return

        model = self.model
        test_sf = self.test_sf.copy()

        # Should pass
        pred = model.extract_features(test_sf, missing_value_action="auto")
        pred = model.extract_features(test_sf, missing_value_action="impute")
        pred = model.extract_features(test_sf, missing_value_action="error")

        # Missing value in each column
        # -------------------------------
        for col in ["int", "float", "dict", "array", "str"]:
            test_sf_with_na = self.fill_with_na(test_sf, col)
            pred_missing = model.extract_features(test_sf_with_na)

        for col in ["int", "float", "array"]:
            test_sf_with_na = self.fill_with_na(test_sf, col)
            self.assertRaises(
                ToolkitError,
                lambda: model.extract_features(
                    test_sf_with_na, missing_value_action="error"
                ),
            )

        # Missing entire columns
        # ----------------------------------
        for col in ["int", "float", "dict", "array", "str"]:
            del test_sf[col]
            pred = model.extract_features(test_sf)

    def test_evaluate(self):
        """
        Test evaluate missing value
        """
        if self.model is None:
            return

        model = self.model
        test_sf = self.test_sf.copy()

        # Should pass
        eval = model.evaluate(test_sf, missing_value_action="auto")
        eval = model.evaluate(test_sf, missing_value_action="impute")
        eval = model.evaluate(test_sf, missing_value_action="error")

        # Missing value in each col type
        # -------------------------------
        for col in ["int", "float", "dict", "array", "str"]:
            test_sf_with_na = self.fill_with_na(test_sf, col)
            eval_missing = model.evaluate(test_sf_with_na)

        for col in ["int", "float", "array"]:
            test_sf_with_na = self.fill_with_na(test_sf, col)
            self.assertRaises(
                ToolkitError,
                lambda: model.evaluate(test_sf_with_na, missing_value_action="error"),
            )

        # Missing columns
        # ----------------------------------
        for col in ["int", "float", "dict", "array", "str"]:
            del test_sf[col]
            model.evaluate(test_sf)

    def test_classify(self):
        """
        Test classify missing value
        """
        if self.model is None or not hasattr(self.model, "classify"):
            return

        model = self.model
        test_sf = self.test_sf.copy()

        # Should pass
        model.classify(test_sf, missing_value_action="auto")
        model.classify(test_sf, missing_value_action="impute")
        model.classify(test_sf, missing_value_action="error")

        # Missing value in each col type
        # -------------------------------
        for col in ["int", "float", "dict", "array", "str"]:
            test_sf_with_na = self.fill_with_na(test_sf, col)
            model.classify(test_sf_with_na)

        for col in ["int", "float", "array"]:
            test_sf_with_na = self.fill_with_na(test_sf, col)
            self.assertRaises(
                ToolkitError,
                lambda: model.classify(test_sf_with_na, missing_value_action="error"),
            )

        # Missing columns
        # ----------------------------------
        for col in ["int", "float", "dict", "array", "str"]:
            del test_sf[col]
            model.classify(test_sf)

    def test_predict_topk(self):
        """
        Test predict topk with missing value
        """
        if self.model is None or not hasattr(self.model, "predict_topk"):
            return

        model = self.model
        test_sf = self.test_sf.copy()

        # Should pass
        model.predict_topk(test_sf, k=1, missing_value_action="auto")
        model.predict_topk(test_sf, k=1, missing_value_action="impute")
        model.predict_topk(test_sf, k=1, missing_value_action="error")

        # Missing value in each col type
        # -------------------------------
        for col in ["int", "float", "dict", "array", "str"]:
            test_sf_with_na = self.fill_with_na(test_sf, col)
            model.predict_topk(test_sf_with_na, k=1)

        for col in ["int", "float", "array"]:
            test_sf_with_na = self.fill_with_na(test_sf, col)
            self.assertRaises(
                ToolkitError,
                lambda: model.predict_topk(
                    test_sf_with_na, k=1, missing_value_action="error"
                ),
            )

        # Missing columns
        # ----------------------------------
        for col in ["int", "float", "dict", "array", "str"]:
            del test_sf[col]
            model.predict_topk(test_sf, k=1)


class LinearRegressionTest(SupervisedLearningMissingValueTest):
    @classmethod
    def setUpClass(self):
        super(LinearRegressionTest, self).setUpClass()
        self.model = tc.linear_regression.create(
            self.sf, self.target, validation_set=None
        )


class RandomForestRegression(SupervisedLearningMissingValueTest):
    @classmethod
    def setUpClass(self):
        super(RandomForestRegression, self).setUpClass()
        self.model = tc.random_forest_regression.create(
            self.sf, self.target, validation_set=None
        )
        self.support_missing_value = True


class DecisionTreeRegression(SupervisedLearningMissingValueTest):
    @classmethod
    def setUpClass(self):
        super(DecisionTreeRegression, self).setUpClass()
        self.model = tc.decision_tree_regression.create(
            self.sf, self.target, validation_set=None
        )
        self.support_missing_value = True


class BoostedTreesRegression(SupervisedLearningMissingValueTest):
    @classmethod
    def setUpClass(self):
        super(BoostedTreesRegression, self).setUpClass()
        self.model = tc.boosted_trees_regression.create(
            self.sf, self.target, validation_set=None
        )
        self.support_missing_value = True


class LogisticRegressionTest(SupervisedLearningMissingValueTest):
    @classmethod
    def setUpClass(self):
        super(LogisticRegressionTest, self).setUpClass()
        self.model = tc.logistic_classifier.create(
            self.sf, self.target, validation_set=None
        )


class SVMClassifierTest(SupervisedLearningMissingValueTest):
    @classmethod
    def setUpClass(self):
        super(SVMClassifierTest, self).setUpClass()
        self.model = tc.svm_classifier.create(self.sf, self.target, validation_set=None)


class RandomForestClassifierTest(SupervisedLearningMissingValueTest):
    @classmethod
    def setUpClass(self):
        super(RandomForestClassifierTest, self).setUpClass()
        self.model = tc.random_forest_classifier.create(
            self.sf, self.target, validation_set=None
        )
        self.support_missing_value = True


class DecisionTreeClassifierTest(SupervisedLearningMissingValueTest):
    @classmethod
    def setUpClass(self):
        super(DecisionTreeClassifierTest, self).setUpClass()
        self.model = tc.decision_tree_classifier.create(
            self.sf, self.target, validation_set=None
        )
        self.support_missing_value = True


class BoostedTreesClassifierTest(SupervisedLearningMissingValueTest):
    @classmethod
    def setUpClass(self):
        super(BoostedTreesClassifierTest, self).setUpClass()
        self.model = tc.boosted_trees_classifier.create(
            self.sf, self.target, validation_set=None
        )
        self.support_missing_value = True

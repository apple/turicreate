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
import uuid
from turicreate.toolkits import evaluation
from turicreate.toolkits._main import ToolkitError
import shutil
import numpy as np


class FastPathPredictionTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        """
        Set up (run only once).
        """

        ## Simulate test data
        np.random.seed(10)
        n, d = 100, 10
        self.sf = tc.SFrame()
        for i in range(d):
            self.sf.add_column(tc.SArray(np.random.randn(n)), inplace=True)
        target = np.random.randint(2, size=n)
        target[0] = 0
        target[1] = 1

        ## Create the model
        self.sf["target"] = target
        self.target = "target"

        self.model = None
        self.regression = False
        self.has_probability = True
        self.has_predict_topk = False
        self.has_probability_vector = True

    def test_save_and_load(self):
        """
        Make sure saving and loading retains things.
        """
        if self.model is None:
            return

        filename = "save_file%s" % (str(uuid.uuid4()))
        old_model = self.model

        self.model.save(filename)
        self.model = tc.load_model(filename)

        try:
            self.test_predict()
            print("Predict passed")
            self.test_classify()
            print("Predict passed")
        except:
            self.assertTrue(False, "Failed during save & load diagnostics")
        finally:
            self.model = old_model
            shutil.rmtree(filename)

    def test_classify(self):
        if self.model is None:
            return

        # Arrange
        sf = self.sf
        model = self.model

        # Act & Assert
        if not self.regression:
            # Act & Assert
            bp = model.classify(sf)
            lp = model.classify(list(sf))
            dp = model.classify(sf[0])
            sf_new = sf[0].copy()
            sf_new["new_column"] = 1
            dp_new = model.classify(sf_new)

            self.assertEqual(len(dp), 1)
            self.assertEqual(list(bp), list(lp))
            self.assertEqual(bp[0], dp[0])
            self.assertEqual(lp[0], dp[0])
            self.assertEqual(lp[0], dp_new[0])

    def test_predict_topk(self):
        if self.model is None:
            return

        # Arrange
        sf = self.sf
        model = self.model
        k = 2

        # Act & Assert
        if self.has_predict_topk:
            # Act & Assert
            output_type = "rank"
            bp = model.predict_topk(sf, output_type, k)
            lp = model.predict_topk(list(sf), output_type, k)
            dp = model.predict_topk(sf[0], output_type, k)
            sf_new = sf[0].copy()
            sf_new["new_column"] = 1
            dp_new = model.predict_topk(sf_new, output_type, k)

            self.assertEqual(len(dp), 2)
            self.assertEqual(list(bp), list(lp))
            self.assertEqual(bp[0], dp[0])
            self.assertEqual(bp[0], dp_new[0])
            self.assertEqual(lp[0], dp[0])
            self.assertEqual(bp[1], dp[1])
            self.assertEqual(lp[1], dp[1])

            # Act & Assert
            output_type = "probability"
            bp = model.predict_topk(sf, output_type, k)
            lp = model.predict_topk(list(sf), output_type, k)
            dp = model.predict_topk(sf[0], output_type, k)
            dp_new = model.predict_topk(sf_new, output_type, k)

            self.assertEqual(len(dp), 2)
            self.assertEqual(list(bp), list(lp))
            self.assertEqual(bp[0], dp[0])
            self.assertEqual(bp[0], dp_new[0])
            self.assertEqual(lp[0], dp[0])
            self.assertEqual(bp[1], dp[1])
            self.assertEqual(lp[1], dp[1])

            # Act & Assert
            output_type = "margin"
            bp = model.predict_topk(sf, output_type, k)
            lp = model.predict_topk(list(sf), output_type, k)
            dp = model.predict_topk(sf[0], output_type, k)
            dp_new = model.predict_topk(sf_new, output_type, k)

            self.assertEqual(len(dp), 2)
            self.assertEqual(list(bp), list(lp))
            self.assertEqual(bp[0], dp[0])
            self.assertEqual(bp[0], dp_new[0])
            self.assertEqual(lp[0], dp[0])
            self.assertEqual(bp[1], dp[1])
            self.assertEqual(lp[1], dp[1])

    def test_predict(self):
        if self.model is None:
            return

        # Arrange
        sf = self.sf
        model = self.model

        # Act & Assert
        if not self.regression:

            # Act & Assert
            output_type = "class"
            bp = model.predict(sf, output_type)
            lp = model.predict(list(sf), output_type)
            dp = model.predict(sf[0], output_type)
            sf_new = sf[0].copy()
            sf_new["new_column"] = 1
            dp_new = model.predict(sf_new, output_type)

            self.assertEqual(len(dp), 1)
            self.assertEqual(list(bp), list(lp))
            self.assertEqual(bp[0], dp_new[0])
            self.assertEqual(bp[0], dp[0])
            self.assertEqual(lp[0], dp[0])

            # act & assert
            output_type = "margin"
            bp = model.predict(sf, output_type)
            lp = model.predict(list(sf), output_type)
            dp = model.predict(sf[0], output_type)
            dp_new = model.predict(sf_new, output_type)

            self.assertEqual(len(dp), 1)
            self.assertEqual(list(bp), list(lp))
            self.assertEqual(bp[0], dp_new[0])
            self.assertEqual(bp[0], dp[0])
            self.assertEqual(lp[0], dp[0])

            # act & assert
            if self.has_probability_vector:
                output_type = "probability_vector"
                bp = model.predict(sf, output_type)
                lp = model.predict(list(sf), output_type)
                dp = model.predict(sf[0], output_type)
                dp_new = model.predict(sf_new, output_type)

                self.assertEqual(len(dp), 1)
                self.assertEqual(list(bp), list(lp))
                self.assertEqual(bp[0], dp_new[0])
                self.assertEqual(bp[0], dp[0])
                self.assertEqual(lp[0], dp[0])

            # act & assert
            if self.has_probability:
                output_type = "probability"
                bp = model.predict(sf, output_type)
                lp = model.predict(list(sf), output_type)
                dp = model.predict(sf[0], output_type)
                dp_new = model.predict(sf_new, output_type)

                self.assertEqual(len(dp), 1)
                self.assertEqual(list(bp), list(lp))
                self.assertEqual(bp[0], dp_new[0])
                self.assertEqual(bp[0], dp[0])
                self.assertEqual(lp[0], dp[0])

        # Act & Assert
        if self.regression:
            model = model
            bp = model.predict(sf)
            lp = model.predict(list(sf))
            dp = model.predict(sf[0])
            sf_new = sf[0].copy()
            sf_new["new_column"] = 1
            dp_new = model.predict(sf_new)

            self.assertEqual(len(dp), 1)
            self.assertEqual(list(bp), list(lp))
            self.assertEqual(lp[0], dp_new[0])
            self.assertEqual(bp[0], dp[0])
            self.assertEqual(lp[0], dp[0])

    def test_wrong_input_type(self):
        if self.model is None:
            return
        # Arrange
        sf = self.sf
        model = self.model

        with self.assertRaises(ToolkitError):
            bad_input = [0, 1, 2]
            model.predict(bad_input)
        with self.assertRaises(ToolkitError):
            bad_input = ["0", "1", "2"]
            model.predict(bad_input)
        with self.assertRaises(ToolkitError):
            bad_input = [[0], [1], [2]]
            model.predict(bad_input)


class LinearRegressionTest(FastPathPredictionTest):
    @classmethod
    def setUpClass(self):
        super(LinearRegressionTest, self).setUpClass()
        self.model = tc.linear_regression.create(
            self.sf, self.target, validation_set=None
        )
        self.regression = True


class RandomForestRegressionTest(FastPathPredictionTest):
    @classmethod
    def setUpClass(self):
        super(RandomForestRegressionTest, self).setUpClass()
        self.model = tc.random_forest_regression.create(
            self.sf, self.target, validation_set=None
        )
        self.regression = True


class DecisionTreeRegressionTest(FastPathPredictionTest):
    @classmethod
    def setUpClass(self):
        super(DecisionTreeRegressionTest, self).setUpClass()
        self.model = tc.decision_tree_regression.create(
            self.sf, self.target, validation_set=None
        )
        self.regression = True


class BoostedTreesRegressionTest(FastPathPredictionTest):
    @classmethod
    def setUpClass(self):
        super(BoostedTreesRegressionTest, self).setUpClass()
        self.model = tc.boosted_trees_regression.create(
            self.sf, self.target, validation_set=None
        )
        self.regression = True


class LogisticRegressionTest(FastPathPredictionTest):
    @classmethod
    def setUpClass(self):
        super(LogisticRegressionTest, self).setUpClass()
        self.model = tc.logistic_classifier.create(
            self.sf, self.target, validation_set=None
        )
        self.has_predict_topk = True


class SVMClassifierTest(FastPathPredictionTest):
    @classmethod
    def setUpClass(self):
        super(SVMClassifierTest, self).setUpClass()
        self.model = tc.svm_classifier.create(self.sf, self.target, validation_set=None)
        self.has_probability = False
        self.has_probability_vector = False


class RandomForestClassifierTest(FastPathPredictionTest):
    @classmethod
    def setUpClass(self):
        super(RandomForestClassifierTest, self).setUpClass()
        self.model = tc.random_forest_classifier.create(
            self.sf, self.target, validation_set=None
        )
        self.has_predict_topk = True


class DecisionTreeClassifierTest(FastPathPredictionTest):
    @classmethod
    def setUpClass(self):
        super(DecisionTreeClassifierTest, self).setUpClass()
        self.model = tc.decision_tree_classifier.create(
            self.sf, self.target, validation_set=None
        )
        self.has_predict_topk = True


class BoostedTreesClassifierTest(FastPathPredictionTest):
    @classmethod
    def setUpClass(self):
        super(BoostedTreesClassifierTest, self).setUpClass()
        self.model = tc.boosted_trees_classifier.create(
            self.sf, self.target, validation_set=None
        )
        self.has_predict_topk = True


class RandomForestClassifierStringClassTest(FastPathPredictionTest):
    @classmethod
    def setUpClass(self):
        super(RandomForestClassifierStringClassTest, self).setUpClass()
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.random_forest_classifier.create(
            self.sf, self.target, validation_set=None
        )
        self.has_predict_topk = True


class DecisionTreeClassifierStringClassTest(FastPathPredictionTest):
    @classmethod
    def setUpClass(self):
        super(DecisionTreeClassifierStringClassTest, self).setUpClass()
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.decision_tree_classifier.create(
            self.sf, self.target, validation_set=None
        )
        self.has_predict_topk = True


class BoostedTreesClassifierStringClassTest(FastPathPredictionTest):
    @classmethod
    def setUpClass(self):
        super(BoostedTreesClassifierStringClassTest, self).setUpClass()
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.boosted_trees_classifier.create(
            self.sf, self.target, validation_set=None
        )
        self.has_predict_topk = True

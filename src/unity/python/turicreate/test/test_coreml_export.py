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

class CoreMLExportTest(unittest.TestCase):

    @classmethod
    def setUpClass(self, multiclass=False):

        ## Simulate test data
        rs = np.random.RandomState(10)
        n, d = 100, 10
        self.sf = tc.SFrame()
        for i in range(d):
            self.sf.add_column(tc.SArray(rs.randn(n)), inplace=True)

        # Add a categorical column
        categories = np.array(['cat', 'dog', 'foosa'])
        cat_index = rs.randint(len(categories), size=n)
        self.sf['cat_column'] = list(categories[cat_index])
        self.sf['dict_column'] = self.sf['cat_column'].apply(lambda x: {x: 1.0})
        self.sf['array_column'] = self.sf.apply(lambda x: [x['X1'] * 1.9, x['X2'] * 2.1])

        # Add a target
        target = rs.randint(2, size=n)
        target[0] = 0
        target[1] = 1

        ## Create the model
        self.sf['target'] = target
        self.target = 'target'

        self.model = None
        self.regression = False
        self.has_probability = True

        if multiclass:
            target = rs.randint(3, size=n)
            target[0] = 0
            target[1] = 1
            target[2] = 2
            self.sf[self.target] = target

    def test_coreml_export(self):
        if self.model is None:
            return

        # Arrange
        model = self.model

        # Act & Assert
        with tempfile.NamedTemporaryFile(mode='w', suffix = '.mlmodel') as mlmodel_file:
            mlmodel_filename = mlmodel_file.name
            model.export_coreml(mlmodel_filename)
            coreml_model = coremltools.models.MLModel(mlmodel_filename)

    @unittest.skipIf(_mac_ver() < (10, 13), 'Only supported on Mac')
    def test_coreml_export_with_predictions(self):
        if self.model is None:
            return

        # Arrange
        sf = self.sf
        model = self.model

        # Act & Assert
        with tempfile.NamedTemporaryFile(mode='w', suffix = '.mlmodel') as mlmodel_file:
            mlmodel_filename = mlmodel_file.name
            model.export_coreml(mlmodel_filename)
            coreml_model = coremltools.models.MLModel(mlmodel_filename)
            def array_to_numpy(row):
                import array
                import numpy
                for r in row:
                    if type(row[r]) == array.array:
                        row[r] = numpy.array(row[r])
                return row
            for row in sf:
                coreml_prediction = coreml_model.predict(array_to_numpy(row))
                tc_prediction = model.predict(row)[0]
                if (self.regression == False) and (type(model.classes[0]) == str):
                    self.assertEqual(coreml_prediction[self.target], tc_prediction)
                else:
                    self.assertAlmostEqual(coreml_prediction[self.target], tc_prediction, delta = 1e-5)

                # If applicable, compare probabilistic output
                if self.has_probability and not self.regression:
                    coreml_ret = coreml_prediction[self.target + 'Probability']
                    _, values_tuple = zip(*sorted(coreml_ret.items()))
                    coreml_probs = np.array(values_tuple)
                    tc_probs = np.array(model.predict(row, output_type='probability_vector')[0])
                    np.testing.assert_array_almost_equal(coreml_probs, tc_probs, decimal=5)


# ------------------------------------------------------------------------------
#
# Regression tests
#
# ------------------------------------------------------------------------------

class LinearRegressionTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(LinearRegressionTest, self).setUpClass()
        self.model = tc.linear_regression.create(self.sf,
                self.target, validation_set=None)
        self.regression = True

class RandomForestRegressionTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(RandomForestRegressionTest, self).setUpClass()
        self.model = tc.random_forest_regression.create(self.sf,
                self.target, validation_set=None)
        self.regression = True

class DecisionTreeRegressionTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(DecisionTreeRegressionTest, self).setUpClass()
        self.model = tc.decision_tree_regression.create(self.sf,
                self.target, validation_set=None)
        self.regression = True

class BoostedTreesRegressionTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(BoostedTreesRegressionTest, self).setUpClass()
        self.model = tc.boosted_trees_regression.create(self.sf,
                             self.target, validation_set=None)
        self.regression = True

# ------------------------------------------------------------------------------
#
#  Binary classifier tests
#
# ------------------------------------------------------------------------------

class LogisticRegressionTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(LogisticRegressionTest, self).setUpClass()
        self.model = tc.logistic_classifier.create(self.sf,
                      self.target, validation_set=None)

class SVMClassifierTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(SVMClassifierTest, self).setUpClass()
        self.model = tc.svm_classifier.create(self.sf, self.target,
                               validation_set=None)
        self.has_probability = False

class RandomForestClassifierTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(RandomForestClassifierTest, self).setUpClass()
        self.model = tc.random_forest_classifier.create(self.sf, self.target,
                  validation_set=None)

class DecisionTreeClassifierTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(DecisionTreeClassifierTest, self).setUpClass()
        self.model = tc.decision_tree_classifier.create(self.sf, self.target,
                  validation_set=None)

class BoostedTreesClassifierTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(BoostedTreesClassifierTest, self).setUpClass()
        self.model = tc.boosted_trees_classifier.create(self.sf,
                                  self.target, validation_set=None)

class LogisticRegressionStringTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(LogisticRegressionStringTest, self).setUpClass()
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.logistic_classifier.create(self.sf,
                      self.target, validation_set=None)
        self.sf[self.target] = self.sf[self.target].astype(int)

class SVMClassifierStringTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(SVMClassifierStringTest, self).setUpClass()
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.svm_classifier.create(self.sf, self.target,
                               validation_set=None)
        self.sf[self.target] = self.sf[self.target].astype(int)
        self.has_probability = False

class RandomForestClassifierStringClassTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(RandomForestClassifierStringClassTest, self).setUpClass()
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.random_forest_classifier.create(self.sf, self.target,
                  validation_set=None)
        self.sf[self.target] = self.sf[self.target].astype(int)
        self.has_predict_topk = True

class DecisionTreeClassifierStringClassTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(DecisionTreeClassifierStringClassTest, self).setUpClass()
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.decision_tree_classifier.create(self.sf, self.target,
                  validation_set=None)
        self.sf[self.target] = self.sf[self.target].astype(int)

class BoostedTreesClassifierStringClassTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(BoostedTreesClassifierStringClassTest, self).setUpClass()
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.boosted_trees_classifier.create(self.sf,
                                  self.target, validation_set=None)
        self.sf[self.target] = self.sf[self.target].astype(int)

# ------------------------------------------------------------------------------
#
#  Multiclass tests
#
# ------------------------------------------------------------------------------

class LogisticRegressionMultiClassTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(LogisticRegressionMultiClassTest, self).setUpClass(multiclass=True)
        self.model = tc.logistic_classifier.create(self.sf,
                      self.target, validation_set=None)

class RandomForestClassifierMultiClassTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(RandomForestClassifierMultiClassTest, self).setUpClass(multiclass=True)
        self.model = tc.random_forest_classifier.create(self.sf, self.target,
                  validation_set=None)

class DecisionTreeClassifierMultiClassTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(DecisionTreeClassifierMultiClassTest, self).setUpClass(multiclass=True)
        self.model = tc.decision_tree_classifier.create(self.sf, self.target,
                  validation_set=None)

class BoostedTreesClassifierMultiClassTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(BoostedTreesClassifierMultiClassTest, self).setUpClass(multiclass=True)
        self.model = tc.boosted_trees_classifier.create(self.sf,
                                  self.target, validation_set=None)

class LogisticRegressionStringMultiClassTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(LogisticRegressionStringMultiClassTest, self).setUpClass(multiclass=True)
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.logistic_classifier.create(self.sf,
                      self.target, validation_set=None)
        self.sf[self.target] = self.sf[self.target].astype(int)

class RandomForestClassifierStringMultiClassTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(RandomForestClassifierStringMultiClassTest, self).setUpClass(multiclass=True)
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.random_forest_classifier.create(self.sf, self.target,
                  validation_set=None)
        self.sf[self.target] = self.sf[self.target].astype(int)
        self.has_predict_topk = True

class DecisionTreeClassifierStringMultiClassTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(DecisionTreeClassifierStringMultiClassTest, self).setUpClass(multiclass=True)
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.decision_tree_classifier.create(self.sf, self.target,
                  validation_set=None)
        self.sf[self.target] = self.sf[self.target].astype(int)

class BoostedTreesClassifierStringMultiClassTest(CoreMLExportTest):
    @classmethod
    def setUpClass(self):
        super(BoostedTreesClassifierStringMultiClassTest, self).setUpClass(multiclass=True)
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.boosted_trees_classifier.create(self.sf,
                                  self.target, validation_set=None)
        self.sf[self.target] = self.sf[self.target].astype(int)

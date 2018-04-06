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

class CoreMLExportTest(object):


    @classmethod
    def setUpClass(self, testtype):

        ## Simulate test data
        n = 100
        code_string = "bnsvAd"

        # numeric; integer, string categorical, list categorical, dictionary,
        # array, nd array (1 dim), nd array (4 dim).

        if testtype == "regression":
            self.sf = tc.util.generate_random_regression_sframe(n, code_string  )
            self.regression = True
        elif testtype == "classification":
            self.sf = tc.util.generate_random_classification_sframe(n, code_string, 2)
            self.regression = False
        elif testtype == "multiclass":
            self.sf = tc.util.generate_random_classification_sframe(n, code_string, 10)
            self.regression = False
        else:
            assert False

        self.test_sf = tc.util.generate_random_sframe(n, code_string, random_seed = 111)
        self.target = "target"

        # Ones with additional classes not present in the training set.
        self.add_cat_sf_val = self.sf.copy()
        self.add_cat_sf_val[self.target] = self.sf[self.target] + 10

        # May be overwritten
        self.has_probability = True
        self.has_predict_topk = False

    def test_coreml_export_new_data(self):

        # Arrange
        model = self.model
        test_data = self.sf[:]
        test_data['cat_column'] = 'new_cat'
        test_data['dict_column'] = [{'new_cat': 1} for i in range(len(test_data))]

        # Assert
        model.predict(test_data)
        with tempfile.NamedTemporaryFile(mode='w', suffix = '.mlmodel') as mlmodel_file:
            mlmodel_filename = mlmodel_file.name
            model.export_coreml(mlmodel_filename)

    def test_coreml_export(self):

        # Arrange
        model = self.model

        # Act & Assert
        with tempfile.NamedTemporaryFile(mode='w', suffix = '.mlmodel') as mlmodel_file:
            mlmodel_filename = mlmodel_file.name
            model.export_coreml(mlmodel_filename)
            coreml_model = coremltools.models.MLModel(mlmodel_filename)

    @unittest.skipIf(_mac_ver() < (10, 13), 'Only supported on Mac')
    def test_coreml_export_with_predictions(self):

        model = self.model

        # Act & Assert
        with tempfile.NamedTemporaryFile(mode='w', suffix = '.mlmodel') as mlmodel_file:
            mlmodel_filename = mlmodel_file.name
            model.export_coreml(mlmodel_filename)
            coreml_model = coremltools.models.MLModel(mlmodel_filename)

            # print(coreml_model.get_spec())

            # import shutil
            # shutil.copyfile(mlmodel_filename, "./bt.mlmodel")

            # model.save("./bt.model")

            def array_to_numpy(row):
                import array
                import numpy
                import copy
                row = copy.copy(row)
                for r in row:
                    if type(row[r]) == array.array:
                        row[r] = numpy.array(row[r])
                return row


            for row in self.test_sf:

                # print(row)

                coreml_prediction = coreml_model.predict(array_to_numpy(row))

                tc_prediction = model.predict(row)[0]

                if (self.regression == False) and (type(model.classes[0]) == str):
                    if not self.has_probability:
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

class LinearRegressionTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(LinearRegressionTest, self).setUpClass("regression")
        self.model = tc.linear_regression.create(self.sf,
                self.target, validation_set=None)

class RandomForestRegressionTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(RandomForestRegressionTest, self).setUpClass("regression")
        self.model = tc.random_forest_regression.create(self.sf,
                self.target, validation_set=None)

class DecisionTreeRegressionTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(DecisionTreeRegressionTest, self).setUpClass("regression")
        self.model = tc.decision_tree_regression.create(self.sf,
                self.target, validation_set=None)

class BoostedTreesRegressionTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(BoostedTreesRegressionTest, self).setUpClass("regression")
        self.model = tc.boosted_trees_regression.create(self.sf,
                             self.target, validation_set=None, max_iterations=1, max_depth=1)

# ------------------------------------------------------------------------------
#
#  Binary classifier tests
#
# ------------------------------------------------------------------------------

class LogisticRegressionTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(LogisticRegressionTest, self).setUpClass("classification")
        self.model = tc.logistic_classifier.create(self.sf,
                      self.target, validation_set=None)

class SVMClassifierTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(SVMClassifierTest, self).setUpClass("classification")
        self.model = tc.svm_classifier.create(self.sf, self.target,
                               validation_set=None)
        self.has_probability = False

class RandomForestClassifierTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(RandomForestClassifierTest, self).setUpClass("classification")
        self.model = tc.random_forest_classifier.create(self.sf, self.target,
                  validation_set=None)

class DecisionTreeClassifierTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(DecisionTreeClassifierTest, self).setUpClass("classification")
        self.model = tc.decision_tree_classifier.create(self.sf, self.target,
                  validation_set=None)

class BoostedTreesClassifierTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(BoostedTreesClassifierTest, self).setUpClass("classification")
        self.model = tc.boosted_trees_classifier.create(self.sf,
                                  self.target, validation_set=None)

class LogisticRegressionStringTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(LogisticRegressionStringTest, self).setUpClass("classification")
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.logistic_classifier.create(self.sf,
                      self.target, validation_set=None)
        self.sf[self.target] = self.sf[self.target].astype(int)

class SVMClassifierStringTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(SVMClassifierStringTest, self).setUpClass("classification")
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.svm_classifier.create(self.sf, self.target,
                               validation_set=None)
        self.sf[self.target] = self.sf[self.target].astype(int)
        self.has_probability = False

class RandomForestClassifierStringClassTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(RandomForestClassifierStringClassTest, self).setUpClass("classification")
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.random_forest_classifier.create(self.sf, self.target,
                  validation_set=None)
        self.sf[self.target] = self.sf[self.target].astype(int)
        self.has_predict_topk = True

class DecisionTreeClassifierStringClassTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(DecisionTreeClassifierStringClassTest, self).setUpClass("classification")
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.decision_tree_classifier.create(self.sf, self.target,
                  validation_set=None)
        self.sf[self.target] = self.sf[self.target].astype(int)

class BoostedTreesClassifierStringClassTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(BoostedTreesClassifierStringClassTest, self).setUpClass("classification")
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.boosted_trees_classifier.create(self.sf,
                                  self.target, validation_set=None)
        self.sf[self.target] = self.sf[self.target].astype(int)


# ------------------------------------------------------------------------------
#
#  Multiclass tests with additional validation data
#
# ------------------------------------------------------------------------------

class LogisticRegressionMultiClassTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(LogisticRegressionMultiClassTest, self).setUpClass("multiclass")
        self.model = tc.logistic_classifier.create(self.sf,
                      self.target, validation_set=self.sf)

class RandomForestClassifierMultiClassTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(RandomForestClassifierMultiClassTest, self).setUpClass("multiclass")
        self.model = tc.random_forest_classifier.create(self.sf, self.target,
                  validation_set=None)

class DecisionTreeClassifierMultiClassTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(DecisionTreeClassifierMultiClassTest, self).setUpClass("multiclass")
        self.model = tc.decision_tree_classifier.create(self.sf, self.target,
                  validation_set=None)

class BoostedTreesClassifierMultiClassTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(BoostedTreesClassifierMultiClassTest, self).setUpClass("multiclass")
        self.model = tc.boosted_trees_classifier.create(self.sf,
                                  self.target, validation_set=None)

class LogisticRegressionStringMultiClassTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(LogisticRegressionStringMultiClassTest, self).setUpClass("multiclass")
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.logistic_classifier.create(self.sf,
                      self.target, validation_set=None)
        self.sf[self.target] = self.sf[self.target].astype(int)

class RandomForestClassifierStringMultiClassTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(RandomForestClassifierStringMultiClassTest, self).setUpClass("multiclass")
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.random_forest_classifier.create(self.sf, self.target,
                  validation_set=None)
        self.sf[self.target] = self.sf[self.target].astype(int)
        self.has_predict_topk = True

class DecisionTreeClassifierStringMultiClassTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(DecisionTreeClassifierStringMultiClassTest, self).setUpClass("multiclass")
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.decision_tree_classifier.create(self.sf, self.target,
                  validation_set=None)
        self.sf[self.target] = self.sf[self.target].astype(int)

class BoostedTreesClassifierStringMultiClassTest(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(BoostedTreesClassifierStringMultiClassTest, self).setUpClass("multiclass")
        self.sf[self.target] = self.sf[self.target].astype(str)
        self.model = tc.boosted_trees_classifier.create(self.sf,
                                  self.target, validation_set=None)
        self.sf[self.target] = self.sf[self.target].astype(int)# ------------------------------------------------------------------------------
#
#  Multiclass tests
#
# ------------------------------------------------------------------------------

class LogisticRegressionMultiClassTest_BadValidationCategories(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(LogisticRegressionMultiClassTest_BadValidationCategories, self).setUpClass("multiclass")
        self.model = tc.logistic_classifier.create(self.sf,
                      self.target, validation_set=self.add_cat_sf_val)

class RandomForestClassifierMultiClassTest_BadValidationCategories(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(RandomForestClassifierMultiClassTest_BadValidationCategories, self).setUpClass("multiclass")
        self.model = tc.random_forest_classifier.create(self.sf, self.target,
                  validation_set=self.add_cat_sf_val)

class DecisionTreeClassifierMultiClassTest_BadValidationCategories(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(DecisionTreeClassifierMultiClassTest_BadValidationCategories, self).setUpClass("multiclass")
        self.model = tc.decision_tree_classifier.create(self.sf, self.target,
                  validation_set=self.add_cat_sf_val)

class BoostedTreesClassifierMultiClassTest_BadValidationCategories(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(BoostedTreesClassifierMultiClassTest_BadValidationCategories, self).setUpClass("multiclass")
        self.model = tc.boosted_trees_classifier.create(self.sf,
                                  self.target, validation_set=self.add_cat_sf_val)

class LogisticRegressionStringMultiClassTest_BadValidationCategories(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(LogisticRegressionStringMultiClassTest_BadValidationCategories, self).setUpClass("multiclass")
        self.model = tc.logistic_classifier.create(self.sf,
                      self.target, validation_set=self.add_cat_sf_val)

class RandomForestClassifierStringMultiClassTest_BadValidationCategories(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(RandomForestClassifierStringMultiClassTest_BadValidationCategories, self).setUpClass("multiclass")
        self.model = tc.random_forest_classifier.create(self.sf, self.target,
                  validation_set=self.add_cat_sf_val)
        self.has_predict_topk = True

class DecisionTreeClassifierStringMultiClassTest_BadValidationCategories(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(DecisionTreeClassifierStringMultiClassTest_BadValidationCategories, self).setUpClass("multiclass")
        self.model = tc.decision_tree_classifier.create(self.sf, self.target,
                  validation_set=self.add_cat_sf_val)

class BoostedTreesClassifierStringMultiClassTest_BadValidationCategories(CoreMLExportTest,unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(BoostedTreesClassifierStringMultiClassTest_BadValidationCategories, self).setUpClass("multiclass")
        self.model = tc.boosted_trees_classifier.create(self.sf,
                                  self.target, validation_set=self.add_cat_sf_val)


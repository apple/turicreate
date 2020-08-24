# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import os
import pandas as pd
import unittest
from coremltools._deps import _HAS_SKLEARN
from coremltools.models.utils import evaluate_regressor, _macos_version, _is_macos

if _HAS_SKLEARN:
    from sklearn.linear_model import LinearRegression
    from sklearn.svm import LinearSVR
    from coremltools.converters.sklearn import convert


@unittest.skipIf(not _HAS_SKLEARN, "Missing scikitlearn. Skipping tests.")
class LinearRegressionScikitTest(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading the dataset and training a model.
        """
        from sklearn.datasets import load_boston
        from sklearn.linear_model import LinearRegression

        scikit_data = load_boston()
        scikit_model = LinearRegression()
        scikit_model.fit(scikit_data["data"], scikit_data["target"])

        # Save the data and the model
        self.scikit_data = scikit_data
        self.scikit_model = scikit_model

    def test_conversion(self):
        input_names = self.scikit_data.feature_names
        spec = convert(self.scikit_model, input_names, "target").get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)

        # Test the interface class
        self.assertEquals(spec.description.predictedFeatureName, "target")

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.output), 1)
        self.assertEquals(spec.description.output[0].name, "target")
        self.assertEquals(
            spec.description.output[0].type.WhichOneof("Type"), "doubleType"
        )
        for input_type in spec.description.input:
            self.assertEquals(input_type.type.WhichOneof("Type"), "doubleType")
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )

        # Test the linear regression parameters.
        self.assertTrue(
            spec.pipelineRegressor.pipeline.models[-1].HasField("glmRegressor")
        )
        lr = spec.pipelineRegressor.pipeline.models[-1].glmRegressor
        self.assertEquals(lr.offset, self.scikit_model.intercept_)
        self.assertEquals(len(lr.weights), 1)
        self.assertEquals(len(lr.weights[0].value), 13)
        i = 0
        for w in lr.weights[0].value:
            self.assertAlmostEqual(w, self.scikit_model.coef_[i])
            i = i + 1

    def test_conversion_bad_inputs(self):
        # Error on converting an untrained model
        with self.assertRaises(TypeError):
            model = LinearRegression()
            spec = convert(model, "data", "out")

        # Check the expected class during covnersion.
        from sklearn.preprocessing import OneHotEncoder

        with self.assertRaises(TypeError):
            model = OneHotEncoder()
            spec = convert(model, "data", "out")

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_linear_regression_evaluation(self):
        """
        Check that the evaluation results are the same in scikit learn and coremltools
        """
        input_names = self.scikit_data.feature_names
        df = pd.DataFrame(self.scikit_data.data, columns=input_names)

        for normalize_value in (True, False):
            cur_model = LinearRegression(normalize=normalize_value)
            cur_model.fit(self.scikit_data["data"], self.scikit_data["target"])
            spec = convert(cur_model, input_names, "target")

            df["prediction"] = cur_model.predict(self.scikit_data.data)

            metrics = evaluate_regressor(spec, df)
            self.assertAlmostEquals(metrics["max_error"], 0)

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_linear_svr_evaluation(self):
        """
        Check that the evaluation results are the same in scikit learn and coremltools
        """
        ARGS = [
            {},
            {"C": 0.5, "epsilon": 0.25},
            {"dual": False, "loss": "squared_epsilon_insensitive"},
            {"tol": 0.005},
            {"fit_intercept": False},
            {"intercept_scaling": 1.5},
        ]

        input_names = self.scikit_data.feature_names
        df = pd.DataFrame(self.scikit_data.data, columns=input_names)

        for cur_args in ARGS:
            print(cur_args)
            cur_model = LinearSVR(**cur_args)
            cur_model.fit(self.scikit_data["data"], self.scikit_data["target"])
            spec = convert(cur_model, input_names, "target")

            df["prediction"] = cur_model.predict(self.scikit_data.data)

            metrics = evaluate_regressor(spec, df)
            self.assertAlmostEquals(metrics["max_error"], 0)

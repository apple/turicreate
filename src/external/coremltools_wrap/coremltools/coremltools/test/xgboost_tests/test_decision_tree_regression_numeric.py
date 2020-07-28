# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import unittest
from coremltools.converters import sklearn as skl_converter
from coremltools.models.utils import evaluate_regressor
import pandas as pd
import os
from coremltools.models.utils import evaluate_regressor, _macos_version, _is_macos
from coremltools._deps import _HAS_SKLEARN
import pytest

if _HAS_SKLEARN:
    from sklearn.tree import DecisionTreeRegressor
    from coremltools.converters.sklearn import _decision_tree_regressor as skl_converter


@unittest.skipIf(not _HAS_SKLEARN, "Missing sklearn. Skipping tests.")
class DecisionTreeRegressorBostonHousingScikitNumericTest(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter and running both models
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading the dataset and training a model.
        """
        from sklearn.datasets import load_boston
        from sklearn.tree import DecisionTreeRegressor

        # Load data and train model
        scikit_data = load_boston()
        self.scikit_data = scikit_data
        self.X = scikit_data["data"]
        self.target = scikit_data["target"]
        self.feature_names = scikit_data.feature_names
        self.output_name = "target"

    def _check_metrics(self, metrics, params={}):
        """
        Check the metrics
        """
        self.assertAlmostEquals(
            metrics["rmse"],
            0,
            delta=1e-5,
            msg="Failed case %s. Results %s" % (params, metrics),
        )
        self.assertAlmostEquals(
            metrics["max_error"],
            0,
            delta=1e-5,
            msg="Failed case %s. Results %s" % (params, metrics),
        )

    def _train_convert_evaluate_assert(self, **scikit_params):
        """
        Train a scikit-learn model, convert it and then evaluate it with CoreML
        """
        scikit_model = DecisionTreeRegressor(random_state=1, **scikit_params)
        scikit_model.fit(self.X, self.target)

        # Convert the model
        spec = skl_converter.convert(scikit_model, self.feature_names, self.output_name)

        if _is_macos() and _macos_version() >= (10, 13):
            # Get predictions
            df = pd.DataFrame(self.X, columns=self.feature_names)
            df["prediction"] = scikit_model.predict(self.X)

            # Evaluate it
            metrics = evaluate_regressor(spec, df, target="target", verbose=False)
            self._check_metrics(metrics, scikit_params)

    def test_boston_housing_simple_regression(self):
        self._train_convert_evaluate_assert(max_depth=20)

    @pytest.mark.slow
    def test_boston_housing_parameter_stress_test(self):

        ## These are all the options in decision tree regression of scikit-learn
        options = dict(
            criterion=["mse"],
            splitter=["best"],
            max_depth=[1, 10, None],
            min_samples_split=[2, 10, 0.5],
            min_samples_leaf=[1, 5],
            min_weight_fraction_leaf=[0.0, 0.5],
            max_features=[None, 1, 5],
            max_leaf_nodes=[None, 20],
            min_impurity_decrease=[0.0, 1e-07, 0.1],
            presort=[False, True],
        )

        # Make a cartesian product of all options
        import itertools

        product = itertools.product(*options.values())
        args = [dict(zip(options.keys(), p)) for p in product]

        print("Testing a total of %s cases. This could take a while" % len(args))
        for it, arg in enumerate(args):
            self._train_convert_evaluate_assert(**arg)

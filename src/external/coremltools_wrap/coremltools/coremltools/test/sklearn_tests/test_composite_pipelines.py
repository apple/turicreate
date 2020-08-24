# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import unittest
import os

import pandas as pd
import itertools
import numpy as np

from coremltools._deps import _HAS_SKLEARN
from coremltools.models.utils import evaluate_transformer
from coremltools.models.utils import evaluate_regressor
from coremltools.models.utils import _macos_version, _is_macos
from coremltools.converters.sklearn import convert

if _HAS_SKLEARN:
    from sklearn.datasets import load_boston
    from sklearn.ensemble import GradientBoostingRegressor
    from sklearn.pipeline import Pipeline
    from sklearn.preprocessing import StandardScaler
    from sklearn.preprocessing import OneHotEncoder


@unittest.skipIf(not _HAS_SKLEARN, "Missing sklearn. Skipping tests.")
class GradientBoostingRegressorBostonHousingScikitNumericTest(unittest.TestCase):
    def test_boston_OHE_plus_normalizer(self):

        data = load_boston()

        pl = Pipeline(
            [
                ("OHE", OneHotEncoder(categorical_features=[8], sparse=False)),
                ("Scaler", StandardScaler()),
            ]
        )

        pl.fit(data.data, data.target)

        # Convert the model
        spec = convert(pl, data.feature_names, "out")

        if _is_macos() and _macos_version() >= (10, 13):
            input_data = [dict(zip(data.feature_names, row)) for row in data.data]
            output_data = [{"out": row} for row in pl.transform(data.data)]

            result = evaluate_transformer(spec, input_data, output_data)
            assert result["num_errors"] == 0

    def test_boston_OHE_plus_trees(self):

        data = load_boston()

        pl = Pipeline(
            [
                ("OHE", OneHotEncoder(categorical_features=[8], sparse=False)),
                ("Trees", GradientBoostingRegressor(random_state=1)),
            ]
        )

        pl.fit(data.data, data.target)

        # Convert the model
        spec = convert(pl, data.feature_names, "target")

        if _is_macos() and _macos_version() >= (10, 13):
            # Get predictions
            df = pd.DataFrame(data.data, columns=data.feature_names)
            df["prediction"] = pl.predict(data.data)

            # Evaluate it
            result = evaluate_regressor(spec, df, "target", verbose=False)

            assert result["max_error"] < 0.0001

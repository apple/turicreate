# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import unittest
from coremltools._deps import _HAS_SKLEARN
import numpy.random as rn
import numpy as np
from coremltools.models.utils import evaluate_transformer, _macos_version, _is_macos

if _HAS_SKLEARN:
    from sklearn.preprocessing import Imputer
    from coremltools.converters import sklearn as converter


@unittest.skipUnless(
    _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
)
@unittest.skipIf(not _HAS_SKLEARN, "Missing sklearn. Skipping tests.")
class NumericalImputerTestCase(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    def test_conversion_boston(self):

        from sklearn.datasets import load_boston

        scikit_data = load_boston()

        sh = scikit_data.data.shape

        rn.seed(0)
        missing_value_indices = [
            (rn.randint(sh[0]), rn.randint(sh[1])) for k in range(sh[0])
        ]

        for strategy in ["mean", "median", "most_frequent"]:
            for missing_value in [0, "NaN", -999]:

                X = np.array(scikit_data.data).copy()

                for i, j in missing_value_indices:
                    X[i, j] = missing_value

                model = Imputer(missing_values=missing_value, strategy=strategy)
                model = model.fit(X)

                tr_X = model.transform(X.copy())

                spec = converter.convert(model, scikit_data.feature_names, "out")

                input_data = [dict(zip(scikit_data.feature_names, row)) for row in X]

                output_data = [{"out": row} for row in tr_X]

                result = evaluate_transformer(spec, input_data, output_data)

                assert result["num_errors"] == 0

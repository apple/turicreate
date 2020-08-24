# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import numpy as _np
import random
import unittest

from coremltools.models.utils import evaluate_transformer, _macos_version, _is_macos

from coremltools._deps import _HAS_SKLEARN

if _HAS_SKLEARN:
    from sklearn.preprocessing import Normalizer
    from coremltools.converters import sklearn as converter


@unittest.skipUnless(
    _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
)
@unittest.skipIf(not _HAS_SKLEARN, "Missing sklearn. Skipping tests.")
class NormalizerScikitTest(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    def test_random(self):
        # Generate some random data_imputeValue.multiArrayValue[i]
        X = _np.random.random(size=(50, 3))

        for param in ("l1", "l2", "max"):
            cur_model = Normalizer(norm=param)

            output = cur_model.fit_transform(X)

            spec = converter.convert(cur_model, ["a", "b", "c"], "out")

            evaluate_transformer(
                spec,
                [dict(zip(["a", "b", "c"], row)) for row in X],
                [{"out": row} for row in output],
            )

    def test_boston(self):
        from sklearn.datasets import load_boston

        scikit_data = load_boston()
        scikit_model = Normalizer(norm="l2").fit(scikit_data.data)

        spec = converter.convert(scikit_model, scikit_data.feature_names, "out")

        input_data = [
            dict(zip(scikit_data.feature_names, row)) for row in scikit_data.data
        ]

        output_data = [{"out": row} for row in scikit_model.transform(scikit_data.data)]

        evaluate_transformer(spec, input_data, output_data)

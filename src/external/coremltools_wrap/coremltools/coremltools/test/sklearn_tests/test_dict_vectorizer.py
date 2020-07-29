# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import unittest
from coremltools._deps import _HAS_SKLEARN
from copy import copy
import numpy as np
from coremltools.models.utils import evaluate_transformer
from coremltools.models.utils import evaluate_classifier
from coremltools.models.utils import _macos_version, _is_macos

if _HAS_SKLEARN:
    from coremltools.converters import sklearn
    from sklearn.feature_extraction import DictVectorizer


@unittest.skipIf(not _HAS_SKLEARN, "Missing sklearn. Skipping tests.")
class DictVectorizerScikitTest(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    def _test_conversion(self, data, trained_dict_vectorizer):

        X = trained_dict_vectorizer.transform(data)

        m = sklearn.convert(
            trained_dict_vectorizer,
            input_features="features",
            output_feature_names="output",
        )

        if _is_macos() and _macos_version() >= (10, 13):
            ret = evaluate_transformer(
                m,
                [{"features": row} for row in data],
                [{"output": x_r} for x_r in X],
                True,
            )
            assert ret["num_errors"] == 0

    def test_dictvectorizer(self):

        D = [
            {"foo": 1, "bar": 3},
            {"bar": 4, "baz": 2},
            {"bar": 1, "quux": 1, "quuux": 2},
        ]

        for sparse in (True, False):
            for dtype in (int, np.float32, np.int16):
                for sort in (True, False):
                    v = DictVectorizer(sparse=sparse, dtype=dtype, sort=sort)
                    v = v.fit(D)
                    self._test_conversion(D, v)

    def test_unseen_or_no_features(self):
        D1 = [{"camelot": 0, "spamalot": 1}]
        D2 = [{}, {"nothing": 21}]

        for sparse in (True, False):
            for dtype in (int, np.float32, np.int16):
                for sort in (True, False):
                    v = DictVectorizer(sparse=sparse, dtype=dtype, sort=sort)
                    v = v.fit(D1)
                    self._test_conversion(D2, v)

    def test_int_features_in_pipeline(self):

        import numpy.random as rn
        import pandas as pd

        rn.seed(0)

        x_train_dict = [
            dict((rn.randint(100), 1) for i in range(20)) for j in range(100)
        ]
        y_train = [0, 1] * 50

        from sklearn.pipeline import Pipeline
        from sklearn.feature_extraction import DictVectorizer
        from sklearn.linear_model import LogisticRegression

        pl = Pipeline([("dv", DictVectorizer()), ("lm", LogisticRegression())])
        pl.fit(x_train_dict, y_train)

        import coremltools

        model = coremltools.converters.sklearn.convert(
            pl, input_features="features", output_feature_names="target"
        )

        if _is_macos() and _macos_version() >= (10, 13):
            x = pd.DataFrame(
                {"features": x_train_dict, "prediction": pl.predict(x_train_dict)}
            )

            cur_eval_metics = evaluate_classifier(model, x)
            self.assertEquals(cur_eval_metics["num_errors"], 0)

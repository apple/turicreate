# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from copy import copy
import unittest
import numpy as np

from coremltools._deps import _HAS_SKLEARN
from coremltools.models.utils import evaluate_transformer, _macos_version, _is_macos

if _HAS_SKLEARN:
    from sklearn.pipeline import Pipeline
    from sklearn.preprocessing import OneHotEncoder
    from sklearn.preprocessing import Normalizer
    from coremltools.converters import sklearn
    from coremltools.models.datatypes import Array
    from sklearn.datasets import load_boston


@unittest.skipIf(not _HAS_SKLEARN, "Missing sklearn. Skipping tests.")
class OneHotEncoderScikitTest(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading the dataset and training a model.
        """
        scikit_data = [[0], [1], [2], [4], [3], [2], [4], [5], [6], [7]]
        scikit_data_multiple_cols = [[0, 1], [1, 0], [2, 2], [3, 3], [4, 4]]
        scikit_model = OneHotEncoder()
        scikit_model.fit(scikit_data)

        # Save the data and the model
        self.scikit_data = np.asarray(scikit_data, dtype="d")
        self.scikit_data_multiple_cols = np.asarray(
            scikit_data_multiple_cols, dtype="d"
        )
        self.scikit_model = scikit_model

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_conversion_one_column(self):
        # Fit a single OHE
        scikit_model = OneHotEncoder()
        scikit_model.fit(self.scikit_data)
        spec = sklearn.convert(scikit_model, "single_feature", "out").get_spec()

        test_data = [{"single_feature": row} for row in self.scikit_data]
        scikit_output = [
            {"out": row} for row in scikit_model.transform(self.scikit_data).toarray()
        ]
        metrics = evaluate_transformer(spec, test_data, scikit_output)

        self.assertIsNotNone(spec)
        self.assertIsNotNone(spec.description)
        self.assertEquals(metrics["num_errors"], 0)

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_conversion_many_columns(self):
        scikit_model = OneHotEncoder()
        scikit_model.fit(self.scikit_data_multiple_cols)
        spec = sklearn.convert(
            scikit_model, ["feature_1", "feature_2"], "out"
        ).get_spec()

        test_data = [
            {"feature_1": row[0], "feature_2": row[1]}
            for row in self.scikit_data_multiple_cols
        ]
        scikit_output = [
            {"out": row}
            for row in scikit_model.transform(self.scikit_data_multiple_cols).toarray()
        ]
        metrics = evaluate_transformer(spec, test_data, scikit_output)

        self.assertIsNotNone(spec)
        self.assertIsNotNone(spec.description)
        self.assertEquals(metrics["num_errors"], 0)

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_conversion_one_column_of_several(self):
        scikit_model = OneHotEncoder(categorical_features=[0])
        scikit_model.fit(copy(self.scikit_data_multiple_cols))
        spec = sklearn.convert(
            scikit_model, ["feature_1", "feature_2"], "out"
        ).get_spec()

        test_data = [
            {"feature_1": row[0], "feature_2": row[1]}
            for row in self.scikit_data_multiple_cols
        ]
        scikit_output = [
            {"out": row}
            for row in scikit_model.transform(self.scikit_data_multiple_cols).toarray()
        ]
        metrics = evaluate_transformer(spec, test_data, scikit_output)

        self.assertIsNotNone(spec)
        self.assertIsNotNone(spec.description)
        self.assertEquals(metrics["num_errors"], 0)

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_boston_OHE(self):
        data = load_boston()

        for categorical_features in [[3], [8], [3, 8], [8, 3]]:
            model = OneHotEncoder(
                categorical_features=categorical_features, sparse=False
            )
            model.fit(data.data, data.target)

            # Convert the model
            spec = sklearn.convert(model, data.feature_names, "out").get_spec()

            input_data = [dict(zip(data.feature_names, row)) for row in data.data]
            output_data = [{"out": row} for row in model.transform(data.data)]

            result = evaluate_transformer(spec, input_data, output_data)

            assert result["num_errors"] == 0

    # This test still isn't working
    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_boston_OHE_pipeline(self):
        data = load_boston()

        for categorical_features in [[3], [8], [3, 8], [8, 3]]:
            # Put it in a pipeline so that we can test whether the output dimension
            # handling is correct.

            model = Pipeline(
                [
                    ("OHE", OneHotEncoder(categorical_features=categorical_features)),
                    ("Normalizer", Normalizer()),
                ]
            )

            model.fit(data.data.copy(), data.target)

            # Convert the model
            spec = sklearn.convert(model, data.feature_names, "out").get_spec()

            input_data = [dict(zip(data.feature_names, row)) for row in data.data]
            output_data = [{"out": row} for row in model.transform(data.data.copy())]

            result = evaluate_transformer(spec, input_data, output_data)

            assert result["num_errors"] == 0

    @unittest.skipUnless(
        _is_macos() and _macos_version() >= (10, 13), "Only supported on macOS 10.13+"
    )
    def test_random_sparse_data(self):

        n_columns = 8
        n_categories = 20

        import numpy.random as rn

        rn.seed(0)
        categories = rn.randint(50000, size=(n_columns, n_categories))

        for dt in ["int32", "float32", "float64"]:

            _X = np.array(
                [
                    [categories[j, rn.randint(n_categories)] for j in range(n_columns)]
                    for i in range(100)
                ],
                dtype=dt,
            )

            # Test this data on a bunch of possible inputs.
            for sparse in (True, False):
                for categorical_features in [
                    "all",
                    [3],
                    [4],
                    range(2, 8),
                    range(0, 4),
                    range(0, 8),
                ]:
                    X = _X.copy()

                    # This appears to be the only type now working.
                    assert X.dtype == np.dtype(dt)

                    model = OneHotEncoder(
                        categorical_features=categorical_features, sparse=sparse
                    )
                    model.fit(X)

                    # Convert the model
                    spec = sklearn.convert(model, [("data", Array(n_columns))], "out")

                    X_out = model.transform(X)
                    if sparse:
                        X_out = X_out.todense()

                    input_data = [{"data": row} for row in X]
                    output_data = [{"out": row} for row in X_out]

                    result = evaluate_transformer(spec, input_data, output_data)

                    assert result["num_errors"] == 0

            # Test normal data inside a pipeline
            for sparse in (True, False):
                for categorical_features in [
                    "all",
                    [3],
                    [4],
                    range(2, 8),
                    range(0, 4),
                    range(0, 8),
                ]:
                    X = _X.copy()

                    model = Pipeline(
                        [
                            (
                                "OHE",
                                OneHotEncoder(
                                    categorical_features=categorical_features,
                                    sparse=sparse,
                                ),
                            ),
                            ("Normalizer", Normalizer()),
                        ]
                    )

                    model.fit(X)

                    # Convert the model
                    spec = sklearn.convert(
                        model, [("data", Array(n_columns))], "out"
                    ).get_spec()

                    X_out = model.transform(X)
                    if sparse:
                        X_out = X_out.todense()

                    input_data = [{"data": row} for row in X]
                    output_data = [{"out": row} for row in X_out]

                    result = evaluate_transformer(spec, input_data, output_data)

                    assert result["num_errors"] == 0

    def test_conversion_bad_inputs(self):
        # Error on converting an untrained model
        with self.assertRaises(TypeError):
            model = OneHotEncoder()
            spec = sklearn.convert(model, "data", "out")

        # Check the expected class during covnersion.
        with self.assertRaises(TypeError):
            from sklearn.linear_model import LinearRegression

            model = LinearRegression()
            spec = sklearn.convert(model, "data", "out")

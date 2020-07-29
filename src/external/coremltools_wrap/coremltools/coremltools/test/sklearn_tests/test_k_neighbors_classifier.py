# Copyright (c) 2019, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import unittest

import numpy as np
from scipy import sparse

from coremltools._deps import _HAS_SKLEARN

if _HAS_SKLEARN:
    from coremltools.converters import sklearn
    from sklearn.datasets import load_iris
    from sklearn.neighbors import KNeighborsClassifier


@unittest.skipIf(not _HAS_SKLEARN, "Missing sklearn. Skipping tests.")
class KNeighborsClassifierScikitTest(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading the dataset and training a model.
        """
        print("Setting up KNeighborsClassifier converter tests")
        iris_samples = load_iris()
        self.iris_X = iris_samples.data
        self.iris_y = iris_samples.target

    def test_conversion_unfitted(self):
        """Tests conversion failure for an unfitted scikit model."""
        scikit_model = KNeighborsClassifier()
        self.assertRaises(TypeError, sklearn.convert, scikit_model)

    def test_conversion_brute_algorithm(self):
        """Tests conversion of a scikit KNeighborsClassifier using the brute force algorithm."""
        scikit_model = KNeighborsClassifier(algorithm="brute", n_neighbors=42)
        scikit_model.fit(self.iris_X, self.iris_y)

        coreml_model = sklearn.convert(scikit_model, "single_input", "single_output")
        coreml_spec = coreml_model.get_spec()

        self.assertIsNotNone(coreml_spec)
        self.assertTrue(coreml_spec.HasField("kNearestNeighborsClassifier"))
        self.assertEqual(
            coreml_spec.kNearestNeighborsClassifier.numberOfNeighbors.defaultValue, 42
        )
        self.assertEqual(
            coreml_spec.kNearestNeighborsClassifier.numberOfNeighbors.range.minValue, 1
        )
        self.assertEqual(
            coreml_spec.kNearestNeighborsClassifier.numberOfNeighbors.range.maxValue,
            len(self.iris_X),
        )
        self.assertTrue(
            coreml_spec.kNearestNeighborsClassifier.HasField("uniformWeighting")
        )
        self.assertEqual(
            coreml_spec.kNearestNeighborsClassifier.nearestNeighborsIndex.numberOfDimensions,
            len(self.iris_X[0]),
        )
        self.assertTrue(
            coreml_spec.kNearestNeighborsClassifier.nearestNeighborsIndex.HasField(
                "linearIndex"
            )
        )
        self.assertTrue(
            coreml_spec.kNearestNeighborsClassifier.nearestNeighborsIndex.HasField(
                "squaredEuclideanDistance"
            )
        )

        self.validate_labels(coreml_spec, self.iris_y)
        self.validate_float_samples(coreml_spec, self.iris_X)

    def test_conversion_kd_tree_algorithm(self):
        """Tests conversion of a scikit KNeighborsClassifier using the brute force algorithm."""
        test_leaf_size = 23
        test_n_neighbors = 42
        scikit_model = KNeighborsClassifier(
            algorithm="kd_tree", leaf_size=test_leaf_size, n_neighbors=test_n_neighbors
        )
        scikit_model.fit(self.iris_X, self.iris_y)

        coreml_model = sklearn.convert(scikit_model, "single_input", "single_output")
        coreml_spec = coreml_model.get_spec()

        self.assertIsNotNone(coreml_spec)
        self.assertTrue(coreml_spec.HasField("kNearestNeighborsClassifier"))
        self.assertEqual(
            coreml_spec.kNearestNeighborsClassifier.numberOfNeighbors.defaultValue,
            test_n_neighbors,
        )
        self.assertEqual(
            coreml_spec.kNearestNeighborsClassifier.numberOfNeighbors.range.minValue, 1
        )
        self.assertEqual(
            coreml_spec.kNearestNeighborsClassifier.numberOfNeighbors.range.maxValue,
            len(self.iris_X),
        )
        self.assertTrue(
            coreml_spec.kNearestNeighborsClassifier.HasField("uniformWeighting")
        )
        self.assertEqual(
            coreml_spec.kNearestNeighborsClassifier.nearestNeighborsIndex.numberOfDimensions,
            len(self.iris_X[0]),
        )
        self.assertTrue(
            coreml_spec.kNearestNeighborsClassifier.nearestNeighborsIndex.HasField(
                "singleKdTreeIndex"
            )
        )
        self.assertEqual(
            test_leaf_size,
            coreml_spec.kNearestNeighborsClassifier.nearestNeighborsIndex.singleKdTreeIndex.leafSize,
        )
        self.assertTrue(
            coreml_spec.kNearestNeighborsClassifier.nearestNeighborsIndex.HasField(
                "squaredEuclideanDistance"
            )
        )

        self.validate_labels(coreml_spec, self.iris_y)
        self.validate_float_samples(coreml_spec, self.iris_X)

    def test_conversion_auto_algorithm(self):
        """Tests conversion of a scikit KNeighborsClassifier using the brute force algorithm."""
        test_n_neighbors = 42
        scikit_model = KNeighborsClassifier(
            algorithm="auto", n_neighbors=test_n_neighbors
        )
        scikit_model.fit(self.iris_X, self.iris_y)

        coreml_model = sklearn.convert(scikit_model, "single_input", "single_output")
        coreml_spec = coreml_model.get_spec()
        self.assertIsNotNone(coreml_spec)

    def test_conversion_unsupported_algorithm(self):
        """Test a scikit KNeighborsClassifier with an invalid algorithm."""
        scikit_model = KNeighborsClassifier(algorithm="ball_tree")
        self.assertRaises(TypeError, sklearn.convert, scikit_model)

    def test_conversion_weight_function_good(self):
        scikit_model = KNeighborsClassifier(weights="uniform")
        scikit_model.fit(self.iris_X, self.iris_y)

        coreml_model = sklearn.convert(scikit_model, "single_input", "single_output")
        coreml_spec = coreml_model.get_spec()
        self.assertIsNotNone(coreml_spec)
        self.assertTrue(
            coreml_spec.kNearestNeighborsClassifier.HasField("uniformWeighting")
        )

    def test_conversion_unsupported_weight_function(self):
        scikit_model = KNeighborsClassifier(algorithm="brute", weights="distance")
        scikit_model.fit(self.iris_X, self.iris_y)
        self.assertRaises(TypeError, sklearn.convert, scikit_model)

        def callable_weight_function():
            print("Inside callable_weight_function")

        scikit_model = KNeighborsClassifier(
            algorithm="brute", weights=callable_weight_function
        )
        scikit_model.fit(self.iris_X, self.iris_y)
        self.assertRaises(TypeError, sklearn.convert, scikit_model)

    def test_conversion_distance_function_good(self):
        """Tests conversion of a scikit KNeighborsClassifier with a valid distance metric."""
        scikit_model = KNeighborsClassifier(algorithm="brute", metric="euclidean")
        scikit_model.fit(self.iris_X, self.iris_y)
        coreml_model = sklearn.convert(scikit_model, "single_input", "single_output")
        coreml_spec = coreml_model.get_spec()
        self.assertIsNotNone(coreml_spec)
        self.assertTrue(
            coreml_spec.kNearestNeighborsClassifier.nearestNeighborsIndex.HasField(
                "squaredEuclideanDistance"
            )
        )

        # Minkowski metric with p=2 is equivalent to the squared Euclidean distance
        scikit_model = KNeighborsClassifier(algorithm="brute", metric="minkowski", p=2)
        scikit_model.fit(self.iris_X, self.iris_y)
        coreml_spec = coreml_model.get_spec()
        self.assertIsNotNone(coreml_spec)
        self.assertTrue(
            coreml_spec.kNearestNeighborsClassifier.nearestNeighborsIndex.HasField(
                "squaredEuclideanDistance"
            )
        )

    def test_conversion_unsupported_distance_function(self):
        """Tests conversion of a scikit KNeighborsClassifier with an invalid distance metric."""
        # There are many possible distance functions for a brute force neighbors function, but these 3 should give us
        # coverage over the converter code.
        scikit_model = KNeighborsClassifier(algorithm="brute", metric="manhattan")
        scikit_model.fit(self.iris_X, self.iris_y)
        self.assertRaises(TypeError, sklearn.convert, scikit_model)

        scikit_model = KNeighborsClassifier(algorithm="kd_tree", metric="chebyshev")
        scikit_model.fit(self.iris_X, self.iris_y)
        self.assertRaises(TypeError, sklearn.convert, scikit_model)

        scikit_model = KNeighborsClassifier(algorithm="brute", metric="minkowski", p=3)
        scikit_model.fit(self.iris_X, self.iris_y)
        self.assertRaises(TypeError, sklearn.convert, scikit_model)

        def callable_distance_function():
            print("Inside callable_distance_function")

        scikit_model = KNeighborsClassifier(
            algorithm="brute", metric=callable_distance_function
        )
        scikit_model.fit(self.iris_X, self.iris_y)
        self.assertRaises(TypeError, sklearn.convert, scikit_model)

    def test_conversion_with_sparse_X(self):
        """Tests conversion of a model that's fitted with sparse data."""
        num_samples = 100
        num_dims = 64
        sparse_X = sparse.rand(
            num_samples, num_dims, format="csr"
        )  # KNeighborsClassifier only supports CSR format
        y = self.iris_y[
            0:num_samples
        ]  # the labels themselves don't matter - just use 100 of the Iris ones

        sklearn_model = KNeighborsClassifier(algorithm="brute")
        sklearn_model.fit(sparse_X, y)

        coreml_model = sklearn.convert(sklearn_model)
        coreml_spec = coreml_model.get_spec()
        self.assertIsNotNone(coreml_spec)

    def test_conversion_with_sparse_y(self):
        """Tests conversion of a model that's fitted with y values in a sparse format."""
        from sklearn.model_selection import train_test_split

        X_train, X_test, y_train, y_test = train_test_split(
            self.iris_X, self.iris_y, test_size=0.2, train_size=0.8
        )

        from sklearn import preprocessing

        lb = preprocessing.LabelBinarizer(sparse_output=True)
        binarized_y = lb.fit_transform(y_train)

        sklearn_model = KNeighborsClassifier(algorithm="brute")
        sklearn_model.fit(X_train, binarized_y)

        self.assertRaises(ValueError, sklearn.convert, sklearn_model)

    def validate_labels(self, spec, expected):
        """Validate the labels returned from the converted scikit KNeighborsClassifier"""
        self.assertTrue(spec.kNearestNeighborsClassifier.HasField("int64ClassLabels"))
        for index, label in enumerate(
            spec.kNearestNeighborsClassifier.int64ClassLabels.vector
        ):
            self.assertEqual(label, expected[index])

    def validate_float_samples(self, spec, expected):
        """Validate the float samples returned from the converted scikit KNeighborsClassifier"""
        num_dimensions = (
            spec.kNearestNeighborsClassifier.nearestNeighborsIndex.numberOfDimensions
        )
        for index, sample in enumerate(
            spec.kNearestNeighborsClassifier.nearestNeighborsIndex.floatSamples
        ):
            for dim in range(0, num_dimensions):
                self.assertAlmostEqual(
                    sample.vector[dim], expected[index][dim], places=6
                )

# Copyright (c) 2019, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import os
import shutil
from coremltools.models.utils import _is_macos

import unittest

from coremltools.models import MLModel
from coremltools.models.nearest_neighbors import KNearestNeighborsClassifierBuilder
from coremltools._deps import _HAS_SKLEARN

if _HAS_SKLEARN:
    from sklearn.datasets import load_iris


@unittest.skipIf(not _HAS_SKLEARN, "Missing sklearn. Skipping tests.")
class NearestNeighborsBuilderTest(unittest.TestCase):
    """
    Unit tests for the nearest neighbors builder class.
    """

    def setUp(self):
        iris_samples = load_iris()
        self.iris_X = iris_samples.data
        self.iris_y = iris_samples.target
        self.training_X = self.iris_X[-30:]
        self.training_y = self.iris_y[-30:]

    def tearDown(self):
        # Do any cleanup here
        pass

    def create_builder(self, default_class_label="default_label"):
        builder = KNearestNeighborsClassifierBuilder(
            input_name="input",
            output_name="output",
            number_of_dimensions=4,
            default_class_label=default_class_label,
        )
        return builder

    def test_builder_output_types(self):
        builder = self.create_builder(default_class_label="default")
        self.assertIsNotNone(builder)
        self.assertTrue(
            builder.spec.kNearestNeighborsClassifier.HasField("stringClassLabels")
        )

        builder = self.create_builder(default_class_label=12)
        self.assertIsNotNone(builder)
        self.assertTrue(
            builder.spec.kNearestNeighborsClassifier.HasField("int64ClassLabels")
        )

        with self.assertRaises(TypeError):
            bad_default_label = float(21.32)
            self.create_builder(default_class_label=bad_default_label)

    def test_builder_training_input(self):
        builder = self.create_builder(default_class_label="default")
        self.assertIsNotNone(builder)
        self.assertTrue(
            builder.spec.kNearestNeighborsClassifier.HasField("stringClassLabels")
        )

        self.assertEqual(builder.spec.description.trainingInput[0].name, "input")
        self.assertEqual(
            builder.spec.description.trainingInput[0].type.WhichOneof("Type"),
            "multiArrayType",
        )
        self.assertEqual(builder.spec.description.trainingInput[1].name, "output")
        self.assertEqual(
            builder.spec.description.trainingInput[1].type.WhichOneof("Type"),
            "stringType",
        )

    def test_make_updatable(self):
        builder = self.create_builder()
        self.assertIsNotNone(builder)

        self.assertTrue(builder.spec.isUpdatable)
        builder.is_updatable = False
        self.assertFalse(builder.spec.isUpdatable)
        builder.is_updatable = True
        self.assertTrue(builder.spec.isUpdatable)

    def test_author(self):
        builder = self.create_builder()
        self.assertIsNotNone(builder)

        self.assertEqual(builder.spec.description.metadata.author, "")
        builder.author = "John Doe"
        self.assertEqual(builder.author, "John Doe")
        self.assertEqual(builder.spec.description.metadata.author, "John Doe")

    def test_description(self):
        builder = self.create_builder()
        self.assertIsNotNone(builder)

        self.assertEqual(builder.spec.description.metadata.shortDescription, "")
        builder.description = "This is a description"
        self.assertEqual(builder.description, "This is a description")
        self.assertEqual(
            builder.spec.description.metadata.shortDescription, "This is a description"
        )

    def test_weighting_scheme(self):
        builder = self.create_builder()
        self.assertIsNotNone(builder)

        builder.weighting_scheme = "uniform"
        self.assertEqual(builder.weighting_scheme, "uniform")

        builder.weighting_scheme = "inverse_distance"
        self.assertEqual(builder.weighting_scheme, "inverse_distance")

        builder.weighting_scheme = "unIfOrM"
        self.assertEqual(builder.weighting_scheme, "uniform")

        builder.weighting_scheme = "InVerSE_DISTance"
        self.assertEqual(builder.weighting_scheme, "inverse_distance")

        with self.assertRaises(TypeError):
            builder.weighting_scheme = "test"

    def test_index_type(self):
        builder = self.create_builder()
        self.assertIsNotNone(builder)

        self.assertEqual(builder.index_type, "linear")
        self.assertEqual(builder.leaf_size, 0)

        builder.set_index_type("kd_tree")
        self.assertEqual(builder.index_type, "kd_tree")  # test default value
        self.assertEqual(builder.leaf_size, 30)

        builder.set_index_type("linear")
        self.assertEqual(builder.index_type, "linear")
        self.assertEqual(builder.leaf_size, 0)

        builder.set_index_type("kd_tree", leaf_size=45)  # test user-defined value
        self.assertEqual(builder.index_type, "kd_tree")
        self.assertEqual(builder.leaf_size, 45)

        builder.set_index_type("linear", leaf_size=37)
        self.assertEqual(builder.index_type, "linear")
        self.assertEqual(builder.leaf_size, 0)

        builder.set_index_type("KD_TrEe", leaf_size=22)  # test user-defined value
        self.assertEqual(builder.index_type, "kd_tree")
        self.assertEqual(builder.leaf_size, 22)

        builder.set_index_type("linEAR")
        self.assertEqual(builder.index_type, "linear")
        self.assertEqual(builder.leaf_size, 0)

        with self.assertRaises(TypeError):
            builder.set_index_type("unsupported_index")

        with self.assertRaises(TypeError):
            builder.set_index_type("kd_tree", -10)

        with self.assertRaises(TypeError):
            builder.set_index_type("kd_tree", 0)

    def test_leaf_size(self):
        builder = self.create_builder()
        self.assertIsNotNone(builder)

        builder.set_index_type("kd_tree", leaf_size=45)  # test user-defined value
        self.assertEqual(builder.index_type, "kd_tree")
        self.assertEqual(builder.leaf_size, 45)

        builder.leaf_size = 12
        self.assertEqual(builder.index_type, "kd_tree")
        self.assertEqual(builder.leaf_size, 12)

    def test_set_number_of_neighbors_with_bounds(self):
        builder = self.create_builder()
        self.assertIsNotNone(builder)

        self.assertEqual(builder.number_of_neighbors, 5)
        (min_value, max_value) = builder.number_of_neighbors_allowed_range()
        self.assertEqual(min_value, 1)
        self.assertEqual(max_value, 1000)

        builder.set_number_of_neighbors_with_bounds(12, allowed_range=(2, 24))
        (min_value, max_value) = builder.number_of_neighbors_allowed_range()
        self.assertEqual(builder.number_of_neighbors, 12)
        self.assertEqual(min_value, 2)
        self.assertEqual(max_value, 24)
        allowed_values = builder.number_of_neighbors_allowed_set()
        self.assertIsNone(allowed_values)

        test_set = {3, 5, 7, 9}
        builder.set_number_of_neighbors_with_bounds(7, allowed_set=test_set)
        self.assertEqual(builder.number_of_neighbors, 7)
        allowed_values = builder.number_of_neighbors_allowed_set()
        self.assertIsNotNone(allowed_values)
        self.assertEqual(allowed_values, test_set)

    def test_set_number_of_neighbors_with_bounds_error_conditions(self):
        builder = self.create_builder()
        self.assertIsNotNone(builder)

        with self.assertRaises(ValueError):
            builder.set_number_of_neighbors_with_bounds(3)

        test_range = (3, 15)
        test_set = {1, 3, 5}
        with self.assertRaises(ValueError):
            builder.set_number_of_neighbors_with_bounds(
                3, allowed_range=test_range, allowed_set=test_set
            )

        with self.assertRaises(ValueError):
            builder.set_number_of_neighbors_with_bounds(3, allowed_range=(-5, 5))

        with self.assertRaises(ValueError):
            builder.set_number_of_neighbors_with_bounds(3, allowed_range=(5, 1))

        with self.assertRaises(ValueError):
            builder.set_number_of_neighbors_with_bounds(
                3, allowed_range=test_range, allowed_set=test_set
            )

        with self.assertRaises(ValueError):
            builder.set_number_of_neighbors_with_bounds(2, allowed_range=test_range)

        with self.assertRaises(TypeError):
            builder.set_number_of_neighbors_with_bounds(5, allowed_set={5, -3, 7})

        with self.assertRaises(ValueError):
            builder.set_number_of_neighbors_with_bounds(4, allowed_set=test_set)

        with self.assertRaises(ValueError):
            builder.set_number_of_neighbors_with_bounds(4, allowed_set=test_set)

        with self.assertRaises(TypeError):
            builder.set_number_of_neighbors_with_bounds(2, allowed_set=[1, 2, 3])

        with self.assertRaises(TypeError):
            builder.set_number_of_neighbors_with_bounds(4, allowed_range={2, 200})

        with self.assertRaises(TypeError):
            builder.set_number_of_neighbors_with_bounds(4, allowed_range=(2, 10, 20))

        with self.assertRaises(TypeError):
            builder.set_number_of_neighbors_with_bounds(4, allowed_set=set())

        with self.assertRaises(TypeError):
            builder.set_number_of_neighbors_with_bounds(4, allowed_range=[])

    def test_set_number_of_neighbors(self):
        builder = self.create_builder()
        self.assertIsNotNone(builder)

        builder.set_number_of_neighbors_with_bounds(12, allowed_range=(2, 24))
        self.assertEqual(builder.number_of_neighbors, 12)

        with self.assertRaises(ValueError):
            builder.set_number_of_neighbors_with_bounds(1, allowed_range=(2, 24))
        builder.set_number_of_neighbors_with_bounds(4, allowed_range=(2, 24))
        self.assertEqual(builder.number_of_neighbors, 4)

        test_set = {3, 5, 7, 9}
        builder.set_number_of_neighbors_with_bounds(7, allowed_set=test_set)

        with self.assertRaises(ValueError):
            builder.set_number_of_neighbors_with_bounds(4, allowed_set=test_set)
        builder.set_number_of_neighbors_with_bounds(5, allowed_set=test_set)
        self.assertEqual(builder.number_of_neighbors, 5)

    def test_add_samples_invalid_data(self):
        builder = self.create_builder()
        self.assertIsNotNone(builder)

        invalid_X = [[1.0, 2.4]]
        with self.assertRaises(TypeError):
            builder.add_samples(invalid_X, self.training_y)

        with self.assertRaises(TypeError):
            builder.add_samples(self.training_X, self.training_y[:3])

        with self.assertRaises(TypeError):
            builder.add_samples([], self.training_y)

        with self.assertRaises(TypeError):
            builder.add_samples(self.training_X, [])

    def test_add_samples_int_labels(self):
        builder = self.create_builder(default_class_label=12)
        self.assertIsNotNone(builder)

        some_X = self.training_X[:10]
        some_y = self.training_y[:10]
        builder.add_samples(some_X, some_y)
        self._validate_samples(builder.spec, some_X, some_y)

        addl_X = self.training_X[10:20]
        addl_y = self.training_y[10:20]
        builder.add_samples(addl_X, addl_y)
        self._validate_samples(builder.spec, self.training_X[:20], self.training_y[:20])

    def test_add_samples_string_labels(self):
        builder = self.create_builder(default_class_label="default")
        self.assertIsNotNone(builder)

        some_X = self.training_X[:3]
        some_y = ["one", "two", "three"]
        builder.add_samples(some_X, some_y)
        self._validate_samples(builder.spec, some_X, some_y)

        addl_X = self.training_X[3:6]
        addl_y = ["four", "five", "six"]
        builder.add_samples(addl_X, addl_y)
        self._validate_samples(builder.spec, self.training_X[0:6], some_y + addl_y)

    def test_add_samples_invalid_label_types(self):
        builder_int_labels = self.create_builder(default_class_label=42)
        self.assertIsNotNone(builder_int_labels)

        some_X = self.training_X[:3]
        invalid_int_y = [0, "one", 2]
        with self.assertRaises(TypeError):
            builder_int_labels.add_samples(some_X, invalid_int_y)

        builder_string_labels = self.create_builder(default_class_label="default")
        self.assertIsNotNone(builder_string_labels)

        invalid_string_y = ["zero", "one", 2]
        with self.assertRaises(TypeError):
            builder_string_labels.add_samples(some_X, invalid_string_y)

    @unittest.skipUnless(_is_macos(), "Only supported on MacOS platform.")
    def test_can_init_and_save_model_from_builder_with_updated_spec(self):
        builder = KNearestNeighborsClassifierBuilder(
            input_name="input",
            output_name="output",
            number_of_dimensions=10,
            default_class_label="defaultLabel",
            k=3,
            weighting_scheme="inverse_distance",
            index_type="kd_tree",
            leaf_size=50,
        )
        builder.author = "CoreML Team"
        builder.license = "MIT"
        builder.description = "test_builder_with_validation"

        # Save the updated spec
        coreml_model = MLModel(builder.spec)
        self.assertIsNotNone(coreml_model)
        coreml_model_path = "/tmp/__test_builder_with_validation.mlmodel"

        try:
            coreml_model.save(coreml_model_path)
            self.assertTrue(os.path.isfile(coreml_model_path))
        finally:
            self._delete_mlmodel_and_mlmodelc(coreml_model_path)

    @unittest.skipUnless(_is_macos(), "Only supported on MacOS platform.")
    def test_can_init_and_save_model_from_builder_default_parameters(self):
        builder = KNearestNeighborsClassifierBuilder(
            input_name="input",
            output_name="output",
            number_of_dimensions=4,
            default_class_label="defaultLabel",
        )

        # Save the updated spec
        coreml_model = MLModel(builder.spec)
        self.assertIsNotNone(coreml_model)
        coreml_model_path = "/tmp/__test_builder_with_validation.mlmodel"

        try:
            coreml_model.save(coreml_model_path)
            self.assertTrue(os.path.isfile(coreml_model_path))
        finally:
            self._delete_mlmodel_and_mlmodelc(coreml_model_path)

    def _validate_samples(self, spec, expected_X, expected_y):
        """Validate the float samples returned from the converted scikit KNeighborsClassifier"""
        num_dimensions = (
            spec.kNearestNeighborsClassifier.nearestNeighborsIndex.numberOfDimensions
        )
        for index, sample in enumerate(
            spec.kNearestNeighborsClassifier.nearestNeighborsIndex.floatSamples
        ):
            for dim in range(0, num_dimensions):
                self.assertAlmostEqual(
                    sample.vector[dim], expected_X[index][dim], places=6
                )

        if spec.kNearestNeighborsClassifier.HasField("int64ClassLabels"):
            for index, label in enumerate(
                spec.kNearestNeighborsClassifier.int64ClassLabels.vector
            ):
                self.assertEqual(label, expected_y[index])

        elif spec.kNearestNeighborsClassifier.HasField("stringClassLabels"):
            for index, label in enumerate(
                spec.kNearestNeighborsClassifier.stringClassLabels.vector
            ):
                self.assertEqual(label, expected_y[index])

    @staticmethod
    def _delete_mlmodel_and_mlmodelc(path_to_mlmodel):
        """Delete the .mlmodel and .mlmodelc for the given .mlmodel."""
        if os.path.exists(path_to_mlmodel):
            os.remove(path_to_mlmodel)
        path_to_mlmodelc = "{}c".format(path_to_mlmodel)
        if os.path.exists(path_to_mlmodelc):
            shutil.rmtree(path_to_mlmodelc)

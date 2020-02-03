# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
import copy
import array
import numpy as np
import turicreate as tc
from . import util as test_util
from turicreate.util import _assert_sframe_equal as assert_sframe_equal
from turicreate.toolkits._main import ToolkitError

import sys

if sys.version_info.major == 3:
    unittest.TestCase.assertItemsEqual = unittest.TestCase.assertCountEqual


### Helper functions ###
### ---------------- ####
def make_classifier_data(n, d, seed=None):
    """
    Construct a synthetic dataset with a variety of data types to test the
    nearest neighbors classifier.
    """
    if seed:
        np.random.seed(seed)

    sf = tc.SFrame()

    # integer and float columns
    for i in range(d):
        sf["int{}".format(i)] = np.random.randint(low=-10, high=10, size=n)

    for i in range(d):
        v = np.random.rand(n)
        sf["float{}".format(i)] = v * 20 - 10

    # array column
    array_feature = []
    for i in range(n):
        array_feature.append(array.array("f", np.random.rand(d)))
    sf["array0"] = array_feature

    # string and dict columns
    for i in range(d + 1):
        sf["str{}".format(i)] = test_util.uniform_string_column(
            n, word_length=5, alphabet_size=5, missingness=0.0
        )

    sf["dict0"] = tc.text_analytics.count_ngrams(
        sf["str{}".format(d)], n=3, method="character", to_lower=False
    )
    sf.remove_column("str{}".format(d), inplace=True)

    # target column (string)
    sf["class"] = test_util.uniform_string_column(
        n, word_length=1, alphabet_size=3, missingness=0.0
    )

    return sf


### Unit test classes ###
### ----------------- ###
class KnnClassifierCreateTest(unittest.TestCase):
    """
    Unit test class for model creation process under various option values and
    types of input data.
    """

    @classmethod
    def setUpClass(self):
        self.sf = make_classifier_data(n=100, d=2, seed=19)
        self.verbose = False
        self.distance = [
            [["int0", "int1", "float0", "float1", "array0"], "euclidean", 1],
            [["int0", "int1"], "manhattan", 1.5],
            [["str0"], "levenshtein", 2],
            [["dict0"], "weighted_jaccard", 1.3],
        ]

    def test_input_mutations(self):
        """
        Make sure inputs to the create() method are not mutated.
        """

        ## Make copies of key objects
        sf = self.sf[:]
        distance = copy.deepcopy(self.distance)
        verbose = self.verbose

        ## Create a model with the copied objects
        m = tc.nearest_neighbor_classifier.create(
            sf, target="class", distance=distance, verbose=self.verbose
        )

        ## Check that the copies still equal the originals
        assert_sframe_equal(sf, self.sf)
        self.assertEqual(distance, self.distance)
        self.assertEqual(verbose, self.verbose)

    def test_bad_data(self):
        """
        Test error trapping for bogus input datasets.
        """

        ## Empty SFrame
        with self.assertRaises(ToolkitError):
            m = tc.nearest_neighbor_classifier.create(
                dataset=tc.SFrame(), target="class", verbose=self.verbose
            )

        ## SArray for the features
        with self.assertRaises(ToolkitError):
            m = tc.nearest_neighbor_classifier.create(
                dataset=self.sf["int0"], target="class", verbose=self.verbose
            )

        ## SArray for the target
        with self.assertRaises(ToolkitError):
            m = tc.nearest_neighbor_classifier.create(
                dataset=self.sf, target=self.sf["class"], verbose=self.verbose
            )

    def test_distances(self):
        """
        Check error trapping and processing of the 'distance' parameter,
        including construction of an automatic composite distance if no distance
        is specified.
        """

        numeric_features = ["int0", "int1", "float0", "float1"]
        array_features = ["array0"]
        string_features = ["str0"]
        dict_features = ["dict0"]

        ## Numeric standard distances should work for numeric columns
        for d in [
            "euclidean",
            "squared_euclidean",
            "manhattan",
            "cosine",
            "transformed_dot_product",
        ]:
            try:
                m = tc.nearest_neighbor_classifier.create(
                    self.sf,
                    target="class",
                    features=numeric_features,
                    distance=d,
                    verbose=False,
                )
            except:
                assert False, "Standard distance {} failed.".format(d)

        ## Numeric standard distances should work for array columns
        for d in [
            "euclidean",
            "squared_euclidean",
            "manhattan",
            "cosine",
            "transformed_dot_product",
        ]:
            try:
                m = tc.nearest_neighbor_classifier.create(
                    self.sf,
                    target="class",
                    features=array_features,
                    distance=d,
                    verbose=False,
                )
            except:
                assert False, "Standard distance {} failed.".format(d)

        ## String standard distances should work.
        for d in ["levenshtein"]:
            try:
                m = tc.nearest_neighbor_classifier.create(
                    self.sf,
                    target="class",
                    features=string_features,
                    distance=d,
                    verbose=False,
                )
            except:
                assert False, "Standard distance {} failed.".format(d)

        ## Dictionary standard distances should work.
        for d in ["jaccard", "weighted_jaccard", "cosine", "transformed_dot_product"]:
            try:
                m = tc.nearest_neighbor_classifier.create(
                    self.sf,
                    target="class",
                    features=dict_features,
                    distance=d,
                    verbose=False,
                )
            except:
                assert False, "Standard distance {} failed.".format(d)

        # Bogus distance strings should not work.
        with self.assertRaises(ValueError):
            m = tc.nearest_neighbor_classifier.create(
                self.sf,
                target="class",
                features=numeric_features,
                distance="fossa",
                verbose=False,
            )

        # Nonsensical combinations of feature types and distances should fail.
        with self.assertRaises(ValueError):
            m = tc.nearest_neighbor_classifier.create(
                self.sf,
                target="class",
                features=numeric_features,
                distance="levenshtein",
                verbose=False,
            )

        with self.assertRaises(ToolkitError):
            m = tc.nearest_neighbor_classifier.create(
                self.sf,
                target="class",
                features=dict_features,
                distance="levenshtein",
                verbose=False,
            )

        with self.assertRaises(ToolkitError):
            m = tc.nearest_neighbor_classifier.create(
                self.sf,
                target="class",
                features=string_features,
                distance="euclidean",
                verbose=False,
            )

        # If no distance is specified, the automatic distance construction
        # should kick in and be correct.
        correct_dist = [
            [["str0"], "levenshtein", 1],
            [["str1"], "levenshtein", 1],
            [["dict0"], "weighted_jaccard", 1],
            [["int0", "int1", "float0", "float1", "array0"], "euclidean", 5],
        ]

        m = tc.nearest_neighbor_classifier.create(
            self.sf, target="class", verbose=False
        )
        self.assertEqual(m.distance, correct_dist)

    def test_features(self):
        """
        Test that the features parameter works correctly in each of four
        combinations of 'features' and 'distance' parameters. Specifically, if
        the 'distance' parameter is a composite distance, the specified features
        in that parameter should take precedence.
        """

        ## Default is to get all features from the SFrame if 'features' is not
        #  specified and if the distance parameter is not a composite distance.
        numeric_features = ["int0", "int1", "float0", "float1", "array0"]
        m = tc.nearest_neighbor_classifier.create(
            self.sf[numeric_features + ["class"]],
            target="class",
            distance="euclidean",
            verbose=False,
        )
        self.assertEqual(m.num_features, 5)
        self.assertItemsEqual(m.features, numeric_features)

        ## 'features' parameter applies if custom distance not provided.
        m = tc.nearest_neighbor_classifier.create(
            self.sf,
            target="class",
            features=numeric_features,
            distance="euclidean",
            verbose=False,
        )
        self.assertEqual(m.num_features, 5)
        self.assertItemsEqual(m.features, numeric_features)

        ## Composite distance features should override the 'features' parameter.
        m = tc.nearest_neighbor_classifier.create(
            self.sf,
            target="class",
            features=["str0", "dict0"],
            distance=self.distance,
            verbose=False,
        )
        self.assertEqual(m.num_features, 7)
        self.assertItemsEqual(
            m.features, ["int0", "int1", "float0", "float1", "array0", "str0", "dict0"]
        )

        ## Make sure the target is removed from the 'features' parameter.
        m = tc.nearest_neighbor_classifier.create(
            self.sf,
            target="class",
            features=numeric_features + ["class"],
            distance="euclidean",
            verbose=False,
        )
        self.assertEqual(m.num_features, 5)
        self.assertItemsEqual(m.features, numeric_features)

        ## Make sure the target is removed from the composite distance features.
        distance = copy.deepcopy(self.distance)
        distance[2][0].append("class")
        m = tc.nearest_neighbor_classifier.create(
            self.sf, target="class", distance=distance, verbose=False
        )

        self.assertEqual(m.num_features, 7)
        self.assertItemsEqual(
            m.features, ["int0", "int1", "float0", "float1", "array0", "str0", "dict0"]
        )

    def test_backward_compatibility(self):
        """
        Test that loading a model from the previous version works correctly.
        This test does not make sense until GLC v1.5 at least, because the first
        version of the toolkit appears in GLC v1.4.
        """
        pass


class KnnClassifierModelTest(unittest.TestCase):
    """
    Unit test class for an already-trained nearest neighbors classifier model,
    except predict and classify methods.
    """

    def setUp(self):

        ## Make data
        self.n = 100
        sf = make_classifier_data(n=self.n, d=2, seed=19)

        ## Make the model
        self.distance = [
            [["int0", "int1", "float0", "float1", "array0"], "euclidean", 1],
            [["int0", "int1"], "manhattan", 1.5],
            [["str0"], "levenshtein", 2],
            [["dict0"], "weighted_jaccard", 1.3],
        ]

        self.model = tc.nearest_neighbor_classifier.create(
            sf, target="class", distance=self.distance, verbose=False
        )

    def test__list_fields(self):
        """
        Check the model _list_fields method.
        """
        correct_fields = [
            "distance",
            "num_distance_components",
            "verbose",
            "num_features",
            "training_time",
            "num_unpacked_features",
            "num_examples",
            "features",
            "target",
            "num_classes",
            "_target_type",
        ]

        self.assertItemsEqual(self.model._list_fields(), correct_fields)

    def test_get(self):
        """
        Check the get method against known answers for each field.
        """

        correct_fields = {
            "distance": self.distance,
            "num_distance_components": 4,
            "verbose": False,
            "num_features": 7,
            "num_examples": self.n,
            "target": "class",
            "num_classes": 3,
        }

        for field, ans in correct_fields.items():
            self.assertEqual(self.model._get(field), ans, "{} failed".format(field))

        self.assertGreater(self.model.training_time, 0)
        self.assertGreater(self.model.num_unpacked_features, self.n)
        self.assertItemsEqual(
            self.model.features,
            ["int0", "int1", "float0", "float1", "array0", "str0", "dict0"],
        )

    def test_summaries(self):
        """
        Unit test for __repr__, __str__, and model summary methods.
        """
        try:
            ans = str(self.model)
        except:
            assert False, "Model __repr__ failed."

        try:
            print(self.model)
        except:
            assert False, "Model print failed."

        try:
            self.model.summary()
        except:
            assert False, "Model summary failed."

    def test_save_and_load(self):
        """
        Ensure that model saving and loading retains all model information.
        """
        with test_util.TempDirectory() as f:
            self.model.save(f)
            self.model = tc.load_model(f)

            try:
                self.test__list_fields()
                print("Saved model list fields passed")

                self.test_get()
                print("Saved model get passed")

                self.test_summaries()
                print("Saved model summaries passed")

            except:
                assert False, "Failed during save and load tests."
            del self.model


class KnnClassifierPredictTest(unittest.TestCase):
    """
    Unit test class for correctness of model predictions.
    """

    @classmethod
    def setUpClass(self):
        self.sf = make_classifier_data(n=100, d=2, seed=19)
        self.sf_test = make_classifier_data(n=10, d=2, seed=92)
        self.distance = [
            [["int0", "int1", "float0", "float1", "array0"], "euclidean", 1],
            [["int0", "int1"], "manhattan", 1.5],
            [["str0"], "levenshtein", 2],
            [["dict0"], "weighted_jaccard", 1.3],
        ]

        self.model = tc.nearest_neighbor_classifier.create(
            self.sf, target="class", distance=self.distance, verbose=False
        )

    def test_bogus_parameters(self):
        """
        Unit test for bogus parameters in model prediction methods.
        """

        ## Bogus maximum number of neighbors in each query
        for k in [-1, 0, "fossa"]:
            with self.assertRaises(ValueError):
                ystar = self.model.predict(self.sf, max_neighbors=k)

            with self.assertRaises(ValueError):
                ystar = self.model.classify(self.sf, max_neighbors=k)

            with self.assertRaises(ValueError):
                ystar = self.model.predict_topk(self.sf, max_neighbors=k)

        ## Bogus neighborhood radius
        for r in [-1, "cat"]:
            with self.assertRaises(ValueError):
                ystar = self.model.predict(self.sf, radius=r)

            with self.assertRaises(ValueError):
                ystar = self.model.classify(self.sf, radius=r)

            with self.assertRaises(ValueError):
                ystar = self.model.predict_topk(self.sf, radius=r)

        ## Bogus number of results to return (for predict_topk only)
        for k in [-1, 0, "fossa"]:
            with self.assertRaises(TypeError):
                ystar = self.model.predict_topk(self.sf, k=k)

        ## Empty prediction dataset
        with self.assertRaises(ToolkitError):
            ystar = self.model.predict(tc.SFrame())

        with self.assertRaises(ToolkitError):
            ystar = self.model.classify(tc.SFrame())

        with self.assertRaises(ToolkitError):
            ystar = self.model.predict_topk(tc.SFrame())

    def test_classify(self):
        """
        Unit test for class label predictions.
        """

        ## Test type of output.
        ystar = self.model.classify(self.sf[:1], verbose=False)

        self.assertIsInstance(ystar, tc.SFrame)
        self.assertItemsEqual(ystar.column_names(), ["class", "probability"])
        self.assertItemsEqual(ystar.column_types(), [str, float])

        ## Test plausibility of probability output.
        self.assertTrue(all(ystar["probability"] >= 0))
        self.assertTrue(all(ystar["probability"] <= 1))

        ## Check that classify on the training data with a single neighbor
        #  returns the training point.
        ystar = self.model.classify(self.sf, max_neighbors=1, verbose=False)

        self.assertTrue((ystar["class"] == self.sf["class"]).all())
        self.assertTrue(all(ystar["probability"] == 1))

        ## Check that classifying test data with a small radius returns None
        #  values if there are no qualified neighbors.
        ystar = self.model.classify(
            self.sf_test, max_neighbors=None, radius=1e-6, verbose=False
        )

        self.assertTrue(all(ystar["class"] == None))
        self.assertTrue(all(ystar["probability"] == None))

        ## Check that the results are correct if *some*, but not *all* of the
        #  test points have no qualified neighbors in the training set.
        ystar = self.model.classify(
            self.sf_test, max_neighbors=None, radius=15.0, verbose=False
        )

        self.assertItemsEqual(ystar.column_types(), [str, float])
        self.assertTrue(
            ystar["class"].countna() > 0
            and ystar["class"].countna() < self.sf_test.num_rows()
        )
        self.assertTrue(
            ystar["probability"].countna() > 0
            and ystar["probability"].countna() < self.sf_test.num_rows()
        )

    def test_predict(self):
        """
        Unit test for predictions in SArray format.
        """

        ## Test that predictions are in an SArray.
        ystar = self.model.predict(self.sf[:1], verbose=False)
        self.assertEqual(type(ystar), tc.SArray)

        ## Test type of output is correct.
        ystar = self.model.predict(self.sf[:1], output_type="class", verbose=False)
        self.assertIs(ystar.dtype, str)

        ystar = self.model.predict(
            self.sf[:1], output_type="probability", verbose=False
        )
        self.assertIs(ystar.dtype, float)

    def test_predict_topk(self):
        """
        Unit test for the top-k predictions method.
        """

        ## Check type out output.
        topk = self.model.predict_topk(self.sf[:1], verbose=False)

        self.assertIsInstance(topk, tc.SFrame)
        self.assertItemsEqual(topk.column_names(), ["row_id", "class", "probability"])
        self.assertItemsEqual(topk.column_types(), [int, str, float])

        ## Test plausibility of probability output.
        self.assertTrue(all(topk["probability"] >= 0))
        self.assertTrue(all(topk["probability"] <= 1))

        ## Test that k = 1 returns the same top probabilities as model.classify.
        #  Note, the existence of ties means it's impossible to check for the
        #  same class labels.
        topk = self.model.predict_topk(self.sf, k=1, verbose=False)
        ystar = self.model.classify(self.sf, verbose=False)

        # TODO @papayawarrior -- this assertion fails now that SArray
        # comparison is meaningful. Please fix the test and uncomment.
        # self.assertTrue((topk['probability'] == ystar['probability']).all())

        ## Check that k results are returned for each query.
        topk = self.model.predict_topk(self.sf, k=2, verbose=False)
        counts = topk.groupby("row_id", tc.aggregate.COUNT)
        self.assertEqual(counts.num_rows(), self.sf.num_rows())
        self.assertTrue(all(counts["Count"] == 2))

        ## Check that 3 results are returned for each query even if k is bigger.
        topk = self.model.predict_topk(self.sf, k=100, verbose=False)
        counts = topk.groupby("row_id", tc.aggregate.COUNT)

        num_classes = len(self.sf["class"].unique())
        self.assertTrue(all(counts["Count"] <= num_classes))
        self.assertEqual(counts.num_rows(), self.sf.num_rows())

    def test_evaluate(self):
        """
        Make sure evaluate works and the results are plausible. In the future
        check correctness as well.
        """
        ans = self.model.evaluate(self.sf)

        ## Check that the right keys are present in the results dict.
        self.assertItemsEqual(ans.keys(), ["accuracy", "confusion_matrix"])

        ## Check plausibility of the accuracy.
        self.assertIsInstance(ans["accuracy"], float)
        self.assertTrue(ans["accuracy"] >= 0 and ans["accuracy"] <= 1)

        ## Check plausibility of the confusion matrix.
        self.assertIsInstance(ans["confusion_matrix"], tc.SFrame)
        self.assertEqual(ans["confusion_matrix"].num_columns(), 3)
        self.assertEqual(len(ans["confusion_matrix"]["target_label"].unique()), 3)
        self.assertEqual(ans["confusion_matrix"]["count"].sum(), 100)

        ## Check the values and types of the confusion matrix of *none* of the
        #  test points have *any* qualified neighbors in the training set.
        evals = self.model.evaluate(self.sf_test, max_neighbors=None, radius=1e-6)
        acc = evals["accuracy"]
        cf_mat = evals["confusion_matrix"]

        self.assertTrue(acc == 0.0)
        self.assertTrue(all(cf_mat["target_label"] != None))
        self.assertTrue(all(cf_mat["predicted_label"] == None))
        self.assertItemsEqual(cf_mat.column_types(), [str, float, int])
        self.assertEqual(cf_mat["count"].sum(), self.sf_test.num_rows())

        ## Check that the results are correct if *some*, but not *all* of the
        #  test points have no qualified neighbors in the training set.
        evals = self.model.evaluate(self.sf_test, max_neighbors=None, radius=15.0)
        acc = evals["accuracy"]
        cf_mat = evals["confusion_matrix"]

        self.assertTrue(acc >= 0.0 and acc <= 1.0)
        self.assertItemsEqual(cf_mat.column_types(), [str, str, int])
        self.assertItemsEqual(
            cf_mat["target_label"].unique(), self.sf_test["class"].unique()
        )
        self.assertTrue(
            cf_mat["predicted_label"].countna() > 0
            and cf_mat["predicted_label"].countna() < self.sf_test.num_rows()
        )

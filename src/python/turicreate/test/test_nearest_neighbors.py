# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
import numpy as np
import copy
import turicreate as tc
from turicreate.toolkits._main import ToolkitError
from turicreate.toolkits._private_utils import _validate_lists
import scipy.spatial.distance as spd
from pandas.util.testing import assert_frame_equal
from . import util
import array
import string
import random
import sys
from functools import partial

from .test_distances import euclidean, squared_euclidean, manhattan, levenshtein
from .test_distances import cosine, transformed_dot_product, jaccard, weighted_jaccard

if sys.version_info.major > 2:
    long = int

import os as _os


class NearestNeighborsCreateTest(unittest.TestCase):
    """
    Unit test class for testing the model create process for various nearest
    neighbors models and methods.
    """

    @classmethod
    def setUpClass(self):

        ## Make data
        np.random.seed(19)
        n, d = 108, 3

        array_features = []
        dict_features = []
        for i in range(n):
            array_features.append(array.array("f", np.random.rand(d)))
            dict_features.append(
                {
                    "alice": np.random.randint(10),
                    "brian": np.random.randint(10),
                    "chris": np.random.randint(10),
                }
            )

        self.refs = tc.SFrame()
        for i in range(d):
            self.refs["X{}".format(i + 1)] = tc.SArray(np.random.rand(n))

        self.label = "label"
        self.refs[self.label] = [str(x) for x in range(n)]
        self.refs["array_ftr"] = array_features
        self.refs["dict_ftr"] = dict_features
        self.refs["str_ftr"] = random_string(n, length=3, num_letters=5)
        self.refs["list_str_ftr"] = random_list_of_str(n, length=3)

    def _test_create(
        self, sf, label, features, distance, method, field=None, value=None
    ):
        """
        Test creation of nearest neighbors models.
        """

        m = tc.nearest_neighbors.create(
            sf, label, features, distance, method, verbose=False
        )
        assert m is not None, "Model creation failed."

        if field is not None:
            ans = m._get(field)
            assert ans == value, "Field '{}' is not correct.".format(field)

    def test_create_default(self):
        """
        Test the default nearest neighbors create configuration.
        """

        ## check auto configurations for the method when features are provided.
        self._test_create(
            self.refs,
            self.label,
            features=["X1", "X2", "X3"],
            distance="auto",
            method="auto",
            field="method",
            value="ball_tree",
        )

        self._test_create(
            self.refs,
            self.label,
            features=["X1", "X2", "X3"],
            distance="euclidean",
            method="auto",
            field="method",
            value="ball_tree",
        )

        ## check auto configurations for distance if features specified.
        m = tc.nearest_neighbors.create(
            self.refs,
            self.label,
            features=["X1", "X2", "X3"],
            distance="auto",
            method="brute_force",
            verbose=False,
        )
        self.assertEqual(m.distance, [[["X1", "X2", "X3"], "euclidean", 1.0]])

        ## check auto configurations for distance if features *not* specified.
        ans_dist = [
            [["X1", "X2", "X3"], "euclidean", 1.0],
            [["dict_ftr"], "jaccard", 1.0],
            [["str_ftr"], "levenshtein", 1.0],
            [["array_ftr"], "euclidean", 1.0],
            [["list_str_ftr"], "jaccard", 1.0],
        ]

        m = tc.nearest_neighbors.create(
            self.refs,
            self.label,
            features=None,
            distance="auto",
            method="brute_force",
            verbose=False,
        )
        self.assertItemsEqual(m.distance, ans_dist)

        m = tc.nearest_neighbors.create(
            self.refs,
            self.label,
            features=None,
            distance=None,
            method="brute_force",
            verbose=False,
        )
        self.assertItemsEqual(m.distance, ans_dist)

        ## check default leaf size for ball tree
        correct_leaf_size = 1000

        self._test_create(
            self.refs,
            self.label,
            features=["array_ftr"],
            distance="euclidean",
            method="ball_tree",
            field="leaf_size",
            value=correct_leaf_size,
        )

        m = tc.nearest_neighbors.create(
            self.refs,
            self.label,
            features=["array_ftr"],
            method="ball_tree",
            leaf_size=0,
            verbose=False,
        )
        assert m is not None, "Model creation failed."
        assert m.leaf_size == correct_leaf_size, (
            "Leaf size explicit default" + "failed."
        )

    def test_create_labels(self):
        """
        Test nearest neighbor model creation with different label configurations.
        """

        sf = self.refs[:]
        sf.remove_column(self.label, inplace=True)

        ## String labels are tested everywhere else in this class.

        ## Passing no label should work, with and without listed features
        self._test_create(
            sf,
            label=None,
            features=None,
            distance="auto",
            method="auto",
            field="label",
            value=None,
        )

        self._test_create(
            sf,
            label=None,
            features=["X1", "X2", "X3"],
            distance="euclidean",
            method="auto",
            field="label",
            value=None,
        )

        ## Integer label should work
        sf = sf.add_row_number(column_name="id")

        self._test_create(
            sf,
            label="id",
            features=None,
            distance="auto",
            method="auto",
            field="label",
            value="id",
        )

        m = tc.nearest_neighbors.create(
            sf,
            label="id",
            features=["X1", "X2", "X3"],
            distance="euclidean",
            method="brute_force",
            verbose=False,
        )
        self.assertEqual(set(m.features), set(["X1", "X2", "X3"]))

        ## Float label should fail
        sf["id"] = sf["id"].astype(float)

        with self.assertRaises(TypeError):
            m = tc.nearest_neighbors.create(sf, label="id")

        ## Specified label, included in the features list should drop the label
        #  from the features.
        m = tc.nearest_neighbors.create(
            sf,
            label=None,
            features=["X1", "X2", "__id"],
            distance="euclidean",
            method="brute_force",
            verbose=False,
        )
        self.assertEqual(set(m.features), set(["X1", "X2"]))

        ## If there is only one feature, and it's specified as the label, this
        #  should raise an informative error.
        sf = sf.add_row_number("id_test")

        with self.assertRaises(ToolkitError):
            m = tc.nearest_neighbors.create(sf, label="id_test", features=["id_test"])

    def test_create_methods(self):
        """
        Test different methods of nearest neighbors models.
        """

        methods = {
            "auto": "ball_tree",
            "brute_force": "brute_force",
            "ball_tree": "ball_tree",
            "lsh": "lsh",
        }

        for m, name in methods.items():
            self._test_create(
                self.refs,
                self.label,
                features=["array_ftr"],
                method=m,
                distance="euclidean",
                field="method",
                value=name,
            )

        ## Cosine and transformed_dot_product distances should not work with ball tree
        for dist in [
            "cosine",
            "transformed_dot_product",
            tc.distances.cosine,
            tc.distances.transformed_dot_product,
        ]:

            with self.assertRaises(TypeError):
                tc.nearest_neighbors.create(
                    self.refs,
                    self.label,
                    features=["array_ftr"],
                    distance=dist,
                    method="ball_tree",
                    verbose=False,
                )

        ## Multiple distance components should cause an automatic switch to
        #  brute force, even if ball tree is specified.
        distance_components = [
            [["X1", "X2", "X3"], "euclidean", 1],
            [["array_ftr"], "manhattan", 1],
            [["str_ftr"], "levenshtein", 1],
        ]

        m = tc.nearest_neighbors.create(
            self.refs,
            self.label,
            method="ball_tree",
            distance=distance_components,
            verbose=False,
        )
        self.assertEqual(m.method, "brute_force")

    def test_kwargs(self):
        """
        Make sure only allowed keyword args are processed. All others should
        raise errors to avoid confusion in downstream errors.
        """
        with self.assertRaises(ToolkitError):
            m = tc.nearest_neighbors.create(
                self.refs,
                self.label,
                feature="array_ftr",  # this is bogus
                method="ball_tree",
                distance="euclidean",
                verbose=False,
            )

    def test_create_dense_distances(self):
        """
        Test various distances in nearest neighbors models.
        """

        dense_dists = {
            "euclidean": tc.distances.euclidean,
            "squared_euclidean": tc.distances.squared_euclidean,
            "gaussian_kernel": tc.distances.gaussian_kernel,
            "manhattan": tc.distances.manhattan,
            "cosine": tc.distances.cosine,
            "transformed_dot_product": tc.distances.transformed_dot_product,
        }

        for dist_name, dist_fn in dense_dists.items():

            ans_dist = [[["array_ftr"], dist_name, 1.0]]

            ## Test the string form of the distance argument
            m = tc.nearest_neighbors.create(
                self.refs,
                self.label,
                features=["array_ftr"],
                distance=dist_name,
                method="brute_force",
                verbose=False,
            )
            self.assertEqual(m.distance, ans_dist)

            ## Test the function form of the distance argument
            m = tc.nearest_neighbors.create(
                self.refs,
                self.label,
                features=["array_ftr"],
                distance=dist_fn,
                method="brute_force",
                verbose=False,
            )
            self.assertEqual(m.distance, ans_dist)

            ## Numeric distances should *not* work with string features
            try:
                tc.nearest_neighbors.create(
                    self.refs, self.label, ["str_ftr"], distance=dist_name
                )
            except ToolkitError as e:
                self.assertTrue(
                    str(e).startswith(
                        "The only distance allowed for string features is 'levenshtein'. "
                        "Please try this distance, or use 'text_analytics.count_ngrams' to "
                        "convert the strings to dictionaries, which permit more distance functions.\n"
                    )
                )

    def test_create_sparse_distances(self):
        """
        Test various distances in nearest neighbors models that operate on
        sparse data, e.g. vectors."""

        sparse_dists = {
            "jaccard": tc.distances.jaccard,
            "weighted_jaccard": tc.distances.weighted_jaccard,
            "cosine": tc.distances.cosine,
            "transformed_dot_product": tc.distances.transformed_dot_product,
        }

        for dist_name, dist_fn in sparse_dists.items():

            ans_dist = [[["dict_ftr"], dist_name, 1.0]]

            ## Test the string form of the distance argument
            m = tc.nearest_neighbors.create(
                self.refs,
                self.label,
                features=["dict_ftr"],
                distance=dist_name,
                method="brute_force",
                verbose=False,
            )
            self.assertEqual(m.distance, ans_dist)

            ## Test the function form of the distance argument
            m = tc.nearest_neighbors.create(
                self.refs,
                self.label,
                features=["dict_ftr"],
                distance=dist_fn,
                method="brute_force",
                verbose=False,
            )
            self.assertEqual(m.distance, ans_dist)

            ## Test the string form of the distance argument with list of str
            ans_dist = [[["list_str_ftr"], dist_name, 1.0]]
            m = tc.nearest_neighbors.create(
                self.refs,
                self.label,
                features=["list_str_ftr"],
                distance=dist_name,
                method="brute_force",
                verbose=False,
            )
            self.assertEqual(m.distance, ans_dist)

        ## Jaccard distances should not work with numeric or string features
        for dist in [
            "jaccard",
            "weighted_jaccard",
            tc.distances.jaccard,
            tc.distances.weighted_jaccard,
        ]:

            try:
                tc.nearest_neighbors.create(
                    self.refs,
                    self.label,
                    features=["array_ftr"],
                    distance=dist,
                    method="brute_force",
                    verbose=False,
                )
            except ToolkitError as e:
                self.assertTrue(
                    str(e).startswith(
                        "Cannot compute jaccard distances with column 'array_ftr'."
                        " Jaccard distances currently can only be computed for"
                        " dictionary and list features.\n"
                    )
                )

            try:
                tc.nearest_neighbors.create(
                    self.refs,
                    self.label,
                    ["str_ftr"],
                    distance=dist_name,
                    verbose=False,
                )
            except ToolkitError as e:
                self.assertTrue(
                    str(e).startswith(
                        "The only distance allowed for string features is 'levenshtein'. "
                        "Please try this distance, or use 'text_analytics.count_ngrams' "
                        "to convert the strings to dictionaries, which permit more distance functions.\n"
                    )
                )

        ## Jacard distance throws TypeError on lists of non-strings
        refs = self.refs.__copy__()
        refs["list_float_ftr"] = refs["array_ftr"].apply(lambda x: list(x), dtype=list)

        # Check autodistance
        with self.assertRaises(TypeError):
            m = tc.nearest_neighbors.create(
                refs, self.label, features=["list_float_ftr"], verbose=False
            )

        # Check user-specified distance
        for distance in ["jaccard", "weighted_jaccard", "euclidean"]:
            with self.assertRaises(TypeError):
                m = tc.nearest_neighbors.create(
                    refs,
                    self.label,
                    features=["list_float_ftr"],
                    distance=distance,
                    method="brute_force",
                    verbose=False,
                )

    def test_create_string_distances(self):
        """
        Test that various string distances can be used to create a nearest
        neighbors model.
        """

        string_dists = {"levenshtein": tc.distances.levenshtein}

        for dist_name, dist_fn in string_dists.items():

            ans_dist = [[["str_ftr"], dist_name, 1.0]]

            ## Test the string form of the distance argument
            m = tc.nearest_neighbors.create(
                self.refs,
                self.label,
                features=["str_ftr"],
                distance=dist_name,
                method="brute_force",
                verbose=False,
            )
            self.assertEqual(m.distance, ans_dist)

            ## Test the function form of the distance argument
            m = tc.nearest_neighbors.create(
                self.refs,
                self.label,
                features=["str_ftr"],
                distance=dist_fn,
                method="brute_force",
                verbose=False,
            )
            self.assertEqual(m.distance, ans_dist)

            ## String distances should not work with numeric or dictionary
            #  features
            try:
                tc.nearest_neighbors.create(
                    self.refs,
                    self.label,
                    features=["dict_ftr"],
                    distance=dist_name,
                    method="brute_force",
                    verbose=False,
                )
            except ToolkitError as e:
                self.assertTrue(
                    str(e).startswith(
                        "Cannot compute {} distance with column 'dict_ftr'.".format(
                            dist_name
                        )
                        + " {} distance can only computed for string features.\n".format(
                            dist_name
                        )
                    )
                )

    def test_create_composite_distances(self):
        """
        Test that nearest neighbors models can be successfully created with
        composite distance parameters.
        """

        distance_components = [
            [["X1", "X2"], "euclidean", 1],
            [
                ["X2", "X3"],
                "manhattan",
                1,
            ],  # note overlap with first component's features
            [["array_ftr"], "manhattan", 1],
            [["str_ftr"], "levenshtein", 1],
        ]

        ## Test that things work correctly in the vanilla case.
        m = tc.nearest_neighbors.create(
            self.refs, self.label, distance=distance_components, verbose=False
        )

        assert m is not None, "Model creation failed."
        self.assertEqual(m.distance, distance_components)
        self.assertEqual(m.num_distance_components, 4)
        self.assertEqual(m.method, "brute_force")
        self.assertEqual(m.num_features, 5)
        self.assertEqual(m.num_unpacked_features, 7)

        ## Make sure the features parameter is ignored if a composite distance
        #  is specified.
        m = tc.nearest_neighbors.create(
            self.refs,
            self.label,
            features=["X1", "X2"],
            distance=distance_components,
            verbose=False,
        )

        assert m is not None, "Model creation failed."
        self.assertEqual(m.distance, distance_components)
        self.assertEqual(m.num_distance_components, 4)
        self.assertEqual(m.num_features, 5)
        self.assertEqual(m.num_unpacked_features, 7)

    def test_create_num_variables(self):
        """
        Test vector features, numeric features, and combinations thereof.
        """
        for ftr_list, v in [
            (["X1", "X2", "X3"], 3),
            (["array_ftr"], 3),
            (["dict_ftr"], 3),
        ]:

            self._test_create(
                self.refs,
                self.label,
                features=ftr_list,
                method="auto",
                distance="auto",
                field="num_unpacked_features",
                value=v,
            )

        for ftr_list, v in [
            (["X1", "X2", "X3"], 3),
            (["array_ftr"], 1),
            (["dict_ftr"], 1),
        ]:

            self._test_create(
                self.refs,
                self.label,
                features=ftr_list,
                method="auto",
                distance="auto",
                field="num_features",
                value=v,
            )

    def test_create_mutations(self):
        """
        Test that inputs to the nearest neighbors create function are not
        mutated by the function.
        """
        sf = self.refs[:]
        label = self.label
        ftrs_orig = ["X1", "X2", "X3", "array_ftr"]
        ftrs_copy = ftrs_orig[:]

        m = tc.nearest_neighbors.create(
            sf,
            label=label,
            features=ftrs_copy,
            method="auto",
            distance="auto",
            verbose=False,
        )

        assert_frame_equal(self.refs.to_dataframe(), sf.to_dataframe())
        self.assertEqual(label, self.label)
        self.assertEqual(ftrs_orig, ftrs_copy)

    def test_missing_data(self):
        """
        Check that the create function errors out (correctly) if there are
        missing data of any type in any cell of the input dataset.
        """

        sf = tc.SFrame({"x0": [1, 2, 3], "x1": ["a", "b", "c"]})
        sf["ints"] = [1, 2, None]
        sf["floats"] = [None, 2.2, 3.3]
        sf["strings"] = ["a", None, "c"]
        sf["dicts"] = [{"a": 1}, {"b": 2}, None]
        sf["arrays"] = [
            array.array("f", [1.0, 2.0]),
            array.array("f", [3.0, 4.0]),
            None,
        ]

        for ftr in ["ints", "floats", "strings", "dicts", "arrays"]:
            with self.assertRaises(ToolkitError):
                m = tc.nearest_neighbors.create(sf[["x0", "x1", ftr]], verbose=False)


class NearestNeighborsEdgeCaseTests(unittest.TestCase):
    """"
    Test bad inputs and edge values for inputs.
    """

    @classmethod
    def setUpClass(self):

        ## Make data
        np.random.seed(19)
        n, d = 100, 3
        self.label = "label"

        array_features = []
        for i in range(n):
            array_features.append(array.array("f", np.random.rand(d)))

        self.refs = tc.SFrame()
        self.refs["array"] = array_features
        self.refs[self.label] = [str(x) for x in range(n)]

    def test_empty_data(self):
        """
        Test the errors for empty reference and query datasets.
        """

        ## Useful objects
        sf_empty = tc.SFrame()

        m = tc.nearest_neighbors.create(
            self.refs,
            label=self.label,
            features=None,
            method="brute_force",
            distance="euclidean",
            verbose=False,
        )

        with self.assertRaises(ToolkitError):
            m = tc.nearest_neighbors.create(sf_empty, self.label)

        with self.assertRaises(ToolkitError):
            knn = tc.nearest_neighbors.create(sf_empty, self.label)

    def test_bogus_labels(self):
        """
        Test the errors with labels that don't exist.
        """

        with self.assertRaises(ToolkitError):
            m = tc.nearest_neighbors.create(self.refs, label="fossa")

        m = tc.nearest_neighbors.create(self.refs)

        with self.assertRaises(ValueError):
            m.query(self.refs, label="fossa")

    def test_empty_composite_distance(self):
        """
        Test that empty composite distance parameters are not allowed.
        """

        with self.assertRaises(ValueError):
            m = tc.nearest_neighbors.create(self.refs, distance=[])

    def test_bogus_parameters(self):
        """
        Test toolkit behavior when arguments are inappropriate.
        """

        ## k is out of bounds raises an error
        m = tc.nearest_neighbors.create(self.refs, self.label, method="brute_force")

        for k in [-1, 0, "cat"]:
            with self.assertRaises(ValueError):
                knn = m.query(self.refs, self.label, k=k)

        ## k > n should default to n
        knn = m.query(self.refs, self.label, k=2 * self.refs.num_rows())
        assert (
            knn.num_rows() == self.refs.num_rows() ** 2
        ), "Query with k > n returned the wrong number of rows."

        ## radius out of bounds should raise an error
        for r in [-1, "cat"]:
            with self.assertRaises(ValueError):
                knn = m.query(self.refs, self.label, radius=r)

        # ## Leaf size is out of bounds
        # import ipdb
        # ipdb.set_trace()

        for ls in [-1, 12.3, "fossa"]:
            with self.assertRaises(ToolkitError):
                m = tc.nearest_neighbors.create(
                    self.refs,
                    self.label,
                    features=["array"],
                    method="ball_tree",
                    distance="euclidean",
                    leaf_size=ls,
                )

        ## Leaf size > n should be fine
        m = tc.nearest_neighbors.create(
            self.refs,
            self.label,
            features=["array"],
            method="ball_tree",
            distance="euclidean",
            leaf_size=2 * self.refs.num_rows(),
        )
        assert m.leaf_size == 2 * self.refs.num_rows()

        ## Distance component weights are out of bounds
        with self.assertRaises(ValueError):
            m = tc.nearest_neighbors.create(
                self.refs, self.label, distance=[[["array"], "euclidean", -1e-7]]
            )

        with self.assertRaises(ValueError):
            m = tc.nearest_neighbors.create(
                self.refs, self.label, distance=[[["array"], "euclidean", -1]]
            )

        with self.assertRaises(ValueError):
            m = tc.nearest_neighbors.create(
                self.refs, self.label, distance=[[["array"], "euclidean", "a"]]
            )

        with self.assertRaises(ToolkitError):
            m = tc.nearest_neighbors.create(
                self.refs, self.label, distance=[[["array"], "euclidean", 1e15]]
            )


class NearestNeighborsBruteForceAPITest(unittest.TestCase):
    """
    Unit test class for testing model methods once models are trained. Excludes
    the correctness of the query results.
    """

    def setUp(self):

        ## Make data
        np.random.seed(19)
        d = 3  # dimension
        n = 100  # number of reference points

        self.refs = tc.SFrame()
        for i in range(d):
            self.refs.add_column(tc.SArray(np.random.rand(n)), inplace=True)

        self.refs["row_label"] = [str(x) for x in range(n)]

        self.label = "row_label"
        self.features = ["X{}".format(i + 1) for i in range(d)]
        self.unpacked_features = ["X{}".format(i + 1) for i in range(d)]

        ## Create the nearest neighbors model
        self.model = tc.nearest_neighbors.create(
            self.refs,
            self.label,
            features=None,
            method="brute_force",
            distance="euclidean",
            verbose=False,
        )

        ## Answers
        self.fields_ans = [
            "training_time",
            "label",
            "unpacked_features",
            "features",
            "method",
            "num_examples",
            "num_unpacked_features",
            "num_features",
            "num_distance_components",
            "distance",
            "distance_for_summary_struct",
        ]

        self.default_opts = {
            "leaf_size": {
                u"default_value": 0,
                u"lower_bound": 0,
                u"upper_bound": 2147483647,
                u"description": u"Max number of points in a leaf node of the ball tree",
                u"parameter_type": u"INTEGER",
            },
            "label": {
                u"default_value": u"",
                u"description": u"Name of the reference dataset column with row labels.",
                u"parameter_type": u"STRING",
            },
        }

        self.opts = {"label": self.label}

        self.get_ans = {
            "distance": lambda x: len(x) == 1,
            "training_time": lambda x: x >= 0,
            "label": lambda x: x == self.label,
            "method": lambda x: x == "brute_force",
            "num_examples": lambda x: x == 100,
            "num_features": lambda x: x == 3,
            "num_unpacked_features": lambda x: x == 3,
            "num_distance_components": lambda x: x == 1,
        }

    def test__list_fields(self):
        """
        Check the _list_fields method.
        """
        assert set(self.model._list_fields()) == set(
            self.fields_ans
        ), "List fields failed with {}.".format(self.model._list_fields())

    def test_get(self):
        """
        Check the get method against known answers for each field.
        """
        for field in self.get_ans.keys():
            ans = self.model._get(field)
            assert self.get_ans[field](
                ans
            ), "Get failed in field '{}'. Output: {}".format(field, ans)

        ## Check names of features and unpacked features
        assert set(self.model.features) == set(self.features)
        assert set(self.model.unpacked_features) == set(self.unpacked_features)

    def test_summary(self):
        """
        Check that the summary method returns information.
        """
        try:
            self.model.summary()
        except:
            assert False, "Model summary failed."

        assert self.model.summary() is not ""

    def test_repr(self):
        """
        """
        try:
            ans = str(self.model)
        except:
            assert False, "Model repr failed."

    def test_save_and_load(self):
        """
        Make sure saving and loading retains everything.
        """
        with util.TempDirectory() as f:
            self.model.save(f)
            self.model = tc.load_model(f)

            try:
                self.test__list_fields()
                self.test_get()
            except:
                assert False, "List fields or get failed after save and load."

            try:
                self.model.query(self.refs, self.label, k=3)
            except:
                assert False, "Querying failed after save and load."

            try:
                g = self.model.similarity_graph()
            except:
                assert False, "Similarity failed after save and load."

            try:
                self.test_summary()
                self.test_repr()
            except:
                assert False, "Summaries failed after save and load."

            del self.model


class NearestNeighborsLshAPITest(unittest.TestCase):
    """
    Unit test class for testing model methods once models are trained. Excludes
    the correctness of the query results.
    """

    def setUp(self):

        ## Make data
        np.random.seed(19)
        d = 3  # dimension
        n = 100  # number of reference points

        refs = []
        for i in range(n):
            refs.append(array.array("f", np.random.rand(d)))

        self.refs = tc.SFrame({"features": refs})
        self.refs["row_label"] = [str(x) for x in range(n)]
        self.query = tc.SFrame({"features": refs})
        self.query["row_label"] = [str(x) for x in range(50, n + 50)]

        self.label = "row_label"
        self.features = ["features"]
        self.unpacked_features = ["features[0]", "features[1]", "features[2]"]

        self.opts = {
            "num_tables": 4,
            "num_projections_per_table": 4,
            "label": self.label,
        }

        ## Create the nearest neighbors model
        self.model = tc.nearest_neighbors.create(
            self.refs,
            self.label,
            features=self.features,
            method="lsh",
            distance="euclidean",
            num_tables=self.opts["num_tables"],
            num_projections_per_table=self.opts["num_projections_per_table"],
        )
        ## Answers
        self.fields_ans = [
            "distance",
            "distance_for_summary_struct",
            "num_distance_components",
            "features",
            "unpacked_features",
            "label",
            "num_tables",
            "num_projections_per_table",
            "method",
            "num_examples",
            "num_unpacked_features",
            "num_features",
            "training_time",
        ]

        self.default_opts = {
            "num_tables": {
                u"default_value": 10,
                u"lower_bound": 1,
                u"upper_bound": 2147483647,
                u"description": u"number of hash tables for LSH",
                u"parameter_type": u"INTEGER",
            },
            "num_projections_per_table": {
                u"default_value": 8,
                u"lower_bound": 1,
                u"upper_bound": 2147483647,
                u"description": u"number of projections in each hash table",
                u"parameter_type": u"INTEGER",
            },
            "label": {
                u"default_value": u"",
                u"description": u"Name of the reference dataset column with row labels.",
                u"parameter_type": u"STRING",
            },
        }

        self.get_ans = {
            "distance": lambda x: len(x) == 1,
            "num_distance_components": lambda x: x == 1,
            "label": lambda x: x == self.label,
            "num_tables": lambda x: x == self.opts["num_tables"],
            "num_projections_per_table": lambda x: x
            == self.opts["num_projections_per_table"],
            "method": lambda x: x == "lsh",
            "num_examples": lambda x: x == 100,
            "num_features": lambda x: x == 1,
            "num_unpacked_features": lambda x: x == 3,
            "training_time": lambda x: x >= 0,
        }

    def test_query(self):
        q = self.model.query(self.query, label=self.label, k=1, verbose=False)
        assert q is not None
        assert q.num_rows() >= self.query.num_rows()
        # all the 1-nearest-neighbor should be the queries themselves (and identical points)
        # so that means a distance of zero
        distances = q["distance"]
        assert len(distances.filter(lambda x: x != 0.0)) == 0

    def test__list_fields(self):
        """
        Check the _list_fields method.
        """
        assert set(self.model._list_fields()) == set(
            self.fields_ans
        ), "List fields failed with {}.".format(self.model._list_fields())

    def test_get(self):
        """
        Check the get method against known answers for each field.
        """
        for field in self.get_ans.keys():
            ans = self.model._get(field)
            assert self.get_ans[field](
                ans
            ), "Get failed in field '{}'. Output: {}".format(field, ans)

        ## Check names of features and unpacked features
        assert set(self.model.features) == set(self.features)
        assert set(self.model.unpacked_features) == set(self.unpacked_features)

    def test_summary(self):
        """
        Check that the summary method returns information.
        """
        try:
            self.model.summary()
        except:
            assert False, "Model summary failed."

        assert self.model.summary() is not ""

    def test_repr(self):
        """
        """
        try:
            ans = str(self.model)
        except:
            assert False, "Model repr failed."

    def test_save_and_load(self):
        """
        Make sure saving and loading retains everything.
        """
        with util.TempDirectory() as f:
            self.model.save(f)
            self.model = tc.load_model(f)

            try:
                print(self.model._list_fields())
                self.test__list_fields()
                self.test_get()
            except:
                assert False, "List fields or get failed after save and load."

            try:
                self.test_summary()
                self.test_repr()
            except:
                assert False, "Summaries failed after save and load."
            del self.model


class NearestNeighborsBallTreeAPITest(unittest.TestCase):
    """
    Unit test class for testing model methods once models are trained. Excludes
    the correctness of the query results.
    """

    def setUp(self):

        ## Make data
        np.random.seed(19)
        d = 3  # dimension
        n = 100  # number of reference points

        refs = []
        for i in range(n):
            refs.append(array.array("f", np.random.rand(d)))

        self.refs = tc.SFrame({"features": refs})
        self.refs["row_label"] = [str(x) for x in range(n)]
        self.query = tc.SFrame({"features": refs})
        self.query["row_label"] = [str(x) for x in range(50, n + 50)]

        self.label = "row_label"
        self.features = ["features"]
        self.unpacked_features = ["features[0]", "features[1]", "features[2]"]

        self.opts = {"leaf_size": 16, "label": self.label}

        ## Create the nearest neighbors model
        self.model = tc.nearest_neighbors.create(
            self.refs,
            self.label,
            features=self.features,
            method="ball_tree",
            distance="euclidean",
            leaf_size=self.opts["leaf_size"],
            verbose=False,
        )

        ## Answers
        self.fields_ans = [
            "distance",
            "distance_for_summary_struct",
            "num_distance_components",
            "features",
            "unpacked_features",
            "label",
            "leaf_size",
            "method",
            "num_examples",
            "num_unpacked_features",
            "num_features",
            "training_time",
            "tree_depth",
        ]

        self.default_opts = {
            "leaf_size": {
                u"default_value": 0,
                u"lower_bound": 0,
                u"upper_bound": 2147483647,
                u"description": u"Max number of points in a leaf node of the ball tree",
                u"parameter_type": u"INTEGER",
            },
            "label": {
                u"default_value": u"",
                u"description": u"Name of the reference dataset column with row labels.",
                u"parameter_type": u"STRING",
            },
        }

        self.get_ans = {
            "distance": lambda x: len(x) == 1,
            "num_distance_components": lambda x: x == 1,
            "label": lambda x: x == self.label,
            "leaf_size": lambda x: x == self.opts["leaf_size"],
            "method": lambda x: x == "ball_tree",
            "num_examples": lambda x: x == 100,
            "num_features": lambda x: x == 1,
            "num_unpacked_features": lambda x: x == 3,
            "training_time": lambda x: x >= 0,
            "tree_depth": lambda x: x == 4,  # assumes n=100, leaf_size=16
        }

    def test_query(self):
        q = self.model.query(self.query, self.label, k=3, verbose=False)
        assert q is not None

    def test__list_fields(self):
        """
        Check the _list_fields method.
        """
        assert set(self.model._list_fields()) == set(
            self.fields_ans
        ), "List fields failed with {}.".format(self.model._list_fields())

    def test_get(self):
        """
        Check the get method against known answers for each field.
        """
        for field in self.get_ans.keys():
            ans = self.model._get(field)
            assert self.get_ans[field](
                ans
            ), "Get failed in field '{}'. Output: {}".format(field, ans)

        ## Check names of features and unpacked features
        assert set(self.model.features) == set(self.features)
        assert set(self.model.unpacked_features) == set(self.unpacked_features)

    def test_summary(self):
        """
        Check that the summary method returns information.
        """
        try:
            self.model.summary()
        except:
            assert False, "Model summary failed."

        assert self.model.summary() is not ""

    def test_repr(self):
        """
        """
        try:
            ans = str(self.model)
        except:
            assert False, "Model repr failed."

    def test_save_and_load(self):
        """
        Make sure saving and loading retains everything.
        """
        with util.TempDirectory() as f:
            self.model.save(f)
            self.model = tc.load_model(f)

            try:
                print(self.model._list_fields())
                self.test__list_fields()
                self.test_get()
            except:
                assert False, "List fields or get failed after save and load."

            try:
                self.test_summary()
                self.test_repr()
            except:
                assert False, "Summaries failed after save and load."

            del self.model


class GeneralSimilarityGraphTest(unittest.TestCase):
    """
    Tests that apply to all nearest neighbors similarity graph methods,
    regardless of data type.
    """

    @classmethod
    def setUpClass(self):

        np.random.seed(19)
        self.dimension = 3  # dimension
        n = 10  # number of reference points

        self.refs = tc.SFrame(np.random.rand(n, self.dimension))
        self.features = ["X1.{}".format(i) for i in range(self.dimension)]
        self.refs = self.refs.unpack("X1")

        self.label = "id"
        self.refs = self.refs.add_row_number(self.label)
        self.refs[self.label] = self.refs[self.label].astype(str) + "a"

        df_refs = self.refs.to_dataframe().drop(self.label, axis=1)
        self.answer_dists = scipy_dist(df_refs, df_refs, "euclidean")

    def test_neighborhood_constraints(self):
        """
        Test various combinations of the k and radius constraints.
        """
        m = tc.nearest_neighbors.create(
            self.refs,
            features=self.features,
            distance="euclidean",
            method="brute_force",
            verbose=False,
        )

        knn = m.similarity_graph(
            k=None,
            radius=None,
            include_self_edges=True,
            output_type="SFrame",
            verbose=False,
        )
        assert_frame_equal(self.answer_dists.to_dataframe(), knn.to_dataframe())

        ## k only, no radius
        knn = m.similarity_graph(
            k=3,
            radius=None,
            include_self_edges=True,
            output_type="SFrame",
            verbose=False,
        )
        ans = self.answer_dists[self.answer_dists["rank"] <= 3]
        assert_frame_equal(ans.to_dataframe(), knn.to_dataframe())

        ## radius only, no k
        knn = m.similarity_graph(
            k=None,
            radius=0.4,
            include_self_edges=True,
            output_type="SFrame",
            verbose=False,
        )
        ans = self.answer_dists[self.answer_dists["distance"] <= 0.4]
        assert_frame_equal(ans.to_dataframe(), knn.to_dataframe())

        ## Both radius and k
        knn = m.similarity_graph(
            k=3,
            radius=0.4,
            include_self_edges=True,
            output_type="SFrame",
            verbose=False,
        )
        ans = self.answer_dists[self.answer_dists["rank"] <= 3]
        ans = ans[ans["distance"] <= 0.4]
        assert_frame_equal(ans.to_dataframe(), knn.to_dataframe())

    def test_self_edges(self):
        """
        Check that the 'include_self_edges' flag performs as expected, with and
        without reference row labels.
        """

        ## Without row labels
        m = tc.nearest_neighbors.create(
            self.refs,
            features=self.features,
            distance="euclidean",
            method="brute_force",
            verbose=False,
        )

        knn = m.similarity_graph(
            k=None,
            radius=None,
            include_self_edges=True,
            output_type="SFrame",
            verbose=False,
        )
        assert_frame_equal(self.answer_dists.to_dataframe(), knn.to_dataframe())

        knn2 = m.similarity_graph(
            k=None,
            radius=None,
            include_self_edges=False,
            output_type="SFrame",
            verbose=False,
        )
        mask = self.answer_dists["query_label"] != self.answer_dists["reference_label"]
        ans = self.answer_dists[mask]
        ans["rank"] = ans["rank"] - 1
        assert_frame_equal(ans.to_dataframe(), knn2.to_dataframe())

        ## With string row labels
        m = tc.nearest_neighbors.create(
            self.refs,
            self.label,
            features=None,
            distance="euclidean",
            method="brute_force",
            verbose=False,
        )

        knn = m.similarity_graph(
            k=None,
            radius=None,
            output_type="SFrame",
            include_self_edges=True,
            verbose=False,
        )

        ans = copy.copy(self.answer_dists)
        ans["query_label"] = ans["query_label"].astype(str) + "a"
        ans["reference_label"] = ans["reference_label"].astype(str) + "a"
        assert_frame_equal(ans.to_dataframe(), knn.to_dataframe())

        knn2 = m.similarity_graph(
            k=None,
            radius=None,
            include_self_edges=False,
            output_type="SFrame",
            verbose=False,
        )
        mask = self.answer_dists["query_label"] != self.answer_dists["reference_label"]
        ans = self.answer_dists[mask]
        ans["rank"] = ans["rank"] - 1
        ans["query_label"] = ans["query_label"].astype(str) + "a"
        ans["reference_label"] = ans["reference_label"].astype(str) + "a"

        assert_frame_equal(ans.to_dataframe(), knn2.to_dataframe())

    def test_output_type(self):
        """
        Check that the results can be returned as either an SFrame or an SGraph
        and that the results match in both of these forms.
        """
        m = tc.nearest_neighbors.create(
            self.refs,
            features=self.features,
            distance="euclidean",
            method="brute_force",
            verbose=False,
        )

        knn = m.similarity_graph(
            k=None,
            radius=None,
            include_self_edges=False,
            output_type="SFrame",
            verbose=False,
        )

        sg = m.similarity_graph(
            k=None,
            radius=None,
            include_self_edges=False,
            output_type="SGraph",
            verbose=False,
        )

        sg_edges = copy.copy(sg.edges)
        sg_edges = sg_edges.rename(
            {"__src_id": "query_label", "__dst_id": "reference_label"}, inplace=True
        )
        sg_edges = sg_edges.sort(["query_label", "distance"])

        assert_frame_equal(sg_edges.to_dataframe(), knn.to_dataframe())

    def test_other_methods(self):
        """
        Similarity graph should also work on ball_tree and lsh
        """

        ## Ball tree
        m = tc.nearest_neighbors.create(
            self.refs,
            self.label,
            features=None,
            distance="euclidean",
            method="ball_tree",
            verbose=False,
        )

        knn = m.similarity_graph(k=5, radius=None, verbose=False)

        ## LSH
        m = tc.nearest_neighbors.create(
            self.refs,
            self.label,
            features=None,
            distance="euclidean",
            method="lsh",
            verbose=False,
        )

        knn = m.similarity_graph(k=5, radius=None, verbose=False)


class GeneralQueryTest(unittest.TestCase):
    """
    Tests that apply to all nearest neighbors model queries, regardless of data
    type.
    """

    @classmethod
    def setUpClass(self):

        np.random.seed(19)
        p = 3  # dimension
        n = 10  # number of reference points
        self.n_query = 2  # number of query points

        self.refs = tc.SFrame(np.random.rand(n, p))
        self.refs = self.refs.unpack("X1")

        self.label = "id"
        self.refs = self.refs.add_row_number(self.label)
        self.queries = self.refs[0 : self.n_query]

        df_refs = self.refs.to_dataframe().drop(self.label, axis=1)
        df_queries = self.queries.to_dataframe().drop(self.label, axis=1)
        self.answer_dists = scipy_dist(df_queries, df_refs, "euclidean")

    def test_neighborhood_constraints(self):
        """
        Test various combinations of the k and radius constraints.
        """

        ## No constraints
        m = tc.nearest_neighbors.create(
            self.refs,
            self.label,
            distance="euclidean",
            method="brute_force",
            verbose=False,
        )

        knn = m.query(self.queries, k=None, radius=None, verbose=False)

        assert_frame_equal(self.answer_dists.to_dataframe(), knn.to_dataframe())

        ## k only, no radius
        knn = m.query(self.queries, k=3, radius=None, verbose=False)
        ans = self.answer_dists[self.answer_dists["rank"] <= 3]
        assert_frame_equal(ans.to_dataframe(), knn.to_dataframe())

        ## radius only, no k
        knn = m.query(self.queries, k=None, radius=0.4, verbose=False)
        ans = self.answer_dists[self.answer_dists["distance"] <= 0.4]
        assert_frame_equal(ans.to_dataframe(), knn.to_dataframe())

        ## Both radius and k
        knn = m.query(self.queries, k=3, radius=0.4, verbose=False)
        ans = self.answer_dists[self.answer_dists["rank"] <= 3]
        ans = ans[ans["distance"] <= 0.4]
        assert_frame_equal(ans.to_dataframe(), knn.to_dataframe())

    def test_labels(self):
        """
        Test query accuracy for various configurations of row labels.
        """
        sfq = self.queries[:]
        sfq.remove_column("id", inplace=True)
        k = 3

        m = tc.nearest_neighbors.create(
            self.refs,
            label=self.label,
            features=None,
            distance="euclidean",
            method="brute_force",
            verbose=False,
        )
        knn_correct = m.query(self.queries, self.label, k=k, verbose=False)

        ## No label should work fine
        knn = m.query(sfq, label=None, k=k, verbose=False)
        self.assertTrue(knn["query_label"].dtype is int)
        assert_frame_equal(knn_correct.to_dataframe(), knn.to_dataframe())

        ## Integer label should work fine
        sfq = sfq.add_row_number(column_name="id")
        knn = m.query(sfq, label="id", k=k, verbose=False)
        self.assertTrue(knn["query_label"].dtype is int)
        assert_frame_equal(knn_correct.to_dataframe(), knn.to_dataframe())

        ## Float label should fail
        sfq["id"] = sfq["id"].astype(float)
        with self.assertRaises(TypeError):
            knn = m.query(sfq, label="id", k=k)


class NearestNeighborsNumericQueryTest(unittest.TestCase):
    """
    Unit test class for checking correctness of exact model queries.
    """

    @classmethod
    def setUpClass(self):

        ## Make data
        np.random.seed(19)
        p = 3  # dimension
        n = 10  # number of reference points
        self.n_query = 2  # number of query points

        self.refs = tc.SFrame(np.random.rand(n, p))
        self.refs = self.refs.unpack("X1")

        self.label = "id"
        self.refs = self.refs.add_row_number(self.label)
        self.queries = self.refs[0 : self.n_query]

        ## Answer items common to all tests
        self.r = self.refs.to_dataframe().drop(self.label, axis=1)
        self.q = self.queries.to_dataframe().drop(self.label, axis=1)

    def _test_query(
        self,
        answer,
        sf_ref,
        sf_query,
        label,
        features,
        distance,
        method,
        k=None,
        radius=None,
    ):
        """
        Test the accuracy of exact queries against python brute force solution,
        from Scipy.
        """

        ## Construct nearest neighbors model and get query results
        m = tc.nearest_neighbors.create(
            sf_ref, label, features, distance, method, verbose=False
        )
        knn = m.query(sf_query, label, k=k, radius=radius, verbose=False)

        ## Trim the answers to the k and radius parameters
        if k is not None:
            answer = answer[answer["rank"] <= k]

        if radius is not None:
            answer = answer[answer["distance"] <= radius]

        ## Test data frame equality
        assert_frame_equal(answer.to_dataframe(), knn.to_dataframe())

    def test_query_distances(self):
        """
        Test query accuracy for various distances.
        """

        idx_row = np.array([[x] for x in range(self.n_query)])

        ## Euclidean and cosine distance
        for dist in ["euclidean", "cosine"]:
            answer = scipy_dist(self.q, self.r, dist)
            self._test_query(
                answer,
                self.refs,
                self.queries,
                self.label,
                features=None,
                distance=dist,
                method="brute_force",
            )

        ## Squared euclidean distances
        answer = scipy_dist(self.q, self.r, "sqeuclidean")
        self._test_query(
            answer,
            self.refs,
            self.queries,
            self.label,
            features=None,
            distance="squared_euclidean",
            method="brute_force",
        )

        ## Manhattan distance
        answer = scipy_dist(self.q, self.r, "cityblock")
        self._test_query(
            answer,
            self.refs,
            self.queries,
            self.label,
            features=None,
            distance="manhattan",
            method="brute_force",
        )

        ## Auto distance (brute force, dense features)
        answer = scipy_dist(self.q, self.r, "euclidean")
        self._test_query(
            answer,
            self.refs,
            self.queries,
            self.label,
            features=None,
            distance="auto",
            method="brute_force",
        )

        ## Transformed dot product distance
        D = self.q.dot(self.r.T)
        D = np.log(1 + np.exp(-1 * D))

        D = D.values
        n_query, n = D.shape

        idx_col = np.argsort(D, axis=1)
        idx_row = np.array([[x] for x in range(n_query)])
        query_labels = list(np.repeat(range(n_query), n))
        ranks = np.tile(range(1, n + 1), n_query)

        answer = tc.SFrame(
            {
                "query_label": query_labels,
                "reference_label": idx_col.flatten(),
                "distance": D[idx_row, idx_col].flatten(),
                "rank": ranks,
            }
        )

        answer.swap_columns("distance", "query_label", inplace=True)
        answer.swap_columns("distance", "reference_label", inplace=True)
        answer.swap_columns("distance", "rank", inplace=True)

        self._test_query(
            answer,
            self.refs,
            self.queries,
            self.label,
            features=None,
            distance="transformed_dot_product",
            method="brute_force",
        )

    def test_query_methods(self):
        """
        Test query accuracy for various nearest neighbor methods.
        """
        answer = scipy_dist(self.q, self.r, "euclidean")

        for method in ["auto", "brute_force", "ball_tree"]:
            self._test_query(
                answer,
                self.refs,
                self.queries,
                self.label,
                features=None,
                distance="euclidean",
                method=method,
            )

    def test_blockwise_brute_force(self):
        """
        Test query completeness and accuracy for brute force queries, with the
        block-wise brute force implementation.
        """

        ## On debug, the following dimensions should produce 121 blocks of
        #  reference data, and 2 blocks of query data.
        n, d = 1927, 1000
        n_query = 21  # more than 20 queries is necessary for block-wise brute force
        k = 5

        sf = tc.SFrame(np.random.rand(n, d))

        m = tc.nearest_neighbors.create(
            sf, method="brute_force", distance="euclidean", verbose=False
        )

        sf_query = tc.SFrame(np.random.rand(n_query, d))

        knn = m.query(sf_query, verbose=False)  # blockwise brute force query
        knn2 = m.query(sf_query[:10], verbose=False)  # pairwise brute force query

        self.assertEqual(knn.num_rows(), 21 * 5)
        assert_frame_equal(knn[: 10 * 5].to_dataframe(), knn2.to_dataframe())

    def test_similarity_graph(self):
        """
        Test accuracy and completeness of the similarity graph method. The
        numeric version also tests the blockwise similarity graph.
        """

        ### Blockwise similarity graph - euclidean distance
        ### -----------------------------------------------
        n, d = 500, 10
        sf = tc.SFrame(np.random.rand(n, d))

        m = tc.nearest_neighbors.create(
            sf, method="brute_force", distance="euclidean", verbose=False
        )

        knn = m.query(sf[:10], k=3, verbose=False)
        knn_graph = m.similarity_graph(k=2, output_type="SFrame", verbose=False)

        ## Basic metadata about the output
        self.assertEqual(knn_graph.num_rows(), 1000)
        self.assertEqual(knn_graph["rank"].max(), 2)
        self.assertGreaterEqual(knn_graph["distance"].min(), 0.0)

        ## No self-edges
        label_diff = knn_graph["query_label"] - knn_graph["reference_label"]
        self.assertEqual(sum(label_diff == 0), 0)

        ## Exact match for query results with different
        knn = knn[knn["rank"] > 1]
        knn["rank"] = knn["rank"] - 1
        assert_frame_equal(knn.to_dataframe(), knn_graph[:20].to_dataframe())

        ### Tweaking query-time parameters
        ### ------------------------------
        knn_graph2 = m.similarity_graph(k=5, output_type="SFrame", verbose=False)
        self.assertEqual(knn_graph2.num_rows(), 2500)
        self.assertEqual(knn_graph2["rank"].max(), 5)
        self.assertGreaterEqual(knn_graph2["distance"].min(), 0.0)

        knn_graph3 = m.similarity_graph(
            k=None, radius=0.5, output_type="SFrame", verbose=False
        )
        self.assertLessEqual(knn_graph3["distance"].max(), 0.5)

        knn_graph4 = m.similarity_graph(
            k=2, radius=0.5, output_type="SFrame", verbose=False
        )
        self.assertEqual(knn_graph4["rank"].max(), 2)
        self.assertLessEqual(knn_graph4["distance"].max(), 0.5)

        ### Pairwise similarity graph - manhattan distance
        ### ----------------------------------------------
        n, d = 500, 10
        sf = tc.SFrame(np.random.rand(n, d))

        m = tc.nearest_neighbors.create(
            sf, method="brute_force", distance="manhattan", verbose=False
        )

        knn = m.query(sf[:10], k=3, verbose=False)
        knn_graph = m.similarity_graph(k=2, output_type="SFrame", verbose=False)

        ## Basic metadata about the output
        self.assertEqual(knn_graph.num_rows(), 1000)
        self.assertEqual(knn_graph["rank"].max(), 2)
        self.assertGreaterEqual(knn_graph["distance"].min(), 0.0)

        ## No self-edges
        label_diff = knn_graph["query_label"] - knn_graph["reference_label"]
        self.assertEqual(sum(label_diff == 0), 0)

        ## Exact match for query results with different
        knn = knn[knn["rank"] > 1]
        knn["rank"] = knn["rank"] - 1
        assert_frame_equal(knn.to_dataframe(), knn_graph[:20].to_dataframe())


class NearestNeighborsSparseQueryTest(unittest.TestCase):
    """
    Unit test class for checking correctness of exact model queries.
    """

    @classmethod
    def setUpClass(self):
        n = 10
        n_query = 2

        # Generate sparse data: an SFrame with a column of type dict
        self.field = "docs"
        self.refs = tc.SFrame()
        self.refs[self.field] = [random_dict(5, 3) for i in range(n)]
        self.k = 3  # number of neighbors to return
        self.radius = 1.0  # radius to use
        self.label = "row_label"
        self.refs[self.label] = [str(x) for x in range(n)]

    def _test_query(
        self, sf_ref, sf_query, label, features, distance, method, k=5, radius=1.0
    ):
        """
        Test the accuracy of exact queries against hand-coded solution above.
        """

        ## Construct nearest neighbors model and get query results
        m = tc.nearest_neighbors.create(
            sf_ref, label, features, distance, method, verbose=False
        )

        knn = m.query(sf_query, label, k=k, radius=radius)

        # TODO: Speed up this test
        for row in knn:
            q = row["query_label"]
            r = row["reference_label"]
            query = sf_ref[int(q)][self.field]
            ref = sf_ref[int(r)][self.field]

            score = row["distance"]

            if distance == "cosine":
                ans = cosine(query, ref)
            elif distance == "dot_product":
                ans = dot_product(query, ref)
            elif distance == "transformed_dot_product":
                ans = transformed_dot_product(query, ref)
            elif distance == "jaccard":
                ans = jaccard(query, ref)
            elif distance == "weighted_jaccard":
                ans = weighted_jaccard(query, ref)
            elif distance == "euclidean":
                ans = euclidean(query, ref)
            elif distance == "squared_euclidean":
                ans = squared_euclidean(query, ref)
            elif distance == "manhattan":
                ans = manhattan(query, ref)
            else:
                raise RuntimeError("Unknown distance")
            self.assertAlmostEqual(score, ans)

    def test_query_distances(self):
        """
        Test query accuracy for various distances.
        """
        for dist in [
            "euclidean",
            "squared_euclidean",
            "manhattan",
            "cosine",
            "transformed_dot_product",
            "jaccard",
            "weighted_jaccard",
        ]:

            self._test_query(
                self.refs,
                self.refs,
                self.label,
                features=None,
                distance=dist,
                method="brute_force",
            )
            self._test_query(
                self.refs,
                self.refs,
                self.label,
                features=None,
                distance=dist,
                method="brute_force",
                k=self.k,
            )
            self._test_query(
                self.refs,
                self.refs,
                self.label,
                features=None,
                distance=dist,
                method="brute_force",
                k=self.k,
                radius=self.radius,
            )

    def test_query_methods(self):
        """
        Test query accuracy for various nearest neighbor methods.
        """
        for method in ["auto", "ball_tree", "brute_force"]:
            self._test_query(
                self.refs,
                self.refs,
                self.label,
                features=None,
                distance="euclidean",
                method=method,
            )

    def test_similarity_graph(self):
        """
        Test accuracy and completeness of the similarity graph method for sparse
        data. In this data context, this is identical to querying with the
        reference dataset.
        """
        n = 30
        sf = tc.SFrame()
        sf["docs"] = [random_dict(5, 3) for i in range(n)]
        sf = sf.add_row_number("id")

        m = tc.nearest_neighbors.create(
            sf, label="id", features=None, distance="euclidean", method="brute_force"
        )

        knn = m.query(sf, k=3, verbose=False)
        knn_graph = m.similarity_graph(k=2, output_type="SFrame", verbose=False)

        ## Basic metadata about the output
        self.assertEqual(knn_graph.num_rows(), 60)
        self.assertEqual(knn_graph["rank"].max(), 2)
        self.assertGreaterEqual(knn_graph["distance"].min(), 0.0)

        ## No self edges
        label_diff = knn_graph["query_label"] - knn_graph["reference_label"]
        self.assertEqual(sum(label_diff == 0), 0)

        ## Exact match to query method, adjusting for no self-edges. NOTE: for
        #  this type of data and distance, there are many ties, so the reference
        #  labels won't necessarily match across the two methods. Only check the
        #  three columns that must match exactly.
        knn = knn[knn["rank"] > 1]
        knn["rank"] = knn["rank"] - 1
        test_ftrs = ["query_label", "distance", "rank"]

        assert_frame_equal(
            knn[test_ftrs].to_dataframe(), knn_graph[test_ftrs].to_dataframe()
        )


class NearestNeighborsStringQueryTest(unittest.TestCase):
    """
    Unit test class for checking correctness of string distances in nearest
    neighbors queries.
    """

    @classmethod
    def setUpClass(self):
        np.random.seed(213)
        n = 5
        word_length = 3
        alphabet_size = 5
        self.label = "id"
        self.refs = tc.SFrame({"X1": random_string(n, word_length, alphabet_size)})
        self.refs = self.refs.add_row_number(self.label)

    def _test_query(self, sf_ref, sf_query, features, distance, method):
        """
        Test the accuracy of string queries against the local python function.
        """

        ## Get the toolkit answer
        m = tc.nearest_neighbors.create(
            sf_ref,
            label=self.label,
            features=features,
            distance=distance,
            method=method,
            verbose=False,
        )
        knn = m.query(sf_query, verbose=False)

        ## Compute the answer from scratch
        knn = knn.join(self.refs, on={"query_label": "id"}, how="left")
        knn = knn.join(self.refs, on={"reference_label": "id"}, how="left")

        if distance == "levenshtein":
            knn["test_dist"] = knn.apply(lambda x: levenshtein(x["X1"], x["X1.1"]))
        else:
            raise ValueError("Distance not found in string query test.")

        self.assertAlmostEqual(sum(knn["distance"] - knn["test_dist"]), 0)

    def test_query_distances(self):
        """
        Test query accuracy for various dista0nces. As of v1.1, only levenshtein
        distance is implemented.
        """
        self._test_query(
            self.refs,
            self.refs,
            features=None,
            distance="levenshtein",
            method="brute_force",
        )

    def test_similarity_graph(self):
        """
        Test accuracy and completeness of the similarity graph method for sparse
        data. In this data context, this is identical to querying with the
        reference dataset.
        """
        n = 30
        sf = tc.SFrame({"X1": random_string(n, length=3, num_letters=5)})
        sf = sf.add_row_number("id")

        m = tc.nearest_neighbors.create(
            sf, label="id", features=None, distance="levenshtein", method="brute_force"
        )

        knn = m.query(sf, k=3, verbose=False)
        knn_graph = m.similarity_graph(k=2, output_type="SFrame", verbose=False)

        ## Basic metadata about the output
        self.assertEqual(knn_graph.num_rows(), 60)
        self.assertEqual(knn_graph["rank"].max(), 2)
        self.assertGreaterEqual(knn_graph["distance"].min(), 0.0)

        ## No self edges
        label_diff = knn_graph["query_label"] - knn_graph["reference_label"]
        self.assertEqual(sum(label_diff == 0), 0)

        ## Exact match to query method, adjusting for no self-edges. NOTE: for
        #  this type of data and distance, there are many ties, so the reference
        #  labels won't necessarily match across the two methods. Only check the
        #  three columns that must match exactly.
        knn = knn[knn["rank"] > 1]
        knn["rank"] = knn["rank"] - 1
        test_ftrs = ["query_label", "distance", "rank"]

        assert_frame_equal(
            knn[test_ftrs].to_dataframe(), knn_graph[test_ftrs].to_dataframe()
        )

    def test_missing_queries(self):
        """
        Check that missing string queries are correctly imputed to be empty
        strings.
        """
        sf = tc.SFrame({"x0": ["a", "b"], "x1": ["d", "e"]})

        sf_query = tc.SFrame(
            {"x0": ["a", None, "b", None], "x1": ["b", "c", None, None]}
        )

        m = tc.nearest_neighbors.create(sf, verbose=False)
        knn = m.query(sf_query, k=None, radius=None)

        answer = tc.SFrame(
            {
                "query_label": [0, 0, 1, 1, 2, 2, 3, 3],
                "distance": [1.0, 2.0, 2.0, 2.0, 1.0, 2.0, 2.0, 2.0],
            }
        )

        assert_frame_equal(
            knn[["distance", "query_label"]].to_dataframe(), answer.to_dataframe()
        )


class NearestNeighborsCompositeQueryTest(unittest.TestCase):
    """
    Unit test class for checking correctness of composite distances in nearest
    neighbors queries.
    """

    @classmethod
    def setUpClass(self):
        n, d = 5, 3
        self.refs = tc.SFrame()

        for i in range(d):
            self.refs.add_column(tc.SArray(np.random.rand(n)), inplace=True)

        # self.refs['address'] = random_string(n, length=3, num_letters=5)
        # self.refs['address_dict'] = tc.text_analytics.count_ngrams(
        #     self.refs['address'], n=2, method='character', to_lower=False,
        #     ignore_space=True)

        self.label = "id"
        self.refs = self.refs.add_row_number(self.label)

    def _test_query(self, composite_params):
        """
        Test query accuracy using arbitrary composite distance inputs.
        """
        ## Get the toolkit answer
        m = tc.nearest_neighbors.create(
            self.refs, label=self.label, distance=composite_params, verbose=False
        )
        knn = m.query(self.refs, verbose=False)

        ## Compute the answer from scratch
        dist_ans = []
        for row in knn:
            query = self.refs[row["query_label"]]
            ref = self.refs[row["reference_label"]]
            dist_row = tc.distances.compute_composite_distance(
                composite_params, query, ref
            )
            dist_ans.append(dist_row)

        knn["test_dist"] = dist_ans
        self.assertAlmostEqual(sum(knn["distance"] - knn["test_dist"]), 0)

    def test_composite_queries(self):
        """
        Test query accurateness for a variety of composite distance
        configurations.
        """

        ## Test accuracy over overlapping feature sets
        distance_components = [
            [["X1", "X2"], "euclidean", 1],
            [["X2", "X3"], "manhattan", 1],
        ]
        self._test_query(distance_components)

        ## Test accuracy of different weights
        distance_components = [
            [["X1", "X2"], "euclidean", 2],
            [["X2", "X3"], "manhattan", 3.4],
        ]
        self._test_query(distance_components)

    def test_similarity_graph(self):
        """
        Test accuracy and completeness of the similarity graph method for sparse
        data. In this data context, this is identical to querying with the
        reference dataset.
        """
        n, d = 30, 3
        sf = tc.SFrame(np.random.random((n, d)))
        sf = sf.unpack("X1", column_name_prefix="")
        sf = sf.add_row_number("id")

        my_dist = [[["0", "1"], "euclidean", 1], [["1", "2"], "manhattan", 1]]

        m = tc.nearest_neighbors.create(
            sf, label="id", features=None, distance=my_dist, method="brute_force"
        )

        knn = m.query(sf, k=3, verbose=False)
        knn_graph = m.similarity_graph(k=2, output_type="SFrame", verbose=False)

        ## Basic metadata about the output
        self.assertEqual(knn_graph.num_rows(), 60)
        self.assertEqual(knn_graph["rank"].max(), 2)
        self.assertGreaterEqual(knn_graph["distance"].min(), 0.0)

        ## No self edges
        label_diff = knn_graph["query_label"] - knn_graph["reference_label"]
        self.assertEqual(sum(label_diff == 0), 0)

        ## Exact match to query method, adjusting for no self-edges.
        knn = knn[knn["rank"] > 1]
        knn["rank"] = knn["rank"] - 1
        test_ftrs = ["query_label", "distance", "rank"]

        assert_frame_equal(knn.to_dataframe(), knn_graph.to_dataframe())


class ValidateListUtilityTest(unittest.TestCase):
    """
    Unit test class for checking correctness of utility function.
    """

    def setUp(self):
        self.check_for_numeric_fixed_length_lists = partial(
            _validate_lists,
            allowed_types=[int, float, long],
            require_equal_length=True,
            require_same_type=True,
        )

        self.check_for_str_lists = partial(
            _validate_lists,
            allowed_types=[str],
            require_equal_length=False,
            require_same_type=True,
        )

    def test_str_cases(self):
        lst = [["a", "b"], ["a", "b", "c"]]
        lst = tc.SArray(lst, dtype=list)
        observed = self.check_for_str_lists(lst)
        self.assertTrue(observed)

        lst = [[1, 2, 3], [1, 2, 3, 4]]
        lst = tc.SArray(lst, dtype=list)
        observed = self.check_for_str_lists(lst)
        self.assertFalse(observed)

        lst = [["a", 2, 3], [1, 2, 3, 4]]
        lst = tc.SArray(lst, dtype=list)
        observed = self.check_for_str_lists(lst)
        self.assertFalse(observed)

    def test_true_numeric_cases(self):

        # Same length, all floats
        lst = [[1.0, 2.0, 3.0], [2.0, 3.0, 4.0]]
        lst = tc.SArray(lst, dtype=list)
        observed = self.check_for_numeric_fixed_length_lists(lst)
        self.assertTrue(observed)

        # Same length, all ints
        lst = [[1, 2, 3], [2, 3, 4]]
        lst = tc.SArray(lst, dtype=list)
        observed = self.check_for_numeric_fixed_length_lists(lst)
        self.assertTrue(observed)

        # Only checks the first 10
        lst = [
            [1, 2],
            [2, 3],
            [1, 2],
            [1, 2],
            [2, 3],
            [1, 2],
            [1, 2],
            [2, 3],
            [1, 2],
            [1, 2],
            [2, 3, 4],
            [1, 2, 3],
            [2, 3, 4, 5, 6],
        ]
        lst = tc.SArray(lst, dtype=list)
        observed = self.check_for_numeric_fixed_length_lists(lst)
        self.assertTrue(observed)

        # Empty assumed to be same length, all numeric.
        lst = []
        lst = tc.SArray(lst, dtype=list)
        observed = self.check_for_numeric_fixed_length_lists(lst)
        self.assertTrue(observed)

    def test_false_numeric_cases(self):

        # Different length
        lst = [[1, 2, 3], [1, 2, 3, 4]]
        lst = tc.SArray(lst, dtype=list)
        observed = self.check_for_numeric_fixed_length_lists(lst)
        self.assertFalse(observed)

        # Second list contains different types
        lst = [[1, 2, 3], [1.0, 2, 3]]
        lst = tc.SArray(lst, dtype=list)
        observed = self.check_for_numeric_fixed_length_lists(lst)
        self.assertFalse(observed)

        # Second list is different typed than first
        lst = [[1, 2, 3], [1.0, 2.0, 3.0]]
        lst = tc.SArray(lst, dtype=list)
        observed = self.check_for_numeric_fixed_length_lists(lst)
        self.assertFalse(observed)

        # Different length arrays
        lst = [[], [1.0]]
        lst = tc.SArray(lst, dtype=list)
        observed = self.check_for_numeric_fixed_length_lists(lst)
        self.assertFalse(observed)

        # Strings are not numeric
        str_list = random_list_of_str(10, length=5)
        lst = tc.SArray(lst, dtype=list)
        observed = self.check_for_numeric_fixed_length_lists(lst)
        self.assertFalse(observed)

    def test_bad_cases(self):

        lst = [{"a": 3}, {"b": 5}]
        lst = tc.SArray(lst)
        with self.assertRaises(ValueError):
            observed = self.check_for_numeric_fixed_length_lists(lst)

        lst = [1, 2]
        lst = tc.SArray(lst)
        with self.assertRaises(ValueError):
            observed = self.check_for_numeric_fixed_length_lists(lst)


## Generate sparse data: an SFrame with a column of type dict
def random_dict(num_elements=10, max_count=20):
    words = string.ascii_lowercase
    d = {}
    for j in range(num_elements):
        w = words[random.randint(0, len(words) - 1)]
        d[w] = random.randint(0, max_count)
    return d


def random_string(number, length, num_letters):
    """
    Generate a list of random strings of lower case letters.
    """
    result = []
    letters = string.ascii_letters[:num_letters]

    for i in range(number):
        word = []
        for j in range(length):
            word.append(random.choice(letters))
        result.append("".join(word))

    return result


def random_list_of_str(number, length):
    """
    Generate a list of random lists of strings.
    """
    results = []
    words = random_string(20, 5, 10)

    for i in range(number):
        result = [random.choice(words) for i in range(length)]
        results.append(result)
    return results


def scipy_dist(q, r, dist):
    n = len(r)
    n_query = len(q)

    D = spd.cdist(q, r, dist)
    idx_col = np.argsort(D, axis=1)
    idx_row = np.array([[x] for x in range(n_query)])
    query_labels = list(np.repeat(range(n_query), n))
    ranks = np.tile(range(1, n + 1), n_query)

    answer = tc.SFrame(
        {
            "query_label": query_labels,
            "reference_label": idx_col.flatten(),
            "distance": D[idx_row, idx_col].flatten(),
            "rank": ranks,
        }
    )

    answer.swap_columns("distance", "query_label", inplace=True)
    answer.swap_columns("distance", "reference_label", inplace=True)
    answer.swap_columns("distance", "rank", inplace=True)

    return answer


if __name__ == "__main__":

    # Check if we are supposed to connect to another server
    for i, v in enumerate(sys.argv):
        if v.startswith("ipc://"):
            tc._stop()
            tc._launch(v)

            # The rest of the arguments need to get passed through to
            # the unittest module
            del sys.argv[i]
            break

    unittest.main()

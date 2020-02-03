# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import copy
import unittest
import numpy as np
import turicreate as tc
from turicreate.toolkits._main import ToolkitError
from collections import Counter

import sys

if sys.version_info.major > 2:
    unittest.TestCase.assertItemsEqual = unittest.TestCase.assertCountEqual


class StandardDistancesTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.a = {"a": 0.5, "b": 0.7}
        self.b = {"b": 1.0, "c": 0.1, "d": 0.5}
        self.av = [3, 4, 1]
        self.bv = [1, 2, 3]
        self.al = ["a", "b", "b", "c"]
        self.bl = ["a", "b"]

    def test_euclidean(self):
        self.assertAlmostEqual(
            euclidean(self.a, self.b), tc.distances.euclidean(self.a, self.b)
        )
        self.assertAlmostEqual(
            (2 * 2 + 2 * 2 + 2 * 2) ** 0.5, tc.distances.euclidean(self.av, self.bv)
        )

    def test_squared_euclidean(self):
        self.assertAlmostEqual(
            euclidean(self.a, self.b) ** 2,
            tc.distances.squared_euclidean(self.a, self.b),
        )

    def test_manhattan(self):
        self.assertAlmostEqual(
            manhattan(self.a, self.b), tc.distances.manhattan(self.a, self.b)
        )

    def test_cosine(self):
        self.assertAlmostEqual(
            cosine(self.a, self.b), tc.distances.cosine(self.a, self.b)
        )

    def test_transformed_dot_product(self):
        self.assertAlmostEqual(
            transformed_dot_product(self.a, self.b),
            tc.distances.transformed_dot_product(self.a, self.b),
        )

    def test_jaccard(self):
        self.assertAlmostEqual(
            jaccard(self.a, self.b), tc.distances.jaccard(self.a, self.b)
        )
        self.assertAlmostEqual(
            jaccard(self.al, self.bl), tc.distances.jaccard(self.al, self.bl)
        )

    def test_weighted_jaccard(self):
        self.assertAlmostEqual(
            weighted_jaccard(self.a, self.b),
            tc.distances.weighted_jaccard(self.a, self.b),
        )
        self.assertAlmostEqual(
            weighted_jaccard(self.al, self.bl),
            tc.distances.weighted_jaccard(self.al, self.bl),
        )

    def test_edge_cases(self):
        self.assertAlmostEqual(tc.distances.euclidean({}, {}), 0.0)
        self.assertAlmostEqual(tc.distances.euclidean({}, {"a": 1.0}), 1.0)
        self.assertAlmostEqual(tc.distances.jaccard({}, {}), 0.0)

        dists = [
            "euclidean",
            "squared_euclidean",
            "manhattan",
            "cosine",
            "jaccard",
            "weighted_jaccard",
            "levenshtein",
        ]

        for d in dists:
            dist_fn = tc.distances.__dict__[d]

            with self.assertRaises(ToolkitError):
                dist_fn([1.0], {"a": 1.0})

            with self.assertRaises(ToolkitError):
                dist_fn(5.0, 7.0)


class DistanceUtilsTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.x = {
            "a": 1.0,
            "b": 1.0,
            "c": 1,
            "d": [1.0, 2.0, 3.0],
            "e": {"cat": 10, "dog": 11, "fossa": 12},
            "f": "what on earth is a fossa?",
        }

        self.y = {
            "a": 2.0,
            "b": 3.0,
            "c": 4.0,
            "d": [4.0, 5.0, 6.0],
            "e": {"eel": 5, "dog": 12, "fossa": 10},
            "f": "a fossa is the best animal on earth",
        }

        self.dist = [
            [("a", "b", "c"), "euclidean", 1],
            [("d",), "manhattan", 2],
            [("e",), "jaccard", 1.5],
            [("f",), "levenshtein", 0.3],
        ]

    def test_composite_dist_validation(self):
        """
        Make sure the composite distance validation utility allows good
        distances through and catches bad distances.
        """

        ## A good distance
        try:
            tc.distances._util._validate_composite_distance(self.dist)
        except:
            assert False, "Composite distance validation failed for a good distance."

        ## An empty distance
        with self.assertRaises(ValueError):
            tc.distances._util._validate_composite_distance([])

        ## A distance not in a list
        with self.assertRaises(TypeError):
            tc.distances._util._validate_composite_distance(tuple(self.dist))

        ## Empty feature list
        dist = copy.deepcopy(self.dist)
        dist.append([[], "euclidean", 13])

        with self.assertRaises(ValueError):
            tc.distances._util._validate_composite_distance(dist)

        ## Feature list with non-strings
        dist = copy.deepcopy(self.dist)
        dist.append([["test", 17], "manhattan", 13])

        with self.assertRaises(TypeError):
            tc.distances._util._validate_composite_distance(dist)

        ## Distance function in the wrong form
        dist = copy.deepcopy(self.dist)
        dist.append([["d"], 17, 13])

        with self.assertRaises(ValueError):
            tc.distances._util._validate_composite_distance(dist)

        ## Non-existent distance function
        dist = copy.deepcopy(self.dist)
        dist.append([["d"], "haversine", 13])

        with self.assertRaises(ValueError):
            tc.distances._util._validate_composite_distance(dist)

        ## Weight of the wrong type
        dist = copy.deepcopy(self.dist)
        dist.append([["d"], "euclidean", "a lot"])

        with self.assertRaises(ValueError):
            tc.distances._util._validate_composite_distance(dist)

    def test_composite_feature_scrub(self):
        """
        Make sure excluded features are properly removed from a composite
        distance specification.
        """
        dist = [
            [("a", "b", "c", "goat"), "euclidean", 1],
            [("d", "horse", "goat"), "manhattan", 2],
            [("e", "ibex", "ibex"), "jaccard", 1.5],
            [("f",), "levenshtein", 0.3],
        ]

        ## Test basic functionality
        feature_blacklist = ["goat", "horse", "ibex"]
        ans = tc.distances._util._scrub_composite_distance_features(
            dist, feature_blacklist
        )

        for d, d_ans in zip(self.dist, ans):
            self.assertSequenceEqual(d[0], d_ans[0])

        ## Test removal of an entire distance component
        feature_blacklist.append("f")  # should remove the entire last component
        ans = tc.distances._util._scrub_composite_distance_features(
            dist, feature_blacklist
        )
        self.assertEqual(len(ans), 3)
        self.assertItemsEqual(
            tc.distances._util._get_composite_distance_features(ans),
            ["a", "b", "c", "d", "e"],
        )

    def test_composite_dist_type_convert(self):
        """
        Make sure the utility to convert distance names to function handles
        works properly.
        """
        converted_dist = tc.distances._util._convert_distance_names_to_functions(
            self.dist
        )

        ans = [
            tc.distances.euclidean,
            tc.distances.manhattan,
            tc.distances.jaccard,
            tc.distances.levenshtein,
        ]

        self.assertSequenceEqual(ans, [x[1] for x in converted_dist])

    def test_composite_dist_compute(self):
        """
        Check the correctness of the composite distance computation utility.
        """

        ## Check that d(x, x) = 0
        d = tc.distances.compute_composite_distance(self.dist, self.x, self.x)
        self.assertAlmostEqual(d, 0.0)

        d = tc.distances.compute_composite_distance(self.dist, self.y, self.y)
        self.assertAlmostEqual(d, 0.0)

        ## Check the distance between two data points against the hard-coded
        #  answer.
        d = tc.distances.compute_composite_distance(self.dist, self.x, self.y)
        self.assertAlmostEqual(d, 30.29165739, places=5)

        ## Check the distance against the nearest neighbors toolkit
        sf = tc.SFrame([self.x, self.y]).unpack("X1", column_name_prefix="")
        m = tc.nearest_neighbors.create(sf, distance=self.dist, verbose=False)
        knn = m.query(sf[:1], k=2, verbose=False)
        self.assertAlmostEqual(d, knn["distance"][1], places=5)

    def test_composite_features_extract(self):
        """
        Test the utility that returns the union of features in a composite
        distance.
        """
        dist = copy.deepcopy(self.dist)
        dist.append([["a", "b", "a"], "cosine", 13])
        ans = ["a", "b", "c", "d", "e", "f"]

        self.assertItemsEqual(
            ans, tc.distances._util._get_composite_distance_features(dist)
        )


class LocalDistancesTest(unittest.TestCase):
    """
    Unit test for the distances computed in this script.
    """

    @classmethod
    def setUpClass(self):
        self.a = {"a": 0.5, "b": 0.7}
        self.b = {"b": 1.0, "c": 0.1, "d": 0.5}
        self.S = "fossa"
        self.T = "fossil"

    def test_local_jaccard(self):
        self.assertAlmostEqual(jaccard(self.a, self.b), 1 - 1.0 / 4)
        self.assertAlmostEqual(jaccard(self.a, {}), 1)
        self.assertAlmostEqual(jaccard(self.a, self.a), 0)

    def test_local_weighted_jaccard(self):
        ans = 1 - (0.0 + 0.7 + 0.0 + 0.0) / (0.5 + 1.0 + 0.1 + 0.5)
        self.assertAlmostEqual(weighted_jaccard(self.a, self.b), ans)

        self.assertAlmostEqual(weighted_jaccard(self.a, {}), 1)
        self.assertAlmostEqual(weighted_jaccard(self.a, self.a), 0)

    def test_local_cosine(self):
        ans = 1 - (
            0.7 / ((0.5 ** 2 + 0.7 ** 2) ** 0.5 * (1 ** 2 + 0.1 ** 2 + 0.5 ** 2) ** 0.5)
        )

        self.assertAlmostEqual(cosine(self.a, self.b), ans)
        self.assertAlmostEqual(cosine(self.a, {}), 1)
        self.assertAlmostEqual(cosine(self.a, self.a), 0)

    def test_local_transformed_dot_product(self):
        ans = np.log(1.0 + np.exp(-0.7))
        self.assertAlmostEqual(transformed_dot_product(self.a, self.b), ans)

        ans = np.log(1 + np.exp(-1 * (0.5 ** 2 + 0.7 ** 2)))
        self.assertAlmostEqual(transformed_dot_product(self.a, self.a), ans)

    def test_local_euclidean(self):
        self.assertAlmostEqual(euclidean(self.a, self.a), 0)

        ans = ((0.5) ** 2 + (1.0 - 0.7) ** 2 + (0.1) ** 2 + (0.5) ** 2) ** 0.5
        self.assertAlmostEqual(euclidean(self.a, self.b), ans)

        ans = ((0.5) ** 2 + (0.7) ** 2) ** 0.5
        self.assertAlmostEqual(euclidean(self.a, {}), ans)

    def test_local_squared_euclidean(self):
        self.assertAlmostEqual(squared_euclidean(self.a, self.a), 0)

        ans = (0.5) ** 2 + (1.0 - 0.7) ** 2 + (0.1) ** 2 + (0.5) ** 2
        self.assertAlmostEqual(squared_euclidean(self.a, self.b), ans)

        ans = (0.5) ** 2 + (0.7) ** 2
        self.assertAlmostEqual(squared_euclidean(self.a, {}), ans)

    def test_local_manhattan(self):
        self.assertAlmostEqual(manhattan(self.a, self.a), 0)

        ans = 0.5 + (1.0 - 0.7) + (0.1) + (0.5)
        self.assertAlmostEqual(manhattan(self.a, self.b), ans)

        ans = (0.5) + (0.7)
        self.assertAlmostEqual(manhattan(self.a, {}), ans)

    def test_local_levenshtein(self):
        self.assertEqual(levenshtein(self.S, self.T), 2)
        self.assertEqual(levenshtein(self.S, self.S), 0)
        self.assertEqual(levenshtein(self.T, self.T), 0)
        self.assertEqual(levenshtein(self.S, ""), len(self.S))
        self.assertEqual(levenshtein(self.T, ""), len(self.T))


### -------------------------------------------------------------- ###
### Local distance functions to check the toolkit distance against ###
### -------------------------------------------------------------- ###
def jaccard(a, b):
    if isinstance(a, dict) and isinstance(b, dict):
        a = a.keys()
        b = b.keys()
    a = set(a)
    b = set(b)
    ans = 1.0 - float(len(a.intersection(b))) / len(a.union(b))
    return ans


def weighted_jaccard(a, b):
    if isinstance(a, list) and isinstance(b, list):
        a = dict(Counter(a))
        b = dict(Counter(b))

    a2 = a.copy()
    b2 = b.copy()

    numer = 0
    denom = 0
    keys = set(list(a.keys()) + list(b.keys()))
    for k in keys:
        a2.setdefault(k, 0)
        b2.setdefault(k, 0)
        numer += min(a2[k], b2[k])
        denom += max(a2[k], b2[k])
    return 1.0 - float(numer) / denom


def cosine(a, b):
    ks = set(a.keys()).intersection(set(b.keys()))
    num = sum([a[k] * b[k] for k in ks])
    den = sum([v ** 2 for k, v in a.items()]) * sum([v ** 2 for k, v in b.items()])
    den = den ** 0.5
    if den == 0:
        den = 0.0001
    return 1 - num / den


def transformed_dot_product(a, b):
    ks = set(a.keys()).intersection(set(b.keys()))
    dotprod = sum([a[k] * b[k] for k in ks])
    return np.log(1 + np.exp(-1 * dotprod))


def euclidean(a, b):
    return squared_euclidean(a, b) ** 0.5


def squared_euclidean(a, b):
    a2 = a.copy()
    b2 = b.copy()

    ans = 0
    keys = set(a.keys()).union(set(b.keys()))
    for k in keys:
        a2.setdefault(k, 0)
        b2.setdefault(k, 0)
        ans += (a2[k] - b2[k]) ** 2
    return ans


def manhattan(a, b):
    a2 = a.copy()
    b2 = b.copy()

    ans = 0
    keys = set(a.keys()).union(set(b.keys()))
    for k in keys:
        a2.setdefault(k, 0)
        b2.setdefault(k, 0)
        ans += abs(a2[k] - b2[k])
    return ans


def levenshtein(a, b):
    m = len(a)
    n = len(b)
    D = np.zeros((m + 1, n + 1), dtype=int)

    D[:, 0] = np.arange(m + 1)
    D[0, :] = np.arange(n + 1)

    for j in range(1, n + 1):
        for i in range(1, m + 1):
            if a[i - 1] == b[j - 1]:
                D[i, j] = D[i - 1, j - 1]
            else:
                D[i, j] = min(D[i - 1, j] + 1, D[i, j - 1] + 1, D[i - 1, j - 1] + 1)

    return D[m, n]

# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
import turicreate as tc
from turicreate.toolkits._decision_tree import DecisionTree, Node
from turicreate.toolkits._main import ToolkitError


def _make_tree(sf):
    model = tc.decision_tree_classifier.create(
        sf, "target", validation_set=None, max_depth=10
    )

    tree = DecisionTree.from_model(model)
    return tree


class PythonDecisionTreeCorrectness(unittest.TestCase):
    def test_categorical(self):
        # Arrange
        sf = tc.SFrame(
            {
                "cat1": ["1", "1", "2", "2", "2"] * 100,
                "cat2": ["1", "3", "3", "1", "1"] * 100,
                "target": ["1", "2", "1", "2", "1"] * 100,
            }
        )

        # Act
        tree = _make_tree(sf)
        root = tree.root

        # Check the root node.
        self.assertEqual(len(tree.nodes), 7)
        self.assertEqual(
            root.to_dict(),
            {
                "is_leaf": False,
                "left_id": 2,
                "node_id": 0,
                "missing_id": 1,
                "node_type": u"indicator",
                "parent_id": None,
                "right_id": 1,
                "split_feature_column": "cat1",
                "split_feature_index": "1",
                "value": 1,
            },
        )

        # Check prediction paths.
        self.assertEqual(tree.get_prediction_path(0), [])
        self.assertEqual(
            tree.get_prediction_path(1),
            [
                {
                    "child_id": 1,
                    "feature": "cat1",
                    "index": "1",
                    "node_type": "indicator",
                    "node_id": 0,
                    "sign": "!=",
                    "value": 1,
                    "is_missing": False,
                }
            ],
        )
        self.assertEqual(
            tree.get_prediction_path(2),
            [
                {
                    "child_id": 2,
                    "feature": "cat1",
                    "index": "1",
                    "node_id": 0,
                    "sign": "=",
                    "value": 1,
                    "node_type": "indicator",
                    "is_missing": False,
                }
            ],
        )
        self.assertEqual(
            tree.get_prediction_path(3),
            [
                {
                    "child_id": 1,
                    "feature": "cat1",
                    "index": "1",
                    "node_id": 0,
                    "sign": "!=",
                    "value": 1,
                    "node_type": "indicator",
                    "is_missing": False,
                },
                {
                    "child_id": 3,
                    "feature": "cat2",
                    "index": "1",
                    "node_id": 1,
                    "sign": "!=",
                    "value": 1,
                    "node_type": "indicator",
                    "is_missing": False,
                },
            ],
        )
        self.assertEqual(
            tree.get_prediction_path(4),
            [
                {
                    "child_id": 1,
                    "feature": "cat1",
                    "index": "1",
                    "node_id": 0,
                    "sign": "!=",
                    "value": 1,
                    "node_type": "indicator",
                    "is_missing": False,
                },
                {
                    "child_id": 4,
                    "feature": "cat2",
                    "index": "1",
                    "node_id": 1,
                    "sign": "=",
                    "value": 1,
                    "node_type": "indicator",
                    "is_missing": False,
                },
            ],
        )
        self.assertEqual(
            tree.get_prediction_path(5),
            [
                {
                    "child_id": 2,
                    "feature": "cat1",
                    "index": "1",
                    "node_id": 0,
                    "sign": "=",
                    "value": 1,
                    "node_type": "indicator",
                    "is_missing": False,
                },
                {
                    "child_id": 5,
                    "feature": "cat2",
                    "index": "1",
                    "node_id": 2,
                    "sign": "!=",
                    "value": 1,
                    "node_type": "indicator",
                    "is_missing": False,
                },
            ],
        )
        self.assertEqual(
            tree.get_prediction_path(6),
            [
                {
                    "child_id": 2,
                    "feature": "cat1",
                    "index": "1",
                    "node_id": 0,
                    "sign": "=",
                    "value": 1,
                    "node_type": "indicator",
                    "is_missing": False,
                },
                {
                    "child_id": 6,
                    "feature": "cat2",
                    "index": "1",
                    "node_id": 2,
                    "sign": "=",
                    "value": 1,
                    "node_type": "indicator",
                    "is_missing": False,
                },
            ],
        )

    def test_dict(self):
        # Arrange
        sf = tc.SFrame(
            {
                "cat1": ["1", "1", "2", "2", "2"] * 100,
                "cat2": ["1", "3", "3", "1", "1"] * 100,
                "target": ["1", "2", "1", "2", "1"] * 100,
            }
        )
        sf["cat1"] = sf["cat1"].apply(lambda x: {x: 1})
        sf["cat2"] = sf["cat2"].apply(lambda x: {x: 1})

        # Act
        tree = _make_tree(sf)
        root = tree.root

        # Check the root node.
        self.assertEqual(len(tree.nodes), 7)
        self.assertEqual(
            root.to_dict(),
            {
                "is_leaf": False,
                "left_id": 1,
                "node_id": 0,
                "node_type": u"float",
                "parent_id": None,
                "right_id": 2,
                "missing_id": 1,
                "split_feature_column": "cat1",
                "split_feature_index": "1",
                "value": -1e-5,
            },
        )

        # Check prediction paths.
        self.assertEqual(tree.get_prediction_path(0), [])
        self.assertEqual(
            tree.get_prediction_path(1),
            [
                {
                    "child_id": 1,
                    "feature": "cat1",
                    "index": "1",
                    "node_id": 0,
                    "sign": "<",
                    "value": -1e-5,
                    "node_type": "float",
                    "is_missing": False,
                }
            ],
        )
        self.assertEqual(
            tree.get_prediction_path(2),
            [
                {
                    "child_id": 2,
                    "feature": "cat1",
                    "index": "1",
                    "node_id": 0,
                    "sign": ">=",
                    "value": -1e-5,
                    "node_type": "float",
                    "is_missing": False,
                }
            ],
        )
        self.assertEqual(
            tree.get_prediction_path(3),
            [
                {
                    "child_id": 1,
                    "feature": "cat1",
                    "index": "1",
                    "node_id": 0,
                    "sign": "<",
                    "value": -1e-05,
                    "node_type": "float",
                    "is_missing": False,
                },
                {
                    "child_id": 3,
                    "feature": "cat2",
                    "index": "1",
                    "node_id": 1,
                    "sign": "<",
                    "value": -1e-05,
                    "node_type": "float",
                    "is_missing": False,
                },
            ],
        )
        self.assertEqual(
            tree.get_prediction_path(4),
            [
                {
                    "child_id": 1,
                    "feature": "cat1",
                    "index": "1",
                    "node_id": 0,
                    "sign": "<",
                    "value": -1e-05,
                    "node_type": "float",
                    "is_missing": False,
                },
                {
                    "child_id": 4,
                    "feature": "cat2",
                    "index": "1",
                    "node_id": 1,
                    "sign": ">=",
                    "value": -1e-05,
                    "node_type": "float",
                    "is_missing": False,
                },
            ],
        )
        self.assertEqual(
            tree.get_prediction_path(5),
            [
                {
                    "child_id": 2,
                    "feature": "cat1",
                    "index": "1",
                    "node_id": 0,
                    "sign": ">=",
                    "value": -1e-05,
                    "node_type": "float",
                    "is_missing": False,
                },
                {
                    "child_id": 5,
                    "feature": "cat2",
                    "index": "1",
                    "node_id": 2,
                    "sign": "<",
                    "value": -1e-05,
                    "node_type": "float",
                    "is_missing": False,
                },
            ],
        )
        self.assertEqual(
            tree.get_prediction_path(6),
            [
                {
                    "child_id": 2,
                    "feature": "cat1",
                    "index": "1",
                    "node_id": 0,
                    "sign": ">=",
                    "value": -1e-05,
                    "node_type": "float",
                    "is_missing": False,
                },
                {
                    "child_id": 6,
                    "feature": "cat2",
                    "index": "1",
                    "node_id": 2,
                    "sign": ">=",
                    "value": -1e-05,
                    "node_type": "float",
                    "is_missing": False,
                },
            ],
        )

    def test_cat_dict(self):
        # Arrange
        sf = tc.SFrame(
            {
                "cat1": [str(i) for i in range(500)],
                "dict2": [
                    {"1": 1, "2": 3.2},
                    {"1": 3.1,},
                    {1: 1, "b": 2},
                    {1: 1, "b": 3},
                    {"a": 2, "b": 3},
                ]
                * 100,
                "target": ["1", "2", "1", "2", "1"] * 100,
            }
        )

        # Act
        tree = _make_tree(sf)
        root = tree.root

        # Assert.
        self.assertEqual(len(tree.nodes), 7)
        self.assertEqual(
            root.to_dict(),
            {
                "is_leaf": False,
                "left_id": 1,
                "node_id": 0,
                "parent_id": None,
                "right_id": 2,
                "split_feature_column": "dict2",
                "split_feature_index": "1",
                "value": 2.05,
                "node_type": "float",
                "missing_id": 1,
            },
        )

    def test_numeric(self):
        sf = tc.SFrame(
            {
                "num1": [1, 2, 3.5, 4, 5] * 100,
                "num2": [1, 2, 3.5, 4, 5] * 100,
                "num3": [1, 2, 3.5, 4, 5] * 100,
                "target": ["1", "2", "1", "2", "1"] * 100,
            }
        )

        # Act
        tree = _make_tree(sf)
        root = tree.root

        # Assert.
        self.assertEqual(len(tree.nodes), 9)
        self.assertEqual(
            root.to_dict(),
            {
                "is_leaf": False,
                "left_id": 1,
                "node_id": 0,
                "parent_id": None,
                "right_id": 2,
                "split_feature_column": "num1",
                "split_feature_index": None,
                "value": 4.5,
                "node_type": "float",
                "missing_id": 1,
            },
        )

    def test_vector(self):
        sf = tc.SFrame(
            {
                "num1": [1, 2, 3.5, 4, 5] * 100,
                "num2": [1, 2, 3.5, 4, 5] * 100,
                "vect": [[1, 2, 3.5, 4, 5]] * 500,
                "target": ["1", "2", "1", "2", "1"] * 100,
            }
        )

        # Act
        tree = _make_tree(sf)
        root = tree.root

        # Assert.
        self.assertEqual(len(tree.nodes), 9)
        self.assertEqual(
            root.to_dict(),
            {
                "is_leaf": False,
                "left_id": 1,
                "node_id": 0,
                "parent_id": None,
                "right_id": 2,
                "split_feature_column": "num1",
                "split_feature_index": None,
                "value": 4.5,
                "node_type": "float",
                "missing_id": 1,
            },
        )

    def test_numeric_dict(self):
        sf = tc.SFrame(
            {
                "num1": [1, 2, 3.5, 4, 5] * 100,
                "num2": [1, 2, 3.5, 4, 5] * 100,
                "vect": [[1, 2, 3.5, 4, 5]] * 500,
                "target": ["1", "2", "1", "2", "1"] * 100,
                "dict[2]": [
                    {"1": 1, "2": 3.2},
                    {"1": 3.1,},
                    {1: 1, "b": 2},
                    {1: 1, "b": 3},
                    {"a": 2, "b": 3},
                ]
                * 100,
            }
        )

        # Act
        tree = _make_tree(sf)
        root = tree.root

        # Assert.
        self.assertEqual(len(tree.nodes), 7)
        self.assertEqual(
            root.to_dict(),
            {
                "is_leaf": False,
                "left_id": 1,
                "node_id": 0,
                "parent_id": None,
                "right_id": 2,
                "split_feature_column": "dict[2]",
                "split_feature_index": "1",
                "value": 2.05,
                "node_type": "float",
                "missing_id": 1,
            },
        )


class PythonDecisionTreeAllModelsTest(unittest.TestCase):
    def _run_test(self, sf):

        sf["target"] = [i < sf.num_rows() / 2 for i in range(sf.num_rows())]

        for model in [
            tc.regression.boosted_trees_regression,
            tc.classifier.boosted_trees_classifier,
            tc.regression.random_forest_regression,
            tc.classifier.random_forest_classifier,
            tc.regression.decision_tree_regression,
            tc.classifier.decision_tree_classifier,
        ]:
            m = model.create(sf, "target", validation_set=None, max_depth=2)
            tree = DecisionTree.from_model(m)
            for nid, node in tree.nodes.items():
                val = tree.get_prediction_score(nid)
                if node.is_leaf:
                    self.assertTrue(type(val) in {float, int})
                else:
                    self.assertEqual(val, None)

    def test_categorical_1(self):

        sf = tc.SFrame(
            {
                "cat1": ["1", "1", "2", "2", "2"] * 100,
                "cat2": ["1", "3", "3", "1", "1"] * 100,
            }
        )
        self._run_test(sf)

    def test_categorical_2(self):
        sf = tc.SFrame(
            {
                "cat[1]": ["1", "1", "2", "2", "2"] * 100,
                "cat[2]": ["1", "3", "3", "1", "1"] * 100,
            }
        )
        self._run_test(sf)

    def test_dict_1(self):
        sf = tc.SFrame(
            {
                "dict1": [
                    {"1": 1, "2": 3.2},
                    {"1": 3.1,},
                    {"1": 1, "b": 2},
                    {"1": 1, "b": 3},
                    {"a": 2, "b": 3},
                ]
                * 100
            }
        )
        self._run_test(sf)

    def test_dict_2(self):
        sf = tc.SFrame(
            {
                "dict1": [
                    {"1": 1, "2": 3.2},
                    {"1": 3.1,},
                    {1: 1, "b": 2},
                    {1: 1, "b": 3},
                    {"a": 2, "b": 3},
                ]
                * 100
            }
        )
        self._run_test(sf)

    def test_dict_3(self):
        sf = tc.SFrame(
            {
                "dict": [
                    {"1": 1, "2": 3.2},
                    {"1": 3.1,},
                    {1: 1, "b": 2},
                    {1: 1, "b": 3},
                    {"a": 2, "b": 3},
                ]
                * 100,
                "dict[2]": [
                    {"1": 1, "2": 3.2},
                    {"1": 3.1,},
                    {1: 1, "b": 2},
                    {1: 1, "b": 3},
                    {"a": 2, "b": 3},
                ]
                * 100,
                "dict[3]": [
                    {"1": 1, "2": 3.2},
                    {"1": 3.1,},
                    {1: 1, "b": 2},
                    {1: 1, "b": 3},
                    {"a": 2, "b": 3},
                ]
                * 100,
            }
        )

        self._run_test(sf)

    def test_cat_dict_1(self):
        sf = tc.SFrame(
            {
                "cat1": [str(i) for i in range(500)],
                "dict2": [
                    {"1": 1, "2": 3.2},
                    {"1": 3.1,},
                    {1: 1, "b": 2},
                    {1: 1, "b": 3},
                    {"a": 2, "b": 3},
                ]
                * 100,
            }
        )

        self._run_test(sf)

    def test_numeric_1(self):
        sf = tc.SFrame(
            {
                "num1": [1, 2, 3.5, 4, 5] * 100,
                "num2": [1, 2, 3.5, 4, 5] * 100,
                "num3": [1, 2, 3.5, 4, 5] * 100,
            }
        )

        self._run_test(sf)

    def test_numeric_2(self):
        sf = tc.SFrame(
            {
                "num1": [1, 2, 3.5, 4, 5] * 100,
                "num2": [1, 2, 3.5, 4, 5] * 100,
                "vect": [[1, 2, 3.5, 4, 5]] * 500,
            }
        )

        self._run_test(sf)

    def test_numeric_dict(self):
        sf = tc.SFrame(
            {
                "num1": [1, 2, 3.5, 4, 5] * 100,
                "num2": [1, 2, 3.5, 4, 5] * 100,
                "vect": [[1, 2, 3.5, 4, 5]] * 500,
                "dict[2]": [
                    {"1": 1, "2": 3.2},
                    {"1": 3.1,},
                    {1: 1, "b": 2},
                    {1: 1, "b": 3},
                    {"a": 2, "b": 3},
                ]
                * 100,
            }
        )
        self._run_test(sf)


class PythonDecisionTreeTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):

        sf = tc.SFrame(
            {
                "cat1": ["1", "1", "2", "2", "2"] * 100,
                "cat2": ["1", "3", "3", "1", "1"] * 100,
                "target": ["1", "2", "1", "2", "1"] * 100,
            }
        )
        model = tc.classifier.boosted_trees_classifier.create(
            sf, "target", validation_set=None, max_depth=2
        )
        tree = DecisionTree.from_model(model)
        self.tree = tree

    def test_repr(self):
        # Arrange
        tree = self.tree

        # Act
        out = tree.__repr__()

        # Assert
        self.assertEqual(type(out), str)

    def test_to_json(self):
        # Arrange
        tree = self.tree

        # Act
        out = tree.to_json()

        # Assert
        self.assertEqual(type(out), dict)
        with self.assertRaises(TypeError):
            score = tree.to_json("foo")
        with self.assertRaises(ToolkitError):
            score = tree.to_json(-1)

    def get_prediction_score(self):
        # Arrange
        tree = self.tree

        # Act
        out_1 = tree.get_prediction_score(0)
        out_2 = tree.get_prediction_score(5)

        # Assert
        self.assertEqual(out_1, None)
        self.assertEqual(type(out_1), float)
        with self.assertRaises(TypeError):
            score = tree.get_prediction_score("foo")
        with self.assertRaises(ToolkitError):
            score = tree.get_prediction_score(-1)

    def get_prediction_path(self, node_id):
        # Arrange
        tree = self.tree

        # Act
        out_1 = tree.get_prediction_path(0)
        out_2 = tree.get_prediction_path(5)

        # Assert
        self.assertEqual(type(out_1), dict)
        self.assertEqual(type(out_2), dict)
        with self.assertRaises(TypeError):
            score = tree.get_prediction_path("foo")
        with self.assertRaises(ToolkitError):
            score = tree.get_prediction_path(-1)

    def root(self):
        # Arrange
        tree = self.tree

        # Act
        out = tree.root

        # Assert
        self.assertEqual(type(out), Node)

    def test_getitem(self):
        # Arrange
        tree = self.tree

        # Act & Assert
        for i in range(tree.num_nodes):
            self.assertEqual(type(tree[i]), Node)

    def test_iter(self):
        # Arrange
        tree = self.tree

        # Act & Assert
        for node in tree:
            self.assertEqual(type(node), Node)

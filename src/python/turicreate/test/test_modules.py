# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
import turicreate as tc
import inspect

class LoadModuleTest(unittest.TestCase):
    """
    Unit test class for testing a classifier model.
    """
    def test_module(self):
        expected_module_set = ['_connect', '_cython', '_deps', '_scripts',
                               '_sys', '_sys_util', 'activity_classifier',
                               'aggregate', 'audio_analysis',
                               'boosted_trees_classifier', 
                               'boosted_trees_regression', 'classifier',
                               'clustering', 'config', 'connected_components',
                               'data_structures', 'dbscan',
                               'decision_tree_classifier',
                               'decision_tree_regression', 'degree_counting',
                               'distances', 'drawing_classifier', 'evaluation',
                               'factorization_recommender', 'graph_analytics',
                               'graph_coloring', 'image_analysis',
                               'image_classifier', 'image_similarity',
                               'item_content_recommender',
                               'item_similarity_recommender', 'kcore', 'kmeans',
                               'label_propagation', 'linear_regression',
                               'logistic_classifier', 
                               'nearest_neighbor_classifier',
                               'nearest_neighbors', 'object_detector',
                               'one_shot_object_detector', 'pagerank',
                               'popularity_recommender',
                               'random_forest_classifier',
                               'random_forest_regression',
                               'ranking_factorization_recommender',
                               'recommender', 'regression', 'shortest_path', 
                               'sound_classifier', 'style_transfer',
                               'svm_classifier', 'text_analytics',
                               'text_classifier', 'toolkits', 'topic_model',
                               'triangle_counting', 'turicreate', 'util',
                               'version_info', 'visualization']

        tc_modules = inspect.getmembers(tc, inspect.ismodule)
        tc_keys = [x[0] for x in tc_modules]

        self.assertTrue(len(expected_module_set) == len(tc_keys));

        tc_keys.sort()
        expected_module_set.sort()

        self.assertTrue(tc_keys == expected_module_set);

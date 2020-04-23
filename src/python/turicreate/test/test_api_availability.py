# -*- coding: utf-8 -*-
# Copyright © 2020 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
import turicreate
import inspect

MODULES = ["clustering", "data_structures", "graph_analytics", "recommender"]
CLUSTER = ["kmeans", "dbscan"]
RECOMMENDERS = [
    "popularity_recommender",
    "item_similarity_recommender",
    "factorization_recommender",
    "ranking_factorization_recommender",
    "item_content_recommender",
]
DATA_STRUCTURES = ["SFrame", "SArray", "SGraph", "Vertex", "Edge"]
GRAPH_ANALYTICS = [
    "connected_components",
    "graph_coloring",
    "kcore",
    "load_sgraph",
    "pagerank",
    "triangle_counting",
    "shortest_path",
]
GENERAL = ["load_model", "load_sframe", "load_sarray"]


class TuriTests(unittest.TestCase):
    def test_top_level(self):
        for x in (
            MODULES
            + CLUSTER
            + DATA_STRUCTURES
            + GRAPH_ANALYTICS
            + GENERAL
            + RECOMMENDERS
        ):
            self.assertTrue(x in dir(turicreate))

        # TODO Test whether or not things are NOT visible


def lazy_modules_force_load():
    mods = [
        turicreate.recommender.factorization_recommender,
        turicreate.recommender.ranking_factorization_recommender,
        turicreate.recommender.item_similarity_recommender,
        turicreate.recommender.item_content_recommender,
        turicreate.recommender.popularity_recommender,
        turicreate.nearest_neighbors,
        turicreate.text_analytics,
        turicreate.logistic_classifier,
        turicreate.toolkits.classifier.boosted_trees_classifier,
        turicreate.toolkits.classifier.random_forest_classifier,
        turicreate.toolkits.object_detector,
        turicreate.toolkits.one_shot_object_detector,
        turicreate.toolkits.image_similarity,
        turicreate.toolkits.image_classifier,
        turicreate.toolkits.text_classifier,
        turicreate.toolkits.sound_classifier,
        turicreate.toolkits.style_transfer,
        turicreate.toolkits.activity_classifier,
        turicreate.toolkits.drawing_classifier,
    ]

    for mod in mods:
        if isinstance(mod, turicreate._DeferredModuleLoader):
            mod.get_module()


def get_visible_items(d):
    return [x for x in dir(d) if not x.startswith("_")]


def check_visible_modules(actual, expected):
    a_set = set(actual)
    e_set = set(expected)
    assert a_set == e_set, (
        "API Surface mis-matched."
        "expected: %s\nactual: %s\nactual - expected: %s\nexpected - actual: %s"
        % (e_set, a_set, a_set - e_set, e_set - a_set)
    )


class TabCompleteVisibilityTests(unittest.TestCase):
    def test_kmeans(self):
        # Testing an instantiated class that inherits from Model
        sf = turicreate.SFrame({"d1": [1, 2, 3, 4, 5, 6], "d2": [5, 4, 3, 2, 1, 6]})
        m = turicreate.kmeans.create(sf, num_clusters=3, verbose=False)

        expected = [
            "batch_size",
            "row_label_name",
            "cluster_id",
            "cluster_info",
            "features",
            "max_iterations",
            "method",
            "num_clusters",
            "num_examples",
            "num_features",
            "num_unpacked_features",
            "save",
            "summary",
            "training_iterations",
            "training_time",
            "unpacked_features",
            "predict",
        ]

        actual = [x for x in dir(m) if not x.startswith("_")]

        for x in actual:
            print(x)

        print("\n")

        for x in expected:
            print(x)

        check_visible_modules(actual, expected)

    def test_supervised(self):
        # Testing an instantiated class that inherits from Model
        sf = turicreate.SFrame({"d1": [1, 2, 3, 4, 5, 6], "d2": [5, 4, 3, 2, 1, 6]})
        m = turicreate.linear_regression.create(sf, target="d1")

        expected = [
            "coefficients",
            "convergence_threshold",
            "disable_posttrain_evaluation",
            "evaluate",
            "export_coreml",
            "feature_rescaling",
            "features",
            "l1_penalty",
            "l2_penalty",
            "lbfgs_memory_level",
            "max_iterations",
            "num_coefficients",
            "num_examples",
            "num_features",
            "num_unpacked_features",
            "predict",
            "progress",
            "save",
            "solver",
            "step_size",
            "summary",
            "target",
            "training_iterations",
            "training_loss",
            "training_max_error",
            "training_rmse",
            "training_solver_status",
            "training_time",
            "unpacked_features",
            "validation_data",
        ]

        actual = [x for x in dir(m) if not x.startswith("_")]
        check_visible_modules(actual, expected)

    def test_topic_model(self):
        sa = turicreate.SArray([{"a": 5, "b": 3}, {"a": 1, "b": 5, "c": 3}])
        m = turicreate.topic_model.create(sa)

        expected = [
            "alpha",
            "beta",
            "evaluate",
            "get_topics",
            "num_burnin",
            "num_iterations",
            "num_topics",
            "predict",
            "print_interval",
            "save",
            "summary",
            "topics",
            "training_iterations",
            "training_time",
            "validation_time",
            "verbose",
            "vocabulary",
        ]

        actual = [x for x in dir(m) if not x.startswith("_")]
        check_visible_modules(actual, expected)


class ModuleVisibilityTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        lazy_modules_force_load()

    def test_Image_type(self):
        expected = ["Image"]
        actual = [x for x in dir(turicreate.data_structures.image) if "_" not in x]

        self.assertTrue(set(actual) == set(expected))

    def test_recommender(self):
        recommenders = [
            "factorization_recommender",
            "ranking_factorization_recommender",
            "popularity_recommender",
            "item_content_recommender",
            "item_similarity_recommender",
        ]
        other = ["util", "create"]
        lazy_module = ["_mod_par", "_", "_DeferredModuleLoader"]

        # Check visibility in turicreate.recommender
        actual = [x for x in dir(turicreate.recommender) if "__" not in x]
        expected = recommenders + other + lazy_module
        check_visible_modules(actual, expected)

        # For each module, check that there is a create() method.
        expected = ["create"]

        actual = get_visible_items(turicreate.recommender.factorization_recommender)
        assert set(actual) == set(expected + ["FactorizationRecommender"])

        actual = get_visible_items(
            turicreate.recommender.ranking_factorization_recommender
        )
        assert set(actual) == set(expected + ["RankingFactorizationRecommender"])

        actual = get_visible_items(turicreate.recommender.item_similarity_recommender)
        assert set(actual) == set(expected + ["ItemSimilarityRecommender"])

        actual = get_visible_items(turicreate.recommender.item_content_recommender)
        assert set(actual) == set(expected + ["ItemContentRecommender"])

        actual = get_visible_items(turicreate.recommender.popularity_recommender)
        assert set(actual) == set(expected + ["PopularityRecommender"])

        actual = get_visible_items(turicreate.recommender.util)
        expected = [
            "random_split_by_user",
            "precision_recall_by_user",
            "compare_models",
        ]

        check_visible_modules(actual, expected)

    def test_nearest_neighbors(self):
        actual = get_visible_items(turicreate.nearest_neighbors)
        expected = ["NearestNeighborsModel", "create"]
        check_visible_modules(actual, expected)

    def test_clustering(self):
        actual = get_visible_items(turicreate.clustering)
        expected = ["kmeans", "dbscan"]
        check_visible_modules(actual, expected)

    def test_topic_model(self):
        actual = get_visible_items(turicreate.topic_model)
        expected = ["topic_model", "create", "perplexity", "TopicModel"]
        check_visible_modules(actual, expected)

    def test_text_analytics(self):
        actual = get_visible_items(turicreate.text_analytics)
        expected = [
            "tf_idf",
            "bm25",
            "stop_words",
            "count_words",
            "count_ngrams",
            "random_split",
            "parse_sparse",
            "parse_docword",
            "tokenize",
            "drop_words",
        ]

        check_visible_modules(actual, expected)

    def test_classifier(self):
        actual = get_visible_items(turicreate.classifier)
        expected = [
            "create",
            "logistic_classifier",
            "nearest_neighbor_classifier",
            "random_forest_classifier",
            "decision_tree_classifier",
            "boosted_trees_classifier",
            "decision_tree",
            "svm_classifier",
            "nearest_neighbor_classifier",
        ]

        check_visible_modules(actual, expected)

    def test_regression(self):
        actual = get_visible_items(turicreate.regression)
        expected = [
            "create",
            "linear_regression",
            "boosted_trees_regression",
            "decision_tree_regression",
            "random_forest_regression",
        ]

        check_visible_modules(actual, expected)

    def test_toolkits(self):
        actual = get_visible_items(turicreate.toolkits)

        expected = [
            "clustering",
            "classifier",
            "graph_analytics",
            "recommender",
            "regression",
            "image_analysis",
            "distances",
            "nearest_neighbors",
            "topic_model",
            "text_analytics",
            "text_classifier",
            "image_classifier",
            "image_similarity",
            "object_detector",
            "one_shot_object_detector",
            "style_transfer",
            "activity_classifier",
            "drawing_classifier",
            "sound_classifier",
            "evaluation",
            "audio_analysis",
        ]

        check_visible_modules(actual, expected)

    def test_models_with_hyper_parameters(self):

        common_functions = ["create"]

        special_functions = {}
        special_functions[turicreate.linear_regression] = ["LinearRegression"]
        special_functions[turicreate.boosted_trees_regression] = [
            "BoostedTreesRegression"
        ]
        special_functions[turicreate.random_forest_regression] = [
            "RandomForestRegression"
        ]
        special_functions[turicreate.decision_tree_regression] = [
            "DecisionTreeRegression"
        ]
        special_functions[turicreate.logistic_classifier] = ["LogisticClassifier"]
        special_functions[turicreate.boosted_trees_classifier] = [
            "BoostedTreesClassifier"
        ]
        special_functions[turicreate.random_forest_classifier] = [
            "RandomForestClassifier"
        ]
        special_functions[turicreate.decision_tree_classifier] = [
            "DecisionTreeClassifier"
        ]

        special_functions[turicreate.recommender.factorization_recommender] = [
            "FactorizationRecommender"
        ]
        special_functions[turicreate.recommender.item_similarity_recommender] = [
            "ItemSimilarityRecommender"
        ]
        special_functions[turicreate.recommender.item_content_recommender] = [
            "ItemContentRecommender"
        ]
        special_functions[turicreate.recommender.ranking_factorization_recommender] = [
            "RankingFactorizationRecommender"
        ]
        special_functions[turicreate.recommender.popularity_recommender] = [
            "PopularityRecommender"
        ]

        for module, funcs in special_functions.items():
            actual = get_visible_items(module)
            expected = common_functions + funcs
            check_visible_modules(actual, expected)

    def test_topic_modelling_models(self):

        common_functions = ["create", "perplexity"]
        special_functions = {}
        special_functions[turicreate.topic_model] = ["topic_model", "TopicModel"]

        for module, funcs in special_functions.items():
            actual = get_visible_items(module)
            expected = common_functions + funcs
            check_visible_modules(actual, expected)

    def test_public_module(self):
        expected = [
            "activity_classifier",
            "aggregate",
            "audio_analysis",
            "boosted_trees_classifier",
            "boosted_trees_regression",
            "classifier",
            "clustering",
            "config",
            "connected_components",
            "data_structures",
            "dbscan",
            "decision_tree_classifier",
            "decision_tree_regression",
            "degree_counting",
            "distances",
            "drawing_classifier",
            "evaluation",
            "factorization_recommender",
            "graph_analytics",
            "graph_coloring",
            "image_analysis",
            "image_classifier",
            "image_similarity",
            "item_content_recommender",
            "item_similarity_recommender",
            "kcore",
            "kmeans",
            "label_propagation",
            "linear_regression",
            "logistic_classifier",
            "nearest_neighbor_classifier",
            "nearest_neighbors",
            "object_detector",
            "one_shot_object_detector",
            "pagerank",
            "popularity_recommender",
            "random_forest_classifier",
            "random_forest_regression",
            "ranking_factorization_recommender",
            "recommender",
            "regression",
            "shortest_path",
            "sound_classifier",
            "style_transfer",
            "svm_classifier",
            "text_analytics",
            "text_classifier",
            "toolkits",
            "topic_model",
            "test",
            "triangle_counting",
            "turicreate",
            "util",
            "version_info",
            "visualization",
        ]

        # When user first run `import turicreate`, `_gl_pickle` and `meta` are not available.
        # They are used by lambda workers. If you run sarray test, they will be imported.
        import turicreate.meta

        expected.append("meta")

        tc_modules = inspect.getmembers(turicreate, inspect.ismodule)
        tc_keys = [x[0] for x in tc_modules]
        tc_keys = filter(lambda x: not x.startswith("_"), tc_keys)

        check_visible_modules(tc_keys, expected)

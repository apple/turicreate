# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
import turicreate

MODULES = [
    "aws",
    "clustering",
    "data_structures",
    "graph_analytics",
    "data_matching",
    "recommender",
]
CLUSTER = ["kmeans", "dbscan"]
RECOMMENDERS = [
    "popularity_recommender",
    "item_similarity_recommender",
    "factorization_recommender",
    "ranking_factorization_recommender",
    "item_content_recommender",
]
ANOMALY_DETECTION = ["local_outlier_factor", "moving_zscore", "bayesian_changepoints"]
DATA_MATCHING = [
    "autotagger",
    "deduplication",
    "nearest_neighbor_deduplication",
    "nearest_neighbor_autotagger",
    "record_linker",
    "similarity_search",
]
DATA_STRUCTURES = ["SFrame", "SArray", "Graph", "Vertex", "Edge"]
GRAPH_ANALYTICS = [
    "connected_components",
    "graph_coloring",
    "kcore",
    "load_sgraph",
    "pagerank",
    "triangle_counting",
    "shortest_path",
]
GENERAL = ["load_model", "load_sframe", "load_sarray", "Model", "CustomModel"]


class TuriTests(unittest.TestCase):
    def test_top_level(self):
        for x in (
            MODULES
            + CLUSTER
            + DATA_STRUCTURES
            + GRAPH_ANALYTICS
            + GENERAL
            + RECOMMENDERS
            + ANOMALY_DETECTION
            + DATA_MATCHING
        ):
            self.assertTrue(x in dir(turicreate))

        # TODO Test whether or not things are NOT visible


def get_visible_items(d):
    return [x for x in dir(d) if not x.startswith("_")]


def check_visible_modules(actual, expected):
    assert set(actual) == set(expected), (
        "API Surface mis-matched. \
            Expected %s. Got %s"
        % (expected, actual)
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
            "get",
            "_list_fields",
            "_list_fields",
            "max_iterations",
            "method",
            "name",
            "num_clusters",
            "num_examples",
            "num_features",
            "num_unpacked_features",
            "save",
            "show",
            "summary",
            "training_iterations",
            "training_time",
            "unpacked_features",
            "verbose",
            "predict",
        ]

        actual = [x for x in dir(m) if not x.startswith("_")]
        check_visible_modules(actual, expected)

    def test_supervised(self):
        # Testing an instantiated class that inherits from Model
        sf = turicreate.SFrame({"d1": [1, 2, 3, 4, 5, 6], "d2": [5, 4, 3, 2, 1, 6]})
        m = turicreate.linear_regression.create(sf, target="d1")

        expected = [
            "coefficients",
            "convergence_threshold",
            "evaluate",
            "export_coreml",
            "feature_rescaling",
            "features",
            "get",
            "l1_penalty",
            "l2_penalty",
            "lbfgs_memory_level",
            "_list_fields",
            "_list_fields",
            "max_iterations",
            "name",
            "num_coefficients",
            "num_examples",
            "num_features",
            "num_unpacked_features",
            "predict",
            "progress",
            "save",
            "show",
            "solver",
            "step_size",
            "summary",
            "target",
            "training_iterations",
            "training_loss",
            "training_rmse",
            "training_solver_status",
            "training_time",
            "unpacked_features",
        ]

        actual = [x for x in dir(m) if not x.startswith("_")]
        check_visible_modules(actual, expected)

    def test_churn_predictor(self):
        # Arrange
        time = [1453845953 + 20000 * x for x in range(500)]
        user = [1, 2, 3, 4, 5] * 20 + [1, 2, 3, 4] * 25 + [1, 2, 3] * 100
        actions = turicreate.SFrame(
            {"user_id": user, "timestamp": time, "action": [1, 2, 3, 4, 5] * 100,}
        )

        def _unix_timestamp_to_datetime(x):
            import datetime

            return datetime.datetime.fromtimestamp(x)

        actions["timestamp"] = actions["timestamp"].apply(_unix_timestamp_to_datetime)
        actions = turicreate.TimeSeries(actions, "timestamp")

        # Act
        m = turicreate.churn_predictor.create(actions)
        actual = [x for x in dir(m) if not x.startswith("_")]

        # Assert.
        expected = [
            "categorical_features",
            "evaluate",
            "extract_features",
            "get_feature_importance",
            "churn_period",
            "grace_period",
            "features",
            "get",
            "is_data_aggregated",
            "_list_fields",
            "_list_fields",
            "lookback_periods",
            "model_options",
            "name",
            "num_features",
            "num_observations",
            "num_users",
            "numerical_features",
            "predict",
            "explain",
            "processed_training_data",
            "save",
            "show",
            "summary",
            "time_boundaries",
            "time_period",
            "trained_model",
            "trained_explanation_model",
            "get_churn_report",
            "get_activity_baseline",
            "views",
            "use_advanced_features",
            "user_id",
        ]
        check_visible_modules(actual, expected)

    def test_topic_model(self):
        sa = turicreate.SArray([{"a": 5, "b": 3}, {"a": 1, "b": 5, "c": 3}])
        m = turicreate.topic_model.create(sa)

        expected = [
            "alpha",
            "beta",
            "evaluate",
            "get",
            "get_topics",
            "_list_fields",
            "name",
            "num_burnin",
            "num_iterations",
            "num_topics",
            "predict",
            "print_interval",
            "save",
            "show",
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

    def test_local_outlier_factor(self):
        # Testing a class that inherits from CustomModel and ExposeAttributesFromProxy
        expected = [
            "distance",
            "features",
            "get",
            "_list_fields",
            "nearest_neighbors_model",
            "num_distance_components",
            "num_examples",
            "num_features",
            "num_neighbors",
            "num_unpacked_features",
            "predict",
            "row_label_name",
            "save",
            "scores",
            "show",
            "summary",
            "threshold_distances",
            "training_time",
            "unpacked_features",
            "verbose",
        ]
        sf = turicreate.SFrame(
            {
                "x0": [0.0, 1.0, 1.0, 0.0, 1.0, 0.0, 5.0],
                "x1": [2.0, 1.0, 0.0, 1.0, 2.0, 1.5, 2.5],
            }
        )
        m = turicreate.local_outlier_factor.create(sf, num_neighbors=3)

        actual = [x for x in dir(m) if not x.startswith("_")]
        check_visible_modules(actual, expected)

    def test_search(self):
        # Testing an instantiated class that inherits from SDKModel
        sf = turicreate.SFrame({"text": ["Hello my friend", "I love this burrito"]})
        m = turicreate._internal.search.create(sf)

        expected = [
            "average_document_length",
            "bm25_b",
            "bm25_k1",
            "data",
            "elapsed_indexing",
            "elapsed_processing",
            "features",
            "get",
            "_list_fields",
            "name",
            "num_documents",
            "num_tokens",
            "packed_sarrays",
            "query",
            "save",
            "show",
            "summary",
            "tfidf_threshold",
            "verbose",
            "vocabulary",
        ]

        actual = [x for x in dir(m) if not x.startswith("_")]
        check_visible_modules(actual, expected)


class ModuleVisibilityTests(unittest.TestCase):
    def test_Image_type(self):
        expected = ["Image", "show"]
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

        # Check visibility in turicreate.recommender
        actual = [x for x in dir(turicreate.recommender) if "__" not in x]
        expected = recommenders + other
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
            "RecommenderViews",
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

    def test_data_matching(self):
        actual = get_visible_items(turicreate.data_matching)
        expected = DATA_MATCHING
        check_visible_modules(actual, expected)

        actual = get_visible_items(turicreate.autotagger)
        expected = ["create"]
        check_visible_modules(actual, expected)

        actual = get_visible_items(turicreate.deduplication)
        expected = ["create"]
        check_visible_modules(actual, expected)

        actual = get_visible_items(turicreate.nearest_neighbor_deduplication)
        expected = ["NearestNeighborDeduplication", "create"]
        check_visible_modules(actual, expected)

    def test_anomaly_detection(self):
        actual = get_visible_items(turicreate.anomaly_detection)
        expected = [
            "local_outlier_factor",
            "moving_zscore",
            "create",
            "bayesian_changepoints",
        ]
        check_visible_modules(actual, expected)

        actual = get_visible_items(turicreate.toolkits.anomaly_detection)
        expected = [
            "local_outlier_factor",
            "moving_zscore",
            "create",
            "bayesian_changepoints",
        ]
        check_visible_modules(actual, expected)

        actual = get_visible_items(turicreate.anomaly_detection.local_outlier_factor)
        expected = ["LocalOutlierFactorModel", "create"]
        check_visible_modules(actual, expected)

        actual = get_visible_items(turicreate.anomaly_detection.moving_zscore)
        expected = ["MovingZScoreModel", "create"]
        check_visible_modules(actual, expected)

    def test_lead_scoring(self):
        expected = ["LeadScoringModel", "create"]

        actual = get_visible_items(turicreate.lead_scoring)
        check_visible_modules(actual, expected)

        actual = get_visible_items(turicreate.toolkits.lead_scoring)
        check_visible_modules(actual, expected)

    def test_topic_model(self):
        actual = get_visible_items(turicreate.topic_model)
        expected = ["TopicModel", "create", "perplexity"]
        check_visible_modules(actual, expected)

    def test_churn_predictor(self):
        actual = get_visible_items(turicreate.churn_predictor)
        expected = ["ChurnPredictor", "create", "random_split"]
        check_visible_modules(actual, expected)

    def test_text_analytics(self):
        actual = get_visible_items(turicreate.text_analytics)
        expected = [
            "tf_idf",
            "bm25",
            "stopwords",
            "count_words",
            "count_ngrams",
            "random_split",
            "parse_sparse",
            "parse_docword",
            "tokenize",
            "trim_rare_words",
            "split_by_sentence",
            "extract_parts_of_speech",
            "PartOfSpeech",
        ]
        check_visible_modules(actual, expected)

    def test_classifier(self):
        actual = get_visible_items(turicreate.classifier)
        expected = [
            "create",
            "logistic_classifier",
            "boosted_trees_classifier",
            "random_forest_classifier",
            "decision_tree_classifier",
            "neuralnet_classifier",
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

    def test_cross_validation(self):
        actual = get_visible_items(turicreate.cross_validation)
        expected = ["cross_val_score", "shuffle", "KFold"]
        check_visible_modules(actual, expected)

    def test_toolkits(self):
        actual = get_visible_items(turicreate.toolkits)
        expected = [
            "anomaly_detection",
            "churn_predictor",
            "classifier",
            "clustering",
            "comparison",
            "cross_validation",
            "data_matching",
            "deeplearning",
            "distances",
            "evaluation",
            "feature_engineering",
            "graph_analytics",
            "image_analysis",
            "lead_scoring",
            "model_parameter_search",
            "nearest_neighbors",
            "pattern_mining",
            "product_sentiment",
            "recommender",
            "regression",
            "sentiment_analysis",
            "text_analytics",
            "topic_model",
        ]
        check_visible_modules(actual, expected)

    def test_graph_analytics(self):

        common_functions = ["create"]

        special_functions = {}
        special_functions[turicreate.toolkits.graph_analytics.connected_components] = [
            "ConnectedComponentsModel"
        ]
        special_functions[turicreate.toolkits.graph_analytics.graph_coloring] = [
            "GraphColoringModel"
        ]
        special_functions[turicreate.toolkits.graph_analytics.kcore] = ["KcoreModel"]
        special_functions[turicreate.toolkits.graph_analytics.shortest_path] = [
            "ShortestPathModel"
        ]
        special_functions[turicreate.toolkits.graph_analytics.triangle_counting] = [
            "TriangleCountingModel"
        ]
        special_functions[turicreate.toolkits.graph_analytics.pagerank] = [
            "PagerankModel"
        ]

        for module, funcs in special_functions.items():
            actual = get_visible_items(module)
            expected = common_functions + funcs
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
        special_functions[turicreate.topic_model] = ["TopicModel"]

        for module, funcs in special_functions.items():
            actual = get_visible_items(module)
            expected = common_functions + funcs
            check_visible_modules(actual, expected)

    def test_feature_engineering(self):
        actual = get_visible_items(turicreate.toolkits.feature_engineering)
        expected = [
            "AutoVectorizer",
            "BM25",
            "CategoricalImputer",
            "CountThresholder",
            "CountFeaturizer",
            "DeepFeatureExtractor",
            "FeatureBinner",
            "FeatureHasher",
            "NGramCounter",
            "NumericImputer",
            "OneHotEncoder",
            "QuadraticFeatures",
            "RandomProjection",
            "TFIDF",
            "Tokenizer",
            "TransformerBase",
            "TransformerChain",
            "TransformToFlatDictionary",
            "WordCounter",
            "RareWordTrimmer",
            "SentenceSplitter",
            "PartOfSpeechExtractor",
            "create",
        ]

        check_visible_modules(actual, expected)

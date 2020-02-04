# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from turicreate._deps import numpy as np, HAS_NUMPY
from turicreate._deps import pandas as pd, HAS_PANDAS
import unittest
from . import util
import os
import sys
import six
import turicreate as tc
import random
import shutil
import tempfile
from os.path import join
from turicreate.data_structures.sframe import SFrame
from turicreate.data_structures.sframe import SArray
from turicreate.toolkits._main import ToolkitError
from turicreate.toolkits.recommender.util import random_split_by_user
import itertools
from turicreate.toolkits.recommender.item_similarity_recommender import (
    ItemSimilarityRecommender,
)
from turicreate.toolkits.recommender.factorization_recommender import (
    FactorizationRecommender,
)
from turicreate.toolkits.recommender.ranking_factorization_recommender import (
    RankingFactorizationRecommender,
)
from turicreate.toolkits.recommender.popularity_recommender import PopularityRecommender
import array
from turicreate.util import _assert_sframe_equal as assert_sframe_equal
from turicreate.toolkits._internal_utils import _mac_ver
from tempfile import mkstemp as _mkstemp
from copy import copy
from subprocess import Popen as _Popen
from subprocess import PIPE as _PIPE
import coremltools as _coremltools


if not HAS_NUMPY:
    raise ImportError("Tests need Numpy")

if not HAS_PANDAS:
    raise ImportError("Tests need Pandas")


DELTA = 0.000001

model_names = [
    "popularity_recommender",
    "popularity_recommender_with_target",
    "item_content_recommender",
    "item_similarity_recommender",
    "item_similarity_recommender_cosine",
    "item_similarity_recommender_pearson",
    "factorization_recommender",
    "factorization_recommender_als",
    "factorization_recommender_binary",
    "factorization_recommender_nmf",
    "ranking_factorization_recommender",
    "ranking_factorization_recommender_ials",
    "ranking_factorization_recommender_no_target",
]


def _coreml_to_tc(preds):
    return {"rank": preds["recommendations"], "score": preds["probabilities"]}


class RecommenderTestBase(unittest.TestCase):
    def _test_coreml_export(self, m, item_ids, ratings=None):
        temp_file_path = _mkstemp()[1]
        if m.target and ratings:
            obs_data_sf = tc.SFrame(
                {m.item_id: tc.SArray(item_ids), m.target: tc.SArray(ratings)}
            )
            predictions_tc = m.recommend_from_interactions(obs_data_sf, k=5)
            interactions = {item_ids[i]: ratings[i] for i in range(len(item_ids))}
        else:
            predictions_tc = m.recommend_from_interactions(item_ids, k=5)
            interactions = {item_ids[i]: -1 for i in range(len(item_ids))}

        # convert TC SFrame into same dict structure as CoreML will return
        predictions_tc_dict = dict()
        for field in [u"score", u"rank"]:
            predictions_tc_dict[field] = dict()
        item_ids_from_preds = predictions_tc[m.item_id]
        scores_from_preds = predictions_tc["score"]
        ranks_from_preds = predictions_tc["rank"]
        for i in range(len(item_ids_from_preds)):
            predictions_tc_dict["score"][item_ids_from_preds[i]] = scores_from_preds[i]
            predictions_tc_dict["rank"][item_ids_from_preds[i]] = ranks_from_preds[i]

        # Do the CoreML export and predict (if on macOS)
        m.export_coreml(temp_file_path)
        if _mac_ver() >= (10, 14):
            coremlmodel = _coremltools.models.MLModel(temp_file_path)
            predictions_coreml = coremlmodel.predict(
                {"interactions": interactions, "k": 5}
            )

            # compare them
            self.assertEqual(predictions_tc_dict, _coreml_to_tc(predictions_coreml))

            # compare user defined data
            import platform

            self.assertDictEqual(
                {
                    "com.github.apple.turicreate.version": tc.__version__,
                    "com.github.apple.os.platform": platform.platform(),
                },
                dict(coremlmodel.user_defined_metadata),
            )
        os.unlink(temp_file_path)

    def _get_trained_model(
        self,
        model_name,
        data,
        user_id="user",
        item_id="item",
        target=None,
        test_export_to_coreml=True,
        **args
    ):

        if model_name == "default":
            m = tc.recommender.create(data, user_id, item_id, target=target, **args)
        elif model_name == "popularity_recommender":
            m = tc.popularity_recommender.create(data, user_id, item_id, **args)

        elif model_name == "popularity_recommender_with_target":
            m = tc.popularity_recommender.create(
                data, user_id, item_id, target=target, **args
            )

        elif model_name == "ranking_factorization_recommender":

            args.setdefault("max_iterations", 10)
            m = tc.ranking_factorization_recommender.create(
                data, user_id=user_id, item_id=item_id, target=target, **args
            )

        elif model_name == "item_content_recommender":

            items = data[item_id].unique()

            alt_data = tc.util.generate_random_sframe(len(items), "ccnv")
            alt_data[item_id] = items

            m = tc.item_content_recommender.create(
                alt_data,
                item_id=item_id,
                observation_data=data,
                user_id=user_id,
                target=target,
                **args
            )

        elif model_name == "ranking_factorization_recommender_ials":

            args.setdefault("max_iterations", 10)
            m = tc.ranking_factorization_recommender.create(
                data,
                user_id=user_id,
                item_id=item_id,
                target=target,
                solver="ials",
                **args
            )

        elif model_name == "ranking_factorization_recommender_no_target":

            args.setdefault("max_iterations", 5)
            m = tc.ranking_factorization_recommender.create(
                data[[user_id, item_id]], user_id=user_id, item_id=item_id, **args
            )

        elif model_name == "factorization_recommender":

            args.setdefault("max_iterations", 10)
            m = tc.factorization_recommender.create(
                data, user_id=user_id, item_id=item_id, target=target, **args
            )

        elif model_name == "factorization_recommender_nmf":

            args.setdefault("max_iterations", 10)
            m = tc.recommender.factorization_recommender.create(
                data,
                user_id=user_id,
                item_id=item_id,
                target=target,
                nmf=True,
                side_data_factorization=False,
                **args
            )

        elif model_name == "factorization_recommender_als":

            args.setdefault("max_iterations", 10)
            m = tc.recommender.factorization_recommender.create(
                data,
                user_id=user_id,
                item_id=item_id,
                target=target,
                solver="als",
                **args
            )

        elif model_name == "factorization_recommender_binary":

            args.setdefault("max_iterations", 10)
            if target in data.column_names():
                data[target] = data[target] > 0.5  # Make it binary
            m = tc.recommender.ranking_factorization_recommender.create(
                data, user_id=user_id, item_id=item_id, target=target, **args
            )

        elif model_name == "item_similarity_recommender":
            m = tc.recommender.item_similarity_recommender.create(
                data, user_id, item_id, target=target, **args
            )

        elif model_name == "item_similarity_recommender_cosine":
            m = tc.recommender.item_similarity_recommender.create(
                data, user_id, item_id, target=target, similarity_type="cosine", **args
            )

        elif model_name == "item_similarity_recommender_pearson":
            m = tc.recommender.item_similarity_recommender.create(
                data, user_id, item_id, target=target, similarity_type="pearson", **args
            )

        elif model_name == "itemcf-user-distance":
            m = tc.recommender.item_similarity_recommender.create(
                data, user_id, item_id, target=target, similarity_type="pearson", **args
            )

            nearest_items = m.get_similar_items()
            m = tc.recommender.item_similarity_recommender.create(
                data,
                user_id,
                item_id,
                target=target,
                similarity_type="cosine",
                nearest_items=nearest_items,
                **args
            )

        elif model_name == "itemcf-jaccard-topk":
            m = tc.recommender.item_similarity_recommender.create(
                data, user_id, item_id, threshold=0.001, only_top_k=100, **args
            )

        else:
            raise NotImplementedError("Unknown model %s requested" % model_name)

        from itertools import chain, permutations

        all_items = data[item_id].unique()
        for some_items in chain(
            *list(permutations(all_items, i) for i in range(min(3, len(all_items))))
        ):

            some_items = list(some_items)

            if target:
                ratings = [random.uniform(0, 1) for i in some_items]
            else:
                ratings = None

            if test_export_to_coreml:
                self._test_coreml_export(m, some_items, ratings)

        return m


class UserDefinedSimilarityTest(RecommenderTestBase):
    def setUp(self):
        sf = tc.SFrame(
            {
                "user_id": ["0", "0", "0", "1", "1", "2", "2", "2"],
                "item_id": ["a", "b", "c", "a", "b", "b", "c", "d"],
                "rating": [0.2, 0.3, 0.4, 0.1, 0.3, 0.3, 0.5, 0.9],
            }
        )
        nearest_items = tc.SFrame(
            {
                "item_id": ["a", "a", "b", "b", "b", "e"],
                "similar": ["b", "c", "a", "c", "e", "f"],
                "score": [0.2, 0.3, 0.4, 0.1, 0.5, 0.8],
            }
        )
        self.sf = sf
        self.nearest_items = nearest_items

    def test_default(self):
        m = tc.recommender.item_similarity_recommender.create(
            self.sf, "user_id", "item_id", nearest_items=self.nearest_items
        )

        y = self.nearest_items.sort(["item_id", "score"])
        z = m.get_similar_items().sort(["item_id", "score"])

        assert all(y["item_id"] == z["item_id"])
        assert all(y["similar"] == z["similar"])

        for vy, vz in zip(y["score"], z["score"]):
            self.assertAlmostEqual(vy, vz, 5)

        """
        TODO: test CoreML export, when we can support serializing user
              data into CoreML model format.

        if m.target:
            self._test_coreml_export(m, ['a','b'], [.2,.3])
        else:
            self._test_coreml_export(m, ['a','b'])
        """

    def tmp_test_bad_input(self):
        x = self.nearest_items
        x.rename({"score": "rating"}, inplace=True)

        self.assertRaises(
            ToolkitError,
            lambda a: tc.item_similarity_recommender.create(
                self.sf, "user_id", "item_id", nearest_items=x
            ),
            "Could not initialize using nearest_items argument.",
        )


class ImmutableTest(RecommenderTestBase):
    def setUp(self):
        df_dict = {
            "user_id": ["0", "0", "0", "1", "1", "2", "2", "2"],
            "item_id": ["a", "b", "c", "a", "b", "b", "c", "d"],
            "rating": [0.2, 0.3, 0.4, 0.1, 0.3, 0.3, 0.5, 0.9],
        }
        df = SFrame(df_dict)
        self.df = df
        self.df_dict = df_dict
        self.train = self.df.head(4)
        self.test = self.df.tail(4)

    def test_immutable(self):
        m = tc.recommender.factorization_recommender.create(
            self.df, target="rating", num_factors=2
        )
        assert (
            type(m) == tc.recommender.factorization_recommender.FactorizationRecommender
        )

        yhat = m.predict(self.df)
        N, P = self.df.shape
        assert len(yhat) == N

        """
        TODO: test CoreML export, when we can support serializing
              factorization model coefficients into CoreML model format.

        if m.target:
            self._test_coreml_export(m, ['a','b'], [.2,.3])
        else:
            self._test_coreml_export(m, ['a','b'])
        """


class EdgeCasesTest(RecommenderTestBase):
    def setUp(self):

        df = SFrame(
            {
                "user_id": ["0", "0", "0", "1", "1", "2", "2"],
                "item_id": ["a", "b", "c", "a", "b", "b", "c"],
                "rating": [0.2, 0.3, 0.4, 0.1, 0.3, 0.5, 0.9],
            }
        )
        self.df = df
        m1 = tc.recommender.popularity_recommender.create(self.df)
        m2 = tc.recommender.item_similarity_recommender.create(self.df)
        m3 = tc.recommender.factorization_recommender.create(self.df, target="rating")
        self.trained_models = [m1, m2, m3]

    def test_recommend_empty(self):

        for m in self.trained_models:

            # Ensure that no recommendations are given to user 0, who has
            # observed all unique items.
            recs = m.recommend(k=3)

            assert "0" not in set(list(recs["user_id"]))

            # Ensure we do not return k recommendations if fewer are available.
            recs = m.recommend(k=10)
            assert "0" not in set(list(recs["user_id"]))
            assert recs.num_rows() == 2

            # TODO: test CoreML export, when we can support serializing
            # factorization or popularity models
            if isinstance(
                m,
                (
                    tc.recommender.factorization_recommender.FactorizationRecommender,
                    tc.recommender.popularity_recommender.PopularityRecommender,
                ),
            ):
                continue

            # TODO - why is item similarity failing here !?
            if m.target:
                self._test_coreml_export(m, ["a", "b"], [0.2, 0.3])
            else:
                self._test_coreml_export(m, ["a", "b"])

    def test_no_target(self):
        df = SFrame(
            {
                "user_id": ["0", "0", "0", "1", "1", "2", "2"],
                "item_id": ["0", "0", "0", "1", "1", "2", "2"],
            }
        )
        model_names_with_targets = [
            "popularity_recommender_with_target",
            "item_similarity_recommender_cosine",
            "item_similarity_recommender_pearson",
            "factorization_recommender",
            "factorization_recommender_binary",
            "factorization_recommender_nmf",
            "ranking_factorization_recommender",
        ]

        for m in model_names_with_targets:
            print(m)

            self.assertRaises(
                Exception,
                lambda: self._get_trained_model(
                    m,
                    df,
                    user_id="user_id",
                    item_id="item_id",
                    target="rating-that-isnt-really-there-just-like-your-happiness",
                ),
            )

    def test_bad_columns(self):
        df = SFrame(
            {
                "user_id": ["0", "0", "0", "1", "1", "2", "2"],
                "rating": [0.2, 0.3, 0.4, 0.1, 0.3, 0.3, 0.9],
            }
        )

        for m in model_names:
            self.assertRaises(
                Exception,
                lambda: self._get_trained_model(
                    m, df, user_id="user_id", item_id="user_id", target="rating"
                ),
            )
            self.assertRaises(
                Exception,
                lambda: self._get_trained_model(
                    m, df, user_id="user_id", item_id="item_id", target="item_id"
                ),
            )

            self.assertRaises(
                Exception,
                lambda: self._get_trained_model(
                    m, df, user_id="user_id", item_id="user_id", target="user_id"
                ),
            )


class ReturnStatisticsTest(RecommenderTestBase):
    def setUp(self):

        self.df = SFrame(
            {
                "user_id": ["0", "0", "0", "1", "1", "2", "2"],
                "item_id": ["a", "b", "c", "a", "b", "b", "c"],
                "rating": [0.2, 0.3, 0.4, 0.1, 0.3, 0.5, 0.9],
            }
        )

    def test_train_rmse_returns(self):
        models_with_targets = [
            "factorization_recommender",
            "ranking_factorization_recommender",
            "item_similarity_recommender_pearson",
            "popularity_recommender_with_target",
        ]
        for name in models_with_targets:
            """
            TODO:
            Test CoreML export, when we have a dirarchiver that doesn't
            depend on the filesystem
            """
            m = self._get_trained_model(
                name,
                self.df,
                user_id="user_id",
                item_id="item_id",
                target="rating",
                test_export_to_coreml=False,
            )

            assert m.training_rmse is None or m.training_rmse >= 0

    def test_get_counts(self):
        m = self._get_trained_model(
            "item_similarity_recommender_pearson",
            self.df,
            user_id="user_id",
            item_id="item_id",
            target="rating",
        )

        item_counts = m.get_num_users_per_item()
        expected = tc.SFrame()
        expected["item_id"] = ["a", "b", "c"]
        expected["num_users"] = [2, 3, 2]
        assert_sframe_equal(item_counts, expected)

        user_counts = m.get_num_items_per_user()
        expected = tc.SFrame()
        expected["user_id"] = ["0", "1", "2"]
        expected["num_items"] = [3, 2, 2]
        assert_sframe_equal(user_counts, expected)


class NewUserTest(RecommenderTestBase):
    def setUp(self):

        n_total = 200
        df_size = 200

        users = ["U%d" % i for i in range(n_total)]
        items = ["I%d" % i for i in range(n_total)]
        ratings = [float(i) / n_total for i in range(n_total)]

        random.seed(0)

        def sample_set(L):
            return [random.choice(L) for i in range(df_size)]

        self.df1 = SFrame(
            {
                "user_id": sample_set(users),
                "item_id": sample_set(items),
                "rating": sample_set(ratings),
            }
        )

        self.df2 = SFrame(
            {
                "user_id": sample_set(users),
                "item_id": sample_set(items),
                "rating": sample_set(ratings),
            }
        )

        self.df3 = SFrame(
            {
                "user_id": sample_set(users),
                "item_id": sample_set(items),
                "rating": sample_set(ratings),
            }
        )

    def test_score(self):

        for model_name in model_names:
            print("New Users: Predict:", model_name)
            """
            TODO:
            Test CoreML export, when we have a dirarchiver that doesn't
            depend on the filesystem
            """
            m = self._get_trained_model(
                model_name,
                self.df1,
                user_id="user_id",
                item_id="item_id",
                target="rating",
                test_export_to_coreml=False,
            )

            for data in [self.df1, self.df2, self.df3]:

                preds = m.predict(data)
                assert type(preds) == SArray

    def test_recommend(self):

        for model_name in model_names:
            print("New Users: Recommend:", model_name)
            """
            TODO:
            Test CoreML export, when we have a dirarchiver that doesn't
            depend on the filesystem
            """
            m = self._get_trained_model(
                model_name,
                self.df1,
                user_id="user_id",
                item_id="item_id",
                target="rating",
                test_export_to_coreml=False,
            )

            for data in [self.df2, self.df3]:

                recs = m.recommend(new_observation_data=data)
                assert type(recs) == SFrame


class GetSimilarItemsTest(RecommenderTestBase):
    def setUp(self):
        item_column = "my_item_column"
        user_column = "my_user_id"
        sf = SFrame(
            {
                user_column: ["0", "0", "0", "1", "1", "2", "2", "3", "3"],
                item_column: ["a", "b", "c", "b", "c", "c", "d", "a", "d"],
                "rating": [1.0, 0.3, 0.5, 0.5, 0.6, 1.0, 0.1, 0.1, 1.5],
            }
        )

        models = []
        for mod in [
            tc.recommender.item_similarity_recommender,
            tc.recommender.factorization_recommender,
            tc.recommender.popularity_recommender,
        ]:
            m = mod.create(sf, user_column, item_column, target="rating")
            """
            TODO:
            Test CoreML export, when we have a dirarchiver that doesn't
            depend on the filesystem
            """
            if isinstance(
                m, tc.recommender.item_similarity_recommender.ItemSimilarityRecommender
            ):
                self._test_coreml_export(m, ["a", "b"], [1.0, 0.3])
            models.append(m)

        self.sf = sf
        self.models = models
        self.item_column = item_column
        self.user_column = user_column

    def run_get_similar_items(self, m):

        sf = m.get_similar_items(k=2)
        sf1 = m.get_similar_items(items=[], k=2)
        sf2 = m.get_similar_items(items=SArray(), k=2)

        self.assertEqual(sf.num_rows(), m.num_items * 2)
        self.assertEqual(sf1.num_rows(), 0)
        self.assertEqual(sf2.num_rows(), 0)

        sf3 = m.get_similar_items(items=["a", "b"], k=2)
        sf4 = m.get_similar_items(items=["a", "e"], k=2)
        sf5 = m.get_similar_items(items=["e", "f"], k=2)
        sf6 = m.get_similar_items(items=["e", "f"], k=2, verbose=False)

        d = list(sf3)

        self.assertEqual(d[0]["rank"], 1)
        self.assertEqual(d[1]["rank"], 2)
        self.assertEqual(d[2]["rank"], 1)
        self.assertEqual(d[3]["rank"], 2)

        self.assertGreaterEqual(d[0]["score"], d[1]["score"])
        self.assertGreaterEqual(d[2]["score"], d[3]["score"])

        # similarity between "b" and "d" is 0.
        # So for "b", only two items are returned for item_similarity model
        self.assertEqual(sf3.num_rows(), 2 * 2)
        self.assertEqual(sf4.num_rows(), 2)
        self.assertEqual(sf5.num_rows(), 0)
        self.assertEqual(sf6.num_rows(), 0)

        for s in [sf, sf1, sf2, sf3, sf4, sf5, sf6]:
            self.assertEqual(s.column_names()[0], self.item_column)
            self.assertEqual(s.column_names()[1], "similar")
            self.assertEqual(s.column_names()[2], "score")  # TEMP
            self.assertEqual(s.column_names()[3], "rank")

    def run_get_similar_users(self, m):
        try:
            sf = m.get_similar_users(k=2)
        except ToolkitError:
            return

        sf1 = m.get_similar_users(users=[], k=2)
        sf2 = m.get_similar_users(users=SArray(), k=2)

        self.assertEqual(sf.num_rows(), m.num_users * 2)
        self.assertEqual(sf1.num_rows(), 0)
        self.assertEqual(sf2.num_rows(), 0)

        sf3 = m.get_similar_users(users=["0", "1"], k=2)
        sf4 = m.get_similar_users(users=["0", "4"], k=2)
        sf5 = m.get_similar_users(users=["4", "5"], k=2)

        d = list(sf3)

        self.assertEqual(d[0]["rank"], 1)
        self.assertEqual(d[1]["rank"], 2)
        self.assertEqual(d[2]["rank"], 1)
        self.assertEqual(d[3]["rank"], 2)

        self.assertGreaterEqual(d[0]["score"], d[1]["score"])
        self.assertGreaterEqual(d[2]["score"], d[3]["score"])

        # similarity between "b" and "d" is 0.
        # So for "b", only two items are returned for item_similarity model
        self.assertEqual(sf3.num_rows(), 2 * 2)
        self.assertEqual(sf4.num_rows(), 2)
        self.assertEqual(sf5.num_rows(), 0)

        for s in [sf, sf1, sf2, sf3, sf4, sf5]:
            self.assertEqual(s.column_names()[0], self.user_column)
            self.assertEqual(s.column_names()[1], "similar")
            self.assertEqual(s.column_names()[2], "score")
            self.assertEqual(s.column_names()[3], "rank")

    def test_similar_items_correctness(self):

        # Generate random data with numerous unique users and items,
        # so everything is essentially unique.  Test the get similar
        # items by duplicating an item and test that it's the most
        # similar.
        data = tc.util.generate_random_regression_sframe(
            500, "ZZ", random_seed=0
        ).rename({"X1-Z": "user", "X2-Z": "item"}, inplace=True)

        item = data["item"][0]

        new_data = data.filter_by([item], "item")
        new_data["item"] = 1000

        data = data.append(new_data)

        def test_model(m):

            ret_sf1 = m.get_similar_items([item], k=1)
            self.assertEqual(ret_sf1[0]["item"], item)
            self.assertEqual(ret_sf1[0]["similar"], 1000)

            ret_sf2 = m.get_similar_items([1000], k=1)
            self.assertEqual(ret_sf2[0]["item"], 1000)
            self.assertEqual(ret_sf2[0]["similar"], item)

        test_model(
            tc.recommender.item_similarity_recommender.create(
                data, "user", "item", "target"
            )
        )
        test_model(
            tc.recommender.popularity_recommender.create(data, "user", "item", "target")
        )

        test_model(
            tc.recommender.factorization_recommender.create(
                data,
                "user",
                "item",
                "target",
                num_factors=8,
                regularization=0,
                solver="als",
                max_iterations=50,
            )
        )

        test_model(
            tc.recommender.ranking_factorization_recommender.create(
                data[["user", "item"]],
                "user",
                "item",
                regularization=0,
                num_factors=8,
                solver="ials",
                max_iterations=50,
            )
        )

    def test_similar_users_correctness(self):

        # Generate random data with numerous unique users and items,
        # so everything is essentially unique.  Test the get similar
        # items by duplicating an item and test that it's the most
        # similar.
        data = tc.util.generate_random_regression_sframe(
            200, "zZ", random_seed=0
        ).rename({"X1-z": "user", "X2-Z": "item"}, inplace=True)

        user = data["user"][0]
        item = data["item"][0]

        new_data = data.filter_by([user], "user")
        new_data["user"] = 10000

        data = data.append(new_data)

        for mod in [
            tc.recommender.factorization_recommender,
            tc.recommender.popularity_recommender,
        ]:
            m = mod.create(data, "user", "item", "target")

        def test_model(m):
            ret_sf1 = m.get_similar_users([user], k=1)
            self.assertEqual(ret_sf1[0]["user"], user)
            self.assertEqual(ret_sf1[0]["similar"], 10000)

            ret_sf2 = m.get_similar_users([10000], k=1)
            self.assertEqual(ret_sf2[0]["user"], 10000)
            self.assertEqual(ret_sf2[0]["similar"], user)
            """
            TODO:
            Test CoreML export, when we have a dirarchiver that doesn't
            depend on the filesystem
            self._test_coreml_export(m, [item])
            """

        test_model(
            tc.recommender.factorization_recommender.create(
                data,
                "user",
                "item",
                "target",
                num_factors=8,
                regularization=0,
                solver="als",
                max_iterations=50,
            )
        )

        test_model(
            tc.recommender.ranking_factorization_recommender.create(
                data[["user", "item"]],
                "user",
                "item",
                regularization=0,
                num_factors=8,
                solver="ials",
                max_iterations=50,
            )
        )

    def test_get_similar_items(self):
        for m in self.models:
            self.run_get_similar_items(m)

    def test_get_similar_users(self):
        for m in self.models[1:]:
            self.run_get_similar_users(m)


class ItemSimTest(RecommenderTestBase):
    def setUp(self):
        df = SFrame(
            {
                "user_id": ["0", "0", "0", "1", "1", "2", "2", "3", "3"],
                "item_id": ["a", "b", "c", "b", "c", "c", "d", "a", "d"],
                "rating": [1.0, 0.3, 0.5, 0.5, 0.6, 1.0, 0.1, 0.1, 1.5],
            }
        )
        self.df = df

    def test_save_load(self):

        m = tc.recommender.item_similarity_recommender.create(
            self.df,
            user_id="user_id",
            item_id="item_id",
            target="rating",
            similarity_type="cosine",
        )

        try:
            write_dir = tempfile.mkdtemp()

            fn = join(write_dir, "tmp.gl")
            m.save(fn)
            m1 = tc.load_model(fn)

            rec = m.recommend()
            rec1 = m1.recommend()
            assert (rec.head(100)["score"] - rec1.head(100)["score"]).sum() < DELTA

        finally:
            shutil.rmtree(write_dir)

        self._test_coreml_export(m, ["a", "b"], [1.0, 0.3])


class PopularityRecommenderTest(RecommenderTestBase):
    def setUp(self):
        self.df = SFrame(
            {
                "user_id": ["0", "0", "0", "1", "1", "2", "2", "2"],
                "item_id": ["a", "b", "c", "a", "b", "b", "c", "d"],
                "rating": [0.2, 0.3, 0.4, 0.1, 0.3, 0.3, 0.5, 0.9],
            }
        )
        self.train = self.df.head(4)
        self.test = self.df.tail(4)
        self.sframe_comparer = util.SFrameComparer()

    def test_saved_predictions(self):
        m = tc.popularity_recommender.create(self.train, target="rating")
        preds = m.item_predictions
        assert preds is not None
        assert preds.num_rows() == len(self.train["item_id"].unique())

        nn = m.get_similar_items()
        assert nn is not None
        """
        TODO:
        Test CoreML export, when we have a dirarchiver that doesn't
        depend on the filesystem
        self._test_coreml_export(m, ['a','b'], [.2,.3])
        """

    def test_popularity_model(self):
        # Test that popularity without targets just counts each item's
        # occurrence.
        m = tc.popularity_recommender.create(self.df)
        actual = m.predict(self.df)
        expected = tc.SArray([2, 3, 2, 2, 3, 3, 2, 1]).astype(float)
        self.sframe_comparer._assert_sarray_equal(actual, expected)
        """
        TODO:
        Test CoreML export, when we have a dirarchiver that doesn't
        depend on the filesystem
        self._test_coreml_export(m, ['a','b'])
        """

        # Test a popularity model that uses the target column.
        m = tc.popularity_recommender.create(self.train, target="rating")
        yhat = m.predict(self.df)
        N, P = self.df.shape
        assert len(yhat) == N
        """
        TODO:
        Test CoreML export, when we have a dirarchiver that doesn't
        depend on the filesystem
        self._test_coreml_export(m, ['a','b'], [.2,.3])
        """

        # Check the dimensions of predictions on the training set
        yhat = m.predict(self.train)
        N, P = self.train.shape
        assert len(yhat) == N

        # Check the mean rating of the first item in the training set
        first_item = self.train["item_id"][0]
        mean_rating = self.train["rating"][self.train["item_id"] == first_item].mean()
        pred = yhat.head(1)[0]
        assert abs(pred - mean_rating) < DELTA

        # Check the dimensions of predictions on the test set
        yhat = m.predict(self.test)
        N, P = self.test.shape
        assert len(yhat) == N

        # Check the mean rating of the first item in the test set.
        # Should be the mean observed rating as seen in the training set.
        first_item = self.test["item_id"][0]
        mean_rating = self.train["rating"][self.train["item_id"] == first_item].mean()
        pred = yhat.head(1)[0]
        assert abs(pred - mean_rating) < DELTA

        chosen = "d"  # pick an item that was in the test set and not in train
        mean_rating = self.train["rating"].mean()
        yhat = m.predict(self.df)
        ix = SArray(self.df["item_id"] == chosen)
        new_item_preds = yhat[ix][0]
        assert abs(new_item_preds - mean_rating) < DELTA

    def test_largescale_recommendations(self):

        user_item_list = []

        for i in range(500):
            for j in range(i):
                user_item_list.append((i, j))

        random.shuffle(user_item_list)

        sf = tc.SFrame(
            {
                "user_id": [u for u, i in user_item_list],
                "item_id": [i for u, i in user_item_list],
            }
        )

        m = tc.popularity_recommender.create(sf)
        """
        TODO:
        Test CoreML export, when we have a dirarchiver that doesn't
        depend on the filesystem
        self._test_coreml_export(m, [1,2])
        """

        res = m.recommend(users=list(range(500)), k=1)

        # Run some tests that make sure these are recommended correctly
        for d in res:
            user = d["user_id"]
            item = d["item_id"]
            score = d["score"]
            rank = d["rank"]

            assert rank == 1
            assert item == user, "user = %s, item = %s" % (user, item)
            assert score == 499 - user  # how many times it's been seen

        # Now run tests with top_k = 2
        res = m.recommend(users=list(range(500)), k=2)

        # Run some tests that make sure these are recommended correctly
        for d in res[::2]:
            user = d["user_id"]
            item = d["item_id"]
            score = d["score"]
            rank = d["rank"]

            assert rank == 1
            assert item == user  # The most popular unseen item
            assert score == 499 - user  # how many times it's been seen

        for d in res[1::2]:
            user = d["user_id"]
            item = d["item_id"]
            score = d["score"]
            rank = d["rank"]

            assert rank == 2
            assert item == user + 1  # The most popular unseen item
            assert score == 498 - user  # how many times it's been seen

    def test_compare_against_baseline(self):

        df_1 = tc.SFrame(
            {
                "user_id": [1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4],
                "item_id": [1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4],
                "rating": [2, 3, 4, 5, 2, 3.5, 3.5, 5, 5, 3, 2.5, 1.5, 5, 4, 3, 0.5],
            }
        )

        df_2 = df_1[["user_id", "item_id"]]

        ############################################################

        base_model = tc.recommender.popularity_recommender.create(
            df_1, "user_id", "item_id", "rating"
        )

        for mod in [
            tc.recommender.popularity_recommender,
            tc.recommender.factorization_recommender,
            tc.recommender.item_similarity_recommender,
        ]:

            m = mod.create(df_1, "user_id", "item_id", "rating")

            pop_model = m._get_popularity_baseline()

            self.sframe_comparer._assert_sframe_equal(
                pop_model.recommend(), base_model.recommend()
            )

            """
            TODO:
            Test CoreML export, when we have a dirarchiver that doesn't
            depend on the filesystem
            if m.target:
                self._test_coreml_export(m, [1,2], [2,3])
            else:
                self._test_coreml_export(m, [1,2])
            """

        ############################################################

        base_model = tc.recommender.popularity_recommender.create(
            df_2, "user_id", "item_id"
        )

        for mod in [
            tc.recommender.popularity_recommender,
            tc.recommender.ranking_factorization_recommender,
            tc.recommender.item_similarity_recommender,
        ]:

            m = mod.create(df_2, "user_id", "item_id")

            pop_model = m._get_popularity_baseline()

            self.sframe_comparer._assert_sframe_equal(
                pop_model.recommend(), base_model.recommend()
            )

            """
            TODO:
            Test CoreML export, when we have a dirarchiver that doesn't
            depend on the filesystem
            self._test_coreml_export(m, [1,2])
            """


class RecommendTest(RecommenderTestBase):
    def setUp(self):

        self.sf = tc.SFrame(
            {
                "user_id": [1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4],
                "item_id": [1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4],
                "rating": [2, 3, 4, 5, 2, 3.5, 3.5, 5, 5, 3, 2.5, 1.5, 5, 4, 3, 0.5],
                "time": [
                    10,
                    11,
                    12,
                    13,
                    10,
                    11,
                    12,
                    13,
                    13,
                    12,
                    11,
                    10,
                    13,
                    12,
                    11,
                    10,
                ],
            }
        )

    def test_num_recommendations(self):

        m = tc.ranking_factorization_recommender.create(
            self.sf, "user_id", "item_id", "rating"
        )

        # Test that observation side data was used.
        assert set(m.observation_data_column_names) == set(
            ["user_id", "item_id", "rating", "time"]
        )

        """
        TODO: test CoreML export, when we can support having side data, and
              thus, all factorization models.

        if m.target:
            self._test_coreml_export(m, [1,2], [2,3])
        else:
            self._test_coreml_export(m, [1,2])
        """

        for diversity in [0, 1]:

            r = m.recommend(
                users=None,
                k=5,
                items=None,
                new_observation_data=None,
                new_user_data=None,
                new_item_data=None,
                exclude=None,
                exclude_known=False,
                diversity=diversity,
            )
            assert r.num_rows() == 16

            r = m.recommend()
            assert r.num_rows() == 0

            r = m.recommend(
                users=None,
                k=2,
                items=None,
                new_observation_data=None,
                new_user_data=None,
                new_item_data=None,
                exclude=None,
                exclude_known=False,
                diversity=diversity,
            )
            assert r.num_rows() == 8

            r = m.recommend(
                users=[1, 2, 3],
                k=2,
                items=None,
                new_observation_data=None,
                new_user_data=None,
                new_item_data=None,
                exclude=None,
                exclude_known=False,
                diversity=diversity,
            )
            assert r.num_rows() == 6

            r = m.recommend(
                users=[1, 2, 3],
                k=3,
                items=[2, 3, 4],
                new_observation_data=None,
                new_user_data=None,
                new_item_data=None,
                exclude=None,
                exclude_known=False,
                diversity=diversity,
            )
            assert r.num_rows() == 9

            new_user_data = tc.SFrame(
                {"user_id": [1, 2, 3], "state": ["OR", "WA", "CA"]}
            )
            r = m.recommend(
                users=[1, 2, 3],
                k=3,
                items=[2, 3, 4],
                new_observation_data=None,
                new_user_data=new_user_data,
                new_item_data=None,
                exclude=None,
                exclude_known=False,
                diversity=diversity,
            )
            assert r.num_rows() == 9

            restriction_sf = tc.SFrame(
                {"user_id": [1, 1, 2, 2], "item_id": [1, 2, 2, 3]}
            )
            r = m.recommend(
                users=[1, 2, 3],
                k=3,
                items=restriction_sf,
                new_observation_data=None,
                new_user_data=None,
                new_item_data=None,
                exclude=None,
                exclude_known=False,
                diversity=diversity,
            )
            assert r.num_rows() == 4

            s = set((t["user_id"], t["item_id"]) for t in r[["user_id", "item_id"]])
            assert s == {(1, 1), (1, 2), (2, 2), (2, 3)}

            exclude = tc.SFrame({"user_id": [2, 3], "item_id": [2, 3]})
            r2 = m.recommend(
                users=None,
                k=5,
                items=None,
                new_observation_data=None,
                new_user_data=None,
                new_item_data=None,
                exclude=exclude,
                exclude_known=False,
                diversity=diversity,
            )
            assert r2.num_rows() == 16 - 2

    def test_other_arguments(self):
        obs = tc.SFrame(
            {"user_id": [5, 5, 5, 5], "item_id": [1, 2, 3, 4], "time": [10, 10, 11, 11]}
        )
        new_user_data = tc.SFrame({"user_id": [1, 2, 3], "state": ["OR", "WA", "CA"]})
        new_item_data = tc.SFrame({"item_id": [1, 2, 3], "category": ["A", "A", "B"]})
        exclude = tc.SFrame({"user_id": [2, 3], "item_id": [2, 3]})

        items = [None, tc.SArray([1, 2, 3])]
        new_observation_datas = [None, obs]
        new_user_datas = [None, new_user_data]
        new_item_datas = [None, new_item_data]
        excludes = [None, exclude]
        diversities = [0, 1]
        options = [
            items,
            new_observation_datas,
            new_user_datas,
            new_item_datas,
            excludes,
            diversities,
        ]
        m = tc.ranking_factorization_recommender.create(self.sf, target="rating")
        """
        TODO: test CoreML export, when we can support having side data, and
              thus, all factorization models.

        self._test_coreml_export(m, [1,2], [2,3])
        """
        for (
            item,
            new_observation_data,
            new_user_data,
            new_item_data,
            exclude,
            diversity,
        ) in itertools.product(*options):
            r = m.recommend(
                users=None,
                k=5,
                items=item,
                new_observation_data=new_observation_data,
                new_user_data=new_user_data,
                new_item_data=new_item_data,
                diversity=diversity,
                exclude=exclude,
                exclude_known=False,
            )
            assert r is not None

    def test_exclude(self):

        exclude = tc.SFrame({"user_id": ["2", "3"], "item_id": [2, 3]})
        m = tc.ranking_factorization_recommender.create(self.sf, target="rating")
        r = m.recommend(users=None, exclude=exclude, exclude_known=False)
        assert r.num_rows() == 14

    def test_side_data_used(self):

        # Test whether or not recommendations change when not using the explicit
        # observation side data.
        sf2 = self.sf[["user_id", "item_id", "rating"]]
        user_query = tc.SFrame(
            {
                "user_id": [1, 1, 2, 2, 3, 3, 4, 4],
                "time": [10, 13, 10, 13, 10, 13, 10, 13],
            }
        )
        user_query_2 = tc.SFrame(
            {
                "user_id": [1, 1, 2, 2, 3, 3, 4, 4],
                "time": [13, 10, 13, 10, 13, 10, 13, 10],
            }
        )

        # Use ranking factorization model with rating and make sure model
        # predictions change based on whether time is included
        m1 = tc.ranking_factorization_recommender.create(self.sf, target="rating")
        m2 = tc.ranking_factorization_recommender.create(sf2, target="rating")

        """
        TODO: test CoreML export, when we can support having side data, and
              thus, all factorization models.

        for m in [m1,m2]:
            self._test_coreml_export(m, [1,2], [2,3])
        """

        r1 = m1.recommend(exclude_known=False)
        r2 = m2.recommend(exclude_known=False)
        assert not all(r1["score"] == r2["score"])

        r3 = m1.recommend(users=user_query, exclude_known=False)
        self.assertRaises(
            ToolkitError, lambda: m2.recommend(users=user_query, exclude_known=False)
        )
        assert not all(r1["score"] == r3["score"][::2])

        # allow to take a list of dictionaries of the form [{'user_id':1,'time':10}] etc.
        flattened_query = list(user_query.apply(lambda x: x))
        r3 = m1.recommend(users=flattened_query, exclude_known=False)
        self.assertRaises(
            ToolkitError,
            lambda: m2.recommend(users=flattened_query, exclude_known=False),
        )
        assert not all(r1["score"] == r3["score"][::2])

        # Use ranking factorization model without rating and make sure model
        # predictions change based on whether time is included.
        m1 = tc.ranking_factorization_recommender.create(
            self.sf[["user_id", "item_id", "time"]]
        )
        m2 = tc.ranking_factorization_recommender.create(
            self.sf[["user_id", "item_id"]]
        )

        r1 = m1.recommend(exclude_known=False)
        r2 = m2.recommend(exclude_known=False)
        assert not all(r1["score"] == r2["score"])

        r3 = m1.recommend(users=user_query, exclude_known=False)
        assert not all(r1["score"] == r3["score"][::2])

        self.assertRaises(
            ToolkitError, lambda: m2.recommend(users=user_query, exclude_known=False)
        )
        r5 = m1.recommend(users=user_query_2, exclude_known=False)

        # Use factorization model with rating and make sure model
        # predictions change based on whether time is included.
        m1 = tc.factorization_recommender.create(self.sf, target="rating")
        m2 = tc.factorization_recommender.create(sf2, target="rating")

        r1 = m1.recommend(exclude_known=False)
        r2 = m2.recommend(exclude_known=False)
        assert not all(r1["score"] == r2["score"])

        r3 = m1.recommend(users=user_query, exclude_known=False)
        self.assertRaises(
            ToolkitError, lambda: m2.recommend(users=user_query, exclude_known=False)
        )
        assert not all(r1["score"] == r3["score"][::2])


class ItemIntersectionTest(RecommenderTestBase):
    def test_with_rating(self):

        df_dict = {
            "user_id": [0, 1, 2, 0, 1, 3, 3, 4],
            "item_id": [0, 0, 0, 1, 1, 1, 2, 2],
            "rating": [2, 3, 4, 5, 6, 7, 8, 9],
        }

        df = SFrame(df_dict)

        m = tc.recommender.popularity_recommender.create(
            df, "user_id", "item_id", "rating"
        )
        """
        TODO:
        Test CoreML export, when we have a dirarchiver that doesn't
        depend on the filesystem
        if m.target:
            self._test_coreml_export(m, [0,1], [2,5])
        else:
            self._test_coreml_export(m, [0,1])
        """

        query_items = [(0, 0), (0, 1), (0, 2), (1, 2), (1, 0), (0, 5)]
        query_sf = tc.SFrame(
            {
                "item_id_1": [v1 for v1, v2 in query_items],
                "item_id_2": [v2 for v1, v2 in query_items],
            }
        )

        out = m._get_item_intersection_info(query_items)

        true_out = copy(query_sf)

        true_out["num_users_1"] = [3, 3, 3, 3, 3, 3]
        true_out["num_users_2"] = [3, 3, 2, 2, 3, 0]
        true_out["intersection"] = [
            {0: (2, 2), 1: (3, 3), 2: (4, 4)},
            {0: (2, 5), 1: (3, 6)},
            {},
            {3: (7, 8)},
            {0: (5, 2), 1: (6, 3)},
            {},
        ]

        util.SFrameComparer()._assert_sframe_equal(out, true_out)

    def test_without_rating(self):

        df_dict = {
            "user_id": [0, 1, 2, 0, 1, 3, 3, 4],
            "item_id": [0, 0, 0, 1, 1, 1, 2, 2],
        }

        df = SFrame(df_dict)

        m = tc.recommender.popularity_recommender.create(df, "user_id", "item_id")

        """
        TODO:
        Test CoreML export, when we have a dirarchiver that doesn't
        depend on the filesystem
        self._test_coreml_export(m, [0,1])
        """

        query_items = [(0, 0), (0, 1), (0, 2), (1, 2), (1, 0), (0, 5)]
        query_sf = tc.SFrame(
            {
                "item_id_1": [v1 for v1, v2 in query_items],
                "item_id_2": [v2 for v1, v2 in query_items],
            }
        )

        out = m._get_item_intersection_info(query_items)

        true_out = copy(query_sf)

        true_out["num_users_1"] = [3, 3, 3, 3, 3, 3]
        true_out["num_users_2"] = [3, 3, 2, 2, 3, 0]
        true_out["intersection"] = [
            {0: (1, 1), 1: (1, 1), 2: (1, 1)},
            {0: (1, 1), 1: (1, 1)},
            {},
            {3: (1, 1)},
            {0: (1, 1), 1: (1, 1)},
            {},
        ]

        util.SFrameComparer()._assert_sframe_equal(out, true_out)


class RecommenderTest(RecommenderTestBase):
    def setUp(self):

        ratings_test_data = """userID,placeID,rating
U1077,135085,0
U1077,135038,1
U1077,132825,1
U1077,135060,1
U1068,135104,0
U1068,132740,0
U1068,132663,0
U1068,132732,0
U1068,132630,0
U1067,132584,1
U1067,132733,1
U1067,132732,1
U1067,132630,1
U1067,135104,1
U1067,132560,1
U1103,132584,0
U1103,132732,0
U1103,132630,0
U1103,132613,0
U1103,132667,0
U1103,135104,0"""

        try:
            write_dir = tempfile.mkdtemp()

            filename = join(write_dir, "tmp_data_file")
            o = open(filename, "w")
            o.write(ratings_test_data)
            o.close()
            self.df = SFrame.read_csv(filename)

            self.df_dict = {n: list(self.df[n]) for n in self.df.column_names()}

            ratings_test_data = """userID,placeID,NewColumn,rating
            U1077,135,085,0
            U1077,135,038,1
            U1077,132,825,1
            U1077,135,060,1
            U1068,135,104,0
            U1068,132,740,0
            U1068,132,663,0
            U1068,132,732,0
            U1068,132,630,0
            U1067,132,584,1
            U1067,132,733,1
            U1067,132,732,1
            U1067,132,630,1
            U1067,135,104,1
            U1067,132,560,1
            U1103,132,584,0
            U1103,132,732,0
            U1103,132,630,0
            U1103,132,613,0
            U1103,132,667,0
            U1103,135,104,0"""

            o = open(filename, "w")
            o.write(ratings_test_data)
            o.close()
            self.df_with_extra_side = SFrame.read_csv(filename)

            self.user_id = "userID"
            self.item_id = "placeID"
            self.target = "rating"
            self.train = self.df.head(10)
            self.test = self.df.tail(self.df.num_rows() - 10)
            self.df_improper = SFrame.read_csv(filename, delimiter="|")
            self.models = []
            for model_name in model_names:
                """
                TODO:
                Test CoreML export, when we have a dirarchiver that doesn't
                depend on the filesystem
                """
                m = self._get_trained_model(
                    model_name,
                    self.df,
                    user_id=self.user_id,
                    item_id=self.item_id,
                    target=self.target,
                    test_export_to_coreml=False,
                )
                self.models.append(m)

        finally:
            shutil.rmtree(write_dir)

    def test_implicit(self):
        implicit = self.df[[self.user_id, self.item_id]]

        m4 = tc.recommender.create(implicit, self.user_id, self.item_id, ranking=False)
        assert m4 is not None

        """
        TODO:
        Test CoreML export, when we have a dirarchiver that doesn't
        depend on the filesystem
        if m4.target:
            self._test_coreml_export(m4, ['135085','135038'], [0,1])
        else:
            self._test_coreml_export(m4, ['135085','135038'])
        """

    def test_recommend_from_interactions(self):
        data = tc.SFrame(
            {
                "userId": [1, 1, 1, 2, 2, 2, 3, 3, 3],
                "movieId": [10, 11, 12, 10, 13, 14, 10, 11, 14],
            }
        )
        exclude_pairs = tc.SFrame({"movieId": [14]})
        recommendations = tc.SFrame({"movieId": [10]})
        model = tc.item_similarity_recommender.create(
            data, user_id="userId", item_id="movieId"
        )
        recommendations = model.recommend_from_interactions(
            observed_items=recommendations, exclude=exclude_pairs
        )
        assert 14 not in recommendations["movieId"]

    def test_compare_models(self):
        from turicreate.toolkits.recommender.util import compare_models

        """
        TODO:
        Test CoreML export, when we have a dirarchiver that doesn't
        depend on the filesystem
        """

        model1 = self._get_trained_model(
            "popularity_recommender_with_target",
            self.df,
            user_id=self.user_id,
            item_id=self.item_id,
            target=self.target,
            test_export_to_coreml=False,
        )
        model2 = self._get_trained_model(
            "item_similarity_recommender",
            self.df,
            user_id=self.user_id,
            item_id=self.item_id,
            target=self.target,
            test_export_to_coreml=False,
        )

        x = compare_models(
            self.test, [model1, model2], skip_set=self.train, make_plot=False
        )
        assert x is not None

        model1 = self._get_trained_model(
            "popularity_recommender",
            self.df,
            user_id=self.user_id,
            item_id=self.item_id,
            target=self.target,
            test_export_to_coreml=False,
        )
        model2 = self._get_trained_model(
            "ranking_factorization_recommender_no_target",
            self.df,
            user_id=self.user_id,
            item_id=self.item_id,
            target=self.target,
            test_export_to_coreml=False,
        )

        x = compare_models(
            self.test, [model1, model2], skip_set=self.train, make_plot=False
        )
        assert x is not None

        model2 = self._get_trained_model(
            "factorization_recommender",
            self.df,
            user_id=self.user_id,
            item_id=self.item_id,
            target=self.target,
            test_export_to_coreml=False,
        )

        x = compare_models(
            self.test, [model1, model2], skip_set=self.train, make_plot=False
        )
        assert x is not None

    def _run_recommend_consistency_test(self, is_regression):

        if is_regression:
            X1 = tc.util.generate_random_sframe(1000, "ZZ")
            X2 = tc.util.generate_random_sframe(500, "ZZ")
            methods = [
                "popularity_recommender",
                "ranking_factorization_recommender",
                "item_similarity_recommender",
            ]

        else:
            X1 = tc.util.generate_random_regression_sframe(1000, "ZZ")
            X2 = tc.util.generate_random_regression_sframe(500, "ZZ")

            methods = [
                "popularity_recommender",
                "factorization_recommender",
                "item_similarity_recommender",
                "item_similarity_recommender_cosine",
                "item_similarity_recommender_pearson",
            ]

        users = list(X2["X1-Z"].unique())

        random.seed(0)
        blocks = sorted(
            list(range(10)) + random.sample(range(X2.num_rows()), 50) + [X2.num_rows()]
        )
        blocks_users = sorted(
            list(range(10)) + random.sample(range(len(users)), 30) + [len(users)]
        )

        for method in methods:

            """
            TODO:
            Test CoreML export, when we have a dirarchiver that doesn't
            depend on the filesystem
            """

            m = self._get_trained_model(
                method, X1, *X1.column_names(), test_export_to_coreml=False
            )

            # Make sure the predictions are the same.
            preds_1 = m.predict(X2)

            ############################################################
            # Broken up predictions

            pred_accumulator = []
            for lb, ub in zip(blocks[:-1], blocks[1:]):
                if lb == ub + 1:
                    p2 = m.predict(X2[lb])
                else:
                    p2 = m.predict(X2[lb:ub])

                pred_accumulator.append(p2)

            for r in pred_accumulator[1:]:
                pred_accumulator[0] = pred_accumulator[0].append(r)

            assert (preds_1 == pred_accumulator[0]).all()

            ############################################################
            # Broken up recommendations
            recs_1 = m.recommend(users)

            recs_accumulator = []
            for lb, ub in zip(blocks_users[:-1], blocks_users[1:]):
                recs_accumulator.append(m.recommend(users[lb:ub]))

            for r in recs_accumulator[1:]:
                recs_accumulator[0] = recs_accumulator[0].append(r)

            assert_sframe_equal(recs_1, recs_accumulator[0])

            ############################################################
            # Broken up recommendations, new data

            recs_2 = m.recommend(users, new_observation_data=X1)

            recs_accumulator_2 = []
            for lb, ub in zip(blocks_users[:-1], blocks_users[1:]):
                recs_accumulator_2.append(
                    m.recommend(users[lb:ub], new_observation_data=X1)
                )

            for r in recs_accumulator_2[1:]:
                recs_accumulator_2[0] = recs_accumulator_2[0].append(r)

            assert_sframe_equal(recs_2, recs_accumulator_2[0])

    def test_recommend_consistency_notarget(self):
        self._run_recommend_consistency_test(False)

    def test_recommend_consistency(self):
        self._run_recommend_consistency_test(True)

    def test_reg_value_regression(self):
        for method in [
            "factorization_recommender",
            "ranking_factorization_recommender",
        ]:
            for sgd_step_size in [0, 1e-20, 1e20]:
                """
                TODO:
                Test CoreML export, when we have a dirarchiver that doesn't
                depend on the filesystem
                """
                args = {
                    "max_iterations": 5,
                    "regularization": 1e-20,
                    "linear_regularization": 1e20,
                    "sgd_step_size": sgd_step_size,
                }
                m = self._get_trained_model(
                    method,
                    self.df,
                    self.user_id,
                    self.item_id,
                    target=self.target,
                    test_export_to_coreml=False,
                    **args
                )
                assert m is not None

                """
                TODO:
                Test CoreML export, when we have a dirarchiver that doesn't
                depend on the filesystem
                """
                args = {
                    "max_iterations": 5,
                    "regularization": 1e20,
                    "linear_regularization": 1e-20,
                    "sgd_step_size": sgd_step_size,
                }
                m = self._get_trained_model(
                    method,
                    self.df,
                    self.user_id,
                    self.item_id,
                    target=self.target,
                    test_export_to_coreml=False,
                    **args
                )
                assert m is not None

    def test_common_functions(self):

        df_dict_tup = {k: tuple(v) for k, v in six.iteritems(self.df_dict)}

        df_dict_ar = df_dict_tup.copy()
        df_dict_ar["rating"] = array.array("d", df_dict_ar["rating"])

        for m in self.models:
            m._name()
            for k in m._list_fields():
                m._get(k)
            m.summary()
            for data in [
                self.df,
                self.train,
                self.test,
                self.df_dict,
                df_dict_ar,
                df_dict_tup,
            ]:

                preds = m.predict(data)
                assert type(preds) == SArray

                recs = m.recommend()
                assert type(recs) == SFrame

                e = m.evaluate(data, verbose=False)
                assert e is not None
                assert type(e) == dict

                e = m.evaluate(data, metric="rmse", verbose=False)
                assert e is not None
                assert type(e) == dict

                e = m.evaluate(data, metric="precision_recall", verbose=False)
                assert e is not None
                assert type(e) == dict

            preds_1 = m.predict(self.df)
            preds_2 = m.predict(self.df_dict)
            preds_3_l = [m.predict(d) for d in self.df]

            preds_3 = preds_3_l[0]
            for p in preds_3_l[1:]:
                preds_3 = preds_3.append(p)

            assert (preds_1 == preds_2).all()
            assert (preds_1 == preds_3).all()

    def test_random_split(self):
        sf = tc.util.generate_random_sframe(20000, "cc")
        sf = sf.rename(dict(zip(sf.column_names(), ("user", "item"))), inplace=True)

        for proportion in [0, 0.2, 0.5, 1]:
            for seed in [0, 1, 2, 3, None]:

                train, test = random_split_by_user(
                    sf, "user", "item", item_test_proportion=proportion
                )

                if proportion == 0:
                    self.assertEqual(test.num_rows(), 0)
                    self.assertEqual(train.num_rows(), sf.num_rows())

                elif proportion == 1:
                    self.assertEqual(test.num_rows(), sf.num_rows())
                    self.assertEqual(train.num_rows(), 0)

                elif proportion < 0.5:
                    assert test.num_rows() <= train.num_rows()

                elif proportion > 0.5:
                    assert test.num_rows() >= train.num_rows()

                assert sf.column_names() == train.column_names()
                assert sf.column_names() == test.column_names()

                assert type(train) == tc.SFrame, "Training split has incorrect type."
                assert type(test) == tc.SFrame, "Test split has incorrect type."
                assert (
                    sf.num_rows() == train.num_rows() + test.num_rows()
                ), "Train/test split not a proper partition."

    def test_random_split_consistency(self):
        sf = tc.util.generate_random_sframe(20000, "cc")
        sf = sf.rename(dict(zip(sf.column_names(), ("user", "item"))), inplace=True)

        for proportion in [0, 0.2, 0.8, 1]:

            for seed in [0, 1, 2, 3]:

                train, test = random_split_by_user(
                    sf,
                    "user",
                    "item",
                    max_num_users=30,
                    random_seed=seed,
                    item_test_proportion=proportion,
                )

                train_2, test_2 = random_split_by_user(
                    sf,
                    "user",
                    "item",
                    max_num_users=30,
                    random_seed=seed,
                    item_test_proportion=proportion,
                )

                assert_sframe_equal(train, train_2)
                assert_sframe_equal(test, test_2)

    def test_random_split_random_generation(self):

        sf = tc.util.generate_random_sframe(20000, "cchHv")
        user_id, item_id = sf.column_names()[:2]

        max_num_users = 20

        for proportion in [0, 0.2, 0.8, 1]:

            for seed in [0, 1, 2, 3]:

                train, test = random_split_by_user(
                    sf,
                    user_id,
                    item_id,
                    random_seed=seed,
                    max_num_users=max_num_users,
                    item_test_proportion=proportion,
                )

                train_2, test_2 = random_split_by_user(
                    sf,
                    user_id,
                    item_id,
                    random_seed=seed,
                    max_num_users=max_num_users,
                    item_test_proportion=proportion,
                )

                if proportion == 0:
                    self.assertEqual(test.num_rows(), 0)
                    self.assertEqual(train.num_rows(), sf.num_rows())

                assert_sframe_equal(train, train_2)
                assert_sframe_equal(test, test_2)

                assert sf.column_names() == train.column_names()
                assert sf.column_names() == test.column_names()

                assert (
                    sf.num_rows() == train.num_rows() + test.num_rows()
                ), "Train/test split not a proper partition."

                rows_1 = set([str(x) for x in (list(train) + list(test))])
                rows_2 = set(str(x) for x in sf)

                self.assertEqual(rows_1, rows_2)

                self.assertLessEqual(len(set(test[user_id])), max_num_users)

    def test_improper_parse(self):
        # Make sure all models get a RuntimeError when trying to train on
        # improperly parsed data
        def _create_recommender(model_name):

            """
            TODO:
            Test CoreML export, when we have a dirarchiver that doesn't
            depend on the filesystem
            """
            return self._get_trained_model(
                model_name,
                self.df_improper,
                user_id=self.user_id,
                item_id=self.item_id,
                target=self.target,
                test_export_to_coreml=False,
            )

        for model_name in model_names:
            self.assertRaises(RuntimeError, lambda: _create_recommender(model_name))

    def test_save_and_load(self):

        try:
            write_dir = tempfile.mkdtemp()

            fn = join(write_dir, "tmp.gl")

            for m in self.models:
                m.save(fn)
                m2 = tc.load_model(fn)
                assert m2 is not None

                # TODO: Test equality of RecommenderModel objects
                assert m2.user_id == self.user_id
                assert m2.item_id == self.item_id
        finally:
            shutil.rmtree(write_dir)

    def test_bad_arguments(self):
        def _create_recommender(m, args):

            """
            TODO:
            Test CoreML export, when we have a dirarchiver that doesn't
            depend on the filesystem
            """
            return self._get_trained_model(
                m,
                self.train,
                user_id=self.user_id,
                item_id=self.item_id,
                target=self.target,
                test_export_to_coreml=False,
                **args
            )

        for m in model_names:
            self.assertRaises(
                Exception, lambda: _create_recommender(m, {"arg_that_isnt_there": None})
            )

        # Test bad arguments for factorization recommender
        model_name = "factorization_recommender"

        self.assertRaises(
            ToolkitError,
            lambda: _create_recommender(model_name, {"num_factors": "chuck_norris"}),
        )

        self.assertRaises(
            ToolkitError,
            lambda: _create_recommender(model_name, {"num_factors": np.NaN}),
        )

        self.assertRaises(
            ToolkitError,
            lambda: _create_recommender(model_name, {"num_factors": np.Inf}),
        )

        self.assertRaises(
            ToolkitError,
            lambda: _create_recommender(model_name, {"num_factors": -np.Inf}),
        )

        self.assertRaises(
            ToolkitError, lambda: _create_recommender(model_name, {"num_factors": -1})
        )

        self.assertRaises(
            ToolkitError,
            lambda: _create_recommender(model_name, {"max_iterations": 1.5}),
        )

        self.assertRaises(
            ToolkitError,
            lambda: _create_recommender(model_name, {"max_iterations": "chuck norris"}),
        )

        self.assertRaises(
            ToolkitError,
            lambda: _create_recommender(model_name, {"max_iterations": -1}),
        )

        self.assertRaises(
            ToolkitError,
            lambda: _create_recommender(model_name, {"max_iterations": np.NaN}),
        )

    def test_recommend(self):
        m = tc.recommender.create(self.train, self.user_id, self.item_id, verbose=False)
        # Test that we can provide recommendations for a subset of users.
        num_recommendations = 5
        train_users = SArray(list(set(self.train["userID"])))
        test_users = SArray(list(set(self.test["userID"])))
        """
        TODO:
        Test CoreML export, when we have a dirarchiver that doesn't
        depend on the filesystem
        if m.target:
            self._test_coreml_export(m, ['135085','135038'], [0,1])
        else:
            self._test_coreml_export(m, ['135085','135038'])
        """
        recs = m.recommend(users=train_users, k=num_recommendations)
        assert recs.num_rows() == num_recommendations * 3  # 3 unique users in train

        selected_users = SArray(list(set(self.test[self.user_id]))[:2])
        recs = m.recommend(users=selected_users, k=num_recommendations)
        assert type(recs) == SFrame
        assert recs.num_rows() == num_recommendations * len(selected_users)

        # Test that returned recommendations are the right shape
        recs = m.recommend(users=test_users, k=num_recommendations)
        assert recs.num_rows() == num_recommendations * 2  # 2 unique users in test

        # Check that the right number of recommendations are returned
        users = self.test[self.user_id].unique()
        assert recs.num_rows() == num_recommendations * len(users)
        assert recs.num_columns() == 4
        assert list(recs.column_names()) == [
            self.user_id,
            self.item_id,
            "score",
            "rank",
        ]

        # Check they are the correct type
        assert type(recs[self.user_id][0]) == type(self.df[self.user_id][0])
        assert type(recs[self.item_id][0]) == type(self.df[self.item_id][0])

        # Returns recommendations for all users in provided data set
        r_train = m.recommend(users=train_users)
        r_test = m.recommend(users=test_users)

        # Returns recommendations for only users in provided data set
        actual_users = frozenset(self.train[self.user_id].unique())
        recommended_users = frozenset(r_train[self.user_id].unique())
        assert actual_users == recommended_users

        # Returns recommendations for only users in provided data set
        actual_users = frozenset(self.test[self.user_id].unique())
        recommended_users = frozenset(r_test[self.user_id].unique())
        assert actual_users == recommended_users

        # All recommended items are in the recommended set
        actual_items = set(self.train[self.item_id].unique()) | set(
            self.test[self.item_id].unique()
        )

        recommended_items = set(r_train[self.item_id].unique())
        assert recommended_items.issubset(actual_items)

        # All recommended items are in the recommended set
        recommended_items = set(r_test[self.item_id].unique())
        assert recommended_items.issubset(actual_items)

        # No scores have NaNs
        assert r_test["score"].countna() == 0
        assert r_test["rank"].countna() == 0

        assert r_train["score"].countna() == 0
        assert r_train["rank"].countna() == 0

        # The provided data set are properly excluded from the returned
        # recommendations
        recs = m.recommend(users=train_users).to_dataframe()
        actual_pairs = [
            str(a) + " " + str(b)
            for (a, b) in zip(self.train[self.user_id], self.train[self.item_id])
        ]
        actual_pairs = frozenset(actual_pairs)
        recommended_pairs = [
            str(a) + " " + str(b)
            for (a, b) in zip(recs[self.user_id], recs[self.item_id])
        ]
        recommended_pairs = frozenset(recommended_pairs)
        assert actual_pairs.intersection(recommended_pairs) == frozenset()

    def test_rmse(self):
        df = self.df.to_dataframe()
        sf = self.df
        m = tc.recommender.factorization_recommender.create(
            sf, self.user_id, self.item_id, target="rating", verbose=False
        )
        """
        TODO:
        Test CoreML export, when we have a dirarchiver that doesn't
        depend on the filesystem
        if m.target:
            self._test_coreml_export(m, ['135085','135038'], [0,1])
        else:
            self._test_coreml_export(m, ['135085','135038'])
        """
        res = m.evaluate_rmse(sf, m.target)

        # Compute real answers
        df["prediction"] = m.predict(sf)
        df["residual"] = np.square(df["prediction"] - df["rating"])
        rmse_by_user = (
            df.groupby(self.user_id)["residual"].mean().apply(lambda x: np.sqrt(x))
        )
        rmse_by_item = (
            df.groupby(self.item_id)["residual"].mean().apply(lambda x: np.sqrt(x))
        )
        rmse_overall = np.sqrt(df["residual"].mean())

        # Compare overall RMSE
        assert (rmse_overall - res["rmse_overall"]) < DELTA

        # Compare by RMSE by user
        cpp_rmse_by_user = res["rmse_by_user"].to_dataframe()
        rmse_by_user = rmse_by_user.reset_index()

        assert set(cpp_rmse_by_user.columns.values) == set(
            [self.user_id, "rmse", "count"]
        )

        # No NaNs
        assert not pd.isnull(cpp_rmse_by_user["rmse"]).any()
        assert not pd.isnull(cpp_rmse_by_user["count"]).any()

        comparison = pd.merge(
            rmse_by_user, cpp_rmse_by_user, left_on=self.user_id, right_on=self.user_id
        )
        assert all(comparison["residual"] - comparison["rmse"] < DELTA)

        cpp_rmse_by_item = res["rmse_by_item"].to_dataframe()

        assert set(cpp_rmse_by_item.columns.values) == set(
            [self.item_id, "rmse", "count"]
        )

        # No NaNs
        assert not pd.isnull(cpp_rmse_by_item["rmse"]).any()
        assert not pd.isnull(cpp_rmse_by_item["count"]).any()

        rmse_by_item = rmse_by_item.reset_index()
        comparison = pd.merge(
            rmse_by_item, cpp_rmse_by_item, left_on=self.item_id, right_on=self.item_id
        )
        assert all(comparison["residual"] - comparison["rmse"] < DELTA)

    def precision(self, actual, predicted, k):
        assert k > 0
        if len(actual) == 0:
            return 0.0
        if len(predicted) == 0:
            return 1.0
        if len(predicted) > k:
            predicted = predicted[:k]
        num_hits = 0.0
        for i, p in enumerate(predicted):
            if p in actual and p not in predicted[:i]:
                num_hits += 1.0
        return num_hits / k

    def recall(self, actual, predicted, k):
        assert k > 0
        if len(actual) == 0:
            return 1.0
        if len(predicted) == 0:
            return 0.0
        if len(predicted) > k:
            predicted = predicted[:k]
        num_hits = 0.0
        for i, p in enumerate(predicted):
            if p in actual and p not in predicted[:i]:
                num_hits += 1.0
        return num_hits / len(actual)

    def test_small_example(self):
        sf = tc.SFrame()
        sf["user_id"] = ["0", "0", "0", "1", "1", "2", "2", "3", "3"]
        sf["item_id"] = ["A", "B", "C", "B", "C", "C", "D", "A", "D"]
        train = sf

        sf = tc.SFrame()
        sf["user_id"] = ["0", "0", "0", "1", "1", "2"]
        sf["item_id"] = ["D", "E", "F", "A", "F", "F"]
        test = sf

        user_id = "user_id"
        item_id = "item_id"

        m = tc.recommender.item_similarity_recommender.create(
            train, user_id, item_id, verbose=False
        )
        """
        TODO:
        Test CoreML export, when we have a dirarchiver that doesn't
        depend on the filesystem
        self._test_coreml_export(m, ['A','B'])
        """
        train_preds = m.predict(train)
        assert len(train_preds) == train.num_rows()

        recs = m.recommend(users=SArray(["0", "1", "2", "3"]))
        sorted_scores = recs.sort(["user_id", "item_id"])["score"]
        diffs = sorted_scores - tc.SArray(
            [
                (1.0 / 3 + 0 + 1.0 / 4) / 3,
                (1.0 / 3 + 1.0 / 4) / 2,
                (1.0 / 4) / 2,
                (1.0 / 4 + 1.0 / 3) / 2,
                (2.0 / 3 + 0) / 2,
                (1.0 / 3 + 0) / 2,
                (1.0 / 4 + 1.0 / 4) / 2,
            ]
        )
        assert all(abs(diffs) < DELTA)

        test_preds = m.predict(test)
        assert len(test_preds) == test.num_rows()

    def test_precision_recall(self):

        train = self.train
        test = self.test
        m = tc.recommender.create(train, self.user_id, self.item_id, verbose=False)

        """
        TODO:
        Test CoreML export, when we have a dirarchiver that doesn't
        depend on the filesystem
        self._test_coreml_export(m, ['135085','135038'])
        """
        users = set(list(test[self.user_id]))
        cutoff = 5

        # Check that method can run without the skip_set option
        r = m.evaluate_precision_recall(test, cutoffs=[5, 10])
        assert r is not None
        assert type(r) == dict

        # Convert to DataFrame for the tests below
        r = m.evaluate_precision_recall(test, cutoffs=[cutoff], skip_set=train)
        assert r is not None
        assert type(r) == dict

        # Test out of order columns
        r = m.evaluate_precision_recall(
            test[[self.item_id, self.user_id]], cutoffs=[cutoff], skip_set=train
        )
        assert r is not None
        assert type(r) == dict

        recs = m.recommend(k=cutoff).to_dataframe()
        results = r["precision_recall_by_user"]
        assert results.column_names() == [
            self.user_id,
            "cutoff",
            "precision",
            "recall",
            "count",
        ]

        for user in users:

            # Get observed values for this user
            actual = list(test[self.test[self.user_id] == user][self.item_id])

            # Get predictions
            predicted = list(recs[recs[self.user_id] == user][self.item_id])

            if len(predicted) > 0:

                # Get answers from C++
                p = results["precision"][results[self.user_id] == user][0]
                r = results["recall"][results[self.user_id] == user][0]

                p2 = self.precision(actual, predicted, cutoff)
                r2 = self.recall(actual, predicted, cutoff)

                # Compare with answers using Python
                assert abs(p - p2) < DELTA
                assert abs(r - r2) < DELTA


class SideDataTests(RecommenderTestBase):
    def setUp(self):

        self.sf = tc.SFrame(
            {
                "userID": ["0", "0", "0", "1", "1", "2", "8", "10"],
                "placeID": ["a", "b", "c", "a", "b", "b", "c", "d"],
                "rating": [0.2, 0.3, 0.4, 0.1, 0.3, 0.3, 0.5, 0.9],
            }
        )

        self.test_sf = tc.SFrame(
            {
                "userID": ["0", "0", "0", "1", "1", "2", "2", "2"],
                "placeID": ["a", "b", "c", "a", "b", "b", "c", "d"],
                "rating": [0.2, 0.3, 0.4, 0.1, 0.3, 0.3, 0.5, 0.9],
            }
        )

        self.user_side = tc.SFrame(
            {
                "userID": ["0", "1", "20"],
                "blahID": ["a", "b", "b"],
                "blahREAL": [0.1, 12, 22],
                "blahVECTOR": [
                    array.array("d", [0, 1]),
                    array.array("d", [0, 2]),
                    array.array("d", [2, 3]),
                ],
                "blahDICT": [{"a": 23}, {"a": 13}, {"a": 23, "b": 32}],
            }
        )

        self.item_side = tc.SFrame(
            {
                "placeID": ["a", "b", "f"],
                "blahID2": ["e", "e", "3"],
                "blahREAL2": [0.4, 12, 22],
                "blahVECTOR2": [
                    array.array("d", [0, 1, 2]),
                    array.array("d", [0, 2, 3]),
                    array.array("d", [2, 3, 3]),
                ],
                "blahDICT2": [{"a": 23}, {"b": 13}, {"a": 23, "c": 32}],
            }
        )

        self.user_id = "userID"
        self.item_id = "placeID"
        self.target = "rating"

    def test_bad_input(self):
        try:
            m = tc.recommender.create(
                self.sf, self.user_id, self.item_id, user_data="bad input"
            )
        except TypeError as e:
            self.assertEqual(str(e), "Provided user_data must be an SFrame.")
        try:
            m = tc.recommender.create(
                self.sf, self.user_id, self.item_id, item_data="bad input"
            )
        except TypeError as e:
            self.assertEqual(str(e), "Provided item_data must be an SFrame.")

    def test_model_creation(self):
        def check_model(m):
            expected = [
                "num_users",
                "num_items",
                "num_user_side_features",
                "num_item_side_features",
                "user_side_data_column_names",
                "user_side_data_column_types",
                "item_side_data_column_names",
                "item_side_data_column_types",
            ]

            observed = m._list_fields()
            for e in expected:
                assert e in observed

        try:
            write_dir = tempfile.mkdtemp()
            fn = join(write_dir, "tmp.gl")

            for u_side in [None, self.user_side]:
                for i_side in [None, self.item_side]:

                    for model_name in model_names:
                        if model_name == "item_content_recommender":
                            continue

                        """
                        TODO:
                        Test CoreML export, when we have a dirarchiver
                        that doesn't depend on the filesystem
                        """
                        m = self._get_trained_model(
                            model_name,
                            self.sf,
                            user_id=self.user_id,
                            item_id=self.item_id,
                            target=self.target,
                            test_export_to_coreml=False,
                            user_data=u_side,
                            item_data=i_side,
                        )
                        m.save(fn)
                        m1 = tc.load_model(fn)
                        check_model(m)
                        check_model(m1)
        finally:
            shutil.rmtree(write_dir)

    def test_recommender_create(self):
        sf_w_target = self.sf
        sf_no_target = self.sf[[self.user_id, self.item_id]]
        sf_binary_target = self.sf
        sf_binary_target[self.target] = 1

        m = tc.recommender.create(sf_w_target, self.user_id, self.item_id)
        assert isinstance(m, ItemSimilarityRecommender)
        self._test_coreml_export(m, ["a", "b"])

        m = tc.recommender.create(sf_no_target, self.user_id, self.item_id)
        assert isinstance(m, ItemSimilarityRecommender)
        self._test_coreml_export(m, ["a", "b"])

        m = tc.recommender.create(sf_w_target, self.user_id, self.item_id, self.target)
        assert isinstance(m, RankingFactorizationRecommender)
        """
        TODO: test CoreML export, when we can support serializing user
              data into CoreML model format.
        self._test_coreml_export(m, ['a','b'], [.2,.3])
        """

        m = tc.recommender.create(
            sf_w_target, self.user_id, self.item_id, self.target, ranking=False
        )
        assert isinstance(m, FactorizationRecommender)
        """
        TODO: test CoreML export, when we can support serializing user
              data into CoreML model format.
        self._test_coreml_export(m, ['a','b'], [.2,.3])
        """

        m = tc.recommender.create(
            sf_binary_target, self.user_id, self.item_id, self.target, ranking=False
        )
        assert isinstance(m, FactorizationRecommender)
        """
        TODO: test CoreML export, when we can support serializing user
              data into CoreML model format.
        self._test_coreml_export(m, ['a','b'], [.2,.3])
        """

        m = tc.recommender.create(
            sf_w_target,
            self.user_id,
            self.item_id,
            self.target,
            ranking=False,
            user_data=self.user_side,
        )
        assert isinstance(m, FactorizationRecommender)
        """
        TODO: test CoreML export, when we can support serializing user
              data into CoreML model format.
        self._test_coreml_export(m, ['a','b'], [.2,.3])
        """

        m = tc.recommender.create(
            sf_w_target,
            self.user_id,
            self.item_id,
            self.target,
            user_data=self.user_side,
            item_data=self.item_side,
        )
        assert isinstance(m, RankingFactorizationRecommender)
        """
        TODO: test CoreML export, when we can support serializing user
              data into CoreML model format.
        self._test_coreml_export(m, ['a','b'], [.2,.3])
        """

        m = tc.recommender.create(
            sf_no_target,
            self.user_id,
            self.item_id,
            ranking=False,
            user_data=self.user_side,
            item_data=self.item_side,
        )
        assert isinstance(m, RankingFactorizationRecommender)
        """
        TODO: test CoreML export, when we can support serializing user
              data into CoreML model format.
        self._test_coreml_export(m, ['a','b'], [.2,.3])
        """

        m = tc.recommender.create(
            sf_no_target, self.user_id, self.item_id, ranking=False
        )
        assert isinstance(m, ItemSimilarityRecommender)
        self._test_coreml_export(m, ["a", "b"])


class FactorizationTests(RecommenderTestBase):
    def setUp(self):
        self.model_names = [
            "default",
            "factorization_recommender",
            "ranking_factorization_recommender",
        ]

        self.df = tc.SFrame(
            {
                "userID": ["0", "0", "0", "1", "1", "2", "2", "2"],
                "placeID": ["a", "b", "c", "a", "b", "b", "c", "d"],
                "rating": [0.2, 0.3, 0.4, 0.1, 0.3, 0.3, 0.5, 0.9],
            }
        )
        self.test_df = tc.SFrame(
            {
                "userID": ["0", "0", "0", "1", "1", "2", "2", "2"],
                "placeID": ["a", "b", "c", "a", "b", "b", "c", "d"],
                "rating": [0.2, 0.3, 0.4, 0.1, 0.3, 0.3, 0.5, 0.9],
            }
        )

        self.user_side = tc.SFrame(
            {
                "userID": ["0", "1", "2"],
                "blahID": ["a", "b", "b"],
                "blahREAL": [0.1, 12, 22],
                "blahVECTOR": [
                    array.array("d", [0, 1]),
                    array.array("d", [0, 2]),
                    array.array("d", [2, 3]),
                ],
                "blahDICT": [{"a": 23}, {"a": 13}, {"a": 23, "b": 32}],
            }
        )

        self.item_side = tc.SFrame(
            {
                "placeID": ["a", "b", "d"],
                "blahID2": ["e", "e", "3"],
                "blahREAL2": [0.4, 12, 22],
                "blahVECTOR2": [
                    array.array("d", [0, 1, 2]),
                    array.array("d", [0, 2, 3]),
                    array.array("d", [2, 3, 3]),
                ],
                "blahDICT2": [{"a": 23}, {"b": 13}, {"a": 23, "c": 32, None: 12}],
            }
        )

        self.user_id = "userID"
        self.item_id = "placeID"
        self.target = "rating"

        self.models = []

        for u_side in [None, self.user_side]:
            for i_side in [None, self.item_side]:

                for model_name in self.model_names:

                    m = self._get_trained_model(
                        model_name,
                        self.df,
                        user_id=self.user_id,
                        item_id=self.item_id,
                        target=self.target,
                        test_export_to_coreml=False,
                        user_data=u_side,
                        item_data=i_side,
                    )
                    self.models.append((model_name, m))

    def test_evaluate_with_side_data(self):

        for u_side in [None, self.user_side]:
            for i_side in [None, self.item_side]:
                for (mname, m) in self.models:

                    e = m.evaluate(
                        self.test_df,
                        new_user_data=u_side,
                        new_item_data=i_side,
                        verbose=False,
                    )
                    assert "precision_recall_by_user" in e

                    recs = m.recommend(k=1, new_user_data=u_side, new_item_data=i_side)

                    assert recs is not None
                    assert recs.num_rows() == len(self.df[self.user_id].unique())

    def test_data_summary_fields(self):
        for (model_name, m) in self.models:
            expected = [
                "num_users",
                "num_items",
                "num_user_side_features",
                "num_item_side_features",
                "observation_data_column_names",
                "user_side_data_column_names",
                "user_side_data_column_types",
                "item_side_data_column_names",
                "item_side_data_column_types",
            ]
            observed = m._list_fields()
            for e in expected:
                assert e in observed

    def test_matrix_factorization_values(self):

        test_vars = [
            ("side_data_factorization", [True, False]),
            # ("verbose", [True, False]),
            ("binary_target", [True, False]),
            ("nmf", [True, False]),
            ("random_seed", [0, 5]),
            ("solver", ["sgd", "als"]),
        ]

        for var, values in test_vars:
            for v in values:
                m = tc.factorization_recommender.create(
                    self.df, "userID", "placeID", "rating", **{var: v}
                )

                assert m._get(var) == v

    def test_ranking_factorization_values(self):

        test_vars = [
            ("side_data_factorization", [True, False]),
            # ("verbose", [True, False]),  # not used
            ("binary_target", [True, False]),
            ("nmf", [True, False]),
            ("random_seed", [0, 5]),
            ("solver", ["sgd", "ials"]),
        ]

        for var, values in test_vars:
            for v in values:
                m = tc.ranking_factorization_recommender.create(
                    self.df, "userID", "placeID", "rating", **{var: v}
                )

                assert m._get(var) == v

    def test_retrieve_factors(self):

        for model_name, m in self.models:

            d = m._get("coefficients")

            if "nmf" not in model_name:
                assert "intercept" in d

            assert "userID" in d
            assert "placeID" in d

            assert set(d["userID"]["userID"]) == set(self.df["userID"])
            assert set(d["placeID"]["placeID"]) == set(self.df["placeID"])

            if "linear_regression" not in model_name:
                assert len(d["userID"]["factors"][0]) == m._get("num_factors")

            if "blahID" in d:
                assert set(d["blahID"]["blahID"]) == set(self.user_side["blahID"])

            if "blahID2" in d:
                assert set(d["blahID2"]["blahID2"]) == set(self.item_side["blahID2"])

            if "blahREAL" in d:
                assert list(d["blahREAL"]["index"]) == [0]

            if "blahREAL2" in d:
                assert list(d["blahREAL2"]["index"]) == [0]

            if "blahVECTOR" in d:
                assert list(d["blahVECTOR"]["index"]) == [0, 1]

            if "blahVECTOR2" in d:
                assert list(d["blahVECTOR2"]["index"]) == [0, 1, 2]

            if "blahDICT" in d:
                assert set(d["blahDICT"]["blahDICT"]) == {"a", "b"}

            if "blahDICT2" in d:
                assert set(d["blahDICT2"]["blahDICT2"]) == {"a", "b", "c", None}

    def test_MF_recommend_bug(self):

        X = tc.SFrame()

        X["user"] = list(range(10000))
        X["item"] = [i % 5 for i in range(10000)]
        X["rating"] = [float(i) / 10000 for i in range(10000)]

        m = tc.recommender.factorization_recommender.create(
            X, "user", "item", "rating", max_iterations=1
        )

        # sometimes segfaults in gl 1.3.0
        m.recommend([10000])


class TestContentRecommender(RecommenderTestBase):
    def test_basic(self):

        item_data = tc.SFrame(
            {"my_item_id": range(10), "data": [[1, 0]] * 5 + [[0, 1]] * 5}
        )

        m = tc.recommender.item_content_recommender.create(item_data, "my_item_id")

        self.assertEqual(m._get("num_users"), 0)
        self.assertEqual(m._get("num_items"), 10)
        self.assertEqual(m._get("num_observations"), 0)

        new_observation_data = tc.SFrame(
            {"__implicit_user__": [0] * 4, "my_item_id": range(4)}
        )

        # Test the recommend API in this case.
        out = m.recommend([0], k=1, new_observation_data=new_observation_data)

        self.assertEqual(out.column_names()[1], "my_item_id")
        self.assertEqual(out["my_item_id"].dtype, int)
        self.assertEqual(out.column_names()[2], "score")
        self.assertEqual(out.column_names()[3], "rank")

        self.assertEqual(out[0]["my_item_id"], 4)

        # Test the recommend_from_interactions.
        out_2 = m.recommend_from_interactions(list(range(4)), k=1)

        self.assertEqual(out_2.column_names()[0], "my_item_id")
        self.assertEqual(out_2["my_item_id"].dtype, int)
        self.assertEqual(out_2.column_names()[1], "score")
        self.assertEqual(out_2.column_names()[2], "rank")

        self.assertEqual(out_2[0]["my_item_id"], 4)
        self._test_coreml_export(m, [0, 1])

    def test_weights(self):

        item_data = tc.SFrame(
            {
                "my_item_id": range(4),
                "data_1": [[1, 0], [1, 0], [0, 1], [0.5, 0.5]],
                "data_2": [[0, 1], [1, 0], [0, 1], [0.5, 0.5]],
            }
        )

        # If the weights are set to auto, then they are currently set
        # equally.  In this case, element 3 will be closer to 0 than 1 or 2.
        m_1 = tc.recommender.item_content_recommender.create(item_data, "my_item_id")
        out_1 = m_1.recommend_from_interactions([0], k=1)

        self.assertEqual(out_1[0]["my_item_id"], 3)

        # If the weights are set so that data_1 is 1 and data_2 is 0,
        # then 1 will be closer to 0 than 2 or 3.
        m_2 = tc.recommender.item_content_recommender.create(
            item_data, "my_item_id", weights={"data_1": 1, "data_2": 0}
        )
        out_2 = m_2.recommend_from_interactions([0], k=1)
        self.assertEqual(out_2[0]["my_item_id"], 1)

        # If the weights are set so that data_1 is 0 and data_2 is 1,
        # then 2 will be closer to 0 than 1 or 3.
        m_3 = tc.recommender.item_content_recommender.create(
            item_data, "my_item_id", weights={"data_1": 0, "data_2": 1}
        )
        out_3 = m_3.recommend_from_interactions([0], k=1)
        self.assertEqual(out_3[0]["my_item_id"], 2)
        for m in [m_1, m_2, m_3]:
            self._test_coreml_export(m, [0, 1])

    def test_basic_string_type(self):

        item_data = tc.SFrame({"my_item_id": range(10), "data": ["a"] * 5 + ["b"] * 5})

        m = tc.recommender.item_content_recommender.create(item_data, "my_item_id")

        self.assertEqual(m._get("num_users"), 0)
        self.assertEqual(m._get("num_items"), 10)
        self.assertEqual(m._get("num_observations"), 0)

        new_observation_data = tc.SFrame(
            {"__implicit_user__": [0] * 4, "my_item_id": range(4)}
        )

        # Test the recommend API in this case.
        out = m.recommend([0], k=1, new_observation_data=new_observation_data)

        self.assertEqual(out.column_names()[1], "my_item_id")
        self.assertEqual(out["my_item_id"].dtype, int)
        self.assertEqual(out.column_names()[2], "score")
        self.assertEqual(out.column_names()[3], "rank")

        self.assertEqual(out[0]["my_item_id"], 4)

        # Test the recommend_from_interactions.
        out_2 = m.recommend_from_interactions(list(range(4)), k=1)

        self.assertEqual(out_2.column_names()[0], "my_item_id")
        self.assertEqual(out_2["my_item_id"].dtype, int)
        self.assertEqual(out_2.column_names()[1], "score")
        self.assertEqual(out_2.column_names()[2], "rank")

        self.assertEqual(out_2[0]["my_item_id"], 4)
        self._test_coreml_export(m, [0, 1])

    def test_basic_mixed_types(self):

        item_data = tc.util.generate_random_sframe(50, "cCsSdDnnnv")
        item_data["item_id"] = range(50)
        item_data["item_id"] = item_data["item_id"].astype(str)

        m = tc.recommender.item_content_recommender.create(item_data, "item_id")

        self.assertEqual(m._get("num_users"), 0)
        self.assertEqual(m._get("num_items"), 50)
        self.assertEqual(m._get("num_observations"), 0)

        # Test the recommend API in this case.
        out = m.recommend_from_interactions(["0", "1", "2"], k=10)

        self.assertEqual(out.num_rows(), 10)

        observation_data = tc.SFrame(
            {
                "users": list(range(8)) * 5,
                "item_id": [str(r) for r in list(range(10)) * 4],
            }
        )

        self._test_coreml_export(m, ["0", "1"])

        m = tc.recommender.item_content_recommender.create(
            item_data, "item_id", observation_data, "users"
        )

        self.assertEqual(m._get("num_users"), 8)
        self.assertEqual(m._get("num_items"), 50)
        self.assertEqual(m._get("num_observations"), 5 * 8)

        # Test the recommend API in this case.
        out_2 = m.recommend_from_interactions(["0", "1", "2"], k=10)

        assert_sframe_equal(out, out_2)

        # Test that it preserves the correct
        out_3 = m.recommend([0], k=10)

        user_0_items = set(
            (observation_data["item_id"])[observation_data["users"] == 0]
        )
        out_3_items = set(out_3["item_id"])

        self.assertEqual(len(user_0_items & out_3_items), 0)
        self._test_coreml_export(m, ["0", "1"])

    def test_get_similar_items(self):

        item_data = tc.util.generate_random_sframe(25, "cCsSdDnnnv")

        # rows 0-25 and 25-50 are the same, but different item ids
        item_data = item_data.append(item_data)
        item_data["item_id"] = range(50)
        item_data["item_id"] = item_data["item_id"].astype(str)

        m = tc.recommender.item_content_recommender.create(item_data, "item_id")

        self.assertEqual(m._get("num_users"), 0)
        self.assertEqual(m._get("num_items"), 50)
        self.assertEqual(m._get("num_observations"), 0)

        sim_items = m.get_similar_items([str(i) for i in range(25)], k=1)

        self.assertEqual(sim_items.num_rows(), 25)

        for d in sim_items:
            self.assertEqual(int(d["similar"]), int(d["item_id"]) + 25)
        self._test_coreml_export(m, ["0", "1"])

    def test_regression_1(self):

        temp_sframe = tc.SFrame(
            {"my_item_id": range(4), "data_1": [0, 1, 0, 0], "data_2": [0, 1, 0, 0]}
        )
        tc.item_content_recommender.create(temp_sframe, "my_item_id")


class ItemSimilarityCoreMLExportTest(unittest.TestCase):
    def test_export_model_size(self):
        # Test that the users are completely dropped.

        X = tc.util.generate_random_sframe(100, "ss")

        Xr = X.copy()
        X2 = X.copy()

        for i in range(19):
            X2["X1-s"] = X["X1-s"].apply(lambda s: s + ("-%d" % i))
            X2.materialize()
            Xr = Xr.append(X2)

        # Train two recommenders, one with 20x the number of users.
        m1 = tc.recommender.item_similarity_recommender.create(
            X, user_id="X1-s", item_id="X2-s"
        )
        m2 = tc.recommender.item_similarity_recommender.create(
            Xr, user_id="X1-s", item_id="X2-s"
        )

        self.assertEqual(m1.num_users, 10)
        self.assertEqual(m2.num_users, 20 * 10)

        temp_file_path_1 = _mkstemp()[1]
        temp_file_path_2 = _mkstemp()[1]

        m1.export_coreml(temp_file_path_1)
        m2.export_coreml(temp_file_path_2)

        s1 = os.path.getsize(temp_file_path_1)
        s2 = os.path.getsize(temp_file_path_2)

        # Make sure that the differences in size is less than 10%
        self.assertLessEqual(abs(s2 - s1) / s1, 0.1)


if __name__ == "__main__":

    # Check if we are supposed to connect to another server
    for i, v in enumerate(sys.argv):
        if v.startswith("ipc://"):
            tc._launch(v)

            # The rest of the arguments need to get passed through to
            # the unittest module
            del sys.argv[i]
            break

    unittest.main()

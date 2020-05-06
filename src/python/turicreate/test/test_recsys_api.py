# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
import sys
import turicreate as tc

DELTA = 0.000001


class AdditionalDataTest(unittest.TestCase):
    def setUp(self):
        data = tc.SFrame()
        data["user_id"] = ["a", "b", "b", "c", "c", "c"]
        data["item_id"] = ["x", "x", "y", "v", "w", "z"]
        data["rating"] = [0, 1, 2, 3, 4, 5]

        # Make internal indices so that we can check predictions/ranking.
        # IDs are in the order they are seen in the above data SFrame.
        user_index = {"a": 0, "b": 1, "c": 2}
        item_index = {"x": 0, "y": 1, "v": 2, "w": 3, "z": 4}
        user_data = tc.SFrame()
        user_data["user_id"] = ["a", "b"]
        user_data["user_feature_value"] = [0.5, 0.9]
        user_data["user_dict_value"] = [{1: 0.5}, {4: 0.9}]
        user_data["user_vect_value"] = [[0, 1, 2], [2, 3, 4]]
        user_data["user_str_dict_value"] = [{"tt": 0.5}, {"ttt": 0.9}]
        item_data = tc.SFrame()
        item_data["item_id"] = ["x", "v", "w", "y"]
        item_data["item_feature_value"] = [-0.3, 0.7, 0.3, 0.05]
        item_data["item_dict_value"] = [{1: 0.5}, {4: 0.9}, {4: 0.9}, {5: 1, 6: 2}]
        item_data["item_vect_value"] = [[0, 1, 2], [2, 3, 4], [2, 3, 4], [2, 3, 5]]
        item_data["item_str_dict_value"] = [
            {"tt": 0.5},
            {"tt": 0.9},
            {"t": 0.9},
            {"ttt": 0.9},
        ]
        new_data = tc.SFrame()
        new_data["user_id"] = ["a", "b"]
        new_data["item_id"] = ["v", "z"]
        new_data["rating"] = [7, 8]
        new_user_data = tc.SFrame()
        new_user_data["user_id"] = ["a", "c"]
        new_user_data["user_feature_value"] = [0.0, 2.9]
        new_user_data["user_dict_value"] = [{1: 0.5}, {4: 0.9}]
        new_user_data["user_vect_value"] = [[0, 1, 2], [2, 3, 4]]
        new_user_data["user_str_dict_value"] = [{"tt": 0.5}, {"ttt": 0.9}]

        new_item_data = tc.SFrame()
        new_item_data["item_id"] = ["y", "z"]
        new_item_data["item_feature_value"] = [0.5, 0.6]
        new_item_data["item_dict_value"] = [{1: 0.5}, {4: 0.9}]
        new_item_data["item_vect_value"] = [[0, 1, 2], [2, 3, 4]]
        new_item_data["item_str_dict_value"] = [{"tt": 0.5}, {"ttt": 0.9}]

        exclude = tc.SFrame()
        exclude["user_id"] = ["a"]
        exclude["item_id"] = ["x"]

        users_all = tc.SArray(["a", "b", "c"])
        items_all = tc.SArray(["v", "w", "x", "y", "z"])
        items_some = tc.SArray(["v", "w"])

        self.data = data
        self.user_data = user_data
        self.item_data = item_data
        self.new_data = new_data
        self.new_user_data = new_user_data
        self.new_item_data = new_item_data
        self.exclude = exclude
        self.users_all = users_all
        self.items_all = items_all
        self.items_some = items_some
        self.user_index = user_index
        self.item_index = item_index

    def test_recommender_models(self):
        data = self.data
        user_data = self.user_data
        item_data = self.item_data
        for mod in [
            tc.factorization_recommender,
            tc.ranking_factorization_recommender,
            tc.popularity_recommender,
            tc.item_similarity_recommender,
        ]:

            m = mod.create(
                data,
                user_id="user_id",
                item_id="item_id",
                target="rating",
                user_data=user_data,
                item_data=item_data,
            )
            assert m is not None

            self._test_score(m)
            self._test_basic(m)
            self._test_recommend(m)

    def _test_basic(self, m):
        result = m.summary()
        assert result is None

        result = m.__repr__()
        assert result is not None

        result = m.__str__()
        assert result is not None

        result = m._list_fields()
        assert result is not None

        for field in m._list_fields():
            m._get(field)

    def _test_score(self, m):
        data = self.data

        # Test predict returns something
        pred = m.predict(data)
        assert len(pred) == data.num_rows()

        new_data = self.new_data
        new_user_data = self.new_user_data
        new_item_data = self.new_item_data

        pred = m.predict(data, new_data, new_user_data, new_item_data)
        assert len(pred) == data.num_rows()

    def _test_recommend(self, m):
        new_data = self.new_data
        new_user_data = self.new_user_data
        new_item_data = self.new_item_data
        users_all = self.users_all
        items_all = self.items_all
        exclude = self.exclude

        top_k = 5

        # Test recommend returns something
        recs = m.recommend(
            users_all,
            top_k,
            exclude,
            items_all,
            new_data,
            new_user_data,
            new_item_data,
            exclude_known=True,
        )
        assert recs is not None

        # Test recommend when no new data is provided
        recs = m.recommend(users_all, top_k, exclude, items_all, exclude_known=True)
        assert recs is not None

        recs2 = m.recommend()
        for c in recs.column_names():
            assert all(recs[c] == recs2[c])

    def test_new_side_data_regression(self):
        data = self.data
        user_data = self.user_data
        item_data = self.item_data

        for mod in [
            tc.recommender.item_similarity_recommender,
            tc.recommender.factorization_recommender,
            tc.recommender.ranking_factorization_recommender,
            tc.recommender.popularity_recommender,
        ]:

            m = mod.create(data, "user_id", "item_id", "rating")

            # Make sure it doesn't crash
            m.recommend(new_user_data=user_data)
            m.recommend(new_item_data=item_data)

    def test_kwargs(self):
        data = self.data

        for mod in [
            tc.recommender.item_similarity_recommender,
            tc.recommender.factorization_recommender,
            tc.recommender.ranking_factorization_recommender,
            tc.recommender.popularity_recommender,
        ]:
            self.assertRaises(
                TypeError,
                lambda: mod.create(
                    data, "user_id", "item_id", "rating", i_want_a_pony=True
                ),
            )

    def test_side_data_errors(self):

        # This test makes sure that passing in a column name as the
        # observation_data that was originally part of the side data
        # causes an error.

        X = tc.SFrame()

        X["user_id"] = [1231, 1232]
        X["item_id"] = [131, 1232]
        X["side_info"] = [111, 111]
        X["rating"] = [12, 13]

        from copy import copy

        X2 = copy(X)

        # Add in one that overlaps the item hair in the field below
        X2["item_hair"] = [1342, 24233]
        del X2["item_id"]

        user_side = tc.SFrame()
        user_side["user_id"] = [1231, 1232]
        user_side["user_hair"] = ["big", "bigger"]

        item_side = tc.SFrame()
        item_side["item_id"] = [1231, 1232]
        item_side["item_hair"] = ["big", "bigger"]

        for mod in [
            tc.recommender.item_similarity_recommender,
            tc.recommender.factorization_recommender,
            tc.recommender.ranking_factorization_recommender,
            tc.recommender.popularity_recommender,
        ]:

            m = mod.create(
                X,
                "user_id",
                "item_id",
                "rating",
                user_data=user_side,
                item_data=item_side,
            )

            self.assertRaises(Exception, lambda: m.recommend(X2))


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

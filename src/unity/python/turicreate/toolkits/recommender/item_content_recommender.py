# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
The Item Content Recommender recommends similar items, where similar is
determined by information about the items rather than the user
interaction patterns.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _turicreate
from turicreate import SFrame as _SFrame
from turicreate import SArray as _SArray
from turicreate.toolkits.recommender.util import _Recommender
from array import array as _array


def create(item_data, item_id,
           observation_data = None,
           user_id = None, target = None,
           weights = 'auto',
           similarity_metrics = 'auto',
           item_data_transform = 'auto',
           max_item_neighborhood_size = 64, verbose=True):

    """Create a content-based recommender model in which the similarity
    between the items recommended is determined by the content of
    those items rather than learned from user interaction data.

    The similarity score between two items is calculated by first
    computing the similarity between the item data for each column,
    then taking a weighted average of the per-column similarities to
    get the final similarity.  The recommendations are generated
    according to the average similarity of a candidate item to all the
    items in a user's set of rated items.

    Parameters
    ----------

    item_data : SFrame
        An SFrame giving the content of the items to use to learn the
        structure of similar items.  The SFrame must have one column
        that matches the name of the `item_id`; this gives a unique
        identifier that can then be used to make recommendations.  The rest
        of the columns are then used in the distance calculations
        below.

    item_id : string
        The name of the column in item_data (and `observation_data`,
        if given) that represents the item ID.

    observation_data : None (optional)
        An SFrame giving user and item interaction data.  This
        information is stored in the model, and the recommender will
        recommend the items with the most similar content to the
        items that were present and/or highly rated for that user.

    user_id : None (optional)
        If observation_data is given, then this specifies the column
        name corresponding to the user identifier.

    target_id : None (optional)
        If observation_data is given, then this specifies the column
        name corresponding to the target or rating.

    weights : dict or 'auto' (optional)
        If given, then weights must be a dictionary of column names
        present in item_data to weights between the column names.  If
        'auto' is given, the all columns are weighted equally.

    max_item_neighborhood_size : int, 64
        For each item, we hold this many similar items to use when
        aggregating models for predictions.  Decreasing this value
        decreases the memory required by the model and decreases the
        time required to generate recommendations, but it may also
        decrease recommendation accuracy.

    verbose : True or False (optional)
        If set to False, then less information is printed.

    Examples
    --------

      >>> item_data = tc.SFrame({"my_item_id" : range(4),
                                 "data_1" : [ [1, 0], [1, 0], [0, 1], [0.5, 0.5] ],
                                 "data_2" : [ [0, 1], [1, 0], [0, 1], [0.5, 0.5] ] })

      >>> m = tc.recommender.item_content_recommender.create(item_data, "my_item_id")
      >>> m.recommend_from_interactions([0])

      Columns:
              my_item_id      int
              score   float
              rank    int

      Rows: 3

      Data:
      +------------+----------------+------+
      | my_item_id |     score      | rank |
      +------------+----------------+------+
      |     3      | 0.707106769085 |  1   |
      |     1      |      0.5       |  2   |
      |     2      |      0.5       |  3   |
      +------------+----------------+------+
      [3 rows x 3 columns]

      >>> m.recommend_from_interactions([0, 1])

      Columns:
              my_item_id      int
              score   float
              rank    int

      Rows: 2

      Data:
      +------------+----------------+------+
      | my_item_id |     score      | rank |
      +------------+----------------+------+
      |     3      | 0.707106769085 |  1   |
      |     2      |      0.25      |  2   |
      +------------+----------------+------+
      [2 rows x 3 columns]

    """
    # item_data is correct type
    if not isinstance(item_data, _SFrame) or item_data.num_rows() == 0:
        raise TypeError("`item_data` argument must be a non-empty SFrame giving item data to use for similarities.")

    # Error checking on column names
    item_columns = set(item_data.column_names())

    if item_id not in item_columns:
            raise ValueError("Item column given as 'item_id = %s', but this is not found in `item_data` SFrame."
                             % item_id)

    # Now, get the set ready to test for other argument issues.
    item_columns.remove(item_id)

    if weights != 'auto':
        if type(weights) is not dict:
            raise TypeError("`weights` parameter must be 'auto' or a dictionary of column "
                            "names in `item_data` to weight values.")

        bad_columns = [col_name for col_name in item_columns if col_name not in item_columns]
        if bad_columns:
            raise ValueError("Columns %s given in weights, but these are not found in item_data."
                             % ', '.join(bad_columns))

        # Now, set any columns not given in the weights column to be
        # weight 0.
        for col_name in item_columns:
            weights.setdefault(col_name, 0)

    ################################################################################
    # Now, check the feature transformer stuff.

    # Pass it through a feature transformer.
    if item_data_transform == 'auto':
        item_data_transform = _turicreate.toolkits._feature_engineering.AutoVectorizer(excluded_features = [item_id])

    if not isinstance(item_data_transform, _turicreate.toolkits._feature_engineering.TransformerBase):
        raise TypeError("item_data_transform must be 'auto' or a valid feature_engineering transformer instance.")

    # Transform the input data.
    item_data = item_data_transform.fit_transform(item_data)

    # Translate any string columns to actually work in nearest
    # neighbors by making it a categorical list.  Also translate lists
    # into dicts, and normalize numeric columns.
    normalization_columns = []
    gaussian_kernel_metrics = set()

    for c in item_columns:
        if item_data[c].dtype is str:
            item_data[c] = item_data[c].apply(lambda s: {s : 1})
        elif item_data[c].dtype in [float, int]:
            item_data[c] = (item_data[c] - item_data[c].mean()) / max(item_data[c].std(), 1e-8)
            gaussian_kernel_metrics.add(c)

    if verbose:
        print("Applying transform:")
        print(item_data_transform)

    # The name of this model.
    method = 'item_content_recommender'

    opts = {'model_name': method}
    response = _turicreate.toolkits._main.run("recsys_init", opts)
    model_proxy = response['model']

    # The user_id is implicit if none is given.
    if user_id is None:
        user_id = "__implicit_user__"

    normalization_factor = 1

    # Set the observation data.
    if observation_data is None:

        # In this case, it's important to make this a string type.  If
        # the user column is not given, it may be given at recommend
        # time, in which case it is cast to a string type and cast
        # back if necessary.
        empty_user = _turicreate.SArray([], dtype=str)
        empty_item = _turicreate.SArray([], dtype=item_data[item_id].dtype)
        observation_data = _turicreate.SFrame( {user_id : empty_user, item_id : empty_item} )

    # Now, work out stuff for the observation_data component
    normalization_factor = 1

    # 1 for the item_id column.
    if item_data.num_columns() >= 3:

        if weights == "auto":

            # TODO: automatically tune this.
            weights = {col_name : 1 for col_name in item_data.column_names() if col_name != item_id}

        # Use the abs value here in case users pass in weights with negative values.
        normalization_factor = sum(abs(v) for v in weights.values())
        if normalization_factor == 0:
            raise ValueError("Weights cannot all be set to 0.")

        distance = [([col_name], ("gaussian_kernel" if col_name in gaussian_kernel_metrics else "cosine"), weight)
                      for col_name, weight in weights.items()]

    else:
        distance = "cosine"

    # Now, build the nearest neighbors model:
    nn = _turicreate.nearest_neighbors.create(item_data, label=item_id, distance = distance, verbose = verbose)
    graph = nn.query(item_data, label = item_id, k=max_item_neighborhood_size, verbose = verbose)
    graph = graph.rename({"query_label" : item_id,
                          "reference_label" : "similar",
                          "distance" : "score"}, inplace=True)

    def process_weights(x):
        return max(-1, min(1, 1 - x / normalization_factor))

    graph["score"] = graph["score"].apply(process_weights)

    opts = {'dataset': observation_data,
            'user_id': user_id,
            'item_id': item_id,
            'target': target,
            'user_data': _turicreate.SFrame(),
            'item_data': item_data,
            'nearest_items': graph,
            'model': model_proxy,
            'similarity_type' : "cosine",
            'max_item_neighborhood_size' : max_item_neighborhood_size}

    response = _turicreate.toolkits._main.run('recsys_train', opts, verbose)
    out_model = ItemContentRecommender(response['model'])

    return out_model

class ItemContentRecommender(_Recommender):
    """A recommender based on the similarity between item content rather
    using user interaction patterns to compute similarity.

    **Creating an ItemContentRecommender**

    This model cannot be constructed directly.  Instead, use
    :func:`turicreate.recommender.item_content_recommender.create`
    to create an instance
    of this model. A detailed list of parameter options and code samples
    are available in the documentation for the create function.

    Notes
    -----
    **Model Definition**

    This model first computes the similarity between items using the
    content of each item. The similarity score between two items is
    calculated by first computing the similarity between the item data
    for each column, then taking a weighted average of the per-column
    similarities to get the final similarity.  The recommendations are
    generated according to the average similarity of a candidate item
    to all the items in a user's set of rated items.

    For more examples, see the associated `create` function.
    """

    def __init__(self, model_proxy):
        '''__init__(self)'''
        self.__proxy__ = model_proxy

    @classmethod
    def _native_name(cls):
        return "item_content_recommender"


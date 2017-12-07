# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Models that rank items based on their popularity.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _turicreate
from turicreate.toolkits.recommender.util import _Recommender

def create(observation_data,
           user_id='user_id', item_id='item_id', target=None,
           user_data=None, item_data=None,
           random_seed=0,
           verbose=True):
    """
    Create a model that makes recommendations using item popularity. When no
    target column is provided, the popularity is determined by the number of
    observations involving each item. When a target is provided, popularity
    is computed using the item's mean target value. When the target column
    contains ratings, for example, the model computes the mean rating for
    each item and uses this to rank items for recommendations.

    Parameters
    ----------
    observation_data : SFrame
        The dataset to use for training the model. It must contain a column of
        user ids and a column of item ids. Each row represents an observed
        interaction between the user and the item.  The (user, item) pairs
        are stored with the model so that they can later be excluded from
        recommendations if desired. It can optionally contain a target ratings
        column. All other columns are interpreted by the underlying model as
        side features for the observations.

        The user id and item id columns must be of type 'int' or 'str'. The
        target column must be of type 'int' or 'float'.

    user_id : string, optional
        The name of the column in `observation_data` that corresponds to the
        user id.

    item_id : string, optional
        The name of the column in `observation_data` that corresponds to the
        item id.

    target : string, optional
        The `observation_data` can optionally contain a column of scores
        representing ratings given by the users. If present, the name of this
        column may be specified variables `target`.

    user_data : SFrame, optional
        Side information for the users.  This SFrame must have a column with
        the same name as what is specified by the `user_id` input parameter.
        `user_data` can provide any amount of additional user-specific
        information.

    item_data : SFrame, optional
        Side information for the items.  This SFrame must have a column with
        the same name as what is specified by the `item_id` input parameter.
        `item_data` can provide any amount of additional item-specific
        information.

    verbose : bool, optional
        Enables verbose output.

    Examples
    --------
    >>> sf = turicreate.SFrame({'user_id': ["0", "0", "0", "1", "1", "2", "2", "2"],
    ...                       'item_id': ["a", "b", "c", "a", "b", "b", "c", "d"],
    ...                       'rating': [1, 3, 2, 5, 4, 1, 4, 3]})
    >>> m = turicreate.popularity_recommender.create(sf, target='rating')

    See Also
    --------
    PopularityRecommender
    """

    opts = {'model_name': 'popularity'}
    response = _turicreate.toolkits._main.run("recsys_init", opts)
    model_proxy = response['model']

    if user_data is None:
        user_data = _turicreate.SFrame()
    if item_data is None:
        item_data = _turicreate.SFrame()

    opts = {'dataset': observation_data,
            'user_id': user_id,
            'item_id': item_id,
            'target': target,
            'user_data': user_data,
            'item_data': item_data,
            'nearest_items': _turicreate.SFrame(),
            'model': model_proxy,
            'random_seed': 1}

    response = _turicreate.toolkits._main.run('recsys_train', opts, verbose)
    return PopularityRecommender(response['model'])

class PopularityRecommender(_Recommender):
    """
    The Popularity Model ranks an item according to its overall popularity.

    When making recommendations, the items are scored by the number of times it
    is seen in the training set. The item scores are the same for all users.
    Hence the recommendations are not tailored for individuals.

    The Popularity Recommender is simple and fast and provides a reasonable baseline.
    It can work well when observation data is sparse. It can be used as a
    "background" model for new users.

    **Creating a PopularityRecommender**

    This model cannot be constructed directly.  Instead, use
    :func:`turicreate.recommender.popularity_recommender.create`
    to create an instance
    of this model. A detailed list of parameter options and code samples
    are available in the documentation for the create function.

    See Also
    --------
    create
    """

    def __init__(self, model_proxy):
        '''__init__(self)'''
        self.__proxy__ = model_proxy

    @classmethod
    def _native_name(cls):
        return "popularity"


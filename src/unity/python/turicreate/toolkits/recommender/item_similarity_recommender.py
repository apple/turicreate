# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Methods for creating models that rank items according to their similarity
to other items.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _turicreate
from turicreate.toolkits.recommender.util import _Recommender
from turicreate.toolkits._model import _get_default_options_wrapper

def create(observation_data,
           user_id='user_id', item_id='item_id', target=None,
           user_data=None, item_data=None,
           nearest_items=None,
           similarity_type='jaccard',
           threshold=0.001,
           only_top_k=64,
           verbose=True,
           target_memory_usage = 8*1024*1024*1024,
           **kwargs):
    """
    Create a recommender that uses item-item similarities based on
    users in common.

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
        information. (NB: This argument is currently ignored by this model.)

    item_data : SFrame, optional
        Side information for the items.  This SFrame must have a column with
        the same name as what is specified by the `item_id` input parameter.
        `item_data` can provide any amount of additional item-specific
        information. (NB: This argument is currently ignored by this model.)

    similarity_type : {'jaccard', 'cosine', 'pearson'}, optional
        Similarity metric to use. See ItemSimilarityRecommender for details.
        Default: 'jaccard'.

    threshold : float, optional
        Predictions ignore items below this similarity value.
        Default: 0.001.

    only_top_k : int, optional
        Number of similar items to store for each item. Default value is
        64.  Decreasing this  decreases the amount of memory required for the
        model, but may also decrease the accuracy.

    nearest_items : SFrame, optional
        A set of each item's nearest items. When provided, this overrides
        the similarity computed above.
        See Notes in the documentation for ItemSimilarityRecommender.
        Default: None.

    target_memory_usage : int, optional
        The target memory usage for the processing buffers and lookup
        tables.  The actual memory usage may be higher or lower than this,
        but decreasing this decreases memory usage at the expense of
        training time, and increasing this can dramatically speed up the
        training time.  Default is 8GB = 8589934592.

    seed_item_set_size : int, optional
        For users that have not yet rated any items, or have only
        rated uniquely occurring items with no similar item info,
        the model seeds the user's item set with the average
        ratings of the seed_item_set_size most popular items when
        making predictions and recommendations.  If set to 0, then
        recommendations based on either popularity (no target present)
        or average item score (target present) are made in this case.

    training_method : (advanced), optional.
        The internal processing is done with a combination of nearest
        neighbor searching, dense tables for tracking item-item
        similarities, and sparse item-item tables.  If 'auto' is chosen
        (default), then the estimated computation time is estimated for
        each, and the computation balanced between the methods in order to
        minimize training time given the target memory usage.  This allows
        the user to force the use of one of these methods.  All should give
        equivalent results; the only difference would be training time.
        Possible values are {'auto', 'dense', 'sparse', 'nn', 'nn:dense',
        'nn:sparse'}. 'dense' uses a dense matrix to store item-item
        interactions as a lookup, and may do multiple passes to control
        memory requirements. 'sparse' does the same but with a sparse lookup
        table; this is better if the data has many infrequent items.  "nn"
        uses a brute-force nearest neighbors search.  "nn:dense" and
        "nn:sparse" use nearest neighbors for the most frequent items
        (see nearest_neighbors_interaction_proportion_threshold below),
        and either sparse or dense matrices for the remainder.  "auto"
        chooses the method predicted to be the fastest based on the
        properties of the data.

    nearest_neighbors_interaction_proportion_threshold : (advanced) float
        Any item that has was rated by more than this proportion of
        users is  treated by doing a nearest neighbors search.  For
        frequent items, this  is almost always faster, but it is slower
        for infrequent items.  Furthermore, decreasing this causes more
        items to be processed using the nearest neighbor path, which may
        decrease memory requirements.

    degree_approximation_threshold : (advanced) int, optional
        Users with more than this many item interactions may be
        approximated.  The approximation is done by a combination of
        sampling and choosing the interactions likely to have the most
        impact on the model.  Increasing this can increase the training time
        and may or may not increase the quality of the model.  Default = 4096.

    max_data_passes : (advanced) int, optional
        The maximum number of passes through the data allowed in
        building the similarity lookup tables.  If it is not possible to
        build the recommender in this many passes (calculated before
        that stage of training), then additional approximations are
        applied; namely decreasing degree_approximation_threshold.  If
        this is not possible, an error is raised.  To decrease the
        number of passes required, increase target_memory_usage or
        decrease nearest_neighbors_interaction_proportion_threshold.
        Default = 1024.

    Examples
    --------
    Given basic user-item observation data, an
    :class:`~turicreate.recommender.item_similarity_recommender.ItemSimilarityRecommender` is created:

    >>> sf = turicreate.SFrame({'user_id': ['0', '0', '0', '1', '1', '2', '2', '2'],
    ...                       'item_id': ['a', 'b', 'c', 'a', 'b', 'b', 'c', 'd']})
    >>> m = turicreate.item_similarity_recommender.create(sf)
    >>> recs = m.recommend()

    When a target is available, one can specify the desired similarity. For
    example we may choose to use a cosine similarity, and use it to make
    predictions or recommendations.

    >>> sf2 = turicreate.SFrame({'user_id': ['0', '0', '0', '1', '1', '2', '2', '2'],
    ...                        'item_id': ['a', 'b', 'c', 'a', 'b', 'b', 'c', 'd'],
    ...                        'rating': [1, 3, 2, 5, 4, 1, 4, 3]})
    >>> m2 = turicreate.item_similarity_recommender.create(sf2, target="rating",
    ...                                                  similarity_type='cosine')
    >>> m2.predict(sf)
    >>> m2.recommend()

    Notes
    -----
    Currently, :class:`~turicreate.recommender.item_similarity_recommender.ItemSimilarityRecommender`
    does not leverage the use of side features `user_data` and `item_data`.

    **Incorporating pre-defined similar items**

    For item similarity models, one may choose to provide user-specified
    nearest neighbors graph using the keyword argument `nearest_items`. This is
    an SFrame containing, for each item, the nearest items and the similarity
    score between them. If provided, these item similarity scores are used for
    recommendations. The SFrame must contain (at least) three columns:

    * 'item_id': a column with the same name as that provided to the `item_id`
      argument (which defaults to the string "item_id").
    * 'similar': a column containing the nearest items for the given item id.
      This should have the same type as the `item_id` column.
    * 'score': a numeric score measuring how similar these two items are.

    For example, suppose you first create an ItemSimilarityRecommender and use
    :class:`~turicreate.recommender.ItemSimilarityRecommender.get_similar_items`:

    >>> sf = turicreate.SFrame({'user_id': ["0", "0", "0", "1", "1", "2", "2", "2"],
    ...                       'item_id': ["a", "b", "c", "a", "b", "b", "c", "d"]})
    >>> m = turicreate.item_similarity_recommender.create(sf)
    >>> nn = m.get_similar_items()
    >>> m2 = turicreate.item_similarity_recommender.create(sf, nearest_items=nn)

    With the above code, the item similarities computed for model `m` can be
    used to create a new recommender object, `m2`. Note that we could have
    created `nn` from some other means, but now use `m2` to make
    recommendations via `m2.recommend()`.


    See Also
    --------
    ItemSimilarityRecommender

    """

    method = 'item_similarity'

    opts = {'model_name': method}
    response = _turicreate.toolkits._main.run("recsys_init", opts)
    model_proxy = response['model']

    if user_data is None:
        user_data = _turicreate.SFrame()
    if item_data is None:
        item_data = _turicreate.SFrame()
    if nearest_items is None:
        nearest_items = _turicreate.SFrame()

    if "training_method" in kwargs and kwargs["training_method"] in ["in_memory", "sgraph"]:
        print("WARNING: training_method = " + str(kwargs["training_method"]) + " deprecated; see documentation.")
        kwargs["training_method"] = "auto"

    opts = {'dataset': observation_data,
            'user_id': user_id,
            'item_id': item_id,
            'target': target,
            'user_data': user_data,
            'item_data': item_data,
            'nearest_items': nearest_items,
            'model': model_proxy,
            'similarity_type': similarity_type,
            'threshold': threshold,
            'target_memory_usage' : float(target_memory_usage),
            'max_item_neighborhood_size': only_top_k}

    if kwargs:
        try:
            possible_args = set(_get_default_options()["name"])
        except (RuntimeError, KeyError):
            possible_args = set()

        bad_arguments = set(kwargs.keys()).difference(possible_args)
        if bad_arguments:
            raise TypeError("Bad Keyword Arguments: " + ', '.join(bad_arguments))

        opts.update(kwargs)

    opts.update(kwargs)

    response = _turicreate.toolkits._main.run('recsys_train', opts, verbose)
    return ItemSimilarityRecommender(response['model'])


_get_default_options = _get_default_options_wrapper(
                          'item_similarity',
                          'recommender.item_similarity',
                          'ItemSimilarityRecommender')

class ItemSimilarityRecommender(_Recommender):
    """
    A model that ranks an item according to its similarity to other items
    observed for the user in question.

    **Creating an ItemSimilarityRecommender**

    This model cannot be constructed directly.  Instead, use
    :func:`turicreate.recommender.item_similarity_recommender.create`
    to create an instance
    of this model. A detailed list of parameter options and code samples
    are available in the documentation for the create function.

    Notes
    -----
    **Model Definition**

    This model first computes the similarity
    between items using the observations of users who have interacted with both
    items. Given a similarity between item :math:`i` and :math:`j`,
    :math:`S(i,j)`, it scores an item :math:`j` for user :math:`u` using a
    weighted average of the user's previous observations :math:`I_u`.

    There are three choices of similarity metrics to use: 'jaccard',
    'cosine' and 'pearson'.

    `Jaccard similarity
    <http://en.wikipedia.org/wiki/Jaccard_index>`_
    is used to measure the similarity between two set of elements.
    In the context of recommendation, the Jaccard similarity between two
    items is computed as

    .. math:: \mbox{JS}(i,j)
            = \\frac{|U_i \cap U_j|}{|U_i \cup U_j|}

    where :math:`U_{i}` is the set of users who rated item :math:`i`.
    Jaccard is a good choice when one only has implicit feedbacks of items
    (e.g., people rated them or not), or when one does not care about how
    many stars items received.

    If one needs to compare the ratings of items, Cosine and Pearson similarity
    are recommended.

    The Cosine similarity between two items is computed as

    .. math:: \mbox{CS}(i,j)
            = \\frac{\sum_{u\in U_{ij}} r_{ui}r_{uj}}
                {\sqrt{\sum_{u\in U_{i}} r_{ui}^2}
                 \sqrt{\sum_{u\in U_{j}} r_{uj}^2}}

    where :math:`U_{i}` is the set of users who rated item :math:`i`,
    and :math:`U_{ij}` is the set of users who rated both items :math:`i` and
    :math:`j`. A problem with Cosine similarity is that it does not consider
    the differences in the mean and variance of the ratings made to
    items :math:`i` and :math:`j`.

    Another popular measure that compares ratings where the effects of means and
    variance have been removed is Pearson Correlation similarity:

    .. math:: \mbox{PS}(i,j)
            = \\frac{\sum_{u\in U_{ij}} (r_{ui} - \\bar{r}_i)
                                        (r_{uj} - \\bar{r}_j)}
                {\sqrt{\sum_{u\in U_{ij}} (r_{ui} - \\bar{r}_i)^2}
                 \sqrt{\sum_{u\in U_{ij}} (r_{uj} - \\bar{r}_j)^2}}

    The predictions of items depend on whether `target` is specified.
    When the `target` is absent, a prediction for item :math:`j` is made via

    .. math:: y_{uj}
            = \\frac{\sum_{i \in I_u} \mbox{SIM}(i,j)  }{|I_u|}


    Otherwise, predictions for ``jaccard`` and ``cosine`` similarities are made via

    .. math:: y_{uj}
            = \\frac{\sum_{i \in I_u} \mbox{SIM}(i,j) r_{ui} }{\sum_{i \in I_u} \mbox{SIM}(i,j)}

    Predictions for ``pearson`` similarity are made via

    .. math:: y_{uj}
            = \\bar{r}_j + \\frac{\sum_{i \in I_u} \mbox{SIM}(i,j) (r_{ui} - \\bar{r}_i) }{\sum_{i \in I_u} \mbox{SIM}(i,j)}


    For more details of item similarity methods, please see, e.g.,
    Chapter 4 of [Ricci_et_al]_.

    See Also
    --------
    create

    References
    ----------
    .. [Ricci_et_al] Francesco Ricci, Lior Rokach, and Bracha Shapira.
        `Introduction to recommender systems handbook
        <http://www.inf.unibz.it/~ricci/papers
        /intro-rec-sys-handbook.pdf>`_. Springer US, 2011.
    """

    def __init__(self, model_proxy):
        '''__init__(self)'''
        self.__proxy__ = model_proxy

    @classmethod
    def _native_name(cls):
        return "item_similarity"


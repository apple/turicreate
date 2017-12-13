# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Methods for performing doing matrix factorization and factorization machines
for making a ranking-based recommender.  See
turicreate.recommender.ranking_factorization_recommender.create for additional documentation.
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
           num_factors=32,
           regularization=1e-9,
           linear_regularization=1e-9,
           side_data_factorization=True,
           ranking_regularization=0.25,
           unobserved_rating_value=None,
           num_sampled_negative_examples=4,
           max_iterations=25,
           sgd_step_size=0,
           random_seed=0,
           binary_target = False,
           solver = 'auto',
           verbose=True,
           **kwargs):
    """Create a RankingFactorizationRecommender that learns latent factors for each
    user and item and uses them to make rating predictions.

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

    num_factors : int, optional
        Number of latent factors.

    regularization : float, optional
        L2 regularization for interaction terms. Default: 1e-10; a typical range
        for this parameter is between 1e-12 and 1. Setting this to 0 may cause
        numerical issues.

    linear_regularization : float, optional
        L2 regularization for linear term. Default: 1e-10; a typical range for this
        parameter is between 1e-12 and 1. Setting this to 0 may cause numerical issues.

    side_data_factorization : boolean, optional
        Use factorization for modeling any additional features beyond the user
        and item columns. If True, and side features or any additional columns are
        present, then a Factorization Machine model is trained. Otherwise, only
        the linear terms are fit to these features.  See
        :class:`turicreate.recommender.ranking_factorization_recommender.RankingFactorizationRecommender`
        for more information. Default: True.

    ranking_regularization : float, optional
        Penalize the predicted value of user-item pairs not in the
        training set. Larger values increase this penalization.
        Suggested values: 0, 0.1, 0.5, 1.  NOTE: if no target column
        is present, this parameter is ignored.

    unobserved_rating_value : float, optional
        Penalize unobserved items with a larger predicted score than this value.
        By default, the estimated 5% quantile is used (mean - 1.96*std_dev).

    num_sampled_negative_examples : integer, optional
        For each (user, item) pair in the data, the ranking sgd solver evaluates
        this many randomly chosen unseen items for the negative example step.
        Increasing this can give better performance at the expense of speed,
        particularly when the number of items is large.  Default is 4.

    binary_target : boolean, optional
        Assume the target column is composed of 0's and 1's. If True, use
        logistic loss to fit the model.

    max_iterations : int, optional
        The training algorithm will make at most this many iterations through
        the observed data. Default: 50.

    sgd_step_size : float, optional
        Step size for stochastic gradient descent. Smaller values generally
        lead to more accurate models that take more time to train. The
        default setting of 0 means that the step size is chosen by trying
        several options on a small subset of the data.

    random_seed :  int, optional
        The random seed used to choose the initial starting point for
        model training. Note that some randomness in the training is
        unavoidable, so models trained with the same random seed may still
        differ. Default: 0.

    solver : string, optional
        Name of the solver to be used to solve the regression. See the
        references for more detail on each solver. The available solvers for
        this model are:

        - *auto (default)*: automatically chooses the best solver for the data
                              and model parameters.
        - *ials*:           Implicit Alternating Least Squares [1].
        - *adagrad*:        Adaptive Gradient Stochastic Gradient Descent.
        - *sgd*:            Stochastic Gradient Descent

    verbose : bool, optional
        Enables verbose output.

    kwargs : optional
        Optional advanced keyword arguments passed in to the model
        optimization procedure. These parameters do not typically
        need to be changed.

    Examples
    --------
    **Basic usage**

    When given just user and item pairs, one can create a RankingFactorizationRecommender
    as follows.

    >>> sf = turicreate.SFrame({'user_id': ["0", "0", "0", "1", "1", "2", "2", "2"],
    ...                       'item_id': ["a", "b", "c", "a", "b", "b", "c", "d"])
    >>> from turicreate.recommender import ranking_factorization_recommender
    >>> m1 = ranking_factorization_recommender.create(sf)

    When a target column is present, one can include this to try and recommend
    items that are rated highly.

    >>> sf = turicreate.SFrame({'user_id': ["0", "0", "0", "1", "1", "2", "2", "2"],
    ...                       'item_id': ["a", "b", "c", "a", "b", "b", "c", "d"],
    ...                       'rating': [1, 3, 2, 5, 4, 1, 4, 3]})

    >>> m1 = ranking_factorization_recommender.create(sf, target='rating')


    **Including side features**

    >>> user_info = turicreate.SFrame({'user_id': ["0", "1", "2"],
    ...                              'name': ["Alice", "Bob", "Charlie"],
    ...                              'numeric_feature': [0.1, 12, 22]})
    >>> item_info = turicreate.SFrame({'item_id': ["a", "b", "c", "d"],
    ...                              'name': ["item1", "item2", "item3", "item4"],
    ...                              'dict_feature': [{'a' : 23}, {'a' : 13},
    ...                                               {'b' : 1},
    ...                                               {'a' : 23, 'b' : 32}]})
    >>> m2 = ranking_factorization_recommender.create(sf, target='rating',
    ...                                               user_data=user_info,
    ...                                               item_data=item_info)

    **Customizing ranking regularization**

    Create a model that pushes predicted ratings of unobserved user-item
    pairs toward 1 or below.

    >>> m3 = ranking_factorization_recommender.create(sf, target='rating',
    ...                                               ranking_regularization = 0.1,
    ...                                               unobserved_rating_value = 1)

    **Using the implicit alternating least squares model**

    Ranking factorization also implements implicit alternating least squares [1] as
    an alternative solver.  This is enable using ``solver = 'ials'``.

    >>> m3 = ranking_factorization_recommender.create(sf, target='rating',
                                                      solver = 'ials')

    See Also
    --------
    :class:`turicreate.recommender.factorization_recommender.FactorizationRecommender`,
    :class:`turicreate.recommender.ranking_factorization_recommender.RankingFactorizationRecommender`

    References
    -----------

    [1] Collaborative Filtering for Implicit Feedback Datasets Hu, Y.; Koren,
        Y.; Volinsky, C. IEEE International Conference on Data Mining
        (ICDM 2008), IEEE (2008).

    """

    method = 'ranking_factorization_recommender'

    opts = {'model_name': method}
    response = _turicreate.toolkits._main.run("recsys_init", opts)
    model_proxy = response['model']

    if user_data is None:
        user_data = _turicreate.SFrame()
    if item_data is None:
        item_data = _turicreate.SFrame()

    if target is None:
        binary_target = True

    opts = {'dataset'                 : observation_data,
            'user_id'                 : user_id,
            'item_id'                 : item_id,
            'target'                  : target,
            'user_data'               : user_data,
            'item_data'               : item_data,
            'nearest_items'           : _turicreate.SFrame(),
            'model'                   : model_proxy,
            'random_seed'             : random_seed,
            'num_factors'             : num_factors,
            'regularization'          : regularization,
            'linear_regularization'   : linear_regularization,
            'ranking_regularization'  : ranking_regularization,
            'binary_target'           : binary_target,
            'max_iterations'          : max_iterations,
            'side_data_factorization' : side_data_factorization,
            'num_sampled_negative_examples' : num_sampled_negative_examples,
            'solver'                  : solver,

            # Has no effect here.
            # 'verbose'                 : verbose,
            'sgd_step_size'           : sgd_step_size}

    if unobserved_rating_value is not None:
        opts["unobserved_rating_value"] = unobserved_rating_value

    if kwargs:
        try:
            possible_args = set(_get_default_options()["name"])
        except (RuntimeError, KeyError):
            possible_args = set()

        bad_arguments = set(kwargs.keys()).difference(possible_args)
        if bad_arguments:
            raise TypeError("Bad Keyword Arguments: " + ', '.join(bad_arguments))

        opts.update(kwargs)

    response = _turicreate.toolkits._main.run('recsys_train', opts, verbose)
    return RankingFactorizationRecommender(response['model'])

_get_default_options = _get_default_options_wrapper(
                          'ranking_factorization_recommender',
                          'recommender.RankingFactorizationRecommender',
                          'RankingFactorizationRecommender')

class RankingFactorizationRecommender(_Recommender):
    r"""
    A RankingFactorizationRecommender learns latent factors for each
    user and item and uses them to rank recommended items according to
    the likelihood of observing those (user, item) pairs. This is
    commonly desired when performing collaborative filtering for
    implicit feedback datasets or datasets with explicit ratings
    for which ranking prediction is desired.

    RankingFactorizationRecommender contains a number of options that
    tailor to a variety of datasets and evaluation metrics, making
    this one of the most powerful models in the Turi Create
    recommender toolkit.

    **Creating a RankingFactorizationRecommender**

    This model cannot be constructed directly.  Instead, use
    :func:`turicreate.recommender.ranking_factorization_recommender.create`
    to create an instance
    of this model. A detailed list of parameter options and code samples
    are available in the documentation for the create function.

    **Side information**

    Side features may be provided via the `user_data` and `item_data` options
    when the model is created.

    Additionally, observation-specific information, such as the time of day when
    the user rated the item, can also be included. Any column in the
    `observation_data` SFrame that is not the user id, item id, or target is
    treated as a observation side features. The same side feature columns must
    be present when calling :meth:`predict`.

    Side features may be numeric or categorical. User ids and item ids are
    treated as categorical variables. For the additional side features, the type
    of the :class:`~turicreate.SFrame` column determines how it's handled: strings
    are treated as categorical variables and integers and floats are treated as
    numeric variables. Dictionaries and numeric arrays are also supported.

    **Optimizing for ranking performance**

    By default, RankingFactorizationRecommender optimizes for the
    precision-recall performance of recommendations.

    **Model parameters**

    Trained model parameters may be accessed using
    `m.get('coefficients')` or equivalently `m['coefficients']`, where `m`
    is a RankingFactorizationRecommender.

    See Also
    --------
    create, :func:`turicreate.recommender.factorization_recommender.create`

    Notes
    -----

    **Model Details**

    `RankingFactorizationRecommender` trains a model capable of predicting a score for
    each possible combination of users and items.  The internal coefficients of
    the model are learned from known scores of users and items.
    Recommendations are then based on these scores.

    In the two factorization models, users and items are represented by weights
    and factors.  These model coefficients are learned during training.
    Roughly speaking, the weights, or bias terms, account for a user or item's
    bias towards higher or lower ratings.  For example, an item that is
    consistently rated highly would have a higher weight coefficient associated
    with them.  Similarly, an item that consistently receives below average
    ratings would have a lower weight coefficient to account for this bias.

    The factor terms model interactions between users and items.  For example,
    if a user tends to love romance movies and hate action movies, the factor
    terms attempt to capture that, causing the model to predict lower scores
    for action movies and higher scores for romance movies.  Learning good
    weights and factors is controlled by several options outlined below.

    More formally, the predicted score for user :math:`i` on item :math:`j` is
    given by

    .. math::
       \operatorname{score}(i, j) =
          \mu + w_i + w_j
          + \mathbf{a}^T \mathbf{x}_i + \mathbf{b}^T \mathbf{y}_j
          + {\mathbf u}_i^T {\mathbf v}_j,

    where :math:`\mu` is a global bias term, :math:`w_i` is the weight term for
    user :math:`i`, :math:`w_j` is the weight term for item :math:`j`,
    :math:`\mathbf{x}_i` and :math:`\mathbf{y}_j` are respectively the user and
    item side feature vectors, and :math:`\mathbf{a}` and :math:`\mathbf{b}`
    are respectively the weight vectors for those side features.
    The latent factors, which are vectors of length ``num_factors``, are given
    by :math:`{\mathbf u}_i` and :math:`{\mathbf v}_j`.


    **Training the model**

    The model is trained using Stochastic Gradient Descent with additional
    tricks to improve convergence. The optimization is done in parallel
    over multiple threads. This procedure is inherently random, so different
    calls to `create()` may return slightly different models, even with the
    same `random_seed`.

    In the explicit rating case, the objective function we are
    optimizing for is:

    .. math::
      \min_{\mathbf{w}, \mathbf{a}, \mathbf{b}, \mathbf{V}, \mathbf{U}}
      \frac{1}{|\mathcal{D}|} \sum_{(i,j,r_{ij}) \in \mathcal{D}}
      \mathcal{L}(\operatorname{score}(i, j), r_{ij})
      + \lambda_1 (\lVert {\mathbf w} \rVert^2_2 + || {\mathbf a} ||^2_2 + || {\mathbf b} ||^2_2 )
      + \lambda_2 \left(\lVert {\mathbf U} \rVert^2_2
                           + \lVert {\mathbf V} \rVert^2_2 \right)

    where :math:`\mathcal{D}` is the observation dataset, :math:`r_{ij}` is the
    rating that user :math:`i` gave to item :math:`j`,
    :math:`{\mathbf U} = ({\mathbf u}_1, {\mathbf u}_2, ...)` denotes the user's
    latent factors and :math:`{\mathbf V} = ({\mathbf v}_1, {\mathbf v}_2, ...)`
    denotes the item latent factors.  The loss function
    :math:`\mathcal{L}(\hat{y}, y)` is :math:`(\hat{y} - y)^2` by default.
    :math:`\lambda_1` denotes the `linear_regularization` parameter and
    :math:`\lambda_2` the `regularization` parameter.

    When ``ranking_regularization`` is nonzero, then the equation
    above gets an additional term.  Let :math:`\lambda_{\text{rr}}` represent
    the value of `ranking_regularization`, and let
    :math:`v_{\text{ur}}` represent `unobserved_rating_value`.  Then the
    objective we attempt to minimize is:

    .. math::
      \min_{\mathbf{w}, \mathbf{a}, \mathbf{b}, \mathbf{V}, \mathbf{U}}
      \frac{1}{|\mathcal{D}|} \sum_{(i,j,r_{ij}) \in \mathcal{D}}
      \mathcal{L}(\operatorname{score}(i, j), r_{ij})
      + \lambda_1 (\lVert {\mathbf w} \rVert^2_2 + || {\mathbf a} ||^2_2 + || {\mathbf b} ||^2_2 )
      + \lambda_2 \left(\lVert {\mathbf U} \rVert^2_2
                           + \lVert {\mathbf V} \rVert^2_2 \right) \\
      + \frac{\lambda_{rr}}{\text{const} * |\mathcal{U}|}
      \sum_{(i,j) \in \mathcal{U}}
      \mathcal{L}\left(\operatorname{score}(i, j), v_{\text{ur}}\right),

    where :math:`\mathcal{U}` is a sample of unobserved user-item pairs.

    In the implicit case when there are no target values, we use
    logistic loss to fit a model that attempts to predict all the
    given (user, item) pairs in the training data as 1 and all others
    as 0.  To train this model, we sample an unobserved item along
    with each observed (user, item) pair, using SGD to push the score
    of the observed pair towards 1 and the unobserved pair towards 0.
    In this case, the `ranking_regularization` parameter is ignored.

    When `binary_targets=True`, then the target values must be 0 or 1;
    in this case, we also use logistic loss to train the model so the
    predicted scores are as close to the target values as possible.
    This time, the rating of the sampled unobserved pair is set to 0
    (thus the `unobserved_rating_value` is ignored).  In this case,
    the loss on the unobserved pairs is weighted by
    `ranking_regularization` as in the non-binary case.

    To choose the unobserved pair complementing a given observation,
    the algorithm selects several (defaults to four) candidate
    negative items that the user in the given observation has not
    rated. The algorithm scores each one using the current model, then
    chooses the item with the largest predicted score.  This adaptive
    sampling strategy provides faster convergence than just sampling a
    single negative item.

    The Factorization Machine is a generalization of Matrix
    Factorization. Like matrix factorization, it predicts target
    rating values as a weighted combination of user and item latent
    factors, biases, side features, and their pairwise combinations.
    In particular, while Matrix Factorization learns latent factors
    for only the user and item interactions, the Factorization Machine
    learns latent factors for all variables, including side features,
    and also allows for interactions between all pairs of
    variables. Thus the Factorization Machine is capable of modeling
    complex relationships in the data. Typically, using
    `linear_side_features=True` performs better in terms of RMSE, but
    may require a longer training time.

    num_sampled_negative_examples: For each (user, item) pair in the data, the
    ranking sgd solver evaluates this many randomly chosen unseen items for the
    negative example step.  Increasing this can give better performance at the
    expense of speed, particularly when the number of items is large.


    When `ranking_regularization` is larger than zero, the model samples
    a small set of unobserved user-item pairs and attempts to drive their rating
    predictions below the value specified with `unobserved_rating_value`.
    This has the effect of improving the precision-recall performance of
    recommended items.


    ** Implicit Matrix Factorization**

    `RankingFactorizationRecommender` had an additional option of optimizing
    for ranking using the implicit matrix factorization model. The internal coefficients of
    the model and its interpretation are identical to the model described above.
    The difference between the two models is in the nature in which the objective
    is achieved. Currently, this model does not incorporate any columns
    beyond user/item (and rating) or side data.


    The model works by transferring the raw observations (or weights) (r) into
    two separate magnitudes with distinct interpretations: preferences (p) and
    confidence levels (c). The functional relationship between the weights (r)
    and the confidence is either linear or logarithmic which can be toggled
    by setting `ials_confidence_scaling_type` = `linear` (the default) or `log`
    respectively. The rate of increase of the confidence with respect to the
    weights is proportional to the `ials_confidence_scaling_factor`
    (default 1.0).


    Examples
    --------
    **Basic usage**

    >>> sf = turicreate.SFrame({'user_id': ["0", "0", "0", "1", "1", "2", "2", "2"],
    ...                       'item_id': ["a", "b", "c", "a", "b", "b", "c", "d"],
    ...                       'rating': [1, 3, 2, 5, 4, 1, 4, 3]})
    >>> m1 = turicreate.ranking_factorization_recommender.create(sf, target='rating')

    For implicit data, no target column is specified:

    >>> sf = turicreate.SFrame({'user':  ["0", "0", "0", "1", "1", "2", "2", "2"],
    ...                       'movie': ["a", "b", "c", "a", "b", "b", "c", "d"]})
    >>> m2 = turicreate.ranking_factorization_recommender.create(sf, 'user', 'movie')

    **Implicit Matrix Factorization**

    >>> sf = turicreate.SFrame({'user_id': ["0", "0", "0", "1", "1", "2", "2", "2"],
    ...                       'item_id': ["a", "b", "c", "a", "b", "b", "c", "d"],
    ...                       'rating': [1, 3, 2, 5, 4, 1, 4, 3]})
    >>> m1 = turicreate.ranking_factorization_recommender.create(sf, target='rating',
                                                               solver='ials')

    For implicit data, no target column is specified:

    >>> sf = turicreate.SFrame({'user':  ["0", "0", "0", "1", "1", "2", "2", "2"],
    ...                       'movie': ["a", "b", "c", "a", "b", "b", "c", "d"]})
    >>> m2 = turicreate.ranking_factorization_recommender.create(sf, 'user', 'movie',
                                                               solver='ials')

    **Including side features**

    >>> user_info = turicreate.SFrame({'user_id': ["0", "1", "2"],
    ...                              'name': ["Alice", "Bob", "Charlie"],
    ...                              'numeric_feature': [0.1, 12, 22]})
    >>> item_info = turicreate.SFrame({'item_id': ["a", "b", "c", "d"],
    ...                              'name': ["item1", "item2", "item3", "item4"],
    ...                              'dict_feature': [{'a' : 23}, {'a' : 13},
    ...                                               {'b' : 1},
    ...                                               {'a' : 23, 'b' : 32}]})
    >>> m2 = turicreate.ranking_factorization_recommender.create(sf,
    ...                                               target='rating',
    ...                                               user_data=user_info,
    ...                                               item_data=item_info)

    **Optimizing for ranking performance**

    Create a model that pushes predicted ratings of unobserved user-item
    pairs toward 1 or below.

    >>> m3 = turicreate.ranking_factorization_recommender.create(sf,
    ...                                               target='rating',
    ...                                               ranking_regularization = 0.1,
    ...                                               unobserved_rating_value = 1)
    """

    def __init__(self, model_proxy):
        '''__init__(self)'''
        self.__proxy__ = model_proxy

    @classmethod
    def _native_name(cls):
        return "ranking_factorization_recommender"


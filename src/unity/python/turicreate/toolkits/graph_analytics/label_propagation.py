# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from turicreate.data_structures.sgraph import SGraph as _SGraph
import turicreate.toolkits._main as _main
from turicreate.toolkits.graph_analytics._model_base import GraphAnalyticsModel as _ModelBase
from turicreate.util import _raise_error_if_not_of_type


class LabelPropagationModel(_ModelBase):
    r"""
    A LabelPropagationModel computes the probability of each class label
    for each unlabeled vertex.

    For each labeled vertices, the probability for class k is fixed to:

        .. math::
          Pr_i(label=k) = I(label[i] == k)

    where :math:`I()` is the indicator function.

    For all unlabeled vertices, the probability for each class k is computed
    from applying the following update iteratively:

        .. math::
          Pr_i(label=k) = Pr_i(label=k) * W_0 + \sum_{j\in N(i)} Pr_j(label=k) * W(j,i)

          Pr_i = Normalize(Pr_i)

    where :math:`N(i)` is the set containing all vertices :math:`j` such that
    there is an edge going from :math:`j` to :math:`i`. :math:`W(j,i)` is
    the edge weight from :math:`j` to :math:`i`, and :math:`W_0` is the
    weight for self edge.

    In the above equation, the first term is the probability
    of keeping the label from the previous iteration, and the second term
    is the probability of transition to a neighbor's label.

    Repeated edges (i.e., multiple edges where the source vertices are the same and the
    destination vertices are the same) are treated like normal edges in the
    above recursion.

    By default, the label propagates from source to target. But if `undirected`
    is set to true in :func:`turicreate.label_propagation.create`, then the label
    propagates in both directions for each edge.

    Below is a list of queryable fields for this model:

    +-------------------+-----------------------------------------------------------+
    | Field             | Description                                               |
    +===================+===========================================================+
    | labels            | An SFrame with label probability for each vertex          |
    +-------------------+-----------------------------------------------------------+
    | graph             | A new SGraph with label probability as vertex properties  |
    +-------------------+-----------------------------------------------------------+
    | delta             | Average changes in label probability during the last      |
    |                   | iteration (avg. of the L2 norm of the changes)            |
    +-------------------+-----------------------------------------------------------+
    | num_iterations    | Number of iterations                                      |
    +-------------------+-----------------------------------------------------------+
    | training_time     | Total training time of the model                          |
    +-------------------+-----------------------------------------------------------+
    | threshold         | The convergence threshold in average L2 norm              |
    +-------------------+-----------------------------------------------------------+
    | self_weight       | The weight for self edge                                  |
    +-------------------+-----------------------------------------------------------+
    | weight_field      | The edge weight field id                                  |
    +-------------------+-----------------------------------------------------------+
    | label_field       | The vertex label field id                                 |
    +-------------------+-----------------------------------------------------------+
    | undirected        | Treat edge as undirected                                  |
    +-------------------+-----------------------------------------------------------+

    This model cannot be constructed directly.  Instead, use
    :func:`turicreate.label_propagation.create` to create an instance
    of this model. A detailed list of parameter options and code samples
    are available in the documentation for the create function.

    See Also
    --------
    create
    """
    def __init__(self, model):
        '''__init__(self)'''
        self.__proxy__ = model

    def _result_fields(self):
        ret = super(LabelPropagationModel, self)._result_fields()
        ret["vertex label probability"] = "SFrame. See m['labels']"
        ret['change in last iteration (avg. of L2)'] = self['delta']
        return ret

    def _metric_fields(self):
        ret = super(LabelPropagationModel, self)._metric_fields()
        ret['number of iterations'] = 'num_iterations'
        return ret

    def _setting_fields(self):
        ret = super(LabelPropagationModel, self)._setting_fields()
        ret['convergence threshold (avg. of L2 norm)'] = 'threshold'
        ret['treated edge as undirected'] = 'undirected'
        ret['weight for self edge'] = 'self_weight'
        ret['edge weight field id'] = 'weight_field'
        ret['vertex label field id'] = 'label_field'
        return ret


    def _get_version(self):
        return 0

    @classmethod
    def _native_name(cls):
        return "label_propagation"

    def _get_native_state(self):
        return {'model':self.__proxy__}

    @classmethod
    def _load_version(cls, state, version):
        assert(version == 0)
        return cls(state['model'])



def create(graph, label_field,
           threshold=1e-3,
           weight_field='',
           self_weight=1.0,
           undirected=False,
           max_iterations=None,
           _single_precision=False,
           _distributed='auto',
           verbose=True):
    """
    Given a weighted graph with observed class labels of a subset of vertices,
    infer the label probability for the unobserved vertices using the
    "label propagation" algorithm.

    The algorithm iteratively updates the label probability of current vertex
    as a weighted sum of label probability of self and the neighboring vertices
    until converge.  See
    :class:`turicreate.label_propagation.LabelPropagationModel` for the details
    of the algorithm.

    Notes: label propagation works well with small number of labels, i.e. binary
    labels, or less than 1000 classes. The toolkit will throw error
    if the number of classes exceeds the maximum value (1000).

    Parameters
    ----------
    graph : SGraph
        The graph on which to compute the label propagation.

    label_field: str
        Vertex field storing the initial vertex labels. The values in
        must be [0, num_classes). None values indicate unobserved vertex labels.

    threshold : float, optional
        Threshold for convergence, measured in the average L2 norm
        (the sum of squared values) of the delta of each vertex's
        label probability vector.

    max_iterations: int, optional
        The max number of iterations to run. Default is unlimited.
        If set, the algorithm terminates when either max_iterations
        or convergence threshold is reached.

    weight_field: str, optional
        Vertex field for edge weight. If empty, all edges are assumed
        to have unit weight.

    self_weight: float, optional
        The weight for self edge.

    undirected: bool, optional
        If true, treat each edge as undirected, and propagates label in
        both directions.

    _single_precision : bool, optional
        If true, running label propagation in single precision. The resulting
        probability values may less accurate, but should run faster
        and use less memory.

    _distributed : distributed environment, internal

    verbose : bool, optional
        If True, print progress updates.

    Returns
    -------
    out : LabelPropagationModel

    References
    ----------
    - Zhu, X., & Ghahramani, Z. (2002). `Learning from labeled and unlabeled data
      with label propagation <http://www.cs.cmu.edu/~zhuxj/pub/CMU-CALD-02-107.pdf>`_.

    Examples
    --------
    If given an :class:`~turicreate.SGraph` ``g``, we can create
    a :class:`~turicreate.label_propagation.LabelPropagationModel` as follows:

    >>> g = turicreate.load_sgraph('http://snap.stanford.edu/data/email-Enron.txt.gz',
    ...                         format='snap')
    # Initialize random classes for a subset of vertices
    # Leave the unobserved vertices with None label.
    >>> import random
    >>> def init_label(vid):
    ...     x = random.random()
    ...     if x < 0.2:
    ...         return 0
    ...     elif x > 0.9:
    ...         return 1
    ...     else:
    ...         return None
    >>> g.vertices['label'] = g.vertices['__id'].apply(init_label, int)
    >>> m = turicreate.label_propagation.create(g, label_field='label')

    We can obtain for each vertex the predicted label and the probability of
    each label in the graph ``g`` using:

    >>> labels = m['labels']     # SFrame
    >>> labels
    +------+-------+-----------------+-------------------+----------------+
    | __id | label | predicted_label |         P0        |       P1       |
    +------+-------+-----------------+-------------------+----------------+
    |  5   |   1   |        1        |        0.0        |      1.0       |
    |  7   |  None |        0        |    0.8213214997   |  0.1786785003  |
    |  8   |  None |        1        | 5.96046447754e-08 | 0.999999940395 |
    |  10  |  None |        0        |   0.534984718273  | 0.465015281727 |
    |  27  |  None |        0        |   0.752801638549  | 0.247198361451 |
    |  29  |  None |        1        | 5.96046447754e-08 | 0.999999940395 |
    |  33  |  None |        1        | 5.96046447754e-08 | 0.999999940395 |
    |  47  |   0   |        0        |        1.0        |      0.0       |
    |  50  |  None |        0        |   0.788279032657  | 0.211720967343 |
    |  52  |  None |        0        |   0.666666666667  | 0.333333333333 |
    +------+-------+-----------------+-------------------+----------------+
    [36692 rows x 5 columns]

    See Also
    --------
    LabelPropagationModel
    """
    _raise_error_if_not_of_type(label_field, str)
    _raise_error_if_not_of_type(weight_field, str)

    if not isinstance(graph, _SGraph):
        raise TypeError('graph input must be a SGraph object.')

    if graph.vertices[label_field].dtype != int:
        raise TypeError('label_field %s must be integer typed.' % label_field)

    opts = {'label_field': label_field,
            'threshold': threshold,
            'weight_field': weight_field,
            'self_weight': self_weight,
            'undirected': undirected,
            'max_iterations': max_iterations,
            'single_precision': _single_precision,
            'graph': graph.__proxy__}

    params = _main.run('label_propagation', opts, verbose)
    model = params['model']
    return LabelPropagationModel(model)

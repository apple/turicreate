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


class PagerankModel(_ModelBase):
    """
    A PageRankModel object contains the pagerank value for each vertex.
    The pagerank value characterizes the importance of a vertex
    in the graph using the following recursive definition:

        .. math::
          pr(i) =  reset_probability + (1-reset_probability) \sum_{j\in N(i)} pr(j) / out_degree(j)

    where :math:`N(i)` is the set containing all vertices :math:`j` such that
    there is an edge going from :math:`j` to :math:`i`. Self edges (i.e., edges
    where the source vertex is the same as the destination vertex) and repeated
    edges (i.e., multiple edges where the source vertices are the same and the
    destination vertices are the same) are treated like normal edges in the
    above recursion.

    Currently, edge weights are not taken into account when computing the
    PageRank.

    Below is a list of queryable fields for this model:

    +-------------------+-----------------------------------------------------------+
    | Field             | Description                                               |
    +===================+===========================================================+
    | reset_probability | The probability of random jumps to any node in the graph  |
    +-------------------+-----------------------------------------------------------+
    | graph             | A new SGraph with the pagerank as a vertex property       |
    +-------------------+-----------------------------------------------------------+
    | delta             | Total changes in pagerank during the last iteration       |
    |                   | (the L1 norm of the changes)                              |
    +-------------------+-----------------------------------------------------------+
    | pagerank          | An SFrame with each vertex's pagerank                     |
    +-------------------+-----------------------------------------------------------+
    | num_iterations    | Number of iterations                                      |
    +-------------------+-----------------------------------------------------------+
    | threshold         | The convergence threshold in L1 norm                      |
    +-------------------+-----------------------------------------------------------+
    | training_time     | Total training time of the model                          |
    +-------------------+-----------------------------------------------------------+
    | max_iterations    | The maximum number of iterations to run                   |
    +-------------------+-----------------------------------------------------------+


    This model cannot be constructed directly.  Instead, use
    :func:`turicreate.pagerank.create` to create an instance
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
        ret = super(PagerankModel, self)._result_fields()
        ret["vertex pagerank"] = "SFrame. See m.pagerank"
        ret['change in last iteration (L1 norm)'] = self.delta
        return ret

    def _metric_fields(self):
        ret = super(PagerankModel, self)._metric_fields()
        ret['number of iterations'] = 'num_iterations'
        return ret

    def _setting_fields(self):
        ret = super(PagerankModel, self)._setting_fields()
        ret['probability of random jumps to any node in the graph'] = 'reset_probability'
        ret['convergence threshold (L1 norm)'] = 'threshold'
        ret['maximum number of iterations'] = 'max_iterations'
        return ret

    def _get_version(self):
        return 0

    @classmethod
    def _native_name(cls):
        return "pagerank"

    def _get_native_state(self):
        return {'model':self.__proxy__}

    @classmethod
    def _load_version(cls, state, version):
        assert(version == 0)
        return cls(state['model'])


def create(graph, reset_probability=0.15,
           threshold=1e-2,
           max_iterations=20,
           _single_precision=False,
           _distributed='auto',
           verbose=True):
    """
    Compute the PageRank for each vertex in the graph. Return a model object
    with total PageRank as well as the PageRank value for each vertex in the
    graph.

    Parameters
    ----------
    graph : SGraph
        The graph on which to compute the pagerank value.

    reset_probability : float, optional
        Probability that a random surfer jumps to an arbitrary page.

    threshold : float, optional
        Threshold for convergence, measured in the L1 norm
        (the sum of absolute value) of the delta of each vertex's
        pagerank value.

    max_iterations : int, optional
        The maximum number of iterations to run.

    _single_precision : bool, optional
        If true, running pagerank in single precision. The resulting
        pagerank values may not be accurate for large graph, but
        should run faster and use less memory.

    _distributed : distributed environment, internal

    verbose : bool, optional
        If True, print progress updates.


    Returns
    -------
    out : PagerankModel

    References
    ----------
    - `Wikipedia - PageRank <http://en.wikipedia.org/wiki/PageRank>`_
    - Page, L., et al. (1998) `The PageRank Citation Ranking: Bringing Order to
      the Web <http://ilpubs.stanford.edu:8090/422/1/1999-66.pdf>`_.

    Examples
    --------
    If given an :class:`~turicreate.SGraph` ``g``, we can create
    a :class:`~turicreate.pagerank.PageRankModel` as follows:

    >>> g = turicreate.load_sgraph('http://snap.stanford.edu/data/email-Enron.txt.gz', format='snap')
    >>> pr = turicreate.pagerank.create(g)

    We can obtain the page rank corresponding to each vertex in the graph ``g``
    using:

    >>> pr_out = pr['pagerank']     # SFrame

    We can add the new pagerank field to the original graph g using:

    >>> g.vertices['pagerank'] = pr['graph'].vertices['pagerank']

    Note that the task above does not require a join because the vertex
    ordering is preserved through ``create()``.

    See Also
    --------
    PagerankModel
    """
    if not isinstance(graph, _SGraph):
        raise TypeError('graph input must be a SGraph object.')

    opts = {'threshold': threshold, 'reset_probability': reset_probability,
            'max_iterations': max_iterations,
            'single_precision': _single_precision,
            'graph': graph.__proxy__}

    params = _main.run('pagerank', opts, verbose)
    model = params['model']

    return PagerankModel(model)

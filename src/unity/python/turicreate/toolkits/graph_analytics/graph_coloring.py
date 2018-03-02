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

class GraphColoringModel(_ModelBase):
    """
    A GraphColoringModel object contains color ID assignments for each vertex
    and the total number of colors used in coloring the entire graph.

    The coloring is the result of a greedy algorithm and therefore is not
    optimal.  Finding optimal coloring is in fact NP-complete.

    Below is a list of queryable fields for this model:

    +----------------+-----------------------------------------------------+
    | Field          | Description                                         |
    +================+=====================================================+
    | graph          | A new SGraph with the color id as a vertex property |
    +----------------+-----------------------------------------------------+
    | training_time  | Total training time of the model                    |
    +----------------+-----------------------------------------------------+
    | num_colors     | Number of colors in the graph                       |
    +----------------+-----------------------------------------------------+

    This model cannot be constructed directly.  Instead, use
    :func:`turicreate.graph_coloring.create` to create an instance
    of this model. A detailed list of parameter options and code samples
    are available in the documentation for the create function.


    See Also
    --------
    create
    """
    def __init__(self, model):
        '''__init__(self)'''
        self.__proxy__ = model
        self.__model_name__ = self.__class__._native_name()

    def _result_fields(self):
        ret = super(GraphColoringModel, self)._result_fields()
        ret['number of colors in the graph'] = self.num_colors
        ret['vertex color id'] = "SFrame. See m.color_id"
        return ret

    def _get_version(self):
        return 0

    @classmethod
    def _native_name(cls):
        return "graph_coloring"

    def _get_native_state(self):
        return {'model':self.__proxy__}

    @classmethod
    def _load_version(cls, state, version):
        assert(version == 0)
        return cls(state['model'])


def create(graph, verbose=True):
    """
    Compute the graph coloring. Assign a color to each vertex such that no
    adjacent vertices have the same color. Return a model object with total
    number of colors used as well as the color ID for each vertex in the graph.
    This algorithm is greedy and is not guaranteed to find the **minimum** graph
    coloring. It is also not deterministic, so successive runs may return
    different answers.

    Parameters
    ----------
    graph : SGraph
        The graph on which to compute the coloring.

    verbose : bool, optional
        If True, print progress updates.

    Returns
    -------
    out : GraphColoringModel

    References
    ----------
    - `Wikipedia - graph coloring <http://en.wikipedia.org/wiki/Graph_coloring>`_

    Examples
    --------
    If given an :class:`~turicreate.SGraph` ``g``, we can create
    a :class:`~turicreate.graph_coloring.GraphColoringModel` as follows:

    >>> g = turicreate.load_sgraph('http://snap.stanford.edu/data/email-Enron.txt.gz', format='snap')
    >>> gc = turicreate.graph_coloring.create(g)

    We can obtain the ``color id`` corresponding to each vertex in the graph ``g``
    as follows:

    >>> color_id = gc['color_id']  # SFrame

    We can obtain the total number of colors required to color the graph ``g``
    as follows:

    >>> num_colors = gc['num_colors']

    See Also
    --------
    GraphColoringModel
    """
    if not isinstance(graph, _SGraph):
        raise TypeError('graph input must be a SGraph object.')

    params = _main.run('graph_coloring', {'graph': graph.__proxy__}, verbose)
    return GraphColoringModel(params['model'])

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


class ConnectedComponentsModel(_ModelBase):
    r"""
    A ConnectedComponentsModel object contains the component ID for each vertex
    and the total number of weakly connected components in the graph.

    A weakly connected component is a maximal set of vertices such that there
    exists an undirected path between any two vertices in the set.

    Below is a list of queryable fields for this model:

    +----------------+-----------------------------------------------------+
    | Field          | Description                                         |
    +================+=====================================================+
    | graph          | A new SGraph with the color id as a vertex property |
    +----------------+-----------------------------------------------------+
    | training_time  | Total training time of the model                    |
    +----------------+-----------------------------------------------------+
    | component_size | An SFrame with the size of each component           |
    +----------------+-----------------------------------------------------+
    | component_id   | An SFrame with each vertex's component id           |
    +----------------+-----------------------------------------------------+

    This model cannot be constructed directly.  Instead, use
    :func:`turicreate.connected_components.create` to create an instance
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

    def _get_version(self):
        return 0

    @classmethod
    def _native_name(cls):
        return "connected_components"

    def _get_native_state(self):
        return {'model':self.__proxy__}

    @classmethod
    def _load_version(cls, state, version):
        assert(version == 0)
        return cls(state['model'])

    def _result_fields(self):
        ret = super(ConnectedComponentsModel, self)._result_fields()
        ret["number of connected components"] = len(self['component_size'])
        ret["component size"] = "SFrame. See m['component_size']"
        ret["vertex component id"] = "SFrame. See m['component_id']"
        return ret


def create(graph, verbose=True):
    """
    Compute the number of weakly connected components in the graph. Return a
    model object with total number of weakly connected components as well as the
    component ID for each vertex in the graph.

    Parameters
    ----------
    graph : SGraph
        The graph on which to compute the triangle counts.

    verbose : bool, optional
        If True, print progress updates.

    Returns
    -------
    out : ConnectedComponentsModel

    References
    ----------
    - `Mathworld Wolfram - Weakly Connected Component
      <http://mathworld.wolfram.com/WeaklyConnectedComponent.html>`_

    Examples
    --------
    If given an :class:`~turicreate.SGraph` ``g``, we can create
    a :class:`~turicreate.connected_components.ConnectedComponentsModel` as
    follows:

    >>> g = turicreate.load_sgraph('http://snap.stanford.edu/data/email-Enron.txt.gz', format='snap')
    >>> cc = turicreate.connected_components.create(g)
    >>> cc.summary()

    We can obtain the ``component id`` corresponding to each vertex in the
    graph ``g`` as follows:

    >>> cc_ids = cc['component_id']  # SFrame

    We can obtain a graph with additional information about the ``component
    id`` corresponding to each vertex as follows:

    >>> cc_graph = cc['graph']      # SGraph

    We can add the new component_id field to the original graph g using:

    >>> g.vertices['component_id'] = cc['graph'].vertices['component_id']

    Note that the task above does not require a join because the vertex
    ordering is preserved through ``create()``.


    See Also
    --------
    ConnectedComponentsModel
    """
    if not isinstance(graph, _SGraph):
        raise TypeError('graph input must be a SGraph object.')

    params = _main.run('connected_components', {'graph': graph.__proxy__},
                       verbose)
    return ConnectedComponentsModel(params['model'])

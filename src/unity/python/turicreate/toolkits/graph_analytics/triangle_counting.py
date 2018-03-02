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

class TriangleCountingModel(_ModelBase):
    """
    Model object containing the triangle count for each vertex, and the total
    number of triangles. The model ignores the edge directions in that
    it assumes there are no multiple edges between
    the same source ang target pair and ignores bidirectional edges.

    The triangle count of individual vertex characterizes the importance of the
    vertex in its neighborhood. The total number of triangles characterizes the
    density of the graph. It can also be calculated using

    >>> m['triangle_count']['triangle_count'].sum() / 3.

    Below is a list of queryable fields for this model:

    +---------------+------------------------------------------------------------+
    | Field         | Description                                                |
    +===============+============================================================+
    | triangle_count| An SFrame with each vertex's id and triangle count         |
    +---------------+------------------------------------------------------------+
    | num_triangles | Total number of triangles in the graph                     |
    +---------------+------------------------------------------------------------+
    | graph         | A new SGraph with the triangle count as a vertex property  |
    +---------------+------------------------------------------------------------+
    | training_time | Total training time of the model                           |
    +---------------+------------------------------------------------------------+

    This model cannot be constructed directly.  Instead, use
    :func:`turicreate.triangle_counting.create` to create an instance
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
        ret = super(TriangleCountingModel, self)._result_fields()
        ret['total number of triangles'] = self.num_triangles
        ret["vertex triangle count"] = "SFrame. See m.triangle_count"
        return ret

    def _get_version(self):
        return 0

    @classmethod
    def _native_name(cls):
        return "triangle_count"

    def _get_native_state(self):
        return {'model':self.__proxy__}

    @classmethod
    def _load_version(cls, state, version):
        assert(version == 0)
        return cls(state['model'])


def create(graph, verbose=True):
    """
    Compute the number of triangles each vertex belongs to, ignoring edge
    directions. A triangle is a complete subgraph with only three vertices.
    Return a model object with total number of triangles as well as the triangle
    counts for each vertex in the graph.

    Parameters
    ----------
    graph : SGraph
        The graph on which to compute triangle counts.

    verbose : bool, optional
        If True, print progress updates.

    Returns
    -------
    out : TriangleCountingModel

    References
    ----------
    - T. Schank. (2007) `Algorithmic Aspects of Triangle-Based Network Analysis
      <http://digbib.ubka.uni-karlsruhe.de/volltexte/documents/4541>`_.

    Examples
    --------
    If given an :class:`~turicreate.SGraph` ``g``, we can create a
    :class:`~turicreate.triangle_counting.TriangleCountingModel` as follows:

    >>> g =
    >>> turicreate.load_sgraph('http://snap.stanford.edu/data/email-Enron.txt.gz',
            >>> format='snap') tc = turicreate.triangle_counting.create(g)

    We can obtain the number of triangles that each vertex in the graph ``g``
    is present in:

    >>> tc_out = tc['triangle_count']  # SFrame

    We can add the new "triangle_count" field to the original graph g using:

    >>> g.vertices['triangle_count'] = tc['graph'].vertices['triangle_count']

    Note that the task above does not require a join because the vertex
    ordering is preserved through ``create()``.

    See Also
    --------
    TriangleCountingModel
    """
    if not isinstance(graph, _SGraph):
        raise TypeError('graph input must be a SGraph object.')

    params = _main.run('triangle_counting', {'graph': graph.__proxy__}, verbose)
    return TriangleCountingModel(params['model'])

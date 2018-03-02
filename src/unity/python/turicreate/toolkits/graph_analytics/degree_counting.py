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


class DegreeCountingModel(_ModelBase):
    """
    Model object containing the in degree, out degree and total degree for each vertex,

    Below is a list of queryable fields for this model:

    +---------------+------------------------------------------------------------+
    | Field         | Description                                                |
    +===============+============================================================+
    | graph         | A new SGraph with the degree counts as vertex properties   |
    +---------------+------------------------------------------------------------+
    | training_time | Total training time of the model                           |
    +---------------+------------------------------------------------------------+

    This model cannot be constructed directly.  Instead, use
    :func:`turicreate.degree_counting.create` to create an instance
    of this model. A detailed list of parameter options and code samples
    are available in the documentation for the create function.

    See Also
    --------
    create
    """
    def __init__(self, model):
        '''__init__(self)'''
        self.__proxy__ = model

    def _get_version(self):
        return 0

    @classmethod
    def _native_name(cls):
        return "degree_count"

    def _get_native_state(self):
        return {'model':self.__proxy__}

    @classmethod
    def _load_version(cls, state, version):
        assert(version == 0)
        return cls(state['model'])

def create(graph, verbose=True):
    """
    Compute the in degree, out degree and total degree of each vertex.

    Parameters
    ----------
    graph : SGraph
        The graph on which to compute degree counts.

    verbose : bool, optional
        If True, print progress updates.

    Returns
    -------
    out : DegreeCountingModel

    Examples
    --------
    If given an :class:`~turicreate.SGraph` ``g``, we can create
    a :class:`~turicreate.degree_counting.DegreeCountingModel` as follows:

    >>> g = turicreate.load_sgraph('http://snap.stanford.edu/data/web-Google.txt.gz',
    ...                         format='snap')
    >>> m = turicreate.degree_counting.create(g)
    >>> g2 = m['graph']
    >>> g2
    SGraph({'num_edges': 5105039, 'num_vertices': 875713})
    Vertex Fields:['__id', 'in_degree', 'out_degree', 'total_degree']
    Edge Fields:['__src_id', '__dst_id']

    >>> g2.vertices.head(5)
    Columns:
        __id	int
        in_degree	int
        out_degree	int
        total_degree	int
    <BLANKLINE>
    Rows: 5
    <BLANKLINE>
    Data:
    +------+-----------+------------+--------------+
    | __id | in_degree | out_degree | total_degree |
    +------+-----------+------------+--------------+
    |  5   |     15    |     7      |      22      |
    |  7   |     3     |     16     |      19      |
    |  8   |     1     |     2      |      3       |
    |  10  |     13    |     11     |      24      |
    |  27  |     19    |     16     |      35      |
    +------+-----------+------------+--------------+

    See Also
    --------
    DegreeCountingModel
    """
    if not isinstance(graph, _SGraph):
        raise TypeError('graph input must be a SGraph object.')

    params = _main.run('degree_count', {'graph': graph.__proxy__}, verbose)
    return DegreeCountingModel(params['model'])

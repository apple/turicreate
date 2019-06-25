# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
The Turi Create nearest neighbors toolkit finds the rows in a tabular
reference dataset that are most similar to a set of queries with the same
schema.

A :py:class:`~turicreate.nearest_neighbors.NearestNeighborsModel` is created with
a reference dataset contained in an :class:`~turicreate.SFrame`, a distance
function, and an indexing method (the latter two options can be done
automatically by the model). An instantiated model has two key methods:
**query**, for finding the closest points in the reference dataset to *new* data
points; and **similarity_graph**, for finding the nearest neighbors of each
point in the original reference set.

.. sourcecode:: python

    >>> references = turicreate.SFrame({'x1': [0.98, 0.62, 0.11],
    ...                               'x2': [0.69, 0.58, 0.36]})
    >>> references.print_rows()
    +------+------+
    |  x1  |  x2  |
    +------+------+
    | 0.98 | 0.69 |
    | 0.62 | 0.58 |
    | 0.11 | 0.36 |
    +------+------+
    [3 rows x 2 columns]
    ...
    >>> model = turicreate.nearest_neighbors.create(references)
    ...
    >>> sim_graph = model.similarity_graph(k=1)
    >>> sim_graph.edges
    +----------+----------+----------------+------+
    | __src_id | __dst_id |    distance    | rank |
    +----------+----------+----------------+------+
    |    0     |    1     | 0.376430604494 |  1   |
    |    2     |    1     | 0.55542776308  |  1   |
    |    1     |    0     | 0.376430604494 |  1   |
    +----------+----------+----------------+------+
    ...
    >>> queries = turicreate.SFrame({'x1': [0.05, 0.61, 0.99],
    ...                            'x2': [0.06, 0.97, 0.86]})
    >>> queries.print_rows()
    +------+------+
    |  x1  |  x2  |
    +------+------+
    | 0.05 | 0.06 |
    | 0.61 | 0.97 |
    | 0.99 | 0.86 |
    +------+------+
    [3 rows x 2 columns]
    ...
    >>> model.query(queries, k=2)
    +-------------+-----------------+----------------+------+
    | query_label | reference_label |    distance    | rank |
    +-------------+-----------------+----------------+------+
    |      0      |        2        | 0.305941170816 |  1   |
    |      0      |        1        | 0.771556867638 |  2   |
    |      1      |        1        | 0.390128184063 |  1   |
    |      1      |        0        | 0.464004310325 |  2   |
    |      2      |        0        | 0.170293863659 |  1   |
    |      2      |        1        | 0.464004310325 |  2   |
    +-------------+-----------------+----------------+------+

In addition to the API documentation, please see the `nearest neighbors chapter of the User Guide
<https://apple.github.io/turicreate/docs/userguide/nearest_neighbors/nearest_neighbors.html>`_
for more details and extended examples.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from ._nearest_neighbors import create
from ._nearest_neighbors import NearestNeighborsModel

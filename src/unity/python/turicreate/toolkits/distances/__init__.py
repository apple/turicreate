# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
The Turi Create distances module provides access to the standard distance
functions and utilities for working with composite distances. Distance functions
are used in all toolkits based on a nearest neighbors search, including
:mod:`~turicreate.toolkits.nearest_neighbors` itself,
:mod:`nearest_neighbor_classifier` , and :mod:`nearest_neighbor_deduplication`.

    .. warning::

        The 'dot_product' distance is deprecated and will be removed in future
        versions of Turi Create. Please use 'logistic' distance instead,
        although note that this is more than a name change; it is a *different*
        transformation of the dot product of two vectors. Please see the
        distances module documentation for more details.

**Standard distance functions** measure the dissimilarity between two data
points consisting of only a single type.

    - *euclidean*, *squared_euclidean*, *manhattan*, *cosine*, and
      *transformed_dot_product* distances work for integer and floating point
      data, which can be thought of a vectors.

    - These distances, as well as the *jaccard* and *weighted_jaccard* distances
      work on data contained in dictionaries.

    - The *levenshtein* distance works for string data, although another
      strategy that often works well is to turn strings into dictionaries with
      the :py:func:`turicreate.text_analytics.count_ngrams` function then use
      Jaccard or weighted Jaccard distance.

These functions may be passed to a model by specifying either the name or the
handle of the function in this module. For example, suppose we have the
following SFrame of data:

>>> sf = turicreate.SFrame({'X1': [0.98, 0.62, 0.11, 1.4, 0.88],
...                       'X2': [0.69, 0.58, 0.36, 1.23, 0.2],
...                       'species': ['cat', 'dog', 'elephant', 'fossa', 'giraffe']})

To find the nearest neighbors of each row, we create a nearest neighbors model,
and we have to indicate how we want to measure the distance between any pair of
rows. Suppose we only want to use the numeric features 'X1' and 'X2'; then we
can use any of the standard numeric distances.

>>> m = turicreate.nearest_neighbors.create(sf, features=['X1', 'X2'],
...                                       distance='euclidean')
...
>>> m2 = turicreate.nearest_neighbors.create(sf, features=['X1', 'X2'],
...                                        distance=turicreate.distances.euclidean)

**Composite distances** provide greater flexibility because they allow distances
on features that have *different* types. A composite distance is simply a
weighted sum of standard distance functions, each of which is applied to a
particular subset of features. To represent this in code, we use a Python list.
Each member of a composite distance list contains three things:

    1. a list or tuple of feature names
    2. the name of a standard distance function
    3. a weight

The weight is a single scalar value (integer or float) that multiplies the
contribution of each component of the distance.

For a concrete example, suppose we want to measure the distance between two rows
:math:`a` and :math:`b` in the SFrame above using a combination of Euclidean
distance on the numeric features and Levenshtein distance on the species name.
To increase the relative contribution of the numeric features we can up-weight
the Euclidean distance by a factor of 2, and down-weight the Levenshtein
distance by a factor of 0.3. Our composite distance is

.. math::

    D(a, b) = 2 * d_{euclidean}(a[X1, X2], b[X1, X2])
            + 0.3 * d_{levenshtein}(a[species], b[species])

This is represented in Python code as:

>>> species_dist = [[('X1', 'X2'), 'euclidean', 2],
...                 [('species',), 'levenshtein', 0.3]]

Composite distances can be used with the following models as a drop-in
replacement for the standard distance name or function.

    - :py:class:`~.nearest_neighbors.NearestNeighborsModel`
    - :py:class:`~.nearest_neighbor_classifier.NearestNeighborClassifier`
    - :py:class:`~.dbscan.DBSCANModel`
    - :py:class:`~.data_matching.nearest_neighbor_deduplication.NearestNeighborDeduplication`
    - :py:class:`~.data_matching.record_linker.RecordLinker`

When a composite distance is used, we no longer need to specify the features,
because the composite distance already contains that information. Models that
use composite distances store the specification so it can be retrieved,
modified, and reused. For example, suppose we decided that the Levenshtein
distance on species name should have a higher weight. We don't have to construct
a composite distance from scratch; we can modify the one we used previously.

>>> m3 = turicreate.nearest_neighbors.create(sf, distance=dist_spec)
...
>>> dist_spec2 = m3['composite_params']
>>> dist_spec2[1][2] = 0.7
>>> m4 = turicreate.nearest_neighbors.create(sf, distance=dist_spec2)

Specifying a composite distance can be tricky. Often we have a general sense for
which features and standard distances to use, but only a vague idea how much
each component should be weighted. The `compute_composite_distance` function can
help with this by evaluating a composite distance on two specific data points.

>>> d1 = turicreate.distances.compute_composite_distance(dist_spec, sf[0], sf[1])
>>> d2 = turicreate.distances.compute_composite_distance(dist_spec, sf[0], sf[2])
>>> print "d1:", d1, "d2:", d2
d1: 1.65286120899 d2: 3.66096749031

This tells that under our first composite distance, the 'cat' and 'dog' data
points are closer than the 'cat' and 'elephant' data points.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

__all__ = ['_distances', '_util']

from ._distances import euclidean, squared_euclidean, manhattan
from ._distances import cosine, dot_product, transformed_dot_product, jaccard, weighted_jaccard, gaussian_kernel
from ._distances import levenshtein

from . import _util
from ._util import compute_composite_distance
from ._util import build_address_distance

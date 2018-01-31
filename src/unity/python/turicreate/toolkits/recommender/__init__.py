# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
The Turi Create recommender toolkit provides a unified interface to train a
variety of recommender models and use them to make recommendations.

Recommender models can be created using :meth:`turicreate.recommender.create` or
loaded from a previously saved model using :meth:`turicreate.load_model`. The
input data must be an SFrame with a column containing user ids, a column
containing item ids, and optionally a column containing target values such as
movie ratings, etc. When a target is not provided (as is the case in implicit
feedback settings), then a collaborative filtering model based on item-item
similarity is returned. For more details, please see the documentation for
:meth:`turicreate.recommender.create`.

A recommender model object can perform key tasks including
`predict`, `recommend`, `evaluate`, and `save`. Model attributes and
statistics may be obtained via `m.get()`, where `m` is a model object. In
particular, trained model parameters may be accessed using
`m.get('coefficients')` or equivalently `m['coefficients']`. For more details,
please see individual model API documentation below.

In addition to the API documentation, please see the `recommender systems chapter of the
User Guide
<https://apple.github.io/turicreate/docs/userguide/recommender/README.html>`_
for more details and extended examples.

.. sourcecode:: python

  >>> sf = turicreate.SFrame({'user_id': ["0", "0", "0", "1", "1", "2", "2", "2"],
  ...                       'item_id': ["a", "b", "c", "a", "b", "b", "c", "d"],
  ...                       'rating': [1, 3, 2, 5, 4, 1, 4, 3]})
  >>> m = turicreate.recommender.create(sf, target='rating')
  >>> recs = m.recommend()
  >>> print recs
  +---------+---------+---------------+------+
  | user_id | item_id |     score     | rank |
  +---------+---------+---------------+------+
  |    0    |    d    | 2.42301885789 |  1   |
  |    1    |    c    | 5.52301720893 |  1   |
  |    1    |    d    | 5.20882169849 |  2   |
  |    2    |    a    |  2.149379798  |  1   |
  +---------+---------+---------------+------+
  [4 rows x 4 columns]

  >>> m['coefficients']
  {'intercept': 3.1321961361684068, 'item_id': Columns:
    item_id str
    linear_terms  float
    factors array

   Rows: 4

   Data:
   +---------+-----------------+--------------------------------+
   | item_id |   linear_terms  |            factors             |
   +---------+-----------------+--------------------------------+
   |    a    | -0.381912890376 | array('d', [0.006779233276 ... |
   |    b    | -0.482275197699 | array('d', [-3.57188659440 ... |
   |    c    |  0.664901063905 | array('d', [-0.00025265078 ... |
   |    d    |  0.352987048665 | array('d', [-0.00197509767 ... |
   +---------+-----------------+--------------------------------+
   [4 rows x 3 columns], 'user_id': Columns:
    user_id str
    linear_terms  float
    factors array

   Rows: 3

   Data:
   +---------+-----------------+--------------------------------+
   | user_id |   linear_terms  |            factors             |
   +---------+-----------------+--------------------------------+
   |    0    |  -1.06188402031 | array('d', [-0.00321943390 ... |
   |    1    |  1.72356956865  | array('d', [0.005337682218 ... |
   |    2    | -0.604970370745 | array('d', [-0.00274082382 ... |
   +---------+-----------------+--------------------------------+
   [3 rows x 3 columns]}
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

__all__ = ['popularity_recommender',
           'factorization_recommender',
           'ranking_factorization_recommender',
           'item_similarity_recommender',
           'create',
           'util']

from . import popularity_recommender
from . import factorization_recommender
from . import ranking_factorization_recommender
from . import item_similarity_recommender
from . import item_content_recommender
from . import util
from .util import _create as create

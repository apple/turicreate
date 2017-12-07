# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
r"""
The Turi Create regression toolkit contains models for regression problems.
Currently, we support linear regression and boosted trees. In addition to these
models, we provide a smart interface that selects the right model based on the
data. If you are unsure about which model to use, simply use
:meth:`~turicreate.regression.create` function.

Training data must contain a column for the 'target' variable and one or more
columns representing feature variables.

.. sourcecode:: python

    # Set up the data
    >>> import turicreate as tc
    >>> data =  tc.SFrame('https://static.turi.com/datasets/regression/houses.csv')

    # Select the best model based on your data.
    >>> model = tc.regression.create(data, target='price',
    ...                                  features=['bath', 'bedroom', 'size'])

    # Make predictions and evaluate results.
    >>> predictions = model.predict(data)
    >>> results = model.evaluate(data)

"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ._regression import create
from . import linear_regression
from . import boosted_trees_regression
from . import random_forest_regression
from . import decision_tree_regression

# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
The Turi Create clustering toolkit provides tools for unsupervised learning
tasks, where the aim is to consolidate unlabeled data points into groups. Points
that are similar to each other should be assigned to the same group and points
that are different should be assigned to different groups.

The clustering toolkit contains two models: K-Means and DBSCAN.

Please see the documentation for each of these models for more details, as well
as the `clustering chapter of the User Guide
<https://apple.github.io/turicreate/docs/userguide/clustering/README.html>`_.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

__all__ = ['kmeans', 'dbscan']

from . import kmeans
from . import dbscan

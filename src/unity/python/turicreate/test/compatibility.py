# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as tc
import array
import numpy as np
import os as _os

_lfs = _os.environ['LFS_ROOT']
s3bucket = _os.path.join(_lfs, 'gl-internal', 'models')

def create_nearest_neighbor_data():
    ## Make data
    np.random.seed(19)
    n, d = 108, 3

    array_features = []
    dict_features = []
    for i in range(n):
        array_features.append(array.array('f', np.random.rand(d)))
        dict_features.append({'alice': np.random.randint(10),
          'brian': np.random.randint(10),
          'chris': np.random.randint(10)})

        refs = tc.SFrame()
    for i in range(d):
        refs['X{}'.format(i+1)] = tc.SArray(np.random.rand(n))

    label = 'label'
    refs[label] = [str(x) for x in xrange(n)]
    refs['array_ftr'] = array_features
    return refs


if tc.version == '1.0.1':

    # Create nearest neighbors model
    data = create_nearest_neighbor_data()
    m = tc.nearest_neighbors.create(data, label='label')
    m.save(s3bucket + '/1.0.1/nearest_neighbors.gl')

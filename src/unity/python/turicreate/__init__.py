# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
@package turicreate
...
Turi Create is a machine learning platform that enables data scientists and app
developers to easily create intelligent applications at scale.
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

__version__ = '{{VERSION_STRING}}'
from turicreate.version_info import __version__

from turicreate.data_structures.sgraph import Vertex, Edge
from turicreate.data_structures.sgraph import SGraph
from turicreate.data_structures.sarray import SArray
from turicreate.data_structures.sframe import SFrame
from turicreate.data_structures.sketch import Sketch
from turicreate.data_structures.image import Image
from .data_structures.sarray_builder import SArrayBuilder
from .data_structures.sframe_builder import SFrameBuilder

from turicreate.data_structures.sgraph import load_sgraph

import turicreate.aggregate
import turicreate.toolkits
import turicreate.toolkits.clustering as clustering
import turicreate.toolkits.distances as distances

import turicreate.toolkits.text_analytics as text_analytics
import turicreate.toolkits.graph_analytics as graph_analytics

from turicreate.toolkits.graph_analytics import connected_components
from turicreate.toolkits.graph_analytics import shortest_path
from turicreate.toolkits.graph_analytics import kcore
from turicreate.toolkits.graph_analytics import pagerank
from turicreate.toolkits.graph_analytics import graph_coloring
from turicreate.toolkits.graph_analytics import triangle_counting
from turicreate.toolkits.graph_analytics import degree_counting
from turicreate.toolkits.graph_analytics import label_propagation

import turicreate.toolkits.recommender as recommender
from turicreate.toolkits.recommender import popularity_recommender
from turicreate.toolkits.recommender import item_similarity_recommender
from turicreate.toolkits.recommender import ranking_factorization_recommender
from turicreate.toolkits.recommender import item_content_recommender
from turicreate.toolkits.recommender import factorization_recommender

import turicreate.toolkits.regression as regression
from turicreate.toolkits.regression import boosted_trees_regression
from turicreate.toolkits.regression import random_forest_regression
from turicreate.toolkits.regression import decision_tree_regression
from turicreate.toolkits.regression import linear_regression

import turicreate.toolkits.classifier as classifier
from turicreate.toolkits.classifier import svm_classifier
from turicreate.toolkits.classifier import logistic_classifier
from turicreate.toolkits.classifier import boosted_trees_classifier
from turicreate.toolkits.classifier import random_forest_classifier
from turicreate.toolkits.classifier import decision_tree_classifier
from turicreate.toolkits.classifier import nearest_neighbor_classifier


import turicreate.toolkits.nearest_neighbors as nearest_neighbors
from turicreate.toolkits.clustering import kmeans
from turicreate.toolkits.clustering import dbscan
from turicreate.toolkits.topic_model import topic_model

from turicreate.toolkits.image_analysis import image_analysis
import turicreate.toolkits.sentence_classifier as sentence_classifier
import turicreate.toolkits.image_classifier as image_classifier
import turicreate.toolkits.image_similarity as image_similarity
import turicreate.toolkits.object_detector as object_detector
import turicreate.toolkits.activity_classifier as activity_classifier

from turicreate.toolkits import evaluation

# internal util
from turicreate.connect.main import launch as _launch
import turicreate.connect.main as glconnect

## bring load functions to the top level
from turicreate.data_structures.sframe import load_sframe
from turicreate.toolkits._model import load_model, Model
from .cython import cy_pylambda_workers

################### Extension Importing ########################
import turicreate.extensions
from turicreate.extensions import ext_import

turicreate.extensions._add_meta_path()

# rewrite the extensions module
class _extensions_wrapper(object):
  def __init__(self, wrapped):
    self._wrapped = wrapped
    self.__doc__ = wrapped.__doc__

  def __getattr__(self, name):
    try:
        return getattr(self._wrapped, name)
    except:
        pass
    turicreate.connect.main.get_unity()
    return getattr(self._wrapped, name)

import sys as _sys
_sys.modules["turicreate.extensions"] = _extensions_wrapper(_sys.modules["turicreate.extensions"])
# rewrite the import
extensions = _sys.modules["turicreate.extensions"]


def _mxnet_check():
    try:
        import mxnet as _mx
        version_tuple = tuple(int(x) for x in _mx.__version__.split('.') if x.isdigit())
        lowest_version = (0, 11, 0)
        not_yet_supported_version = (1, 0, 0)
        recommended_version_str = '0.12.1'
        if not (lowest_version <= version_tuple < not_yet_supported_version):
            print('WARNING: You are using MXNet', _mx.__version__, 'which may result in breaking behavior.')
            print('         To fix this, please install the currently recommended version:')
            print()
            print('             pip uninstall -y mxnet && pip install mxnet==%s' % recommended_version_str)
            print()
            print("         If you want to use a CUDA GPU, then change 'mxnet' to 'mxnet-cu90' (adjust 'cu90' depending on your CUDA version):")
            print()
    except (ImportError, OSError):
        pass

_mxnet_check()

from .visualization import show

_launch()

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

from ._deps import LazyModuleLoader as _LazyModuleLoader
from ._deps import LazyCallable as _LazyCallable

__version__ = '{{VERSION_STRING}}'
from turicreate.version_info import __version__

from turicreate.toolkits import evaluation
# expose name aggregate as global variable
# this won't interfere with import turicreate.aggregate as agg
aggregate = _LazyModuleLoader('turicreate.aggregate')
toolkits =  _LazyModuleLoader('turicreate.toolkits')

# expose name config as a global variable
config =  _LazyModuleLoader('turicreate.config')

# must load
from turicreate.data_structures.sgraph import Vertex, Edge
from turicreate.data_structures.sgraph import SGraph
from turicreate.data_structures.sarray import SArray
from turicreate.data_structures.sframe import SFrame
from turicreate.data_structures.sketch import Sketch
from turicreate.data_structures.image import Image
from .data_structures.sarray_builder import SArrayBuilder
from .data_structures.sframe_builder import SFrameBuilder

## bring load functions to the top level
from turicreate.data_structures.sgraph import load_sgraph
from turicreate.data_structures.sframe import load_sframe
from turicreate.data_structures.sarray import load_sarray

# lazy import under leaf module level
import turicreate.toolkits.clustering as clustering
from turicreate.toolkits.clustering import kmeans
from turicreate.toolkits.clustering import dbscan

# lazy import under leaf module level
import turicreate.toolkits.graph_analytics as graph_analytics
from turicreate.toolkits.graph_analytics import connected_components
from turicreate.toolkits.graph_analytics import shortest_path
from turicreate.toolkits.graph_analytics import kcore
from turicreate.toolkits.graph_analytics import pagerank
from turicreate.toolkits.graph_analytics import graph_coloring
from turicreate.toolkits.graph_analytics import triangle_counting
from turicreate.toolkits.graph_analytics import degree_counting
from turicreate.toolkits.graph_analytics import label_propagation

# lazy import under leaf module level
import turicreate.toolkits.recommender as recommender
from turicreate.toolkits.recommender import popularity_recommender
from turicreate.toolkits.recommender import item_similarity_recommender
from turicreate.toolkits.recommender import ranking_factorization_recommender
from turicreate.toolkits.recommender import item_content_recommender
from turicreate.toolkits.recommender import factorization_recommender

# lazy load under leaf node level
import turicreate.toolkits.regression as regression
from turicreate.toolkits.regression import boosted_trees_regression
from turicreate.toolkits.regression import random_forest_regression
from turicreate.toolkits.regression import decision_tree_regression
from turicreate.toolkits.regression import linear_regression

# lazy load under leaf node level
import turicreate.toolkits.classifier as classifier
from turicreate.toolkits.classifier import svm_classifier
from turicreate.toolkits.classifier import logistic_classifier
from turicreate.toolkits.classifier import boosted_trees_classifier
from turicreate.toolkits.classifier import random_forest_classifier
from turicreate.toolkits.classifier import decision_tree_classifier
from turicreate.toolkits.classifier import nearest_neighbor_classifier

# lazy load under leaf node level
from turicreate.toolkits.image_analysis import image_analysis

# self-encaps modules without from import statements
# we can use top-level lazy import for them
distances = _LazyModuleLoader('turicreate.toolkits.distances')
nearest_neighbors = _LazyModuleLoader('turicreate.toolkits.nearest_neighbors')
topci_model = _LazyModuleLoader('turicreate.toolkits.topci_model')
text_analytics = _LazyModuleLoader('turicreate.toolkits.text_analytics')
text_classifier = _LazyModuleLoader('turicreate.toolkits.text_classifier')
image_classifier = _LazyModuleLoader('turicreate.toolkits.image_classifier')
image_similarity = _LazyModuleLoader('turicreate.toolkits.image_similarity')
object_detector = _LazyModuleLoader('turicreate.toolkits.object_detector')
one_shot_object_detector = _LazyModuleLoader('turicreate.toolkits.one_shot_object_detector')
style_transfer = _LazyModuleLoader('turicreate.toolkits.style_transfer')
activity_classifier = _LazyModuleLoader('turicreate.toolkits.activity_classifier')
drawing_classifier = _LazyModuleLoader('turicreate.toolkits.drawing_classifier')

# modules that don't expose attributes from __init__.py
sound_classifier = _LazyModuleLoader('turicreate.toolkits.sound_classifier.sound_classifier')
audio_analysis = _LazyModuleLoader('turicreate.toolkits.audio_analysis.audio_analysis')
# lazy callable
load_images = _LazyCallable(image_analysis, 'load_images')
load_audio = _LazyCallable(audio_analysis, 'load_audio')

load_model = _LazyCallable(_LazyModuleLoader(
    'turicreate.toolkits._model'), 'load_model')

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
    turicreate._connect.main.get_unity()
    return getattr(self._wrapped, name)

import sys as _sys
_sys.modules["turicreate.extensions"] = _extensions_wrapper(_sys.modules["turicreate.extensions"])
# rewrite the import
extensions = _sys.modules["turicreate.extensions"]

from .visualization import plot, show

# internal util
from turicreate._connect.main import launch as _launch
_launch()

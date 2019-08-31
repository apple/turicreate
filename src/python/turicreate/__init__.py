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

#### lazy importing all rarely used pkgs ###
from turicreate._deps import LazyModuleLoader
LazyModuleLoader('requests')
LazyModuleLoader('prettytable.PrettyTable')
########### end of lazy import #############

__version__ = '{{VERSION_STRING}}'
from turicreate.version_info import __version__

# well, imports are order dependent
from turicreate.data_structures.sgraph import Vertex, Edge
from turicreate.data_structures.sgraph import load_sgraph
SGraph = LazyModuleLoader('turicreate.data_structures.sgraph')

from turicreate.data_structures.sarray import SArray
from turicreate.data_structures.sframe import SFrame
Sketch = LazyModuleLoader('turicreate.data_structures.sketch')
Image = LazyModuleLoader('turicreate.data_structures.image')
from turicreate.data_structures.sarray_builder import SArrayBuilder
from turicreate.data_structures.sframe_builder import SFrameBuilder

LazyModuleLoader('turicreate.aggregate')
LazyModuleLoader('turicreate.toolkits')
clustering = LazyModuleLoader('turicreate.toolkits.clustering')
distances = LazyModuleLoader('turicreate.toolkits.distances')

text_analytics = LazyModuleLoader('turicreate.toolkits.text_analytics')
graph_analytics = LazyModuleLoader('turicreate.toolkits.graph_analytics')

connected_components = LazyModuleLoader('turicreate.toolkits.graph_analytics.shortest_path')
shortest_path = LazyModuleLoader('turicreate.toolkits.graph_analytics.shortest_path')
kcore = LazyModuleLoader('turicreate.toolkits.graph_analytics.kcore')
pagerank = LazyModuleLoader('turicreate.toolkits.graph_analytics.pagerank')
graph_coloring = LazyModuleLoader('turicreate.toolkits.graph_analytics.graph_coloring')
triangle_counting = LazyModuleLoader('tricreate.toolkits.graph_analytics.triangle_counting')
degree_counting = LazyModuleLoader('turicreate.toolkits.graph_analytics.degree_counting')
label_propagation = LazyModuleLoader('turicreate.toolkits.graph_analytics.label_propagation')

recommender = LazyModuleLoader('turicreate.toolkits.recommender')
popularity_recommender = LazyModuleLoader('turicreate.toolkits.recommender.popularity_recommender')
item_similarity_recommender = LazyModuleLoader('turicreate.toolkits.recommender.item_similarity_recommender')
ranking_factorization_recommender =  LazyModuleLoader('turicreate.toolkits.recommender.ranking_factorization_recommender')
item_content_recommender = LazyModuleLoader('turicreate.toolkits.recommender.item_content_recommender')
factorization_recommender = LazyModuleLoader('turicreate.toolkits.recommender.factorization_recommender')

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

nearest_neighbors = LazyModuleLoader('turicreate.toolkits.nearest_neighbors')
kmeans = LazyModuleLoader('turicreate.toolkits.clustering.kmeans')
dbscan = LazyModuleLoader('turicreate.toolkits.clustering.dbscan')
topic_model = LazyModuleLoader('turicreate.toolkits.topic_model.topic_model')

text_classifier = LazyModuleLoader('turicreate.toolkits.text_classifier')
image_classifier = LazyModuleLoader('turicreate.toolkits.image_classifier')
image_similarity = LazyModuleLoader('turicreate.toolkits.image_similarity')
object_detector = LazyModuleLoader('turicreate.toolkits.object_detector')
one_shot_object_detector = LazyModuleLoader('turicreate.toolkits.one_shot_object_detector')
style_transfer = LazyModuleLoader('turicreate.toolkits.style_transfer')
sound_classifier = LazyModuleLoader('turicreate.toolkits.sound_classifier.sound_classifier')
activity_classifier = LazyModuleLoader('turicreate.toolkits.activity_classifier')
drawing_classifier = LazyModuleLoader('turicreate.toolkits.drawing_classifier')

image_analysis = LazyModuleLoader('turicreate.toolkits.image_analysis')
from turicreate.toolkits.image_analysis.image_analysis import load_images
from turicreate.toolkits.audio_analysis.audio_analysis import load_audio

evaluation = LazyModuleLoader('turicreate.toolkits.evaluation')

# internal util
from turicreate._connect.main import launch as _launch

## bring load functions to the top level
from turicreate.data_structures.sframe import load_sframe
from turicreate.data_structures.sarray import load_sarray
from turicreate.toolkits._model import load_model

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
    turicreate._connect.main.get_server()
    return getattr(self._wrapped, name)

import sys as _sys
_sys.modules["turicreate.extensions"] = _extensions_wrapper(_sys.modules["turicreate.extensions"])
# rewrite the import
extensions = _sys.modules["turicreate.extensions"]

from turicreate.visualization import plot, show

_launch()

# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

'''
Turi Create offers a broad set of essential machine learning models as well as
task specific toolkits that let you to get started quickly while still giving you the
ability to go back and customize models later.
'''

from turicreate._deps import LazyModuleLoader as _LazyModuleLoader

_mod_par = 'turicreate.toolkits.'

# okay, this is a hack
_feature_engineering = _LazyModuleLoader(_mod_par + '_feature_engineering')
evaluation = _LazyModuleLoader(_mod_par + 'evaluation')

distances = _LazyModuleLoader(_mod_par + 'distances')
nearest_neighbors = _LazyModuleLoader(_mod_par + 'nearest_neighbors')
topic_model = _LazyModuleLoader(_mod_par + 'topic_model')
text_analytics = _LazyModuleLoader(_mod_par + 'text_analytics')
text_classifier = _LazyModuleLoader(_mod_par + 'text_classifier')
image_classifier = _LazyModuleLoader(_mod_par + 'image_classifier')
image_similarity = _LazyModuleLoader(_mod_par + 'image_similarity')
object_detector = _LazyModuleLoader(_mod_par + 'object_detector')
one_shot_object_detector = _LazyModuleLoader(_mod_par + 'one_shot_object_detector')
style_transfer = _LazyModuleLoader(_mod_par + 'style_transfer')
activity_classifier = _LazyModuleLoader(_mod_par + 'activity_classifier')
drawing_classifier = _LazyModuleLoader(_mod_par + 'drawing_classifier')

sound_classifier = _LazyModuleLoader(_mod_par + 'sound_classifier.sound_classifier')
audio_analysis = _LazyModuleLoader(_mod_par + 'audio_analysis.audio_analysis')
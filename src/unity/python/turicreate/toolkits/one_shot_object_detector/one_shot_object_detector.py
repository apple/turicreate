# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
#

import turicreate as _tc
import tarfile as _tarfile
from turicreate import extensions as _extensions
from turicreate.toolkits._model import CustomModel as _CustomModel
from turicreate.toolkits._model import PythonProxy as _PythonProxy
from turicreate.toolkits.object_detector.object_detector import ObjectDetector as _ObjectDetector
from turicreate.toolkits.one_shot_object_detector.util._augmentation import preview_augmented_images

def create(data,
           target,
           backgrounds=None,
           batch_size=0,
           max_iterations=0,
           verbose=True):
    """
    Create a :class:`ObjectDetector` model.

    Parameters
    ----------

    data : SFrame | tc.Image
        An SFrame that contains all the starter images along with their
        corresponding labels.
        If the dataset is a single tc.Image, this parameter is just that image.
        Every starter image should entirely be just the starter image without
        any padding.
        RGB and RGBA images allowed.

    target : string
        The target column name in the SFrame that contains all the labels. 
        If the dataset is a single tc.Image, this parameter is just a label for
        that image.

    backgrounds : optional SArray
        A list of backgrounds that the user wants to provide for data
        augmentation.
        If this is provided, only the backgrounds provided as this field will be
        used for augmentation.

    batch_size : int
        The number of images per training iteration. If 0, then it will be
        automatically determined based on resource availability.

    max_iterations : int
        The number of training iterations. If 0, then it will be automatically
        be determined based on the amount of data you provide.

    verbose : bool optional
        If True, print progress updates and model details.
    """
    augmented_data = preview_augmented_images(data, target, backgrounds)
    model = _tc.object_detector.create( augmented_data,
                                        batch_size=batch_size,
                                        max_iterations=max_iterations,
                                        verbose=verbose)
    state = {
        "detector": model,
        "augmented_data": augmented_data
        }
    return OneShotObjectDetector(state)

class OneShotObjectDetector(_CustomModel):
    _PYTHON_ONE_SHOT_OBJECT_DETECTOR_VERSION = 1
    
    def __init__(self, state):
        # We use PythonProxy here so that we get tab completion
        self.__proxy__ = _PythonProxy(state)

    def predict(self, dataset):
        return self.__proxy__['detector'].predict(dataset)

    def evaluate(self, dataset, metric="auto"):
        return self.__proxy__['detector'].evaluate(dataset, metric)

    def get_augmented_images(self):
        return self.__proxy__['augmented_data']

    def _get_version(self):
        return self._PYTHON_ONE_SHOT_OBJECT_DETECTOR_VERSION

    @classmethod
    def _native_name(cls):
        return "one_shot_object_detector"

    def _get_native_state(self):
        # make sure to not accidentally modify the proxy object.
        # take a copy of it.
        state = self.__proxy__.get_state()

        # We don't know how to serialize a Python class, hence we need to 
        # reduce the detector to the proxy object before saving it.
        state['detector'] = state['detector'].__proxy__
        return state

    @classmethod
    def _load_version(cls, state, version):
        assert(version == _PYTHON_ONE_SHOT_OBJECT_DETECTOR_VERSION)
        # we need to undo what we did at save and turn the proxy object
        # back into a Python class
        state['detector'] = _ObjectDetector(state['detector'])
        return OneShotObjectDetector(state)


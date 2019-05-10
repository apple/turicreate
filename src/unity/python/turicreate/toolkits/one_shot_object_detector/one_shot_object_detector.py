# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
#
import random as _random
import turicreate as _tc
from turicreate import extensions as _extensions
from turicreate.toolkits._model import CustomModel as _CustomModel

def create(dataset,
           target,
           backgrounds=None,
           feature=None,
           batch_size=0,
           max_iterations=0,
           seed=None,
           verbose=True):
    model = _extensions.one_shot_object_detector()
    if seed is None: seed = _random.randint(0, 2**32 - 1)
    if backgrounds is None:
        # replace this with loading backgrounds from developer.apple.com
        backgrounds = _tc.SArray()
    # Option arguments to pass in to C++ Object Detector, if we use it:
    # {'mlmodel_path':'darknet.mlmodel', 'max_iterations' : 25}
    augmented_data = model.augment(dataset, target, backgrounds, {"seed":seed})
    od_model = _tc.object_detector.create(augmented_data)
    state = {'detector':od_model}
    return OneShotObjectDetector(state)


class OneShotObjectDetector(_CustomModel):
    _PYTHON_ONE_SHOT_OBJECT_DETECTOR_VERSION = 1
    
    def __init__(self, state):
        # We use PythonProxy here so that we get tab completion
        self.__proxy__ = PythonProxy(state)

    def predict(self, dataset):
        return self.__proxy__['detector'].predict(dataset)

    def evaluate(self, dataset, metric="auto"):
        return self.__proxy__['detector'].evaluate(dataset, metric)

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
        state['detector'] = ObjectDetector(state['detector'])
        return OneShotObjectDetector(state)

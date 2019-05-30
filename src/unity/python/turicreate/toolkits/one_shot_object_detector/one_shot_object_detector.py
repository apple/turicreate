# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
#

import turicreate as _tc
from turicreate import extensions as _extensions
from turicreate.toolkits._model import CustomModel as _CustomModel
from turicreate.toolkits._model import PythonProxy as _PythonProxy
from turicreate.toolkits.object_detector.object_detector import ObjectDetector as _ObjectDetector
from turicreate.toolkits.one_shot_object_detector.util._augmentation import preview_synthetic_training_data as _preview_synthetic_training_data
import turicreate.toolkits._internal_utils as _tkutl

def create(data,
           target,
           backgrounds=None,
           batch_size=0,
           max_iterations=0,
           verbose=True):
    """
    Create a :class:`OneShotObjectDetector` model.

    Parameters
    ----------
    data : SFrame | tc.Image
        A single starter image or an SFrame that contains the starter images
        along with their corresponding labels.  These image(s) can be in either
        RGB or RGBA format. They should not be padded.

    target : string
        Name of the target (when data is a single image) or the target column
        name (when data is an SFrame of images).

    backgrounds : optional SArray
        A list of backgrounds used for synthetic data generation. When set to
        None, a set of default backgrounds are downloaded and used.

    batch_size : int
        The number of images per training iteration. If 0, then it will be
        automatically determined based on resource availability.

    max_iterations : int
        The number of training iterations. If 0, then it will be automatically
        be determined based on the amount of data you provide.

    verbose : bool optional
        If True, print progress updates and model details.

    Examples
    --------
    .. sourcecode:: python

        # Train an object detector model
        >>> model = turicreate.one_shot_object_detector.create(train_data, label = 'cards')

        # Make predictions on the training set and as column to the SFrame
        >>> test_data['predictions'] = model.predict(test_data)
    """
    augmented_data = _preview_synthetic_training_data(data, target, backgrounds)
    model = _tc.object_detector.create( augmented_data,
                                        batch_size=batch_size,
                                        max_iterations=max_iterations,
                                        verbose=verbose)
    if isinstance(data, _tc.SFrame):
        num_starter_images = len(data)
    else:
        num_starter_images = 1
    state = {
        "detector": model,
        "target": target,
        "num_classes": model.num_classes,
        "num_starter_images": num_starter_images,
        "_detector_version": _ObjectDetector._PYTHON_OBJECT_DETECTOR_VERSION,
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
        state['detector'] = state['detector']._get_native_state()
        return state

    @classmethod
    def _load_version(cls, state, version):
        assert(version == cls._PYTHON_ONE_SHOT_OBJECT_DETECTOR_VERSION)
        # we need to undo what we did at save and turn the proxy object
        # back into a Python class
        state['detector'] = _ObjectDetector._load_version(
            state['detector'], state["_detector_version"])
        return OneShotObjectDetector(state)

    def __str__(self):
        """
        Return a string description of the model to the ``print`` method.

        Returns
        -------
        out : string
            A description of the OneShotObjectDetector
        """
        return self.__repr__()

    def __repr__(self):
        """
        Print a string description of the model when the model name is entered
        in the terminal.
        """
        width = 40
        sections, section_titles = self._get_summary_struct()
        detector = self.__proxy__['detector']
        out = _tkutl._toolkit_repr_print(detector, sections, section_titles,
                                         width=width, class_name='OneShotObjectDetector')
        return out

    def _get_summary_struct(self):
        """
        Returns a structured description of the model, including (where
        relevant) the schema of the training data, description of the training
        data, training statistics, and model hyperparameters.

        Returns
        -------
        sections : list (of list of tuples)
            A list of summary sections.
              Each section is a list.
                Each item in a section list is a tuple of the form:
                  ('<label>','<field>')
        section_titles: list
            A list of section titles.
              The order matches that of the 'sections' object.
        """
        model_fields = [
            ('Number of classes', 'num_classes'),
            ('Input image shape', 'input_image_shape'),
        ]
        data_fields = [
            ('Number of synthetically generated examples', 'num_examples'),
            ('Number of synthetically generated bounding boxes', 'num_bounding_boxes'),
        ]
        training_fields = [
            ('Training time', '_training_time_as_string'),
            ('Training iterations', 'training_iterations'),
            ('Training epochs', 'training_epochs'),
            ('Final loss (specific to model)', 'training_loss'),
        ]

        section_titles = ['Model summary', 'Synthetic data summary', 'Training summary']
        return([model_fields, data_fields, training_fields], section_titles)

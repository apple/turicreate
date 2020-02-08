# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
#

import turicreate as _tc
from turicreate import extensions as _extensions
from turicreate.toolkits._model import CustomModel as _CustomModel
from turicreate.toolkits._model import (
    PythonProxy as _PythonProxy, ToolkitError as _ToolkitError
)
from turicreate.toolkits.object_detector.object_detector import (
    ObjectDetector as _ObjectDetector,
)
from turicreate.toolkits.one_shot_object_detector.util._augmentation import (
    preview_synthetic_training_data as _preview_synthetic_training_data,
)
import turicreate.toolkits._internal_utils as _tkutl


def create(
    data, target, backgrounds=None, batch_size=0, max_iterations=0, verbose=True
):
    """
    Create a :class:`OneShotObjectDetector` model. Note: The One Shot Object Detector
    is currently in beta.

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
    if not isinstance(data, _tc.SFrame) and not isinstance(data, _tc.Image):
        raise TypeError("'data' must be of type SFrame or tc.Image.")
    if isinstance(data, _tc.SFrame) and len(data) == 0:
        raise _ToolkitError("'data' can not be an empty SFrame")

    augmented_data = _preview_synthetic_training_data(data, target, backgrounds)

    model = _tc.object_detector.create(
        augmented_data,
        batch_size=batch_size,
        max_iterations=max_iterations,
        verbose=verbose,
    )

    if isinstance(data, _tc.SFrame):
        num_starter_images = len(data)
    else:
        num_starter_images = 1

    state = {
        "detector": model,
        "target": target,
        "num_classes": model.num_classes,
        "num_starter_images": num_starter_images,
        "_detector_version": _ObjectDetector._CPP_OBJECT_DETECTOR_VERSION,
    }
    return OneShotObjectDetector(state)


class OneShotObjectDetector(_CustomModel):
    """
    An trained model that is ready to use for classification, exported to
    Core ML, or for feature extraction.

    This model should not be constructed directly.
    """

    _PYTHON_ONE_SHOT_OBJECT_DETECTOR_VERSION = 1

    def __init__(self, state):
        # We use PythonProxy here so that we get tab completion
        self.__proxy__ = _PythonProxy(state)

    def predict(
        self, dataset, confidence_threshold=0.25, iou_threshold=0.45, verbose=True
    ):
        """
        Predict object instances in an SFrame of images.

        Parameters
        ----------
        dataset : SFrame | SArray | turicreate.Image
            The images on which to perform object detection.
            If dataset is an SFrame, it must have a column with the same name
            as the feature column during training. Additional columns are
            ignored.

        confidence_threshold : float
            Only return predictions above this level of confidence. The
            threshold can range from 0 to 1.

        iou_threshold : float
            Threshold value for non-maximum suppression. Non-maximum suppression
            prevents multiple bounding boxes appearing over a single object.
            This threshold, set between 0 and 1, controls how aggressive this
            suppression is. A value of 1 means no maximum suppression will
            occur, while a value of 0 will maximally suppress neighboring
            boxes around a prediction.

        verbose : bool
            If True, prints prediction progress.

        Returns
        -------
        out : SArray
            An SArray with model predictions. Each element corresponds to
            an image and contains a list of dictionaries. Each dictionary
            describes an object instances that was found in the image. If
            `dataset` is a single image, the return value will be a single
            prediction.

        See Also
        --------
        draw_bounding_boxes

        Examples
        --------
        .. sourcecode:: python

            # Make predictions
            >>> pred = model.predict(data)
            >>> predictions_with_bounding_boxes = tc.one_shot_object_detector.util.draw_bounding_boxes(data['images'], pred)
            >>> predictions_with_bounding_boxes.explore()

        """
        return self.__proxy__["detector"].predict(
            dataset=dataset,
            confidence_threshold=confidence_threshold,
            iou_threshold=iou_threshold,
            verbose=verbose,
        )

    def export_coreml(
        self,
        filename,
        include_non_maximum_suppression=True,
        iou_threshold=None,
        confidence_threshold=None,
    ):
        """
        Save the model in Core ML format. The Core ML model takes an image of
        fixed size as input and produces two output arrays: `confidence` and
        `coordinates`.

        The first one, `confidence` is an `N`-by-`C` array, where `N` is the
        number of instances predicted and `C` is the number of classes. The
        number `N` is fixed and will include many low-confidence predictions.
        The instances are not sorted by confidence, so the first one will
        generally not have the highest confidence (unlike in `predict`). Also
        unlike the `predict` function, the instances have not undergone
        what is called `non-maximum suppression`, which means there could be
        several instances close in location and size that have all discovered
        the same object instance. Confidences do not need to sum to 1 over the
        classes; any remaining probability is implied as confidence there is no
        object instance present at all at the given coordinates. The classes
        appear in the array alphabetically sorted.

        The second array `coordinates` is of size `N`-by-4, where the first
        dimension `N` again represents instances and corresponds to the
        `confidence` array. The second dimension represents `x`, `y`, `width`,
        `height`, in that order.  The values are represented in relative
        coordinates, so (0.5, 0.5) represents the center of the image and (1,
        1) the bottom right corner. You will need to multiply the relative
        values with the original image size before you resized it to the fixed
        input size to get pixel-value coordinates similar to `predict`.

        See Also
        --------
        save

        Parameters
        ----------
        filename : string
            The path of the file where we want to save the Core ML model.

        include_non_maximum_suppression : bool
            Non-maximum suppression is only available in iOS 12+.
            A boolean parameter to indicate whether the Core ML model should be
            saved with built-in non-maximum suppression or not.
            This parameter is set to True by default.

        iou_threshold : float
            Threshold value for non-maximum suppression. Non-maximum suppression
            prevents multiple bounding boxes appearing over a single object.
            This threshold, set between 0 and 1, controls how aggressive this
            suppression is. A value of 1 means no maximum suppression will
            occur, while a value of 0 will maximally suppress neighboring
            boxes around a prediction.

        confidence_threshold : float
            Only return predictions above this level of confidence. The
            threshold can range from 0 to 1.

        Examples
        --------
        >>> model.export_coreml('one_shot.mlmodel')
        """
        from turicreate.toolkits import _coreml_utils

        additional_user_defined_metadata = _coreml_utils._get_tc_version_info()
        short_description = _coreml_utils._mlmodel_short_description("Object Detector")
        options = {
            "include_non_maximum_suppression": include_non_maximum_suppression,
        }

        options["version"] = self._PYTHON_ONE_SHOT_OBJECT_DETECTOR_VERSION

        if confidence_threshold is not None:
            options["confidence_threshold"] = confidence_threshold

        if iou_threshold is not None:
            options["iou_threshold"] = iou_threshold

        additional_user_defined_metadata = _coreml_utils._get_tc_version_info()
        short_description = _coreml_utils._mlmodel_short_description(
            "One Shot Object Detector"
        )
        self.__proxy__["detector"].__proxy__.export_to_coreml(
            filename, short_description, additional_user_defined_metadata, options
        )

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
        state["detector"] = {"detector_model": state["detector"].__proxy__}
        return state

    @classmethod
    def _load_version(cls, state, version):
        assert version == cls._PYTHON_ONE_SHOT_OBJECT_DETECTOR_VERSION
        # we need to undo what we did at save and turn the proxy object
        # back into a Python class
        state["detector"] = _ObjectDetector._load_version(
            state["detector"], state["_detector_version"]
        )
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
        detector = self.__proxy__["detector"]
        out = _tkutl._toolkit_repr_print(
            detector,
            sections,
            section_titles,
            width=width,
            class_name="OneShotObjectDetector",
        )
        return out

    def summary(self, output=None):
        """
        Print a summary of the model. The summary includes a description of
        training data, options, hyper-parameters, and statistics measured
        during model creation.

        Parameters
        ----------
        output : str, None
            The type of summary to return.

            - None or 'stdout' : print directly to stdout.

            - 'str' : string of summary

            - 'dict' : a dict with 'sections' and 'section_titles' ordered
              lists. The entries in the 'sections' list are tuples of the form
              ('label', 'value').

        Examples
        --------
        >>> m.summary()
        """
        from turicreate.toolkits._internal_utils import (
            _toolkit_serialize_summary_struct,
        )

        if output is None or output == "stdout":
            pass
        elif output == "str":
            return self.__repr__()
        elif output == "dict":
            return _toolkit_serialize_summary_struct(
                self.__proxy__["detector"], *self._get_summary_struct()
            )
        try:
            print(self.__repr__())
        except:
            return self.__class__.__name__

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
            ("Number of classes", "num_classes"),
            ("Input image shape", "input_image_shape"),
        ]
        data_fields = [
            ("Number of synthetically generated examples", "num_examples"),
            ("Number of synthetically generated bounding boxes", "num_bounding_boxes"),
        ]
        training_fields = [
            ("Training time", "_training_time_as_string"),
            ("Training iterations", "training_iterations"),
            ("Training epochs", "training_epochs"),
            ("Final loss (specific to model)", "training_loss"),
        ]

        section_titles = ["Model summary", "Synthetic data summary", "Training summary"]
        return ([model_fields, data_fields, training_fields], section_titles)

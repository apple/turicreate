# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Class definition and utilities for the object detection toolkit.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import time as _time
import itertools as _itertools
from datetime import datetime as _datetime

import six as _six
import turicreate as _tc
import numpy as _np
from threading import Thread as _Thread
from six.moves.queue import Queue as _Queue

from turicreate.toolkits._model import CustomModel as _CustomModel
from turicreate.toolkits._model import Model as _Model
import turicreate.toolkits._internal_utils as _tkutl
from turicreate.toolkits import _coreml_utils
from turicreate.toolkits._model import PythonProxy as _PythonProxy
from turicreate.toolkits._internal_utils import (
    _raise_error_if_not_sframe,
    _numeric_param_check_range,
    _raise_error_if_not_iterable,
)
from turicreate import config as _tc_config
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from .. import _pre_trained_models
from ._evaluation import average_precision as _average_precision
from .._mps_utils import (
    use_mps as _use_mps,
    mps_device_memory_limit as _mps_device_memory_limit,
    MpsGraphAPI as _MpsGraphAPI,
    MpsGraphNetworkType as _MpsGraphNetworkType,
    MpsGraphMode as _MpsGraphMode,
)


def _get_mps_od_net(
    input_image_shape, batch_size, output_size, anchors, config, weights={}
):
    """
    Initializes an MpsGraphAPI for object detection.
    """
    network = _MpsGraphAPI(network_id=_MpsGraphNetworkType.kODGraphNet)

    c_in, h_in, w_in = input_image_shape
    c_out = output_size
    h_out = h_in // 32
    w_out = w_in // 32

    c_view = c_in
    h_view = h_in
    w_view = w_in

    network.init(
        batch_size,
        c_in,
        h_in,
        w_in,
        c_out,
        h_out,
        w_out,
        weights=weights,
        config=config,
    )
    return network


# Standard lib functions would be great here, but the formatting options of
# timedelta are not great
def _seconds_as_string(seconds):
    """
    Returns seconds as a human-friendly string, e.g. '1d 4h 47m 41s'
    """
    TIME_UNITS = [("s", 60), ("m", 60), ("h", 24), ("d", None)]
    unit_strings = []
    cur = max(int(seconds), 1)
    for suffix, size in TIME_UNITS:
        if size is not None:
            cur, rest = divmod(cur, size)
        else:
            rest = cur
        if rest > 0:
            unit_strings.insert(0, "%d%s" % (rest, suffix))
    return " ".join(unit_strings)


def _raise_error_if_not_detection_sframe(
    dataset, feature, annotations, require_annotations
):
    _raise_error_if_not_sframe(dataset, "datset")
    if feature not in dataset.column_names():
        raise _ToolkitError("Feature column '%s' does not exist" % feature)
    if dataset[feature].dtype != _tc.Image:
        raise _ToolkitError("Feature column must contain images")

    if require_annotations:
        if annotations not in dataset.column_names():
            raise _ToolkitError("Annotations column '%s' does not exist" % annotations)
        if dataset[annotations].dtype not in [list, dict]:
            raise _ToolkitError("Annotations column must be of type dict or list")


def create(
    dataset,
    annotations=None,
    feature=None,
    model="darknet-yolo",
    classes=None,
    batch_size=0,
    max_iterations=0,
    verbose=True,
    grid_shape=[13, 13],
    **kwargs
):
    """
    Create a :class:`ObjectDetector` model.

    Parameters
    ----------
    dataset : SFrame
        Input data. The columns named by the ``feature`` and ``annotations``
        parameters will be extracted for training the detector.

    annotations : string
        Name of the column containing the object detection annotations.  This
        column should be a list of dictionaries (or a single dictionary), with
        each dictionary representing a bounding box of an object instance. Here
        is an example of the annotations for a single image with two object
        instances::

            [{'label': 'dog',
              'type': 'rectangle',
              'coordinates': {'x': 223, 'y': 198,
                              'width': 130, 'height': 230}},
             {'label': 'cat',
              'type': 'rectangle',
              'coordinates': {'x': 40, 'y': 73,
                              'width': 80, 'height': 123}}]

        The value for `x` is the horizontal center of the box paired with
        `width` and `y` is the vertical center of the box paired with `height`.
        'None' (the default) indicates the only list column in `dataset` should
        be used for the annotations.

    feature : string
        Name of the column containing the input images. 'None' (the default)
        indicates the only image column in `dataset` should be used as the
        feature.

    model : string optional
        Object detection model to use:

           - "darknet-yolo" : Fast and medium-sized model

    grid_shape : array optional
        Shape of the grid used for object detection. Higher values increase precision for small objects, but at a higher computational cost

           - [13, 13] : Default grid value for a Fast and medium-sized model

    classes : list optional
        List of strings containing the names of the classes of objects.
        Inferred from the data if not provided.

    batch_size: int
        The number of images per training iteration. If 0, then it will be
        automatically determined based on resource availability.

    max_iterations : int
        The number of training iterations. If 0, then it will be automatically
        be determined based on the amount of data you provide.

    verbose : bool, optional
        If True, print progress updates and model details.

    Returns
    -------
    out : ObjectDetector
        A trained :class:`ObjectDetector` model.

    See Also
    --------
    ObjectDetector

    Examples
    --------
    .. sourcecode:: python

        # Train an object detector model
        >>> model = turicreate.object_detector.create(data)

        # Make predictions on the training set and as column to the SFrame
        >>> data['predictions'] = model.predict(data)

        # Visualize predictions by generating a new column of marked up images
        >>> data['image_pred'] = turicreate.object_detector.util.draw_bounding_boxes(data['image'], data['predictions'])
    """
    _raise_error_if_not_sframe(dataset, "dataset")

    if len(dataset) == 0:
        raise _ToolkitError("Unable to train on empty dataset")

    _numeric_param_check_range("max_iterations", max_iterations, 0, _six.MAXSIZE)
    start_time = _time.time()

    supported_detectors = ["darknet-yolo"]

    if feature is None:
        feature = _tkutl._find_only_image_column(dataset)
        if verbose:
            print("Using '%s' as feature column" % feature)
    if annotations is None:
        annotations = _tkutl._find_only_column_of_type(
            dataset, target_type=[list, dict], type_name="list", col_name="annotations"
        )
        if verbose:
            print("Using '%s' as annotations column" % annotations)

    _raise_error_if_not_detection_sframe(
        dataset, feature, annotations, require_annotations=True
    )
    _tkutl._handle_missing_values(dataset, feature, "dataset")
    is_annotations_list = dataset[annotations].dtype == list

    _tkutl._check_categorical_option_type("model", model, supported_detectors)

    base_model = model.split("-", 1)[0]
    ref_model = _pre_trained_models.OBJECT_DETECTION_BASE_MODELS[base_model]()

    pretrained_model = _pre_trained_models.OBJECT_DETECTION_BASE_MODELS[
        "darknet_mlmodel"
    ]()
    pretrained_model_path = pretrained_model.get_model_path()

    params = {
        "anchors": [
            (1.0, 2.0),
            (1.0, 1.0),
            (2.0, 1.0),
            (2.0, 4.0),
            (2.0, 2.0),
            (4.0, 2.0),
            (4.0, 8.0),
            (4.0, 4.0),
            (8.0, 4.0),
            (8.0, 16.0),
            (8.0, 8.0),
            (16.0, 8.0),
            (16.0, 32.0),
            (16.0, 16.0),
            (32.0, 16.0),
        ],
        "grid_shape": grid_shape,
        "aug_resize": 0,
        "aug_rand_crop": 0.9,
        "aug_rand_pad": 0.9,
        "aug_rand_gray": 0.0,
        "aug_aspect_ratio": 1.25,
        "aug_hue": 0.05,
        "aug_brightness": 0.05,
        "aug_saturation": 0.05,
        "aug_contrast": 0.05,
        "aug_horizontal_flip": True,
        "aug_min_object_covered": 0,
        "aug_min_eject_coverage": 0.5,
        "aug_area_range": (0.15, 2),
        "aug_pca_noise": 0.0,
        "aug_max_attempts": 20,
        "aug_inter_method": 2,
        "lmb_coord_xy": 10.0,
        "lmb_coord_wh": 10.0,
        "lmb_obj": 100.0,
        "lmb_noobj": 5.0,
        "lmb_class": 2.0,
        "non_maximum_suppression_threshold": 0.45,
        "rescore": True,
        "clip_gradients": 0.025,
        "weight_decay": 0.0005,
        "sgd_momentum": 0.9,
        "learning_rate": 1.0e-3,
        "shuffle": True,
        "mps_loss_mult": 8,
        # This large buffer size (8 batches) is an attempt to mitigate against
        # the SFrame shuffle operation that can occur after each epoch.
        "io_thread_buffer_size": 8,
        "mlmodel_path": pretrained_model_path,
    }

    # create tensorflow model here
    import turicreate.toolkits.libtctensorflow

    if classes == None:
        classes = []

    _raise_error_if_not_iterable(classes)
    _raise_error_if_not_iterable(grid_shape)

    grid_shape = [int(x) for x in grid_shape]
    assert len(grid_shape) == 2

    tf_config = {
        "grid_height": params["grid_shape"][0],
        "grid_width": params["grid_shape"][1],
        "mlmodel_path": params["mlmodel_path"],
        "classes": classes,
        "compute_final_metrics": False,
        "verbose": verbose,
        "model" : "darknet-yolo"
    }

    # If batch_size or max_iterations = 0, they will be automatically
    # generated in C++.
    if batch_size > 0:
        tf_config["batch_size"] = batch_size

    if max_iterations > 0:
        tf_config["max_iterations"] = max_iterations

    model = _tc.extensions.object_detector()
    model.train(
        data=dataset,
        annotations_column_name=annotations,
        image_column_name=feature,
        options=tf_config,
    )
    return ObjectDetector(model_proxy=model, name="object_detector")


class ObjectDetector(_Model):
    """
    A trained model using C++ implementation that is ready to use for classification
    or export to CoreML.

    This model should not be constructed directly.
    """

    _CPP_OBJECT_DETECTOR_VERSION = 1

    def __init__(self, model_proxy=None, name=None):
        self.__proxy__ = model_proxy
        self.__name__ = name

    @classmethod
    def _native_name(cls):
        return "object_detector"

    def __str__(self):
        """
        Return a string description of the model to the ``print`` method.

        Returns
        -------
        out : string
            A description of the ObjectDetector.
        """
        return self.__repr__()

    def __repr__(self):
        """
        Print a string description of the model when the model name is entered
        in the terminal.
        """

        width = 40
        sections, section_titles = self._get_summary_struct()
        out = _tkutl._toolkit_repr_print(self, sections, section_titles, width=width)
        return out

    def _get_version(self):
        return self._CPP_OBJECT_DETECTOR_VERSION

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
        >>> model.export_coreml('detector.mlmodel')
        """
        options = {}
        options["include_non_maximum_suppression"] = include_non_maximum_suppression
        options["version"] = self._get_version()
        if confidence_threshold is not None:
            options["confidence_threshold"] = confidence_threshold
        if iou_threshold is not None:
            options["iou_threshold"] = iou_threshold
        additional_user_defined_metadata = _coreml_utils._get_tc_version_info()
        short_description = _coreml_utils._mlmodel_short_description("Object Detector")

        self.__proxy__.export_to_coreml(
            filename, short_description, additional_user_defined_metadata, options
        )

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
        evaluate

        Examples
        --------
        .. sourcecode:: python

            # Make predictions
            >>> pred = model.predict(data)

            # Stack predictions, for a better overview
            >>> turicreate.object_detector.util.stack_annotations(pred)
            Data:
            +--------+------------+-------+-------+-------+-------+--------+
            | row_id | confidence | label |   x   |   y   | width | height |
            +--------+------------+-------+-------+-------+-------+--------+
            |   0    |    0.98    |  dog  | 123.0 | 128.0 |  80.0 | 182.0  |
            |   0    |    0.67    |  cat  | 150.0 | 183.0 | 129.0 | 101.0  |
            |   1    |    0.8     |  dog  |  50.0 | 432.0 |  65.0 |  98.0  |
            +--------+------------+-------+-------+-------+-------+--------+
            [3 rows x 7 columns]

            # Visualize predictions by generating a new column of marked up images
            >>> data['image_pred'] = turicreate.object_detector.util.draw_bounding_boxes(data['image'], data['predictions'])
        """
        _numeric_param_check_range(
            "confidence_threshold", confidence_threshold, 0.0, 1.0
        )
        _numeric_param_check_range("iou_threshold", iou_threshold, 0.0, 1.0)
        options = {}
        options["confidence_threshold"] = confidence_threshold
        options["iou_threshold"] = iou_threshold
        options["verbose"] = verbose
        return self.__proxy__.predict(dataset, options)

    def evaluate(
        self,
        dataset,
        metric="auto",
        output_type="dict",
        confidence_threshold=0.001,
        iou_threshold=0.45,
    ):
        """
        Evaluate the model by making predictions and comparing these to ground
        truth bounding box annotations.

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the annotations and feature used for model training.
            Additional columns are ignored.

        metric : str or list, optional
            Name of the evaluation metric or list of several names. The primary
            metric is average precision, which is the area under the
            precision/recall curve and reported as a value between 0 and 1 (1
            being perfect). Possible values are:

            - 'auto'                      : Returns all primary metrics.
            - 'all'                       : Returns all available metrics.
            - 'average_precision_50'      : Average precision per class with
                                            intersection-over-union threshold at
                                            50% (PASCAL VOC metric).
            - 'average_precision'         : Average precision per class calculated over multiple
                                            intersection-over-union thresholds
                                            (at 50%, 55%, ..., 95%) and averaged.
            - 'mean_average_precision_50' : Mean over all classes (for ``'average_precision_50'``).
                                            This is the primary single-value metric.
            - 'mean_average_precision'    : Mean over all classes (for ``'average_precision'``)

        Returns
        -------
        out : dict / SFrame
            Output type depends on the option `output_type`.

        See Also
        --------
        create, predict

        Examples
        --------
        >>> results = model.evaluate(data)
        >>> print('mAP: {:.1%}'.format(results['mean_average_precision']))
        mAP: 43.2%
        """
        _numeric_param_check_range(
            "confidence_threshold", confidence_threshold, 0.0, 1.0
        )
        _numeric_param_check_range("iou_threshold", iou_threshold, 0.0, 1.0)
        options = {}
        options["confidence_threshold"] = confidence_threshold
        options["iou_threshold"] = iou_threshold
        return self.__proxy__.evaluate(dataset, metric, output_type, options)

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
            ("Model", "model"),
            ("Number of classes", "num_classes"),
            ("Input image shape", "input_image_shape"),
        ]
        training_fields = [
            ("Training time", "_training_time_as_string"),
            ("Training epochs", "training_epochs"),
            ("Training iterations", "training_iterations"),
            ("Number of examples (images)", "num_examples"),
            ("Number of bounding boxes (instances)", "num_bounding_boxes"),
            ("Final loss (specific to model)", "training_loss"),
        ]

        section_titles = ["Schema", "Training summary"]
        return ([model_fields, training_fields], section_titles)

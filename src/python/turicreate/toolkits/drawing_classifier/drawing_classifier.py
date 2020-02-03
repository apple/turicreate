# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _tc
import numpy as _np
import time as _time
from turicreate.toolkits import _coreml_utils
from turicreate.toolkits._model import CustomModel as _CustomModel
from turicreate.toolkits._model import Model as _Model
from turicreate.toolkits._model import PythonProxy as _PythonProxy
from turicreate.toolkits import evaluation as _evaluation
import turicreate.toolkits._internal_utils as _tkutl
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from ..image_classifier._evaluation import Evaluation as _Evaluation
from turicreate import extensions as _extensions
from .. import _pre_trained_models
from six.moves import reduce as _reduce

BITMAP_WIDTH = 28
BITMAP_HEIGHT = 28
TRAIN_VALIDATION_SPLIT = 0.95


def _raise_error_if_not_drawing_classifier_input_sframe(dataset, feature, target):
    """
    Performs some sanity checks on the SFrame provided as input to
    `turicreate.drawing_classifier.create` and raises a ToolkitError
    if something in the dataset is missing or wrong.
    """
    from turicreate.toolkits._internal_utils import _raise_error_if_not_sframe

    _raise_error_if_not_sframe(dataset)
    if feature not in dataset.column_names():
        raise _ToolkitError("Feature column '%s' does not exist" % feature)
    if target not in dataset.column_names():
        raise _ToolkitError("Target column '%s' does not exist" % target)
    if dataset[feature].dtype != _tc.Image and dataset[feature].dtype != list:
        raise _ToolkitError(
            "Feature column must contain images"
            + " or stroke-based drawings encoded as lists of strokes"
            + " where each stroke is a list of points and"
            + " each point is stored as a dictionary"
        )
    if dataset[target].dtype != int and dataset[target].dtype != str:
        raise _ToolkitError(
            "Target column contains "
            + str(dataset[target].dtype)
            + " but it must contain strings or integers to represent"
            + " labels for drawings."
        )
    if len(dataset) == 0:
        raise _ToolkitError("Input Dataset is empty!")


def create(
    input_dataset,
    target,
    feature=None,
    validation_set="auto",
    warm_start="auto",
    batch_size=256,
    max_iterations=500,
    verbose=True,
    random_seed=None,
    **kwargs
):
    """
    Create a :class:`DrawingClassifier` model.

    Parameters
    ----------
    dataset : SFrame
        Input data. The columns named by the ``feature`` and ``target``
        parameters will be extracted for training the drawing classifier.

    target : string
        Name of the column containing the target variable. The values in this
        column must be of string or integer type.

    feature : string optional
        Name of the column containing the input drawings.
        The feature column can contain either bitmap-based drawings or
        stroke-based drawings. Bitmap-based drawing input can be a grayscale
        tc.Image of any size.
        Stroke-based drawing input must be in the following format:
        Every drawing must be represented by a list of strokes, where each
        stroke must be a list of points in the order in which they were drawn
        on the canvas.
        Each point must be a dictionary with two keys, "x" and "y", and their
        respective values must be numerical, i.e. either integer or float.

    validation_set : SFrame optional
        A dataset for monitoring the model's generalization performance.
        The format of this SFrame must be the same as the training set.
        By default this argument is set to 'auto' and a validation set is
        automatically sampled and used for progress printing. If
        validation_set is set to None, then no additional metrics
        are computed. The default value is 'auto'.

    warm_start : string optional
        A string to denote which pretrained model to use. Set to "auto"
        by default which uses a model trained on 245 of the 345 classes in the
        Quick, Draw! dataset. To disable warm start, pass in None to this
        argument. Here is a list of all the pretrained models that
        can be passed in as this argument:
        "auto": Uses quickdraw_245_v0
        "quickdraw_245_v0": Uses a model trained on 245 of the 345 classes in the
                         Quick, Draw! dataset.
        None: No Warm Start

    batch_size: int optional
        The number of drawings per training step. If not set, a default
        value of 256 will be used. If you are getting memory errors,
        try decreasing this value. If you have a powerful computer, increasing
        this value may improve performance.

    max_iterations : int optional
        The maximum number of allowed passes through the data. More passes over
        the data can result in a more accurately trained model.

    verbose : bool optional
        If True, print progress updates and model details.

    random_seed : int, optional
        The results can be reproduced when given the same seed.

    Returns
    -------
    out : DrawingClassifier
        A trained :class:`DrawingClassifier` model.

    See Also
    --------
    DrawingClassifier

    Examples
    --------
    .. sourcecode:: python

        # Train a drawing classifier model
        >>> model = turicreate.drawing_classifier.create(data)

        # Make predictions on the training set and as column to the SFrame
        >>> data['predictions'] = model.predict(data)
    """

    accepted_values_for_warm_start = ["auto", "quickdraw_245_v0", None]
    if warm_start is not None:
        if type(warm_start) is not str:
            raise TypeError(
                "'warm_start' must be a string or None. "
                + "'warm_start' can take in the following values: "
                + str(accepted_values_for_warm_start)
            )
        if warm_start not in accepted_values_for_warm_start:
            raise _ToolkitError(
                "Unrecognized value for 'warm_start': "
                + warm_start
                + ". 'warm_start' can take in the following "
                + "values: "
                + str(accepted_values_for_warm_start)
            )
        # Replace 'auto' with name of current default Warm Start model.
        warm_start = warm_start.replace("auto", "quickdraw_245_v0")

    if "_advanced_parameters" in kwargs:
        # Make sure no additional parameters are provided
        new_keys = set(kwargs["_advanced_parameters"].keys())
        set_keys = set(params.keys())
        unsupported = new_keys - set_keys
        if unsupported:
            raise _ToolkitError("Unknown advanced parameters: {}".format(unsupported))

        params.update(kwargs["_advanced_parameters"])

    # @TODO: Should be able to automatically choose number of iterations
    # based on data size: Tracked in Github Issue #1576
    if not isinstance(input_dataset, _tc.SFrame):
        raise TypeError('"input_dataset" must be of type SFrame.')

    # automatically infer feature column
    if feature is None:
        feature = _tkutl._find_only_drawing_column(input_dataset)

    _raise_error_if_not_drawing_classifier_input_sframe(input_dataset, feature, target)

    if batch_size is not None and not isinstance(batch_size, int):
        raise TypeError("'batch_size' must be an integer >= 1")
    if batch_size is not None and batch_size < 1:
        raise ValueError("'batch_size' must be >= 1")
    if max_iterations is not None and not isinstance(max_iterations, int):
        raise TypeError("'max_iterations' must be an integer >= 1")
    if max_iterations is not None and max_iterations < 1:
        raise ValueError("'max_iterations' must be >= 1")

    import turicreate.toolkits.libtctensorflow

    model = _tc.extensions.drawing_classifier()
    options = dict()
    options["batch_size"] = batch_size
    options["max_iterations"] = max_iterations
    options["verbose"] = verbose
    options["_show_loss"] = False
    if validation_set is None:
        validation_set = _tc.SFrame()
    if warm_start:
        # Load CoreML warmstart model
        pretrained_mlmodel = _pre_trained_models.DrawingClassifierPreTrainedMLModel()
        options["mlmodel_path"] = pretrained_mlmodel.get_model_path()
    if random_seed is not None:
        options["random_seed"] = random_seed
    options["warm_start"] = "" if warm_start is None else warm_start
    model.train(input_dataset, target, feature, validation_set, options)
    return DrawingClassifier(model_proxy=model, name="drawing_classifier")


class DrawingClassifier(_Model):
    """
    A trained model using C++ implementation that is ready to use for classification or export to
    CoreML.

    This model should not be constructed directly.
    """

    _CPP_DRAWING_CLASSIFIER_VERSION = 1

    def __init__(self, model_proxy=None, name=None):
        self.__proxy__ = model_proxy
        self.__name__ = name

    @classmethod
    def _native_name(cls):
        return "drawing_classifier"

    def __str__(self):
        """
        Return a string description of the model to the ``print`` method.

        Returns
        -------
        out : string
            A description of the DrawingClassifier.
        """
        return self.__repr__()

    def __repr__(self):
        """
        Returns a string description of the model, including (where relevant)
        the schema of the training data, description of the training data,
        training statistics, and model hyperparameters.

        Returns
        -------
        out : string
            A description of the model.
        """

        width = 40
        sections, section_titles = self._get_summary_struct()
        out = _tkutl._toolkit_repr_print(self, sections, section_titles, width=width)
        return out

    def _get_version(self):
        return self._CPP_DRAWING_CLASSIFIER_VERSION

    def export_coreml(self, filename):
        """
        Export the model in Core ML format.

        Parameters
        ----------
        filename: str
          A valid filename where the model can be saved.

        Examples
        --------
        >>> model.export_coreml("MyModel.mlmodel")
        """
        additional_user_defined_metadata = _coreml_utils._get_tc_version_info()
        short_description = _coreml_utils._mlmodel_short_description(
            "Drawing Classifier"
        )
        self.__proxy__.export_to_coreml(
            filename, short_description, additional_user_defined_metadata
        )

    def predict(self, dataset, output_type="class"):
        """
        Predict on an SFrame or SArray of drawings, or on a single drawing.

        Parameters
        ----------
        data : SFrame | SArray | tc.Image
            The drawing(s) on which to perform drawing classification.
            If dataset is an SFrame, it must have a column with the same name
            as the feature column during training. Additional columns are
            ignored.
            If the data is a single drawing, it can be either of type tc.Image,
            in which case it is a bitmap-based drawing input,
            or of type list, in which case it is a stroke-based drawing input.

        output_type : {'probability', 'class', 'probability_vector'}, optional
            Form of the predictions which are one of:

            - 'class': Class prediction. For multi-class classification, this
              returns the class with maximum probability.
            - 'probability': Prediction probability associated with the True
              class (not applicable for multi-class classification)
            - 'probability_vector': Prediction probability associated with each
              class as a vector. Label ordering is dictated by the ``classes``
              member variable.

        batch_size : int, optional
            If you are getting memory errors, try decreasing this value. If you
            have a powerful computer, increasing this value may improve
            performance.

        verbose : bool, optional
            If True, prints prediction progress.

        Returns
        -------
        out : SArray
            An SArray with model predictions. Each element corresponds to
            a drawing and contains a single value corresponding to the
            predicted label. Each prediction will have type integer or string
            depending on the type of the classes the model was trained on.
            If `data` is a single drawing, the return value will be a single
            prediction.

        See Also
        --------
        evaluate

        Examples
        --------
        .. sourcecode:: python

            # Make predictions
            >>> pred = model.predict(data)

            # Print predictions, for a better overview
            >>> print(pred)
            dtype: int
            Rows: 10
            [3, 4, 3, 3, 4, 5, 8, 8, 8, 4]
        """
        if isinstance(dataset, _tc.SArray):
            dataset = _tc.SFrame({self.feature: dataset})
        return self.__proxy__.predict(dataset, output_type)

    def predict_topk(self, dataset, output_type="probability", k=3):
        """
        Return top-k predictions for the ``dataset``, using the trained model.
        Predictions are returned as an SFrame with three columns: `id`,
        `class`, and `probability` or `rank`, depending on the ``output_type``
        parameter.

        Parameters
        ----------
        dataset : SFrame | SArray | turicreate.Image
            Drawings to be classified.
            If dataset is an SFrame, it must include columns with the same
            names as the features used for model training, but does not require
            a target column. Additional columns are ignored.

        output_type : {'probability', 'rank'}, optional
            Choose the return type of the prediction:

            - `probability`: Probability associated with each label in the
                             prediction.
            - `rank`       : Rank associated with each label in the prediction.

        k : int, optional
            Number of classes to return for each input example.

        batch_size : int, optional
            If you are getting memory errors, try decreasing this value. If you
            have a powerful computer, increasing this value may improve
            performance.

        Returns
        -------
        out : SFrame
            An SFrame with model predictions.

        See Also
        --------
        predict, evaluate

        Examples
        --------
        >>> pred = m.predict_topk(validation_data, k=3)
        >>> print(pred)
        +----+-------+-------------------+
        | id | class |   probability     |
        +----+-------+-------------------+
        | 0  |   4   |   0.995623886585  |
        | 0  |   9   |  0.0038311756216  |
        | 0  |   7   | 0.000301006948575 |
        | 1  |   1   |   0.928708016872  |
        | 1  |   3   |  0.0440889261663  |
        | 1  |   2   |  0.0176190119237  |
        | 2  |   3   |   0.996967732906  |
        | 2  |   2   |  0.00151345680933 |
        | 2  |   7   | 0.000637513934635 |
        | 3  |   1   |   0.998070061207  |
        | .. |  ...  |        ...        |
        +----+-------+-------------------+
        [35688 rows x 3 columns]
        """
        if isinstance(dataset, _tc.SArray):
            dataset = _tc.SFrame({self.feature: dataset})
        return self.__proxy__.predict_topk(dataset, output_type, k)

    def evaluate(self, dataset, metric="auto"):
        """
        Evaluate the model by making predictions of target values and comparing
        these to actual values.

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the session_id, target and features used for model training.
            Additional columns are ignored.

        metric : str, optional
            Name of the evaluation metric.  Possible values are:

            - 'auto'             : Returns all available metrics.
            - 'accuracy'         : Classification accuracy (micro average).
            - 'auc'              : Area under the ROC curve (macro average)
            - 'precision'        : Precision score (macro average)
            - 'recall'           : Recall score (macro average)
            - 'f1_score'         : F1 score (macro average)
            - 'log_loss'         : Log loss
            - 'confusion_matrix' : An SFrame with counts of possible
                                   prediction/true label combinations.
            - 'roc_curve'        : An SFrame containing information needed for an
                                   ROC curve

        Returns
        -------
        out : dict
            Dictionary of evaluation results where the key is the name of the
            evaluation metric (e.g. `accuracy`) and the value is the evaluation
            score.

        See Also
        ----------
        create, predict

        Examples
        ----------
        .. sourcecode:: python

          >>> results = model.evaluate(data)
          >>> print results['accuracy']
        """

        evaluation_result = self.__proxy__.evaluate(dataset, metric)

        class_label = evaluation_result["prediction_class"]
        probability_vector = evaluation_result["prediction_prob"]

        del evaluation_result["prediction_class"]
        del evaluation_result["prediction_prob"]

        predicted = _tc.SFrame(
            {"label": class_label, "probability": probability_vector}
        )
        labels = self.classes

        from .._evaluate_utils import (
            entropy,
            confidence,
            relative_confidence,
            get_confusion_matrix,
            hclusterSort,
            l2Dist,
        )

        evaluation_result["num_test_examples"] = len(dataset)
        for k in ["num_classes", "num_examples", "training_time", "max_iterations"]:
            evaluation_result[k] = getattr(self, k)

        # evaluation_result['input_image_shape'] = getattr(self, 'input_image_shape')

        evaluation_result["model_name"] = "Drawing Classifier"
        extended_test = dataset.add_column(predicted["probability"], "probs")
        extended_test["label"] = dataset[self.target]

        extended_test = extended_test.add_columns(
            [
                extended_test.apply(
                    lambda d: labels[d["probs"].index(confidence(d["probs"]))]
                ),
                extended_test.apply(lambda d: entropy(d["probs"])),
                extended_test.apply(lambda d: confidence(d["probs"])),
                extended_test.apply(lambda d: relative_confidence(d["probs"])),
            ],
            ["predicted_label", "entropy", "confidence", "relative_confidence"],
        )

        extended_test = extended_test.add_column(
            extended_test.apply(lambda d: d["label"] == d["predicted_label"]), "correct"
        )

        sf_conf_mat = get_confusion_matrix(extended_test, labels)
        confidence_threshold = 0.5
        hesitant_threshold = 0.2
        evaluation_result["confidence_threshold"] = confidence_threshold
        evaluation_result["hesitant_threshold"] = hesitant_threshold
        evaluation_result["confidence_metric_for_threshold"] = "relative_confidence"

        evaluation_result["conf_mat"] = list(sf_conf_mat)

        vectors = map(
            lambda l: {
                "name": l,
                "pos": list(
                    sf_conf_mat[sf_conf_mat["target_label"] == l].sort(
                        "predicted_label"
                    )["norm_prob"]
                ),
            },
            labels,
        )
        evaluation_result["sorted_labels"] = hclusterSort(vectors, l2Dist)[0][
            "name"
        ].split("|")

        per_l = extended_test.groupby(
            ["label"],
            {
                "count": _tc.aggregate.COUNT,
                "correct_count": _tc.aggregate.SUM("correct"),
            },
        )
        per_l["recall"] = per_l.apply(lambda l: l["correct_count"] * 1.0 / l["count"])

        per_pl = extended_test.groupby(
            ["predicted_label"],
            {
                "predicted_count": _tc.aggregate.COUNT,
                "correct_count": _tc.aggregate.SUM("correct"),
            },
        )
        per_pl["precision"] = per_pl.apply(
            lambda l: l["correct_count"] * 1.0 / l["predicted_count"]
        )
        per_pl = per_pl.rename({"predicted_label": "label"})
        evaluation_result["label_metrics"] = list(
            per_l.join(per_pl, on="label", how="outer").select_columns(
                [
                    "label",
                    "count",
                    "correct_count",
                    "predicted_count",
                    "recall",
                    "precision",
                ]
            )
        )
        evaluation_result["labels"] = labels

        extended_test = extended_test.add_row_number("__idx").rename(
            {"label": "target_label"}
        )

        evaluation_result["test_data"] = extended_test
        evaluation_result["feature"] = self.feature

        return _Evaluation(evaluation_result)

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
            ("Feature column", "feature"),
            ("Target column", "target"),
        ]
        training_fields = [
            ("Training Iterations", "max_iterations"),
            ("Training Accuracy", "training_accuracy"),
            ("Validation Accuracy", "validation_accuracy"),
            ("Training Time", "training_time"),
            ("Number of Examples", "num_examples"),
        ]

        section_titles = ["Schema", "Training summary"]
        return ([model_fields, training_fields], section_titles)

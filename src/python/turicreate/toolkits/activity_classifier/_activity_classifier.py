# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Class definition and utilities for the activity classification toolkit.
"""
from __future__ import absolute_import as _
from __future__ import print_function as _
from __future__ import division as _

import numpy as _np
import time as _time
import six as _six

from turicreate import SArray as _SArray, SFrame as _SFrame
from turicreate import aggregate as _agg

import turicreate.toolkits._internal_utils as _tkutl
from turicreate.toolkits import _coreml_utils
import turicreate.toolkits._feature_engineering._internal_utils as _fe_tkutl
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from turicreate.toolkits import evaluation as _evaluation

from turicreate.toolkits._model import CustomModel as _CustomModel
from turicreate.toolkits._model import Model as _Model
from turicreate.toolkits._model import PythonProxy as _PythonProxy

from .util import random_split_by_session as _random_split_by_session
from .util import _MIN_NUM_SESSIONS_FOR_SPLIT


def create(
    dataset,
    session_id,
    target,
    features=None,
    prediction_window=100,
    validation_set="auto",
    max_iterations=10,
    batch_size=32,
    verbose=True,
    random_seed=None,
):
    """
    Create an :class:`ActivityClassifier` model.

    Parameters
    ----------
    dataset : SFrame
        Input data which consists of `sessions` of data where each session is
        a sequence of data. The data must be in `stacked` format, grouped by
        session. Within each session, the data is assumed to be sorted
        temporally. Columns in `features` will be used to train a model that
        will make a prediction using labels in the `target` column.

    session_id : string
        Name of the column that contains a unique ID for each session.

    target : string
        Name of the column containing the target variable. The values in this
        column must be of string or integer type. Use `model.classes` to
        retrieve the order in which the classes are mapped.

    features : list[string], optional
        Name of the columns containing the input features that will be used
        for classification. If set to `None`, all columns except `session_id`
        and `target` will be used.

    prediction_window : int, optional
        Number of time units between predictions. For example, if your input
        data is sampled at 100Hz, and the `prediction_window` is set to 100,
        then this model will make a prediction every 1 second.

    validation_set : SFrame, optional
        A dataset for monitoring the model's generalization performance to
        prevent the model from overfitting to the training data.

        For each row of the progress table, accuracy is measured over the
        provided training dataset and the `validation_set`. The format of this
        SFrame must be the same as the training set.

        When set to 'auto', a validation set is automatically sampled from the
        training data (if the training data has > 100 sessions). If
        validation_set is set to None, then all the data will be used for
        training.

    max_iterations : int , optional
        Maximum number of iterations/epochs made over the data during the
        training phase.

    batch_size : int, optional
        Number of sequence chunks used per training step. Must be greater than
        the number of GPUs in use.

    verbose : bool, optional
        If True, print progress updates and model details.

    random_seed : int, optional
        The results can be reproduced when given the same seed.

    Returns
    -------
    out : ActivityClassifier
        A trained :class:`ActivityClassifier` model.

    Examples
    --------
    .. sourcecode:: python

        >>> import turicreate as tc

        # Training on dummy data
        >>> data = tc.SFrame({
        ...    'accelerometer_x': [0.1, 0.2, 0.3, 0.4, 0.5] * 10,
        ...    'accelerometer_y': [0.5, 0.4, 0.3, 0.2, 0.1] * 10,
        ...    'accelerometer_z': [0.01, 0.01, 0.02, 0.02, 0.01] * 10,
        ...    'session_id': [0, 0, 0] * 10 + [1, 1] * 10,
        ...    'activity': ['walk', 'run', 'run'] * 10 + ['swim', 'swim'] * 10
        ... })

        # Create an activity classifier
        >>> model = tc.activity_classifier.create(data,
        ...     session_id='session_id', target='activity',
        ...     features=['accelerometer_x', 'accelerometer_y', 'accelerometer_z'])

        # Make predictions (as probability vector, or class)
        >>> predictions = model.predict(data)
        >>> predictions = model.predict(data, output_type='probability_vector')

        # Get both predictions and classes together
        >>> predictions = model.classify(data)

        # Get topk predictions (instead of only top-1) if your labels have more
        # 2 classes
        >>> predictions = model.predict_topk(data, k = 3)

        # Evaluate the model
        >>> results = model.evaluate(data)

    See Also
    --------
    ActivityClassifier, util.random_split_by_session
    """

    _tkutl._raise_error_if_not_sframe(dataset, "dataset")
    if not isinstance(target, str):
        raise _ToolkitError("target must be of type str")
    if not isinstance(session_id, str):
        raise _ToolkitError("session_id must be of type str")
    if not isinstance(batch_size, int):
        raise _ToolkitError("batch_size must be of type int")

    _tkutl._raise_error_if_sframe_empty(dataset, "dataset")
    _tkutl._numeric_param_check_range("prediction_window", prediction_window, 1, 400)
    _tkutl._numeric_param_check_range("max_iterations", max_iterations, 0, _six.MAXSIZE)

    if features is None:
        features = _fe_tkutl.get_column_names(
            dataset, interpret_as_excluded=True, column_names=[session_id, target]
        )
    if not hasattr(features, "__iter__"):
        raise TypeError("Input 'features' must be a list.")
    if not all([isinstance(x, str) for x in features]):
        raise TypeError("Invalid feature %s: Feature names must be of type str." % x)
    if len(features) == 0:
        raise TypeError("Input 'features' must contain at least one column name.")

    start_time = _time.time()
    dataset = _tkutl._toolkits_select_columns(dataset, features + [session_id, target])
    _tkutl._raise_error_if_sarray_not_expected_dtype(
        dataset[target], target, [str, int]
    )
    _tkutl._raise_error_if_sarray_not_expected_dtype(
        dataset[session_id], session_id, [str, int]
    )

    for feature in features:
        _tkutl._handle_missing_values(dataset, feature, "training_dataset")

    # Check for missing values for sframe validation set
    if isinstance(validation_set, _SFrame):
        _tkutl._raise_error_if_sframe_empty(validation_set, "validation_set")
        for feature in features:
            _tkutl._handle_missing_values(validation_set, feature, "validation_set")

    # C++ model
    name = "activity_classifier"

    import turicreate as _turicreate

    # Imports tensorflow
    import turicreate.toolkits.libtctensorflow

    model = _turicreate.extensions.activity_classifier()
    options = {}
    options["prediction_window"] = prediction_window
    options["batch_size"] = batch_size
    options["max_iterations"] = max_iterations
    options["verbose"] = verbose
    options["_show_loss"] = False
    options["random_seed"] = random_seed

    model.train(dataset, target, session_id, validation_set, options)
    return ActivityClassifier(model_proxy=model, name=name)

def _encode_target(data, target, mapping=None):
    """ Encode targets to integers in [0, num_classes - 1] """
    if mapping is None:
        mapping = {t: i for i, t in enumerate(sorted(data[target].unique()))}

    data[target] = data[target].apply(lambda t: mapping[t])
    return data, mapping


class ActivityClassifier(_Model):
    """
    A trained model using C++ implementation that is ready to use for classification or export to
    CoreML.

    This model should not be constructed directly.
    """

    _CPP_ACTIVITY_CLASSIFIER_VERSION = 1

    def __init__(self, model_proxy=None, name=None):
        self.__proxy__ = model_proxy
        self.__name__ = name

    @classmethod
    def _native_name(cls):
        return "activity_classifier"

    def __str__(self):
        """
        Return a string description of the model to the ``print`` method.

        Returns
        -------
        out : string
            A description of the ActivityClassifier.
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
        return self._CPP_ACTIVITY_CLASSIFIER_VERSION

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
        short_description = _coreml_utils._mlmodel_short_description(
            "Activity classifier"
        )
        additional_user_defined_metadata = _coreml_utils._get_tc_version_info()
        self.__proxy__.export_to_coreml(
            filename, short_description, additional_user_defined_metadata
        )

    def predict(self, dataset, output_type="class", output_frequency="per_row"):
        """
        Return predictions for ``dataset``, using the trained activity classifier.
        Predictions can be generated as class labels, or as a probability
        vector with probabilities for each class.

        The activity classifier generates a single prediction for each
        ``prediction_window`` rows in ``dataset``, per ``session_id``. The number
        of these predictions is smaller than the length of ``dataset``. By default,
        when ``output_frequency='per_row'``, each prediction is repeated ``prediction_window`` to return
        a prediction for each row of ``dataset``. Use ``output_frequency=per_window`` to
        get the unreplicated predictions.

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the features used for model training, but does not require
            a target column. Additional columns are ignored.

        output_type : {'class', 'probability_vector'}, optional
            Form of each prediction which is one of:

            - 'probability_vector': Prediction probability associated with each
              class as a vector. The probability of the first class (sorted
              alphanumerically by name of the class in the training set) is in
              position 0 of the vector, the second in position 1 and so on.
            - 'class': Class prediction. This returns the class with maximum
              probability.

        output_frequency : {'per_row', 'per_window'}, optional
            The frequency of the predictions which is one of:

            - 'per_window': Return a single prediction for each
              ``prediction_window`` rows in ``dataset`` per ``session_id``.
            - 'per_row': Convenience option to make sure the number of
              predictions match the number of rows in the dataset. Each
              prediction from the model is repeated ``prediction_window``
              times during that window.

        Returns
        -------
        out : SArray | SFrame
            If ``output_frequency`` is 'per_row' return an SArray with predictions
            for each row in ``dataset``.
            If ``output_frequency`` is 'per_window' return an SFrame with
            predictions for ``prediction_window`` rows in ``dataset``.

        See Also
        ----------
        create, evaluate, classify

        Examples
        --------

        .. sourcecode:: python

            # One prediction per row
            >>> probability_predictions = model.predict(
            ...     data, output_type='probability_vector', output_frequency='per_row')[:4]
            >>> probability_predictions

            dtype: array
            Rows: 4
            [array('d', [0.01857384294271469, 0.0348394550383091, 0.026018327102065086]),
             array('d', [0.01857384294271469, 0.0348394550383091, 0.026018327102065086]),
             array('d', [0.01857384294271469, 0.0348394550383091, 0.026018327102065086]),
             array('d', [0.01857384294271469, 0.0348394550383091, 0.026018327102065086])]

            # One prediction per window
            >>> class_predictions = model.predict(
            ...     data, output_type='class', output_frequency='per_window')
            >>> class_predictions

            +---------------+------------+-----+
            | prediction_id | session_id |class|
            +---------------+------------+-----+
            |       0       |     3      |  5  |
            |       1       |     3      |  5  |
            |       2       |     3      |  5  |
            |       3       |     3      |  5  |
            |       4       |     3      |  5  |
            |       5       |     3      |  5  |
            |       6       |     3      |  5  |
            |       7       |     3      |  4  |
            |       8       |     3      |  4  |
            |       9       |     3      |  4  |
            |      ...      |    ...     | ... |
            +---------------+------------+-----+
        """
        _tkutl._check_categorical_option_type(
            "output_frequency", output_frequency, ["per_window", "per_row"]
        )
        if output_frequency == "per_row":
            return self.__proxy__.predict(dataset, output_type)
        elif output_frequency == "per_window":
            return self.__proxy__.predict_per_window(dataset, output_type)

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
        return self.__proxy__.evaluate(dataset, metric)

    def predict_topk(
        self, dataset, output_type="probability", k=3, output_frequency="per_row"
    ):
        """
        Return top-k predictions for the ``dataset``, using the trained model.
        Predictions are returned as an SFrame with three columns: `prediction_id`,
        `class`, and `probability`, or `rank`, depending on the ``output_type``
        parameter.

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the features and session id used for model training, but
            does not require a target column. Additional columns are ignored.

        output_type : {'probability', 'rank'}, optional
            Choose the return type of the prediction:

            - `probability`: Probability associated with each label in the prediction.
            - `rank`       : Rank associated with each label in the prediction.

        k : int, optional
            Number of classes to return for each input example.

        output_frequency : {'per_row', 'per_window'}, optional
            The frequency of the predictions which is one of:

            - 'per_row': Each prediction is returned ``prediction_window`` times.
            - 'per_window': Return a single prediction for each
              ``prediction_window`` rows in ``dataset`` per ``session_id``.

        Returns
        -------
        out : SFrame
            An SFrame with model predictions.

        See Also
        --------
        predict, classify, evaluate

        Examples
        --------
        >>> pred = m.predict_topk(validation_data, k=3)
        >>> pred
        +---------------+-------+-------------------+
        |     row_id    | class |    probability    |
        +---------------+-------+-------------------+
        |       0       |   4   |   0.995623886585  |
        |       0       |   9   |  0.0038311756216  |
        |       0       |   7   | 0.000301006948575 |
        |       1       |   1   |   0.928708016872  |
        |       1       |   3   |  0.0440889261663  |
        |       1       |   2   |  0.0176190119237  |
        |       2       |   3   |   0.996967732906  |
        |       2       |   2   |  0.00151345680933 |
        |       2       |   7   | 0.000637513934635 |
        |       3       |   1   |   0.998070061207  |
        |      ...      |  ...  |        ...        |
        +---------------+-------+-------------------+
        """
        if not isinstance(k, int):
            raise TypeError("k must be of type int")
        _tkutl._numeric_param_check_range("k", k, 1, _six.MAXSIZE)
        return self.__proxy__.predict_topk(dataset, output_type, k, output_frequency)

    def classify(self, dataset, output_frequency="per_row"):
        """
        Return a classification, for each ``prediction_window`` examples in the
        ``dataset``, using the trained activity classification model. The output
        SFrame contains predictions as both class labels as well as probabilities
        that the predicted value is the associated label.

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the features and session id used for model training, but
            does not require a target column. Additional columns are ignored.

        output_frequency : {'per_row', 'per_window'}, optional
            The frequency of the predictions which is one of:

            - 'per_row': Each prediction is returned ``prediction_window`` times.
            - 'per_window': Return a single prediction for each
              ``prediction_window`` rows in ``dataset`` per ``session_id``.

        Returns
        -------
        out : SFrame
            An SFrame with model predictions i.e class labels and probabilities.

        See Also
        ----------
        create, evaluate, predict

        Examples
        ----------
        >>> classes = model.classify(data)
        """
        return self.__proxy__.classify(dataset, output_frequency)

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
            ("Number of examples", "num_examples"),
            ("Number of sessions", "num_sessions"),
            ("Number of classes", "num_classes"),
            ("Number of feature columns", "num_features"),
            ("Prediction window", "prediction_window"),
        ]
        training_fields = [
            ("Log-likelihood", "training_log_loss"),
            ("Training time (sec)", "training_time"),
        ]

        section_titles = ["Schema", "Training summary"]
        return ([model_fields, training_fields], section_titles)

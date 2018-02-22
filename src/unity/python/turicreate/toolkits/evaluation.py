# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
The evaluation module includes performance metrics to evaluate machine learning
models. The metrics can be broadly categorized as:
- Classification metrics
- Regression metrics

The evaluation module supports the following classification metrics:
- accuracy
- confusion_matrix
- f1_score
- fbeta_score
- log_loss
- precision
- recall
- roc_curve

The evaluation module supports the following regression metrics:
- rmse
- max_error
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _turicreate
from ..deps import numpy
from turicreate.toolkits._internal_utils import _raise_error_if_not_sarray,\
                                              _check_categorical_option_type
from turicreate.toolkits._main import ToolkitError as _ToolkitError

def _check_prob_and_prob_vector(predictions):
    """
    Check that the predictionsa are either probabilities of prob-vectors.
    """
    ptype = predictions.dtype
    import array
    if ptype not in [float, numpy.ndarray, array.array, int]:
        err_msg  = "Input `predictions` must be of numeric type (for binary "
        err_msg += "classification) or array (of probability vectors) for "
        err_msg += "multiclass classification."
        raise TypeError(err_msg)

def _supervised_evaluation_error_checking(targets, predictions):
    """
    Perform basic error checking for the evaluation metrics. Check
    types and sizes of the inputs.
    """
    _raise_error_if_not_sarray(targets, "targets")
    _raise_error_if_not_sarray(predictions, "predictions")
    if (len(targets) != len(predictions)):
        raise _ToolkitError(
         "Input SArrays 'targets' and 'predictions' must be of the same length.")

# The ignore_float_check is because of [None, None, None] being cast as float :(
def _check_same_type_not_float(targets, predictions, ignore_float_check = False):
    if not ignore_float_check:
        if targets.dtype == float:
            raise TypeError("Input `targets` cannot be an SArray of type float.")
        if predictions.dtype == float:
            raise TypeError("Input `predictions` cannot be an SArray of type float.")
    if targets.dtype != predictions.dtype:
        raise TypeError("Inputs SArrays `targets` and `predictions` must be of the same type.")

def _check_target_not_float(targets):
    if targets.dtype == float:
        raise TypeError("Input `targets` cannot be an SArray of type float.")

def log_loss(targets, predictions):
    r"""
    Compute the logloss for the given targets and the given predicted
    probabilities. This quantity is defined to be the negative of the sum
    of the log probability of each observation, normalized by the number of
    observations:

    .. math::

        \textrm{logloss} = - \frac{1}{N} \sum_{i \in 1,\ldots,N}
            (y_i \log(p_i) + (1-y_i)\log(1-p_i)) ,

    where y_i is the i'th target value and p_i is the i'th predicted
    probability.

    For multiclass situations, the definition is a slight generalization of the
    above:

    .. math::

        \textrm{logloss} = - \frac{1}{N} \sum_{i \in 1,\ldots,N}
            \sum_{j \in 1, \ldots, L}
            (y_{ij} \log(p_{ij})) ,

    where :math:`L` is the number of classes and :math:`y_{ij}` indicates that
    observation `i` has class label `j`.

    Parameters
    ----------
    targets : SArray
        Ground truth class labels. This can either contain integers or strings.

    predictions : SArray
        The predicted probability that corresponds to each target value. For
        binary classification, the probability corresponds to the probability
        of the "positive" label being predicted. For multi-class
        classification, the predictions are expected to be an array of
        predictions for each class.

    Returns
    -------
    out : float
        The log_loss.

    See Also
    --------
    accuracy

    Notes
    -----
     - For binary classification, when the target label is of type "string",
       then the labels are sorted alphanumerically and the largest label is
       chosen as the "positive" label.  For example, if the classifier labels
       are {"cat", "dog"}, then "dog" is chosen as the positive label for the
       binary classification case.
     - For multi-class classification, when the target label is of type
       "string", then the probability vector is assumed to be a vector of
       probabilities of classes as sorted alphanumerically. Hence, for the
       probability vector [0.1, 0.2, 0.7] for a dataset with classes "cat",
       "dog", and "rat"; the 0.1 corresponds to "cat", the 0.2 to "dog" and the
       0.7 to "rat".
     - Logloss is undefined when a probability value p = 0, or p = 1.  Hence,
       probabilities are clipped to max(EPSILON, min(1 - EPSILON, p)) where
       EPSILON = 1e-15.

    References
    ----------
    https://www.kaggle.com/wiki/LogLoss

    Examples
    --------
    .. sourcecode:: python

        import turicreate as tc
        targets = tc.SArray([0, 1, 1, 0])
        predictions = tc.SArray([0.1, 0.35, 0.7, 0.99])
        log_loss = tc.evaluation.log_loss(targets, predictions)

    For binary classification, when the target label is of type "string", then
    the labels are sorted alphanumerically and the largest label is chosen as
    the "positive" label.

    .. sourcecode:: python

        import turicreate as tc
        targets = tc.SArray(["cat", "dog", "dog", "cat"])
        predictions = tc.SArray([0.1, 0.35, 0.7, 0.99])
        log_loss = tc.evaluation.log_loss(targets, predictions)

    In the multi-class setting, log-loss requires a vector of probabilities
    (that sum to 1) for each class label in the input dataset. In this example,
    there are three classes [0, 1, 2], and the vector of probabilities
    correspond to the probability of prediction for each of the three classes.

    .. sourcecode:: python

        target    = tc.SArray([ 1, 0, 2, 1])
        predictions = tc.SArray([[.1, .8, 0.1],
                                [.9, .1, 0.0],
                                [.8, .1, 0.1],
                                [.3, .6, 0.1]])
        log_loss = tc.evaluation.log_loss(targets, predictions)

    For multi-class classification, when the target label is of type "string",
    then the probability vector is assumed to be a vector of probabilities of
    class as sorted alphanumerically. Hence, for the probability vector [0.1,
    0.2, 0.7] for a dataset with classes "cat", "dog", and "rat"; the 0.1
    corresponds to "cat", the 0.2 to "dog" and the 0.7 to "rat".

    .. sourcecode:: python

        target    = tc.SArray([ "dog", "cat", "foosa", "dog"])
        predictions = tc.SArray([[.1, .8, 0.1],
                                [.9, .1, 0.0],
                                [.8, .1, 0.1],
                                [.3, .6, 0.1]])
        log_loss = tc.evaluation.log_loss(targets, predictions)

    """

    _supervised_evaluation_error_checking(targets, predictions)
    _check_prob_and_prob_vector(predictions)
    _check_target_not_float(targets)
    multiclass = predictions.dtype not in [float, int]

    if multiclass:
        result = _turicreate.extensions._supervised_streaming_evaluator(targets,
            predictions, "multiclass_logloss", {})
    else:
        result = _turicreate.extensions._supervised_streaming_evaluator(targets,
            predictions, "binary_logloss", {})
    return result


def max_error(targets, predictions):
    r"""
    Compute the maximum absolute deviation between two SArrays.

    Parameters
    ----------
    targets : SArray[float or int]
        An Sarray of ground truth target values.

    predictions : SArray[float or int]
        The prediction that corresponds to each target value.
        This vector must have the same length as ``targets``.

    Returns
    -------
    out : float
        The maximum absolute deviation error between the two SArrays.

    See Also
    --------
    rmse

    Notes
    -----
    The maximum absolute deviation between two vectors, x and y, is defined as:

    .. math::

        \textrm{max error} = \max_{i \in 1,\ldots,N} \|x_i - y_i\|

    Examples
    --------
    >>> targets = turicreate.SArray([3.14, 0.1, 50, -2.5])
    >>> predictions = turicreate.SArray([3.1, 0.5, 50.3, -5])
    >>> turicreate.evaluation.max_error(targets, predictions)
    2.5
    """

    _supervised_evaluation_error_checking(targets, predictions)
    return _turicreate.extensions._supervised_streaming_evaluator(targets,
                                  predictions, "max_error", {})

def rmse(targets, predictions):
    r"""
    Compute the root mean squared error between two SArrays.

    Parameters
    ----------
    targets : SArray[float or int]
        An Sarray of ground truth target values.

    predictions : SArray[float or int]
        The prediction that corresponds to each target value.
        This vector must have the same length as ``targets``.

    Returns
    -------
    out : float
        The RMSE between the two SArrays.

    See Also
    --------
    max_error

    Notes
    -----
    The root mean squared error between two vectors, x and y, is defined as:

    .. math::

        RMSE = \sqrt{\frac{1}{N} \sum_{i=1}^N (x_i - y_i)^2}

    References
    ----------
    - `Wikipedia - root-mean-square deviation
      <http://en.wikipedia.org/wiki/Root-mean-square_deviation>`_

    Examples
    --------
    >>> targets = turicreate.SArray([3.14, 0.1, 50, -2.5])
    >>> predictions = turicreate.SArray([3.1, 0.5, 50.3, -5])

    >>> turicreate.evaluation.rmse(targets, predictions)
    1.2749117616525465
    """

    _supervised_evaluation_error_checking(targets, predictions)
    return _turicreate.extensions._supervised_streaming_evaluator(targets,
                                       predictions, "rmse", {})
def confusion_matrix(targets, predictions):
    r"""
    Compute the confusion matrix for classifier predictions.

    Parameters
    ----------
    targets : SArray
        Ground truth class labels (cannot be of type float).

    predictions : SArray
        The prediction that corresponds to each target value.
        This vector must have the same length as ``targets``. The predictions
        SArray cannot be of type float.

    Returns
    -------
    out : SFrame
        An SFrame containing counts for 'target_label', 'predicted_label' and
        'count' corresponding to each pair of true and predicted labels.

    See Also
    --------
    accuracy

    Examples
    --------
    >>> targets = turicreate.SArray([0, 1, 1, 0])
    >>> predictions = turicreate.SArray([1, 0, 1, 0])

    >>> turicreate.evaluation.confusion_matrix(targets, predictions)
    """

    _supervised_evaluation_error_checking(targets, predictions)
    _check_same_type_not_float(targets, predictions)
    return _turicreate.extensions._supervised_streaming_evaluator(targets,
                       predictions, "confusion_matrix_no_map", {})

def accuracy(targets, predictions, average='micro'):
    r"""
    Compute the accuracy score; which measures the fraction of predictions made
    by the classifier that are exactly correct. The score lies in the range [0,1]
    with 0 being the worst and 1 being the best.

    Parameters
    ----------
    targets : SArray
        An SArray of ground truth class labels. Can be of any type except
        float.

    predictions : SArray
        The prediction that corresponds to each target value.  This SArray must
        have the same length as ``targets`` and must be of the same type as the
        ``targets`` SArray.


    average : string, [None, 'micro' (default), 'macro']
        Metric averaging strategies for multiclass classification. Averaging
        strategies can be one of the following:

            - None: No averaging is performed and a single metric is returned
              for each class.
            - 'micro': Calculate metrics globally by counting the total true
              positives, false negatives and false positives.
            - 'macro': Calculate metrics for each label, and find their
              unweighted mean. This does not take label imbalance into account.

        For a more precise definition of `micro` and `macro` averaging refer
        to [1] below.

    Returns
    -------
    out : float (for binary classification) or dict[float] (for multi-class, average=None)
        Score for the positive class (for binary classification) or an average
        score for each class for multi-class classification.  If
        `average=None`, then a dictionary is returned where the key is the
        class label and the value is the score for the corresponding class
        label.

    See Also
    --------
    confusion_matrix, precision, recall, f1_score, auc, log_loss, roc_curve

    Examples
    --------

    .. sourcecode:: python

        # Targets and Predictions
        >>> targets = turicreate.SArray([0, 1, 2, 3, 0, 1, 2, 3])
        >>> predictions = turicreate.SArray([1, 0, 2, 1, 3, 1, 0, 1])

        # Micro average of the accuracy score.
        >>> turicreate.evaluation.accuracy(targets, predictions, average = 'micro')
        0.25

        # Macro average of the accuracy score.
        >>> turicreate.evaluation.accuracy(targets, predictions, average = 'macro')
        0.24305555555555558

        # Accuracy score for each class.
        >>> turicreate.evaluation.accuracy(targets, predictions, average = None)
        {0: 0.0, 1: 0.4166666666666667, 2: 0.5555555555555556, 3: 0.0}

    This metric also works when the targets are of type `str`

    .. sourcecode:: python

        # Targets and Predictions
        >>> targets = turicreate.SArray(["cat", "dog", "foosa", "cat", "dog"])
        >>> predictions   = turicreate.SArray(["cat", "foosa", "dog", "cat", "foosa"])

        # Micro average of the accuracy score.
        >>> turicreate.evaluation.accuracy(targets, predictions, average = 'micro')
        0.4

        # Macro average of the accuracy score.
        >>> turicreate.evaluation.accuracy(targets, predictions, average = 'macro')
        0.6

        # Accuracy score for each class.
        >>> turicreate.evaluation.accuracy(targets, predictions, average = None)
        {'cat': 1.0, 'dog': 0.4, 'foosa': 0.4}

    References
    ----------
    - [1] Sokolova, Marina, and Guy Lapalme. "A systematic analysis of
      performance measures for classification tasks." Information Processing &
      Management 45.4 (2009): 427-437.

    """
    _supervised_evaluation_error_checking(targets, predictions)
    _check_same_type_not_float(targets, predictions)
    opts = {"average": average}
    return _turicreate.extensions._supervised_streaming_evaluator(targets,
                          predictions, "flexible_accuracy", opts)


def fbeta_score(targets, predictions, beta=1.0, average='macro'):
    r"""
    Compute the F-beta score. The F-beta score is the weighted harmonic mean of
    precision and recall. The score lies in the range [0,1] with 1 being ideal
    and 0 being the worst.

    The `beta` value is the weight given to `precision` vs `recall` in the
    combined score. `beta=0` considers only precision, as `beta` increases, more
    weight is given to recall with `beta > 1` favoring recall over precision.

    The F-beta score is defined as:

        .. math::
            f_{\beta} = (1 + \beta^2) \times \frac{(p \times r)}{(\beta^2 p + r)}

    Where :math:`p` is the precision and :math:`r` is the recall.

    Parameters
    ----------
    targets : SArray
        An SArray of ground truth class labels. Can be of any type except
        float.

    predictions : SArray
        The prediction that corresponds to each target value.  This SArray must
        have the same length as ``targets`` and must be of the same type
        as the ``targets`` SArray.

    beta: float
        Weight of the `precision` term in the harmonic mean.

    average : string, [None, 'macro' (default), 'micro']
        Metric averaging strategies for multiclass classification. Averaging
        strategies can be one of the following:

            - None: No averaging is performed and a single metric is returned
              for each class.
            - 'micro': Calculate metrics globally by counting the total true
              positives, false negatives and false positives.
            - 'macro': Calculate metrics for each label, and find their
              unweighted mean. This does not take label imbalance into account.

        For a more precise definition of `micro` and `macro` averaging refer
        to [1] below.

    Returns
    -------
    out : float (for binary classification) or dict[float] (for multi-class, average=None)
        Score for the positive class (for binary classification) or an average
        score for each class for multi-class classification.  If
        `average=None`, then a dictionary is returned where the key is the
        class label and the value is the score for the corresponding class
        label.

    Notes
    -----
     - For binary classification, if the target label is of type "string",
       then the labels are sorted alphanumerically and the largest label is
       chosen as the "positive" label.  For example, if the classifier labels
       are {"cat", "dog"}, then "dog" is chosen as the positive label for the
       binary classification case.


    See Also
    --------
    confusion_matrix, accuracy, precision, recall, f1_score

    Examples
    --------

    .. sourcecode:: python

        # Targets and Predictions
        >>> targets = turicreate.SArray([0, 1, 2, 3, 0, 1, 2, 3])
        >>> predictions = turicreate.SArray([1, 0, 2, 1, 3, 1, 0, 1])

        # Micro average of the F-Beta score
        >>> turicreate.evaluation.fbeta_score(targets, predictions,
        ...                                 beta=2.0, average = 'micro')
        0.25

        # Macro average of the F-Beta score
        >>> turicreate.evaluation.fbeta_score(targets, predictions,
        ...                                 beta=2.0, average = 'macro')
        0.24305555555555558

        # F-Beta score for each class.
        >>> turicreate.evaluation.fbeta_score(targets, predictions,
        ...                                 beta=2.0, average = None)
        {0: 0.0, 1: 0.4166666666666667, 2: 0.5555555555555556, 3: 0.0}

    This metric also works when the targets are of type `str`

    .. sourcecode:: python

        # Targets and Predictions
        >>> targets = turicreate.SArray(
        ...      ["cat", "dog", "foosa", "snake", "cat", "dog", "foosa", "snake"])
        >>> predictions = turicreate.SArray(
        ...      ["dog", "cat", "foosa", "dog", "snake", "dog", "cat", "dog"])

        # Micro average of the F-Beta score
        >>> turicreate.evaluation.fbeta_score(targets, predictions,
        ...                                 beta=2.0, average = 'micro')
        0.25

        # Macro average of the F-Beta score
        >>> turicreate.evaluation.fbeta_score(targets, predictions,
        ...                                 beta=2.0, average = 'macro')
        0.24305555555555558

        # F-Beta score for each class.
        >>> turicreate.evaluation.fbeta_score(targets, predictions,
        ...                                 beta=2.0, average = None)
        {'cat': 0.0, 'dog': 0.4166666666666667, 'foosa': 0.5555555555555556, 'snake': 0.0}

    References
    ----------
    - [1] Sokolova, Marina, and Guy Lapalme. "A systematic analysis of
      performance measures for classification tasks." Information Processing &
      Management 45.4 (2009): 427-437.

    """
    _supervised_evaluation_error_checking(targets, predictions)
    _check_categorical_option_type('average', average,
                         ['micro', 'macro', None])
    _check_same_type_not_float(targets, predictions)

    opts = {"beta"    : beta,
            "average" : average}
    return _turicreate.extensions._supervised_streaming_evaluator(targets,
                          predictions, "fbeta_score", opts)

def f1_score(targets, predictions, average='macro'):
    r"""
    Compute the F1 score (sometimes known as the balanced F-score or
    F-measure). The F1 score is commonly interpreted as the average of
    precision and recall. The score lies in the range [0,1] with 1 being ideal
    and 0 being the worst.

    The F1 score is defined as:

        .. math::
            f_{1} = \frac{2 \times p \times r}{p + r}

    Where :math:`p` is the precision and :math:`r` is the recall.

    Parameters
    ----------
    targets : SArray
        An SArray of ground truth class labels. Can be of any type except
        float.

    predictions : SArray
        The class prediction that corresponds to each target value. This SArray
        must have the same length as ``targets`` and must be of the same type
        as the ``targets`` SArray.

    average : string, [None, 'macro' (default), 'micro']
        Metric averaging strategies for multiclass classification. Averaging
        strategies can be one of the following:

            - None: No averaging is performed and a single metric is returned
              for each class.
            - 'micro': Calculate metrics globally by counting the total true
              positives, false negatives and false positives.
            - 'macro': Calculate metrics for each label, and find their
              unweighted mean. This does not take label imbalance into account.

        For a more precise definition of `micro` and `macro` averaging refer
        to [1] below.

    Returns
    -------
    out : float (for binary classification) or dict[float] (multi-class, average=None)
        Score for the positive class (for binary classification) or an average
        score for each class for multi-class classification.  If
        `average=None`, then a dictionary is returned where the key is the
        class label and the value is the score for the corresponding class
        label.

    See Also
    --------
    confusion_matrix, accuracy, precision, recall, fbeta_score

    Notes
    -----
     - For binary classification, when the target label is of type "string",
       then the labels are sorted alphanumerically and the largest label is
       chosen as the "positive" label.  For example, if the classifier labels
       are {"cat", "dog"}, then "dog" is chosen as the positive label for the
       binary classification case.

    Examples
    --------

    .. sourcecode:: python

        # Targets and Predictions
        >>> targets = turicreate.SArray([0, 1, 2, 3, 0, 1, 2, 3])
        >>> predictions = turicreate.SArray([1, 0, 2, 1, 3, 1, 0, 1])

        # Micro average of the F-1 score
        >>> turicreate.evaluation.f1_score(targets, predictions,
        ...                              average = 'micro')
        0.25

        # Macro average of the F-1 score
        >>> turicreate.evaluation.f1_score(targets, predictions,
        ...                              average = 'macro')
        0.25

        # F-1 score for each class.
        >>> turicreate.evaluation.f1_score(targets, predictions,
        ...                              average = None)
        {0: 0.0, 1: 0.4166666666666667, 2: 0.5555555555555556, 3: 0.0}

    This metric also works for string classes.

    .. sourcecode:: python

        # Targets and Predictions
        >>> targets = turicreate.SArray(
        ...      ["cat", "dog", "foosa", "snake", "cat", "dog", "foosa", "snake"])
        >>> predictions = turicreate.SArray(
        ...      ["dog", "cat", "foosa", "dog", "snake", "dog", "cat", "dog"])

        # Micro average of the F-1 score
        >>> turicreate.evaluation.f1_score(targets, predictions,
        ...                              average = 'micro')
        0.25

        # Macro average of the F-1 score
        >>> turicreate.evaluation.f1_score(targets, predictions,
        ...                             average = 'macro')
        0.25

        # F-1 score for each class.
        >>> turicreate.evaluation.f1_score(targets, predictions,
        ...                              average = None)
        {'cat': 0.0, 'dog': 0.4166666666666667, 'foosa': 0.5555555555555556, 'snake': 0.0}

    References
    ----------
    - [1] Sokolova, Marina, and Guy Lapalme. "A systematic analysis of
      performance measures for classification tasks." Information Processing &
      Management 45.4 (2009): 427-437.

    """
    return fbeta_score(targets, predictions, beta = 1.0, average = average)

def precision(targets, predictions, average='macro'):
    r"""

    Compute the precision score for classification tasks. The precision score
    quantifies the ability of a classifier to not label a `negative` example as
    `positive`. The precision score can be interpreted as the probability that
    a `positive` prediction made by the classifier is `positive`. The score is
    in the range [0,1] with 0 being the worst, and 1 being perfect.


    The precision score is defined as the ratio:
        .. math::
            \frac{tp}{tp + fp}

    where `tp` is the number of true positives and `fp` the number of false
    positives.

    Parameters
    ----------
    targets : SArray
        Ground truth class labels.

    predictions : SArray
        The prediction that corresponds to each target value.  This SArray must
        have the same length as ``targets`` and must be of the same type
        as the ``targets`` SArray.

    average : string, [None, 'macro' (default), 'micro']
        Metric averaging strategies for multiclass classification. Averaging
        strategies can be one of the following:

            - None: No averaging is performed and a single metric is returned
              for each class.
            - 'micro': Calculate metrics globally by counting the total true
              positives, and false positives.
            - 'macro': Calculate metrics for each label, and find their
              unweighted mean. This does not take label imbalance into account.

    Returns
    -------
    out : float (for binary classification) or dict[float]
        Score for the positive class (for binary classification) or an average
        score for each class for multi-class classification.  If
        `average=None`, then a dictionary is returned where the key is the
        class label and the value is the score for the corresponding class
        label.

    Notes
    -----
     - For binary classification, when the target label is of type "string",
       then the labels are sorted alphanumerically and the largest label is
       chosen as the "positive" label.  For example, if the classifier labels
       are {"cat", "dog"}, then "dog" is chosen as the positive label for the
       binary classification case.

    See Also
    --------
    confusion_matrix, accuracy, recall, f1_score

    Examples
    --------

    .. sourcecode:: python

        # Targets and Predictions
        >>> targets = turicreate.SArray([0, 1, 2, 3, 0, 1, 2, 3])
        >>> predictions = turicreate.SArray([1, 0, 2, 1, 3, 1, 0, 1])

        # Micro average of the precision scores for each class.
        >>> turicreate.evaluation.precision(targets, predictions,
        ...                               average = 'micro')
        0.25

        # Macro average of the precision scores for each class.
        >>> turicreate.evaluation.precision(targets, predictions,
        ...                              average = 'macro')
        0.3125

        # Precision score for each class.
        >>> turicreate.evaluation.precision(targets, predictions,
        ...                               average = None)
        {0: 0.0, 1: 0.25, 2: 1.0, 3: 0.0}

    This metric also works for string classes.

    .. sourcecode:: python

        # Targets and Predictions
        >>> targets = turicreate.SArray(
        ...      ["cat", "dog", "foosa", "snake", "cat", "dog", "foosa", "snake"])
        >>> predictions = turicreate.SArray(
        ...      ["dog", "cat", "foosa", "dog", "snake", "dog", "cat", "dog"])

        # Micro average of the precision scores for each class.
        >>> turicreate.evaluation.precision(targets, predictions,
        ...                               average = 'micro')
        0.25

        # Macro average of the precision scores for each class.
        >>> turicreate.evaluation.precision(targets, predictions,
        ...                              average = 'macro')
        0.3125

        # Precision score for each class.
        >>> turicreate.evaluation.precision(targets, predictions,
        ...                               average = None)
        {0: 0.0, 1: 0.25, 2: 1.0, 3: 0.0}
    """
    _supervised_evaluation_error_checking(targets, predictions)
    _check_categorical_option_type('average', average,
                         ['micro', 'macro', None])
    _check_same_type_not_float(targets, predictions)
    opts = {"average": average}
    return _turicreate.extensions._supervised_streaming_evaluator(targets,
                          predictions, "precision", opts)


def recall(targets, predictions, average='macro'):
    r"""
    Compute the recall score for classification tasks. The recall score
    quantifies the ability of a classifier to predict `positive` examples.
    Recall can be interpreted as the probability that a randomly selected
    `positive` example is correctly identified by the classifier. The score
    is in the range [0,1] with 0 being the worst, and 1 being perfect.


    The recall score is defined as the ratio:
        .. math::
            \frac{tp}{tp + fn}

    where `tp` is the number of true positives and `fn` the number of false
    negatives.

    Parameters
    ----------
    targets : SArray
        Ground truth class labels. The SArray can be of any type.

    predictions : SArray
        The prediction that corresponds to each target value.  This SArray must
        have the same length as ``targets`` and must be of the same type
        as the ``targets`` SArray.

    average : string, [None, 'macro' (default), 'micro']
        Metric averaging strategies for multiclass classification. Averaging
        strategies can be one of the following:

            - None: No averaging is performed and a single metric is returned
              for each class.
            - 'micro': Calculate metrics globally by counting the total true
              positives, false negatives, and false positives.
            - 'macro': Calculate metrics for each label and find their
              unweighted mean. This does not take label imbalance into account.

    Returns
    -------
    out : float (for binary classification) or dict[float]
        Score for the positive class (for binary classification) or an average
        score for each class for multi-class classification.  If
        `average=None`, then a dictionary is returned where the key is the
        class label and the value is the score for the corresponding class
        label.

    Notes
    -----
     - For binary classification, when the target label is of type "string",
       then the labels are sorted alphanumerically and the largest label is
       chosen as the "positive" label.  For example, if the classifier labels
       are {"cat", "dog"}, then "dog" is chosen as the positive label for the
       binary classification case.

    See Also
    --------
    confusion_matrix, accuracy, precision, f1_score

    Examples
    --------

    .. sourcecode:: python

        # Targets and Predictions
        >>> targets = turicreate.SArray([0, 1, 2, 3, 0, 1, 2, 3])
        >>> predictions = turicreate.SArray([1, 0, 2, 1, 3, 1, 2, 1])

        # Micro average of the recall scores for each class.
        >>> turicreate.evaluation.recall(targets, predictions,
        ...                            average = 'micro')
        0.375

        # Macro average of the recall scores for each class.
        >>> turicreate.evaluation.recall(targets, predictions,
        ...                            average = 'macro')
        0.375

        # Recall score for each class.
        >>> turicreate.evaluation.recall(targets, predictions,
        ...                            average = None)
        {0: 0.0, 1: 0.5, 2: 1.0, 3: 0.0}

    This metric also works for string classes.

    .. sourcecode:: python

        # Targets and Predictions
        >>> targets = turicreate.SArray(
        ...      ["cat", "dog", "foosa", "snake", "cat", "dog", "foosa", "snake"])
        >>> predictions = turicreate.SArray(
        ...      ["dog", "cat", "foosa", "dog", "snake", "dog", "cat", "dog"])

        # Micro average of the recall scores for each class.
        >>> turicreate.evaluation.recall(targets, predictions,
        ...                            average = 'micro')
        0.375

        # Macro average of the recall scores for each class.
        >>> turicreate.evaluation.recall(targets, predictions,
        ...                            average = 'macro')
        0.375

        # Recall score for each class.
        >>> turicreate.evaluation.recall(targets, predictions,
        ...                            average = None)
        {0: 0.0, 1: 0.5, 2: 1.0, 3: 0.0}
    """
    _supervised_evaluation_error_checking(targets, predictions)
    _check_categorical_option_type('average', average,
                         ['micro', 'macro', None])
    _check_same_type_not_float(targets, predictions)
    opts = {"average": average}
    return _turicreate.extensions._supervised_streaming_evaluator(targets,
                          predictions, "recall", opts)

def roc_curve(targets, predictions, average=None):
    r"""
    Compute an ROC curve for the given targets and predictions. Currently,
    only binary classification is supported.

    Parameters
    ----------
    targets : SArray
        An SArray containing the observed values. For binary classification,
        the alpha-numerically first category is considered the reference
        category.

    predictions : SArray
        The prediction that corresponds to each target value.  This vector must
        have the same length as ``targets``. Target scores, can either be
        probability estimates of the positive class, confidence values, or
        binary decisions.

    average : string, [None (default)]
        Metric averaging strategies for multiclass classification. Averaging
        strategies can be one of the following:

            - None: No averaging is performed and a single metric is returned
              for each class.

    Returns
    -------
    out : SFrame
        Each row represents the predictive performance when using a given
        cutoff threshold, where all predictions above that cutoff are
        considered "positive". Four columns are used to describe the
        performance:

            - tpr   : True positive rate, the number of true positives divided by the number of positives.
            - fpr   : False positive rate, the number of false positives divided by the number of negatives.
            - p     : Total number of positive values.
            - n     : Total number of negative values.
            - class : Reference class for this ROC curve.

    See Also
    --------
    confusion_matrix, auc

    References
    ----------
    `An introduction to ROC analysis. Tom Fawcett.
    <https://ccrma.stanford.edu/workshops/mir2009/references/ROCintro.pdf>`_

    Notes
    -----
     - For binary classification, when the target label is of type "string",
       then the labels are sorted alphanumerically and the largest label is
       chosen as the "positive" label.  For example, if the classifier labels
       are {"cat", "dog"}, then "dog" is chosen as the positive label for the
       binary classification case.
     - For multi-class classification, when the target label is of type
       "string", then the probability vector is assumed to be a vector of
       probabilities of classes as sorted alphanumerically. Hence, for the
       probability vector [0.1, 0.2, 0.7] for a dataset with classes "cat",
       "dog", and "rat"; the 0.1 corresponds to "cat", the 0.2 to "dog" and the
       0.7 to "rat".
     - The ROC curve is computed using a binning approximation with 1M bins and
       is hence accurate only to the 5th decimal.


    Examples
    --------
    .. sourcecode:: python

        >>> targets = turicreate.SArray([0, 1, 1, 0])
        >>> predictions = turicreate.SArray([0.1, 0.35, 0.7, 0.99])

        # Calculate the roc-curve.
        >>> roc_curve =  turicreate.evaluation.roc_curve(targets, predictions)
        +-------------------+-----+-----+---+---+
        |     threshold     | fpr | tpr | p | n |
        +-------------------+-----+-----+---+---+
        |        0.0        | 1.0 | 1.0 | 2 | 2 |
        | 9.99999974738e-06 | 1.0 | 1.0 | 2 | 2 |
        | 1.99999994948e-05 | 1.0 | 1.0 | 2 | 2 |
        | 2.99999992421e-05 | 1.0 | 1.0 | 2 | 2 |
        | 3.99999989895e-05 | 1.0 | 1.0 | 2 | 2 |
        | 4.99999987369e-05 | 1.0 | 1.0 | 2 | 2 |
        | 5.99999984843e-05 | 1.0 | 1.0 | 2 | 2 |
        | 7.00000018696e-05 | 1.0 | 1.0 | 2 | 2 |
        |  7.9999997979e-05 | 1.0 | 1.0 | 2 | 2 |
        | 9.00000013644e-05 | 1.0 | 1.0 | 2 | 2 |
        +-------------------+-----+-----+---+---+
        [100001 rows x 5 columns]

    For the multi-class setting, an ROC curve is returned for each class.

    .. sourcecode:: python

        # Targets and Predictions
        >>> targets = turicreate.SArray([0, 1, 2, 3, 0, 1, 2, 3])
        >>> predictions = turicreate.SArray([1, 0, 2, 1, 3, 1, 2, 1])

        # Micro average of the recall scores for each class.
        >>> turicreate.evaluation.recall(targets, predictions,
        ...                            average = 'micro')
        0.375

        # Macro average of the recall scores for each class.
        >>> turicreate.evaluation.recall(targets, predictions,
        ...                            average = 'macro')
        0.375

        # Recall score for each class.
        >>> turicreate.evaluation.recall(targets, predictions,
        ...                            average = None)
        {0: 0.0, 1: 0.5, 2: 1.0, 3: 0.0}

    This metric also works in the multi-class setting.

    .. sourcecode:: python

        # Targets and Predictions
        >>> targets     = turicreate.SArray([ 1, 0, 2, 1])
        >>> predictions = turicreate.SArray([[.1, .8, 0.1],
        ...                                [.9, .1, 0.0],
        ...                                [.8, .1, 0.1],
        ...                                [.3, .6, 0.1]])

        # Compute the ROC curve.
        >>> roc_curve = turicreate.evaluation.roc_curve(targets, predictions)
        +-----------+-----+-----+---+---+-------+
        | threshold | fpr | tpr | p | n | class |
        +-----------+-----+-----+---+---+-------+
        |    0.0    | 1.0 | 1.0 | 1 | 3 |   0   |
        |   1e-05   | 1.0 | 1.0 | 1 | 3 |   0   |
        |   2e-05   | 1.0 | 1.0 | 1 | 3 |   0   |
        |   3e-05   | 1.0 | 1.0 | 1 | 3 |   0   |
        |   4e-05   | 1.0 | 1.0 | 1 | 3 |   0   |
        |   5e-05   | 1.0 | 1.0 | 1 | 3 |   0   |
        |   6e-05   | 1.0 | 1.0 | 1 | 3 |   0   |
        |   7e-05   | 1.0 | 1.0 | 1 | 3 |   0   |
        |   8e-05   | 1.0 | 1.0 | 1 | 3 |   0   |
        |   9e-05   | 1.0 | 1.0 | 1 | 3 |   0   |
        +-----------+-----+-----+---+---+-------+
        [300003 rows x 6 columns]

    This metric also works for string classes.

    .. sourcecode:: python

        # Targets and Predictions
        >>> targets     = turicreate.SArray(["cat", "dog", "foosa", "dog"])
        >>> predictions = turicreate.SArray([[.1, .8, 0.1],
        ...                                [.9, .1, 0.0],
        ...                                [.8, .1, 0.1],
        ...                                [.3, .6, 0.1]])

        # Compute the ROC curve.
        >>> roc_curve = turicreate.evaluation.roc_curve(targets, predictions)
        +-----------+-----+-----+---+---+-------+
        | threshold | fpr | tpr | p | n | class |
        +-----------+-----+-----+---+---+-------+
        |    0.0    | 1.0 | 1.0 | 1 | 3 |  cat  |
        |   1e-05   | 1.0 | 1.0 | 1 | 3 |  cat  |
        |   2e-05   | 1.0 | 1.0 | 1 | 3 |  cat  |
        |   3e-05   | 1.0 | 1.0 | 1 | 3 |  cat  |
        |   4e-05   | 1.0 | 1.0 | 1 | 3 |  cat  |
        |   5e-05   | 1.0 | 1.0 | 1 | 3 |  cat  |
        |   6e-05   | 1.0 | 1.0 | 1 | 3 |  cat  |
        |   7e-05   | 1.0 | 1.0 | 1 | 3 |  cat  |
        |   8e-05   | 1.0 | 1.0 | 1 | 3 |  cat  |
        |   9e-05   | 1.0 | 1.0 | 1 | 3 |  cat  |
        +-----------+-----+-----+---+---+-------+
        [300003 rows x 6 columns]
    """
    _supervised_evaluation_error_checking(targets, predictions)
    _check_categorical_option_type('average', average, [None])
    _check_prob_and_prob_vector(predictions)
    _check_target_not_float(targets)
    opts = {"average": average,
            "binary": predictions.dtype in [int, float]}
    return _turicreate.extensions._supervised_streaming_evaluator(targets,
                       predictions, "roc_curve", opts)

def auc(targets, predictions, average='macro'):
    r"""
    Compute the area under the ROC curve for the given targets and predictions.

    Parameters
    ----------
    targets : SArray
        An SArray containing the observed values. For binary classification,
        the alpha-numerically first category is considered the reference
        category.

    predictions : SArray
        Prediction probability that corresponds to each target value. This must
        be of same length as ``targets``.

    average : string, [None, 'macro' (default)]
        Metric averaging strategies for multiclass classification. Averaging
        strategies can be one of the following:

            - None: No averaging is performed and a single metric is returned
              for each class.
            - 'macro': Calculate metrics for each label, and find their
              unweighted mean. This does not take label imbalance into account.

    Returns
    -------
    out : float (for binary classification) or dict[float]
        Score for the positive class (for binary classification) or an average
        score for each class for multi-class classification.  If
        `average=None`, then a dictionary is returned where the key is the
        class label and the value is the score for the corresponding class
        label.

    See Also
    --------
    roc_curve, confusion_matrix

    Examples
    --------
    .. sourcecode:: python

        >>> targets = turicreate.SArray([0, 1, 1, 0])
        >>> predictions = turicreate.SArray([0.1, 0.35, 0.7, 0.99])

        # Calculate the auc-score
        >>> auc =  turicreate.evaluation.auc(targets, predictions)
        0.5

    This metric also works when the targets are strings (Here "cat" is chosen
    as the reference class).

    .. sourcecode:: python

        >>> targets = turicreate.SArray(["cat", "dog", "dog", "cat"])
        >>> predictions = turicreate.SArray([0.1, 0.35, 0.7, 0.99])

        # Calculate the auc-score
        >>> auc =  turicreate.evaluation.auc(targets, predictions)
        0.5


    For the multi-class setting, the auc-score can be averaged.

    .. sourcecode:: python

        # Targets and Predictions
        >>> targets     = turicreate.SArray([ 1, 0, 2, 1])
        >>> predictions = turicreate.SArray([[.1, .8, 0.1],
        ...                                [.9, .1, 0.0],
        ...                                [.8, .1, 0.1],
        ...                                [.3, .6, 0.1]])

        #  Macro average of the scores for each class.
        >>> turicreate.evaluation.auc(targets, predictions, average = 'macro')
        0.8888888888888888

        # Scores for each class.
        >>> turicreate.evaluation.auc(targets, predictions, average = None)
        {0: 1.0, 1: 1.0, 2: 0.6666666666666666}

    This metric also works for "string" targets in the multi-class setting

    .. sourcecode:: python

        # Targets and Predictions
        >>> targets     = turicreate.SArray([ "dog", "cat", "foosa", "dog"])
        >>> predictions = turicreate.SArray([[.1, .8, 0.1],
                                           [.9, .1, 0.0],
                                           [.8, .1, 0.1],
                                           [.3, .6, 0.1]])

        # Macro average.
        >>> auc =  turicreate.evaluation.auc(targets, predictions)
        0.8888888888888888

        # Score for each class.
        >>> auc =  turicreate.evaluation.auc(targets, predictions, average=None)
        {'cat': 1.0, 'dog': 1.0, 'foosa': 0.6666666666666666}

    """
    _supervised_evaluation_error_checking(targets, predictions)
    _check_categorical_option_type('average', average,
                         ['macro', None])
    _check_prob_and_prob_vector(predictions)
    _check_target_not_float(targets)
    opts = {"average": average,
            "binary": predictions.dtype in [int, float]}
    return _turicreate.extensions._supervised_streaming_evaluator(targets,
                      predictions, "auc", opts)

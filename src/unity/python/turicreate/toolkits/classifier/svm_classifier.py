# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Methods for creating and using an SVM model.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _turicreate
import turicreate.toolkits._supervised_learning as _sl
from turicreate.toolkits._supervised_learning import Classifier as _Classifier
from turicreate.toolkits._internal_utils import _toolkit_repr_print, \
                                        _toolkit_get_topk_bottomk, \
                                        _raise_error_evaluation_metric_is_valid, \
                                        _check_categorical_option_type, \
                                        _summarize_coefficients

_DEFAULT_SOLVER_OPTIONS = {
'convergence_threshold': 1e-2,
'max_iterations': 10,
'lbfgs_memory_level': 11,
}

def create(dataset, target, features=None,
    penalty=1.0, solver='auto',
    feature_rescaling=True,
    convergence_threshold = _DEFAULT_SOLVER_OPTIONS['convergence_threshold'],
    lbfgs_memory_level = _DEFAULT_SOLVER_OPTIONS['lbfgs_memory_level'],
    max_iterations = _DEFAULT_SOLVER_OPTIONS['max_iterations'],
    class_weights = None,
    validation_set = 'auto',
    verbose=True):
    """
    Create a :class:`~turicreate.svm_classifier.SVMClassifier` to predict the class of a binary
    target variable based on a model of which side of a hyperplane the example
    falls on. In addition to standard numeric and categorical types, features
    can also be extracted automatically from list- or dictionary-type SFrame
    columns.

    This loss function for the SVM model is the sum of an L1 mis-classification
    loss (multiplied by the 'penalty' term) and a l2-norm on the weight vectors.

    Parameters
    ----------
    dataset : SFrame
        Dataset for training the model.

    target : string
        Name of the column containing the target variable. The values in this
        column must be of string or integer type. String target variables are
        automatically mapped to integers in alphabetical order of the variable
        values. For example, a target variable with 'cat' and 'dog' as possible
        values is mapped to 0 and 1 respectively with 0 being the base class
        and 1 being the reference class.

    features : list[string], optional
        Names of the columns containing features. 'None' (the default) indicates
        that all columns except the target variable should be used as features.

        The features are columns in the input SFrame that can be of the
        following types:

        - *Numeric*: values of numeric type integer or float.

        - *Categorical*: values of type string.

        - *Array*: list of numeric (integer or float) values. Each list element
          is treated as a separate feature in the model.

        - *Dictionary*: key-value pairs with numeric (integer or float) values
          Each key of a dictionary is treated as a separate feature and the
          value in the dictionary corresponds to the value of the feature.
          Dictionaries are ideal for representing sparse data.

        Columns of type *list* are not supported. Convert them to array in
        case all entries in the list are of numeric types and separate them
        out into different columns if they are of mixed type.

    penalty : float, optional
        Penalty term on the mis-classification loss of the model. The larger
        this weight, the more the model coefficients shrink toward 0.  The
        larger the penalty, the lower is the emphasis placed on misclassified
        examples, and the classifier would spend more time maximizing the
        margin for correctly classified examples. The default value is 1.0;
        this parameter must be set to a value of at least 1e-10.


    solver : string, optional
        Name of the solver to be used to solve the problem. See the
        references for more detail on each solver. Available solvers are:

        - *auto (default)*: automatically chooses the best solver (from the ones
          listed below) for the data and model parameters.
        - *lbfgs*: lLimited memory BFGS (``lbfgs``) is a robust solver for wide
          datasets(i.e datasets with many coefficients).

        The solvers are all automatically tuned and the default options should
        function well. See the solver options guide for setting additional
        parameters for each of the solvers.

    feature_rescaling : bool, default = true

        Feature rescaling is an important pre-processing step that ensures
        that all features are on the same scale. An l2-norm rescaling is
        performed to make sure that all features are of the same norm. Categorical
        features are also rescaled by rescaling the dummy variables that
        are used to represent them. The coefficients are returned in original
        scale of the problem.

    convergence_threshold :

        Convergence is tested using variation in the training objective. The
        variation in the training objective is calculated using the difference
        between the objective values between two steps. Consider reducing this
        below the default value (0.01) for a more accurately trained model.
        Beware of overfitting (i.e a model that works well only on the training
        data) if this parameter is set to a very low value.

    max_iterations : int, optional

        The maximum number of allowed passes through the data. More passes over
        the data can result in a more accurately trained model. Consider
        increasing this (the default value is 10) if the training accuracy is
        low and the *Grad-Norm* in the display is large.

    lbfgs_memory_level : int, optional

        The L-BFGS algorithm keeps track of gradient information from the
        previous ``lbfgs_memory_level`` iterations. The storage requirement for
        each of these gradients is the ``num_coefficients`` in the problem.
        Increasing the ``lbfgs_memory_level`` can help improve the quality of
        the model trained. Setting this to more than ``max_iterations`` has the
        same effect as setting it to ``max_iterations``.

    class_weights : {dict, `auto`}, optional

        Weights the examples in the training data according to the given class
        weights. If set to `None`, all classes are supposed to have weight one. The
        `auto` mode set the class weight to be inversely proportional to number of
        examples in the training data with the given class.

    validation_set : SFrame, optional

        A dataset for monitoring the model's generalization performance.
        For each row of the progress table, the chosen metrics are computed
        for both the provided training dataset and the validation_set. The
        format of this SFrame must be the same as the training set.
        By default this argument is set to 'auto' and a validation set is
        automatically sampled and used for progress printing. If
        validation_set is set to None, then no additional metrics
        are computed. The default value is 'auto'.

    verbose : bool, optional
        If True, print progress updates.

    Returns
    -------
    out : SVMClassifier
        A trained model of type
        :class:`~turicreate.svm_classifier.SVMClassifier`.

    See Also
    --------
    SVMClassifier

    Notes
    -----
    - Categorical variables are encoded by creating dummy variables. For
      a variable with :math:`K` categories, the encoding creates :math:`K-1`
      dummy variables, while the first category encountered in the data is used
      as the baseline.

    - For prediction and evaluation of SVM models with sparse dictionary
      inputs, new keys/columns that were not seen during training are silently
      ignored.

    - The penalty parameter is analogous to the 'C' term in the C-SVM. See the
      reference on training SVMs for more details.

    - Any 'None' values in the data will result in an error being thrown.

    - A constant term of '1' is automatically added for the model intercept to
      model the bias term.

    - Note that the hinge loss is approximated by the scaled logistic loss
      function. (See user guide for details)

    References
    ----------
    - `Wikipedia - Support Vector Machines
      <http://en.wikipedia.org/wiki/svm>`_

    - Zhang et al. - Modified Logistic Regression: An Approximation to
      SVM and its Applications in Large-Scale Text Categorization (ICML 2003)


    Examples
    --------

    Given an :class:`~turicreate.SFrame` ``sf``, a list of feature columns
    [``feature_1`` ... ``feature_K``], and a target column ``target`` with 0 and
    1 values, create a
    :class:`~turicreate.svm.SVMClassifier` as follows:

    >>> data =  turicreate.SFrame('https://static.turi.com/datasets/regression/houses.csv')
    >>> data['is_expensive'] = data['price'] > 30000
    >>> model = turicreate.svm_classifier.create(data, 'is_expensive')
    """

    # Regression model names.
    model_name = "classifier_svm"
    solver = solver.lower()

    model = _sl.create(dataset, target, model_name, features=features,
                        validation_set = validation_set, verbose = verbose,
                        penalty = penalty,
                        feature_rescaling = feature_rescaling,
                        convergence_threshold = convergence_threshold,
                        lbfgs_memory_level = lbfgs_memory_level,
                        max_iterations = max_iterations,
                        class_weights = class_weights)

    return SVMClassifier(model.__proxy__)


class SVMClassifier(_Classifier):
    """
    Support Vector Machines can be used to predict binary target variable using
    several feature variables.

    The :py:class:`~turicreate.svm.SVMClassifier` model predicts a binary target
    variable given one or more feature variables. In an SVM model, the examples
    are represented as points in space, mapped so that the examples from the
    two classes being classified are divided by linear separator.

    Given a set of features :math:`x_i`, and a label :math:`y_i \in \{0,1\}`,
    SVM minimizes the loss function:

        .. math::
          f_i(\\theta) =  \max(1 - \\theta^T x, 0)

    An intercept term is added by appending a column of 1's to the features.
    Regularization is often required to prevent over fitting by penalizing
    models with extreme parameter values. The composite objective being
    optimized for is the following:

        .. math::
           \min_{\\theta} \sum_{i = 1}^{n} f_i(\\theta) + \lambda ||\\theta||^{2}_{2}

    where :math:`\lambda` is the ``penalty`` parameter.

    This model cannot be constructed directly.  Instead, use
    :func:`turicreate.svm_classifier.create` to create an instance of this model.
    Additional details on parameter options and code samples are available in
    the documentation for the create function.

    Examples
    --------

    .. sourcecode:: python

        # Load the data (From an S3 bucket)
        >>> import turicreate as tc
        >>> data =  tc.SFrame('https://static.turi.com/datasets/regression/houses.csv')

        # Make sure the target is binary 0/1
        >>> data['is_expensive'] = data['price'] > 30000

        # Make a logistic regression model
        >>> model = tc.svm_classifier.create(data, target='is_expensive'
                                        , features=['bath', 'bedroom', 'size'])

        # Extract the coefficients
        >>> coefficients = model.coefficients # an SFrame

        # Make predictions (as margins, or class)
        >>> predictions = model.progressredict(data)    # Predicts 0/1
        >>> predictions = model.progressredict(data, output_type='margin')

        # Evaluate the model
        >>> results = model.progressvaluate(data)               # a dictionary

    See Also
    --------
    create

    """
    def __init__(self, model_proxy):
        '''__init__(self)'''
        self.__proxy__ = model_proxy
        self.__name__ = self.__class__._native_name()

    @classmethod
    def _native_name(cls):
        return "classifier_svm"

    def __str__(self):
        """
        Return a string description of the model to the ``print`` method.

        Returns
        -------
        out : string
            A description of the model.
        """
        return self.__repr__()

    def _get_summary_struct(self):
        """
        Returns a structured description of the model, including (where relevant)
        the schema of the training data, description of the training data,
        training statistics, and model hyperparameters.

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
            ('Number of coefficients', 'num_coefficients'),
            ('Number of examples', 'num_examples'),
            ('Number of classes', 'num_classes'),
            ('Number of feature columns', 'num_features'),
            ('Number of unpacked features', 'num_unpacked_features')]

        hyperparam_fields = [
            ("Mis-classification penalty", 'penalty'),
        ]

        solver_fields = [
            ("Solver", 'solver'),
            ("Solver iterations", 'training_iterations'),
            ("Solver status", 'training_solver_status'),
            ("Training time (sec)", 'training_time')]

        training_fields = [
            ("Train Loss", 'training_loss')]

        coefs = self.coefficients
        (top_coefs, bottom_coefs) = _toolkit_get_topk_bottomk(coefs, k=5)

        (coefs_list, titles_list) = _summarize_coefficients(top_coefs, \
                                                                    bottom_coefs)

        return ([model_fields, hyperparam_fields, solver_fields, \
                                            training_fields] + coefs_list,
                            [ 'Schema', 'Hyperparameters', \
                            'Training Summary', 'Settings'] + titles_list, )

    def __repr__(self):
        """
        Print a string description of the model, when the model name is entered
        in the terminal.
        """

        (sections, section_titles) = self._get_summary_struct()
        return _toolkit_repr_print(self, sections, section_titles, width=30)

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
        from turicreate.extensions import _linear_svm_export_as_model_asset
        from turicreate.toolkits import _coreml_utils
        display_name = "svm classifier"
        short_description = _coreml_utils._mlmodel_short_description(display_name)
        context = {"class": self.__class__.__name__,
                   "version": _turicreate.__version__,
                   "short_description": short_description}
        _linear_svm_export_as_model_asset(self.__proxy__, filename, context)

    def _get(self, field):
        """
        Return the value of a given field. The list of all queryable fields is
        detailed below, and can be obtained programmatically with the
        :func:`~turicreate.svm.SVMClassifier._list_fields` method.


        +------------------------+-------------------------------------------------------------+
        |      Field             | Description                                                 |
        +========================+=============================================================+
        | coefficients           | Classifier coefficients                                     |
        +------------------------+-------------------------------------------------------------+
        | convergence_threshold  | Desired solver accuracy                                     |
        +------------------------+-------------------------------------------------------------+
        | feature_rescaling      | Bool indicating l2-rescaling of features                    |
        +------------------------+---------+---------------------------------------------------+
        | features               | Feature column names                                        |
        +------------------------+-------------------------------------------------------------+
        | lbfgs_memory_level     | Number of updates to store (lbfgs only)                     |
        +------------------------+-------------------------------------------------------------+
        | max_iterations         | Maximum number of solver iterations                         |
        +------------------------+-------------------------------------------------------------+
        | num_coefficients       | Number of coefficients in the model                         |
        +------------------------+-------------------------------------------------------------+
        | num_examples           | Number of examples used for training                        |
        +------------------------+-------------------------------------------------------------+
        | num_features           | Number of dataset columns used for training                 |
        +------------------------+-------------------------------------------------------------+
        | num_unpacked_features  | Number of features (including expanded list/dict features)  |
        +------------------------+-------------------------------------------------------------+
        | penalty                | Misclassification penalty term                              |
        +------------------------+-------------------------------------------------------------+
        | solver                 | Type of solver                                              |
        +------------------------+-------------------------------------------------------------+
        | target                 | Target column name                                          |
        +------------------------+-------------------------------------------------------------+
        | training_iterations    | Number of solver iterations                                 |
        +------------------------+-------------------------------------------------------------+
        | training_loss          | Maximized Log-likelihood                                    |
        +------------------------+-------------------------------------------------------------+
        | training_solver_status | Solver status after training                                |
        +------------------------+-------------------------------------------------------------+
        | training_time          | Training time (excludes preprocessing)                      |
        +------------------------+-------------------------------------------------------------+
        | unpacked_features      | Feature names (including expanded list/dict features)       |
        +------------------------+-------------------------------------------------------------+

        Parameters
        ----------
        field : string
            Name of the field to be retrieved.

        Returns
        -------
        out
            Value of the requested fields.
        """
        return super(_Classifier, self)._get(field)

    def predict(self, dataset, output_type='class', missing_value_action='auto'):
        """
        Return predictions for ``dataset``, using the trained logistic
        regression model. Predictions can be generated as class labels (0 or
        1), or margins (i.e. the distance of the observations from the hyperplane
        separating the classes). By default, the predict method returns class
        labels.

        For each new example in ``dataset``, the margin---also known as the
        linear predictor---is the inner product of the example and the model
        coefficients plus the intercept term. Predicted classes are obtained by
        thresholding the margins at 0.

        Parameters
        ----------
        dataset : SFrame | dict
            Dataset of new observations. Must include columns with the same
            names as the features used for model training, but does not require
            a target column. Additional columns are ignored.

        output_type : {'margin', 'class'}, optional
            Form of the predictions which are one of:

            - 'margin': Distance of the observations from the hyperplane
              separating the classes.
            - 'class': Class prediction.

        missing_value_action : str, optional
            Action to perform when missing values are encountered. This can be
            one of:

            - 'auto': Default to 'impute'
            - 'impute': Proceed with evaluation by filling in the missing
              values with the mean of the training data. Missing
              values are also imputed if an entire column of data is
              missing during evaluation.
            - 'error' : Do not proceed with prediction and terminate with
              an error message.

        Returns
        -------
        out : SArray
            An SArray with model predictions.

        See Also
        ----------
        create, evaluate, classify

        Examples
        ----------
        >>> data =  turicreate.SFrame('https://static.turi.com/datasets/regression/houses.csv')

        >>> data['is_expensive'] = data['price'] > 30000
        >>> model = turicreate.svm_classifier.create(data,
                                  target='is_expensive',
                                  features=['bath', 'bedroom', 'size'])

        >>> class_predictions = model.progressredict(data)
        >>> margin_predictions = model.progressredict(data, output_type='margin')

        """

        _check_categorical_option_type('output_type', output_type,
                                               ['class', 'margin'])
        return super(_Classifier, self).predict(dataset,
                                                output_type=output_type,
                                                missing_value_action=missing_value_action)

    def classify(self, dataset, missing_value_action='auto'):
        """
        Return a classification, for each example in the ``dataset``, using the
        trained SVM model. The output SFrame contains predictions
        as class labels (0 or 1) associated with the the example.

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the features used for model training, but does not require
            a target column. Additional columns are ignored.

        missing_value_action : str, optional
            Action to perform when missing values are encountered. This can be
            one of:

            - 'auto': Default to 'impute'
            - 'impute': Proceed with evaluation by filling in the missing
              values with the mean of the training data. Missing
              values are also imputed if an entire column of data is
              missing during evaluation.
            - 'error' : Do not proceed with prediction and terminate with
              an error message.

        Returns
        -------
        out : SFrame
            An SFrame with model predictions i.e class labels.

        See Also
        ----------
        create, evaluate, predict

        Examples
        ----------
        >>> data =  turicreate.SFrame('https://static.turi.com/datasets/regression/houses.csv')

        >>> data['is_expensive'] = data['price'] > 30000
        >>> model = turicreate.svm_classifier.create(data, target='is_expensive',
                                      features=['bath', 'bedroom', 'size'])

        >>> classes = model.classify(data)

        """
        return super(SVMClassifier, self).classify(dataset, missing_value_action=missing_value_action)

    
    def evaluate(self, dataset, metric='auto', missing_value_action='auto'):
        """
        Evaluate the model by making predictions of target values and comparing
        these to actual values.

        Two metrics are used to evaluate SVM. The confusion table contains the
        cross-tabulation of actual and predicted classes for the target
        variable. classifier accuracy is the fraction of examples whose
        predicted and actual classes match.

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the target and features used for model training. Additional
            columns are ignored.

        metric : str, optional
            Name of the evaluation metric.  Possible values are:

            - 'auto'             : Returns all available metrics.
            - 'accuracy '        : Classification accuracy (micro average).
            - 'precision'        : Precision score (micro average)
            - 'recall'           : Recall score (micro average)
            - 'f1_score'         : F1 score (micro average)
            - 'confusion_matrix' : An SFrame with counts of possible prediction/true
                                   label combinations.

        missing_value_action : str, optional
            Action to perform when missing values are encountered. This can be
            one of:

            - 'auto': Default to 'impute'
            - 'impute': Proceed with evaluation by filling in the missing
              values with the mean of the training data. Missing
              values are also imputed if an entire column of data is
              missing during evaluation.
            - 'error': Do not proceed with evaluation and terminate with
              an error message.

        Returns
        -------
        out : dict
            Dictionary of evaluation results where the key is the name of the
            evaluation metric (e.g. `accuracy`) and the value is the evaluation
            score.

        See Also
        ----------
        create, predict, classify

        Examples
        ----------
        .. sourcecode:: python

          >>> data =  turicreate.SFrame('https://static.turi.com/datasets/regression/houses.csv')

          >>> data['is_expensive'] = data['price'] > 30000
          >>> model = turicreate.svm_classifier.create(data, \
          ...                                        target='is_expensive', \
          ...                                        features=['bath', 'bedroom', 'size'])
          >>> results = model.progressvaluate(data)
          >>> print results['accuracy']
        """
        _raise_error_evaluation_metric_is_valid(metric,
             ['auto', 'accuracy', 'confusion_matrix', 'precision', 'recall',
              'f1_score'])
        return super(_Classifier, self).evaluate(dataset,
                               missing_value_action=missing_value_action,
                               metric=metric)

    @classmethod
    def _get_queryable_methods(cls):
        '''Returns a list of method names that are queryable through Predictive
        Service'''
        return {'predict':{},
                'classify':{}}

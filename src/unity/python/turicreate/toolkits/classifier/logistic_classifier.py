# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
training_Methods for creating and using a logistic regression model.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _turicreate
import turicreate.toolkits._supervised_learning as _sl
from turicreate.toolkits._supervised_learning import Classifier as _Classifier
from turicreate.toolkits._internal_utils import _toolkit_repr_print, \
                                        _toolkit_get_topk_bottomk, \
                                        _raise_error_if_not_sframe, \
                                        _check_categorical_option_type, \
                                        _map_unity_proxy_to_object, \
                                        _raise_error_evaluation_metric_is_valid, \
                                        _summarize_coefficients


_DEFAULT_SOLVER_OPTIONS = {
'convergence_threshold': 1e-2,
'step_size': 1.0,
'lbfgs_memory_level': 11,
'max_iterations': 10}

def create(dataset, target, features=None,
    l2_penalty=0.01, l1_penalty=0.0,
    solver='auto', feature_rescaling=True,
    convergence_threshold = _DEFAULT_SOLVER_OPTIONS['convergence_threshold'],
    step_size = _DEFAULT_SOLVER_OPTIONS['step_size'],
    lbfgs_memory_level = _DEFAULT_SOLVER_OPTIONS['lbfgs_memory_level'],
    max_iterations = _DEFAULT_SOLVER_OPTIONS['max_iterations'],
    class_weights = None,
    validation_set = 'auto',
    verbose=True,
    seed=None):
    """
    Create a :class:`~turicreate.logistic_classifier.LogisticClassifier` (using
    logistic regression as a classifier) to predict the class of a discrete
    target variable (binary or multiclass) based on a model of class probability
    as a logistic function of a linear combination of the features.  In addition
    to standard numeric and categorical types, features can also be extracted
    automatically from list or dictionary-type SFrame columns.

    This model can be regularized with an l1 penalty, an l2 penalty, or both. By
    default this model has an l2 regularization weight of 0.01.

    Parameters
    ----------
    dataset : SFrame
        Dataset for training the model.

    target : string or int
        Name of the column containing the target variable. The values in this
        column must be of string or integer type. String target variables are
        automatically mapped to integers in the order in which they are provided.
        For example, a target variable with 'cat' and 'dog' as possible
        values is mapped to 0 and 1 respectively with 0 being the base class
        and 1 being the reference class. Use `model.classes` to retrieve
        the order in which the classes are mapped.

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

        Columns of type *list* are not supported. Convert such feature
        columns to type array if all entries in the list are of numeric
        types. If the lists contain data of mixed types, separate
        them out into different columns.

    l2_penalty : float, optional
        Weight on l2 regularization of the model. The larger this weight, the
        more the model coefficients shrink toward 0. This introduces bias into
        the model but decreases variance, potentially leading to better
        predictions. The default value is 0.01; setting this parameter to 0
        corresponds to unregularized logistic regression. See the ridge
        regression reference for more detail.

    l1_penalty : float, optional
        Weight on l1 regularization of the model. Like the l2 penalty, the
        higher the l1 penalty, the more the estimated coefficients shrink toward
        0. The l1 penalty, however, completely zeros out sufficiently small
        coefficients, automatically indicating features that are not useful
        for the model. The default weight of 0 prevents any features from
        being discarded. See the LASSO regression reference for more detail.

    solver : string, optional
        Name of the solver to be used to solve the regression. See the
        references for more detail on each solver. Available solvers are:

        - *auto (default)*: automatically chooses the best solver for the data
          and model parameters.
        - *newton*: Newton-Raphson
        - *lbfgs*: limited memory BFGS
        - *fista*: accelerated gradient descent

        For this model, the Newton-Raphson method is equivalent to the
        iteratively re-weighted least squares algorithm. If the l1_penalty is
        greater than 0, use the 'fista' solver.

        The model is trained using a carefully engineered collection of methods
        that are automatically picked based on the input data. The ``newton``
        method  works best for datasets with plenty of examples and few features
        (long datasets). Limited memory BFGS (``lbfgs``) is a robust solver for
        wide datasets (i.e datasets with many coefficients).  ``fista`` is the
        default solver for l1-regularized linear regression. The solvers are all
        automatically tuned and the default options should function well. See
        the solver options guide for setting additional parameters for each of
        the solvers.

        See the user guide for additional details on how the solver is chosen.
        (see `here
        <https://apple.github.io/turicreate/docs/userguide/supervised-learning/linear-regression.html>`_)



    feature_rescaling : boolean, optional

        Feature rescaling is an important pre-processing step that ensures that
        all features are on the same scale. An l2-norm rescaling is performed
        to make sure that all features are of the same norm. Categorical
        features are also rescaled by rescaling the dummy variables that are
        used to represent them. The coefficients are returned in original scale
        of the problem. This process is particularly useful when features
        vary widely in their ranges.


    convergence_threshold : float, optional

        Convergence is tested using variation in the training objective. The
        variation in the training objective is calculated using the difference
        between the objective values between two steps. Consider reducing this
        below the default value (0.01) for a more accurately trained model.
        Beware of overfitting (i.e a model that works well only on the training
        data) if this parameter is set to a very low value.

    lbfgs_memory_level : float, optional

        The L-BFGS algorithm keeps track of gradient information from the
        previous ``lbfgs_memory_level`` iterations. The storage requirement for
        each of these gradients is the ``num_coefficients`` in the problem.
        Increasing the ``lbfgs_memory_level ``can help improve the quality of
        the model trained. Setting this to more than ``max_iterations`` has the
        same effect as setting it to ``max_iterations``.

    max_iterations : float, optional

        The maximum number of allowed passes through the data. More passes over
        the data can result in a more accurately trained model. Consider
        increasing this (the default value is 10) if the training accuracy is
        low and the *Grad-Norm* in the display is large.

    step_size : float, optional

        The starting step size to use for the ``fista`` solver. The default is
        set to 1.0, this is an aggressive setting. If the first iteration takes
        a considerable amount of time, reducing this parameter may speed up
        model training.

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

    seed : int, optional
        Seed for random number generation. Set this value to ensure that the
        same model is created every time.

    Returns
    -------
    out : LogisticClassifier
        A trained model of type
        :class:`~turicreate.logistic_classifier.LogisticClassifier`.

    See Also
    --------
    LogisticClassifier, turicreate.boosted_trees_classifier.BoostedTreesClassifier,
    turicreate.svm_classifier.SVMClassifier, turicreate.classifier.create

    Notes
    -----
    - Categorical variables are encoded by creating dummy variables. For a
      variable with :math:`K` categories, the encoding creates :math:`K-1` dummy
      variables, while the first category encountered in the data is used as the
      baseline.

    - For prediction and evaluation of logistic regression models with sparse
      dictionary inputs, new keys/columns that were not seen during training
      are silently ignored.

    - During model creation, 'None' values in the data will result in an error
      being thrown.

    - A constant term is automatically added for the model intercept. This term
      is not regularized.

    - Standard errors on coefficients are only availiable when `solver=newton`
      or when the default `auto` solver option choses the newton method and if
      the number of examples in the training data is more than the number of
      coefficients. If standard errors cannot be estimated, a column of `None`
      values are returned.


    References
    ----------
    - `Wikipedia - logistic regression
      <http://en.wikipedia.org/wiki/Logistic_regression>`_

    - Hoerl, A.E. and Kennard, R.W. (1970) `Ridge regression: Biased Estimation
      for Nonorthogonal Problems
      <http://amstat.tandfonline.com/doi/abs/10.1080/00401706.1970.10488634>`_.
      Technometrics 12(1) pp.55-67

    - Tibshirani, R. (1996) `Regression Shrinkage and Selection via the Lasso <h
      ttp://www.jstor.org/discover/10.2307/2346178?uid=3739256&uid=2&uid=4&sid=2
      1104169934983>`_. Journal of the Royal Statistical Society. Series B
      (Methodological) 58(1) pp.267-288.

    - Zhu, C., et al. (1997) `Algorithm 778: L-BFGS-B: Fortran subroutines for
      large-scale bound-constrained optimization
      <https://dl.acm.org/citation.cfm?id=279236>`_. ACM Transactions on
      Mathematical Software 23(4) pp.550-560.

    - Beck, A. and Teboulle, M. (2009) `A Fast Iterative Shrinkage-Thresholding
      Algorithm for Linear Inverse Problems
      <http://epubs.siam.org/doi/abs/10.1137/080716542>`_. SIAM Journal on
      Imaging Sciences 2(1) pp.183-202.


    Examples
    --------

    Given an :class:`~turicreate.SFrame` ``sf``, a list of feature columns
    [``feature_1`` ... ``feature_K``], and a target column ``target`` with 0 and
    1 values, create a
    :class:`~turicreate.logistic_classifier.LogisticClassifier` as follows:

    >>> data =  turicreate.SFrame('https://static.turi.com/datasets/regression/houses.csv')
    >>> data['is_expensive'] = data['price'] > 30000
    >>> model = turicreate.logistic_classifier.create(data, 'is_expensive')

    By default all columns of ``data`` except the target are used as features, but
    specific feature columns can be specified manually.

    >>> model = turicreate.logistic_classifier.create(data, 'is_expensive', ['bedroom', 'size'])


    .. sourcecode:: python

      # L2 regularizer
      >>> model_ridge = turicreate.logistic_classifier.create(data, 'is_expensive', l2_penalty=0.1)

      # L1 regularizer
      >>> model_lasso = turicreate.logistic_classifier.create(data, 'is_expensive', l2_penalty=0.,
                                                                   l1_penalty=1.0)

      # Both L1 and L2 regularizer
      >>> model_enet  = turicreate.logistic_classifier.create(data, 'is_expensive', l2_penalty=0.5, l1_penalty=0.5)

    """


    # Regression model names.
    model_name = "classifier_logistic_regression"
    solver = solver.lower()

    model = _sl.create(dataset, target, model_name, features=features,
                        validation_set = validation_set, verbose = verbose,
                        l2_penalty=l2_penalty, l1_penalty = l1_penalty,
                        feature_rescaling = feature_rescaling,
                        convergence_threshold = convergence_threshold,
                        step_size = step_size,
                        solver = solver,
                        lbfgs_memory_level = lbfgs_memory_level,
                        max_iterations = max_iterations,
                        class_weights = class_weights,
                        seed=seed)

    return LogisticClassifier(model.__proxy__)

class LogisticClassifier(_Classifier):
    """
    Logistic regression models a discrete target variable as a function of
    several feature variables.

    The :class:`~turicreate.logistic_classifier.logisticClassifier` uses
    a discrete target variable :math:`y` instead of a scalar. For each
    observation, the probability that :math:`y=1` (instead of 0) is modeled as
    the logistic function of a linear combination of the feature values.

    Given a set of features :math:`x_i`, and a label :math:`y_i \in \{0,1\}`,
    logistic regression interprets the probability that the label is in one class
    as a logistic function of a linear combination of the features.

        .. math::
          f_i(\\theta) =  p(y_i = 1 | x) = \\frac{1}{1 + \exp(-\\theta^T x)}

    An intercept term is added by appending a column of 1's to the features.
    Regularization is often required to prevent over fitting by penalizing
    models with extreme parameter values. The logistic regression module
    supports l1 and l2 regularization, which are added to the loss function.

    The composite objective being optimized for is the following;

        .. math::
           \min_{\\theta} \sum_{i = 1}^{n} f_i(\\theta) + \lambda_1 ||\\theta||_1 + \lambda_2 ||\\theta||^{2}_{2}

    where :math:`\lambda_1` is the ``l1_penalty`` and :math:`\lambda_2` is the
    ``l2_penalty``.

    For multi-class models, we perform multinomial logistic regression, which
    is an extension of the binary logistic regression model discussed above.

    This model cannot be constructed directly.  Instead, use
    :func:`turicreate.logistic_classifier.create` to create an instance of this
    model. A detailed list of parameter options and code samples are available
    in the documentation for the create function.

    Examples
    --------
    .. sourcecode:: python

        # Load the data (From an S3 bucket)
        >>> data =  turicreate.SFrame('https://static.turi.com/datasets/regression/houses.csv')

        # Make sure the target is discrete
        >>> data['is_expensive'] = data['price'] > 30000

        # Make a logistic regression model
        >>> model = turicreate.logistic_classifier.create(data, target='is_expensive', features=['bath', 'bedroom', 'size'])

        # Extract the coefficients
        >>> coefficients = model.coefficients

        # Make predictions (as margins, probability, or class)
        >>> predictions = model.predict(data)
        >>> predictions = model.predict(data, output_type='probability')
        >>> predictions = model.predict(data, output_type='margin')

        # Evaluate the model
        >>> results = model.evaluate(data)

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
        return "classifier_logistic_regression"

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
            ("L1 penalty", 'l1_penalty'),
            ("L2 penalty", 'l2_penalty')
        ]

        solver_fields = [
            ("Solver", 'solver'),
            ("Solver iterations", 'training_iterations'),
            ("Solver status", 'training_solver_status'),
            ("Training time (sec)", 'training_time')]

        training_fields = [
            ("Log-likelihood", 'training_loss')]

        coefs = self.coefficients
        top_coefs, bottom_coefs = _toolkit_get_topk_bottomk(coefs,k=5)

        (coefs_list, titles_list) = _summarize_coefficients(top_coefs, \
                                                                    bottom_coefs)

        return ([ model_fields, hyperparam_fields, \
                        solver_fields, training_fields ] + coefs_list, \
                        [ 'Schema', 'Hyperparameters', \
                        'Training Summary', 'Settings' ] + titles_list )

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
        from turicreate.extensions import _logistic_classifier_export_as_model_asset
        from turicreate.toolkits import _coreml_utils
        display_name = "logistic classifier"
        short_description = _coreml_utils._mlmodel_short_description(display_name)
        context = {"class": self.__class__.__name__,
                   "version": _turicreate.__version__,
                   "short_description": short_description}
        _logistic_classifier_export_as_model_asset(self.__proxy__, filename, context)

    def _get(self, field):
        """
        Return the value of a given field. The list of all queryable fields is
        detailed below, and can be obtained programmatically with the
        :func:`~turicreate.logistic_classifier.LogisticClassifier._list_fields`
        method.

        +------------------------+-------------------------------------------------------------+
        |      Field             | Description                                                 |
        +========================+=============================================================+
        | coefficients           | Regression coefficients                                     |
        +------------------------+-------------------------------------------------------------+
        | convergence_threshold  | Desired solver accuracy                                     |
        +------------------------+-------------------------------------------------------------+
        | feature_rescaling      | Bool indicating l2-rescaling of features                    |
        +------------------------+-------------------------------------------------------------+
        | features               | Feature column names                                        |
        +------------------------+-------------------------------------------------------------+
        | l1_penalty             | l1 regularization weight                                    |
        +------------------------+-------------------------------------------------------------+
        | l2_penalty             | l2 regularization weight                                    |
        +------------------------+-------------------------------------------------------------+
        | lbfgs_memory_level     | LBFGS memory level                                          |
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
        | solver                 | Type of solver                                              |
        +------------------------+-------------------------------------------------------------+
        | step_size              | Initial step size for the solver                            |
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

    def predict(self, dataset, output_type='class',
                missing_value_action='auto'):
        """
        Return predictions for ``dataset``, using the trained logistic
        regression model. Predictions can be generated as class labels,
        probabilities that the target value is True, or margins (i.e. the
        distance of the observations from the hyperplane separating the
        classes). `probability_vector` returns a vector of probabilities by
        each class.

        For each new example in ``dataset``, the margin---also known as the
        linear predictor---is the inner product of the example and the model
        coefficients. The probability is obtained by passing the margin through
        the logistic function. Predicted classes are obtained by thresholding
        the predicted probabilities at 0.5. If you would like to threshold
        predictions at a different probability level, you can use the
        Turi Create evaluation toolkit.

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the features used for model training, but does not require
            a target column. Additional columns are ignored.

        output_type : {'probability', 'margin', 'class', 'probability_vector'}, optional
            Form of the predictions which are one of:

            - 'probability': Prediction probability associated with the True
              class (not applicable for multi-class classification)
            - 'probability_vector': Prediction probability associated with each
              class as a vector. The probability of the first class (sorted
              alphanumerically by name of the class in the training set) is in
              position 0 of the vector, the second in position 1 and so on.
            - 'class': Class prediction. For multi-class classification, this
              returns the class with maximum probability.

        missing_value_action : str, optional
            Action to perform when missing values are encountered. Can be
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
        out : SArray
            An SArray with model predictions.

        See Also
        ----------
        create, evaluate, classify

        Examples
        ----------
        >>> data =  turicreate.SFrame('https://static.turi.com/datasets/regression/houses.csv')

        >>> data['is_expensive'] = data['price'] > 30000
        >>> model = turicreate.logistic_classifier.create(data,
                                             target='is_expensive',
                                             features=['bath', 'bedroom', 'size'])

        >>> probability_predictions = model.predict(data, output_type='probability')
        >>> margin_predictions = model.predict(data, output_type='margin')
        >>> class_predictions = model.predict(data, output_type='class')

        """

        return super(_Classifier, self).predict(dataset,
                                                output_type=output_type,
                                                missing_value_action=missing_value_action)

    def classify(self, dataset, missing_value_action='auto'):
        """
        Return a classification, for each example in the ``dataset``, using the
        trained logistic regression model. The output SFrame contains predictions
        as both class labels (0 or 1) as well as probabilities that the predicted
        value is the associated label.

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
            - 'error': Do not proceed with evaluation and terminate with
              an error message.

        Returns
        -------
        out : SFrame
            An SFrame with model predictions i.e class labels and probabilities.

        See Also
        ----------
        create, evaluate, predict

        Examples
        ----------
        >>> data =  turicreate.SFrame('https://static.turi.com/datasets/regression/houses.csv')

        >>> data['is_expensive'] = data['price'] > 30000
        >>> model = turicreate.logistic_classifier.create(data,
                                             target='is_expensive',
                                             features=['bath', 'bedroom', 'size'])

        >>> classes = model.classify(data)

        """

        return super(LogisticClassifier, self).classify(dataset,
                                                        missing_value_action=missing_value_action)

    def predict_topk(self, dataset, output_type="probability", k=3, missing_value_action='auto'):
        """
        Return top-k predictions for the ``dataset``, using the trained model.
        Predictions are returned as an SFrame with three columns: `id`,
        `class`, and `probability`, `margin`,  or `rank`, depending on the ``output_type``
        parameter. Input dataset size must be the same as for training of the model.

        Parameters
        ----------
        dataset : SFrame
            A dataset that has the same columns that were used during training.
            If the target column exists in ``dataset`` it will be ignored
            while making predictions.

        output_type : {'probability', 'rank', 'margin'}, optional
            Choose the return type of the prediction:

            - `probability`: Probability associated with each label in the prediction.
            - `rank`       : Rank associated with each label in the prediction.
            - `margin`     : Margin associated with each label in the prediction.

        k : int, optional
            Number of classes to return for each input example.

        missing_value_action : str, optional
            Action to perform when missing values are encountered. Can be
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
        out : SFrame
            An SFrame with model predictions.

        See Also
        --------
        predict, classify, evaluate

        Examples
        --------
        >>> pred = m.predict_topk(validation_data, k=3)
        >>> pred
        +--------+-------+-------------------+
        | id     | class |   probability     |
        +--------+-------+-------------------+
        |   0    |   4   |   0.995623886585  |
        |   0    |   9   |  0.0038311756216  |
        |   0    |   7   | 0.000301006948575 |
        |   1    |   1   |   0.928708016872  |
        |   1    |   3   |  0.0440889261663  |
        |   1    |   2   |  0.0176190119237  |
        |   2    |   3   |   0.996967732906  |
        |   2    |   2   |  0.00151345680933 |
        |   2    |   7   | 0.000637513934635 |
        |   3    |   1   |   0.998070061207  |
        |  ...   |  ...  |        ...        |
        +--------+-------+-------------------+
        [35688 rows x 3 columns]
        """
        _check_categorical_option_type('output_type', output_type,
                                       ['rank', 'margin', 'probability'])
        _check_categorical_option_type('missing_value_action', missing_value_action,
                                       ['auto', 'impute', 'error'])
        if missing_value_action == 'auto':
            missing_value_action = 'impute'

        # Low latency path
        if isinstance(dataset, list):
            return _turicreate.extensions._fast_predict_topk(self.__proxy__, dataset,
                    output_type, missing_value_action, k)
        if isinstance(dataset, dict):
            return _turicreate.extensions._fast_predict_topk(self.__proxy__, [dataset],
                    output_type, missing_value_action, k)
        # Fast path
        _raise_error_if_not_sframe(dataset, "dataset")
        options = dict()
        if (missing_value_action == 'auto'):
            missing_value_action = _sl.select_default_missing_value_policy(
                                                              self, 'predict')
        options.update({'model': self.__proxy__,
                        'model_name': self.__name__,
                        'dataset': dataset,
                        'output_type': output_type,
                        'topk': k,
                        'missing_value_action': missing_value_action})
        target = _turicreate.toolkits._main.run(
                  'supervised_learning_predict_topk', options)
        return _map_unity_proxy_to_object(target['predicted'])

    
    def evaluate(self, dataset, metric='auto', missing_value_action='auto'):
        """
        Evaluate the model by making predictions of target values and comparing
        these to actual values.

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the target and features used for model training. Additional
            columns are ignored.

        metric : str, optional
            Name of the evaluation metric.  Possible values are:

            - 'auto'             : Returns all available metrics.
            - 'accuracy'         : Classification accuracy (micro average).
            - 'auc'              : Area under the ROC curve (macro average)
            - 'precision'        : Precision score (macro average)
            - 'recall'           : Recall score (macro average)
            - 'f1_score'         : F1 score (macro average)
            - 'log_loss'         : Log loss
            - 'confusion_matrix' : An SFrame with counts of possible prediction/true label combinations.
            - 'roc_curve'        : An SFrame containing information needed for an ROC curve

            For more flexibility in calculating evaluation metrics, use the
            :class:`~turicreate.evaluation` module.

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
          >>> model = turicreate.logistic_classifier.create(data,
          ...                             target='is_expensive',
          ...                             features=['bath', 'bedroom', 'size'])
          >>> results = model.evaluate(data)
          >>> print results['accuracy']
        """

        _raise_error_evaluation_metric_is_valid(metric,
                ['auto', 'accuracy', 'confusion_matrix', 'roc_curve', 'auc',
                 'log_loss', 'precision', 'recall', 'f1_score'])
        return super(_Classifier, self).evaluate(dataset,
                                 missing_value_action=missing_value_action,
                                 metric=metric)

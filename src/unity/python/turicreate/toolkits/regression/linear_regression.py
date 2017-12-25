# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Methods for creating and using a linear regression model.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _turicreate
import turicreate.toolkits._supervised_learning as _sl
from turicreate.toolkits._supervised_learning import SupervisedLearningModel as \
                                    _SupervisedLearningModel
from turicreate.toolkits._internal_utils import _toolkit_repr_print, \
                                        _toolkit_get_topk_bottomk, \
                                        _summarize_coefficients, \
                                        _raise_error_evaluation_metric_is_valid


_DEFAULT_SOLVER_OPTIONS = {
'convergence_threshold': 1e-2,
'step_size': 1.0,
'lbfgs_memory_level': 11,
'max_iterations': 10}

def create(dataset, target, features=None, l2_penalty=1e-2, l1_penalty=0.0,
    solver='auto', feature_rescaling=True,
    convergence_threshold = _DEFAULT_SOLVER_OPTIONS['convergence_threshold'],
    step_size = _DEFAULT_SOLVER_OPTIONS['step_size'],
    lbfgs_memory_level = _DEFAULT_SOLVER_OPTIONS['lbfgs_memory_level'],
    max_iterations = _DEFAULT_SOLVER_OPTIONS['max_iterations'],
    validation_set = "auto",
    verbose=True):

    """
    Create a :class:`~turicreate.linear_regression.LinearRegression` to
    predict a scalar target variable as a linear function of one or more
    features. In addition to standard numeric and categorical types, features
    can also be extracted automatically from list- or dictionary-type SFrame
    columns.

    The linear regression module can be used for ridge regression, Lasso, and
    elastic net regression (see References for more detail on these methods). By
    default, this model has an l2 regularization weight of 0.01.

    Parameters
    ----------
    dataset : SFrame
        The dataset to use for training the model.

    target : string
        Name of the column containing the target variable.

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
        Weight on the l2-regularizer of the model. The larger this weight, the
        more the model coefficients shrink toward 0. This introduces bias into
        the model but decreases variance, potentially leading to better
        predictions. The default value is 0.01; setting this parameter to 0
        corresponds to unregularized linear regression. See the ridge
        regression reference for more detail.

    l1_penalty : float, optional
        Weight on l1 regularization of the model. Like the l2 penalty, the
        higher the l1 penalty, the more the estimated coefficients shrink toward
        0. The l1 penalty, however, completely zeros out sufficiently small
        coefficients, automatically indicating features that are not useful for
        the model. The default weight of 0 prevents any features from being
        discarded. See the LASSO regression reference for more detail.

    solver : string, optional
        Solver to use for training the model. See the references for more detail
        on each solver.

        - *auto (default)*: automatically chooses the best solver for the data
          and model parameters.
        - *newton*: Newton-Raphson
        - *lbfgs*: limited memory BFGS
        - *fista*: accelerated gradient descent

        The model is trained using a carefully engineered collection of methods
        that are automatically picked based on the input data. The ``newton``
        method  works best for datasets with plenty of examples and few features
        (long datasets). Limited memory BFGS (``lbfgs``) is a robust solver for
        wide datasets (i.e datasets with many coefficients).  ``fista`` is the
        default solver for l1-regularized linear regression.  The solvers are
        all automatically tuned and the default options should function well.
        See the solver options guide for setting additional parameters for each
        of the solvers.

        See the user guide for additional details on how the solver is chosen.

    feature_rescaling : boolean, optional
        Feature rescaling is an important pre-processing step that ensures that
        all features are on the same scale. An l2-norm rescaling is performed
        to make sure that all features are of the same norm. Categorical
        features are also rescaled by rescaling the dummy variables that are
        used to represent them. The coefficients are returned in original scale
        of the problem. This process is particularly useful when features
        vary widely in their ranges.

    validation_set : SFrame, optional

        A dataset for monitoring the model's generalization performance.
        For each row of the progress table, the chosen metrics are computed
        for both the provided training dataset and the validation_set. The
        format of this SFrame must be the same as the training set.
        By default this argument is set to 'auto' and a validation set is
        automatically sampled and used for progress printing. If
        validation_set is set to None, then no additional metrics
        are computed. The default value is 'auto'.

    convergence_threshold : float, optional

      Convergence is tested using variation in the training objective. The
      variation in the training objective is calculated using the difference
      between the objective values between two steps. Consider reducing this
      below the default value (0.01) for a more accurately trained model.
      Beware of overfitting (i.e a model that works well only on the training
      data) if this parameter is set to a very low value.

    lbfgs_memory_level : int, optional

      The L-BFGS algorithm keeps track of gradient information from the
      previous ``lbfgs_memory_level`` iterations. The storage requirement for
      each of these gradients is the ``num_coefficients`` in the problem.
      Increasing the ``lbfgs_memory_level`` can help improve the quality of
      the model trained. Setting this to more than ``max_iterations`` has the
      same effect as setting it to ``max_iterations``.

    max_iterations : int, optional

      The maximum number of allowed passes through the data. More passes over
      the data can result in a more accurately trained model. Consider
      increasing this (the default value is 10) if the training accuracy is
      low and the *Grad-Norm* in the display is large.

    step_size : float, optional (fista only)

      The starting step size to use for the ``fista`` and ``gd`` solvers. The
      default is set to 1.0, this is an aggressive setting. If the first
      iteration takes a considerable amount of time, reducing this parameter
      may speed up model training.

    verbose : bool, optional
        If True, print progress updates.

    Returns
    -------
    out : LinearRegression
        A trained model of type
        :class:`~turicreate.linear_regression.LinearRegression`.

    See Also
    --------
    LinearRegression, turicreate.boosted_trees_regression.BoostedTreesRegression, turicreate.regression.create

    Notes
    -----
    - Categorical variables are encoded by creating dummy variables. For a
      variable with :math:`K` categories, the encoding creates :math:`K-1` dummy
      variables, while the first category encountered in the data is used as the
      baseline.

    - For prediction and evaluation of linear regression models with sparse
      dictionary inputs, new keys/columns that were not seen during training
      are silently ignored.

    - Any 'None' values in the data will result in an error being thrown.

    - A constant term is automatically added for the model intercept. This term
      is not regularized.

    - Standard errors on coefficients are only available when `solver=newton`
      or when the default `auto` solver option chooses the newton method and if
      the number of examples in the training data is more than the number of
      coefficients. If standard errors cannot be estimated, a column of `None`
      values are returned.


    References
    ----------
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

    - Barzilai, J. and Borwein, J. `Two-Point Step Size Gradient Methods
      <http://imajna.oxfordjournals.org/content/8/1/141.short>`_. IMA Journal of
      Numerical Analysis 8(1) pp.141-148.

    - Beck, A. and Teboulle, M. (2009) `A Fast Iterative Shrinkage-Thresholding
      Algorithm for Linear Inverse Problems
      <http://epubs.siam.org/doi/abs/10.1137/080716542>`_. SIAM Journal on
      Imaging Sciences 2(1) pp.183-202.

    - Zhang, T. (2004) `Solving large scale linear prediction problems using
      stochastic gradient descent algorithms
      <https://dl.acm.org/citation.cfm?id=1015332>`_. ICML '04: Proceedings of
      the twenty-first international conference on Machine learning p.116.


    Examples
    --------

    Given an :class:`~turicreate.SFrame` ``sf`` with a list of columns
    [``feature_1`` ... ``feature_K``] denoting features and a target column
    ``target``, we can create a
    :class:`~turicreate.linear_regression.LinearRegression` as follows:

    >>> data =  turicreate.SFrame('https://static.turi.com/datasets/regression/houses.csv')

    >>> model = turicreate.linear_regression.create(data, target='price',
    ...                                  features=['bath', 'bedroom', 'size'])


    For ridge regression, we can set the ``l2_penalty`` parameter higher (the
    default is 0.01). For Lasso regression, we set the l1_penalty higher, and
    for elastic net, we set both to be higher.

    .. sourcecode:: python

      # Ridge regression
      >>> model_ridge = turicreate.linear_regression.create(data, 'price', l2_penalty=0.1)

      # Lasso
      >>> model_lasso = turicreate.linear_regression.create(data, 'price', l2_penalty=0.,
                                                                   l1_penalty=1.0)

      # Elastic net regression
      >>> model_enet  = turicreate.linear_regression.create(data, 'price', l2_penalty=0.5,
                                                                 l1_penalty=0.5)

    """

    # Regression model names.
    model_name = "regression_linear_regression"
    solver = solver.lower()

    model = _sl.create(dataset, target, model_name, features=features,
                        validation_set = validation_set,
                        solver = solver, verbose = verbose,
                        l2_penalty=l2_penalty, l1_penalty = l1_penalty,
                        feature_rescaling = feature_rescaling,
                        convergence_threshold = convergence_threshold,
                        step_size = step_size,
                        lbfgs_memory_level = lbfgs_memory_level,
                        max_iterations = max_iterations)

    return LinearRegression(model.__proxy__)


class LinearRegression(_SupervisedLearningModel):
    """
    Linear regression is an approach for modeling a scalar target :math:`y` as
    a linear function of one or more explanatory variables denoted :math:`X`.

    Given a set of features :math:`x_i`, and a label :math:`y_i`, linear
    regression interprets the probability that the label is in one class as
    a linear function of a linear combination of the features.

        .. math::
          f_i(\\theta) =  \\theta^T x + \epsilon_i

    where :math:`\epsilon_i` is noise.  An intercept term is added by appending
    a column of 1's to the features.  Regularization is often required to
    prevent overfitting by penalizing models with extreme parameter values. The
    linear regression module supports l1 and l2 regularization, which are added
    to the loss function.

    The composite objective being optimized for is the following:

        .. math::
           \min_{\\theta} \sum_{i = 1}^{n} (\\theta^Tx - y_i)^2 + \lambda_1 ||\\theta||_1 + \lambda_2 ||\\theta||^{2}_{2}

    where :math:`\lambda_1` is the ``l1_penalty`` and :math:`\lambda_2` is the
    ``l2_penalty``.

    This model cannot be constructed directly.  Instead, use
    :func:`turicreate.linear_regression.create` to create an instance
    of this model. A detailed list of parameter options and code samples
    are available in the documentation for the create function.

    Examples
    --------
    .. sourcecode:: python

        # Load the data (From an S3 bucket)
        >>> data =  turicreate.SFrame('https://static.turi.com/datasets/regression/houses.csv')

        # Make a linear regression model
        >>> model = turicreate.linear_regression.create(data, target='price', features=['bath', 'bedroom', 'size'])

        # Extract the coefficients
        >>> coefficients = model.coefficients

        # Make predictions
        >>> predictions = model.predict(data)

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
        return "regression_linear_regression"

    def __str__(self):
        """
        Return a string description of the model, including a description of
        the training data, training statistics, and model hyper-parameters.

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
            ('Number of feature columns', 'num_features'),
            ('Number of unpacked features', 'num_unpacked_features')]

        hyperparam_fields = [
            ("L1 penalty", 'l1_penalty'),
            ("L2 penalty", 'l2_penalty')]

        solver_fields = [
            ("Solver", 'solver'),
            ("Solver iterations", 'training_iterations'),
            ("Solver status", 'training_solver_status'),
            ("Training time (sec)", 'training_time')]

        training_fields = [
            ("Residual sum of squares", 'training_loss'),
            ("Training RMSE", 'training_rmse')]

        coefs = self.coefficients
        top_coefs, bottom_coefs = _toolkit_get_topk_bottomk(coefs,k=5)

        (coefs_list, titles_list) = _summarize_coefficients(top_coefs, \
                                                                    bottom_coefs)

        return ([model_fields, hyperparam_fields,
                        solver_fields, training_fields] + coefs_list, \
                            [ 'Schema', 'Hyperparameters', \
                            'Training Summary', 'Settings' ] + titles_list )

    def __repr__(self):
        """
        Return a string description of the model, including a description of
        the training data, training statistics, and model hyper-parameters.

        Returns
        -------
        out : string
            A description of the model.
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
        from turicreate.extensions import _linear_regression_export_as_model_asset
        from turicreate.toolkits import _coreml_utils
        display_name = "linear regression"
        short_description = _coreml_utils._mlmodel_short_description(display_name)
        context = {"class": self.__class__.__name__,
                   "version": _turicreate.__version__,
                   "short_description": short_description}
        _linear_regression_export_as_model_asset(self.__proxy__, filename, context)

    def _get(self, field):
        """
        Get the value of a given field. The list of all queryable fields is
        detailed below, and can be obtained programmatically using the
        :func:`~turicreate.linear_regression.LinearRegression._list_fields`
        method.

        +------------------------+-------------------------------------------------------------+
        |      Field             | Description                                                 |
        +========================+=============================================================+
        | coefficients           | Regression coefficients                                     |
        +------------------------+-------------------------------------------------------------+
        | convergence_threshold  | Desired solver accuracy                                     |
        +------------------------+-------------------------------------------------------------+
        | feature_rescaling      | Bool indicating if features were rescaled during training   |
        +------------------------+-------------------------------------------------------------+
        | features               | Feature column names                                        |
        +------------------------+-------------------------------------------------------------+
        | l1_penalty             | l1 regularization weight                                    |
        +------------------------+-------------------------------------------------------------+
        | l2_penalty             | l2 regularization weight                                    |
        +------------------------+-------------------------------------------------------------+
        | lbfgs_memory_level     | LBFGS memory level ('lbfgs only')                           |
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
        | training_loss          | Residual sum-of-squares training loss                       |
        +------------------------+-------------------------------------------------------------+
        | training_rmse          | Training root-mean-squared-error (RMSE)                     |
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
        out : [various]
            The current value of the requested field.
        """
        return super(LinearRegression, self)._get(field)

    def predict(self, dataset, missing_value_action='auto'):
        """
        Return target value predictions for ``dataset``, using the trained
        linear regression model. This method can be used to get fitted values
        for the model by inputting the training dataset.

        Parameters
        ----------
        dataset : SFrame | pandas.Dataframe
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
            - 'error': Do not proceed with prediction and terminate with
              an error message.


        Returns
        -------
        out : SArray
            Predicted target value for each example (i.e. row) in the dataset.

        See Also
        ----------
        create, evaluate

        Examples
        ----------
        >>> data =  turicreate.SFrame('https://static.turi.com/datasets/regression/houses.csv')

        >>> model = turicreate.linear_regression.create(data,
                                             target='price',
                                             features=['bath', 'bedroom', 'size'])
        >>> results = model.predict(data)
        """

        return super(LinearRegression, self).predict(dataset, missing_value_action=missing_value_action)

    
    def evaluate(self, dataset, metric='auto', missing_value_action='auto'):
        r"""Evaluate the model by making target value predictions and comparing
        to actual values.

        Two metrics are used to evaluate linear regression models.  The first
        is root-mean-squared error (RMSE) while the second is the absolute
        value of the maximum error between the actual and predicted values.
        Let :math:`y` and :math:`\hat{y}` denote vectors of length :math:`N`
        (number of examples) with actual and predicted values. The RMSE is
        defined as:

        .. math::

            RMSE = \sqrt{\frac{1}{N} \sum_{i=1}^N (\widehat{y}_i - y_i)^2}

        while the max-error is defined as

        .. math::

            max-error = \max_{i=1}^N \|\widehat{y}_i - y_i\|

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the target and features used for model training. Additional
            columns are ignored.

        metric : str, optional
            Name of the evaluation metric.  Possible values are:
            - 'auto': Compute all metrics.
            - 'rmse': Rooted mean squared error.
            - 'max_error': Maximum error.

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
            Results from  model evaluation procedure.

        See Also
        ----------
        create, predict

        Examples
        ----------
        >>> data =  turicreate.SFrame('https://static.turi.com/datasets/regression/houses.csv')

        >>> model = turicreate.linear_regression.create(data,
                                             target='price',
                                             features=['bath', 'bedroom', 'size'])
        >>> results = model.evaluate(data)
        """

        _raise_error_evaluation_metric_is_valid(metric,
                                          ['auto', 'rmse', 'max_error'])
        return super(LinearRegression, self).evaluate(dataset, missing_value_action=missing_value_action,
                                                      metric=metric)

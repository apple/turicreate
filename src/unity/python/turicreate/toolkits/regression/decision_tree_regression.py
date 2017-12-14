# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
This package contains the decision tree model class and the create function.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _turicreate
from turicreate.toolkits._supervised_learning import SupervisedLearningModel as _SupervisedLearningModel
import turicreate.toolkits._supervised_learning as _sl
import turicreate.toolkits._main as _toolkits_main
from turicreate.toolkits._internal_utils import _toolkit_repr_print
from turicreate.toolkits._internal_utils import _raise_error_evaluation_metric_is_valid
from turicreate.toolkits._internal_utils import _raise_error_if_column_exists

from turicreate.toolkits._tree_model_mixin import TreeModelMixin as _TreeModelMixin

from turicreate.toolkits._internal_utils import _raise_error_if_not_sframe
from turicreate.toolkits._internal_utils import _map_unity_proxy_to_object



_DECISION_TREE_MODEL_PARAMS_KEYS = ['max_depth', 'min_child_weight',
'min_loss_reduction']
_DECISION_TREE_TRAINING_PARAMS_KEYS = ['objective', 'training_time',
'training_error', 'validation_error', 'evaluation_metric']
_DECISION_TREE_TRAINING_DATA_PARAMS_KEYS = ['target', 'features',
'num_features', 'num_examples', 'num_validation_examples']


class DecisionTreeRegression(_SupervisedLearningModel, _TreeModelMixin):
    """

    The prediction is based on a collection of base learners, `regression trees
    <http://en.wikipedia.org/wiki/Decision_tree_learning>`_. This algorithm is
    a special case for boosted trees regression with number of trees set to 1.


    Different from linear models, e.g. linear regression, the gradient boost
    trees model is able to model non-linear interactions between the features
    and the target using decision trees as the subroutine.  It is good for
    handling numerical features and categorical features with tens of
    categories but is less suitable for highly sparse features such as text
    data.

    This model cannot be constructed directly.  Instead, use
    :func:`turicreate.decision_tree_regression.create` to create an instance of
    this model. A detailed list of parameter options and code samples are
    available in the documentation for the create function.

    See Also
    --------
    create
    """
    def __init__(self, proxy):
        """__init__(self)"""
        self.__proxy__ = proxy
        self.__name__ = self.__class__._native_name()

    @classmethod
    def _native_name(cls):
        return "decision_tree_regression"

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
        data_fields = [
            ('Number of examples', 'num_examples'),
            ('Number of feature columns', 'num_features'),
            ('Number of unpacked features', 'num_unpacked_features')]

        training_fields = [
            ("Max tree depth", 'max_depth'),
            ("Train RMSE", 'training_rmse'),
            ("Validation RMSE", 'validation_rmse'),
            ("Training time (sec)", 'training_time')]

        return ( [data_fields, training_fields], ['Schema', 'Settings'])

    def __repr__(self):
        """
        Print a string description of the model, when the model name is entered
        in the terminal.
        """

        (sections, section_titles) = self._get_summary_struct()

        return _toolkit_repr_print(self, sections, section_titles, width=30)

    def _get(self, field):
        """
        Get the value of a given field. The list of all queryable fields is
        detailed below, and can be obtained programmatically using the
        :func:`~turicreate.decision_tree_regression._list_fields` method.

        +-------------------------+--------------------------------------------------------------------------------+
        | Field                   | Description                                                                    |
        +=========================+================================================================================+
        | column_subsample        | Percentage of the columns for training each individual tree                    |
        +-------------------------+--------------------------------------------------------------------------------+
        | features                | Names of the feature columns                                                   |
        +-------------------------+--------------------------------------------------------------------------------+
        | max_depth               | The maximum depth of individual trees                                          |
        +-------------------------+--------------------------------------------------------------------------------+
        | min_child_weight        | Minimum weight required on the leave nodes                                     |
        +-------------------------+--------------------------------------------------------------------------------+
        | min_loss_reduction      | Minimum loss reduction required for splitting a node                           |
        +-------------------------+--------------------------------------------------------------------------------+
        | num_features            | Number of features in the model                                                |
        +-------------------------+--------------------------------------------------------------------------------+
        | num_unpacked_features   | Number of features in the model (including unpacked dict/list type columns)    |
        +-------------------------+--------------------------------------------------------------------------------+
        | num_examples            | Number of training examples                                                    |
        +-------------------------+--------------------------------------------------------------------------------+
        | num_validation_examples | Number of validation examples                                                  |
        +-------------------------+--------------------------------------------------------------------------------+
        | target                  | Name of the target column                                                      |
        +-------------------------+--------------------------------------------------------------------------------+
        | training_error          | Error on training data                                                         |
        +-------------------------+--------------------------------------------------------------------------------+
        | training_time           | Time spent on training the model in seconds                                    |
        +-------------------------+--------------------------------------------------------------------------------+
        | trees_json              | Tree encoded using JSON                                                        |
        +-------------------------+--------------------------------------------------------------------------------+
        | validation_error        | Error on validation data                                                       |
        +-------------------------+--------------------------------------------------------------------------------+
        | unpacked_features       | Feature names (including expanded list/dict features)                          |
        +-------------------------+--------------------------------------------------------------------------------+
        | random_seed             | Seed for row and column subselection                                           |
        +-------------------------+--------------------------------------------------------------------------------+
        | metric                  | Performance metric(s) that are tracked during training                         |
        +-------------------------+--------------------------------------------------------------------------------+

        Parameters
        ----------
        field : string
            Name of the field to be retrieved.

        Returns
        -------
        out : [various]
            The current value of the requested field.

        Examples
        --------
        >>> m.get('training_error')
        """
        return super(DecisionTreeRegression, self)._get(field)

    def evaluate(self, dataset, metric='auto', missing_value_action='auto'):
        """
        Evaluate the model on the given dataset.

        Parameters
        ----------
        dataset : SFrame
            Dataset in the same format used for training. The columns names and
            types of the dataset must be the same as that used in training.

        metric : str, optional
            Name of the evaluation metric.  Can be one of:

            - 'auto': Compute all metrics.
            - 'rmse': Rooted mean squared error.
            - 'max_error': Maximum error.

        missing_value_action : str, optional
            Action to perform when missing values are encountered. Can be
            one of:

            - 'auto': By default the model will treat missing value as is.
            - 'impute': Proceed with evaluation by filling in the missing
              values with the mean of the training data. Missing
              values are also imputed if an entire column of data is
              missing during evaluation.
            - 'error': Do not proceed with evaluation and terminate with
              an error message.

        Returns
        -------
        out : dict
            A dictionary containing the evaluation result.

        See Also
        ----------
        create, predict

        Examples
        --------
        ..sourcecode:: python

          >>> results = model.evaluate(test_data, 'rmse')

        """
        _raise_error_evaluation_metric_is_valid(
                metric, ['auto', 'rmse', 'max_error'])
        return super(DecisionTreeRegression, self).evaluate(dataset,
                                 missing_value_action=missing_value_action,
                                 metric=metric)

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
        from turicreate.toolkits import _coreml_utils
        display_name = "decision tree regression"
        short_description = _coreml_utils._mlmodel_short_description(display_name)
        context = {"mode" : "regression",
                   "model_type" : "decision_tree",
                   "version": _turicreate.__version__,
                   "class": self.__class__.__name__,
                   "short_description": short_description}
        self._export_coreml_impl(filename, context)

    def predict(self, dataset, missing_value_action='auto'):
        """
        Predict the target column of the given dataset.

        The target column is provided during
        :func:`~turicreate.decision_tree_regression.create`. If the target column is in the
        `dataset` it will be ignored.

        Parameters
        ----------
        dataset : SFrame
          A dataset that has the same columns that were used during training.
          If the target column exists in ``dataset`` it will be ignored
          while making predictions.

        missing_value_action : str, optional
            Action to perform when missing values are encountered. Can be
            one of:

            - 'auto': By default the model will treat missing value as is.
            - 'impute': Proceed with evaluation by filling in the missing
              values with the mean of the training data. Missing
              values are also imputed if an entire column of data is
              missing during evaluation.
            - 'error': Do not proceed with evaluation and terminate with
              an error message.

        Returns
        -------
        out : SArray
           Predicted target value for each example (i.e. row) in the dataset.

        See Also
        ----------
        create, predict

        Examples
        --------
        >>> m.predict(testdata)
        """
        return super(DecisionTreeRegression, self).predict(dataset, output_type='margin',
                                                           missing_value_action=missing_value_action)

    @classmethod
    def _get_queryable_methods(cls):
        '''Returns a list of method names that are queryable through Predictive
        Service'''
        methods = _SupervisedLearningModel._get_queryable_methods()
        methods['extract_features'] = {'dataset': 'sframe'}
        return methods


def create(dataset, target,
           features=None,
           validation_set='auto',
           max_depth=6,
           min_loss_reduction=0.0, min_child_weight=0.1,
           verbose=True,
           random_seed = None,
           metric = 'auto',
           **kwargs):
    """
    Create a :class:`~turicreate.decision_tree_regression.DecisionTreeRegression` to predict
    a scalar target variable using one or more features. In addition to standard
    numeric and categorical types, features can also be extracted automatically
    from list- or dictionary-type SFrame columns.


    Parameters
    ----------
    dataset : SFrame
        A training dataset containing feature columns and a target column.
        Only numerical typed (int, float) target column is allowed.

    target : str
        The name of the column in ``dataset`` that is the prediction target.
        This column must have a numeric type.

    features : list[str], optional
        A list of columns names of features used for training the model.
        Defaults to None, using all columns.

    validation_set : SFrame, optional
        The validation set that is used to watch the validation result as
        boosting progress.

    max_depth : float, optional
        Maximum depth of a tree. Must be at least 1.

    min_loss_reduction : float, optional (non-negative)
        Minimum loss reduction required to make a further partition/split a
        node during the tree learning phase. Larger (more positive) values
        can help prevent overfitting by avoiding splits that do not
        sufficiently reduce the loss function.

    min_child_weight : float, optional (non-negative)
        Controls the minimum weight of each leaf node. Larger values result in
        more conservative tree learning and help prevent overfitting.
        Formally, this is minimum sum of instance weights (hessians) in each
        node. If the tree learning algorithm results in a leaf node with the
        sum of instance weights less than `min_child_weight`, tree building
        will terminate.

    verbose : boolean, optional
        If True, print progress information during training.

    random_seed: int, optional
        Seeds random operations such as column and row subsampling, such that
        results are reproducible.

    metric : str or list[str], optional
        Performance metric(s) that are tracked during training. When specified,
        the progress table will display the tracked metric(s) on training and
        validation set.
        Supported metrics are: {'rmse', 'max_error'}

    Returns
    -------
      out : DecisionTreeRegression
          A trained decision tree model

    References
    ----------
    - `Wikipedia - Gradient tree boosting
      <http://en.wikipedia.org/wiki/Gradient_boosting#Gradient_tree_boosting>`_
    - `Trevor Hastie's slides on Boosted Trees and Random Forest
      <http://jessica2.msri.org/attachments/10778/10778-boost.pdf>`_

    See Also
    --------
    DecisionTreeRegression, turicreate.linear_regression.LinearRegression, turicreate.regression.create

    Examples
    --------

    Setup the data:

    >>> url = 'https://static.turi.com/datasets/xgboost/mushroom.csv'
    >>> data = turicreate.SFrame.read_csv(url)
    >>> data['label'] = data['label'] == 'p'

    Split the data into training and test data:

    >>> train, test = data.random_split(0.8)

    Create the model:

    >>> model = turicreate.decision_tree_regression.create(train, target='label')

    Make predictions and evaluate the model:

    >>> predictions = model.predict(test)
    >>> results = model.evaluate(test)

    """

    if random_seed is not None:
        kwargs['random_seed'] = random_seed

    model = _sl.create(dataset = dataset,
                        target = target,
                        features = features,
                        model_name = 'decision_tree_regression',
                        validation_set = validation_set,
                        max_depth = max_depth,
                        min_loss_reduction = min_loss_reduction,
                        min_child_weight = min_child_weight,
                        verbose = verbose, **kwargs)
    return DecisionTreeRegression(model.__proxy__)

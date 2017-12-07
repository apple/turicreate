# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
This package contains the Gradient Boosted Trees model class and the create function.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _turicreate
from turicreate.toolkits._supervised_learning import Classifier as _Classifier
import turicreate.toolkits._supervised_learning as _sl
import turicreate.toolkits._main as _toolkits_main

from turicreate.toolkits._internal_utils import _toolkit_repr_print
from turicreate.toolkits._internal_utils import _raise_error_evaluation_metric_is_valid
from turicreate.toolkits._internal_utils import _raise_error_if_not_sframe
from turicreate.toolkits._internal_utils import _raise_error_if_column_exists
from turicreate.toolkits._internal_utils import _check_categorical_option_type
from turicreate.toolkits._internal_utils import _map_unity_proxy_to_object
from turicreate.toolkits._tree_model_mixin import TreeModelMixin as _TreeModelMixin
from turicreate.util import _make_internal_url


__doc_string_context = '''
      >>> url = 'https://static.turi.com/datasets/xgboost/mushroom.csv'
      >>> data = turicreate.SFrame.read_csv(url)

      >>> train, test = data.random_split(0.8)
      >>> model = turicreate.boosted_trees_classifier.create(train, target='label')
'''

class BoostedTreesClassifier(_Classifier, _TreeModelMixin):
    """
    The gradient boosted trees model can be used as a classifier for predictive
    tasks.

    The prediction is based on a collection of base learners i.e
    `decision tree classifiers <http://en.wikipedia.org/wiki/Decision_tree_learning>`_
    and combines them through a technique called `gradient boosting
    <http://en.wikipedia.org/wiki/Gradient_boosting>`_.

    Different from linear models like logistic regression or SVM, gradient
    boost trees can model non-linear interactions between the features and the
    target. This model is suitable for handling numerical features and
    categorical features with tens of categories but is less suitable for highly
    sparse features (text data), or with categorical variables that encode a
    large number of categories.

    This model cannot be constructed directly.  Instead, use
    :func:`turicreate.boosted_trees_classifier.create` to create an instance of
    this model.
    Additional details on parameter options and code samples are available in
    the documentation for the create function.

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
        return "boosted_trees_classifier"

    def __str__(self):
        """
        Return a string description of the model to the ``print`` method.

        Returns
        -------
        out : string
            A description of the model.
        """
        return self.__repr__()

    def __repr__(self):
        """
        Print a string description of the model, when the model name is entered
        in the terminal.
        """
        (sections, section_titles) = self._get_summary_struct()

        return _toolkit_repr_print(self, sections, section_titles, width=30)

    def _get(self, field):
        """
        Get the value of a given field. The following table describes each
        of the fields below.

        +-------------------------+--------------------------------------------------------------------------------+
        | Field                   | Description                                                                    |
        +=========================+================================================================================+
        | column_subsample        | Percentage of the columns for training each individual tree                    |
        +-------------------------+--------------------------------------------------------------------------------+
        | features                | Names of the feature columns                                                   |
        +-------------------------+--------------------------------------------------------------------------------+
        | max_depth               | The maximum depth of the individual decision trees                             |
        +-------------------------+--------------------------------------------------------------------------------+
        | max_iterations          | Maximum number of iterations for training (one tree is trained per iteration)  |
        +-------------------------+--------------------------------------------------------------------------------+
        | min_child_weight        | Minimum weight assigned to leaf nodes                                          |
        +-------------------------+--------------------------------------------------------------------------------+
        | min_loss_reduction      | Minimum loss reduction required for splitting a node                           |
        +-------------------------+--------------------------------------------------------------------------------+
        | num_features            | Number of feature columns in the model                                         |
        +-------------------------+--------------------------------------------------------------------------------+
        | num_unpacked_features   | Number of features in the model (including unpacked dict/list type columns)    |
        +-------------------------+--------------------------------------------------------------------------------+
        | num_examples            | Number of training examples                                                    |
        +-------------------------+--------------------------------------------------------------------------------+
        | num_validation_examples | Number of validation examples                                                  |
        +-------------------------+--------------------------------------------------------------------------------+
        | row_subsample           | Percentage of the rows sampled for training each individual tree               |
        +-------------------------+--------------------------------------------------------------------------------+
        | step_size               | Step_size used for combining the weight of individual trees                    |
        +-------------------------+--------------------------------------------------------------------------------+
        | target                  | Name of the target column                                                      |
        +-------------------------+--------------------------------------------------------------------------------+
        | training_accuracy       | Classification accuracy measured on the training data                          |
        +-------------------------+--------------------------------------------------------------------------------+
        | training_time           | Time spent on training the model in seconds                                    |
        +-------------------------+--------------------------------------------------------------------------------+
        | trees_json              | Tree encoded using JSON                                                        |
        +-------------------------+--------------------------------------------------------------------------------+
        | validation_accuracy     | Classification accuracy measured on the validation set                         |
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
        """
        return super(_Classifier, self)._get(field)

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
        --------
        .. sourcecode:: python

          >>> results = model.evaluate(test_data)
          >>> results = model.evaluate(test_data, metric='accuracy')
          >>> results = model.evaluate(test_data, metric='confusion_matrix')

        """
        _raise_error_evaluation_metric_is_valid(metric,
                ['auto', 'accuracy', 'confusion_matrix', 'roc_curve', 'auc',
                 'log_loss', 'precision', 'recall', 'f1_score'])
        return super(_Classifier, self).evaluate(dataset,
                                 missing_value_action=missing_value_action,
                                 metric=metric)

    def predict(self, dataset, output_type='class', missing_value_action='auto'):
        """
        A flexible and advanced prediction API.

        The target column is provided during
        :func:`~turicreate.boosted_trees.create`. If the target column is in the
        `dataset` it will be ignored.

        Parameters
        ----------
        dataset : SFrame
          A dataset that has the same columns that were used during training.
          If the target column exists in ``dataset`` it will be ignored
          while making predictions.

        output_type : {'probability', 'margin', 'class', 'probability_vector'}, optional.
            Form of the predictions which are one of:

            - 'probability': Prediction probability associated with the True
               class (not applicable for multi-class classification)
            - 'margin': Margin associated with the prediction (not applicable
              for multi-class classification)
            - 'probability_vector': Prediction probability associated with each
              class as a vector. The probability of the first class (sorted
              alphanumerically by name of the class in the training set) is in
              position 0 of the vector, the second in position 1 and so on.
            - 'class': Class prediction. For multi-class classification, this
               returns the class with maximum probability.

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
        create, evaluate, classify

        Examples
        --------
        >>> m.predict(testdata)
        >>> m.predict(testdata, output_type='probability')
        >>> m.predict(testdata, output_type='margin')
        """
        _check_categorical_option_type('output_type', output_type,
                ['class', 'margin', 'probability', 'probability_vector'])
        return super(_Classifier, self).predict(dataset,
                                                output_type=output_type,
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

            - 'auto': By default the model will treat missing value as is.
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
        _check_categorical_option_type('output_type', output_type, ['rank', 'margin', 'probability'])
        if missing_value_action == 'auto':
            missing_value_action = _sl.select_default_missing_value_policy(self, 'predict')

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
        options.update({'model': self.__proxy__,
                        'model_name': self.__name__,
                        'dataset': dataset,
                        'output_type': output_type,
                        'topk': k,
                        'missing_value_action': missing_value_action})
        target = _turicreate.toolkits._main.run('supervised_learning_predict_topk', options)
        return _map_unity_proxy_to_object(target['predicted'])

    def classify(self, dataset, missing_value_action='auto'):
        """
        Return a classification, for each example in the ``dataset``, using the
        trained boosted trees model. The output SFrame contains predictions
        as class labels (0 or 1) and probabilities associated with the the example.

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the features used for model training, but does not require
            a target column. Additional columns are ignored.

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
        out : SFrame
            An SFrame with model predictions i.e class labels and probabilities
            associated with each of the class labels.

        See Also
        ----------
        create, evaluate, predict

        Examples
        ----------
        >>> data =  turicreate.SFrame('https://static.turi.com/datasets/regression/houses.csv')

        >>> data['is_expensive'] = data['price'] > 30000
        >>> model = turicreate.boosted_trees_classifier.create(data,
        >>>                                                  target='is_expensive',
        >>>                                                  features=['bath', 'bedroom', 'size'])

        >>> classes = model.classify(data)

        """
        return super(BoostedTreesClassifier, self).classify(dataset,
                                                            missing_value_action=missing_value_action)

    @classmethod
    def _get_queryable_methods(cls):
        '''Returns a list of method names that are queryable through Predictive
        Service'''
        methods = _Classifier._get_queryable_methods()
        methods['extract_features'] = {'dataset': 'sframe'}
        return methods

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
        display_name = "boosted trees classifier"
        short_description = _coreml_utils._mlmodel_short_description(display_name)
        context = {"mode" : "classification",
                   "model_type" : "boosted_trees",
                   "version": _turicreate.__version__,
                   "class": self.__class__.__name__,
                   "short_description": short_description}
        self._export_coreml_impl(filename, context)

def create(dataset, target,
           features=None, max_iterations=10,
           validation_set='auto',
           class_weights = None,
           max_depth=6, step_size=0.3,
           min_loss_reduction=0.0, min_child_weight=0.1,
           row_subsample=1.0, column_subsample=1.0,
           verbose=True,
           random_seed = None,
           metric='auto',
           **kwargs):
    """
    Create a (binary or multi-class) classifier model of type
    :class:`~turicreate.boosted_trees_classifier.BoostedTreesClassifier` using
    gradient boosted trees (sometimes known as GBMs).

    Parameters
    ----------
    dataset : SFrame
        A training dataset containing feature columns and a target column.

    target : str
        Name of the column containing the target variable. The values in this
        column must be of string or integer type.  String target variables are
        automatically mapped to integers in alphabetical order of the variable values.
        For example, a target variable with 'cat', 'dog', and 'foosa' as possible
        values is mapped to 0, 1, and, 2 respectively.

    features : list[str], optional
        A list of columns names of features used for training the model.
        Defaults to None, which uses all columns in the SFrame ``dataset``
        excepting the target column..

    max_iterations : int, optional
        The maximum number of iterations for boosting. Each iteration results
        in the creation of an extra tree.

    validation_set : SFrame, optional
        A dataset for monitoring the model's generalization performance.
        For each row of the progress table, the chosen metrics are computed
        for both the provided training dataset and the validation_set. The
        format of this SFrame must be the same as the training set.
        By default this argument is set to 'auto' and a validation set is
        automatically sampled and used for progress printing. If
        validation_set is set to None, then no additional metrics
        are computed. This is computed once per full iteration. Large
        differences in model accuracy between the training data and validation
        data is indicative of overfitting. The default value is 'auto'.

    class_weights : {dict, `auto`}, optional

        Weights the examples in the training data according to the given class
        weights. If provided, the dictionary must contain a key for each class
        label. The value can be any positive number greater than 1e-20. Weights
        are interpreted as relative to each other. So setting the weights to be
        2.0 for the positive class and 1.0 for the negative class has the same
        effect as setting them to be 20.0 and 10.0, respectively. If set to
        `None`, all classes are taken to have weight 1.0. The `auto` mode sets
        the class weight to be inversely proportional to the number of examples
        in the training data with the given class.

    max_depth : float, optional
        Maximum depth of a tree. Must be at least 1.

    step_size : float, [0,1], optional
        Step size (shrinkage) used in update to prevents overfitting.  It
        shrinks the prediction of each weak learner to make the boosting
        process more conservative.  The smaller the step size, the more conservative
        the algorithm will be. Smaller step_size work well when
        `max_iterations` is large.

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

    row_subsample : float, [0,1], optional
        Subsample the ratio of the training set in each iteration of tree
        construction.  This is called the bagging trick and can usually help
        prevent overfitting.  Setting this to a value of 0.5 results in the
        model randomly sampling half of the examples (rows) to grow each tree.

    column_subsample : float, [0,1], optional
        Subsample ratio of the columns in each iteration of tree
        construction.  Like row_subsample, this can also help prevent
        model overfitting.  Setting this to a value of 0.5 results in the
        model randomly sampling half of the columns to grow each tree.

    verbose : boolean, optional
        Print progress information during training (if set to true).

    random_seed : int, optional
        Seeds random opertations such as column and row subsampling, such that
        results are reproducable.

    metric : str or list[str], optional
        Performance metric(s) that are tracked during training. When specified,
        the progress table will display the tracked metric(s) on training and
        validation set.
        Supported metrics are: {'accuracy', 'auc', 'log_loss'}

    kwargs : dict, optional
        Additional arguments for training the model.

        - ``early_stopping_rounds`` : int, default None
            If the validation metric does not improve after <early_stopping_rounds>,
            stop training and return the best model.
            If multiple metrics are being tracked, the last one is used.

        - ``model_checkpoint_path`` : str, default None
            If specified, checkpoint the model training to the given path every n iterations,
            where n is specified by ``model_checkpoint_interval``.
            For instance, if `model_checkpoint_interval` is 5, and `model_checkpoint_path` is
            set to ``/tmp/model_tmp``, the checkpoints will be saved into
            ``/tmp/model_tmp/model_checkpoint_5``, ``/tmp/model_tmp/model_checkpoint_10``, ... etc.
            Training can be resumed by setting ``resume_from_checkpoint`` to one of these checkpoints.

        - ``model_checkpoint_interval`` : int, default 5
            If model_check_point_path is specified,
            save the model to the given path every n iterations.

        - ``resume_from_checkpoint`` : str, default None
            Continues training from a model checkpoint. The model must take
            exact the same training data as the checkpointed model.

    Returns
    -------
      out : BoostedTreesClassifier
          A trained gradient boosted trees model for classifications tasks.

    References
    ----------

    - `Wikipedia - Gradient tree boosting
      <http://en.wikipedia.org/wiki/Gradient_boosting#Gradient_tree_boosting>`_
    - `Trevor Hastie's slides on Boosted Trees and Random Forest
      <http://jessica2.msri.org/attachments/10778/10778-boost.pdf>`_

    See Also
    --------
    BoostedTreesClassifier, turicreate.logistic_classifier.LogisticClassifier, turicreate.svm_classifier.SVMClassifier

    Examples
    --------

    .. sourcecode:: python

      >>> url = 'https://static.turi.com/datasets/xgboost/mushroom.csv'
      >>> data = turicreate.SFrame.read_csv(url)

      >>> train, test = data.random_split(0.8)
      >>> model = turicreate.boosted_trees_classifier.create(train, target='label')

      >>> predictions = model.classify(test)
      >>> results = model.evaluate(test)
    """
    if random_seed is not None:
        kwargs['random_seed'] = random_seed
    if 'model_checkpoint_path' in kwargs:
        kwargs['model_checkpoint_path'] = _make_internal_url(kwargs['model_checkpoint_path'])
    if 'resume_from_checkpoint' in kwargs:
        kwargs['resume_from_checkpoint'] = _make_internal_url(kwargs['resume_from_checkpoint'])

    model = _sl.create(dataset = dataset,
                        target = target,
                        features = features,
                        model_name = 'boosted_trees_classifier',
                        max_iterations = max_iterations,
                        validation_set = validation_set,
                        class_weights = class_weights,
                        max_depth = max_depth,
                        step_size = step_size,
                        min_loss_reduction = min_loss_reduction,
                        min_child_weight = min_child_weight,
                        row_subsample = row_subsample,
                        column_subsample = column_subsample,
                        verbose = verbose,
                        metric = metric,
                        **kwargs)
    return BoostedTreesClassifier(model.__proxy__)

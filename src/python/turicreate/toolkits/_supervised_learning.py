# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
##\internal
"""@package turicreate.toolkits
This module defines the (internal) functions used by the supervised_learning_models.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _turicreate

from turicreate.toolkits._model import Model
from turicreate.toolkits._internal_utils import _raise_error_if_not_sframe
from turicreate.toolkits._internal_utils import _validate_data
from turicreate.toolkits._main import ToolkitError
from turicreate._cython.cy_server import QuietProgress


class SupervisedLearningModel(Model):
    """
    Supervised learning module to predict a target variable as a function of
    several feature variables.
    """

    def __init__(self, model_proxy=None, name=None):
        self.__proxy__ = model_proxy
        self.__name__ = name

    @classmethod
    def _native_name(cls):
        return None

    def __str__(self):
        """
        Return a string description of the model to the ``print`` method.

        Returns
        -------
        out : string
            A description of the model.
        """
        return self.__class__.__name__

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
        return self.__class__.__name__

    def predict(
        self, dataset, missing_value_action="auto", output_type="", options={}, **kwargs
    ):
        """
        Return predictions for ``dataset``, using the trained supervised_learning
        model. Predictions are generated as class labels (0 or
        1).

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the features used for model training, but does not require
            a target column. Additional columns are ignored.

        missing_value_action: str, optional
            Action to perform when missing values are encountered. This can be
            one of:

            - 'auto': Choose a model dependent missing value policy.
            - 'impute': Proceed with evaluation by filling in the missing
                        values with the mean of the training data. Missing
                        values are also imputed if an entire column of data is
                        missing during evaluation.
            - 'none': Treat missing value as is. Model must be able to handle missing value.
            - 'error' : Do not proceed with prediction and terminate with
                        an error message.

        output_type : str, optional
            output type that maybe needed by some of the toolkits

        options : dict
            additional options to be passed in to prediction

        kwargs : dict
            additional options to be passed into prediction

        Returns
        -------
        out : SArray
            An SArray with model predictions.
        """
        if missing_value_action == "auto":
            missing_value_action = select_default_missing_value_policy(self, "predict")

        # Low latency path
        if isinstance(dataset, list):
            return self.__proxy__.fast_predict(
                dataset, missing_value_action, output_type
            )
        if isinstance(dataset, dict):
            return self.__proxy__.fast_predict(
                [dataset], missing_value_action, output_type
            )

        # Batch predictions path
        else:
            _raise_error_if_not_sframe(dataset, "dataset")

            return self.__proxy__.predict(dataset, missing_value_action, output_type)

    def evaluate(
        self,
        dataset,
        metric="auto",
        missing_value_action="auto",
        with_predictions=False,
        options={},
        **kwargs
    ):
        """
        Evaluate the model by making predictions of target values and comparing
        these to actual values.

        Parameters
        ----------
        dataset : SFrame
            Dataset in the same format used for training. The columns names and
            types of the dataset must be the same as that used in training.

        metric : str, list[str]
            Evaluation metric(s) to be computed.

        missing_value_action: str, optional
            Action to perform when missing values are encountered. This can be
            one of:

            - 'auto': Choose a model dependent missing value policy.
            - 'impute': Proceed with evaluation by filling in the missing
                        values with the mean of the training data. Missing
                        values are also imputed if an entire column of data is
                        missing during evaluation.
            - 'none': Treat missing value as is. Model must be able to handle missing value.
            - 'error' : Do not proceed with prediction and terminate with
                        an error message.

        options : dict
            additional options to be passed in to prediction

        kwargs : dict
            additional options to be passed into prediction
        """
        if missing_value_action == "auto":
            missing_value_action = select_default_missing_value_policy(self, "evaluate")

        _raise_error_if_not_sframe(dataset, "dataset")
        results = self.__proxy__.evaluate(
            dataset, missing_value_action, metric, with_predictions=with_predictions
        )
        return results

    def _training_stats(self):
        """
        Return a dictionary containing statistics collected during model
        training. These statistics are also available with the ``get`` method,
        and are described in more detail in the documentation for that method.

        Notes
        -----
        """
        return self.__proxy__.get_train_stats()

    def _get(self, field):
        """
        Get the value of a given field.

        Parameters
        ----------
        field : string
            Name of the field to be retrieved.

        Returns
        -------
        out : [various]
            The current value of the requested field.
        """
        return self.__proxy__.get_value(field)


class Classifier(SupervisedLearningModel):
    """
    Classifier module to predict a discrete target variable as a function of
    several feature variables.
    """

    @classmethod
    def _native_name(cls):
        return None

    def classify(self, dataset, missing_value_action="auto"):
        """
        Return predictions for ``dataset``, using the trained supervised_learning
        model. Predictions are generated as class labels (0 or
        1).

        Parameters
        ----------
        dataset: SFrame
            Dataset of new observations. Must include columns with the same
            names as the features used for model training, but does not require
            a target column. Additional columns are ignored.

        missing_value_action: str, optional
            Action to perform when missing values are encountered. This can be
            one of:

            - 'auto': Choose model dependent missing value action
            - 'impute': Proceed with evaluation by filling in the missing
              values with the mean of the training data. Missing
              values are also imputed if an entire column of data is
              missing during evaluation.
            - 'error': Do not proceed with prediction and terminate with
              an error message.
        Returns
        -------
        out : SFrame
            An SFrame with model predictions.
        """
        if missing_value_action == "auto":
            missing_value_action = select_default_missing_value_policy(self, "classify")

        # Low latency path
        if isinstance(dataset, list):
            return self.__proxy__.fast_classify(dataset, missing_value_action)
        if isinstance(dataset, dict):
            return self.__proxy__.fast_classify([dataset], missing_value_action)

        _raise_error_if_not_sframe(dataset, "dataset")
        return self.__proxy__.classify(dataset, missing_value_action)


def print_validation_track_notification():
    print(
        "PROGRESS: Creating a validation set from 5 percent of training data. This may take a while.\n"
        "          You can set ``validation_set=None`` to disable validation tracking.\n"
    )


def create(
    dataset,
    target,
    model_name,
    features=None,
    validation_set="auto",
    distributed="auto",
    verbose=True,
    seed=None,
    **kwargs
):
    """
    Create a :class:`~turicreate.toolkits.SupervisedLearningModel`,

    This is generic function that allows you to create any model that
    implements SupervisedLearningModel This function is normally not called, call
    specific model's create function instead

    Parameters
    ----------
    dataset : SFrame
        Dataset for training the model.

    target : string
        Name of the column containing the target variable. The values in this
        column must be 0 or 1, of integer type.

    model_name : string
        Name of the model

    features : list[string], optional
        List of feature names used by feature column

    validation_set : SFrame, optional
        A dataset for monitoring the model's generalization performance.
        For each row of the progress table, the chosen metrics are computed
        for both the provided training dataset and the validation_set. The
        format of this SFrame must be the same as the training set.
        By default this argument is set to 'auto' and a validation set is
        automatically sampled and used for progress printing. If
        validation_set is set to None, then no additional metrics
        are computed. The default value is 'auto'.

    distributed: env
        The distributed environment

    verbose : boolean
        whether print out messages during training

    seed : int, optional
        Seed for random number generation. Set this value to ensure that the
        same model is created every time.

    kwargs : dict
        Additional parameter options that can be passed
    """

    # Perform error-checking and trim inputs to specified columns
    dataset, validation_set = _validate_data(dataset, target, features, validation_set)

    # Sample a validation set from the training data if requested
    if isinstance(validation_set, str):
        assert validation_set == "auto"
        if dataset.num_rows() >= 100:
            if verbose:
                print_validation_track_notification()
            dataset, validation_set = dataset.random_split(0.95, seed=seed, exact=True)
        else:
            validation_set = _turicreate.SFrame()
    elif validation_set is None:
        validation_set = _turicreate.SFrame()

    # Sanitize model-specific options
    options = {k.lower(): kwargs[k] for k in kwargs}

    # Create a model instance and train it
    model = _turicreate.extensions.__dict__[model_name]()
    with QuietProgress(verbose):
        model.train(dataset, target, validation_set, options)

    return SupervisedLearningModel(model, model_name)


def create_classification_with_model_selector(
    dataset, target, model_selector, features=None, validation_set="auto", verbose=True
):
    """
    Create a :class:`~turicreate.toolkits.SupervisedLearningModel`,

    This is generic function that allows you to create any model that
    implements SupervisedLearningModel. This function is normally not called, call
    specific model's create function instead.

    Parameters
    ----------
    dataset : SFrame
        Dataset for training the model.

    target : string
        Name of the column containing the target variable. The values in this
        column must be 0 or 1, of integer type.

    model_name : string
        Name of the model

    model_selector: function
        Provide a model selector.

    features : list[string], optional
        List of feature names used by feature column

    verbose : boolean
        whether print out messages during training

    """

    # Perform error-checking and trim inputs to specified columns
    dataset, validation_set = _validate_data(dataset, target, features, validation_set)

    # Sample the data
    features_sframe = dataset
    if features_sframe.num_rows() > 1e5:
        fraction = 1.0 * 1e5 / features_sframe.num_rows()
        features_sframe = features_sframe.sample(fraction, seed=0)

    # Get available models for this dataset
    num_classes = len(dataset[target].unique())
    selected_model_names = model_selector(num_classes, features_sframe)

    # Create a validation set
    if isinstance(validation_set, str):
        if validation_set == "auto":
            if dataset.num_rows() >= 100:
                if verbose:
                    print_validation_track_notification()
                dataset, validation_set = dataset.random_split(0.95, exact=True)
            else:
                validation_set = None
        else:
            raise TypeError("Unrecognized value for validation_set.")

    # Match C++ model names with user model names
    python_names = {
        "boosted_trees_classifier": "BoostedTreesClassifier",
        "random_forest_classifier": "RandomForestClassifier",
        "decision_tree_classifier": "DecisionTreeClassifier",
        "classifier_logistic_regression": "LogisticClassifier",
        "classifier_svm": "SVMClassifier",
    }

    # Print useful user-facing progress messages
    if verbose:
        print("PROGRESS: The following methods are available for this type of problem.")
        print("PROGRESS: " + ", ".join([python_names[x] for x in selected_model_names]))
        if len(selected_model_names) > 1:
            print(
                "PROGRESS: The returned model will be chosen according to validation accuracy."
            )

    models = {}
    metrics = {}
    for model_name in selected_model_names:

        # Fit each of the available models
        m = create_selected(
            model_name, dataset, target, features, validation_set, verbose
        )
        models[model_name] = m

        if "validation_accuracy" in m._list_fields():
            metrics[model_name] = m.validation_accuracy
        elif "training_accuracy" in m._list_fields():
            metrics[model_name] = m.training_accuracy

        # Most models have this.
        elif "progress" in m._list_fields():
            prog = m.progress
            validation_column = "Validation Accuracy"
            accuracy_column = "Training Accuracy"
            if validation_column in prog.column_names():
                metrics[model_name] = float(prog[validation_column].tail(1)[0])
            else:
                metrics[model_name] = float(prog[accuracy_column].tail(1)[0])
        else:
            raise ValueError(
                "Model does not have metrics that can be used for model selection."
            )

    # Choose model based on either validation, if available.
    best_model = None
    best_acc = None
    for model_name in selected_model_names:
        if best_acc is None:
            best_model = model_name
            best_acc = metrics[model_name]
        if best_acc is not None and best_acc < metrics[model_name]:
            best_model = model_name
            best_acc = metrics[model_name]

    ret = []
    width = 32
    if len(selected_model_names) > 1:
        ret.append("PROGRESS: Model selection based on validation accuracy:")
        ret.append("---------------------------------------------")
        key_str = "{:<{}}: {}"
        for model_name in selected_model_names:
            name = python_names[model_name]
            row = key_str.format(name, width, str(metrics[model_name]))
            ret.append(row)
        ret.append("---------------------------------------------")
        ret.append(
            "Selecting "
            + python_names[best_model]
            + " based on validation set performance."
        )

    if verbose:
        print("\nPROGRESS: ".join(ret))
    return models[best_model]


def create_selected(
    selected_model_name, dataset, target, features, validation_set="auto", verbose=True
):

    # Create the model
    model = create(
        dataset,
        target,
        selected_model_name,
        features=features,
        validation_set=validation_set,
        verbose=verbose,
    )

    return wrap_model_proxy(model.__proxy__)


def wrap_model_proxy(model_proxy):
    selected_model_name = model_proxy.__class__.__name__

    # Return the model
    if selected_model_name == "boosted_trees_regression":
        return _turicreate.boosted_trees_regression.BoostedTreesRegression(model_proxy)
    elif selected_model_name == "random_forest_regression":
        return _turicreate.random_forest_regression.RandomForestRegression(model_proxy)
    elif selected_model_name == "decision_tree_regression":
        return _turicreate.decision_tree_classifier.DecisionTreeRegression(model_proxy)
    elif selected_model_name == "regression_linear_regression":
        return _turicreate.linear_regression.LinearRegression(model_proxy)
    elif selected_model_name == "boosted_trees_classifier":
        return _turicreate.boosted_trees_classifier.BoostedTreesClassifier(model_proxy)
    elif selected_model_name == "random_forest_classifier":
        return _turicreate.random_forest_classifier.RandomForestClassifier(model_proxy)
    elif selected_model_name == "decision_tree_classifier":
        return _turicreate.decision_tree_classifier.DecisionTreeClassifier(model_proxy)
    elif selected_model_name == "classifier_logistic_regression":
        return _turicreate.logistic_classifier.LogisticClassifier(model_proxy)
    elif selected_model_name == "classifier_svm":
        return _turicreate.svm_classifier.SVMClassifier(model_proxy)
    else:
        raise ToolkitError("Internal error: Incorrect model returned.")


def select_default_missing_value_policy(model, action):
    from .classifier.boosted_trees_classifier import BoostedTreesClassifier
    from .classifier.random_forest_classifier import RandomForestClassifier
    from .classifier.decision_tree_classifier import DecisionTreeClassifier
    from .regression.boosted_trees_regression import BoostedTreesRegression
    from .regression.random_forest_regression import RandomForestRegression
    from .regression.decision_tree_regression import DecisionTreeRegression

    tree_models = [
        BoostedTreesClassifier,
        BoostedTreesRegression,
        RandomForestClassifier,
        RandomForestRegression,
        DecisionTreeClassifier,
        DecisionTreeRegression,
    ]

    if any(isinstance(model, tree_model) for tree_model in tree_models):
        return "none"
    else:
        return "impute"

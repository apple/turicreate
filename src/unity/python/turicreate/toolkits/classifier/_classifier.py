# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _turicreate
from turicreate.toolkits import _supervised_learning as _sl

def create(dataset, target, features=None, validation_set = 'auto',
        verbose=True):
    """
    Automatically create a suitable classifier model based on the provided
    training data.

    To use specific options of a desired model, use the ``create`` function
    of the corresponding model.

    Parameters
    ----------
    dataset : SFrame
        Dataset for training the model.

    target : string
        Name of the column containing the target variable. The values in this
        column must be of string or integer type. String target variables are
        automatically mapped to integers in the order in which they are
        provided.  For example, a target variable with 'cat' and 'dog' as
        possible values is mapped to 0 and 1 respectively with 0 being the base
        class and 1 being the reference class. Use `model.classes` to
        retrieve the order in which the classes are mapped.

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

    validation_set : SFrame, optional
        A dataset for monitoring the model's generalization performance.  For
        each row of the progress table, the chosen metrics are computed for
        both the provided training dataset and the validation_set. The format
        of this SFrame must be the same as the training set.  By default this
        argument is set to 'auto' and a validation set is automatically sampled
        and used for progress printing. If validation_set is set to None, then
        no additional metrics are computed. The default value is 'auto'.

    verbose : boolean, optional
        If True, print progress information during training.

    Returns
    -------
      out : A trained classifier model.

    See Also
    --------
    turicreate.boosted_trees_classifier.BoostedTreesClassifier,
    turicreate.logistic_classifier.LogisticClassifier,
    turicreate.svm_classifier.SVMClassifier,
    turicreate.nearest_neighbor_classifier.NearestNeighborClassifier

    Examples
    --------
    .. sourcecode:: python

      # Setup the data
      >>> import turicreate as tc
      >>> data =  tc.SFrame('https://static.turi.com/datasets/regression/houses.csv')
      >>> data['is_expensive'] = data['price'] > 30000

      # Selects the best model based on your data.
      >>> model = tc.classifier.create(data, target='is_expensive',
      ...                              features=['bath', 'bedroom', 'size'])

      # Make predictions and evaluate results.
      >>> predictions = model.classify(data)
      >>> results = model.evaluate(data)

    """
    return _sl.create_classification_with_model_selector(dataset, target,
            model_selector = _turicreate.extensions._classifier_available_models,
            features = features, validation_set = validation_set, verbose =
            verbose)

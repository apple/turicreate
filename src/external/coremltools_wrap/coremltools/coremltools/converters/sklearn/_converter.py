# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools import __version__ as ct_version
from coremltools.models import _METADATA_VERSION, _METADATA_SOURCE

"""
Defines the primary function for converting scikit-learn models.
"""


def convert(sk_obj, input_features=None, output_feature_names=None):
    """
    Convert scikit-learn pipeline, classifier, or regressor to Core ML format.

    Parameters
    ----------
    sk_obj: model | [model] of scikit-learn format.
        Scikit learn model(s) to convert to a Core ML format.

        The input model may be a single scikit learn model, a scikit learn
        pipeline model, or a list of scikit learn models.

        Currently supported scikit learn models are:

        -   Linear and Logistic Regression
        -   LinearSVC and LinearSVR
        -   SVC and SVR
        -   NuSVC and NuSVR
        -   Gradient Boosting Classifier and Regressor
        -   Decision Tree Classifier and Regressor
        -   Random Forest Classifier and Regressor
        -   Normalizer
        -   Imputer
        -   Standard Scaler
        -   DictVectorizer
        -   One Hot Encoder
        -   KNeighborsClassifier

        The input model, or the last model in a pipeline or list of models,
        determines whether this is exposed as a Transformer, Regressor,
        or Classifier.

        Note that there may not be a one-to-one correspondence between scikit
        learn models and which Core ML models are used to represent them.  For
        example, many scikit learn models are embedded in a pipeline to handle
        processing of input features.


    input_features: str | dict | list

        Optional name(s) that can be given to the inputs of the scikit-learn
        model. Defaults to 'input'.

        Input features can be specified in a number of forms.

        -   Single string: In this case, the input is assumed to be a single
            array, with the number of dimensions set using num_dimensions.

        -   List of strings: In this case, the overall input dimensions to the
            scikit-learn model is assumed to be the length of the list.  If
            neighboring names are identical, they are assumed to be an input
            array of that length.  For example:

               ["a", "b", "c"]

            resolves to

                [("a", Double), ("b", Double), ("c", Double)].

            And:

                ["a", "a", "b"]

            resolves to

                [("a", Array(2)), ("b", Double)].

        - Dictionary: Where the keys are the names and the indices or ranges of
          feature indices.

            In this case, it's presented as a mapping from keys to indices or
            ranges of contiguous indices.  For example,

                {"a" : 0, "b" : [2,3], "c" : 1}

            Resolves to

                [("a", Double), ("c", Double), ("b", Array(2))].

            Note that the ordering is determined by the indices.

        -   List of tuples of the form `(name, datatype)`.  Here, `name` is the
            name of the exposed feature, and `datatype` is an instance of
            `String`, `Double`, `Int64`, `Array`, or `Dictionary`.

    output_feature_names: string or list of strings
            Optional name(s) that can be given to the inputs of the scikit-learn
            model.

        The output_feature_names is interpreted according to the model type:

        - If the scikit-learn model is a transformer, it is the name of the
          array feature output by the final sequence of the transformer
          (defaults to "output").
        - If it is a classifier, it should be a 2-tuple of names giving the top
          class prediction and the array of scores for each class (defaults to
          "classLabel" and "classScores").
        - If it is a regressor, it should give the name of the prediction value
          (defaults to "prediction").

    Returns
    -------
    model:MLModel
        Returns an MLModel instance representing a Core ML model.

    Examples
    --------
    .. sourcecode:: python

        >>> from sklearn.linear_model import LinearRegression
        >>> import pandas as pd

        # Load data
        >>> data = pd.read_csv('houses.csv')

        # Train a model
        >>> model = LinearRegression()
        >>> model.fit(data[["bedroom", "bath", "size"]], data["price"])

         # Convert and save the scikit-learn model
        >>> import coremltools
        >>> coreml_model = coremltools.converters.sklearn.convert(model,
                                                                 ["bedroom", "bath", "size"],
                                                                 "price")
        >>> coreml_model.save('HousePricer.mlmodel')
    """

    # This function is just a thin wrapper around the internal converter so
    # that sklearn isn't actually imported unless this function is called
    from ...models import MLModel

    # NOTE: Providing user-defined class labels will be enabled when
    # several issues with the ordering of the classes are worked out.  For now,
    # to use custom class labels, directly import the internal function below.
    from ._converter_internal import _convert_sklearn_model

    spec = _convert_sklearn_model(
        sk_obj, input_features, output_feature_names, class_labels=None
    )

    model = MLModel(spec)
    from sklearn import __version__ as sklearn_version

    model.user_defined_metadata[_METADATA_VERSION] = ct_version
    model.user_defined_metadata[_METADATA_SOURCE] = "scikit-learn=={0}".format(
        sklearn_version
    )
    return model

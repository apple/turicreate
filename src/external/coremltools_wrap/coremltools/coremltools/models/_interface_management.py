# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import numpy as _np

from . import datatypes
from ._feature_management import process_or_validate_features
from ._feature_management import is_valid_feature_list
from . import _feature_management as _fm
from ..proto import Model_pb2


def set_classifier_interface_params(
    spec,
    features,
    class_labels,
    model_accessor_for_class_labels,
    output_features=None,
    training_features=None,
):
    """
    Common utilities to set the regression interface params.
    """
    # Normalize the features list.
    features = _fm.process_or_validate_features(features)

    if class_labels is None:
        raise ValueError("List of class labels must be provided.")

    n_classes = len(class_labels)

    output_features = _fm.process_or_validate_classifier_output_features(
        output_features, class_labels
    )

    if len(output_features) == 1:
        predicted_class_output, pred_cl_type = output_features[0]
        score_output = None
    elif len(output_features) == 2:
        predicted_class_output, pred_cl_type = output_features[0]
        score_output, score_output_type = output_features[1]
    else:
        raise ValueError(
            "Provided output classes for a classifier must be "
            "a list of features, predicted class and (optionally) class_score."
        )

    spec.description.predictedFeatureName = predicted_class_output

    # Are they out of order?
    if not (pred_cl_type == datatypes.Int64() or pred_cl_type == datatypes.String()):
        raise ValueError(
            "Provided predicted class output type not Int64 or String (%s)."
            % repr(pred_cl_type)
        )

    if score_output is not None:
        if not isinstance(score_output_type, datatypes.Dictionary):
            raise ValueError(
                "Provided class score output type not a Dictionary (%s)."
                % repr(score_output_type)
            )

        if score_output_type.key_type != pred_cl_type:
            raise ValueError(
                (
                    "Provided class score output (%s) key_type (%s) does not "
                    "match type of class prediction (%s)."
                )
                % (score_output, repr(score_output_type.key_type), repr(pred_cl_type))
            )

        spec.description.predictedProbabilitiesName = score_output

    # add input
    for index, (cur_input_name, input_type) in enumerate(features):
        input_ = spec.description.input.add()
        input_.name = cur_input_name
        datatypes._set_datatype(input_.type, input_type)

    # add output
    for index, (cur_output_name, output_type) in enumerate(output_features):
        output_ = spec.description.output.add()
        output_.name = cur_output_name
        datatypes._set_datatype(output_.type, output_type)

    # Add training features
    if training_features is not None:
        spec = set_training_features(spec, training_features)

    # Worry about the class labels
    if pred_cl_type == datatypes.String():
        try:
            for c in class_labels:
                getattr(
                    spec, model_accessor_for_class_labels
                ).stringClassLabels.vector.append(str(c))
        # Not all the classifiers have class labels; in particular the pipeline
        # classifier.  Thus it's not an error if we can't actually set them.
        except AttributeError:
            pass

    else:
        for c in class_labels:
            conv_error = False
            try:
                if not (int(c) == c):
                    conv_error = True
            except:
                conv_error = True

            if conv_error:
                raise TypeError(
                    ("Cannot cast '%s' class to an int type " % str(c))
                    + "(class type determined by type of first class)."
                )

            try:
                getattr(
                    spec, model_accessor_for_class_labels
                ).int64ClassLabels.vector.append(int(c))
            # Not all the classifiers have class labels; in particular the pipeline
            # classifier.  Thus it's not an error if we can't actually set them.
            except AttributeError:
                break

    # And we are done!
    return spec


def set_regressor_interface_params(
    spec, features, output_features, training_features=None
):
    """ Common utilities to set the regressor interface params.
    """
    if output_features is None:
        output_features = [("predicted_class", datatypes.Double())]
    else:
        output_features = _fm.process_or_validate_features(output_features, 1)

    if len(output_features) != 1:
        raise ValueError(
            "Provided output features for a regressor must be " "one Double feature."
        )

    if output_features[0][1] != datatypes.Double():
        raise ValueError("Output type of a regressor must be a Double.")

    prediction_name = output_features[0][0]
    spec.description.predictedFeatureName = prediction_name

    # Normalize the features list.
    features = _fm.process_or_validate_features(features)

    # add input and output features
    for cur_input_name, feature_type in features:
        input_ = spec.description.input.add()
        input_.name = cur_input_name
        datatypes._set_datatype(input_.type, feature_type)

    # Add training features
    if training_features is not None:
        spec = set_training_features(spec, training_features)

    output_ = spec.description.output.add()
    output_.name = prediction_name
    datatypes._set_datatype(output_.type, "Double")
    return spec


def set_transform_interface_params(
    spec,
    input_features,
    output_features,
    are_optional=False,
    training_features=None,
    array_datatype=Model_pb2.ArrayFeatureType.DOUBLE,
):
    """ Common utilities to set transform interface params.
    """
    input_features = _fm.process_or_validate_features(input_features)
    output_features = _fm.process_or_validate_features(output_features)

    # Add input and output features
    for (fname, ftype) in input_features:
        input_ = spec.description.input.add()
        input_.name = fname
        datatypes._set_datatype(input_.type, ftype, array_datatype=array_datatype)
        if are_optional:
            input_.type.isOptional = are_optional

    for (fname, ftype) in output_features:
        output_ = spec.description.output.add()
        output_.name = fname
        datatypes._set_datatype(output_.type, ftype, array_datatype=array_datatype)

    # Add training features
    if training_features is not None:
        spec = set_training_features(spec, training_features)

    return spec


def set_training_features(spec, training_features):
    for (fname, ftype) in training_features:
        training_input_ = spec.description.trainingInput.add()
        training_input_.name = fname
        if ftype:
            datatypes._set_datatype(training_input_.type, ftype)

    return spec

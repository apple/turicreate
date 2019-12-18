# Copyright (c) 2019, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from ..._deps import HAS_SKLEARN
from ...models import MLModel as _MLModel
import coremltools
# from coremltools.proto import Model_pb2
from coremltools.proto import FeatureTypes_pb2

if HAS_SKLEARN:
    import sklearn.neighbors as _neighbors
    from . import _sklearn_util

import numpy as np
import scipy as sp
import six as _six

model_type = 'classifier'
sklearn_class = _neighbors.KNeighborsClassifier

def convert(model, input_name, output_name):
    """Convert a scikit KNeighborsClassifier to protobuf format.

    Parameters
    ----------
    model : KNeighborsClassifier
        A trained scikit-learn KNeighborsClassifier model.

    input_name: str
        Name of the input column.

    output_name: str
        Name of the output column.

    Returns
    -------
    model_spec: An object of type Model_pb.
        Protobuf representation of the model
    """
    if not (HAS_SKLEARN):
        raise RuntimeError('scikit-learn not found. scikit-learn conversion API is disabled.')

    _sklearn_util.check_expected_type(model, sklearn_class)

    _check_fitted(model)
    _check_algorithm(model)
    _check_weighting_scheme(model)
    _check_distance_metric(model)

    return _MLModel(_convert_k_neighbors_classifier(model, input_name, output_name))

def supports_output_scores(model):
    """KNeighborsClassifier models do not support output scores."""
    return False

def get_output_classes(model):
    """Get the candidate classes for the model."""
    _check_fitted(model)
    return list(model.classes_)

def _convert_k_neighbors_classifier(model, input_name, output_name):
    """Convert the scikit KNeighborsClassifier to CoreML. Assumes initial validation of the scikit model has been done."""

    spec = coremltools.proto.Model_pb2.Model()
    spec.specificationVersion = coremltools.SPECIFICATION_VERSION

    spec.kNearestNeighborsClassifier.numberOfNeighbors.defaultValue = model.n_neighbors
    spec.kNearestNeighborsClassifier.numberOfNeighbors.range.minValue = 1
    spec.kNearestNeighborsClassifier.numberOfNeighbors.range.maxValue = _number_of_samples(model, spec) # is there a better heuristic to use here?

    number_of_dimensions = 0
    if _is_algorithm_brute(model):
        number_of_dimensions = model._fit_X.shape[1]
        spec.kNearestNeighborsClassifier.nearestNeighborsIndex.linearIndex.MergeFromString(b'')
    elif _is_algorithm_kd_tree(model):
        npdata = np.asarray(model._tree.data)
        number_of_dimensions = get_input_dimension(model)
        spec.kNearestNeighborsClassifier.nearestNeighborsIndex.singleKdTreeIndex.leafSize = model.leaf_size
    else:
        raise TypeError('KNeighbors algorithm not supported for CoreML conversion: {}'.format(model.algorithm))
    spec.kNearestNeighborsClassifier.nearestNeighborsIndex.numberOfDimensions = number_of_dimensions

    # Make sure the distance function is set
    spec.kNearestNeighborsClassifier.nearestNeighborsIndex.squaredEuclideanDistance.MergeFromString(b'')

    input_features = spec.description.input.add()
    input_features.name = input_name[0][0]
    input_features.type.multiArrayType.shape.extend([number_of_dimensions])
    input_features.type.multiArrayType.dataType = FeatureTypes_pb2.ArrayFeatureType.FLOAT32

    output_label = spec.description.output.add()
    output_label.name = output_name[0][0]

    # predictedFeatureName is required since KNN is a classifier and it should be same as outputName.
    spec.description.predictedFeatureName = output_label.name

    # Need to confirm if scikit only accepts integer labels
    output_label.type.int64Type.MergeFromString(b'')
    spec.kNearestNeighborsClassifier.uniformWeighting.MergeFromString(b'')

    _extract_training_data(model, spec)

    return spec

def _number_of_samples(model, spec):
    """Get the number of samples the model is fitted to."""

    if _is_algorithm_brute(model):
        return model._fit_X.shape[0]
    elif _is_algorithm_kd_tree(model):
        return len(np.asarray(model._tree.data))
    return 0

def _extract_training_data(model, spec):
    """Extract the training data from the scikit model and add it to the CoreML spec"""

    if _is_algorithm_brute(model):
        X = model._fit_X
        if _is_valid_sparse_format(X):
            X = _unpack_sparse(X)

        for sample in X:
            coreml_sample = spec.kNearestNeighborsClassifier.nearestNeighborsIndex.floatSamples.add()
            for feature in sample:
                coreml_sample.vector.append(feature)

    elif _is_algorithm_kd_tree(model):
        # sklearn guarantees that tree data is not stored in a sparse format
        npdata = np.asarray(model._tree.data)
        for sample in npdata:
            coreml_sample = spec.kNearestNeighborsClassifier.nearestNeighborsIndex.floatSamples.add()
            for feature in sample:
                coreml_sample.vector.append(feature)

    for label in model._y:
        spec.kNearestNeighborsClassifier.int64ClassLabels.vector.append(label)

def get_input_dimension(model):
    """Get the input dimension for the model"""
    _check_fitted(model)
    number_of_dimensions = 0
    if _is_algorithm_brute(model):
        number_of_dimensions = model._fit_X.shape[1]
    elif _is_algorithm_kd_tree(model):
        npdata = np.asarray(model._tree.data)
        number_of_dimensions = len(npdata[0])
    else:
        raise TypeError('KNeighbors algorithm not supported for CoreML conversion: {}'.format(model.algorithm))
    return number_of_dimensions

def _check_fitted(model):
    """Simple wrapper to check if the KNeighborsClassifier has been fitted."""
    return _sklearn_util.check_fitted(model, lambda m: hasattr(m, '_fit_method') or hasattr(m, '_fit_X'))

def _check_algorithm(model):
    """Ensure the kNeighbors algorithm for the given scikit model is a supported type"""
    is_valid = False
    print_name = ''
    if model.algorithm is 'brute' or model.algorithm is 'kd_tree':
        is_valid = True
        print_name = model.algorithm
    elif model.algorithm is 'auto' and model._fit_method is 'kd_tree':
        is_valid = True
        print_name = 'kd_tree'
    elif model.algorithm is 'auto' and model._fit_method is 'brute':
        is_valid = True
        print_name = 'brute'
    if not is_valid:
        raise TypeError('KNeighbors algorithm not supported for CoreML conversion: {}'.format(print_name))

def _check_weighting_scheme(model):
    """Simple wrapper to ensure the weighting scheme is valid for CoreML conversion"""
    is_valid = False
    if model.weights is 'uniform':
        is_valid = True

    # Other cases CoreML doesn't support include weighting by distance or a user-provided 'callable' object.

    if not is_valid:
        print_name = ''
        if _is_printable(model.weights):
            print_name = model.weights
        else:
            print_name = getattr(model.weights, '__name__', repr(model.weights))
        raise TypeError('KNeighbors weight function not supported for CoreML conversion: {}'.format(print_name))

def _check_distance_metric(model):
    """Simple wrapper to ensure the distance metric is valid for CoreML conversion"""
    is_valid = False
    if model.metric is 'euclidean':
        is_valid = True
    elif model.metric is 'minkowski' and model.p == 2:
        is_valid = True

    # There are a number of other distance metrics supported by scikit that CoreML doesn't currently support.

    if not is_valid:
        print_name = ''
        if _is_printable(model.metric):
            print_name = model.metric
        else:
            print_name = getattr(model.metric, '__name__', repr(model.metric))
        raise TypeError('KNeighbors distance metric not supported for CoreML conversion: {}'.format(print_name))

def _is_algorithm_brute(model):
    """Checks if the algorithm for the scikit model is set to 'brute'."""
    return model.algorithm is 'brute' or (model.algorithm is 'auto' and model._fit_method is 'brute')

def _is_algorithm_kd_tree(model):
    """Checks if the algorithm for the scikit model is set to 'kd_tree'."""
    return model.algorithm is 'kd_tree' or (model.algorithm is 'auto' and model._fit_method is 'kd_tree')

def _is_printable(obj):
    """Check if the object is a valid text type."""
    return isinstance(obj, _six.string_types)

def _is_valid_sparse_format(obj):
    """Check if the object is in CSR sparse format (the only valid type for KNeighborsClassifier)"""
    return isinstance(obj, sp.sparse.csr_matrix)

def _unpack_sparse(obj):
    """Unpack the sparse matrix into a format that we can easily iterate over for insertion into a CoreML model."""
    if not sp.sparse.issparse(obj):
        raise TypeError('Object {} is not a scipy sparse matrix type'.format(type(obj)))
    return obj.toarray()

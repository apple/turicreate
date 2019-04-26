# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from six import string_types as _string_types

from . import _libsvm_converter
from . import _libsvm_util

from ..._deps import HAS_LIBSVM as _HAS_LIBSVM

if _HAS_LIBSVM:
    import svm as _libsvm


def convert(model, input_names='input', target_name='target',
            probability='classProbability', input_length='auto'):
    """
    Convert a LIBSVM model to Core ML format.

    Parameters
    ----------

    model: a libsvm model (C-SVC, nu-SVC, epsilon-SVR, or nu-SVR)
        or string path to a saved model.

    input_names: str | [str]
        Name of the input column(s).
        If a single string is used (the default) the input will be an array. The
        length of the array will be inferred from the model, this can be overridden
        using the 'input_length' parameter.

    target: str
        Name of the output column.

    probability: str
        Name of the output class probability column.
        Only used for C-SVC and nu-SVC that have been trained with probability
        estimates enabled.

    input_length: int
        Set the length of the input array.
        This parameter should only be used when the input is an array (i.e. when
        'input_name' is a string).

    Returns
    -------
    model: MLModel
        Model in Core ML format.

    Examples
    --------
    .. sourcecode:: python

        # Make a LIBSVM model
        >>> import svmutil
        >>> problem = svmutil.svm_problem([0,0,1,1], [[0,1], [1,1], [8,9], [7,7]])
        >>> libsvm_model = svmutil.svm_train(problem, svmutil.svm_parameter())

        # Convert using default input and output names
        >>> import coremltools
        >>> coreml_model = coremltools.converters.libsvm.convert(libsvm_model)

        # Save the CoreML model to a file.
        >>> coreml_model.save('./my_model.mlmodel')

        # Convert using user specified input names
        >>> coreml_model = coremltools.converters.libsvm.convert(libsvm_model, input_names=['x', 'y'])
    """
    if not(_HAS_LIBSVM):
        raise RuntimeError('libsvm not found. libsvm conversion API is disabled.')

    if isinstance(model, _string_types):
        libsvm_model = _libsvm_util.load_model(model)
    else:
        libsvm_model = model
    if not isinstance(libsvm_model, _libsvm.svm_model):
        raise TypeError("Expected 'model' of type '%s' (got %s)" % (_libsvm.svm_model, type(libsvm_model)))

    if not isinstance(target_name, _string_types):
        raise TypeError("Expected 'target_name' of type str (got %s)" % type(libsvm_model))

    if input_length != 'auto' and not isinstance(input_length, int):
        raise TypeError("Expected 'input_length' of type int, got %s" % type(input_length))

    if input_length != 'auto' and not isinstance(input_names, _string_types):
        raise ValueError("'input_length' should not be used unless the input will be only one array.")

    if not isinstance(probability, _string_types):
        raise TypeError("Expected 'probability' of type str (got %s)" % type(probability))

    return _libsvm_converter.convert(libsvm_model, input_names, target_name, input_length, probability)

# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

"""
List of all external dependancies for this package. Imported as
optional includes
"""
from distutils.version import StrictVersion as _StrictVersion
import logging as _logging
import platform as _platform
import re as _re
import sys as _sys


def __get_version(version):
    # matching 1.6.1, and 1.6.1rc, 1.6.1.dev
    version_regex = r"^\d+\.\d+\.\d+"
    version = _re.search(version_regex, str(version)).group(0)
    return _StrictVersion(version)


# ---------------------------------------------------------------------------------------

_IS_MACOS = _sys.platform == "darwin"
_MACOS_VERSION = ()

if _IS_MACOS:
    ver_str = _platform.mac_ver()[0]
    MACOS_VERSION = tuple([int(v) for v in ver_str.split(".")])

MSG_ONLY_MACOS = "Only supported on macOS"

# ---------------------------------------------------------------------------------------
_HAS_SKLEARN = True
_SKLEARN_VERSION = None
_SKLEARN_MIN_VERSION = "0.17"
_SKLEARN_MAX_VERSION = "0.19.2"


def __get_sklearn_version(version):
    # matching 0.15b, 0.16bf, etc
    version_regex = r"^\d+\.\d+"
    version = _re.search(version_regex, str(version)).group(0)
    return _StrictVersion(version)


try:
    import sklearn

    _SKLEARN_VERSION = __get_sklearn_version(sklearn.__version__)
    if _SKLEARN_VERSION < _StrictVersion(
        _SKLEARN_MIN_VERSION
    ) or _SKLEARN_VERSION > _StrictVersion(_SKLEARN_MAX_VERSION):
        _HAS_SKLEARN = False
        _logging.warning(
            (
                "scikit-learn version %s is not supported. Minimum required version: %s. "
                "Maximum required version: %s. "
                "Disabling scikit-learn conversion API."
            )
            % (sklearn.__version__, _SKLEARN_MIN_VERSION, _SKLEARN_MAX_VERSION)
        )
except:
    _HAS_SKLEARN = False
MSG_SKLEARN_NOT_FOUND = "Sklearn not found."

# ---------------------------------------------------------------------------------------
_HAS_LIBSVM = True
try:
    from libsvm import svm
except:
    _HAS_LIBSVM = False
MSG_LIBSVM_NOT_FOUND = "Libsvm not found."

# ---------------------------------------------------------------------------------------
_HAS_XGBOOST = True
try:
    import xgboost
except:
    _HAS_XGBOOST = False

# ---------------------------------------------------------------------------------------
_HAS_TF = True
_HAS_TF_1 = False
_HAS_TF_2 = False
_TF_1_MIN_VERSION = "1.0.0"
_TF_1_MAX_VERSION = "1.15.0"
_TF_2_MIN_VERSION = "2.1.0"
_TF_2_MAX_VERSION = "2.2.0"

try:
    import tensorflow

    tf_ver = __get_version(tensorflow.__version__)

    # TensorFlow
    if tf_ver < _StrictVersion("2.0.0"):
        _HAS_TF_1 = True

    if tf_ver >= _StrictVersion("2.0.0"):
        _HAS_TF_2 = True

    if _HAS_TF_1:
        if tf_ver < _StrictVersion(_TF_1_MIN_VERSION):
            _logging.warn(
                (
                    "TensorFlow version %s is not supported. Minimum required version: %s ."
                    "TensorFlow conversion will be disabled."
                )
                % (tensorflow.__version__, _TF_1_MIN_VERSION)
            )
        elif tf_ver > _StrictVersion(_TF_1_MAX_VERSION):
            _logging.warn(
                "TensorFlow version %s detected. Last version known to be fully compatible is %s ."
                % (tensorflow.__version__, _TF_1_MAX_VERSION)
            )
    elif _HAS_TF_2:
        if tf_ver < _StrictVersion(_TF_2_MIN_VERSION):
            _logging.warn(
                (
                    "TensorFlow version %s is not supported. Minimum required version: %s ."
                    "TensorFlow conversion will be disabled."
                )
                % (tensorflow.__version__, _TF_2_MIN_VERSION)
            )
        elif tf_ver > _StrictVersion(_TF_2_MAX_VERSION):
            _logging.warn(
                "TensorFlow version %s detected. Last version known to be fully compatible is %s ."
                % (tensorflow.__version__, _TF_2_MAX_VERSION)
            )

except:
    _HAS_TF = False
    _HAS_TF_1 = False
    _HAS_TF_2 = False

MSG_TF1_NOT_FOUND = "TensorFlow 1.x not found."
MSG_TF2_NOT_FOUND = "TensorFlow 2.x not found."

# ---------------------------------------------------------------------------------------
_HAS_KERAS_TF = True
_HAS_KERAS2_TF = True
_KERAS_MIN_VERSION = "1.2.2"
_KERAS_MAX_VERSION = "2.2.4"
MSG_KERAS1_NOT_FOUND = "Keras 1 not found."
MSG_KERAS2_NOT_FOUND = "Keras 2 not found."

try:
    # Prevent keras from printing things that are not errors to standard error.
    import six
    import sys

    if six.PY2:
        import StringIO

        temp = StringIO.StringIO()
    else:
        import io

        temp = io.StringIO()
    stderr = sys.stderr
    try:
        sys.stderr = temp
        import keras
    except:
        # Print out any actual error message and re-raise.
        sys.stderr = stderr
        sys.stderr.write(temp.getvalue())
        raise
    finally:
        sys.stderr = stderr
    import tensorflow

    k_ver = __get_version(keras.__version__)

    # keras 1 version too old
    if k_ver < _StrictVersion(_KERAS_MIN_VERSION):
        _HAS_KERAS_TF = False
        _HAS_KERAS2_TF = False
        _logging.warning(
            (
                "Keras version %s is not supported. Minimum required version: %s ."
                "Keras conversion will be disabled."
            )
            % (keras.__version__, _KERAS_MIN_VERSION)
        )
    # keras version too new
    if k_ver > _StrictVersion(_KERAS_MAX_VERSION):
        _HAS_KERAS_TF = False
        _logging.warning(
            (
                "Keras version %s detected. Last version known to be fully compatible of Keras is %s ."
            )
            % (keras.__version__, _KERAS_MAX_VERSION)
        )
    # Using Keras 2 rather than 1
    if k_ver >= _StrictVersion("2.0.0"):
        _HAS_KERAS_TF = False
        _HAS_KERAS2_TF = True
    # Using Keras 1 rather than 2
    else:
        _HAS_KERAS_TF = True
        _HAS_KERAS2_TF = False
    if keras.backend.backend() != "tensorflow":
        _HAS_KERAS_TF = False
        _HAS_KERAS2_TF = False
        _logging.warning(
            (
                "Unsupported Keras backend (only TensorFlow is currently supported). "
                "Keras conversion will be disabled."
            )
        )

except:
    _HAS_KERAS_TF = False
    _HAS_KERAS2_TF = False

# ---------------------------------------------------------------------------------------
_HAS_CAFFE2 = True
try:
    import caffe2
except:
    _HAS_CAFFE2 = False

# ---------------------------------------------------------------------------------------
_HAS_TORCH = True
try:
    import torch
except:
    _HAS_TORCH = False
MSG_TORCH_NOT_FOUND = "PyTorch not found."

# ---------------------------------------------------------------------------------------
_HAS_ONNX = True
try:
    import onnx
except:
    _HAS_ONNX = False

# ---------------------------------------------------------------------------------------
_HAS_GRAPHVIZ = True
try:
    import graphviz
except:
    _HAS_GRAPHVIZ = False
MSG_ONNX_NOT_FOUND = "ONNX not found."

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
import re as _re

def __get_version(version):
    # matching 1.6.1, and 1.6.1rc, 1.6.1.dev
    version_regex = '^\d+\.\d+\.\d+'
    version = _re.search(version_regex, str(version)).group(0)
    return _StrictVersion(version)

# ---------------------------------------------------------------------------------------
HAS_SKLEARN = True
SKLEARN_VERSION = None
SKLEARN_MIN_VERSION = '0.17'
def __get_sklearn_version(version):
    # matching 0.15b, 0.16bf, etc
    version_regex = '^\d+\.\d+'
    version = _re.search(version_regex, str(version)).group(0)
    return _StrictVersion(version)

try:
    import sklearn
    SKLEARN_VERSION = __get_sklearn_version(sklearn.__version__)
    if SKLEARN_VERSION < _StrictVersion(SKLEARN_MIN_VERSION):
        HAS_SKLEARN = False
        _logging.warn(('scikit-learn version %s is not supported. Minimum required version: %s. '
                      'Disabling scikit-learn conversion API.')
                      % (sklearn.__version__, SKLEARN_MIN_VERSION) )
except:
    HAS_SKLEARN = False

# ---------------------------------------------------------------------------------------
HAS_LIBSVM = True
try:
    import svm
except:
    HAS_LIBSVM = False

# ---------------------------------------------------------------------------------------
HAS_XGBOOST = True
try:
    import xgboost
except:
    HAS_XGBOOST = False

# ---------------------------------------------------------------------------------------
HAS_KERAS_TF = True
HAS_KERAS2_TF = True
KERAS_MIN_VERSION = '1.2.2'
KERAS_MAX_VERSION = '2.2.4'
TF_MIN_VERSION = '1.0.0'
TF_MAX_VERSION = '1.12.0'

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

    tf_ver = __get_version(tensorflow.__version__)
    k_ver = __get_version(keras.__version__)

    # keras 1 version too old
    if k_ver < _StrictVersion(KERAS_MIN_VERSION):
        HAS_KERAS_TF = False
        HAS_KERAS2_TF = False
        _logging.warn(('Keras version %s is not supported. Minimum required version: %s .'
                      'Keras conversion will be disabled.')
                      % (keras.__version__, KERAS_MIN_VERSION))
    # keras version too new
    if k_ver > _StrictVersion(KERAS_MAX_VERSION):
        HAS_KERAS_TF = False
        _logging.warn(('Keras version %s detected. Last version known to be fully compatible of Keras is %s .')
                      % (keras.__version__, KERAS_MAX_VERSION))
    # Using Keras 2 rather than 1
    if k_ver >= _StrictVersion('2.0.0'):
        HAS_KERAS_TF = False
        HAS_KERAS2_TF = True
    # Using Keras 1 rather than 2
    else:
        HAS_KERAS_TF = True
        HAS_KERAS2_TF = False
    # TensorFlow too old
    if tf_ver < _StrictVersion(TF_MIN_VERSION):
        HAS_KERAS_TF = False
        HAS_KERAS2_TF = False
        _logging.warn(('TensorFlow version %s is not supported. Minimum required version: %s .'
                      'Keras conversion will be disabled.')
                      % (tensorflow.__version__, TF_MIN_VERSION))
    if tf_ver > _StrictVersion(TF_MAX_VERSION):
        _logging.warn(('TensorFlow version %s detected. Last version known to be fully compatible is %s .')
                      % (tensorflow.__version__, TF_MAX_VERSION))
    if keras.backend.backend() != 'tensorflow':
        HAS_KERAS_TF = False
        HAS_KERAS2_TF = False
        _logging.warn(('Unsupported Keras backend (only Tensorflow is currently supported). '
                      'Keras conversion will be disabled.'))

except:
    HAS_KERAS_TF = False
    HAS_KERAS2_TF = False

# ---------------------------------------------------------------------------------------
HAS_CAFFE2 = True
try:
    import caffe2
except:
    HAS_CAFFE2 = False
# ---------------------------------------------------------------------------------------
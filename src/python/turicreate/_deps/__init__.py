# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from distutils.version import StrictVersion as _StrictVersion
import logging as _logging
import re as _re


def __get_version(version):
    # matching 1.6.1, and 1.6.1rc, 1.6.1.dev
    version_regex = '^\d+\.\d+\.\d+'
    version = _re.search(version_regex, str(version)).group(0)
    return _StrictVersion(version)


HAS_PANDAS = True
PANDAS_MIN_VERSION = '0.13.0'
try:
    import pandas
    if __get_version(pandas.__version__) < _StrictVersion(PANDAS_MIN_VERSION):
        HAS_PANDAS = False
        _logging.warn(('Pandas version %s is not supported. Minimum required version: %s. '
                      'Pandas support will be disabled.')
                      % (pandas.__version__, PANDAS_MIN_VERSION) )
except:
    HAS_PANDAS = False
    from . import pandas_mock as pandas


HAS_NUMPY = True
NUMPY_MIN_VERSION = '1.8.0'
try:
    import numpy

    if __get_version(numpy.__version__) < _StrictVersion(NUMPY_MIN_VERSION):
        HAS_NUMPY = False
        _logging.warn(('Numpy version %s is not supported. Minimum required version: %s. '
                      'Numpy support will be disabled.')
                      % (numpy.__version__, NUMPY_MIN_VERSION) )
except:
    HAS_NUMPY = False
    from . import numpy_mock as numpy


HAS_SKLEARN = True
SKLEARN_MIN_VERSION = '0.15'
def __get_sklearn_version(version):
    # matching 0.15b, 0.16bf, etc
    version_regex = '^\d+\.\d+'
    version = _re.search(version_regex, str(version)).group(0)
    return _StrictVersion(version)

try:
    import sklearn
    if __get_sklearn_version(sklearn.__version__) < _StrictVersion(SKLEARN_MIN_VERSION):
        HAS_SKLEARN = False
        _logging.warn(('sklearn version %s is not supported. Minimum required version: %s. '
                      'sklearn support will be disabled.')
                      % (sklearn.__version__, SKLEARN_MIN_VERSION) )
except:
    HAS_SKLEARN = False
    from . import sklearn_mock as sklearn



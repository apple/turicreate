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
import sys
import six

if six.PY2:
    import imp
else:
    import importlib

from types import ModuleType

def default_init_func(name):
    if six.PY2:
        # if module is not found, it will raise ImportError
        fp, pathname, description = imp.find_module(name)
        try:
            return imp.load_module(name, fp, pathname, description)
        finally:
            # Since we may exit via an exception, close fp explicitly.
            if fp:
                fp.close()
    else:
        return importlib.import_module(name)

class LazyModuleLoader(ModuleType):
    def __init__(self, name, init_func = default_init_func):
        self.loaded_ = False
        self.name_ = name
        self.module_ = None
        self.init_func_ = init_func
        if init_func is None:
            raise ValueError("init function should not be None")
        # never do this
        # sys.modules[self.name_] = self which will override mod with None

    def __getattr__(self, attr):
        # call module.__init__ after import introspection is done
        if not self.loaded_:
            self.loaded_ = True
            self.module_ = self.init_func_(self.name_)
        return getattr(self.module_, attr)

    def __str__(self):
        return "lazy loading of module %s" % self.name_

    def __repr__(self):
        return "lazy loading of module %s" % self.name_

def __get_version(version):
    # matching 1.6.1, and 1.6.1rc, 1.6.1.dev
    version_regex = '^\d+\.\d+\.\d+'
    version = _re.search(version_regex, str(version)).group(0)
    return _StrictVersion(version)

def __has_module(name):
    if six.PY2:
        return imp.find_module(name) is not None
    else:
        return importlib.util.find_spec(name) is not None

HAS_PANDAS = __has_module('pandas')
PANDAS_MIN_VERSION = '0.13.0'

def _dynamic_load_pandas(name):
    global HAS_PANDAS
    if HAS_PANDAS:
        import pandas
        if __get_version(pandas.__version__) < _StrictVersion(PANDAS_MIN_VERSION):
            HAS_PANDAS = False
            _logging.warn(('Pandas version %s is not supported. Minimum required version: %s. '
                        'Pandas support will be disabled.')
                        % (pandas.__version__, PANDAS_MIN_VERSION) )
    if not HAS_PANDAS:
        from . import pandas_mock as pandas

    return pandas

pandas = LazyModuleLoader('pandas', _dynamic_load_pandas)

HAS_NUMPY = __has_module('numpy')
NUMPY_MIN_VERSION = '1.8.0'

def _dynamic_load_numpy(name):
    global HAS_NUMPY
    if HAS_NUMPY:
        import numpy as _ret
        if __get_version(_ret.__version__) < _StrictVersion(NUMPY_MIN_VERSION):
            HAS_NUMPY = False
            _logging.warn(('Numpy version %s is not supported. Minimum required version: %s. '
                        'Numpy support will be disabled.')
                        % (numpy.__version__, NUMPY_MIN_VERSION) )
    if not HAS_NUMPY:
        from . import numpy_mock as _ret
    return _ret

numpy = LazyModuleLoader('numpy', _dynamic_load_numpy)

HAS_SKLEARN = __has_module('sklearn')
SKLEARN_MIN_VERSION = '0.15'

def __get_sklearn_version(version):
    # matching 0.15b, 0.16bf, etc
    version_regex = '^\d+\.\d+'
    version = _re.search(version_regex, str(version)).group(0)
    return _StrictVersion(version)

def _dynamic_load_sklean(name):
    global HAS_SKLEARN
    if HAS_SKLEARN:
        import sklearn
        if __get_sklearn_version(sklearn.__version__) < _StrictVersion(SKLEARN_MIN_VERSION):
            HAS_SKLEARN = False
            _logging.warn(('sklearn version %s is not supported. Minimum required version: %s. '
                        'sklearn support will be disabled.')
                        % (sklearn.__version__, SKLEARN_MIN_VERSION) )
    if not HAS_SKLEARN:
        from . import sklearn_mock as sklearn
    return sklearn

sklearn = LazyModuleLoader("sklearn", _dynamic_load_sklean)

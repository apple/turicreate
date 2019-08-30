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
from six.moves import reload_module
from types import ModuleType

if six.PY2:
    import imp
else:
    import importlib

def run_from_ipython():
    # Taken from https://stackoverflow.com/questions/5376837
    try:
        __IPYTHON__
        return True
    except NameError:
        return False

## builtin __import__(path, globals, locals, fromlist)
# referenced from https://github.com/mnmelo/lazy_import/blob/master/lazy_import/__init__.py
if six.PY2:
    import imp
    class _ImportLockContext:
        def __enter__(self):
            imp.acquire_lock()
        def __exit__(self, exc_type, exc_value, exc_traceback):
            imp.release_lock()
else:
    from importlib._bootstrap import _ImportLockContext

def default_init_func(name, *args):
    """
    called within _ImportLockContext(); no import lock should be hold inside
    """
    _ret = sys.modules.get(name, None)

    # already imported by others
    if _ret is not None:
        return _ret

    if isinstance(_ret, LazyModuleLoader) and _ret.is_loaded(no_lock=True):
        return _ret

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
    """ defer to load a module. If it's loaded, no exception but warning will be given. """
    members_ = ['init_func_', 'loaded_', 'module_', 'members_', 'name_']
    def __init__(self, name, init_func = default_init_func):
        """
        Once write then read only. Reload is not supported.

        LazyModuleLoader won't touch sys.modules[name] because:
            1.

        this class only supports syntax `import a.b [as c]`;
        you need to translate `from a import b as c` to the form described above.

        if sys.modules[name] is LazyMuduleLoader
        """
        if init_func is None:
            raise ValueError("init function should not be None")
        if not isinstance(name, six.string_types):
            raise ValueError("name must be str type")
        if len(name) == 0:
            raise ValueError("module name should not be empty")
        if name.startswith('.'):
            raise ValueError("only support absolute path import style")
        # this will call setattr
        self.name_ = name
        self.init_func_ = init_func

        with _ImportLockContext():
            self.module_ = sys.modules.get(name, None)
            self.loaded_ = self.module_ is not None
            # print(name, self.module_, self.loaded_)
            if self.loaded_ and not isinstance(self.module_, LazyModuleLoader):
                _logging.debug("turicreate: {} is loaded and it cannot "
                              " be lazily loaded. lazy load strategy fails by {}".format(name, self.__name__))

            # never do this since it's hard to maintain this singleton during reloading
            # you have to del sys.modules[name] first
            # else:
            #     sys.modules[name] = self


    def __getattr__(self, attr):
        self._load_module()
        return getattr(self.module_, attr)

    def __str__(self):
        if self.module_:
            return self.module_.__str__()
        return "lazy loading of module %s" % self.name_

    def __repr__(self):
        if self.module_:
            return self.module_.__repr__()
        return "lazy loading of module %s" % self.name_

    def __dir__(self):
        # for interactive autocompletion
        if self.module_:
            return dir(self.module_)
        return self.members_

    def __setattr__(self, attr, value):
        # whitelist for member variables
        # _logging.warn(attr + " to set with " + str(value))
        if attr in self.members_:
            # workaround for the recursive setattr
            return super(LazyModuleLoader, self).__setattr__(attr, value)
        else:
            self._load_module()
            setattr(self.module_, attr, value)

    def is_loaded(self, no_lock=True):
        return isinstance(self, LazyModuleLoader)
        if no_lock:
            return self.loaded_
        else:
            with _ImportLockContext():
                return self.loaded_

    # should this be locked ???
    def _load_module(self):
        # call module.__init__ after import introspection is done
        if not self.loaded_:
            with _ImportLockContext():
                # avoid concurrent loading
                if not self.loaded_:
                    # if it's imported by other pkg after __init__,
                    # we don't bother to inject ourselves because
                    # there's no need to lazy load if it's loaded explicitly.
                    self.module_ = sys.modules.get(self.name_, None)
                    if self.module_ is None:
                        # release our hostage, commit our crime
                        self.module_ = self.init_func_(self.name_)
                        sys.modules[self.name_] = self.module_
                        self = self.module_


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

# called within import lock; don't lock inside
def _dynamic_load_pandas(name):
    global HAS_PANDAS
    if HAS_PANDAS:
        _ret = sys.modules.get(name, None)
        if _ret is None:
            import pandas as _ret
        if __get_version(_ret.__version__) < _StrictVersion(PANDAS_MIN_VERSION):
            HAS_PANDAS = False
            _logging.warn(('Pandas version %s is not supported. Minimum required version: %s. '
                           'Pandas support will be disabled.')
                          % (pandas.__version__, PANDAS_MIN_VERSION))
    if not HAS_PANDAS:
        from . import pandas_mock as _ret
    return _ret

pandas = LazyModuleLoader('pandas', _dynamic_load_pandas)

HAS_NUMPY = __has_module('numpy')
NUMPY_MIN_VERSION = '1.8.0'

def _dynamic_load_numpy(name):
    global HAS_NUMPY
    if HAS_NUMPY:
        _ret = sys.modules.get(name, None)
        if _ret is None:
            import numpy as _ret
        if __get_version(_ret.__version__) < _StrictVersion(NUMPY_MIN_VERSION):
            HAS_NUMPY = False
            _logging.warn(('Numpy version %s is not supported. Minimum required version: %s. '
                           'Numpy support will be disabled.')
                          % (numpy.__version__, NUMPY_MIN_VERSION))
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
        _ret = sys.modules.get(name, None)
        if _ret is None:
            import sklearn as _ret
        if __get_sklearn_version(_ret.__version__) < _StrictVersion(SKLEARN_MIN_VERSION):
            HAS_SKLEARN = False
            _logging.warn(('sklearn version %s is not supported. Minimum required version: %s. '
                           'sklearn support will be disabled.')
                          % (sklearn.__version__, SKLEARN_MIN_VERSION))
    if not HAS_SKLEARN:
        from . import sklearn_mock as _ret
    return _ret

sklearn = LazyModuleLoader("sklearn", _dynamic_load_sklean)

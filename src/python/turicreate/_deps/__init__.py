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
    # reentrant lock
    # https://docs.python.org/2/library/imp.html#imp.acquire_lock
    class _ImportLockContext:
        def __enter__(self):
            imp.acquire_lock()
        def __exit__(self, exc_type, exc_value, exc_traceback):
            imp.release_lock()
else:
    # importlib uses _imp internally; check source you will know ;-)
    from importlib._bootstrap import _ImportLockContext

# should always be guared by _ImportLockContext
def default_init_func(name, *args):
    """
    called within _ImportLockContext(); no import lock should be hold inside
    """
    _ret = sys.modules.get(name, None)
    if _ret is not None:
        assert isinstance(_ret, LazyModuleLoader)
        sys.modules.pop(name)
        # _ret -> self with ref cnt >= 1

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
    # read-only
    members_ = ['init_func_', 'loaded_', 'module_', 'members_', 'name_']
    # ensure singleton; must be guarded by _ImportLockContext lock
    registry_ = set()
    def __init__(self, name, init_func = default_init_func):
        """
        Once write then read only. `reload` can reset the state.
        Only singleton instance is allowed.
        Context _ImportLockContext is reentrant.

        This Class only supports syntax `import a.b [as c]`;
        you need to translate `from a import b as c` to the form described above.

        LazyModuleLoader hijacks sys.modules[name] to defer the
        use of the module if the module is not already loaded yet.

        The load of module is deferred until LazyModuleLoader is called
        by __getattr__ on `attr` other than `LazyModuleLoader.members_`.
        The side effect is that sys.modules[name] will be set to real module object.

        >>> np = LazyModuleLoader('numpy')
        >>> import numpy # won't actually load
        >>> np.ndarray # load the numpy module

        If the module is loaded before LazyModuleLoader hijack,
        then LzyModuleLoader won't do anything but just a thin wrapper
        to the real module object.

        >>> import numpy # load the numpy
        >>> np = LazyModuleLoader('numpy') # no effect
        >>> np.ndarray # works as the same as numpy.ndarray

        This example is very nasty since it will actualy delete
        the LazyModuleLoader object due to garbage collection:

        >>> def baz():
        ...     LazyModuleLoader('foo').bar
        >>> baz()

        Above code will load the module `foo`, which is same effcet
        as `import foo` but no variable can refer it. To reuse the module,

        >>> import foo

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
            if name in LazyModuleLoader.registry_:
                raise RuntimeError("pkg {} is already taken by other LazyModuleLoader instance.".format(name))
            LazyModuleLoader.registry_.add(name)
            self.module_ = sys.modules.get(name, None)
            self.loaded_ = self.module_ is not None

            if self.loaded_:
                if not isinstance(self.module_, LazyModuleLoader):
                    _logging.debug("turicreate: {} is loaded ahead and it cannot be"
                                   " be hijacked by LazyModuleLoader".format(name))
                else:
                    err_msg = "LazyModuleLoader '%s' is not a singleton T_T" % name
                    _logging.fatal(err_msg)
                    raise RuntimeError(err_msg)
            else:
                # consequently, `import numpy` will be hijacked;
                # numpy import will be deferred
                # hijact state: self.module_ is None; self.loaded_ is False
                sys.modules[name] = self

    def __del__(self):
        with _ImportLockContext():
            _logging.debug("turicreate LazyModuleLoader %s is deleted;"
                           "force load if not load" % self.name_)
            try:
                LazyModuleLoader.registry_.remove(self.name_)
                if not self.loaded_:
                    # since at least one ref cnt should be from
                    # sys.modules[self.name_]
                    # the only situation is triggered is that
                    # variable expired and self is poped from sys.modules:
                    #
                    # LazyModuleLoader('foo') -> expired
                    # del sys.modules[self.name_] -> cause one
                    # uninitialized LzyModuleLoader to be deleted.
                    #
                    # def baz():
                    #   bar = LazyModuleLoader('foo')
                    #
                    # baz() -> LazyModuleLoader is only referred by sys.modules
                    # sys.modules.pop('foo') -> cuz last LazyModuleLoader to be deleted
                    #
                    # def baz():
                    #   bar = LazyModuleLoader('foo')
                    #   sys.modules.pop('foo')
                    #   import foo
                    #
                    # assert sys.modules.get(self.name_) is None -> won't be true
                    # so we should make sure
                    # it cannot be None
                    print(sys.modules)
                    try:
                        assert sys.modules[self.name_] is not None, ("if sys.modules[{name}] exists, it should not be None. "
                                                                    "Only under one situation, it can be both existent and None: "
                                                                    "user explicitly uses 'sys.modules[{name}] = None'").format(name=self.name_)
                    except Exception as e:
                        print(e, type(e))
            except KeyError:
                pass

    def __getattr__(self, attr):
        self._load_module()
        return getattr(self.module_, attr)

    def __str__(self):
        if self.module_:
            return "triggered LazyModuleLoader: {}".format(self.module_.__repr__())
        return "lazy loading of module %s" % self.name_

    def __repr__(self):
        if self.module_:
            return "triggered LazyModuleLoader: {}".format(self.module_.__repr__())
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
        if no_lock:
            return self.loaded_
        else:
            with _ImportLockContext():
                return self.loaded_

    def reload(self):
        # for dev purpose
        with _ImportLockContext():
            self.module_ = None
            self.loaded_ = False
            sys.modules[self.name_] = self

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
                    # e.g.
                    # import numpy as np
                    # np = LazyModuleLoader('numpy')
                    self.module_ = sys.modules.get(self.name_)
                    if self.module_ is self:
                        # release our hostage, commit our crime
                        # may cause __del__ on self
                        sys.modules.pop(self.name_)
                        self.module_ = self.init_func_(self.name_)
                        # subsequent imports will use the real
                        # module object since we are arrested
                        sys.modules[self.name_] = self.module_
                    self.loaded_ = True



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

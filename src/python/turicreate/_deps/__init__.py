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


if six.PY2:
    import imp

    # reentrant lock: https://docs.python.org/2/library/imp.html#imp.acquire_lock
    class _ImportLockContext:
        def __enter__(self):
            imp.acquire_lock()

        def __exit__(self, exc_type, exc_value, exc_traceback):
            imp.release_lock()


else:
    # importlib uses _imp internally; check source you will know ;-)
    from importlib._bootstrap import _ImportLockContext

# should always be guared by _ImportLockContext
def default_init_func(name, *args, **kwargs):
    """
    called within _ImportLockContext(); no import lock should be hold inside
    """
    _ret = sys.modules.get(name, None)
    if _ret is not None:
        assert name not in sys.modules.keys(), (
            "sys.modules[%s] cannot be None" " during module loading" % name
        )
    if six.PY2:
        # imp importing submodule is too tricky
        # moulde and pkg naming should follow pep8
        if not all([x.isalnum() or x in "._" for x in name]):
            raise ValueError("invalid module name")
        exec("import %s as _py2_ret" % name)
        # import a.b.c will register name as a.b.c
        return sys.modules[name]
    else:
        return importlib.import_module(name)


class DeferredModuleLoader(ModuleType, object):
    """ defer to load a module. If it's loaded, no exception but warning will be given. """

    # read-only member list -> used by __setattr__; see below
    _my_attrs_ = [
        "is_loaded",
        "is_model",
        "reload",
        "get_module",
        "_name",
        "_init_func",
        "_module",
        "_loaded",
        "_is_model",
    ]
    # ensure singleton; must be guarded by _ImportLockContext lock
    registry_ = set()

    def __init__(self, name, init_func=default_init_func, singleton=True, **kwargs):
        """
        Parameters
        ----------

        init_func: Callable
        singleton: bool
            Nothing special. Just to improve code quality. When used in pickle,
            set this to False

        Once write then read only. `reload` can reset the state.
        Only singleton instance is allowed.
        Context _ImportLockContext is reentrant.

        This Class only supports syntax `import a.b [as c]`;
        you need to translate `from a import b as c` to the form described above.

        DeferredModuleLoader hijacks sys.modules[name] to defer the
        use of the module if the module is not already loaded yet.

        The load of module is deferred until DeferredModuleLoader is called
        by __getattr__, __setattr__, or __delattr__ on `attr` other than
        `dir(DeferredModuleLoader())` The side effect is that sys.modules[name]
        will be set to real module object.

        >>> np = DeferredModuleLoader('numpy')
        >>> import numpy # won't actually load
        >>> np.ndarray # load the numpy module

        If the module is loaded before DeferredModuleLoader loads the underlying module,
        then LzyModuleLoader won't do anything but just a thin wrapper to forward
        any requests to the real module object.

        >>> import numpy # load the numpy
        >>> np = DeferredModuleLoader('numpy') # no effect
        >>> np.ndarray # works as the same as numpy.ndarray

        This example is very nasty since it will actualy delete
        the DeferredModuleLoader object due to garbage collection, with the actual
        module is included into sys.modules. The `module name` will be removed
        from a singleton dictionary of DeferredModuleLoader.

        >>> def baz():
        ...     DeferredModuleLoader('foo').bar
        >>> baz() # the actual module `foo` is loaded

        Caveat:

        The design purpose is to enable user to use this DeferredModuleLoader
        object as a regular module object. There's one subtlty that if you want
        to force loading module (or call member functions) between DeferredModuleLoader
        and module object. You need to first make sure the type of the module-like object
        that is referred by is still a DeferredModuleLoader type.

        For module references, python will do some tricky stuff to find the
        real module object. Here's an example,

        >>> import turicreate as tc
        >>> print(type(tc.recommender.factorization_recommender))
        >>> <class 'turicreate._deps.DeferredModuleLoader'>
        >>> tc.recommender.factorization_recommender.get_module()
        >>> print(type(tc.recommender.factorization_recommender))
        >>> <class 'module'>

        After first force loading, `tc.recommender.factorization_recommender`
        refers to the real module object, and it won't refer to the
        DeferredModuleLoader being injected anymore.
        """

        if init_func is None:
            raise ValueError("init function should not be None")
        if not isinstance(name, six.string_types):
            raise ValueError("name must be str type")
        if len(name) == 0:
            raise ValueError("module name should not be empty")
        if name.startswith("."):
            raise ValueError("only support absolute path import style")
        # must set first to avoid recursion
        self._name = name
        self._init_func = init_func

        try:
            self._is_model = kwargs["is_model"]
        except KeyError:
            self._is_model = True

        with _ImportLockContext():
            if name in DeferredModuleLoader.registry_ and singleton:
                raise RuntimeError(
                    "pkg {} is already taken by other DeferredModuleLoader instance.".format(
                        name
                    )
                )

            DeferredModuleLoader.registry_.add(name)
            self._module = sys.modules.get(name, None)
            self._loaded = self._module is not None

            if self._loaded:
                if not isinstance(self._module, DeferredModuleLoader):
                    _logging.debug(
                        "turicreate: {} is loaded ahead and it cannot be"
                        " be deffered by DeferredModuleLoader".format(name)
                    )
                else:
                    err_msg = "DeferredModuleLoader '%s' is not a singleton T_T" % name
                    _logging.fatal(err_msg)
                    raise RuntimeError(err_msg)

    def __del__(self):
        try:
            with _ImportLockContext():
                try:
                    DeferredModuleLoader.registry_.remove(self._name)
                except Exception as e:
                    _logging.fatal(
                        "error{}: {} removes {}".format(
                            e, DeferredModuleLoader.registry_, self._name
                        )
                    )
        except Exception as e:
            # TypeError: 'NoneType' object is not callable
            # happens when python exits and modules are all destructed
            # even _logging is set to None
            if "NoneType" in str(e):
                pass
            else:
                raise e

    def __str__(self):
        if self._module:
            return "triggered DeferredModuleLoader: {}".format(self._module.__repr__())
        return "lazy loading of module %s" % self._name

    def __repr__(self):
        if self._module:
            return "triggered DeferredModuleLoader: {}".format(self._module.__repr__())
        return "lazy loading of module %s" % self._name

    def __dir__(self):
        # for interactive autocompletion
        if self._module:
            return dir(self._module)
        return self._my_attrs_ + super(DeferredModuleLoader, self).__dir__()

    def __getattr__(self, attr):
        self._load_module()
        return getattr(self._module, attr)

    def __delattr__(self, attr):
        self._load_module()
        return delattr(self._module, attr)

    def __setattr__(self, attr, value):
        """
        must filter attr by self._my_attrs_ to register members for self
        instead of the real module object;
        """
        # don't use is_loaded; it will trigger getattr
        # avoid infinite recursion by self._load_module()
        # when self._load is not set first
        if attr in self._my_attrs_:
            # workaround for the recursive setattr
            return super(DeferredModuleLoader, self).__setattr__(attr, value)
        else:
            self._load_module()
            setattr(self._module, attr, value)

    def is_loaded(self, no_lock=True):
        if no_lock:
            return self._loaded
        else:
            with _ImportLockContext():
                return self._loaded

    def is_model(self):
        return self._is_model

    def reload(self):
        # for dev purpose
        with _ImportLockContext():
            self._module = None
            self._loaded = False
            sys.modules.pop(self._name, None)

    # for python `pickle` (serialization) purpose
    # in order to please lambda worker
    def get_module(self):
        with _ImportLockContext():
            self._load_module()
            return self._module

    def _load_module(self):
        # call module.__init__ after import introspection is done
        if not self._loaded:
            with _ImportLockContext():
                # avoid concurrent loading
                if not self._loaded:
                    # if it's imported by other pkg after __init__,
                    # we don't bother to inject ourselves because
                    # there's no need to lazy load if it's loaded explicitly.
                    # e.g.
                    # import numpy as np
                    # np = DeferredModuleLoader('numpy')
                    self._module = sys.modules.get(self._name, None)
                    if self._module is None:
                        # make sure we clean it up
                        sys.modules.pop(self._name, None)
                        self._module = self._init_func(self._name)
                        # subsequent imports will use the real object
                        # import pandas from outside turicreate won't know
                        # nothing inside
                        sys.modules[self._name] = self._module
                    self._loaded = True

    # pickle
    def __reduce__(self):
        def _init(*args, **kwargs):
            return DeferredModuleLoader(*args, **kwargs)

        # be aware, singleton must be set false
        return (_init, (self._name, self._init_func, False))


class DeferredCallableLoader(object):
    def __init__(self, lmod, func_name):
        if not isinstance(lmod, ModuleType):
            raise TypeError("lmod must be a ModuleType")
        self._lmod = lmod
        if not isinstance(func_name, six.string_types):
            raise TypeError("func_name must be a string")
        self._func_name = func_name

    def __call__(self, *args, **kwargs):
        return getattr(self._lmod, self._func_name)(*args, **kwargs)

    # for pickle purpose
    # I like korean pickle too; so let's make some pickle
    def get_callable(self, *args, **kwargs):
        return getattr(self._lmod, self._func_name)(*args, **kwargs)

    def __str__(self):
        return "DeferredCallableLoader of function '%s'" % self._func_name

    def __repr__(self):
        return repr(getattr(self._lmod, self._func_name))

    def __reduce__(self):
        _init = lambda x: x
        return (_init, getattr(self._lmod, self._func_name))


def __get_version(version):
    # matching 1.6.1, and 1.6.1rc, 1.6.1.dev
    version_regex = "^\d+\.\d+\.\d+"
    version = _re.search(version_regex, str(version)).group(0)
    return _StrictVersion(version)


def __has_module(name):
    if six.PY2:
        return imp.find_module(name) is not None
    else:
        return importlib.util.find_spec(name) is not None


HAS_PANDAS = __has_module("pandas")
PANDAS_MIN_VERSION = "0.13.0"

# called within import lock; don't lock inside
def _dynamic_load_pandas(name):
    global HAS_PANDAS
    if HAS_PANDAS:
        _ret = sys.modules.get(name, None)
        if _ret is None:
            assert name not in sys.modules.keys(), (
                "sys.modules[%s] cannot be None" " during moudle loading" % name
            )
            import pandas as _ret
        if __get_version(_ret.__version__) < _StrictVersion(PANDAS_MIN_VERSION):
            HAS_PANDAS = False
            _logging.warn(
                (
                    "Pandas version %s is not supported. Minimum required version: %s. "
                    "Pandas support will be disabled."
                )
                % (pandas.__version__, PANDAS_MIN_VERSION)
            )
    if not HAS_PANDAS:
        from . import pandas_mock as _ret
    return _ret


# hijacks `import pandas`
# if this pandas is used first, all subsequent `import pandas`
# will be DeferredModuleLoader objects instead of real pandas module object.
pandas = DeferredModuleLoader("pandas", _dynamic_load_pandas)

HAS_NUMPY = __has_module("numpy")
NUMPY_MIN_VERSION = "1.8.0"


def _dynamic_load_numpy(name):
    global HAS_NUMPY
    if HAS_NUMPY:
        _ret = sys.modules.get(name, None)
        if _ret is None:
            assert name not in sys.modules.keys(), (
                "sys.modules[%s] cannot be None" " during module loading" % name
            )
            import numpy as _ret
        if __get_version(_ret.__version__) < _StrictVersion(NUMPY_MIN_VERSION):
            HAS_NUMPY = False
            _logging.warn(
                (
                    "Numpy version %s is not supported. Minimum required version: %s. "
                    "Numpy support will be disabled."
                )
                % (numpy.__version__, NUMPY_MIN_VERSION)
            )
    if not HAS_NUMPY:
        from . import numpy_mock as _ret
    return _ret


numpy = DeferredModuleLoader("numpy", _dynamic_load_numpy)

HAS_SKLEARN = __has_module("sklearn")
SKLEARN_MIN_VERSION = "0.15"


def __get_sklearn_version(version):
    # matching 0.15b, 0.16bf, etc
    version_regex = "^\d+\.\d+"
    version = _re.search(version_regex, str(version)).group(0)
    return _StrictVersion(version)


def _dynamic_load_sklean(name):
    global HAS_SKLEARN
    if HAS_SKLEARN:
        _ret = sys.modules.get(name, None)
        if _ret is None:
            assert name not in sys.modules.keys(), (
                "sys.modules[%s] cannot be None" " during module loading" % name
            )
            import sklearn as _ret
        if __get_sklearn_version(_ret.__version__) < _StrictVersion(
            SKLEARN_MIN_VERSION
        ):
            HAS_SKLEARN = False
            _logging.warn(
                (
                    "sklearn version %s is not supported. Minimum required version: %s. "
                    "sklearn support will be disabled."
                )
                % (sklearn.__version__, SKLEARN_MIN_VERSION)
            )
    if not HAS_SKLEARN:
        from . import sklearn_mock as _ret
    return _ret


sklearn = DeferredModuleLoader("sklearn", _dynamic_load_sklean)

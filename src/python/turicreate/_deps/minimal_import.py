# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _  # noqa
from __future__ import division as _  # noqa
from __future__ import absolute_import as _  # noqa
import logging as _logging
import functools
import sys
import six
from turicreate import __version__
from types import ModuleType

USE_MINIAL = False


def is_minimal_pkg():
    return USE_MINIAL


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


def run_from_python_interpreter():
    return bool(getattr(sys, "ps1", sys.flags.interactive))


IS_INTERACTIVE = run_from_python_interpreter() or run_from_ipython()


if six.PY2:
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
    try:
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
    except ImportError as e:
        if USE_MINIAL:
            _name = name
            try:
                _name = kwargs["prompt_name"]
            except KeyError:
                pass

            e.msg = (
                "{}. This is a minimal package for SFrame only, without {} pinned"
                " as a dependency. You can try install all required packages by installing"
                "full package by:\n"
                "pip install turicreate=={}"
            ).format(e.msg, _name, __version__)

            try:
                extra_msg = kwargs["extra_instruction"]
                if extra_msg is not None:
                    e.msg += " " + extra_msg
            except KeyError:
                pass

        raise e


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

        try:
            self._is_model = kwargs["is_model"]
            del kwargs["is_model"]
        except KeyError:
            self._is_model = True

        self._init_func = functools.partial(init_func, **kwargs)

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

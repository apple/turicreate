# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
This module hosts all the extension functions and classes created via SDK.

The function :py:func:`ext_import` is used to import a toolkit module (shared library)
into the workspace. The shared library can be directly imported
from a remote source, e.g. http, s3, or hdfs.
The imported module will be under namespace `.extensions`.

Alternatively, if the shared library is local, it can be directly imported
using the python import statement. Note that turicreate must be imported first.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

# This is a fake meta namespace which contains toolkit functions and toolkit
# models implemented as extensions in C++

import sys as _sys
from . import SArray as _SArray, SFrame as _SFrame, SGraph as _SGraph
from .connect.main import get_unity as _get_unity
from .util import _make_internal_url
from .cython.cy_sframe import UnitySFrameProxy as _UnitySFrameProxy
from .cython.cy_sarray import UnitySArrayProxy as _UnitySArrayProxy
from .cython.cy_graph import UnityGraphProxy as _UnityGraphProxy
from .cython.cy_model import UnityModel as _UnityModel
from .toolkits._main import ToolkitError as _ToolkitError
from .cython.context import debug_trace as cython_context
from sys import version_info as _version_info
import types as _types
if _sys.version_info.major == 2:
    from types import ClassType as _ClassType
    _class_type = _ClassType
else:
    _class_type = type


# Now. a bit of magic hackery is going to happen to this module.
# This module is going to be first imported as sframe.extensions
# After which, inside turicreate/__init__.py, sys.modules['sframe.extensions']
# will be modified to become a class called _extension_wrapper which redirects
# getattr calls into this module.
#
# The reason for this wrapping is so that uses of functions in tc.extensions
# (for instance)
#
#     import turicreate as tc
#     tc.extensions._demo_addone(5)
#
# This will normally not work because tc.extensions._publish() was not called
# hence _demo_addone will not be found.
#
# By wrapping the extensions module in another class, we can redefine
# __getattr__ on that class and have it force tc.extensions._publish() when
# an attribute name is not found.
#
# However, there are some odd sideeffects due to the use of the metapath
# system as well. the metapath importer (this module) is going to look in
# tc.extensions, but tc.extensions is going poke this module hence resulting
# in an interesting recursive relationship.
#
# Also, we need tc.extensions.__dict__ to have all the published information
# so that tab completion in ipython works.
#
# The result is that we need tc.extensions._publish() to publish into both
# places.
#  - the current module
#  - the tc.extensions wrapper
#
# Then the metapath importer (this module) will just need to look in this
# module, breaking the recursive relation. And the tc.extensions wrapper will
# have all the stuff in it for tab completion by IPython.

import sys as _sys
_thismodule = _sys.modules[__name__]
class_uid_to_class = {}

def _wrap_function_return(val):
    """
    Recursively walks each thing in val, opening lists and dictionaries,
    converting all occurrences of UnityGraphProxy to an SGraph,
    UnitySFrameProxy to SFrame, and UnitySArrayProxy to SArray.
    """

    if type(val) is _UnityGraphProxy:
        return _SGraph(_proxy = val)
    elif type(val) is _UnitySFrameProxy:
        return _SFrame(_proxy = val)
    elif type(val) is _UnitySArrayProxy:
        return _SArray(_proxy = val)
    elif type(val) is _UnityModel:
        # we need to cast it up to the appropriate type
        try:
            if '__uid__' in val.list_fields():
                uid = val.get('__uid__')
                if uid in class_uid_to_class:
                    return class_uid_to_class[uid](_proxy=val)
        except:
            pass
        return val
    elif type(val) is list:
        return [_wrap_function_return(i) for i in val]
    elif type(val) is dict:
        return dict( (i, _wrap_function_return(val[i])) for i in val)
    else:
        return val

def _setattr_wrapper(mod, key, value):
    """
    A setattr wrapper call used only by _publish(). This ensures that anything
    published into this module is also published into tc.extensions
    """
    setattr(mod, key, value)
    if mod == _thismodule:
        setattr(_sys.modules[__name__], key, value)

def _run_toolkit_function(fnname, arguments, args, kwargs):
    """
    Dispatches arguments to a toolkit function.

    Parameters
    ----------
    fnname : string
        The toolkit function to run

    arguments : list[string]
        The list of all the arguments the function takes.

    args : list
        The arguments that were passed

    kwargs : dictionary
        The keyword arguments that were passed
    """
    # scan for all the arguments in args
    num_args_got = len(args) + len(kwargs)
    num_args_required = len(arguments)
    if num_args_got != num_args_required:
        raise TypeError("Expecting " + str(num_args_required) + " arguments, got " + str(num_args_got))

    ## fill the dict first with the regular args
    argument_dict = {}
    for i in range(len(args)):
        argument_dict[arguments[i]] = args[i]

    # now fill with the kwargs.
    for k in kwargs.keys():
        if k in argument_dict:
            raise TypeError("Got multiple values for keyword argument '" + k + "'")
        argument_dict[k] = kwargs[k]

    # unwrap it
    with cython_context():
        ret = _get_unity().run_toolkit(fnname, argument_dict)
    # handle errors
    if not ret[0]:
        if len(ret[1]) > 0:
            raise _ToolkitError(ret[1])
        else:
            raise _ToolkitError("Toolkit failed with unknown error")

    ret = _wrap_function_return(ret[2])
    if type(ret) is dict and 'return_value' in ret:
        return ret['return_value']
    else:
        return ret

def _make_injected_function(fn, arguments):
    return lambda *args, **kwargs: _run_toolkit_function(fn, arguments, args, kwargs)

def _class_instance_from_name(class_name, *arg, **kwarg):
    """
    class_name is of the form modA.modB.modC.class module_path splits on "."
    and the import_path is then ['modA','modB','modC'] the __import__ call is
    really annoying but essentially it reads like:

    import class from modA.modB.modC

    - Then the module variable points to modC
    - Then you get the class from the module.

    """
    # we first look in tc.extensions for the class name
    module_path = class_name.split('.')
    import_path = module_path[0:-1]
    module = __import__('.'.join(import_path), fromlist=[module_path[-1]])
    class_ = getattr(module, module_path[-1])
    instance = class_(*arg, **kwarg)
    return instance

def _create_class_instance(class_name, _proxy):
    """
    Look for the class in .extensions in case it has already been
    imported (perhaps as a builtin extensions hard compiled into unity_server).
    """
    root_package_name = __import__(__name__.split('.')[0]).__name__
    try:
        return _class_instance_from_name(root_package_name + ".extensions." + class_name, _proxy=_proxy)
    except:
        pass
    return _class_instance_from_name(class_name, _proxy=_proxy)


class _ToolkitClass:
    """
    The actual class class that is rewritten to become each user defined
    toolkit class.

    Certain care with attributes (__getattr__ / __setattr__) has to be done to
    inject functions, and attributes into their appropriate places.
    """

    _functions = {} # The functions in the class
    _get_properties = [] # The getable properties in the class
    _set_properties = [] # The setable properties in the class
    _tkclass = None


    def __init__(self, *args, **kwargs):
        tkclass_name = getattr(self.__init__, "tkclass_name")
        _proxy = None
        if "_proxy" in kwargs:
            _proxy = kwargs['_proxy']
            del kwargs['_proxy']

        if _proxy:
            self.__dict__['_tkclass'] = _proxy
        elif tkclass_name:
            self.__dict__['_tkclass'] = _get_unity().create_toolkit_class(tkclass_name)
        try:
            # fill the functions and properties
            self.__dict__['_functions'] = self._tkclass.get('list_functions')
            self.__dict__['_get_properties'] = self._tkclass.get('list_get_properties')
            self.__dict__['_set_properties'] = self._tkclass.get('list_set_properties')
            # rewrite the doc string for this class
            try:
                self.__dict__['__doc__'] = self._tkclass.get('get_docstring', {'__symbol__':'__doc__'})
                self.__class__.__dict__['__doc__'] = self.__dict__['__doc__']
            except:
                pass
        except:
            raise _ToolkitError("Cannot create Toolkit Class for this class. "
                               "This class was not created with the new toolkit class system.")
        # for compatibility with older classes / models
        self.__dict__['__proxy__'] = self.__dict__['_tkclass']

        if '__init__' in self.__dict__['_functions']:
            self.__run_class_function("__init__", args, kwargs)
        elif len(args) != 0 or len(kwargs) != 0:
            raise TypeError("This constructor takes no arguments")

    def __dir__(self):
        return list(self._functions.keys()) + self._get_properties + self._set_properties


    def __run_class_function(self, fnname, args, kwargs):
        # scan for all the arguments in args
        arguments = self._functions[fnname]
        num_args_got = len(args) + len(kwargs)
        num_args_required = len(arguments)
        if num_args_got != num_args_required:
            raise TypeError("Expecting " + str(num_args_required) + " arguments, got " + str(num_args_got))

        ## fill the dict first with the regular args
        argument_dict = {}
        for i in range(len(args)):
            argument_dict[arguments[i]] = args[i]

        # now fill with the kwargs.
        for k in kwargs.keys():
            if k in argument_dict:
                raise TypeError("Got multiple values for keyword argument '" + k + "'")
            argument_dict[k] = kwargs[k]
        # unwrap it
        argument_dict['__function_name__'] = fnname
        ret = self._tkclass.get('call_function', argument_dict)
        ret = _wrap_function_return(ret)
        return ret


    def __getattr__(self, name):
        if name == '__proxy__':
            return self.__dict__['__proxy__']
        elif name in self._get_properties:
            # is it an attribute?
            arguments = {'__property_name__':name}
            return _wrap_function_return(self._tkclass.get('get_property', arguments))
        elif name in self._functions:
            # is it a function?
            ret = lambda *args, **kwargs: self.__run_class_function(name, args, kwargs)
            ret.__doc__ = "Name: " + name + "\nParameters: " + str(self._functions[name]) + "\n"
            try:
                ret.__doc__ += self._tkclass.get('get_docstring', {'__symbol__':name})
                ret.__doc__ += '\n'
            except:
                pass
            return ret
        else:
            raise AttributeError("no attribute " + name)


    def __setattr__(self, name, value):
        if name == '__proxy__':
            self.__dict__['__proxy__'] = value
        elif name in self._set_properties:
            # is it a setable property?
            arguments = {'__property_name__':name, 'value':value}
            return _wrap_function_return(self._tkclass.get('set_property', arguments))
        else:
            raise AttributeError("no attribute " + name)

def _list_functions():
    """
    Lists all the functions registered in unity_server.
    """
    unity = _get_unity()
    return unity.list_toolkit_functions()

def _publish():

    import copy
    """
    Publishes all functions and classes registered in unity_server.
    The functions and classes will appear in the module turicreate.extensions
    """
    unity = _get_unity()
    fnlist = unity.list_toolkit_functions()
    # Loop through all the functions and inject it into
    # turicreate.extensions.[blah]
    # Note that [blah] may be somemodule.somefunction
    # and so the injection has to be
    # turicreate.extensions.somemodule.somefunction
    for fn in fnlist:
        props = unity.describe_toolkit_function(fn)
        # quit if there is nothing we can process
        if 'arguments' not in props:
            continue
        arguments = props['arguments']

        newfunc = _make_injected_function(fn, arguments)

        newfunc.__doc__ = "Name: " + fn + "\nParameters: " + str(arguments) + "\n"
        if 'documentation' in props:
            newfunc.__doc__ += props['documentation'] + "\n"

        newfunc.__dict__['__glmeta__'] = {'extension_name':fn}
        modpath = fn.split('.')
        # walk the module tree
        mod = _thismodule
        for path in modpath[:-1]:
            try:
                getattr(mod, path)
            except:
                _setattr_wrapper(mod, path, _types.ModuleType(name=path))
            mod = getattr(mod, path)
        _setattr_wrapper(mod, modpath[-1], newfunc)

    # Repeat for classes
    tkclasslist = unity.list_toolkit_classes()
    for tkclass in tkclasslist:
        m = unity.describe_toolkit_class(tkclass)
        # of v2 type
        if not ('functions' in m and 'get_properties' in m and 'set_properties' in m and 'uid' in m):
            continue

        # create a new class
        if _version_info.major == 3:
            new_class = _ToolkitClass.__dict__.copy()
            del new_class['__dict__']
            del new_class['__weakref__']
        else:
            new_class = copy.deepcopy(_ToolkitClass.__dict__)

        new_class['__init__'] = _types.FunctionType(new_class['__init__'].__code__,
                                                    new_class['__init__'].__globals__,
                                                    name='__init__',
                                                    argdefs=(),
                                                    closure=())

        # rewrite the init method to add the toolkit class name so it will
        # default construct correctly
        new_class['__init__'].tkclass_name = tkclass

        newclass = _class_type(tkclass, (), new_class)
        setattr(newclass, '__glmeta__', {'extension_name':tkclass})
        class_uid_to_class[m['uid']] = newclass
        modpath = tkclass.split('.')
        # walk the module tree
        mod = _thismodule
        for path in modpath[:-1]:
            try:
                getattr(mod, path)
            except:
                _setattr_wrapper(mod, path, _types.ModuleType(name=path))
            mod = getattr(mod, path)
        _setattr_wrapper(mod, modpath[-1], newclass)



class _ExtMetaPath(object):
    """
    This is a magic metapath searcher. To understand how this works,
    See the PEP 302 document. Essentially this class is inserted into
    the sys.meta_path list. This class must implement find_module()
    and load_module(). After which, this class is called first when any
    particular module import was requested, allowing this to essentially
    'override' the default import behaviors.
    """
    def find_module(self, fullname, submodule_path=None):
        """
        We have to see if fullname refers to a module we can import.
        Some care is needed here because:

        import xxx   # tries to load xxx.so from any of the python import paths
        import aaa.bbb.xxx # tries to load aaa/bbb/xxx.so from any of the python import paths
        """
        # first see if we have this particular so has been loaded by
        # turicreate's extension library before
        ret = self.try_find_module(fullname, submodule_path)
        if ret is not None:
            return ret
        # nope. has not been loaded before
        # lets try to find a ".so" or a ".dylib" if any of the python
        # locations
        import sys
        import os
        # This drops the last "." So if I am importing aaa.bbb.xxx
        # module_subpath is aaa.bbb
        module_subpath = ".".join(fullname.split('.')[:-1])
        for path in sys.path:
            # joins the path to aaa/bbb/xxx
            pathname = os.path.join(path, os.sep.join(fullname.split('.')))
            # try to laod the ".so" extension
            try:
                if os.path.exists(pathname + '.so'):
                    ext_import(pathname + '.so', module_subpath)
                    break
            except:
                pass

            # try to laod the ".dylib" extension
            try:
                if os.path.exists(pathname + '.dylib'):
                    ext_import(pathname + '.dylib', module_subpath)
                    break
            except:
                pass
        ret = self.try_find_module(fullname, submodule_path)
        if ret is not None:
            return ret

    def try_find_module(self, fullname, submodule_path=None):
        # check if the so has been loaded before
        # try to find the module inside of tc.extensions
        # Essentially: if fullname == aaa.bbb.xxx
        # Then we try to see if we have loaded tc.extensions.aaa.bbb.xxx
        mod = _thismodule
        modpath = fullname.split('.')
        # walk the module tree
        mod = _thismodule
        for path in modpath:
            try:
                mod = getattr(mod, path)
            except:
                return None
        return self

    def load_module(self, fullname):
        import sys
        # we may have already been loaded
        if fullname in sys.modules:
            return sys.modules[fullname]
        # try to find the module inside of tc.extensions
        # Essentially: if fullname == aaa.bbb.xxx
        # Then we try to look for tc.extensions.aaa.bbb.xxx
        mod = _thismodule
        modpath = fullname.split('.')
        for path in modpath:
            mod = getattr(mod, path)

        # Inject the module into aaa.bbb.xxx
        mod.__loader__ = self
        mod.__package__ = fullname
        mod.__name__ = fullname
        sys.modules[fullname] = mod
        return mod

_ext_meta_path_singleton = None

def _add_meta_path():
    """
    called on unity_server import to insert the meta path loader.
    """
    import sys
    global _ext_meta_path_singleton
    if _ext_meta_path_singleton is None:
        _ext_meta_path_singleton = _ExtMetaPath()
        sys.meta_path += [_ext_meta_path_singleton]


def ext_import(soname, module_subpath=""):
    """
    Loads a turicreate toolkit module (a shared library) into the
    tc.extensions namespace.

    Toolkit module created via SDK can either be directly imported,
    e.g. ``import example`` or via this function, e.g. ``turicreate.ext_import("example.so")``.
    Use ``ext_import`` when you need more namespace control, or when
    the shared library is not local, e.g. in http, s3 or hdfs.

    Parameters
    ----------
    soname : string
        The filename of the shared library to load.
        This can be a URL, or a HDFS location. For instance if soname is
        somewhere/outthere/toolkit.so
        The functions in toolkit.so will appear in tc.extensions.toolkit.*

    module_subpath : string, optional
        Any additional module paths to prepend to the toolkit module after
        it is imported. For instance if soname is
        somewhere/outthere/toolkit.so, by default
        the functions in toolkit.so will appear in tc.extensions.toolkit.*.
        However, if I module_subpath="somewhere.outthere", the functions
        in toolkit.so will appear in tc.extensions.somewhere.outthere.toolkit.*

    Returns
    -------
    out : a list of functions and classes loaded.

    Examples
    --------
    For instance, given a module which implements the function "square_root",

    .. code-block:: c++

        #include <cmath>
        #include <turicreate/sdk/toolkit_function_macros.hpp>
        double square_root(double a) {
          return sqrt(a);
        }

        BEGIN_FUNCTION_REGISTRATION
        REGISTER_FUNCTION(square_root, "a");
        END_FUNCTION_REGISTRATION

    compiled into example.so

    >>> turicreate.ext_import('example1.so')
    ['example1.square_root']

    >>> turicreate.extensions.example1.square_root(9)
    3.0

    We can customize the import location with module_subpath which can be
    used to avoid namespace conflicts when you have multiple toolkits with the
    same filename.

    >>> turicreate.ext_import('example1.so', 'math')
    ['math.example1.square_root']
    >>> turicreate.extensions.math.example1.square_root(9)
    3.0

    The module can also be imported directly, but turicreate *must* be imported
    first. turicreate will intercept the module loading process to load the
    toolkit.

    >>> import turicreate
    >>> import example1 #searches for example1.so in all the python paths
    >>> example1.square_root(9)
    3.0
    """
    unity = _get_unity()
    import os
    if os.path.exists(soname):
        soname = os.path.abspath(soname)
    else:
        soname = _make_internal_url(soname)
    ret = unity.load_toolkit(soname, module_subpath)
    if len(ret) > 0:
        raise RuntimeError(ret)
    _publish()
    # push the functions into the corresponding module namespace
    filename = os.path.basename(soname)
    return unity.list_toolkit_functions_in_dynamic_module(soname) + unity.list_toolkit_classes_in_dynamic_module(soname)



def _get_toolkit_function_name_from_function(fn):
    """
    If fn is a toolkit function either imported by turicreate.extensions.ext_import
    or the magic import system, we return the name of toolkit function.
    Otherwise we return an empty string.
    """
    try:
        if '__glmeta__' in fn.__dict__:
            return fn.__dict__['__glmeta__']['extension_name']
        else:
            return ""
    except:
        return ""

def _get_argument_list_from_toolkit_function_name(fn):
    """
    Given a toolkit function name, return the argument list
    """
    unity = _get_unity()
    fnprops = unity.describe_toolkit_function(fn)
    argnames = fnprops['arguments']
    return argnames

class _Closure:
    """
    Defines a closure class describing a lambda closure. Contains 2 fields:

    native_fn_name: The toolkit native function name

    arguments: An array of the same length as the toolkit native function.
        Each array element is an array of 2 elements [is_capture, value]

        If is_capture == 1:
            value contains the captured value
        If is_capture == 0:
            value contains a number denoting the lambda argument position.

    Example:
        lambda x, y: fn(10, x, x, y)

    Then arguments will be
        [1, 10], -->  is captured value. has value 10
        [0, 0],  -->  is not captured value. is argument 0 of the lambda.
        [0, 0],  -->  is not captured value. is argument 0 of the lambda.
        [0, 1]   -->  is not captured value. is argument 1 of the lambda.
    """
    def __init__(self, native_fn_name, arguments):
        self.native_fn_name = native_fn_name
        self.arguments = arguments


def _descend_namespace(caller_globals, name):
    """
    Given a globals dictionary, and a name of the form "a.b.c.d", recursively
    walk the globals expanding caller_globals['a']['b']['c']['d'] returning
    the result. Raises an exception (IndexError) on failure.
    """
    names =  name.split('.')
    cur = caller_globals
    for i in names:
        if type(cur) is dict:
            cur = cur[i]
        else:
            cur = getattr(cur, i)
    return cur

def _build_native_function_call(fn):
    """
    If fn can be interpreted and handled as a native function: i.e.
    fn is one of the extensions, or fn is a simple lambda closure using one of
    the extensions.

       fn = tc.extensions.add
       fn = lambda x: tc.extensions.add(5)

    Then, this returns a closure object, which describes the function call
    which can then be passed to C++.

    Returns a _Closure object on success, raises an exception on failure.

    """
    # See if fn is the native function itself
    native_function_name = _get_toolkit_function_name_from_function(fn)
    if native_function_name != "":
        # yup!
        # generate an "identity" argument list
        argnames = _get_argument_list_from_toolkit_function_name(native_function_name)
        arglist = [[0, i] for i in range(len(argnames))]
        return _Closure(native_function_name, arglist)

    # ok. its not a native function
    from .util.lambda_closure_capture import translate
    from .util.lambda_closure_capture import Parameter

    # Lets see if it is a simple lambda
    capture = translate(fn)
    # ok. build up the closure arguments
    # Try to pick up the lambda
    function = _descend_namespace(capture.caller_globals, capture.closure_fn_name)
    native_function_name = _get_toolkit_function_name_from_function(function)
    if native_function_name == "":
        raise RuntimeError("Lambda does not contain a native function")

    argnames = _get_argument_list_from_toolkit_function_name(native_function_name)

    # ok. build up the argument list. this is mildly annoying due to the mix of
    # positional and named arguments
    # make an argument list with a placeholder for everything first
    arglist = [[-1, i] for i in argnames]
    # loop through the positional arguments
    for i in range(len(capture.positional_args)):
        arg = capture.positional_args[i]
        if type(arg) is Parameter:
            # This is a lambda argument
            # arg.name is the actual string of the argument
            # here we need the index
            arglist[i] = [0, capture.input_arg_names.index(arg.name)]
        else:
            # this is a captured value
            arglist[i] = [1, arg]

    # now. the named arguments are somewhat annoying
    for i in capture.named_args:
        arg = capture.named_args[i]
        if type(arg) is Parameter:
            # This is a lambda argument
            # arg.name is the actual string of the argument
            # here we need the index
            arglist[argnames.index(i)] = [0, capture.input_arg_names.index(arg.name)]
        else:
            # this is a captured value
            arglist[argnames.index(i)] = [1, arg]

    # done. Make sure all arguments are filled
    for i in arglist:
        if i[0] == -1:
            raise RuntimeError("Incomplete function specification")

    # attempt to recursively break down any other functions
    import inspect
    for i in range(len(arglist)):
        if arglist[i][0] == 1 and  inspect.isfunction(arglist[i][1]):
            try:
                arglist[i][1] = _build_native_function_call(arglist[i][1])
            except:
                pass
    return _Closure(native_function_name, arglist)

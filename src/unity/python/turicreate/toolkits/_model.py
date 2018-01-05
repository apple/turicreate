# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
@package turicreate.toolkits

Defines a basic interface for a model object.
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import json

import turicreate as _tc
import turicreate.connect.main as glconnect
from turicreate.data_structures.sframe import SFrame as _SFrame
from turicreate.toolkits._internal_utils import _toolkit_serialize_summary_struct
from turicreate.util import _make_internal_url
from turicreate.toolkits._main import ToolkitError
import turicreate.util.file_util as file_util
import os
from copy import copy as _copy
import six as _six

MODEL_NAME_MAP = {}

def load_model(location):
    """
    Load any Turi Create model that was previously saved.

    This function assumes the model (can be any model) was previously saved in
    Turi Create model format with model.save(filename).

    Parameters
    ----------
    location : string
        Location of the model to load. Can be a local path or a remote URL.
        Because models are saved as directories, there is no file extension.

    Examples
    ----------
    >>> model.save('my_model_file')
    >>> loaded_model = tc.load_model('my_model_file')
    """

    # Check if the location is a dir_archive, if not, use glunpickler to load
    # as pure python model
    # If the location is a http location, skip the check, and directly proceed
    # to load model as dir_archive. This is because
    # 1) exists() does not work with http protocol, and
    # 2) GLUnpickler does not support http
    protocol = file_util.get_protocol(location)
    dir_archive_exists = False
    if protocol == '':
        model_path = file_util.expand_full_path(location)
        dir_archive_exists = file_util.exists(os.path.join(model_path, 'dir_archive.ini'))
    else:
        model_path = location
        if protocol in ['http', 'https']:
            dir_archive_exists = True
        else:
            import posixpath
            dir_archive_exists = file_util.exists(posixpath.join(model_path, 'dir_archive.ini'))
    model = None
    if not dir_archive_exists:
        raise IOError("Directory %s does not exist" % location)

    _internal_url = _make_internal_url(location)
    saved_state = glconnect.get_unity().load_model(_internal_url)
    # The archive version could be both bytes/unicode
    key = u'archive_version'
    archive_version = saved_state[key] if key in saved_state else saved_state[key.encode()]
    if archive_version < 0:
        raise ToolkitError("File does not appear to be a Turi Create model.")
    elif archive_version > 1:
        raise ToolkitError("Unable to load model.\n\n"
                           "This model looks to have been saved with a future version of Turi Create.\n"
                           "Please upgrade Turi Create before attempting to load this model file.")
    elif archive_version == 1:
        cls = MODEL_NAME_MAP[saved_state['model_name']]
        if 'model' in saved_state:
            # this is a native model
            return cls(saved_state['model'])
        else:
            # this is a CustomModel
            model_data = saved_state['side_data']
            model_version = model_data['model_version']
            del model_data['model_version']
            return cls._load_version(model_data, model_version)
    else:
        # very legacy model format. Attempt pickle loading
        import sys
        sys.stderr.write("This model was saved in a legacy model format. Compatibility cannot be guaranteed in future versions.\n")
        if _six.PY3:
            raise ToolkitError("Unable to load legacy model in Python 3.\n\n"
                               "To migrate a model, try loading it using Turi Create 4.0 or\n"
                               "later in Python 2 and then re-save it. The re-saved model should\n"
                               "work in Python 3.")

        if 'graphlab' not in sys.modules:
            sys.modules['graphlab'] = sys.modules['turicreate']
            # backward compatibility. Otherwise old pickles will not load
            sys.modules["turicreate_util"] = sys.modules['turicreate.util']
            sys.modules["graphlab_util"] = sys.modules['turicreate.util']

            # More backwards compatibility with the turicreate namespace code.
            for k, v in list(sys.modules.items()):
                if 'turicreate' in k:
                    sys.modules[k.replace('turicreate', 'graphlab')] = v
        #legacy loader
        import pickle
        model_wrapper = pickle.loads(saved_state[b'model_wrapper'])
        return model_wrapper(saved_state[b'model_base'])


def _get_default_options_wrapper(unity_server_model_name,
                                module_name='',
                                python_class_name='',
                                sdk_model = False):
    """
    Internal function to return a get_default_options function.

    Parameters
    ----------
    unity_server_model_name: str
        Name of the class/toolkit as registered with the unity server

    module_name: str, optional
        Name of the module.

    python_class_name: str, optional
        Name of the Python class.

    sdk_model : bool, optional (default False)
        True if the SDK interface was used for the model. False otherwise.

    Examples
    ----------
    get_default_options = _get_default_options_wrapper('classifier_svm',
                                                       'svm', 'SVMClassifier')
    """
    def get_default_options_for_model(output_type = 'sframe'):
        """
        Get the default options for the toolkit
        :class:`~turicreate.{module_name}.{python_class_name}`.

        Parameters
        ----------
        output_type : str, optional

            The output can be of the following types.

            - `sframe`: A table description each option used in the model.
            - `json`: A list of option dictionaries suitable for JSON serialization.

            | Each dictionary/row in the dictionary/SFrame object describes the
              following parameters of the given model.

            +------------------+-------------------------------------------------------+
            |      Name        |                  Description                          |
            +==================+=======================================================+
            | name             | Name of the option used in the model.                 |
            +------------------+---------+---------------------------------------------+
            | description      | A detailed description of the option used.            |
            +------------------+-------------------------------------------------------+
            | type             | Option type (REAL, BOOL, INTEGER or CATEGORICAL)      |
            +------------------+-------------------------------------------------------+
            | default_value    | The default value for the option.                     |
            +------------------+-------------------------------------------------------+
            | possible_values  | List of acceptable values (CATEGORICAL only)          |
            +------------------+-------------------------------------------------------+
            | lower_bound      | Smallest acceptable value for this option (REAL only) |
            +------------------+-------------------------------------------------------+
            | upper_bound      | Largest acceptable value for this option (REAL only)  |
            +------------------+-------------------------------------------------------+

        Returns
        -------
        out : dict/SFrame

        See Also
        --------
        turicreate.{module_name}.{python_class_name}.get_current_options

        Examples
        --------
        .. sourcecode:: python

          >>> import turicreate

          # SFrame formatted output.
          >>> out_sframe = turicreate.{module_name}.get_default_options()

          # dict formatted output suitable for JSON serialization.
          >>> out_json = turicreate.{module_name}.get_default_options('json')
        """
        if sdk_model:
            response = _tc.extensions._toolkits_sdk_get_default_options(
                                                          unity_server_model_name)
        else:
            response = _tc.extensions._toolkits_get_default_options(
                                                          unity_server_model_name)

        if output_type == 'json':
          return response
        else:
          json_list = [{'name': k, '': v} for k,v in response.items()]
          return _SFrame(json_list).unpack('X1', column_name_prefix='')\
                                   .unpack('X1', column_name_prefix='')

    # Change the doc string before returning.
    get_default_options_for_model.__doc__ = get_default_options_for_model.\
            __doc__.format(python_class_name = python_class_name,
                  module_name = module_name)
    return get_default_options_for_model

class RegistrationMetaClass(type):
    def __new__(meta, name, bases, class_dict):
        global MODEL_NAME_MAP
        cls = type.__new__(meta, name, bases, class_dict)
        # do nothing for the base Model/CustomModel classes
        if name == 'Model' or name == 'CustomModel':
            return cls

        native_name = cls._native_name()
        if isinstance(native_name, (list, tuple)):
            for i in native_name:
                MODEL_NAME_MAP[i] = cls
        elif native_name is not None:
            MODEL_NAME_MAP[native_name] = cls
        return cls


class PythonProxy(object):
    """
    Simple wrapper around a Python dict that exposes get/list_fields to emulate
    a "proxy" object entirely from Python.
    """

    def __init__(self, state={}):
        self.state = _copy(state)

    def get(self, key):
        return self.state[key]

    def keys(self):
        return self.state.keys()

    def list_fields(self):
        return list(self.state.keys())

    def __contains__(self, key):
        return self.state.__contains__(key)

    def __getitem__(self, field):
        return self.state[field]

    def __setitem__(self, key, value):
        self.state[key] = value

    def __delitem__(self, key):
        del self.state[key]

    def pop(self, key):
        return self.state.pop(key)

    def update(self, d):
        self.state.update(d)

    def get_state(self):
        return _copy(self.state)

class ExposeAttributesFromProxy(object):
    """Mixin to use when a __proxy__ class attribute should be used for
    additional fields. This allows tab-complete (i.e., calling __dir__ on the
    object) to include class methods as well as the results of
    __proxy__.list_fields().
    """
    """The UnityModel Proxy Object"""
    __proxy__ = None

    def __dir__(self):
        """
        Combine the results of dir from the current class with the results of
        list_fields().
        """
        # Combine dir(current class), the proxy's fields, and the method
        # list_fields (which is hidden in __getattribute__'s implementation.
        return dir(self.__class__) + list(self._list_fields()) + ['_list_fields']

    def _get(self, field):
        """
        Return the value contained in the model's ``field``.

        Parameters
        ----------
        field : string
            Name of the field to be retrieved.

        Returns
        -------
        out
            Value of the requested field.

        See Also
        --------
        list_fields
        """
        try:
            return self.__proxy__[field]
        except:
            raise ValueError("There is no model field called {}".format(field))

    def __getattribute__(self, attr):
        """
        Use the internal proxy object for obtaining list_fields.
        """
        proxy = object.__getattribute__(self, '__proxy__')

        # If no proxy exists, use the properties defined for the current class
        if proxy is None:
            return object.__getattribute__(self, attr)

        # Get the fields defined by the proxy object
        if not hasattr(proxy, 'list_fields'):
            fields = []
        else:
            fields = proxy.list_fields()

        def list_fields():
            return fields

        if attr == '_list_fields':
            return list_fields
        elif attr in fields:
            return self._get(attr)
        else:
            return object.__getattribute__(self, attr)

@_six.add_metaclass(RegistrationMetaClass)
class Model(ExposeAttributesFromProxy):
    """
    This class defines the minimal interface of a model object which is 
    backed by a C++ model implementation.

    All state in a Model must be stored in the C++-side __proxy__ object.

    _native_name must be implemented. _native_name can returns a list if there 
    are multiple C++ types for the same Python object. The native names *must*
    match the registered name of the model (name() method)

    The constructor must also permit construction from only 1 argument : the proxy object.

    For instance:

    class MyModel:
        @classmethod
        def _native_name(cls):
            return "MyModel"

    Or:

    class NearestNeighborsModel:
        @classmethod
        def _native_name(cls):
            return ["nearest_neighbors_ball_tree", "nearest_neighbors_brute_force", "nearest_neighbors_lsh"]
    """

    def _name(self):
        """
        Returns the name of the model class.

        Returns
        -------
        out : str
            The name of the model class.

        Examples
        --------
        >>> model_name = m._name()
        """
        return self.__class__.__name__

    def _get(self, field):
        """Return the value for the queried field.

        Each of these fields can be queried in one of two ways:

        >>> out = m['field']
        >>> out = m.get('field')  # equivalent to previous line

        Parameters
        ----------
        field : string
            Name of the field to be retrieved.

        Returns
        -------
        out : value
            The current value of the requested field.

        """
        if field in self._list_fields():
            return self.__proxy__.get(field)
        else:
            raise KeyError('Field \"%s\" not in model. Available fields are'
                         '%s.') % (field, ', '.join(self.list_fields()))

    @classmethod
    def _native_name(cls):
        raise NotImplementedError("_native_name not implemented")

    def save(self, location):
        """
        Save the model. The model is saved as a directory which can then be
        loaded using the :py:func:`~turicreate.load_model` method.

        Parameters
        ----------
        location : string
            Target destination for the model. Can be a local path or remote URL.

        See Also
        ----------
        turicreate.load_model

        Examples
        ----------
        >>> model.save('my_model_file')
        >>> loaded_model = turicreate.load_model('my_model_file')
        """
        return glconnect.get_unity().save_model(self, _make_internal_url(location))

    def summary(self, output=None):
        """
        Print a summary of the model. The summary includes a description of
        training data, options, hyper-parameters, and statistics measured
        during model creation.

        Parameters
        ----------
        output : str, None
            The type of summary to return.

            - None or 'stdout' : print directly to stdout.

            - 'str' : string of summary

            - 'dict' : a dict with 'sections' and 'section_titles' ordered
              lists. The entries in the 'sections' list are tuples of the form
              ('label', 'value').

        Examples
        --------
        >>> m.summary()
        """
        if output is None or output == 'stdout':
            pass
        elif (output == 'str'):
            return self.__repr__()
        elif output == 'dict':
            return _toolkit_serialize_summary_struct( self, \
                                            *self._get_summary_struct() )
        try:
            print(self.__repr__())
        except:
            return self.__class__.__name__


    def __repr__(self):
        raise NotImplementedError

    def __str__(self):
        return self.__repr__()


@_six.add_metaclass(RegistrationMetaClass)
class CustomModel(ExposeAttributesFromProxy):
    """
    This class is used to implement Python-only models. 

    The following must be implemented
    - _get_version
    - _get_native_state 
    - _native_name (class method)
    - _load_version (class method)

    On save, get_native_state is called which must return a dictionary
    containing the state of the object. This must contain
    all the relevant information needed to reconstruct the model. 

    On load _load_version is used to reconstruct the object.

    _native_name must return a globally unique name. This is the name used to
    identify the model.

    Example
    -------
    class MyModelMinimal(CustomModel):
        def __init__(self, prediction):
            # We use PythonProxy here so that we get tab completion
            self.__proxy__ = PythonProxy(state)

        @classmethod
        def create(cls, prediction):
            return MyModelMinimal({'prediction':prediction})

        def predict(self):
            return self.__proxy__['prediction']

        def _get_version(self):
            return 0

        @classmethod
        def _native_name(cls):
            return "MyModelMinimal"

        def _get_native_state(self):
            # return a dictionary completely describing the object
            return self.__proxy__.get_state()

        @classmethod
        def _load_version(cls, state, version):
            # loads back from a dictionary
            return MyModelMinimal(state)


    # a wrapper around logistic classifier
    class MyModelComplicated(CustomModel):
        def __init__(self, state):
            # We use PythonProxy here so that we get tab completion
            self.__proxy__ = PythonProxy(state)

        @classmethod
        def create(cls, sf, target):
            classifier = tc.logistic_classifier.create(sf, target=target)
            state = {'classifier':classifier, 'target':target}
            return MyModelComplicated(state)

        def predict(self, sf):
            return self.__proxy__['classifier'].predict(sf)

        def _get_version(self):
            return 0

        @classmethod
        def _native_name(cls):
            return "MyModelComplicated"

        def _get_native_state(self):
            # make sure to not accidentally modify the proxy object.
            # take a copy of it.
            state = self.__proxy__.get_state()

            # We don't know how to serialize a Python class, hence we need to 
            # reduce the classifier to the proxy object before saving it.
            state['classifier'] = state['classifier'].__proxy__
            return state

        @classmethod
        def _load_version(cls, state, version):
            assert(version == 0)
            # we need to undo what we did at save and turn the proxy object
            # back into a Python class
            state['classifier'] = LogisticClassifier(state['classifier'])
            return MyModelComplicated(state)


    # Construct the model
    >>> custom_model = MyModel(sf, glc_model)

    ## The model can be saved and loaded like any Turi Create model.
    >>> model.save('my_model_file')
    >>> loaded_model = tc.load_model('my_model_file')
    """

    def __init__(self):
        pass

    def name(self):
        """
        Returns the name of the model.

        Returns
        -------
        out : str
            The name of the model object.

        Examples
        --------
        >>> model_name = m.name()
        """
        return self.__class__.__name__

    def summary(self, output=None):
        """
        Print a summary of the model. The summary includes a description of
        training data, options, hyper-parameters, and statistics measured
        during model creation.

        Parameters
        ----------
        output : str, None
            The type of summary to return.

            - None or 'stdout' : print directly to stdout.

            - 'str' : string of summary

            - 'dict' : a dict with 'sections' and 'section_titles' ordered
              lists. The entries in the 'sections' list are tuples of the form
              ('label', 'value').

        Examples
        --------
        >>> m.summary()
        """
        if output is None or output == 'stdout':
            pass
        elif (output == 'str'):
            return self.__repr__()
        elif output == 'dict':
            return _toolkit_serialize_summary_struct( self, \
                                            *self._get_summary_struct() )
        try:
            print(self.__repr__())
        except:
            return self.__class__.__name__


    def _get_version(self):
        raise NotImplementedError("_get_version not implemented")

    def __getitem__(self, key):
        return self.get(key)

    def _get_native_state(self):
        raise NotImplementedError("_get_native_state not implemented")

    def save(self, location):
        """
        Save the model. The model is saved as a directory which can then be
        loaded using the :py:func:`~turicreate.load_model` method.

        Parameters
        ----------
        location : string
            Target destination for the model. Can be a local path or remote URL.

        See Also
        ----------
        turicreate.load_model

        Examples
        ----------
        >>> model.save('my_model_file')
        >>> loaded_model = tc.load_model('my_model_file')

        """
        import copy
        state = copy.copy(self._get_native_state())
        state['model_version'] = self._get_version()
        return glconnect.get_unity().save_model2(self.__class__._native_name(), location, state)

    @classmethod
    def _native_name(cls):
        raise NotImplementedError("native_name")

    @classmethod
    def _load_version(cls, state, version):
        """
        An function to load an object with a specific version of the class.

        WARNING: This implementation is very simple.
                 Overwrite for smarter implementations.

        Parameters
        ----------
        state : dict
            The saved state object

        version : int
            A version number as obtained from _get_version()
        """
        raise NotImplementedError("load")

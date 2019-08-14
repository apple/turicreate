# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _tc

# GLC Utils.
from turicreate.toolkits._internal_utils import _toolkit_repr_print
from turicreate.toolkits._internal_utils import _precomputed_field
from turicreate.util import _raise_error_if_not_of_type

# Feature engineering utils.
from . import _internal_utils
from ._feature_engineering import TransformerBase as _TransformerBase
from ._feature_engineering import Transformer as _Transformer
from copy import copy as _copy
import inspect as _inspect
import sys as _sys

class TransformerChain(_TransformerBase):
    """
    Sequentially apply a list of transforms.

    Each of the individual steps in the chain must be transformers (i.e a child
    class of `TransformerBase`) which can be one of the following:

    - Native transformer modules in Turi Create (e.g.
      :py:class:`~turicreate.toolkits.feature_engineering._feature_hasher.FeatureHasher`).
    - User-created modules (defined by inheriting
      :py:class:`~turicreate.toolkits.feature_engineering._feature_engineering.TransformerBase`).

    Parameters
    ----------
    steps: list[Transformer]
        The list of transformers to be chained. A step in the chain can be
        another chain.

    See Also
    --------
    turicreate.toolkits.feature_engineering.create

    Examples
    --------

    .. sourcecode:: python

        # Create data.
        >>> sf = turicreate.SFrame({'a': [1,2,3], 'b' : [2,3,4]})

        # Create a chain a transformers.
        >>> from turicreate.feature_engineering import *

        # Create a chain of transformers.
        >>> chain = turicreate.feature_engineering.create(sf,[
                                    QuadraticFeatures(),
                                    FeatureHasher()
                                  ])

        # Create a chain of transformers with names for each of the steps.
        >>> chain = turicreate.feature_engineering.create(sf, [
                                    ('quadratic', QuadraticFeatures()),
                                    ('hasher', FeatureHasher())
                                  ])

        # Transform the data.
        >>> transformed_sf = chain.transform(sf)

        # Save the transformer.
        >>> chain.save('save-path')

        # Access each of the steps in the transformer by name or index
        >>> steps = chain['steps']
        >>> steps = chain['steps_by_name']
    """
    _TRANSFORMER_CHAIN_VERSION = 0

    def __init__(self, steps):
        """
        Parameters
        ----------
        steps: list[Transformer] | list[tuple(name, Transformer)]

            List of Transformers or (name, Transformer) tuples. These are
            chained in the order in which they are provided in the list.

        """
        # Basic type checking.
        _raise_error_if_not_of_type(steps, [list])

        # Split into (name, transformer) pairs. If the name is not present
        # then use the index as name.
        transformers = []
        index = 0
        for step in steps:
            if isinstance(step, tuple):
                name, tr = step
            else:
                tr = step
                name = index

            if isinstance(tr, list):
                tr = TransformerChain(tr)
            if not issubclass(tr.__class__, _TransformerBase):
                raise TypeError("Each step in the chain must be a Transformer.")
            transformers.append((name, tr))
            index = index + 1

        # Save into a dictionary for lookups by name and index.
        self._state = {}
        self._state["steps"] = steps
        self._state["steps_by_name"] = {}
        index = 0
        for name, tr in transformers:
            self._state["steps_by_name"][name] = tr
            index = index + 1

        # The transformers as (name, obj) tuple (used here for fitting
        # and transforming).
        self._transformers = transformers

    @staticmethod
    def _compact_class_repr(obj):
        """ A compact version of __repr__ for each of the steps.
        """
        dict_str_list = []
        post_repr_string = ""

        # If features are present, then shorten it.
        init_func = obj.__init__
        if _sys.version_info.major == 2:
            init_func = init_func.__func__

        fields = _inspect.getargspec(init_func).args
        fields = fields[1:] # remove self
        if 'features' in fields:
            fields.remove('features')
            features = obj.get("features")
            if features is not None:
                post_repr_string = ' on %s feature(s)' % len(features)
        if 'excluded_features' in fields:
            fields.remove('excluded_features')

        # GLC transformers.
        if issubclass(obj.__class__, _Transformer):
            for attr in fields:
                dict_str_list.append("%s=%s" % (attr, obj.get(attr).__repr__()))

        # Chains
        elif obj.__class__ == TransformerChain:
            _step_classes = list(map(lambda x: x.__class__.__name__, obj.get('steps')))
            _steps = _internal_utils.pretty_print_list(
                                          _step_classes, 'steps', False)
            dict_str_list.append(_steps)

        # For user defined transformers.
        else:
            for attr in fields:
                dict_str_list.append("%s=%s" % (attr, obj.__dict__[attr]))

        return "%s(%s)%s" % (obj.__class__.__name__, ", ".join(dict_str_list),
                             post_repr_string)

    def _get_struct_summary(self):
        model_fields = []
        for name, tr in self._transformers:
            model_fields.append((name,
                         _precomputed_field(self._compact_class_repr(tr))))

        sections = [model_fields]
        section_titles = ['Steps']

        return (sections, section_titles)

    def __repr__(self):
        (sections, section_titles) = self._get_struct_summary()
        return _toolkit_repr_print(self, sections, section_titles, width=8)

    @staticmethod
    def __get_steps_repr__(steps):
        def __repr__(steps):
            for name, tr in self._transformers:
                model_fields.append((name,
                             _precomputed_field(self._compact_class_repr(tr))))
            return _toolkit_repr_print(steps, [model_fields], width=8,
                                           section_titles = ['Steps'])
        return __repr__

    def _preprocess(self, data):
        """
        Internal function to perform fit_transform() on all but last step.
        """
        transformed_data = _copy(data)
        for name, step in self._transformers[:-1]:
            transformed_data = step.fit_transform(transformed_data)
            if type(transformed_data) != _tc.SFrame:
                raise RuntimeError("The transform function in step '%s' did not"
                    " return an SFrame (got %s instead)." % (name,
                                            type(transformed_data).__name__))
        return transformed_data

    def fit(self, data):
        """
        Fits a transformer using the SFrame `data`.

        Parameters
        ----------
        data : SFrame
            The data used to fit the transformer.

        Returns
        -------
        self (A fitted object)

        See Also
        --------
        transform, fit_transform

        Examples
        --------
        .. sourcecode:: python

          >> chain = chain.fit(sf)
        """
        if not self._transformers:
            return

        transformed_data = self._preprocess(data)
        final_step = self._transformers[-1]
        final_step[1].fit(transformed_data)

    def fit_transform(self, data):
        """
        First fit a transformer using the SFrame `data` and then return a transformed
        version of `data`.

        Parameters
        ----------
        data : SFrame
            The data used to fit the transformer. The same data is then also
            transformed.

        Returns
        -------
        Transformed SFrame.

        See Also
        --------
        transform, fit_transform

        Notes
        -----
        - The default implementation calls fit() and then calls transform().
          You may override this function with a more efficient implementation."

        Examples
        --------
        .. sourcecode:: python

          >> transformed_sf = chain.fit_transform(sf)

        """
        if not self._transformers:
            return self._preprocess(data)

        transformed_data = self._preprocess(data)
        final_step = self._transformers[-1]
        return final_step[1].fit_transform(transformed_data)

    def transform(self, data):
        """
        Transform the SFrame `data` using a fitted model.

        Parameters
        ----------
        data : SFrame
            The data  to be transformed.

        Returns
        -------
        A transformed SFrame.

        Returns
        -------
        out: SFrame
            A transformed SFrame.

        See Also
        --------
        fit, fit_transform

        Examples
        --------
        .. sourcecode:: python

          >> my_tr = turicreate.feature_engineering.create(train_data, MyTransformer())
          >> transformed_sf = my_tr.transform(sf)
        """
        transformed_data = _copy(data)
        for name, step in self._transformers:
            transformed_data = step.transform(transformed_data)
            if type(transformed_data) != _tc.SFrame:
                raise TypeError("The transform function in step '%s' did not return"
                        " an SFrame." % name)
        return transformed_data

    def _list_fields(self):
        """
        List the model's queryable fields.

        Returns
        -------
        out : list
            Each element in the returned list can be queried with the ``get``
            method.
        """
        return list(self._state.keys())

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
        """
        try:
            return self._state[field]
        except:
            raise ValueError("There is no model field called {}.".format(field))

    def __getitem__(self, key):
        return self.get(key)

    def _get_version(self):
        return self._TRANSFORMER_CHAIN_VERSION

    @classmethod
    def _load_version(cls, unpickler, version):
        """
        An function to load an object with a specific version of the class.

        Parameters
        ----------
        pickler : file
            A GLUnpickler file handle.

        version : int
            A version number as maintained by the class writer.
        """
        obj = unpickler.load()
        return TransformerChain(obj._state["steps"])

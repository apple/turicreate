# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Builds a content transformer.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from . import TransformerBase as _TransformerBase
from . import TransformerChain as _TransformerChain
from . import TransformToFlatDictionary as _TransformToFlatDictionary
from turicreate.toolkits._model import PythonProxy as _PythonProxy
from turicreate.toolkits._model import ExposeAttributesFromProxy
from turicreate.toolkits._internal_utils import _toolkit_repr_print
from turicreate.toolkits._internal_utils import _precomputed_field
from turicreate.toolkits._internal_utils import _raise_error_if_not_sframe
from turicreate.util import _raise_error_if_not_of_type
from turicreate.toolkits._internal_utils import _check_categorical_option_type
from . import _internal_utils
from array import array as _array
import textwrap as _textwrap

from turicreate import SFrame as _SFrame
import turicreate as _tc

class _ColumnFunctionTransformation(_TransformerBase):
    """
    Utility transformer: Passes all specified columns through a given function.
    """

    _COLUMN_FUNCTION_TRANSFORMATION_VERSION = 0

    def _get_version(self):
        return self._COLUMN_FUNCTION_TRANSFORMATION_VERSION

    def _setup(self):
        self.__proxy__ = _PythonProxy()


    def __init__(self, features=None, excluded_features=None, output_column_prefix=None,
                 transform_function = lambda x: x, transform_function_name = "none"):

        self._setup()

        # Process and make a copy of the features, exclude.
        _features, _exclude = _internal_utils.process_features(features, excluded_features)

        #Type check
        _raise_error_if_not_of_type(output_column_prefix, [str, type(None)])

        state = {}
        state['output_column_prefix'] = output_column_prefix
        state['features'] = _features
        state['excluded_features'] = _exclude
        state['fitted'] = False
        state['transform_function'] = transform_function
        state['transform_function_name'] = transform_function_name

        if _exclude:
            self._exclude = True
            self._features = _exclude
        else:
            self._exclude = False
            self._features = _features

        self.__proxy__.update(state)

    @classmethod
    def _load_version(cls, unpickler, version):
        """
        A function to load a previously saved SentenceSplitter instance.

        Parameters
        ----------
        unpickler : GLUnpickler
            A GLUnpickler file handler.

        version : int
            Version number maintained by the class writer.
        """
        state, _exclude, _features = unpickler.load()

        features = state['features']
        excluded_features = state['excluded_features']

        model = cls.__new__(cls)
        model._setup()
        model.__proxy__.update(state)
        model._exclude = _exclude
        model._features = _features

        return model

    def _save_impl(self, pickler):
        """
        Save the model as a directory, which can be loaded with the
        :py:func:`~turicreate.load_model` method.

        Parameters
        ----------
        pickler : GLPickler
            An opened GLPickle archive (Do not close the archive).

        See Also
        --------
        turicreate.load_model

        Examples
        --------
        >>> model.save('my_model_file')
        >>> loaded_model = turicreate.load_model('my_model_file')
        """
        raise NotImplementedError("save/load not implemented for feature transformers")
        pickler.dump( (self.__proxy__.state, self._exclude, self._features) )


    def _get_summary_struct(self):
        """
        Returns a structured description of the model, including (where relevant)
        the schema of the training data, description of the training data,
        training statistics, and model hyperparameters.

        Returns
        -------
        sections : list (of list of tuples)
            A list of summary sections.
              Each section is a list.
                Each item in a section list is a tuple of the form:
                  ('<label>','<field>')
        section_titles: list
            A list of section titles.
              The order matches that of the 'sections' object.
        """

        fields = [
            ("Features", "features"),
            ("Excluded_features", "excluded_features"),
            ("Transform", "transform_function_name")
        ]
        section_titles = ['Model fields']

        return ([fields], section_titles)

    def __repr__(self):
        (sections, section_titles) = self._get_summary_struct()
        return _toolkit_repr_print(self, sections, section_titles)

    def fit(self, data):
        """
        Fits the transformer using the given data.
        """

        _raise_error_if_not_sframe(data, "data")

        fitted_state = {}
        feature_columns = _internal_utils.get_column_names(data, self._exclude, self._features)

        if not feature_columns:
            raise RuntimeError("No valid feature columns specified in transformation.")

        fitted_state['features'] = feature_columns
        fitted_state['fitted'] = True

        self.__proxy__.update(fitted_state)

        return self

    def transform(self, data):
        """
        Transforms the data.
        """

        if not self._get("fitted"):
            raise RuntimeError("`transform` called before `fit` or `fit_transform`.")

        data = data.copy()
        output_column_prefix = self._get("output_column_prefix")

        if output_column_prefix is None:
            prefix = ""
        else:
            prefix = output_column_prefix + '.'

        transform_function = self._get("transform_function")

        feature_columns = self._get("features")
        feature_columns = _internal_utils.select_feature_subset(data, feature_columns)

        for f in feature_columns:
            data[prefix + f] = transform_function(data[f])

        return data

    def fit_transform(self, data):
        """
        Fits and transforms the data.
        """
        self.fit(data)
        return self.transform(data)


class _interpretations_class(object):
    """

    The actions taken for each content interpretation and column type.
    These are looked up as '<interpretation>__<type>'.  Adding a method
    to this class is sufficient to add a new transformation type.
    """

    def __get_copy_transform(self, column_name, output_column_prefix):
        if output_column_prefix:
            return [_ColumnFunctionTransformation(
                features = [column_name], transform_function = lambda x: x,
                output_column_prefix = output_column_prefix,
                transform_function_name = "identity")]
        else:
            return []

    ########################################

    def short_text__str(self, column_name, output_column_prefix):
        """
        Transforms short text into a dictionary of TFIDF-weighted 3-gram
        character counts.
        """

        from ._ngram_counter import NGramCounter
        from ._tfidf import TFIDF

        return [NGramCounter(features=[column_name],
                             n = 3,
                             method = "character",
                             output_column_prefix = output_column_prefix),

                 TFIDF(features=[column_name],
                       min_document_frequency=0.01,
                       max_document_frequency=0.5,
                       output_column_prefix = output_column_prefix)]

    short_text__str.description = "3-Character NGram Counts -> TFIDF"
    short_text__str.output_type = dict

    ########################################

    def long_text__str(self, column_name, output_column_prefix):
        """
        Transforms long text into a dictionary of TFIDF-weighted 2-gram word
        dictionaries.
        """

        from ._ngram_counter import NGramCounter
        from ._tfidf import TFIDF

        return [NGramCounter(features=[column_name],
                             n = 2,
                             method = "word",
                             output_column_prefix = output_column_prefix),

                 TFIDF(features=[column_name],
                       min_document_frequency=0.01,
                       max_document_frequency=0.5,
                       output_column_prefix = output_column_prefix)]

    long_text__str.description = "2-Word NGram Counts -> TFIDF"
    long_text__str.output_type = dict

    ########################################

    def categorical__str(self, column_name, output_column_prefix):
        """
        Interprets a string column as a categorical variable.
        """
        return self.__get_copy_transform(column_name, output_column_prefix)

    categorical__str.description = "None"
    categorical__str.output_type = str

    ########################################

    def categorical__int(self, column_name, output_column_prefix):
        """
        Interprets an integer column as a categorical variable.
        """

        return [_ColumnFunctionTransformation(
            features = [column_name],
            output_column_prefix = output_column_prefix,
            transform_function = lambda col: col.astype(str),
            transform_function_name = "astype(str)")]

    categorical__int.description = "astype(str)"
    categorical__int.output_type = str

    ########################################

    def categorical__float(self, column_name, output_column_prefix):
        """
        Interprets a float column as a categorical variable.
        """

        return [_ColumnFunctionTransformation(
            features = [column_name],
            output_column_prefix = output_column_prefix,
            transform_function = lambda col: col.astype(str),
            transform_function_name = "astype(str)")]

    categorical__float.description = "astype(str)"
    categorical__float.output_type = str

    ########################################

    def categorical__list(self, column_name, output_column_prefix):
        """
        Interprets a list of categories as a sparse vector.
        """

        return [_TransformToFlatDictionary(features = [column_name],
                                           output_column_prefix = output_column_prefix)]

    categorical__list.description = "Flatten"
    categorical__list.output_type = dict

    ########################################

    def sparse_vector__dict(self, column_name, output_column_prefix):
        """
        Interprets a dictionary as a sparse_vector.
        """

        return [_TransformToFlatDictionary(features = [column_name],
                                           output_column_prefix = output_column_prefix)]

    sparse_vector__dict.description = "Flatten"
    sparse_vector__dict.output_type = dict

    ########################################

    def numerical__float(self, column_name, output_column_prefix):
        """
        Interprets a float column as numerical.
        """

        return self.__get_copy_transform(column_name, output_column_prefix)

    numerical__float.description = "None"
    numerical__float.output_type = float

    ########################################

    def numerical__int(self, column_name, output_column_prefix):
        """
        Interprets an integer column as numerical.
        """

        return self.__get_copy_transform(column_name, output_column_prefix)

    numerical__int.description = "None"
    numerical__int.output_type = int

    ########################################

    def vector__array(self, column_name, output_column_prefix):
        """
        Interprets a vector column as a numerical vector.
        """

        return self.__get_copy_transform(column_name, output_column_prefix)

    vector__array.description = "None"
    vector__array.output_type = _array


    ############################################################

_interpretations = _interpretations_class()

def _get_interpretation_function(interpretation, dtype):
    """
    Retrieves the interpretation function used.
    """

    type_string = dtype.__name__
    name = "%s__%s" % (interpretation, type_string)

    global _interpretations

    if not hasattr(_interpretations, name):
        raise ValueError("No transform available for type '%s' with interpretation '%s'."
                         % (type_string, interpretation))

    return getattr(_interpretations, name)

def _get_interpretation_description_and_output_type(interpretation, dtype):
    """
    Returns the description and output type for a given interpretation.
    """

    type_string = dtype.__name__
    name = "%s__%s" % (interpretation, type_string)

    if not hasattr(_interpretations_class, name):
        raise ValueError("No transform available for type '%s' with interpretation '%s'."
                         % (type_string, interpretation))

    # Need unbound method to get the attributes
    func = getattr(_interpretations_class, name)

    return func.description, func.output_type

def _get_embeddable_interpretation_doc(indent = 0):
    """
    Returns a list of the available interpretations and what they do.

    If indent is specified, then the entire doc string is indented by that amount.
    """

    output_rows = []

    # Pull out the doc string and put it in a table.
    for name in sorted(dir(_interpretations)):
        if name.startswith("_") or "__" not in name:
            continue

        interpretation, type_str = name.split("__")

        func = getattr(_interpretations, name)

        output_rows.append("%s (%s type):" % (interpretation, type_str))
        output_rows += [("  " + line) for line in _textwrap.dedent(func.__doc__).strip().split("\n")]

        output_rows.append("")

    return "\n".join(" "*indent + line for line in output_rows)

def infer_column_interpretation(column):
    """
    Returns a guessed interpretation of the column.
    """

    from turicreate.extensions import _infer_content_interpretation
    return _infer_content_interpretation(column)


class AutoVectorizer(_TransformerBase, ExposeAttributesFromProxy):
    __doc__ = _textwrap.dedent(
        """Creates a feature transformer based on the content in the provided
    data that turns arbitrary content into informative features usable
    by any Turi ML algorithm.  For example, text is parsed and
    converted into a sparse dictionary of features based on word
    occurrence and JSON blobs are flattened into a single sparse
    dictionary.

    WARNING: This feature transformer is still in beta, and some
    interpretation rules may change in the future.

    Parameters
    ----------

    features : list[str] , optional
        Column names of features to be transformed. If None, all columns are
        selected. Features must be of type str, list[str].

    excluded_features : list[str] | str | None, optional
        Column names of features to be ignored in transformation. Can be string
        or list of strings. Either 'excluded_features' or 'features' can be
        passed, but not both.

     output_column_prefix : str, optional
        The prefix to use for the column name of each transformed column.
        When provided, the transformation will add columns to the input data,
        where the new name is "`output_column_prefix`.original_column_name".
        If `output_column_prefix=None` (default), then the output column name
        is the same as the original feature column name.

     column_interpretations : dict (optional)
         If given, specifies a map of column name to interpretations. If
         this parameter is not given, or any column is left unspecified,
         then the interpretation is automatically detected.

         The column interpretation can be any one of a number of string
         values:

    Interpretations:
    ----------------

    If the interpretation of a column is not specified, then the type
    and data in each column is used to determine a good interpretation.
    Possible interpretations and corresponding types are:

    %(interpretation_docstrings)s

    """) % {"interpretation_docstrings" : _get_embeddable_interpretation_doc(indent = 2) }

    @classmethod
    def _get_instance_and_data(cls):
        sf = _tc.SFrame({'a' : [1, 2, 3, 2, 3], 'b' : ["a", "b", "a", "b", "b"]})
        encoder = AutoVectorizer( features = ['a', 'b'] )
        return encoder.fit(sf), sf

    def _setup(self):
        """
        Sets stuff up.
        """
        self.__proxy__ = _PythonProxy()

    def __init__(self, features = None, excluded_features = None, output_column_prefix = None,
                 column_interpretations = None, verbose = True):

        self._setup()

        _features, _exclude = _internal_utils.process_features(features, excluded_features)

        # Check the column_interpretations parameter type
        if column_interpretations is None:
            column_interpretations = {}

        if (not isinstance(column_interpretations, dict)
            or not all(isinstance(k, str) and isinstance(v, str)
                       for k, v in column_interpretations.items())):

            raise TypeError("`column_interpretations` must be a dictionary of "
                            "column names to interpretation strings.")

        state = {}
        state['user_column_interpretations'] = column_interpretations.copy()
        state['column_interpretations'] = column_interpretations.copy()
        state['output_column_prefix'] = output_column_prefix
        state['fitted'] = False
        state['verbose'] = verbose

        state['transforms'] = {}
        state['transform_chain'] = None

        state['features'] = _features
        state['excluded_features'] = _exclude

        if _exclude:
            self._exclude = True
            self._features = _exclude
        else:
            self._exclude = False
            self._features = _features

        self.__proxy__.update(state)


    def _setup_from_data(self, data):
        """
        Sets up the content transforms.
        """

        fitted_state = {}

        _raise_error_if_not_of_type(data, [_SFrame])

        feature_columns = _internal_utils.get_column_names(data, self._exclude, self._features)

        if not feature_columns:
            raise RuntimeError("No valid feature columns specified in transformation.")

        fitted_state["features"] = feature_columns

        ################################################################################
        # Helper functions

        def get_valid_interpretations():
            return list(n.split("__")[0] for n in dir(_interpretations) if not n.startswith("_"))

        ################################################################################
        # Check input data.

        if not isinstance(data, _SFrame):
            raise TypeError("`data` parameter must be an SFrame.")

        all_col_names = set(feature_columns)

        column_interpretations = self._get("column_interpretations").copy()

        # Make sure all the interpretations are valid.
        for k, v in column_interpretations.items():
            if k not in all_col_names:
                raise ValueError("Column '%s' in column_interpretations, but not found in `data`." % k)

        # Get the automatic column interpretations.
        for col_name in feature_columns:
            if col_name not in column_interpretations:
                n = column_interpretations[col_name] = infer_column_interpretation(data[col_name])

                if n.startswith("unknown"):
                    raise ValueError("Interpretation inference failed on column '%s'; %s"
                                     % (col_name, n[len("unknown"):].strip()))

        # Now, build up the feature transforms.
        transforms = {}
        input_types = {}

        output_column_prefix = self._get("output_column_prefix")

        assert output_column_prefix is None or type(output_column_prefix) is str

        tr_chain = []
        for col_name in feature_columns:
            in_type = input_types[col_name] = data[col_name].dtype

            intr_func = _get_interpretation_function(column_interpretations[col_name], in_type)
            tr_list = intr_func(col_name, output_column_prefix)
            transforms[col_name] = tr_list
            tr_chain += tr_list

        fitted_state["transform_chain"] = _TransformerChain(tr_chain)
        fitted_state["transforms"] = transforms
        fitted_state["input_types"] = input_types

        fitted_state["column_interpretations"] = column_interpretations

        self.__proxy__.update(fitted_state)

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
        """

        self._setup_from_data(data)
        self.transform_chain.fit(data)

        self.__proxy__.update({"fitted" : True})
        return self

    def fit_transform(self, data):
        """
        Fits and transforms the SFrame `data` using a fitted model.

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
        fit, transform
        """

        self._setup_from_data(data)
        ret = self.transform_chain.fit_transform(data)
        self.__proxy__.update({"fitted" : True})
        return ret

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
        """

        if self.transform_chain is None:
            raise RuntimeError("`transform()` method called before `fit` or `fit_transform`.")

        return self.transform_chain.transform(data)


    def _get_summary_struct(self):
        """
        Returns a structured description of the model, including (where relevant)
        the schema of the training data, description of the training data,
        training statistics, and model hyperparameters.

        Returns
        -------
        sections : list (of list of tuples)
            A list of summary sections.
              Each section is a list.
                Each item in a section list is a tuple of the form:
                  ('<feature>','<field>')
        section_titles: list
            A list of section titles.
              The order matches that of the 'sections' object.
        """

        sections = []
        fields = []

        _features = _precomputed_field(_internal_utils.pretty_print_list(self.features))
        _exclude = _precomputed_field(_internal_utils.pretty_print_list(self.excluded_features))

        header_fields = [("Features", "features"),
                         ("Excluded Features", "excluded_features")]

        sections.append("Model Fields")
        fields.append(header_fields)

        if self.user_column_interpretations:
            sections.append("User Specified Interpretations")
            fields.append(list(sorted(self._get("user_column_interpretations").items())))

        column_interpretations = self._get("column_interpretations")
        features = self._get("features")

        if self._get("fitted") and features is not None:

            n_rows = len(features)
            transform_info = [None]*n_rows

            for i, f in enumerate(features):
                interpretation = column_interpretations[f]
                input_type = self.input_types[f]
                description, output_type = _get_interpretation_description_and_output_type(
                    interpretation, input_type)

                transform_info[i] = (f, input_type.__name__, interpretation, description, output_type.__name__)

            transform_table = _SFrame()
            transform_table["Column"] = [t[0] for t in transform_info]
            transform_table["Type"] = [t[1] for t in transform_info]
            transform_table["Interpretation"] = [t[2] for t in transform_info]
            transform_table["Transforms"] = [t[3] for t in transform_info]
            transform_table["Output Type"] = [t[4] for t in transform_info]

            fields[-1].append(transform_table)

        return fields, sections

    def __repr__(self):
        """
        Return a string description of the transform.
        """
        (sections, section_titles) = self._get_summary_struct()
        return _toolkit_repr_print(self, sections, section_titles)

    @classmethod
    def _load_version(cls, unpickler, version):
        """
        A function to load a previously saved SentenceSplitter instance.

        Parameters
        ----------
        unpickler : GLUnpickler
            A GLUnpickler file handler.

        version : int
            Version number maintained by the class writer.
        """
        state, _exclude, _features = unpickler.load()

        features = state['features']
        excluded_features = state['excluded_features']

        model = cls.__new__(cls)
        model._setup()
        model.__proxy__.update(state)
        model._exclude = _exclude
        model._features = _features

        return model

    def _save_impl(self, pickler):
        """
        Save the model as a directory, which can be loaded with the
        :py:func:`~turicreate.load_model` method.

        Parameters
        ----------
        pickler : GLPickler
            An opened GLPickle archive (Do not close the archive).

        See Also
        --------
        turicreate.load_model

        Examples
        --------
        >>> model.save('my_model_file')
        >>> loaded_model = turicreate.load_model('my_model_file')
        """
        pickler.dump( (self.__proxy__.state, self._exclude, self._features) )

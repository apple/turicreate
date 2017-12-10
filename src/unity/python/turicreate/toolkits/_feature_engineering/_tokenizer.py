# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _tc
from ._feature_engineering import Transformer
from turicreate.toolkits._internal_utils import _toolkit_repr_print
from turicreate.toolkits._internal_utils import _precomputed_field
from turicreate.util import _raise_error_if_not_of_type

from . import _internal_utils

_NoneType = type(None)


_fit_examples_doc = '''
            >>> import turicreate as tc

            # Create the data
            >>> sf = tc.SFrame({
            ...     'string': ['sentence one', 'sentence two...'],
            ...     'string2': ['one', 'One Two THREE']})

            # Create a Tokenizer object that transforms all string
            # columns by default.
            >>> encoder = tc.feature_engineering.Tokenizer()

            # Fit the encoder for a given dataset.
            >>> encoder = encoder.fit(sf)

            # Inspect the object and verify that it includes all columns as
            # features.
            >>> encoder['features']
            [string, string2]
'''

_fit_transform_examples_doc = '''
            >>> import turicreate as tc

            # Create the data
            >>> sf = tc.SFrame({
            ...     'string': ['Bob\'s $books$', 'sentence two...'],
            ...     'string2': ['one', 'One Two THREE']})

            # Transform the data
            >>> encoder = tc.feature_engineering.Tokenizer()
            >>> encoder = encoder.fit(sf)
            >>> output_sf = encoder.transform(sf)
            >>> output_sf
            Columns:
                string  list
                string2 list

            Rows: 2

            Data:
            +--------------------+-------------------+
            |       string       |      string2      |
            +--------------------+-------------------+
            |  [Bob's, $books$]  |       [one]       |
            | [sentence, two...] | [One, Two, THREE] |
            +--------------------+-------------------+
            [2 rows x 2 columns]

            # Alternatively, fit and transform the data in one step
            >>> output2 = tc.feature_engineering.Tokenizer().fit_transform(sf)
            >>> output2
            Columns:
                string  list
                string2 list

            Rows: 2

            Data:
            +--------------------+-------------------+
            |       string       |      string2      |
            +--------------------+-------------------+
            |  [Bob's, $books$]  |       [one]       |
            | [sentence, two...] | [One, Two, THREE] |
            +--------------------+-------------------+
            [2 rows x 2 columns]
'''

_transform_examples_doc = '''
            >>> import turicreate as tc

            # Create the data
            >>> sf = tc.SFrame({
            ...     'string': ['Bob\'s $books$', 'sentence two...'],
            ...     'string2': ['one', 'One Two THREE']})

            # Transform the data
            >>> encoder = tc.feature_engineering.Tokenizer()
            >>> encoder = encoder.fit(sf)
            >>> output_sf = encoder.transform(sf)
            >>> output_sf
            Columns:
                string  list
                string2 list

            Rows: 2

            Data:
            +--------------------+-------------------+
            |       string       |      string2      |
            +--------------------+-------------------+
            |  [Bob's, $books$]  |       [one]       |
            | [sentence, two...] | [One, Two, THREE] |
            +--------------------+-------------------+
            [2 rows x 2 columns]
'''

class Tokenizer(Transformer):
    '''
    __init__(features=None, excluded_features=None,
        to_lower=False, delimiters=["\\\\r", "\\\\v", "\\\\n", "\\\\f", "\\\\t", " "],
        output_column_prefix=None)

    Tokenizing is a method of breaking natural language text into its smallest
    standalone and meaningful components (in English, usually space-delimited
    words, but not always).

    By default, Tokenizer tokenizes strings by space characters. The user may
    specify a customized list of delimiters, or use Penn treebank-style
    tokenization.

    .. warning::
        The default tokenization setting is now different from that of
        Turi Create v1.6. The old default was Penn treebank-style
        tokenization. (This is still available by setting `delimiters=None`.)
        The current default is to tokenize by space characters.

    Parameters
    ----------
    features : list[str] | str | None, optional
        Name(s) of feature column(s) to be transformed. If set to None, then all
        feature columns are used.

    excluded_features : list[str] | str | None, optional
        Name(s) of feature columns in the input dataset to be ignored. Either
        `excluded_features` or `features` can be passed, but not both.

    to_lower : bool, optional
        Indicates whether to map the input strings to lower case before counting.

    delimiters: list[string], optional
        A list of delimiter characters for tokenization. By default, the list
        is defined to be the list of space characters. The user can define
        any custom list of single-character delimiters. Alternatively, setting
        `delimiters=None` will use a Penn treebank-style tokenization that
        separates individual punctuation marks and detects positive and negative
        real numbers, phone numbers with no spaces, urls, and emails. The
        Penn treebank-style tokenization also attempts to separate contractions
        and possessives. For instance, "don't" would be tokenized as
        ["do", "n\'t"].

    output_column_prefix : str, optional
        The prefix to use for the column name of each transformed column.
        When provided, the transformation will add columns to the input data,
        where the new name is "`output_column_prefix`.original_column_name".
        If `output_column_prefix=None` (default), then the output column name
        is the same as the original feature column name.

    Returns
    -------
    out : Tokenizer
        A Tokenizer object which is initialized with the defined parameters.

    Notes
    -----
    This implementation of Tokenizer applies regular expressions to the natural
    language text to capture a high-recall set of valid text patterns.

    If the SFrame to be transformed already contains a column with the
    designated output column name, then that column will be replaced with the
    new output. In particular, this means that `output_column_prefix=None` will
    overwrite the original feature columns.

    References
    ----------
    - `Penn treebank tokenization <https://web.archive.org/web/19970614072242/http://www.cis.upenn.edu:80/~treebank/tokenization.html>`_

    See Also
    --------
    turicreate.toolkits.text_analytics.tokenize,
    turicreate.toolkits.feature_engineering._word_counter.WordCounter,
    turicreate.toolkits.feature_engineering._ngram_counter.NGramCounter,
    turicreate.toolkits.feature_engineering._tfidf.TFIDF,
    turicreate.toolkits.feature_engineering.create

    Examples
    --------

    .. sourcecode:: python

        >>> import turicreate
        >>> from turicreate.feature_engineering import *

        # Create a sample dataset
        >>> sf = turicreate.SFrame({
        ...    'docs': ["This is a document!", "This one's also a document."]})

        # Construct a tokenizer with default options.
        >>> tokenizer = Tokenizer()

        # Transform the data using the tokenizer.
        >>> tokenized_sf = tokenizer.fit_transform(sf)
        >>> tokenized_sf
        Columns:
            docs    list

        Rows: 2

        Data:
        +-----------------------------------+
        |                docs               |
        +-----------------------------------+
        |      [This, is, a, document!]     |
        | [This, one's, also, a, document.] |
        +-----------------------------------+
        [2 rows x 1 columns]

        # Convert to lower case and use Penn treebank-style tokenization.
        >>> ptb_tokenizer = Tokenizer(to_lower=True, delimiters=None)
        >>> tokenized_sf = ptb_tokenizer.fit_transform(sf)
        >>> tokenized_sf
        Columns:
            docs    list

        Rows: 2

        Data:
        +---------------------------------------+
        |                  docs                 |
        +---------------------------------------+
        |       [this, is, a, document, !]      |
        | [this, one, 's, also, a, document, .] |
        +---------------------------------------+
        [2 rows x 1 columns]

        # Tokenize only a single column 'docs'.
        >>> tokenizer = Tokenizer(features = ['docs'])
        >>> tokenizer['features']
        ['docs']

        # Tokenize all columns except 'docs'.
        >>> tokenizer = Tokenizer(excluded_features = ['docs'])
        >>> tokenizer['features']  # `features` are set to `None`


    '''

    _fit_examples_doc = _fit_examples_doc
    _transform_examples_doc = _transform_examples_doc
    _fit_transform_examples_doc = _fit_transform_examples_doc

    def __init__(self, features=None, excluded_features=None,
        to_lower=False, delimiters=["\r", "\v", "\n", "\f", "\t", " "],
        output_column_prefix=None):

        # Process and make a copy of the features, exclude.
        _features, _exclude = _internal_utils.process_features(features, excluded_features)

        # Type checking
        _raise_error_if_not_of_type(features, [list, str, _NoneType])
        _raise_error_if_not_of_type(excluded_features, [list, str, _NoneType])
        _raise_error_if_not_of_type(to_lower, [bool])
        _raise_error_if_not_of_type(delimiters, [list, _NoneType])
        _raise_error_if_not_of_type(output_column_prefix, [str, _NoneType])

        if delimiters is not None:
            for delim in delimiters:
                _raise_error_if_not_of_type(delim, str, "delimiters")
                if (len(delim) != 1):
                    raise ValueError("Delimiters must be single-character strings")

        # Set up options
        opts = {
          'features': features,
          'to_lower': to_lower,
          'delimiters': delimiters,
          'output_column_prefix' : output_column_prefix
        }
        if _exclude:
            opts['exclude'] = True
            opts['features'] = _exclude
        else:
            opts['exclude'] = False
            opts['features'] = _features

        # Initialize object
        proxy = _tc.extensions._Tokenizer()
        proxy.init_transformer(opts)
        super(Tokenizer, self).__init__(proxy, self.__class__)

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
        _features = _precomputed_field(
            _internal_utils.pretty_print_list(self.get('features')))

        fields = [
            ("Features", _features),
            ("Convert strings to lower case", 'to_lower'),
            ("Delimiters", "delimiters"),
            ("Output column prefix", 'output_column_prefix')
        ]
        section_titles = ['Model fields']

        return ([fields], section_titles)

    def __repr__(self):
        (sections, section_titles) = self._get_summary_struct()
        return _toolkit_repr_print(self, sections, section_titles, width=30)

    @classmethod
    def _get_instance_and_data(self):
        sf = _tc.SFrame({'docs': ["this is a test", "this is another test"]})
        encoder = Tokenizer('docs')
        return encoder.fit(sf), sf

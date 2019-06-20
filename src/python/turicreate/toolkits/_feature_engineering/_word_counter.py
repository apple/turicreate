# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _tc

# Toolkit utils.
from ._feature_engineering import Transformer
from turicreate.toolkits._internal_utils import _toolkit_repr_print
from turicreate.toolkits._internal_utils import _precomputed_field
from turicreate.util import _raise_error_if_not_of_type
# Feature engineering utils
from . import _internal_utils

_NoneType = type(None)


_fit_examples_doc = '''
            >>> import turicreate as tc

            # Create the data
            >>> sf = tc.SFrame(
            ...    {'dict': [{'this': 1, 'is': 1, 'a': 2, 'sample': 1},
            ...              {'This': 1, 'is': 1, 'example': 1, 'EXample': 2}],
            ...     'string': ['sentence one', 'sentence two...'],
            ...     'list': [['one', 'One'], ['two']]})

            # Create a WordCounter object that transforms all string/dict/list
            # columns by default.
            >>> encoder = tc.feature_engineering.WordCounter()

            # Fit the encoder for a given dataset.
            >>> encoder = encoder.fit(sf)

            # Inspect the object and verify that it includes all columns as
            # features.
            >>> encoder['features']
            ['dict', 'list', 'string']
'''

_fit_transform_examples_doc = '''
            >>> import turicreate as tc

            # Create the data
            >>> sf = tc.SFrame(
            ...    {'dict': [{'this': 1, 'is': 1, 'a': 2, 'sample': 1},
            ...              {'This': 1, 'is': 1, 'example': 1, 'EXample': 2}],
            ...     'string': ['sentence one', 'sentence two...'],
            ...     'list': [['one', 'One'], ['two']]})

            # Transform the data
            >>> encoder = tc.feature_engineering.WordCounter()
            >>> encoder = encoder.fit(sf)
            >>> output_sf = encoder.transform(sf)
            >>> output_sf[0]
            {'dict': {'a': 1, 'is': 1, 'sample': 1, 'this': 1},
             'list': {'one': 2},
             'string': {'one': 1, 'sentence': 1}}

            # Alternatively, fit and transform the data in one step
            >>> output2 = tc.feature_engineering.WordCounter().fit_transform(sf)
            >>> output2
            Columns:
                dict    dict
                list    dict
                string  dict

            Rows: 2

            Data:
            +-------------------------------------------+------------+
            |                    dict                   |    list    |
            +-------------------------------------------+------------+
            | {'sample': 1, 'a': 1, 'is': 1, 'this': 1} | {'one': 2} |
            |     {'this': 1, 'is': 1, 'example': 2}    | {'two': 1} |
            +-------------------------------------------+------------+
            +------------------------------+
            |            string            |
            +------------------------------+
            |  {'sentence': 1, 'one': 1}   |
            | {'two...': 1, 'sentence': 1} |
            +------------------------------+
            [2 rows x 3 columns]
'''

_transform_examples_doc = '''
            >>> import turicreate as tc

            # For list columns (string elements converted to lower case by default):

            >>> l1 = ['a','good','example']
            >>> l2 = ['a','better','example']
            >>> sf = tc.SFrame({'a' : [l1,l2]})
            >>> wc = tc.feature_engineering.WordCounter('a')
            >>> fit_wc = wc.fit(sf)
            >>> transformed_sf = fit_wc.transform(sf)
            Columns:
                a   dict

            Rows: 2

            Data:
            +-------------------------------------+
            |                  a                  |
            +-------------------------------------+
            |  {'a': 1, 'good': 1, 'example': 1}  |
            | {'better': 1, 'a': 1, 'example': 1} |
            +-------------------------------------+
            [2 rows x 1 columns]

            # For string columns (converted to lower case by default):

            >>> sf = tc.SFrame({'a' : ['a good example', 'a better example']})
            >>> wc = tc.feature_engineering.WordCounter('a')
            >>> fit_wc = wc.fit(sf)
            >>> transformed_sf = fit_wc.transform(sf)
            Columns:
                a   dict

            Rows: 2

            Data:
            +-------------------------------------+
            |                  a                  |
            +-------------------------------------+
            |  {'a': 1, 'good': 1, 'example': 1}  |
            | {'better': 1, 'a': 1, 'example': 1} |
            +-------------------------------------+
            [2 rows x 1 columns]

            # For dictionary columns (keys converted to lower case by default):
            >>> sf = tc.SFrame(
            ...    {'docs': [{'this': 1, 'is': 1, 'a': 2, 'sample': 1},
            ...              {'this': 1, 'IS': 1, 'another': 2, 'example': 3}]})
            >>> wc = tc.feature_engineering.WordCounter('docs')
            >>> fit_wc = wc.fit(sf)
            >>> transformed_sf = fit_wc.transform(sf)
            +--------------------------------------------------+
            |                      docs                        |
            +--------------------------------------------------+
            |    {'sample': 1, 'a': 2, 'is': 1, 'this': 1}     |
            | {'this': 1, 'is': 1, 'example': 3, 'another': 2} |
            +--------------------------------------------------+
            [2 rows x 1 columns]
'''



class WordCounter(Transformer):
    '''
    __init__(features=None, excluded_features=None,
        to_lower=True, delimiters=["\\\\r", "\\\\v", "\\\\n", "\\\\f", "\\\\t", " "],
        output_column_prefix=None)

    Transform string/dict/list columns of an SFrame into their respective
    bag-of-words representation.

    Bag-of-words is a common text representation. An input text string is first
    tokenized. Each token is understood to be a word. The output is a dictionary
    of the count of the number of times each unique word appears in the text
    string. This dictionary is a sparse representation because most of the
    words in the vocabulary do not appear in every single sentence, hence their
    count is zero, which are not explicitly included in the dictionary.

    WordCounter can be applied to all the string-, dictionary-, and list-typed
    columns in a given SFrame. Its behavior for each supported input column
    type is as follows. (See :func:`~turicreate.feature_engineering.WordCounter.transform`
    for usage examples).

    * **string** : The string is first tokenized. By default, all letters are
      first converted to lower case, then tokenized by space characters. The
      user can specify a custom delimiter list, or use Penn tree-bank style
      tokenization (see input parameter description for details). Each token
      is taken to be a word, and a dictionary is generated where each key is a
      unique word that appears in the input text string, and the value is the
      number of times the word appears. For example, "I really like Really
      fluffy dogs" would get converted to
      {'i' : 1, 'really': 2, 'like': 1, 'fluffy': 1, 'dogs':1}.

    * **list** : Each element of the list must be a string, which is tokenized
      according to the input method and tokenization settings, followed by
      counting. The behavior is analogous to that of dict-type input, where the
      count of each list element is taken to be 1. For example, under default
      settings, an input list of ['alice bob Bob', 'Alice bob'] generates an
      output bag-of-words dictionary of {'alice': 2, 'bob': 3}.

    * **dict** : The method first obtains the list of keys in the dictionary.
      This list is processed as described above.

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
        `delimiters=None` will use a Penn treebank type tokenization, which
        is better at handling punctuations. (See reference below for details.)

    output_column_prefix : str, optional
        The prefix to use for the column name of each transformed column.
        When provided, the transformation will add columns to the input data,
        where the new name is "`output_column_prefix`.original_column_name".
        If `output_column_prefix=None` (default), then the output column name
        is the same as the original feature column name.

    Returns
    -------
    out : WordCounter
        A WordCounter feature engineering object which is initialized with
        the defined parameters.

    Notes
    -----
    If the SFrame to be transformed already contains a column with the
    designated output column name, then that column will be replaced with the
    new output. In particular, this means that `output_column_prefix=None` will
    overwrite the original feature columns.

    References
    ----------
    - `Penn treebank tokenization <https://web.archive.org/web/19970614072242/http://www.cis.upenn.edu:80/~treebank/tokenization.html>`_

    See Also
    --------
    turicreate.toolkits.text_analytics.count_words,
    turicreate.toolkits.feature_engineering._ngram_counter.NGramCounter,
    turicreate.toolkits.feature_engineering._tfidf.TFIDF,
    turicreate.toolkits.feature_engineering._tokenizer.Tokenizer,
    turicreate.toolkits.feature_engineering.create

    Examples
    --------

    .. sourcecode:: python

        >>> import turicreate as tc

        # Create data.
        >>> sf = tc.SFrame({
        ...    'string': ['sentences Sentences', 'another sentence'],
        ...    'dict': [{'bob': 1, 'Bob': 0.5}, {'a': 0, 'cat': 5}],
        ...    'list': [['one', 'two', 'three'], ['a', 'cat']]})

        # Create a WordCounter transformer.
        >>> from turicreate.feature_engineering import WordCounter
        >>> encoder = WordCounter()

        # Fit and transform the data.
        >>> transformed_sf = encoder.fit_transform(sf)
        Columns:
            dict    dict
            list    dict
            string  dict

        Rows: 2

        Data:
        +------------------------+----------------------------------+
        |          dict          |               list               |
        +------------------------+----------------------------------+
        |      {'bob': 1.5}      | {'one': 1, 'three': 1, 'two': 1} |
        | {'a': 0, 'cat': 5}     |        {'a': 1, 'cat': 1}        |
        +------------------------+----------------------------------+
        +-------------------------------+
        |             string            |
        +-------------------------------+
        |        {'sentences': 2}       |
        | {'another': 1, 'sentence': 1} |
        +-------------------------------+
        [2 rows x 3 columns]

        # Penn treebank-style tokenization (recommended for smarter handling
        #    of punctuations)
        >>> sf = tc.SFrame({'string': ['sentence $$one', 'sentence two...']})
        >>> WordCounter(delimiters=None).fit_transform(sf)
        Columns:
            string  dict

        Rows: 2

        Data:
        +-----------------------------------+
        |               string              |
        +-----------------------------------+
        | {'sentence': 1, '$': 2, 'one': 1} |
        | {'sentence': 1, 'two': 1, '.': 3} |
        +-----------------------------------+
        [2 rows x 1 columns]

        # Save the transformer.
        >>> encoder.save('save-path')
'''

    # Doc strings
    _fit_examples_doc = _fit_examples_doc
    _fit_transform_examples_doc = _fit_transform_examples_doc
    _transform_examples_doc  = _transform_examples_doc

    def __init__(self, features=None, excluded_features=None,
        to_lower=True, delimiters=["\r", "\v", "\n", "\f", "\t", " "],
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
        proxy = _tc.extensions._WordCounter()
        proxy.init_transformer(opts)
        super(WordCounter, self).__init__(proxy, self.__class__)

    def _get_summary_struct(self):
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
        return _toolkit_repr_print(self, sections, section_titles, 30)

    @classmethod
    def _get_instance_and_data(self):
        sf = _tc.SFrame(
            {'docs': [{'this': 1, 'is': 1, 'a': 2, 'sample': 1},
                      {'this': 1, 'is': 1, 'another': 2, 'example': 3}]})
        encoder = WordCounter('docs')
        encoder = encoder.fit(sf)
        return encoder, sf

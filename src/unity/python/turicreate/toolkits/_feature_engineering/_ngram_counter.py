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
import warnings


_NoneType = type(None)

_fit_examples_doc = '''
            import turicreate as tc

            # Create the data
            >>> sf = tc.SFrame(
            ...    {'dict': [{'this': 1, 'is': 1, 'a': 2, 'sample': 1},
            ...              {'This': 1, 'is': 1, 'example': 1, 'EXample': 2}],
            ...     'string': ['sentence one', 'sentence two...'],
            ...     'list': [['one', 'One'], ['two']]})

            # Create a NGramCounter object that transforms all string/dict/list
            # columns by default.
            >>> encoder = tc.feature_engineering.NGramCounter()

            # Fit the encoder for a given dataset.
            >>> encoder = encoder.fit(sf)

            # Inspect the object and verify that it includes all columns as
            # features.
            >>> encoder['features']
            ['dict', 'list', 'string']
'''

_fit_transform_examples_doc = '''
            import turicreate as tc

            # Create the data
            >>> sf = tc.SFrame(
            ...    {'dict': [{'this is a': 1, 'is a sample': 1},
            ...              {'This is': 1, 'is example': 1, 'is EXample': 2}],
            ...     'string': ['sentence one', 'sentence one...'],
            ...     'list': [['one', 'One'], ['two two']]})

            # Transform the data
            >>> encoder = tc.feature_engineering.NGramCounter()
            >>> encoder = encoder.fit(sf)
            >>> output_sf = encoder.transform(sf)
            >>> output_sf.print_rows(max_column_width=60)
            +------------------------------------------+----------------+
            |                   dict                   |      list      |
            +------------------------------------------+----------------+
            | {'a sample': 1, 'this is': 1, 'is a': 2} |       {}       |
            |     {'is example': 3, 'this is': 1}      | {'two two': 1} |
            +------------------------------------------+----------------+
            +---------------------+
            |        string       |
            +---------------------+
            | {'sentence one': 1} |
            | {'sentence one': 1} |
            +---------------------+
            [2 rows x 3 columns]

            # Alternatively, fit and transform the data in one step
            >>> output2 = tc.feature_engineering.NGramCounter().fit_transform(sf)
            >>> output2
            Columns:
                dict    dict
                list    dict
                string  dict

            Rows: 2

            Data:
            +------------------------------------------+----------------+
            |                   dict                   |      list      |
            +------------------------------------------+----------------+
            | {'a sample': 1, 'this is': 1, 'is a': 2} |       {}       |
            |     {'is example': 3, 'this is': 1}      | {'two two': 1} |
            +------------------------------------------+----------------+
            +---------------------+
            |        string       |
            +---------------------+
            | {'sentence one': 1} |
            | {'sentence one': 1} |
            +---------------------+
            [2 rows x 3 columns]
'''

_transform_examples_doc = '''
            >>> import turicreate as tc

            # For list columns (string elements converted to lower case by default):
            >>> l1 = ['a good example', 'good example']
            >>> l2 = ['another example','example']
            >>> sf = tc.SFrame({'a' : [l1,l2]})
            >>> wc = tc.feature_engineering.NGramCounter('a')
            >>> fit_wc = wc.fit(sf)
            >>> transformed_sf = fit_wc.transform(sf)
            Columns:
                a   dict

            Rows: 2

            Data:
            +----------------------------------+
            |                a                 |
            +----------------------------------+
            | {'good example': 2, 'a good': 1} |
            |      {'another example': 1}      |
            +----------------------------------+
            [2 rows x 1 columns]

            # For string columns (converted to lower case by default):
            >>> sf = tc.SFrame({'a' : ['a good example', 'a better example']})
            >>> wc = tc.feature_engineering.NGramCounter('a')
            >>> fit_wc = wc.fit(sf)
            >>> transformed_sf = fit_wc.transform(sf)
            Columns:
                a   dict

            Rows: 2

            Data:
            +--------------------------------------+
            |                  a                   |
            +--------------------------------------+
            |   {'good example': 1, 'a good': 1}   |
            | {'a better': 1, 'better example': 1} |
            +--------------------------------------+
            [2 rows x 1 columns]

            # For dictionary columns (keys converted to lower case by default):
            >>> sf = tc.SFrame(
            ...    {'docs': [{'this sample': 1, 'this sample is': 2},
            ...              {'this sample IS': 1, 'another example': 3}]})
            >>> wc = tc.feature_engineering.NGramCounter('docs')
            >>> fit_wc = wc.fit(sf)
            >>> transformed_sf = fit_wc.transform(sf)
            Columns:
                a   dict

            Rows: 2

            Data:
            +----------------------------------------------------------+
            |                           docs                           |
            +----------------------------------------------------------+
            |            {'this sample': 3, 'sample is': 2}            |
            | {'another example': 3, 'this sample': 1, 'sample is': 1} |
            +----------------------------------------------------------+
            [2 rows x 1 columns]

            # Character n-grams:
            >>> sf = tc.SFrame({'a' : ['fox ox ox', 'aaa.$dd ']})
            >>> wc = tc.feature_engineering.NGramCounter('a', n=3, method='character')
            >>> fit_wc = wc.fit(sf)
            >>> transformed_sf = fit_wc.transform(sf)
            Columns:
                a   dict

            Rows: 2

            Data:
            +--------------------------------+
            |               a                |
            +--------------------------------+
            | {'oxo': 2, 'fox': 1, 'xox': 2} |
            | {'aad': 1, 'add': 1, 'aaa': 1} |
            +--------------------------------+
            [2 rows x 1 columns]

'''



class NGramCounter(Transformer):
    '''
    __init__(self, features=None, excluded_features=None,
    n=2, method="word", to_lower=True, ignore_punct=True, ignore_space=True,
    delimiters=["\\\\r", "\\\\v", "\\\\n", "\\\\f", "\\\\t", " ", \
    "!", "#", "$", "%", "&", "'", "(", ")", \
    "*", "+", ",", "-", ".", "/", ":", ";", \
    "<", "=", ">", "?", "@", "[", "\\\\", "]", \
    "^", "_", "`", "{", "|", "}", "~"], \
    output_column_prefix=None)

    Transform string/dict/list columns of an SFrame into their respective
    bag-of-ngrams representation.

    An ngram is a sequence of n consecutive tokens. NGrams are often used to
    represent natural text. Text ngrams can be word-based or character-based.
    To formulate word-based ngrams, a text string is first tokenized into words.
    An ngram is then a sliding window of n words. For character ngrams, no
    tokenization is necessary, and the sliding window is taken directly over
    accepted characters.

    The output is a dictionary of the count of the number of times each unique
    ngram appears in the text string. This dictionary is a sparse representation
    because most of the ngrams do not appear in every single sentence, hence
    they have a zero count and are not explicitly included in the dictionary.

    NGramCounter can be applied to all the string-, dictionary-, and list-typed
    columns in a given SFrame. Its behavior for each supported input column
    type is as follows. (See :func:`~turicreate.feature_engineering.NGramCounter.transform`
    for usage examples).

    * **string** : By default, all letters are first converted to lower case.
      Then, if computing word ngrams, each string is tokenized by space and
      punctuation characters. (The user can specify a custom delimiter
      list, or use Penn tree-bank style tokenization. See input parameter
      description for details.) If computing character ngrams, then each
      accepted character is understood to be a token. What is accepted is
      determined based on the flags `ignore_punct` and `ignore_space`.
      A dictionary is generated where each key is a sequence of `n` tokens that
      appears in the input text string, and the value is the number of times
      the ngram appears. For example, based on default settings, the string "I
      really like Really fluffy dogs" would generate these 2-gram counts:
      {'i really': 1, 'really like': 1, 'like really': 1, 'really fluffy': 1, 'fluffy dogs': 1}.
      The string "aaa..hhh" would generate these character 2-gram counts:
      {'aa': 2, 'ah': 1, 'hh': 2}.

    * **dict** : Each (key, value) pair is treated as a string-count pair. The
      keys are tokenized according to either word or character tokenization
      methods. Input keys must be strings and input values numeric (integer or
      float). The output dictionary is a sum of the input values for the
      ngrams in the key string. For example, under default settings, the input
      dictionary {'alice bob Bob': 1, 'Alice bob': 2.5} would generate a word
      2-gram dictionary of {'alice bob': 3.5, 'bob bob': 1}.

    * **list** : Each element of the list must be a string, which is tokenized
      according to the input method and tokenization settings, followed by
      ngram counting. The behavior is analogous to that of dict-type input,
      where the count of each list element is taken to be 1. For example, under
      the default settings, an input list of ['alice bob Bob', 'Alice bob']
      generates an output word 2-gram dictionary of {'alice bob': 2, 'bob bob': 1}.

    Parameters
    ----------
    features : list[str] | str | None, optional
        Name(s) of feature column(s) to be transformed. If set to None, then all
        feature columns are used.

    excluded_features : list[str] | str | None, optional
        Name(s) of feature columns in the input dataset to be ignored. Either
        `excluded_features` or `features` can be passed, but not both.

    n : int, optional
        The number of words in each n-gram. An ``n`` value of 1 returns word
        counts.

    method : {'word', 'character'}, optional
        If "word", the function performs a count of word n-grams. If
        "character", does a character n-gram count.

    to_lower : bool, optional
        If True, all strings are converted to lower case before counting.

    ignore_punct : bool, optional
        If method is "character", indicates if *punctuations* between words are
        counted as part of the n-gram. For instance, with the input SArray
        element of "fun.games", if this parameter is set to False one
        tri-gram would be 'n.g'. If ``ignore_punct`` is set to True, there
        would be no such tri-gram (there would still be 'nga'). This
        parameter has no effect if the method is set to "word".

    ignore_space : bool, optional
        If method is "character", indicates if *spaces* between words are
        counted as part of the n-gram. For instance, with the input SArray
        element of "fun games", if this parameter is set to False one
        tri-gram would be 'n g'. If ``ignore_space`` is set to True, there
        would be no such tri-gram (there would still be 'nga'). This
        parameter has no effect if the method is set to "word".

    delimiters: list[string], optional
        A list of delimiter characters for tokenization. By default, the list
        is defined to be the list of space and punctuation characters. The
        user can define any custom list of single-character delimiters.
        Alternatively, setting `delimiters=None` will use a Penn treebank type
        tokenization, which is better at handling punctuations. (See reference
        below for details.)

    output_column_prefix : str, optional
        The prefix to use for the column name of each transformed column.
        When provided, the transformation will add columns to the input data,
        where the new name is "`output_column_prefix`.original_column_name".
        If `output_column_prefix=None` (default), then the output column name
        is the same as the original feature column name.

    Returns
    -------
    out : NGramCounter
        A NGramCounter feature engineering object which is initialized with
        the defined parameters.

    Notes
    -----
    If the SFrame to be transformed already contains a column with the
    designated output column name, then that column will be replaced with the
    new output. In particular, this means that `output_column_prefix=None` will
    overwrite the original feature columns.

    A bag-of-words representation is essentially an ngram where `n=1`. Larger
    `n` generates more unique ngrams. Therefore the output dictionary will
    be more sparse, contain more unique keys, and will be more expensive to
    compute. Calling this function with large values `n` (larger than 3 or 4)
    should be done very carefully.

    References
    ----------
    - `N-gram wikipedia article <http://en.wikipedia.org/wiki/N-gram>`_
    - `Penn treebank tokenization <https://web.archive.org/web/19970614072242/http://www.cis.upenn.edu:80/~treebank/tokenization.html>`_

    See Also
    --------
    turicreate.toolkits.text_analytics.count_ngrams,
    turicreate.toolkits.feature_engineering._ngram_counter.WordCounter,
    turicreate.toolkits.feature_engineering._tfidf.TFIDF,
    turicreate.toolkits.feature_engineering._tokenizer.Tokenizer,
    turicreate.toolkits.feature_engineering.create

    Examples
    --------

    .. sourcecode:: python

        import turicreate as tc

        # Create data.
        >>> sf = tc.SFrame({
        ...    'string': ['sent.ences Sent.ences', 'another sentence'],
        ...    'dict': [{'alice bob': 1, 'Bob alice': 0.5}, {'a dog': 0, 'a dog cat': 5}],
        ...    'list': [['one', 'bar bah'], ['a dog', 'a dog cat']]})

        # Create a NGramCounter transformer.
        >>> from turicreate.feature_engineering import NGramCounter
        >>> encoder = NGramCounter()

        # Save the transformer.
        >>> encoder.save('save-path')

        # Fit and transform the data.
        >>> transformed_sf = encoder.fit_transform(sf)
        Columns:
            dict    dict
            list    dict
            string  dict

        Rows: 2

        Data:
        +------------------------------------+----------------------------+
        |                dict                |            list            |
        +------------------------------------+----------------------------+
        | {'bob alice': 0.5, 'alice bob': 1} |       {'bar bah': 1}       |
        |     {'dog cat': 5, 'a dog': 5}     | {'dog cat': 1, 'a dog': 2} |
        +------------------------------------+----------------------------+
        +------------------------------------+
        |               string               |
        +------------------------------------+
        | {'sent ences': 2, 'ences sent': 1} |
        |      {'another sentence': 1}       |
        +------------------------------------+
        [2 rows x 3 columns]

        # Penn treebank-style tokenization (recommended for smarter handling
        #    of punctuations)
        >>> sf = tc.SFrame({'string': ['sentence $$one', 'sentence two...']})
        >>> NGramCounter(delimiters=None).fit_transform(sf)
        Columns:
            string  dict

        Rows: 2

        Data:
        +-------------------------------------------+
        |                   string                  |
        +-------------------------------------------+
        |  {'sentence $': 1, '$ $': 1, '$ one': 1}  |
        | {'sentence two': 1, '. .': 2, 'two .': 1} |
        +-------------------------------------------+
        [2 rows x 1 columns]

        # Character n-grams
        >>> sf = tc.SFrame({'string': ['aa$bb.', ' aa bb  ']})
        >>> NGramCounter(method='character').fit_transform(sf)
        Columns:
            string  dict

        Rows: 2

        Data:
        +-----------------------------+
        |            string           |
        +-----------------------------+
        | {'aa': 1, 'ab': 1, 'bb': 1} |
        | {'aa': 1, 'ab': 1, 'bb': 1} |
        +-----------------------------+
        [2 rows x 1 columns]

        # Character n-grams, not skipping over spaces or punctuations
        >>> sf = tc.SFrame({'string': ['aa$bb.', ' aa bb  ']})
        >>> encoder = NGramCounter(method='character', ignore_punct=False, ignore_space=False)
        >>> encoder.fit_transform(sf)
        Columns:
            string  dict

        Rows: 2
        Data:
        +-----------------------------------------------------------------+
        |                              string                             |
        +-----------------------------------------------------------------+
        |          {'aa': 1, 'b.': 1, '$b': 1, 'a$': 1, 'bb': 1}          |
        | {' b': 1, 'aa': 1, '  ': 1, ' a': 1, 'b ': 1, 'bb': 1, 'a ': 1} |
        +-----------------------------------------------------------------+
        [2 rows x 1 columns]

    '''

    # Doc strings
    _fit_examples_doc = _fit_examples_doc
    _fit_transform_examples_doc = _fit_transform_examples_doc
    _transform_examples_doc  = _transform_examples_doc

    def __init__(self, features=None, excluded_features=None,
        n=2, method="word", to_lower=True, ignore_punct=True, ignore_space=True,
        delimiters=["\r", "\v", "\n", "\f", "\t", " ",
                    "!", "#", "$", "%", "&", "'", "(", ")",
                    "*", "+", ",", "-", ".", "/", ":", ";",
                    "<", "=", ">", "?", "@", "[", "\\", "]",
                    "^", "_", "`", "{", "|", "}", "~"],
        output_column_prefix=None):

        # Process and make a copy of the features, exclude.
        _features, _exclude = _internal_utils.process_features(features, excluded_features)

        # Type checking
        _raise_error_if_not_of_type(features, [list, str, _NoneType])
        _raise_error_if_not_of_type(excluded_features, [list, str, _NoneType])
        _raise_error_if_not_of_type(n, [int])
        _raise_error_if_not_of_type(method, [str])
        _raise_error_if_not_of_type(to_lower, [bool])
        _raise_error_if_not_of_type(ignore_punct, [bool])
        _raise_error_if_not_of_type(ignore_space, [bool])
        _raise_error_if_not_of_type(delimiters, [list, _NoneType])
        _raise_error_if_not_of_type(output_column_prefix, [str, _NoneType])

        if delimiters is not None:
            for delim in delimiters:
                _raise_error_if_not_of_type(delim, str, "delimiters")
                if (len(delim) != 1):
                    raise ValueError("Delimiters must be single-character strings")

        if n < 1:
            raise ValueError("Input 'n' must be greater than 0")

        if n > 5 and method == 'word':
            warnings.warn("It is unusual for n-grams to be of size larger than 5.")

        if method != "word" and method != "character":
            raise ValueError("Invalid 'method' input  value. Please input " +
                             "either 'word' or 'character' ")

        # Set up options
        opts = {
          'n': n,
          'features': features,
          'ngram_type': method,
          'to_lower': to_lower,
          'ignore_punct': ignore_punct,
          'ignore_space': ignore_space,
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
        proxy = _tc.extensions._NGramCounter()
        proxy.init_transformer(opts)
        super(NGramCounter, self).__init__(proxy, self.__class__)

    def _get_summary_struct(self):
        _features = _precomputed_field(
            _internal_utils.pretty_print_list(self.get('features')))
        fields = [
            ("NGram length", 'n'),
            ("NGram type (word or character)", 'ngram_type'),
            ("Convert strings to lower case", 'to_lower'),
            ("Ignore punctuation in character ngram", 'ignore_punct'),
            ("Ignore space in character ngram", 'ignore_space'),
            ("Delimiters", "delimiters"),
            ("Features", _features),
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
        encoder = _tc.feature_engineering.NGramCounter('docs')
        encoder = encoder.fit(sf)
        return encoder, sf

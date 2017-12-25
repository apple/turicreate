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
from turicreate.toolkits._private_utils import _summarize_accessible_fields
from turicreate.util import _raise_error_if_not_of_type
# Feature engineering utils
from . import _internal_utils


_fit_examples_doc = '''
            >>> import turicreate as tc

            # Create the data
            >>> sf = tc.SFrame(
            ...    {'dict': [{'this': 1, 'is': 1, 'a': 2, 'sample': 1},
            ...              {'This': 1, 'is': 1, 'example': 1, 'EXample': 2}],
            ...     'string': ['sentence one', 'sentence two...'],
            ...     'list': [['one', 'One'], ['two']]})

            # Create a RareWordTrimmer object that transforms all string/dict/list
            # columns by default.
            >>> encoder = tc.feature_engineering.RareWordTrimmer()

            # Fit the encoder for a given dataset.
            >>> trimmer = trimmer.fit(sf)

            # Inspect the object and verify that it includes all columns as
            # features.
            >>> trimmer['features']
            ['dict', 'list', 'string']

            # Inspect the retained vocabulary
            >>> trimmer['vocabulary']
            Columns:
                column  str
                word    str
                count   int

            Rows: 6

            Data:
            +--------+----------+-------+
            | column |   word   | count |
            +--------+----------+-------+
            |  dict  |   this   |   2   |
            |  dict  |    a     |   2   |
            |  dict  | example  |   2   |
            |  dict  |    is    |   2   |
            |  list  |   one    |   2   |
            | string | sentence |   2   |
            +--------+----------+-------+
            [6 rows x 3 columns]
'''

_fit_transform_examples_doc = '''
            >>> import turicreate as tc

            # Create the data
            >>> sf = tc.SFrame(
            ...    {'dict': [{'this': 1, 'is': 1, 'a': 2, 'sample': 1},
            ...              {'This': 1, 'is': 1, 'example': 1, 'EXample': 2}],
            ...     'string': ['sentence one', 'sentence two...'],
            ...     'list': [['one', 'One'], ['two', 'two', 'Three']]})

            # Transform the data
            >>> trimmer = tc.feature_engineering.RareWordTrimmer()
            >>> trimmer = trimmer.fit(sf)
            >>> output_sf = trimmer.transform(sf)
            >>> output_sf[0]
            {'dict': {'a': 2, 'is': 1, 'this': 1},
             'list': ['one', 'one'],
             'string': 'sentence'}

            # Alternatively, fit and transform the data in one step
            >>> output2 = tc.feature_engineering.RareWordTrimmer().fit_transform(sf)
            >>> output2
            Columns:
                dict    dict
                list    list
                string  str

            Rows: 2

            Data:
            +-------------------------------+------------+----------+
            |              dict             |    list    |  string  |
            +-------------------------------+------------+----------+
            |  {'this': 1, 'a': 2, 'is': 1} | [one, one] | sentence |
            | {'this': 1, 'is': 1, 'exam... | [two, two] | sentence |
            +-------------------------------+------------+----------+
            [2 rows x 3 columns]
'''

_transform_examples_doc = '''
            >>> import turicreate as tc

            # For list columns (string elements converted to lower case by default):

            >>> l1 = ['a','good','example']
            >>> l2 = ['a','better','example']
            >>> sf = tc.SFrame({'a' : [l1,l2]})
            >>> wt = tc.feature_engineering.RareWordTrimmer('a')
            >>> fit_wt = wt.fit(sf)
            >>> transformed_sf = fit_wt.transform(sf)
            Columns:
                a   list

            Rows: 2

            Data:
            +--------------+
            |      a       |
            +--------------+
            | [a, example] |
            | [a, example] |
            +--------------+
            [2 rows x 1 columns]

            # For string columns (converted to lower case by default):

            >>> sf = tc.SFrame({'a' : ['a good example', 'a better example']})
            >>> wc = tc.feature_engineering.RareWordTrimmer('a')
            >>> fit_wt = wt.fit(sf)
            >>> transformed_sf = fit_wt.transform(sf)
            Columns:
                a	str

            Rows: 2

            Data:
            +-----------+
            |     a     |
            +-----------+
            | a example |
            | a example |
            +-----------+
            [2 rows x 1 columns]

            # For dictionary columns (keys converted to lower case by default):
            >>> sf = tc.SFrame(
            ...    {'docs': [{'this': 1, 'is': 1, 'a': 2, 'sample': 1},
            ...              {'this': 1, 'IS': 1, 'another': 2, 'example': 3}]})
            >>> wt = tc.feature_engineering.RareWordTrimmer('docs')
            >>> fit_wt = wt.fit(sf)
            >>> transformed_sf = fit_wt.transform(sf)
            Columns:
                docs    dict

            Rows: 2

            Data:
            +-------------------------------+
            |              docs             |
            +-------------------------------+
            |  {'this': 1, 'a': 2, 'is': 1} |
            | {'this': 1, 'is': 1, 'exam... |
            +-------------------------------+
            [2 rows x 1 columns]
'''


class RareWordTrimmer(Transformer):
    '''
    Remove words that occur below a certain number of times in a given column.
    This is a common method of cleaning text before it is used, and can increase the
    quality and explainability of the models learned on the transformed data.

    RareWordTrimmer can be applied to all the string-, dictionary-, and list-typed
    columns in a given SFrame. Its behavior for each supported input column
    type is as follows. (See :func:`~turicreate.feature_engineering.RareWordTrimmer.transform`
    for usage examples).

    * **string** : The string is first tokenized. By default, all letters are
      first converted to lower case, then tokenized by space characters. Each
      token is taken to be a word, and the words occurring below a threshold
      number of times across the entire column are removed, then the remaining
      tokens are concatenated back into a string.

    * **list** : Each element of the list must be a string, where each element
      is assumed to be a token. The remaining tokens are then filtered
      by count occurrences and a threshold value.

    * **dict** : The method first obtains the list of keys in the dictionary.
      This list is then processed as a standard list, except the value of each
      key must be of integer type and is considered to be the count of that key.

    Parameters
    ----------
    features : list[str] | str | None, optional
        Name(s) of feature column(s) to be transformed. If set to None, then all
        feature columns are used.

    excluded_features : list[str] | str | None, optional
        Name(s) of feature columns in the input dataset to be ignored. Either
        `excluded_features` or `features` can be passed, but not both.

    threshold : int, optional
        The count below which words are removed from the input.

    stopwords: list[str], optional
        A manually specified list of stopwords, which are removed regardless
        of count.

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
    out : RareWordTrimmer
        A RareWordTrimmer feature engineering object which is initialized with
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
        ...    'string': ['sentences Sentences', 'another sentence another year'],
        ...    'dict': [{'bob': 1, 'Bob': 2}, {'a': 0, 'cat': 5}],
        ...    'list': [['one', 'two', 'three', 'Three'], ['a', 'cat', 'Cat']]})

        # Create a RareWordTrimmer transformer.
        >>> from turicreate.feature_engineering import RareWordTrimmer
        >>> trimmer = RareWordTrimmer()

        # Fit and transform the data.
        >>> transformed_sf = trimmer.fit_transform(sf)
        Columns:
            dict    dict
            list    list
            string  str

        Rows: 2

        Data:
        +------------+----------------+---------------------+
        |    dict    |      list      |        string       |
        +------------+----------------+---------------------+
        | {'bob': 2} | [three, three] | sentences sentences |
        | {'cat': 5} |   [cat, cat]   |   another another   |
        +------------+----------------+---------------------+
        [2 rows x 3 columns]

       # Save the transformer.
       >>> trimmer.save('save-path')
'''

    # Doc strings
    _fit_examples_doc = _fit_examples_doc
    _transform_examples_doc = _transform_examples_doc
    _fit_transform_examples_doc = _fit_transform_examples_doc

    def __init__(self, features=None, excluded_features=None,
            threshold=2,stopwords=None,to_lower=True, delimiters=["\r", "\v", "\n", "\f", "\t", " "],
output_column_prefix = None):

        # Process and make a copy of the features, exclude.
        _features, _exclude = _internal_utils.process_features(
                                        features, excluded_features)

        # Type checking
        _raise_error_if_not_of_type(features, [list, str, type(None)])
        _raise_error_if_not_of_type(threshold, [int, type(None)])
        _raise_error_if_not_of_type(output_column_prefix, [str, type(None)])
        _raise_error_if_not_of_type(stopwords, [list, type(None)])
        _raise_error_if_not_of_type(to_lower, [bool])
        _raise_error_if_not_of_type(delimiters, [list, type(None)])

        if delimiters is not None:
            for delim in delimiters:
                _raise_error_if_not_of_type(delim, str, "delimiters")
                if (len(delim) != 1):
                    raise ValueError("Delimiters must be single-character strings")



        # Set up options
        opts = {
          'threshold': threshold,
          'output_column_prefix': output_column_prefix,
          'to_lower' : to_lower,
          'stopwords' : stopwords,
          'delimiters': delimiters
        }
        if _exclude:
            opts['exclude'] = True
            opts['features'] = _exclude
        else:
            opts['exclude'] = False
            opts['features'] = _features

        # Initialize object
        proxy = _tc.extensions._RareWordTrimmer()
        proxy.init_transformer(opts)
        super(RareWordTrimmer, self).__init__(proxy, self.__class__)

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
        _exclude = _precomputed_field(
            _internal_utils.pretty_print_list(self.get('excluded_features')))
        _stopwords = _precomputed_field(
            _internal_utils.pretty_print_list(self.get('stopwords')))

        fields = [
            ("Features", _features),
            ("Excluded features", _exclude),
            ("Output column name", 'output_column_prefix'),
            ("Word count threshold", 'threshold'),
            ("Manually specified stopwords", _stopwords),
            ("Whether to convert to lowercase", "to_lower"),
            ("Delimiters" , "delimiters")
        ]
        section_titles = ['Model fields']

        return ([fields], section_titles)

    def __repr__(self):
        """
        Return a string description of the model, including a description of
        the training data, training statistics, and model hyper-parameters.

        Returns
        -------
        out : string
            A description of the model.
        """
        accessible_fields = {
            "vocabulary": "The vocabulary of the trimmed input."}
        (sections, section_titles) = self._get_summary_struct()
        out = _toolkit_repr_print(self, sections, section_titles, width=30)
        out2 = _summarize_accessible_fields(accessible_fields, width=30)
        return out + "\n" + out2

    @classmethod
    def _get_instance_and_data(cls):
        sf = _tc.SFrame({'a' : ['dog', 'dog' , 'dog'], 'b' : ['cat', 'one' ,'one']})
        trimmer = RareWordTrimmer(
                    features = ['a', 'b'])
        return trimmer.fit(sf), sf

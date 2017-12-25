# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _turicreate
from turicreate import SArray as _SArray
from turicreate import SFrame as _SFrame
from turicreate.toolkits._internal_utils import _raise_error_if_not_sarray
import random
import six
import turicreate.toolkits._feature_engineering as _feature_engineering


def count_words(sa, to_lower=True,
                delimiters=["\r", "\v", "\n", "\f", "\t", " "]):
    """
    count_words(sa, to_lower=True, delimiters=["\\\\r", "\\\\v", "\\\\n", "\\\\f", "\\\\t", " "])

    Convert the content of string/dict/list type SArrays to a dictionary of
    (word, count) pairs. Dictionary keys and list elements must be strings.
    The strings are first tokenized into words according to the specified
    `to_lower` and `delimiters` options. Then, word counts are accumulated.
    In each output dictionary, the keys are the words in the corresponding
    input data entry, and the values are the number of times the words appears.
    By default, words are split on all whitespace and newline characters. The
    output is commonly known as the "bag-of-words" representation of text data.

    This function is implemented using

    Parameters
    ----------
    sa : SArray[str | dict | list]
        Input data to be tokenized and counted. 

    to_lower : bool, optional
        If True, all strings are converted to lower case before counting.

    delimiters : list[str], None, optional
        Input strings are tokenized using delimiter characters in this list.
        Each entry in this list must contain a single character. If set to
        `None`, then a Penn treebank-style tokenization is used, which contains
        smart handling of punctuations.

    Returns
    -------
    out : SArray[dict]
        Each entry contains a dictionary with the frequency count of each word
        in the corresponding input entry.

    See Also
    --------
    count_ngrams, tf_idf, tokenize,

    References
    ----------
    - `Bag of words model <http://en.wikipedia.org/wiki/Bag-of-words_model>`_
    - `Penn treebank tokenization <https://web.archive.org/web/19970614072242/http://www.cis.upenn.edu:80/~treebank/tokenization.html>`_

    Examples
    --------
    .. sourcecode:: python

        >>> import turicreate

        # Create input data
        >>> sa = turicreate.SArray(["The quick brown fox jumps.",
        ...                       "Word word WORD, word!!!word"])

        # Run count_words
        >>> turicreate.text_analytics.count_words(sa)
        dtype: dict
        Rows: 2
        [{'quick': 1, 'brown': 1, 'the': 1, 'fox': 1, 'jumps.': 1},
         {'word,': 1, 'word!!!word': 1, 'word': 2}]

        # Run count_words with Penn treebank style tokenization to handle
        # punctuations
        >>> turicreate.text_analytics.count_words(sa, delimiters=None)
        dtype: dict
        Rows: 2
        [{'brown': 1, 'jumps': 1, 'fox': 1, '.': 1, 'quick': 1, 'the': 1},
         {'word': 3, 'word!!!word': 1, ',': 1}]

        # Run count_words with dictionary input
        >>> sa = turicreate.SArray([{'alice bob': 1, 'Bob alice': 0.5},
        ...                       {'a dog': 0, 'a dog cat': 5}])
        >>> turicreate.text_analytics.count_words(sa)
        dtype: dict
        Rows: 2
        [{'bob': 1.5, 'alice': 1.5}, {'a': 5, 'dog': 5, 'cat': 5}]

        # Run count_words with list input
        >>> sa = turicreate.SArray([['one', 'bar bah'], ['a dog', 'a dog cat']])
        >>> turicreate.text_analytics.count_words(sa)
        dtype: dict
        Rows: 2
        [{'bar': 1, 'bah': 1, 'one': 1}, {'a': 2, 'dog': 2, 'cat': 1}]

    """

    _raise_error_if_not_sarray(sa, "sa")

    ## Compute word counts
    sf = _turicreate.SFrame({'docs': sa})
    fe = _feature_engineering.WordCounter(features='docs',
                                                   to_lower=to_lower,
                                                   delimiters=delimiters,
                                                   output_column_prefix=None)
    output_sf = fe.fit_transform(sf)

    return output_sf['docs']

def count_ngrams(sa, n=2, method="word", to_lower=True,
    delimiters=["\r", "\v", "\n", "\f", "\t", " ",
                "!", "#", "$", "%", "&", "'", "(", ")",
                "*", "+", ",", "-", ".", "/", ":", ";",
                "<", "=", ">", "?", "@", "[", "\\", "]",
                "^", "_", "`", "{", "|", "}", "~"],
    ignore_punct=True, ignore_space=True):
    """
    count_ngrams(sa, to_lower=True, delimiters=["\\\\r", "\\\\v", "\\\\n", "\\\\f", "\\\\t", " ", "!", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/", ":", ";", "<", "=", ">", "?", "@", "[", "\\\\", "]", "^", "_", "`", "{", "|", "}", "~"], ignore_punct=True, ignore_space=True)

    Return an SArray of ``dict`` type where each element contains the count
    for each of the n-grams that appear in the corresponding input element.
    The n-grams can be specified to be either character n-grams or word
    n-grams.  The input SArray could contain strings, dicts with string keys
    and numeric values, or lists of strings.

    This function is implemented using

    Parameters
    ----------
    sa : SArray[str | dict | list]
        Input text data. 

    n : int, optional
        The number of words in each n-gram. An ``n`` value of 1 returns word
        counts.

    method : {'word', 'character'}, optional
        If "word", the function performs a count of word n-grams. If
        "character", does a character n-gram count.

    to_lower : bool, optional
        If True, all words are converted to lower case before counting.

    delimiters : list[str], None, optional
        If method is "word", input strings are tokenized using delimiter
        characters in this list. Each entry in this list must contain a single
        character. If set to `None`, then a Penn treebank-style tokenization is
        used, which contains smart handling of punctuations. If method is
        "character," this option is ignored.

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

    Returns
    -------
    out : SArray[dict]
      An SArray of dictionary type, where each key is the n-gram string
      and each value is its count.

    See Also
    --------
    count_words, tokenize,

    Notes
    -----
    - Ignoring case (with ``to_lower``) involves a full string copy of the
      SArray data. To increase speed for large documents, set ``to_lower`` to
      False.

    - Punctuation and spaces are both delimiters by default when counting
      word n-grams. When counting character n-grams, one may choose to ignore
      punctuations, spaces, neither, or both.

    References
    ----------
    - `N-gram wikipedia article <http://en.wikipedia.org/wiki/N-gram>`_
    - `Penn treebank tokenization <https://web.archive.org/web/19970614072242/http://www.cis.upenn.edu:80/~treebank/tokenization.html>`_

    Examples
    --------
    .. sourcecode:: python

        >>> import turicreate

        # Counting word n-grams:
        >>> sa = turicreate.SArray(['I like big dogs. I LIKE BIG DOGS.'])
        >>> turicreate.text_analytics.count_ngrams(sa, 3)
        dtype: dict
        Rows: 1
        [{'big dogs i': 1, 'like big dogs': 2, 'dogs i like': 1, 'i like big': 2}]

        # Counting character n-grams:
        >>> sa = turicreate.SArray(['Fun. Is. Fun'])
        >>> turicreate.text_analytics.count_ngrams(sa, 3, "character")
        dtype: dict
        Rows: 1
        {'fun': 2, 'nis': 1, 'sfu': 1, 'isf': 1, 'uni': 1}]

        # Run count_ngrams with dictionary input
        >>> sa = turicreate.SArray([{'alice bob': 1, 'Bob alice': 0.5},
        ...                       {'a dog': 0, 'a dog cat': 5}])
        >>> turicreate.text_analytics.count_ngrams(sa)
        dtype: dict
        Rows: 2
        [{'bob alice': 0.5, 'alice bob': 1}, {'dog cat': 5, 'a dog': 5}]

        # Run count_ngrams with list input
        >>> sa = turicreate.SArray([['one', 'bar bah'], ['a dog', 'a dog cat']])
        >>> turicreate.text_analytics.count_ngrams(sa)
        dtype: dict
        Rows: 2
        [{'bar bah': 1}, {'dog cat': 1, 'a dog': 2}]
    """

    _raise_error_if_not_sarray(sa, "sa")

    ## Compute word counts
    sf = _turicreate.SFrame({'docs': sa})
    fe = _feature_engineering.NGramCounter(features='docs',
                                                    n=n,
                                                    method=method,
                                                    to_lower=to_lower,
                                                    delimiters=delimiters,
                                                    ignore_punct=ignore_punct,
                                                    ignore_space=ignore_space,
                                                    output_column_prefix=None)
    output_sf = fe.fit_transform(sf)

    return output_sf['docs']

def tf_idf(dataset):
    """
    Compute the TF-IDF scores for each word in each document. The collection
    of documents must be in bag-of-words format.

    .. math::
        \mbox{TF-IDF}(w, d) = tf(w, d) * log(N / f(w))

    where :math:`tf(w, d)` is the number of times word :math:`w` appeared in
    document :math:`d`, :math:`f(w)` is the number of documents word :math:`w`
    appeared in, :math:`N` is the number of documents, and we use the
    natural logarithm.

    This function is implemented using

    Parameters
    ----------
    dataset : SArray[str | dict | list]
        Input text data. 

    Returns
    -------
    out : SArray[dict]
        The same document corpus where each score has been replaced by the
        TF-IDF transformation.

    See Also
    --------
    count_words, count_ngrams, tokenize,

    References
    ----------
    - `Wikipedia - TF-IDF <https://en.wikipedia.org/wiki/TFIDF>`_

    Examples
    --------
    .. sourcecode:: python

        >>> import turicreate

        >>> docs = turicreate.SArray('https://static.turi.com/datasets/nips-text')
        >>> docs_tfidf = turicreate.text_analytics.tf_idf(docs)
    """
    _raise_error_if_not_sarray(dataset, "dataset")

    if len(dataset) == 0:
        return _turicreate.SArray()

    dataset = _turicreate.SFrame({'docs': dataset})
    scores = _feature_engineering.TFIDF('docs').fit_transform(dataset)

    return scores['docs']

def trim_rare_words(sa, threshold=2, to_lower=True,
                delimiters=["\r", "\v", "\n", "\f", "\t", " "], stopwords=None):
    '''
    Remove words that occur below a certain number of times in an SArray.
    This is a common method of cleaning text before it is used, and can increase the
    quality and explainability of the models learned on the transformed data.

    RareWordTrimmer can be applied to all the string-, dictionary-, and list-typed
    columns in an SArray.

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
    sa: SArray[str | dict | list]
        The input text data.

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

    Returns
    -------
    out : SArray.
        An SArray with words below a threshold removed.

    See Also
    --------
    count_ngrams, tf_idf, tokenize,

    References
    ----------
    - `Penn treebank tokenization <https://web.archive.org/web/19970614072242/http://www.cis.upenn.edu:80/~treebank/tokenization.html>`_

    Examples
    --------
    .. sourcecode:: python

        >>> import turicreate

        # Create input data
        >>> sa = turicreate.SArray(["The quick brown fox jumps in a fox like way.",
        ...                       "Word word WORD, word!!!word"])

        # Run trim_rare_words
        >>> turicreate.text_analytics.trim_rare_words(sa)
        dtype: str
        Rows: 2
        ['fox fox', 'word word']

        # Run trim_rare_words with Penn treebank style tokenization to handle
        # punctuations
        >>> turicreate.text_analytics.trim_rare_words(sa, delimiters=None)
        dtype: str
        Rows: 2
        ['fox fox', 'word word word']

        # Run trim_rare_words with dictionary input
        >>> sa = turicreate.SArray([{'alice bob': 1, 'Bob alice': 2},
        ...                       {'a dog': 0, 'a dog cat': 5}])
        >>> turicreate.text_analytics.trim_rare_words(sa)
        dtype: dict
        Rows: 2
        [{'bob alice': 2}, {'a dog cat': 5}]

        # Run trim_rare_words with list input
        >>> sa = turicreate.SArray([['one', 'bar bah', 'One'],
        ...                     ['a dog', 'a dog cat', 'A DOG']])
        >>> turicreate.text_analytics.trim_rare_words(sa)
        dtype: list
        Rows: 2
        [['one', 'one'], ['a dog', 'a dog']]


'''

    _raise_error_if_not_sarray(sa, "sa")

    ## Compute word counts
    sf = _turicreate.SFrame({'docs': sa})
    fe = _feature_engineering.RareWordTrimmer(features='docs',
                                                 threshold=threshold,
                                                 to_lower=to_lower,
                                                 delimiters=delimiters,
                                                 stopwords=stopwords,
                                                 output_column_prefix=None)
    tokens = fe.fit_transform(sf)

    return tokens['docs']

def tokenize(sa, to_lower=False,
                delimiters=["\r", "\v", "\n", "\f", "\t", " "]):
    """
    tokenize(sa, to_lower=False, delimiters=["\\\\r", "\\\\v", "\\\\n", "\\\\f", "\\\\t", " "])

    Tokenize the input SArray of text strings and return the list of tokens.

    Parameters
    ----------
    sa : SArray[str]
        Input data of strings representing English text. This tokenizer is not
        intended to process XML, HTML, or other structured text formats.

    to_lower : bool, optional
        If True, all strings are converted to lower case before tokenization.

    delimiters : list[str], None, optional
        Input strings are tokenized using delimiter characters in this list.
        Each entry in this list must contain a single character. If set to
        `None`, then a Penn treebank-style tokenization is used, which contains
        smart handling of punctuations.

    Returns
    -------
    out : SArray[list]
        Each text string in the input is mapped to a list of tokens.

    See Also
    --------
    count_words, count_ngrams, tf_idf

    References
    ----------
    - `Penn treebank tokenization <https://web.archive.org/web/19970614072242/http://www.cis.upenn.edu:80/~treebank/tokenization.html>`_

    Examples
    --------
    .. sourcecode:: python

        >>> import turicreate

        >>> docs = turicreate.SArray(['This is the first sentence.',
        ...                         "This one, it's the second sentence."])

        # Default tokenization by space characters
        >>> turicreate.text_analytics.tokenize(docs)
        dtype: list
        Rows: 2
        [['This', 'is', 'the', 'first', 'sentence.'],
         ['This', 'one,', "it's", 'the', 'second', 'sentence.']]

        # Penn treebank-style tokenization
        >>> turicreate.text_analytics.tokenize(docs, delimiters=None)
        dtype: list
        Rows: 2
        [['This', 'is', 'the', 'first', 'sentence', '.'],
         ['This', 'one', ',', 'it', "'s", 'the', 'second', 'sentence', '.']]

    """
    _raise_error_if_not_sarray(sa, "sa")

    ## Compute word counts
    sf = _turicreate.SFrame({'docs': sa})
    fe = _feature_engineering.Tokenizer(features='docs',
                                                 to_lower=to_lower,
                                                 delimiters=delimiters,
                                                 output_column_prefix=None)
    tokens = fe.fit_transform(sf)

    return tokens['docs']

def bm25(dataset, query, k1=1.5, b=.75):
    """
    For a given query and set of documents, compute the BM25 score for each
    document. If we have a query with words q_1, ..., q_n the BM25 score for
    a document is:

        .. math:: \sum_{i=1}^N IDF(q_i)\\frac{f(q_i) * (k_1+1)}{f(q_i) + k_1 * (1-b+b*|D|/d_avg))}

    where

    * :math:`\mbox{IDF}(q_i) = log((N - n(q_i) + .5)/(n(q_i) + .5)`
    * :math:`f(q_i)` is the number of times q_i occurs in the document
    * :math:`n(q_i)` is the number of documents containing q_i
    * :math:`|D|` is the number of words in the document
    * :math:`d_avg` is the average number of words per document in the corpus
    * :math:`k_1` and :math:`b` are free parameters.

    Parameters
    ----------
    dataset : SArray of type dict, list, or str
        An SArray where each element either represents a document in:

        * **dict** : a bag-of-words format, where each key is a word and each
          value is the number of times that word occurs in the document.

        * **list** : The list is converted to bag of words of format, where the
          keys are the unique elements in the list and the values are the counts
          of those unique elements. After this step, the behaviour is identical
          to dict.

        * **string** : Behaves identically to a **dict**, where the dictionary
          is generated by converting the string into a bag-of-words format.
          For example, 'I really like really fluffy dogs" would get converted
          to {'I' : 1, 'really': 2, 'like': 1, 'fluffy': 1, 'dogs':1}.

    query : A list, set, or SArray of type str
        A list, set or SArray where each element is a word.

    k1 : float, optional
        Free parameter which controls the relative importance of term
        frequencies. Recommended values are [1.2, 2.0].

    b : float, optional
        Free parameter which controls how much to downweight scores for long
        documents. Recommended value is 0.75.

    Returns
    -------
    out : SFrame
        An SFrame containing the BM25 score for each document containing one of
        the query words. The doc_id column is the row number of the document.

    Examples
    --------
    .. sourcecode:: python

        >>> import turicreate

        >>> dataset = turicreate.SArray([
          {'a':5, 'b':7, 'c':10},
          {'a':3, 'c':1, 'd':2},
          {'a':10, 'b':3, 'e':5},
          {'a':1},
          {'f':5}])

        >>> query = ['a', 'b', 'c']
        >>> turicreate.text_analytics.bm25(dataset, query)


    References
    ----------
    .. [BM25] `"Okapi BM-25" <http://en.wikipedia.org/wiki/Okapi_BM25>`_
    """

    if type(dataset) != _turicreate.SArray:
        raise TypeError('bm25 requires an SArray of dict, list, or str type'+\
            ', where each dictionary whose keys are words and whose values' + \
            ' are word frequency.')
    sf = _SFrame({'docs' : dataset})

    if type(query) is dict:  # For backwards compatibility
        query = list(query.keys())
    if type(query) is _turicreate.SArray:
        query = list(query)
    if type(query) is set:
        query = list(query)
    if type(query) is not list:
        raise TypeError('The query must either be an SArray of str type, '+\
           ' a list of strings, or a set of strings.')

    # Calculate BM25
    sf = sf.add_row_number('doc_id')
    sf = sf.dropna('docs') # Drop missing documents
    scores = _feature_engineering.BM25('docs',query, k1, b, output_column_name = 'bm25').fit_transform(sf)

    # Find documents with query words

    if scores['docs'].dtype is dict:
        scores['doc_terms'] = scores['docs'].dict_keys()
    elif scores['docs'].dtype is list:
        scores['doc_terms'] = scores['docs'].apply(lambda x: list(set(x)))
    elif scores['docs'].dtype is str:
        scores['doc_terms'] = count_words(scores['docs']).dict_keys()
    else:
        # This should never occur (handled by BM25)
        raise TypeError('bm25 requires an SArray of dict, list, or str type')
    scores['doc_counts'] = scores['doc_terms'].apply(lambda x: len([word for word in query if word in x]))
    scores = scores[scores['doc_counts'] > 0] # Drop documents without query word
    scores = scores.select_columns(['doc_id','bm25'])

    return scores


def parse_sparse(filename, vocab_filename):
    """
    Parse a file that's in libSVM format. In libSVM format each line of the text
    file represents a document in bag of words format:

    num_unique_words_in_doc word_id:count another_id:count

    The word_ids have 0-based indexing, i.e. 0 corresponds to the first
    word in the vocab filename.

    Parameters
    ----------
    filename : str
        The name of the file to parse.

    vocab_filename : str
        A list of words that are used for this data set.

    Returns
    -------
    out : SArray
        Each element represents a document in bag-of-words format.

    Examples
    --------
    If we have two documents:
    1. "It was the best of times, it was the worst of times"
    2. "It was the age of wisdom, it was the age of foolishness"

    Then the vocabulary file might contain the unique words, with a word
    on each line, in the following order:
    it, was, the, best, of, times, worst, age, wisdom, foolishness

    In this case, the file in libSVM format would have two lines:
    7 0:2 1:2 2:2 3:1 4:2 5:1 6:1
    7 0:2 1:2 2:2 7:2 8:1 9:1 10:1

    The following command will parse the above two files into an SArray
    of type dict.

    >>> file = 'https://static.turi.com/datasets/text/ap.dat'
    >>> vocab = 'https://static.turi.com/datasets/text/ap.vocab.txt'
    >>> docs = turicreate.text_analytics.parse_sparse(file, vocab)
    """
    vocab = _turicreate.SFrame.read_csv(vocab_filename, header=None)['X1']
    vocab = list(vocab)

    docs = _turicreate.SFrame.read_csv(filename, header=None)

    # Remove first word
    docs = docs['X1'].apply(lambda x: x.split(' ')[1:])

    # Helper function that checks whether we get too large a word id
    def get_word(word_id):
        assert int(word_id) < len(vocab), \
                "Text data contains integers that are larger than the \
                 size of the provided vocabulary."
        return vocab[word_id]

    def make_dict(pairs):
        pairs = [z.split(':') for z in pairs]
        ret = {}
        for k, v in pairs:
            ret[get_word(int(k))] = int(v)
        return ret

    # Split word_id and count and make into a dictionary
    docs = docs.apply(lambda x: make_dict(x))

    return docs


def parse_docword(filename, vocab_filename):
    """
    Parse a file that's in "docword" format. This consists of a 3-line header
    comprised of the document count, the vocabulary count, and the number of
    tokens, i.e. unique (doc_id, word_id) pairs. After the header, each line
    contains a space-separated triple of (doc_id, word_id, frequency), where
    frequency is the number of times word_id occurred in document doc_id.

    This format assumes that documents and words are identified by a positive
    integer (whose lowest value is 1).
    Thus, the first word in the vocabulary file has word_id=1.

    2
    272
    5
    1 5 1
    1 105 3
    1 272 5
    2 1 3
    ...

    Parameters
    ----------
    filename : str
        The name of the file to parse.

    vocab_filename : str
        A list of words that are used for this data set.

    Returns
    -------
    out : SArray
        Each element represents a document in bag-of-words format.

    Examples
    --------
    >>> textfile = 'https://static.turi.com/datasets/text/docword.nips.txt')
    >>> vocab = 'https://static.turi.com/datasets/text/vocab.nips.txt')
    >>> docs = turicreate.text_analytics.parse_docword(textfile, vocab)
    """
    vocab = _turicreate.SFrame.read_csv(vocab_filename, header=None)['X1']
    vocab = list(vocab)

    sf = _turicreate.SFrame.read_csv(filename, header=False)
    sf = sf[3:]
    sf['X2'] = sf['X1'].apply(lambda x: [int(z) for z in x.split(' ')])
    del sf['X1']
    sf = sf.unpack('X2', column_name_prefix='', column_types=[int,int,int])
    docs = sf.unstack(['1', '2'], 'bow').sort('0')['bow']
    docs = docs.apply(lambda x: {vocab[k-1]:v for (k, v) in six.iteritems(x)})

    return docs


def random_split(dataset, prob=.5):
    """
    Utility for performing a random split for text data that is already in
    bag-of-words format. For each (word, count) pair in a particular element,
    the counts are uniformly partitioned in either a training set or a test
    set.

    Parameters
    ----------
    dataset : SArray of type dict, SFrame with columns of type dict
        A data set in bag-of-words format.

    prob : float, optional
        Probability for sampling a word to be placed in the test set.

    Returns
    -------
    train, test : SArray
        Two data sets in bag-of-words format, where the combined counts are
        equal to the counts in the original data set.

    Examples
    --------
    >>> docs = turicreate.SArray([{'are':5, 'you':3, 'not': 1, 'entertained':10}])
    >>> train, test = turicreate.text_analytics.random_split(docs)
    >>> print train
    [{'not': 1.0, 'you': 3.0, 'are': 3.0, 'entertained': 7.0}]
    >>> print test
    [{'are': 2.0, 'entertained': 3.0}]
    """

    def grab_values(x, train=True):
        if train:
            ix = 0
        else:
            ix = 1
        return dict([(key, value[ix]) for key, value in six.iteritems(x) \
                    if value[ix] != 0])

    def word_count_split(n, p):
        num_in_test = 0
        for i in range(n):
            if random.random() < p:
                num_in_test += 1
        return [n - num_in_test, num_in_test]

    # Get an SArray where each word has a 2 element list containing
    # the count that will be for the training set and the count that will
    # be assigned to the test set.
    data = dataset.apply(lambda x: dict([(key, word_count_split(int(value), prob)) \
                                for key, value in six.iteritems(x)]))

    # Materialize the data set
    data.__materialize__()

    # Grab respective counts for each data set
    train = data.apply(lambda x: grab_values(x, train=True))
    test  = data.apply(lambda x: grab_values(x, train=False))

    return train, test


def stopwords(lang='en'):
    """
    Get common words that are often removed during preprocessing of text data,
    i.e. "stopwords". Currently only English stop words are provided.

    Parameters
    ----------
    lang : str, optional
        The desired language. Default: 'en' (English).

    Returns
    -------
    out : set
        A set of strings.

    Examples
    --------
    You may remove stopwords from an SArray as follows:

    >>> docs = turicreate.SArray([{'are': 1, 'you': 1, 'not': 1, 'entertained':1}])
    >>> docs.dict_trim_by_keys(turicreate.text_analytics.stopwords(), True)
    dtype: dict
    Rows: 1
    [{'entertained': 1}]
    """
    if lang=='en' or lang=='english':
        return set(['a', 'able', 'about', 'above', 'according', 'accordingly', 'across', 'actually', 'after', 'afterwards', 'again', 'against', 'all', 'allow', 'allows', 'almost', 'alone', 'along', 'already', 'also', 'although', 'always', 'am', 'among', 'amongst', 'an', 'and', 'another', 'any', 'anybody', 'anyhow', 'anyone', 'anything', 'anyway', 'anyways', 'anywhere', 'apart', 'appear', 'appreciate', 'appropriate', 'are', 'around', 'as', 'aside', 'ask', 'asking', 'associated', 'at', 'available', 'away', 'awfully', 'b', 'be', 'became', 'because', 'become', 'becomes', 'becoming', 'been', 'before', 'beforehand', 'behind', 'being', 'believe', 'below', 'beside', 'besides', 'best', 'better', 'between', 'beyond', 'both', 'brief', 'but', 'by', 'c', 'came', 'can', 'cannot', 'cant', 'cause', 'causes', 'certain', 'certainly', 'changes', 'clearly', 'co', 'com', 'come', 'comes', 'concerning', 'consequently', 'consider', 'considering', 'contain', 'containing', 'contains', 'corresponding', 'could', 'course', 'currently', 'd', 'definitely', 'described', 'despite', 'did', 'different', 'do', 'does', 'doing', 'done', 'down', 'downwards', 'during', 'e', 'each', 'edu', 'eg', 'eight', 'either', 'else', 'elsewhere', 'enough', 'entirely', 'especially', 'et', 'etc', 'even', 'ever', 'every', 'everybody', 'everyone', 'everything', 'everywhere', 'ex', 'exactly', 'example', 'except', 'f', 'far', 'few', 'fifth', 'first', 'five', 'followed', 'following', 'follows', 'for', 'former', 'formerly', 'forth', 'four', 'from', 'further', 'furthermore', 'g', 'get', 'gets', 'getting', 'given', 'gives', 'go', 'goes', 'going', 'gone', 'got', 'gotten', 'greetings', 'h', 'had', 'happens', 'hardly', 'has', 'have', 'having', 'he', 'hello', 'help', 'hence', 'her', 'here', 'hereafter', 'hereby', 'herein', 'hereupon', 'hers', 'herself', 'hi', 'him', 'himself', 'his', 'hither', 'hopefully', 'how', 'howbeit', 'however', 'i', 'ie', 'if', 'ignored', 'immediate', 'in', 'inasmuch', 'inc', 'indeed', 'indicate', 'indicated', 'indicates', 'inner', 'insofar', 'instead', 'into', 'inward', 'is', 'it', 'its', 'itself', 'j', 'just', 'k', 'keep', 'keeps', 'kept', 'know', 'knows', 'known', 'l', 'last', 'lately', 'later', 'latter', 'latterly', 'least', 'less', 'lest', 'let', 'like', 'liked', 'likely', 'little', 'look', 'looking', 'looks', 'ltd', 'm', 'mainly', 'many', 'may', 'maybe', 'me', 'mean', 'meanwhile', 'merely', 'might', 'more', 'moreover', 'most', 'mostly', 'much', 'must', 'my', 'myself', 'n', 'name', 'namely', 'nd', 'near', 'nearly', 'necessary', 'need', 'needs', 'neither', 'never', 'nevertheless', 'new', 'next', 'nine', 'no', 'nobody', 'non', 'none', 'noone', 'nor', 'normally', 'not', 'nothing', 'novel', 'now', 'nowhere', 'o', 'obviously', 'of', 'off', 'often', 'oh', 'ok', 'okay', 'old', 'on', 'once', 'one', 'ones', 'only', 'onto', 'or', 'other', 'others', 'otherwise', 'ought', 'our', 'ours', 'ourselves', 'out', 'outside', 'over', 'overall', 'own', 'p', 'particular', 'particularly', 'per', 'perhaps', 'placed', 'please', 'plus', 'possible', 'presumably', 'probably', 'provides', 'q', 'que', 'quite', 'qv', 'r', 'rather', 'rd', 're', 'really', 'reasonably', 'regarding', 'regardless', 'regards', 'relatively', 'respectively', 'right', 's', 'said', 'same', 'saw', 'say', 'saying', 'says', 'second', 'secondly', 'see', 'seeing', 'seem', 'seemed', 'seeming', 'seems', 'seen', 'self', 'selves', 'sensible', 'sent', 'serious', 'seriously', 'seven', 'several', 'shall', 'she', 'should', 'since', 'six', 'so', 'some', 'somebody', 'somehow', 'someone', 'something', 'sometime', 'sometimes', 'somewhat', 'somewhere', 'soon', 'sorry', 'specified', 'specify', 'specifying', 'still', 'sub', 'such', 'sup', 'sure', 't', 'take', 'taken', 'tell', 'tends', 'th', 'than', 'thank', 'thanks', 'thanx', 'that', 'thats', 'the', 'their', 'theirs', 'them', 'themselves', 'then', 'thence', 'there', 'thereafter', 'thereby', 'therefore', 'therein', 'theres', 'thereupon', 'these', 'they', 'think', 'third', 'this', 'thorough', 'thoroughly', 'those', 'though', 'three', 'through', 'throughout', 'thru', 'thus', 'to', 'together', 'too', 'took', 'toward', 'towards', 'tried', 'tries', 'truly', 'try', 'trying', 'twice', 'two', 'u', 'un', 'under', 'unfortunately', 'unless', 'unlikely', 'until', 'unto', 'up', 'upon', 'us', 'use', 'used', 'useful', 'uses', 'using', 'usually', 'uucp', 'v', 'value', 'various', 'very', 'via', 'viz', 'vs', 'w', 'want', 'wants', 'was', 'way', 'we', 'welcome', 'well', 'went', 'were', 'what', 'whatever', 'when', 'whence', 'whenever', 'where', 'whereafter', 'whereas', 'whereby', 'wherein', 'whereupon', 'wherever', 'whether', 'which', 'while', 'whither', 'who', 'whoever', 'whole', 'whom', 'whose', 'why', 'will', 'willing', 'wish', 'with', 'within', 'without', 'wonder', 'would', 'would', 'x', 'y', 'yes', 'yet', 'you', 'your', 'yours', 'yourself', 'yourselves', 'z', 'zero'])
    else:
        raise NotImplementedError('Only English stopwords are currently available.')



def _check_input(dataset):
    if isinstance(dataset, _SFrame):
        assert dataset.num_columns() == 1, \
        "The provided SFrame contains more than one column. It should have " +\
        "only one column of type dict."
        colname = dataset.column_names()[0]
        dataset = dataset[colname]

    assert isinstance(dataset, _SArray), \
    "Provided data must be an SArray."

    assert dataset.dtype == dict, \
    "Provided data must be of type dict, representing the documents in " + \
    "bag-of-words format. Please consult the documentation."

    return dataset
